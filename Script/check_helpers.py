#!/usr/bin/env python3
"""
Finds new code that repeats what the tree already has (docs/refactor/STYLE.md, "Shared helpers").

    python Script/check_helpers.py                     # the lines added since the merge base
    python Script/check_helpers.py --base <rev>        # ... since another revision
    python Script/check_helpers.py --all               # every file-local function: an audit, exit code 0
    python Script/check_helpers.py --repo ../ags4-upstream --base upstream/ags4   # a branch for upstream

Two checks, both on the lines a change adds to first-party C and C++ files:

  helpers     a new function outside the shared headers (SHARED below: Common/util, data_helpers.h)
              whose name is the name of a shared function, or shares a telling word with one
              ("Escape", "Utf8", "Base64", "Trim", "Split"): `static String UnescapeText(...)` next to
              StrUtil::Unescape. The shared function is used, or extended where it falls short; a new
              one that is general goes into the shared header. A finding that is neither is listed
              in KNOWN with its reason.
  references  a comment that names a member of an Editor class, "(Translation.EscapeText)", which
              that class does not have in this tree. The tools mirror the Editor of the repository
              they are in: a branch for upstream that cites a member only the fork's Editor has
              carries fork behaviour (upstream PR #3190).

The first check reads names, not code: it cannot see a loop that does by hand what a helper does.
It is the reminder to look, at the moment a helper is written. Exit code 1 if anything is found.
"""
import argparse
import os
import re
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
FIRST_PARTY = ('Common/', 'Engine/', 'Compiler/', 'Tools/', 'Plugins/')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude|_deps)/')
EXTENSIONS = ('.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.inl')
TESTS = re.compile(r'(^|/)test\d*/')
# Where the shared helpers are declared. A function added there is a shared one, not a finding
SHARED = ('Common/util/', 'Common/data/data_helpers.h')
# (path, function) of the accepted findings, each with the reason why the shared helper does not do
KNOWN = {
}
HUNK = re.compile(r'^@@ -\S+ \+(\d+)(?:,(\d+))? @@')
# The first line of a function definition or declaration: at most one level of indentation
# (upstream indents inside namespaces), a return type, a name that is not a member's
FUNCTION = re.compile(r'^(?: {4}|\t)?(?:(?:static|inline|constexpr|extern)\s+)*'
                      r'(?:[A-Za-z_][\w:<>,]*[\s\*&]+)+(?P<name>[A-Za-z_]\w*)\s*\(')
CALLED = re.compile(r'\b([A-Za-z_]\w*)\s*\(')
NOT_FUNCTIONS = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'defined', 'catch', 'operator', 'main', 'else',
                 'do', 'new', 'delete', 'throw', 'case', 'using', 'typedef', 'template', 'namespace', 'decltype',
                 'alignof', 'static_cast', 'const_cast', 'reinterpret_cast', 'dynamic_cast', 'static_assert'}
WORD = re.compile(r'[A-Z]?[a-z]+\d*|[A-Z]+\d*(?![a-z])|\d+')
# The words that name an operation on text or bytes of any kind, whatever the program is about: a
# new function with one of them in its name is compared with the shared functions that have it too.
# Words of the game's domain ("Script", "Room", "Event") are left out on purpose, they are in every name
TELLING_WORDS = set('''
    escape unescape escaped unescaped quote quoted unquote doublequote undoublequote doublequoted
    trim trimmed split join concat tokenize wildcard regex
    utf8 utf16 unicode ascii wstr codepage base64 hex lower upper nocase starts ends
    encrypt decrypt crc crc32 checksum compress decompress inflate deflate lzw endian bswap
'''.split())
# An equal name counts only if it says more than these words do: `AppendLine` is a name anyone gives
PLAIN_WORDS = set('''
    get set is has to from read write make create add find new open close for with of as at in on by and the
    string str cstr text file files data stream value int bool char size count name list item array buffer
    load save init free copy move check test parse print format append begin end first last next index length
    impl all any no not or if do run update reset clear delete remove insert push pop put line lines
'''.split())
# "(Type.Member)" in a comment, where Editor/**/Type.cs exists
CITED_MEMBER = re.compile(r'\b([A-Z][A-Za-z0-9]+)\.([A-Z][A-Za-z0-9_]+)\b')
COMMENT = re.compile(r'//(.*)$')

repo = REPO


def git(*cmd):
    r = subprocess.run(['git', '-c', 'core.quotepath=off', *cmd], cwd=repo, capture_output=True, text=True,
                       encoding='utf-8', errors='replace')
    if r.returncode != 0:
        sys.exit('git %s failed: %s' % (' '.join(cmd), r.stderr.strip()))
    return r.stdout


def default_base():
    for ref in ('@{upstream}', 'origin/ags4draconianr21'):
        r = subprocess.run(['git', 'merge-base', 'HEAD', ref], cwd=repo, capture_output=True, text=True)
        if r.returncode == 0:
            return r.stdout.strip()
    sys.exit('no --base given and no upstream branch to compare with')


def is_first_party(path):
    return path.startswith(FIRST_PARTY) and path.endswith(EXTENSIONS) and not THIRD_PARTY.search(path)


def is_shared(path):
    return path.startswith(SHARED)


def read(path):
    try:
        with open(os.path.join(repo, path), encoding='utf-8', errors='replace') as f:
            return f.read()
    except OSError:
        return ''


def words_of(name):
    return [w.lower() for w in WORD.findall(name)]


