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

An `extern` declaration in a source file of the engine reaches a global without an include, which this check
cannot see, so there is none (5.1 step 1 took out the 552 there were): a global is declared in a header, once,
and the file that uses it includes the header. `extern "C"` blocks are not that and pass.

Nor is there a new global (5.1 step 3): an `extern` variable declaration in a header of the engine must name a
variable listed in EXTERN_ALLOWED, with the reason it stays a global (a constant, a stateless manager, the script
VM's own state, the crash handler's, ...). A new value that lives as long as the process is a member of
EngineServices (platform/base/engine_services.h), one of a play-through a member of Game (game/game_context.h),
one of the loaded room a member of Room (game/world/room_context.h); a cache private to a module is a `static`
of its .cpp, reached through the module's functions. The check also fails on an entry of EXTERN_ALLOWED that no
header declares any more, so the table only gets shorter. Function declarations and `extern "C"` do not count.
`--self-test` runs the declaration parser and both failures on made-up headers.

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
EXTERN = re.compile(r'^\s*extern\s+(?!"C")')

# The globals that an engine header may still declare `extern`: name -> why it is not a member of a context.
_CONSTANT = 'a constant'
_MANAGER = 'a stateless manager of script objects: no data member, registered by address with the managed pool'
_VM = ('the script VM\'s own state: Engine/script is beside the host ring and below the game ring, so it can '
       'reach neither EngineServices nor Game (one pool per Game is PLAN.md 5.11\'s)')
_CRASH = 'crash handler state, read while the process dies'
EXTERN_ALLOWED = {
    # Constants
    'turnlooporder': _CONSTANT,
    'sccmd_info': _CONSTANT,
    'UserSavedgamesRootToken': _CONSTANT,
    'GameSavedgamesDirToken': _CONSTANT,
    'GameDataDirToken': _CONSTANT,
    'DefaultConfigFileName': _CONSTANT,
    'ScriptEventCb': _CONSTANT,
    'RBSwitchNames': _CONSTANT,
    # Stateless managers
    'ccDynamicAudioClip': _MANAGER,
    'ccDynamicAudio': _MANAGER,
    'ccDynamicCharacter': _MANAGER,
    'ccDynamicInv': _MANAGER,
    'ccDynamicGUI': _MANAGER,
    'ccDynamicDialog': _MANAGER,
    'ccDynamicGUIControl': _MANAGER,
    'ccDynamicGUIButton': _MANAGER,
    'ccDynamicGUIInvWindow': _MANAGER,
    'ccDynamicGUILabel': _MANAGER,
    'ccDynamicGUIListBox': _MANAGER,
    'ccDynamicGUISlider': _MANAGER,
    'ccDynamicGUITextBox': _MANAGER,
    'ccDynamicTextWindowGUI': _MANAGER,
    'ccDynamicHotspot': _MANAGER,
    'ccDynamicObject': _MANAGER,
    'ccDynamicRegion': _MANAGER,
    'ccDynamicWalkarea': _MANAGER,
    'ccDynamicWalkbehind': _MANAGER,
    'GameStaticManager': _MANAGER,
    'GlobalStaticManager': _MANAGER,
    'globalDynamicArray': _MANAGER,
    'globalDynamicStruct': _MANAGER,
    'myScriptStringImpl': _MANAGER,
    # The script ring's own state
    'scriptExecutor': _VM,
    'simp': _VM,
    'simp_for_plugin': _VM,
    'new_line_hook': _VM,
    'pool': _VM,
    # Crash handler state
    'excinfo': _CRASH,
    'miniDumpResultCode': _CRASH,
    'eip_guinum': _CRASH,
    'eip_guiobj': _CRASH,
    # Decided by their users' rings
    'pluginReaders': ('the plugins\' object readers: a vector of PluginObjectReader, a type of the composition ring, '
                      'which EngineServices (host ring) cannot hold since its .cpp cannot see the complete type'),
    # Until the batch `sprites` of 5.1 step 3
    'spriteset': 'the sprite cache, until it becomes EngineServices::Sprites',
}


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


def _strip_nested(text, open_ch, close_ch):
    """Removes the innermost open_ch...close_ch groups until none is left."""
    pattern = re.compile('%s[^%s%s]*%s' % (re.escape(open_ch), re.escape(open_ch), re.escape(close_ch),
                                            re.escape(close_ch)))
    while True:
        text, n = pattern.subn(' ', text)
        if not n:
            return text


