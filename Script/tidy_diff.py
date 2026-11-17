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
one is edited. That is the intent: legacy code is cleaned up by tasks of its own. A rename of the
parameters also changes the first line without making the function new, so a function-size or
function-name diagnostic is dropped when the base revision's copy of the file already has a function
of that name (PLAN.md phase 4 touches every declaration; function names are 4.2, lengths are 5.4).
A new or renamed function is checked in full. A line whose code did not change, only its // comment or
its spacing (the comment pass, PLAN.md 4.5), is not a changed line.

Moved and copied code is not new code (PLAN.md 5.2 moves functions between files, 5.3 moves whole files).
An added line is not a changed line when its code (code_of: no // comment, no spacing) is the code of
any line of the base revision's copy of a C or C++ file that the diff modifies, deletes or renames, the
lines the diff keeps there included: the source of a copy keeps its line (a `using namespace` the new
file repeats), and a move can land in a file that never lost a line. Only files the diff touches are
read, not the whole tree, so a line copied from an untouched file still counts. A header line matches
header lines only and a source line source lines, since a header has rules of its own (no `using
namespace` there) and code that goes from a source into a header is checked as a header's. Every line is
compared, braces and `return 0;` too, since a diagnostic is about a line's code and that code passed
when it stood in the base tree. A renamed file (`git mv`, then its `#include` lines rewritten) comes out
as its rewritten lines alone, whether git pairs the two paths or not (then the old path counts as
deleted). Untracked files get the same rule. The base copies are read in one `git cat-file --batch` and
indexed once, so a diff of hundreds of files costs one pass over them. The function rule above follows
the same reading: a function-size or function-name diagnostic is dropped when a function of that name
is in the base copy of any file the diff touches (a Register*API that moved to a new file), and a
diagnostic on a declaration line that moved unchanged is gone already, since that line is not changed.

clang-tidy's naming check knows case styles, not words, so a new "CharacterDef *chaa" would pass it.
The banned abbreviations (CRYPTIC_WATCHLIST in refactor_metrics.py; STYLE.md "Naming") are found through
readability-identifier-length, which .clang-tidy sets up to report every variable and parameter declared
on a changed line: the ones on the watchlist are reported as [ags-cryptic-name], the rest dropped. Only
a declaration is flagged; a changed line that uses a legacy "chaa" is not, and neither is an `extern`
declaration of a global, which repeats the name its definition has (5.1 step 1 moved the declarations of
`croom`, `objs` and `scsystem` into headers; a new global is flagged where it is defined). The same list serves the
reserved names (STYLE.md "Naming": `ch` is a character, `btn` a GUIButton):
a declaration of one of them is read from the source line and reported as [ags-reserved-name] when
the type written there is not the reserved one. A declaration by `auto` is not checked.

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
# The files whose base copies a moved or copied line can come from, in any directory (see the docstring)
CODE_FILES = ('.c',) + SOURCES + HEADERS
DIAGNOSTIC = re.compile(r'^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): (?P<text>.*?)'
                        r' \[(?P<check>[A-Za-z0-9_.,-]+)\]$')
ANSI = re.compile('\x1b\\[[0-9;]*m')
HUNK = re.compile(r'^@@ -\S+ \+(\d+)(?:,(\d+))? @@')
# A // comment to the end of the line, not inside a string literal (good enough to compare two lines' code)
LINE_COMMENT = re.compile(r'//(?=(?:[^"]*"[^"]*")*[^"]*$).*$')
# The check that lists the declared names (see the docstring), and the name in its message
DECLARED_CHECK = 'readability-identifier-length'
DECLARED_NAME = re.compile(r"name '(\w+)' is too short")
CRYPTIC_CHECK = 'ags-cryptic-name'
# Reserved names and the types they may have (STYLE.md "Naming"); a type token of the declaration must match
RESERVED_NAMES = {'ch': ('Character', 'CharacterDef', 'AGSCharacter'),
                  'btn': ('GUIButton',)}
RESERVED_CHECK = 'ags-reserved-name'
# The diagnostics about a whole function, reported on its declaration line (see the docstring)
FUNCTION_CHECKS = {'readability-function-size': re.compile(r"^function '([\w:~]+)' exceeds"),
                   'readability-identifier-naming': re.compile(r"^invalid case style for (?:function|method) '([\w:~]+)'")}


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


def code_of(line):
    """The code of a source line with its // comment and all whitespace taken out ('' for a comment line)."""
    return re.sub(r'\s+', '', LINE_COMMENT.sub('', line))


def base_texts(base, repo=REPO):
    """{path: text} of the base revision's copies of the C and C++ files that the diff from base to the
    working tree modifies, deletes or renames (the old path of a rename), read in one git call."""
    r = subprocess.run(['git', '-c', 'core.quotepath=off', 'diff', '--name-status', '-z', '-M', '--no-ext-diff',
                        base], cwd=repo, capture_output=True)
    if r.returncode != 0:
        sys.exit('git diff --name-status failed: %s' % r.stderr.decode('utf-8', 'replace').strip())
    fields = r.stdout.decode('utf-8', 'replace').split('\0')
    paths, i = [], 0
    while i < len(fields) - 1:
        status = fields[i]
        old = fields[i + 1]
        i += 3 if status[:1] in 'RC' else 2
        if status[:1] not in 'AC' and old.endswith(CODE_FILES):
            paths.append(old)
    if not paths:
        return {}
    request = ''.join('%s:%s\n' % (base, p) for p in paths).encode('utf-8')
    r = subprocess.run(['git', 'cat-file', '--batch'], cwd=repo, input=request, capture_output=True)
    if r.returncode != 0:
        sys.exit('git cat-file --batch failed: %s' % r.stderr.decode('utf-8', 'replace').strip())
    texts, out, pos = {}, r.stdout, 0
    for path in paths:
        end = out.index(b'\n', pos)
        header = out[pos:end].split()
        pos = end + 1
        if len(header) == 3 and header[1] == b'blob':
            size = int(header[2])
            texts[path] = out[pos:pos + size].decode('utf-8', 'replace')
            pos += size + 1  # the content and the newline after it
    return texts


def changed_lines(base, texts):
    """{repo-relative path: [(first, last), ...]} of the added and rewritten lines, base to working tree.

    An added line whose code is that of a line in texts (base_texts: only its // comment or its spacing
    changed, or it moved or was copied from a file the diff touches) is left out, see the docstring."""
    known = {False: set(), True: set()}  # by whether the file is a header
    for base_path, text in texts.items():
        known[base_path.endswith(HEADERS)].update(map(code_of, text.splitlines()))
    lines = {}
    path = None
    kept = 0

    def add(path, number, code):
        nonlocal kept
        if code and code in known[path.endswith(HEADERS)]:
            kept += 1
        else:
            lines.setdefault(path, []).append(number)

    for line in git('diff', '-U0', '--no-color', '--no-ext-diff', '--diff-filter=AMR', base).splitlines():
        if line.startswith('+++ '):
            path = line[6:] if line.startswith('+++ b/') else None
            if path is not None and not is_checked(path):
                path = None
            continue
        m = HUNK.match(line)
        if m:
            number = int(m.group(1))
        elif line.startswith('+') and path is not None:
            add(path, number, code_of(line[1:]))
            number += 1
    for path in git('ls-files', '--others', '--exclude-standard').splitlines():
        if is_checked(path):
            with open(os.path.join(REPO, path), encoding='utf-8', errors='replace') as f:
                for number, text in enumerate(f.read().splitlines(), 1):
                    add(path, number, code_of(text))
    result = {}
    for path, numbers in lines.items():
        for number in numbers:
            ranges = result.setdefault(path, [])
            if ranges and ranges[-1][1] == number - 1:
                ranges[-1] = (ranges[-1][0], number)
            else:
                ranges.append((number, number))
    return result, kept


class BaseFunctions:
    """Tells whether a function name is in the base copy of any file the diff touches (base_texts)."""

    def __init__(self, texts):
        self._text = '\n'.join(texts.values())
        self._known = {}

    def has(self, name):
        name = name.split('::')[-1]
        if name not in self._known:
            self._known[name] = re.search(r'\b%s\s*\(' % re.escape(name), self._text) is not None
        return self._known[name]


def is_extern_declaration(path, line):
    """Whether that source line is an `extern` declaration of a global: it names nothing new, the definition did."""
    try:
        with open(os.path.join(REPO, path), encoding='utf-8', errors='replace') as f:
            text = f.read().splitlines()[line - 1]
    except (OSError, IndexError):
        return False
    return re.match(r'\s*extern\s+(?!"C")', text) is not None


def reserved_name_misuse(path, line, name):
    """The type a reserved name is declared with on that source line, if it is not an allowed one; else None."""
    try:
        with open(os.path.join(REPO, path), encoding='utf-8', errors='replace') as f:
            text = f.read().splitlines()[line - 1]
    except (OSError, IndexError):
        return None
    text = text.split('//')[0]
    # every declaration of the name on the line: the tokens before it, back to the previous separator
    for m in re.finditer(r'\b%s\b' % re.escape(name), text):
        before = re.split(r'[(,;{]', text[:m.start()])[-1]
        tokens = re.findall(r'[A-Za-z_]\w*', before)
        if not tokens or 'auto' in tokens or tokens[-1] in ('return', 'else', 'case', 'delete', 'throw', 'co_return'):
            continue  # a use, not a declaration, or a type we cannot read
        if any(t == allowed for t in tokens for allowed in RESERVED_NAMES[name]):
            continue
        return ' '.join(tokens)
    return None


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
    # warnings, such as an MSVC or GCC flag it does not use, into errors that stop the analysis.
    # -fno-delayed-template-parsing: with an MSVC compile database clang parses a template's body only when it is
    # instantiated, so a header handed over on its own had its templates skipped, and CI's GCC database found what
    # the local run did not (a branch clone in field_table.h, 2026-11-17). A GCC database has it off already.
    cmd = [tidy, '-p', build, '--quiet', line_filter, '--checks=' + DECLARED_CHECK,
           '--extra-arg=-Wno-unknown-warning-option', '--extra-arg=-Wno-error', '--extra-arg=-fno-color-diagnostics',
           '--extra-arg=-fno-delayed-template-parsing', path]
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
    texts = base_texts(base)
    changed, kept = changed_lines(base, texts)
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
    print('tidy_diff: %d files, %d changed lines since %s (%d added lines left out as moved or copied)' %
          (len(files), sum(b - a + 1 for rs in changed.values() for a, b in rs), base[:12], kept), flush=True)

    # A diff of a few hundred files makes a filter longer than a Windows command line may be (32k), so it
    # goes into a response file, written as one quoted argument
    response = os.path.join(build, 'tidy_diff_line_filter.rsp')
    with open(response, 'w', encoding='utf-8') as fh:
        fh.write('"--line-filter=%s"' % line_filter.replace('"', '\\"'))
    line_filter = '@' + response

    found = {}
    broken = []
    legacy = BaseFunctions(texts)
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
                        if is_extern_declaration(name, int(m.group('line'))):
                            continue  # the name was given where the global is defined; this line declares it
                        text = "banned abbreviation '%s' in a new name (STYLE.md \"Naming\")" % declared.group(1)
                        found[(name, int(m.group('line')), CRYPTIC_CHECK, text)] = True
                    elif declared and declared.group(1) in RESERVED_NAMES:
                        misuse = reserved_name_misuse(name, int(m.group('line')), declared.group(1))
                        if misuse:
                            text = "'%s' is reserved for a %s, not a '%s' (STYLE.md \"Naming\")" % (
                                declared.group(1), ' or '.join(RESERVED_NAMES[declared.group(1)]), misuse)
                            found[(name, int(m.group('line')), RESERVED_CHECK, text)] = True
                elif m.group('check') in FUNCTION_CHECKS and \
                        (fn := FUNCTION_CHECKS[m.group('check')].match(m.group('text'))) and \
                        legacy.has(fn.group(1)):
                    continue  # an old function whose declaration line changed or moved, not a new one
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
