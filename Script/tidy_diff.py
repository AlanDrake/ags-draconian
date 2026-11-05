#!/usr/bin/env python3
"""
The clang-tidy gate for new code (docs/refactor/PLAN.md, 2.4): runs clang-tidy with the root .clang-tidy
and reports only the diagnostics on lines that differ from a base revision. The legacy code violates
the naming and size rules by the thousand, so the gate looks at what a push or a pull request touches.

    python Script/tidy_diff.py --build <dir with compile_commands.json> [--base REV]

The diff is REV against the working tree, so uncommitted edits count and untracked C++ files count in
full. Without --base it is the merge base with the upstream branch (CI passes the pull request's base or
the push's "before" commit). Source files go to clang-tidy with a --line-filter of their changed lines;
headers go there too, unless a .cpp of the same name is in the diff and covers them. Third-party code
(libsrc, libinclude, anything outside Common, Engine, Compiler, Tools, Plugins) is never checked.

A function-size or naming diagnostic is reported on the line of the declaration, so a long function is
flagged when its first line is in the diff (new or rewritten functions), not when a line inside an old
one is edited. That is the intent: legacy code is cleaned up by tasks of its own.

clang-tidy's naming check knows case styles, not words, so a new "CharacterInfo *chaa" would pass it.
The banned abbreviations (CRYPTIC_WATCHLIST in refactor_metrics.py; STYLE.md "Naming") are found through
readability-identifier-length, which .clang-tidy sets up to report every variable and parameter declared
on a changed line: the ones on the watchlist are reported as [ags-cryptic-name], the rest dropped. Only
a declaration is flagged; a changed line that uses a legacy "chaa" is not.

Needs clang-tidy 19.1 (CLANG_TIDY env var, --clang-tidy, or on PATH; pip install clang-tidy==19.1.0).
On Windows the compile database comes from an MSVC build, so run it from a VS developer environment
(Script/prepush.py does). Exit code 0: no diagnostics on changed lines; 1: diagnostics; 2: cannot run.
"""
import argparse
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

from refactor_metrics import CRYPTIC_WATCHLIST

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CLANG_TIDY_VERSION = '19.1'
CHECKED_DIRS = ('Common/', 'Engine/', 'Compiler/', 'Tools/', 'Plugins/')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude|_deps)/')
SOURCES = ('.cpp', '.cc', '.cxx')
HEADERS = ('.h', '.hpp', '.inl')
DIAGNOSTIC = re.compile(r'^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): (?P<text>.*?)'
                        r' \[(?P<check>[A-Za-z0-9_.,-]+)\]$')
ANSI = re.compile('\x1b\\[[0-9;]*m')
HUNK = re.compile(r'^@@ -\S+ \+(\d+)(?:,(\d+))? @@')
# The check that lists the declared names (see the docstring), and the name in its message
DECLARED_CHECK = 'readability-identifier-length'
DECLARED_NAME = re.compile(r"name '(\w+)' is too short")
CRYPTIC_CHECK = 'ags-cryptic-name'


def git(*cmd):
    r = subprocess.run(['git', '-c', 'core.quotepath=off', *cmd], cwd=REPO, capture_output=True, text=True,
                       encoding='utf-8', errors='replace')
    if r.returncode != 0:
        sys.exit('git %s failed: %s' % (' '.join(cmd), r.stderr.strip()))
    return r.stdout


def is_checked(path):
    return path.startswith(CHECKED_DIRS) and not THIRD_PARTY.search(path) and path.endswith(SOURCES + HEADERS)


def default_base():
    for ref in ('@{upstream}', 'origin/ags4draconianr21'):
        r = subprocess.run(['git', 'merge-base', 'HEAD', ref], cwd=REPO, capture_output=True, text=True)
        if r.returncode == 0:
            return r.stdout.strip()
    sys.exit('no --base given and no upstream branch to compare with')


def changed_lines(base):
    """{repo-relative path: [(first, last), ...]} of the added and rewritten lines, base to working tree."""
    result = {}
    path = None
    for line in git('diff', '-U0', '--no-color', '--no-ext-diff', '--diff-filter=AMR', base).splitlines():
        if line.startswith('+++ '):
            path = line[6:] if line.startswith('+++ b/') else None
            if path is not None and not is_checked(path):
                path = None
            continue
        m = HUNK.match(line)
        if m and path is not None:
            first, count = int(m.group(1)), int(m.group(2) or '1')
            if count > 0:
                result.setdefault(path, []).append((first, first + count - 1))
    for path in git('ls-files', '--others', '--exclude-standard').splitlines():
        if is_checked(path):
            with open(os.path.join(REPO, path), 'rb') as f:
                count = len(f.read().splitlines())
            if count > 0:
                result[path] = [(1, count)]
    return result


def find_clang_tidy(explicit):
    """The path of a clang-tidy 19.1, or None."""
    cands = [explicit, os.environ.get('CLANG_TIDY')]
    for sub in ('Scripts/clang-tidy.exe', 'bin/clang-tidy'):
        cands.append(os.path.join(REPO, '.compare', 'cfenv', sub))
    for cand in cands + ['clang-tidy', 'clang-tidy-19']:
        if not cand:
            continue
        try:
            out = subprocess.run([cand, '--version'], capture_output=True, text=True).stdout
        except OSError:
            continue
        if 'version ' + CLANG_TIDY_VERSION in out:
            return cand
    return None


