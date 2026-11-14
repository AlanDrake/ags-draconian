#!/usr/bin/env python3
"""
Checks the direction of the #include lines between the rings of the engine (docs/refactor/PLAN.md, 5.3).

    python Script/check_layers.py             # exit 1 on an include that is not allowed and not in the baseline
    python Script/check_layers.py --list      # the baseline grouped by ring pair and by included header
    python Script/check_layers.py --update    # rewrite the baseline from the tree (review the diff)

The rings, each a set of directories, and what a file of a ring may include:

    common       Common/                                  common
    base         Engine/util, Engine/debug                common, base
    host         Engine/platform, device, gfx, media,     common, base, host
                 resource
    vm           Engine/script                            common, base, vm
    game         Engine/game                              common, base, host, vm, game
    composition  Engine/main, Engine/plugin               everything
    test         Engine/test, Common/test                 everything

So the game logic is a module on top of a host (window, graphics, audio, files) and a script VM that know
nothing about it, and Engine/main puts them together. Two rules on single headers come with the rings (HEADER_RULES):
script/script_api.h, the text formatting of the script bindings, is for the VM and Engine/game/script_api only.

The tree does not satisfy the rules yet. The includes that break them today are listed in
Script/check_layers_baseline.txt, one "includer<TAB>included" per line. The check fails on a violation that is
not in the list, and also on a line of the list that is no longer a violation, so the list can only get
shorter: whoever fixes an include removes its line (or runs --update) in the same commit.

An include is resolved as the compilers do: the includer's own directory, then the include roots (Engine/ for a
file of the engine, then Common/; Common/ alone for a file of Common, which therefore reaches the engine
only through a relative path). Third-party code (libsrc, libinclude) and system headers are not looked at;
the tools, the compiler and the editor are outside the rings (they include Common only).
"""
import argparse
import collections
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
BASELINE = os.path.join(REPO, 'Script', 'check_layers_baseline.txt')

# Longest prefix wins
RINGS = [
    ('Common/test/', 'test'),
    ('Common/', 'common'),
    ('Engine/test/', 'test'),
    ('Engine/util/', 'base'),
    ('Engine/debug/', 'base'),
    ('Engine/platform/', 'host'),
    ('Engine/device/', 'host'),
    ('Engine/gfx/', 'host'),
    ('Engine/media/', 'host'),
    ('Engine/resource/', 'host'),
    ('Engine/script/', 'vm'),
    ('Engine/game/', 'game'),
    ('Engine/main/', 'composition'),
    ('Engine/plugin/', 'composition'),
]
EVERYTHING = {'common', 'base', 'host', 'vm', 'game', 'composition', 'test'}
ALLOWED = {
    'common': {'common'},
    'base': {'common', 'base'},
    'host': {'common', 'base', 'host'},
    'vm': {'common', 'base', 'vm'},
    'game': {'common', 'base', 'host', 'vm', 'game'},
    'composition': EVERYTHING - {'test'},
    'test': EVERYTHING,
}
# A header that only some directories may include: header -> prefixes of the files that may (tests always may)
HEADER_RULES = {
    'Engine/script/script_api.h': ('Engine/script/', 'Engine/game/script_api/'),
}
SOURCES = ('.cpp', '.h', '.inl', '.c', '.cc', '.mm', '.m')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude)/')
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')


def ring_of(path):
    best = None
    for prefix, ring in RINGS:
        if path.startswith(prefix) and (best is None or len(prefix) > len(best[0])):
            best = (prefix, ring)
    return best[1] if best else None


def tracked_sources(repo):
    out = subprocess.run(['git', '-C', repo, 'ls-files', 'Engine', 'Common'], capture_output=True, text=True,
                         check=True).stdout
    return sorted(f for f in out.splitlines() if f.endswith(SOURCES) and not THIRD_PARTY.search(f))


def violations(repo):
    """Returns ({(includer, included): reason}, number of includes looked at)."""
    files = tracked_sources(repo)
    known = set(files)
    roots = {'Engine': ('Engine/', 'Common/'), 'Common': ('Common/',)}
    found = {}
    count = 0
    for path in files:
        ring = ring_of(path)
        if ring is None:
            sys.exit('check_layers: %s is in no ring: add its directory to RINGS' % path)
        with open(os.path.join(repo, path), encoding='utf-8', errors='replace') as fh:
            for line in fh:
                m = INCLUDE.match(line)
                if not m:
                    continue
                name = m.group(1)
                local = os.path.normpath(os.path.join(os.path.dirname(path), name)).replace('\\', '/')
                target = local if local in known else next(
                    (root + name for root in roots[path.split('/')[0]] if root + name in known), None)
                if target is None:
                    continue  # a system or third-party header
                count += 1
                other = ring_of(target)
                if other not in ALLOWED[ring]:
                    found[(path, target)] = '%s includes %s' % (ring, other)
                rule = HEADER_RULES.get(target)
                if rule and ring != 'test' and not path.startswith(rule):
                    found[(path, target)] = 'only %s may include it' % ' and '.join(rule)
    return found, count


def read_baseline():
    if not os.path.isfile(BASELINE):
        return set()
    with open(BASELINE, encoding='utf-8') as fh:
        return {tuple(line.rstrip('\n').split('\t')) for line in fh if line.strip() and not line.startswith('#')}


def write_baseline(found):
    with open(BASELINE, 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('# The includes that break the ring rules of Script/check_layers.py today: includer<TAB>included.\n')
        fh.write('# A new line here needs a reason in its commit message; a fixed include takes its line away.\n')
        for a, b in sorted(found):
            fh.write('%s\t%s\n' % (a, b))


def print_grouped(found):
    groups = collections.defaultdict(list)
    for pair, reason in found.items():
        groups[reason].append(pair)
    for reason in sorted(groups):
        pairs = groups[reason]
        print('%s: %d includes in %d files' % (reason, len(pairs), len({a for a, b in pairs})))
        for header, n in collections.Counter(b for a, b in pairs).most_common():
            print('  %3d  %s' % (n, header))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--repo', default=REPO, help='checkout to look at (default: this one); the baseline is this one\'s')
    ap.add_argument('--list', action='store_true', help='print the violations grouped, and stop')
    ap.add_argument('--update', action='store_true', help='rewrite the baseline from the tree')
    args = ap.parse_args()

    found, count = violations(os.path.abspath(args.repo))
    if args.list:
        print_grouped(found)
        return 0
    if args.update:
        write_baseline(found)
        print('check_layers: baseline rewritten, %d known violations' % len(found))
        return 0
    baseline = read_baseline()
    new = sorted(set(found) - baseline)
    fixed = sorted(baseline - set(found))
    for a, b in new:
        print('%s includes %s: %s' % (a, b, found[(a, b)]))
    for a, b in fixed:
        print('no longer a violation, remove it from %s: %s\t%s' % (os.path.relpath(BASELINE, REPO), a, b))
    if new:
        print('An include against the rings: see the table at the top of Script/check_layers.py. Move the '
              'declaration to the ring that owns it, or pass what is needed as a parameter.')
    print('check_layers: %d includes, %d known violations in the baseline, %d new, %d fixed and still listed'
          % (count, len(baseline & set(found)), len(new), len(fixed)))
    return 1 if new or fixed else 0


if __name__ == '__main__':
    sys.exit(main())