def function_on(line):
    """The name of the function that the line defines or declares, or None."""
    if line.rstrip().endswith(';') and '{' not in line:
        return None  # a prototype or a call: the definition is what counts
    m = FUNCTION.match(line)
    if not m:
        return None
    name = m.group('name')
    before = line[:m.start('name')]
    if name in NOT_FUNCTIONS or name.isupper() or before.rstrip().endswith('::') or '=' in before \
            or re.search(r'\b(return|else|typedef|using|new|delete|throw|case|goto)\b', before):
        return None
    return name


def shared_functions():
    """{name: header} of the functions the shared headers declare or define."""
    names = {}
    for path in git('ls-files', '--', *SHARED).splitlines():
        if not path.endswith(('.h', '.hpp', '.inl')):
            continue
        for line in read(path).splitlines():
            code = line.split('//')[0]
            if code.lstrip().startswith('#'):
                continue
            for name in CALLED.findall(code):
                if name not in NOT_FUNCTIONS and not name.isupper() and len(name) > 2:
                    names.setdefault(name, path)
    return names


def telling_words(shared):
    """{word of TELLING_WORDS: [the shared functions with it in their names]}."""
    uses = {}
    for name in shared:
        for word in set(words_of(name)) & TELLING_WORDS:
            uses.setdefault(word, []).append(name)
    return {word: sorted(names) for word, names in uses.items()}


def added_lines(base):
    """[(path, line number, text)] of the lines the working tree adds to first-party files since base."""
    lines = []
    path, number = None, 0
    for line in git('diff', '-U0', '--no-color', '--no-ext-diff', '--diff-filter=AMR', base).splitlines():
        if line.startswith('+++ '):
            path = line[6:] if line.startswith('+++ b/') else None
            if path is not None and not is_first_party(path):
                path = None
            continue
        m = HUNK.match(line)
        if m:
            number = int(m.group(1))
        elif path is not None and line.startswith('+'):
            lines.append((path, number, line[1:]))
            number += 1
    for path in git('ls-files', '--others', '--exclude-standard').splitlines():
        if is_first_party(path):
            lines += [(path, i + 1, text) for i, text in enumerate(read(path).splitlines())]
    return lines


def all_lines():
    """[(path, line number, text)] of the file-local functions' first lines in the whole tree."""
    lines = []
    for path in git('ls-files', '--', *FIRST_PARTY).splitlines():
        if is_first_party(path) and path.endswith(('.c', '.cc', '.cpp', '.cxx')):
            for i, text in enumerate(read(path).splitlines()):
                if text.startswith(('static ', 'inline ')):
                    lines.append((path, i + 1, text))
    return lines


def check_helpers(lines, shared, telling):
    findings = []
    for path, number, text in lines:
        if is_shared(path) or TESTS.search(path):
            continue
        name = function_on(text)
        if name is None or (path, name) in KNOWN:
            continue
        if name in shared and set(words_of(name)) - PLAIN_WORDS:
            alike = [name]
        else:
            hits = {}
            for word in set(words_of(name)):
                for other in telling.get(word, ()):
                    hits[other] = hits.get(other, 0) + 1
            alike = sorted(hits, key=lambda other: (-hits[other], other))[:4]
        if alike:
            findings.append('%s:%d: %s() resembles %s' % (
                path, number, name, ', '.join('%s (%s)' % (other, shared[other]) for other in alike)))
    return findings


def check_references(lines):
    editor_files = {}
    for path in git('ls-files', '--', 'Editor').splitlines():
        if path.endswith('.cs'):
            editor_files.setdefault(os.path.basename(path)[:-3], []).append(path)
    texts = {}
    findings = []
    for path, number, text in lines:
        m = COMMENT.search(text)
        if not m:
            continue
        for type_name, member in CITED_MEMBER.findall(m.group(1)):
            if type_name not in editor_files:
                continue
            if type_name not in texts:
                texts[type_name] = '\n'.join(read(p) for p in editor_files[type_name])
            if not re.search(r'\b%s\b' % re.escape(member), texts[type_name]):
                findings.append('%s:%d: the comment cites %s.%s, which %s does not have in this tree' % (
                    path, number, type_name, member, ', '.join(editor_files[type_name])))
    return findings


def main():
    global repo
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--base', help='the revision to compare the working tree with\n'
                                   '(default: the merge base with the upstream branch)')
    ap.add_argument('--repo', help='another working tree to check, for example the worktree of the\n'
                                   'branches sent upstream; needs --base (upstream/ags4)')
    ap.add_argument('--all', action='store_true', help='list every file-local function that resembles a\n'
                                                       'shared helper, not only the new ones (exit code 0)')
    args = ap.parse_args()
    if args.repo:
        repo = os.path.abspath(args.repo)
        if not args.base and not args.all:
            sys.exit('--repo needs --base')

    shared = shared_functions()
    telling = telling_words(shared)
    if args.all:
        findings = check_helpers(all_lines(), shared, telling)
        print('\n'.join(findings))
        print('check_helpers: %d file-local function(s) resemble a shared helper' % len(findings))
        return 0
    lines = added_lines(args.base or default_base())
    helpers = check_helpers(lines, shared, telling)
    references = check_references(lines)
    for finding in helpers + references:
        print(finding)
    if helpers:
        print('Use the shared function, extend it, or list the new one in KNOWN (%s) with the reason.'
              % os.path.relpath(__file__, REPO).replace(os.sep, '/'))
    if references:
        print('Mirror the Editor of this tree: name a member it has, or drop the behaviour.')
    print('check_helpers: %d added line(s), %d new function(s) resembling a shared helper, %d stale reference(s)'
          % (len(lines), len(helpers), len(references)))
    return 1 if helpers or references else 0


if __name__ == '__main__':
    sys.exit(main())