def plan(changed, build):
    """The files to hand to clang-tidy: sources of the compile database, and headers nothing else covers."""
    with open(os.path.join(build, 'compile_commands.json'), encoding='utf-8') as f:
        database = {os.path.normcase(os.path.normpath(e['file'])) for e in json.load(f)}
    files, skipped = [], []
    stems = {os.path.splitext(p)[0] for p in changed if p.endswith(SOURCES)}
    for path in sorted(changed):
        full = os.path.normcase(os.path.normpath(os.path.join(REPO, path)))
        if path.endswith(SOURCES):
            (files if full in database else skipped).append(path)
        elif os.path.splitext(path)[0] not in stems:
            files.append(path)
    return files, skipped


def run_one(tidy, build, path, line_filter):
    # -Wno-error: a build with warnings as errors (AGS_WARNINGS_AS_ERRORS) must not turn clang's own
    # warnings, such as an MSVC or GCC flag it does not use, into errors that stop the analysis
    cmd = [tidy, '-p', build, '--quiet', '--line-filter=' + line_filter, '--checks=' + DECLARED_CHECK,
           '--extra-arg=-Wno-unknown-warning-option', '--extra-arg=-Wno-error', '--extra-arg=-fno-color-diagnostics',
           path]
    r = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, encoding='utf-8', errors='replace')
    return path, r.stdout, r.stderr, r.returncode


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--build', required=True, help='directory with compile_commands.json')
    ap.add_argument('--base', help='revision to compare with (default: merge base with the upstream branch)')
    ap.add_argument('--clang-tidy', help='path of clang-tidy %s' % CLANG_TIDY_VERSION)
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    args = ap.parse_args()

    build = os.path.abspath(args.build)
    if not os.path.isfile(os.path.join(build, 'compile_commands.json')):
        print('no compile_commands.json in %s (configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)' % build)
        return 2
    tidy = find_clang_tidy(args.clang_tidy)
    if tidy is None:
        print('clang-tidy %s not found: pip install clang-tidy==19.1.0 and pass --clang-tidy or set CLANG_TIDY'
              % CLANG_TIDY_VERSION)
        return 2
    base = args.base or default_base()
    changed = changed_lines(base)
    files, skipped = plan(changed, build)
    for path in skipped:
        print('not in the compile database, not checked: ' + path)
    if not files:
        print('tidy_diff: no first-party C++ lines changed since %s' % base[:12])
        return 0
    # Lines of every changed file, for the headers a source pulls in. clang-tidy matches the name as a
    # suffix of the path it reports, so the separators must be the platform's.
    line_filter = json.dumps([{'name': p.replace('/', os.sep), 'lines': [list(r) for r in rs]}
                              for p, rs in sorted(changed.items())])
    print('tidy_diff: %d files, %d changed lines since %s' %
          (len(files), sum(b - a + 1 for rs in changed.values() for a, b in rs), base[:12]), flush=True)

    found = {}
    broken = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run_one, tidy, build, p, line_filter) for p in files]
        for fut in futures:
            path, out, err, code = fut.result()
            parsed = False
            for raw in ANSI.sub('', out).splitlines():
                m = DIAGNOSTIC.match(raw)
                if not m:
                    continue
                parsed = True
                name = os.path.relpath(m.group('file'), REPO).replace(os.sep, '/') if os.path.isabs(m.group('file')) \
                    else m.group('file').replace(os.sep, '/')
                if m.group('check') == 'clang-diagnostic-error' or m.group('kind') == 'error':
                    broken.append('%s: %s [%s]' % (path, m.group('text'), m.group('check')))
                elif m.group('check').startswith('clang-diagnostic-'):
                    continue  # compiler warnings are the build's business
                elif m.group('check') == DECLARED_CHECK:
                    declared = DECLARED_NAME.search(m.group('text'))
                    if declared and declared.group(1) in CRYPTIC_WATCHLIST:
                        text = "banned abbreviation '%s' in a new name (STYLE.md \"Naming\")" % declared.group(1)
                        found[(name, int(m.group('line')), CRYPTIC_CHECK, text)] = True
                else:
                    found[(name, int(m.group('line')), m.group('check'), m.group('text'))] = True
            if code != 0 and not parsed:
                broken.append('%s: clang-tidy exited with %d: %s' % (path, code, ' '.join(err.split())[-300:]))

    for (name, line, check, text) in sorted(found):
        print('%s:%d: %s [%s]' % (name, line, text, check))
    for text in broken:
        print('could not analyse: ' + text)
    if found:
        print('tidy_diff: %d diagnostic(s) on changed lines' % len(found))
        return 1
    if broken:
        print('tidy_diff: %d file(s) did not compile, so they were not checked' % len(broken))
        return 2
    print('tidy_diff: clean')
    return 0


if __name__ == '__main__':
    sys.exit(main())