def extern_variables(lines):
    """The variables declared `extern` in a header, as (line number, name): a declaration may span lines and
    name several variables (`extern int a, b;`); function declarations and `extern "C"` are left out, a pointer
    to a function (`extern void (*hook)(int);`) is a variable."""
    found = []
    number = 0
    while number < len(lines):
        line = lines[number]
        number += 1
        if not EXTERN.match(line):
            continue
        first = number
        statement = line.split('//')[0]
        while ';' not in statement and number < len(lines):
            statement += ' ' + lines[number].split('//')[0]
            number += 1
        statement = re.sub(r'/\*.*?\*/', ' ', statement.split(';')[0], flags=re.S)
        statement = re.sub(r'^\s*extern\s+', '', statement).split('=')[0]
        statement = _strip_nested(_strip_nested(statement, '[', ']'), '<', '>')
        pointer = re.search(r'\(\s*\*\s*(\w+)\s*\)', statement)
        if pointer:
            found.append((first, pointer.group(1)))
            continue
        if '(' in statement:
            continue  # a function
        for declarator in statement.split(','):
            m = re.search(r'(\w+)\s*$', declarator)
            if m:
                found.append((first, m.group(1)))
    return found


def check_header_externs(declared, allowed):
    """declared: [(path, line number, name)]. Returns (the declarations whose name is not allowed, the allowed
    names that nothing declares)."""
    unlisted = [d for d in declared if d[2] not in allowed]
    stale = sorted(set(allowed) - {name for _, _, name in declared})
    return unlisted, stale


def self_test():
    header = ['#pragma once\n',
              'extern "C" void c_function();\n',
              'extern void needless(int a, int b);\n',
              'extern int eip_a, eip_b; // two names\n',
              'extern const std::array<const char *, 3> Names;\n',
              'extern std::unique_ptr<A::B<C, D>> owner;\n',
              'extern const Info table[COUNT];\n',
              'extern void (*hook)(int line);\n',
              'extern struct _RECORD\n',
              '    record;\n',
              '  extern int indented;\n',
              'int not_extern;\n']
    got = extern_variables(header)
    want = [(4, 'eip_a'), (4, 'eip_b'), (5, 'Names'), (6, 'owner'), (7, 'table'), (8, 'hook'), (9, 'record'),
            (11, 'indented')]
    assert got == want, got
    declared = [('Engine/x.h', n, name) for n, name in got]
    allowed = {name: 'reason' for _, name in want}
    assert check_header_externs(declared, allowed) == ([], [])
    # a new global fails
    declared.append(('Engine/y.h', 3, 'new_global'))
    assert check_header_externs(declared, allowed) == ([('Engine/y.h', 3, 'new_global')], [])
    # an entry that nothing declares any more fails
    allowed['gone'] = 'reason'
    assert check_header_externs(declared[:-1], allowed) == ([], ['gone'])
    print('check_layers: self-test passed')
    return 0


def violations(repo):
    """Returns ({(includer, included): reason}, number of includes looked at, the extern declarations in the
    engine's source files as (path, line number, text), the variables declared extern in the engine's headers
    as (path, line number, name))."""
    files = tracked_sources(repo)
    known = set(files)
    roots = {'Engine': ('Engine/', 'Common/'), 'Common': ('Common/',)}
    found = {}
    externs = []
    header_externs = []
    count = 0
    for path in files:
        ring = ring_of(path)
        if ring is None:
            sys.exit('check_layers: %s is in no ring: add its directory to RINGS' % path)
        is_engine_source = path.startswith('Engine/') and not path.endswith(('.h', '.inl'))
        with open(os.path.join(repo, path), encoding='utf-8', errors='replace') as fh:
            lines = fh.readlines()
            if path.startswith('Engine/') and path.endswith(('.h', '.inl')):
                header_externs += [(path, n, name) for n, name in extern_variables(lines)]
            for number, line in enumerate(lines, 1):
                if is_engine_source and EXTERN.match(line):
                    externs.append((path, number, line.strip()))
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
    return found, count, externs, header_externs


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
    ap.add_argument('--self-test', action='store_true', help='test the extern parser and the EXTERN_ALLOWED check')
    args = ap.parse_args()
    if args.self_test:
        return self_test()

    found, count, externs, header_externs = violations(os.path.abspath(args.repo))
    unlisted, stale = check_header_externs(header_externs, EXTERN_ALLOWED)
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
    for path, number, text in externs:
        print('%s:%d: %s' % (path, number, text))
    if externs:
        print('An extern declaration in a source file of the engine: declare the global in the header of the '
              'file that defines it, and include that header (Script/collect_externs.py does the second part).')
    for path, number, name in unlisted:
        print('%s:%d: extern %s is not in EXTERN_ALLOWED' % (path, number, name))
    if unlisted:
        print('A new global in a header of the engine: make it a member of EngineServices, Game or Room, or a '
              'static of its module (see the top of Script/check_layers.py).')
    for name in stale:
        print('no header declares %s any more: remove it from EXTERN_ALLOWED in Script/check_layers.py' % name)
    print('check_layers: %d includes, %d known violations in the baseline, %d new, %d fixed and still listed, '
          '%d extern declarations in source files, %d extern variables in headers (%d not allowed, %d allowed '
          'and gone)'
          % (count, len(baseline & set(found)), len(new), len(fixed), len(externs), len(header_externs),
             len(unlisted), len(stale)))
    return 1 if new or fixed or externs or unlisted or stale else 0


if __name__ == '__main__':
    sys.exit(main())
