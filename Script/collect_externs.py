#!/usr/bin/env python3
"""
Takes the `extern` declarations out of the engine's .cpp files (docs/refactor/PLAN.md, 5.1 step 1): a global is
declared once, in a header, and a file that uses it includes that header.

    python Script/collect_externs.py             # report what would change, exit 1 if something is left
    python Script/collect_externs.py --apply     # rewrite the files (run clang-format on them afterwards)
    python Script/collect_externs.py --repo <dir>

What it does to every .cpp under Engine/ (tests included; third-party code is not looked at):
  * An `extern` line (not `extern "C"`) whose names are all declared in one header of the engine is deleted,
    and `#include "<header>"` is added after the file's last include of its main include block unless the file
    includes that header already (directly). A header declares a variable with `extern`, a function with a
    plain declaration.
  * An `extern` declaration of a function that no header declares, in the file that defines the function or
    of `main`, loses the keyword only: it is a forward declaration, and `extern` says nothing there.
  * A .cpp that defines a global which a header declares gets the include of that header when the header is
    not among the files it includes, directly or not, so that the compiler compares the two types.

What it does not do: write the declarations into the headers. A name that no header declares, or that two
headers declare, is reported and its line is left alone. The headers are edited by hand first, then this runs.

A global defined in one ring of the engine and used in a lower one (Script/check_layers.py) was a dependency
that the include rules could not see; its include now breaks them openly, and is added to the baseline of that
check with the reason, or fixed.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

SOURCES = ('.cpp', '.mm', '.m', '.c', '.cc')
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude)/')
EXTERN = re.compile(r'^(\s*)extern\s+(?!"C")(.*)$')
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
IDENT = r'[A-Za-z_]\w*'


def git_files(repo, *paths):
    out = subprocess.run(['git', '-C', repo, 'ls-files', '--'] + list(paths), capture_output=True, text=True,
                         check=True).stdout
    return [f for f in out.split('\n') if f and not THIRD_PARTY.search(f)]


def read(repo, path):
    with open(os.path.join(repo, path), encoding='utf-8', newline='') as f:
        return f.read()


def strip_comment(line):
    return line.split('//')[0]


def split_declarators(body):
    """'int a, *b[3]' -> ['a', 'b']; a function declaration -> its name and True."""
    body = strip_comment(body).strip().rstrip(';').strip()
    m = re.match(r'^(.*?)(' + IDENT + r')\s*\((.*)\)\s*$', body)
    if m and not re.search(r'[=\[]', m.group(1)):
        return [m.group(2)], True
    parts, depth, cur = [], 0, ''
    for c in body:
        if c in '<([':
            depth += 1
        elif c in '>)]':
            depth -= 1
        if c == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += c
    parts.append(cur)
    names = []
    for i, p in enumerate(parts):
        p = re.sub(r'\[[^\]]*\]', '', p).strip()
        m = re.search(r'(' + IDENT + r')\s*$', p)
        if not m:
            raise ValueError(body)
        names.append(m.group(1))
    return names, False


def resolve(repo, includer, inc, known):
    """The repository path that `#include "inc"` in includer names, as the compilers find it."""
    roots = [os.path.dirname(includer), 'Engine', 'Common']
    for r in roots:
        p = os.path.normpath(os.path.join(r, inc)).replace('\\', '/')
        if p in known:
            return p
    return None


class Tree:
    def __init__(self, repo):
        self.repo = repo
        self.files = [f for f in git_files(repo, 'Engine', 'Common') if f.endswith(SOURCES + ('.h', '.inl'))]
        # untracked headers and sources of the engine count too (a header added by this change)
        extra = subprocess.run(['git', '-C', repo, 'ls-files', '--others', '--exclude-standard', '--', 'Engine'],
                               capture_output=True, text=True, check=True).stdout.split('\n')
        self.files += [f for f in extra if f and f.endswith(SOURCES + ('.h',)) and not THIRD_PARTY.search(f)]
        self.known = set(self.files)
        self.text = {f: read(repo, f) for f in self.files}
        self.lines = {f: self.text[f].split('\n') for f in self.files}
        self._direct = {}
        self._closure = {}

    def direct(self, f):
        if f not in self._direct:
            out = []
            for l in self.lines[f]:
                m = INCLUDE.match(l)
                if m:
                    p = resolve(self.repo, f, m.group(1), self.known)
                    if p:
                        out.append(p)
            self._direct[f] = out
        return self._direct[f]

    def closure(self, f):
        if f not in self._closure:
            seen, todo = set(), [f]
            while todo:
                for p in self.direct(todo.pop()):
                    if p not in seen:
                        seen.add(p)
                        todo.append(p)
            self._closure[f] = seen
        return self._closure[f]


def header_declarations(tree):
    """name -> set of engine headers that declare it: variables by `extern`, functions by a plain declaration."""
    variables = collections.defaultdict(set)
    for f in tree.files:
        if not (f.startswith('Engine/') and f.endswith('.h')):
            continue
        for l in tree.lines[f]:
            m = EXTERN.match(l)
            if not m or m.group(1):
                continue
            try:
                names, is_func = split_declarators(m.group(2))
            except ValueError:
                continue
            for n in names:
                variables[n].add(f)
    return variables


def function_headers(tree, name):
    pat = re.compile(r'^(?!\s)(?!.*\b(return|typedef|using)\b)[\w:<>,\*&\s]*?[\s\*&]' + re.escape(name) + r'\s*\(')
    out = set()
    for f in tree.files:
        if f.startswith('Engine/') and f.endswith('.h'):
            if any(pat.match(strip_comment(l)) for l in tree.lines[f]):
                out.add(f)
    return out


def definitions(tree, names):
    """name -> set of engine .cpp files with a definition at file scope (a heuristic: a line that starts in
    column 0, is not a declaration with extern or static, and has the name as a declarator)."""
    out = collections.defaultdict(set)
    wanted = set(names)
    word = re.compile(IDENT)
    for f in tree.files:
        if not (f.startswith('Engine/') and f.endswith(SOURCES)):
            continue
        depth = 0
        for l in tree.lines[f]:
            code = strip_comment(l)
            if depth == 0 and code[:1] not in (' ', '\t', '#', '}', '') and \
                    not re.match(r'(extern|static|typedef|using|namespace|return|template|class|struct|enum)\b', code):
                head = code.split('=')[0] if '(' not in code.split('=')[0] else code.split('(')[0]
                # every declarator of a `type a = 1, b = 2;` line
                cands = set()
                if '(' in code.split('=')[0]:
                    m = re.search(r'(?<!:)\b(' + IDENT + r')\s*$', head)  # not a member: Class::name(
                    if m and not code.rstrip().endswith(';'):
                        cands.add(m.group(1))  # a function definition
                    elif m and re.search(r'\)\s*;\s*$', code) and re.match(r'^[\w:<>\*&\s]+\s' + IDENT + r'\(', code):
                        cands.add(m.group(1))  # an object with constructor arguments
                else:
                    try:
                        cands.update(split_declarators(re.sub(r'=[^,;]*', '', code))[0])
                    except ValueError:
                        pass
                for n in cands & wanted:
                    out[n].add(f)
            depth += code.count('{') - code.count('}')
            depth = max(depth, 0)
    return out


def include_name(header):
    for root in ('Engine/', 'Common/'):
        if header.startswith(root):
            return header[len(root):]
    return header


def main_include_block(lines, before):
    """Index of the line after which a new include goes: the end of the largest run of quoted includes, before
    line `before`, that is under the same #if nesting as most of the file's quoted includes. (Some files have a
    second, small run of includes further down; the new one belongs with the others at the top.)"""
    stack, found, n = [], [], 0
    for i, l in enumerate(lines):
        s = l.strip()
        if re.match(r'#\s*if', s):
            n += 1
            stack.append(n)
        elif re.match(r'#\s*endif', s):
            if stack:
                stack.pop()
        elif INCLUDE.match(l):
            found.append((i, tuple(stack)))
    if not found:
        return None
    early = [x for x in found if x[0] < before] or found
    counts = collections.Counter(st for _, st in early)
    best = max(counts, key=lambda st: (counts[st], -len(st)))
    runs = []
    for i in (i for i, st in early if st == best):
        if runs and i == runs[-1][-1] + 1:
            runs[-1].append(i)
        else:
            runs.append([i])
    return max(runs, key=len)[-1]


def process(tree, path, declared, func_cache, report):
    lines = list(tree.lines[path])
    direct = set(tree.direct(path))
    remove, strip_kw, need = [], [], []
    for i, l in enumerate(lines):
        m = EXTERN.match(l)
        if not m:
            continue
        try:
            names, is_func = split_declarators(m.group(2))
        except ValueError:
            report['left'].append((path, i + 1, l.strip(), 'not understood'))
            continue
        headers = set()
        missing = []
        for n in names:
            hs = set(declared.get(n, ()))
            if is_func and not hs:
                if n not in func_cache:
                    func_cache[n] = function_headers(tree, n)
                hs = func_cache[n]
            if len(hs) == 1:
                headers |= hs
            elif not hs:
                missing.append(n)
            else:
                report['left'].append((path, i + 1, l.strip(), '%s is declared in %s' % (n, ', '.join(sorted(hs)))))
                headers = None
                break
        if headers is None:
            continue
        if missing:
            defined_here = is_func and re.search(r'^[\w:<>\*&\s]*?[\s\*&]' + re.escape(names[0]) + r'\s*\([^;]*$',
                                                 '\n'.join(lines[i + 1:]), re.M)
            if is_func and (defined_here or names[0] == 'main'):
                strip_kw.append(i)
                report['keyword'].append((path, i + 1, l.strip()))
            else:
                report['left'].append((path, i + 1, l.strip(), 'no header declares %s' % ', '.join(missing)))
            continue
        remove.append(i)
        for h in sorted(headers):
            if h not in direct and h not in need and h != os.path.splitext(path)[0] + '.h':
                need.append(h)
            elif h == os.path.splitext(path)[0] + '.h' and h not in direct and h not in need:
                need.append(h)
        # a comment line right above a run of externs may describe them: show it for review
        if i > 0 and lines[i - 1].strip().startswith('//') and (i - 1) not in remove:
            report['comments'].append((path, i, lines[i - 1].strip()))
    if not (remove or strip_kw):
        return None, need
    for i in strip_kw:
        lines[i] = re.sub(r'^(\s*)extern\s+', r'\1', lines[i])
    first = min(remove) if remove else 0
    at = main_include_block(lines, first) if need else None
    if need and at is None:
        report['left'].append((path, 0, '', 'no include block to add %s to' % ', '.join(need)))
        return None, need
    out = []
    gone = set(remove)
    for i, l in enumerate(lines):
        if i in gone:
            # a blank line on both sides of the run that went: keep one
            nxt = i + 1
            while nxt in gone:
                nxt += 1
            if (not out or out[-1].strip() == '') and nxt < len(lines) and lines[nxt].strip() == '' and \
                    (i + 1) not in gone:
                lines[nxt] = None
            continue
        if l is None:
            continue
        out.append(l)
        if at is not None and i == at:
            eol = '\r' if l.endswith('\r') else ''
            for h in need:
                out.append('#include "%s"%s' % (include_name(h), eol))
    report['removed'] += len(remove)
    return '\n'.join(out), need


def add_include(tree, path, header):
    lines = list(tree.lines[path])
    at = main_include_block(lines, len(lines))
    if at is None:
        return None
    eol = '\r' if lines[at].endswith('\r') else ''
    lines.insert(at + 1, '#include "%s"%s' % (include_name(header), eol))
    return '\n'.join(lines)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--repo', default=os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))
    ap.add_argument('--apply', action='store_true', help='rewrite the files')
    args = ap.parse_args()

    tree = Tree(args.repo)
    declared = header_declarations(tree)
    report = {'left': [], 'keyword': [], 'comments': [], 'removed': 0}
    func_cache = {}
    changed = {}
    includes = collections.Counter()
    for f in sorted(tree.files):
        if not (f.startswith('Engine/') and f.endswith(SOURCES)):
            continue
        text, need = process(tree, f, declared, func_cache, report)
        if text is not None:
            changed[f] = text
            for h in need:
                includes[(f, h)] += 1

    # The file that defines a declared global includes the declaration
    twice = sorted((n, hs) for n, hs in declared.items() if len(hs) > 1)
    defs = definitions(tree, declared.keys())
    unseen = []
    for n, hs in sorted(declared.items()):
        for d in sorted(defs.get(n, ())):
            for h in sorted(hs):
                if h in tree.closure(d) or h == d:
                    continue
                # the rewrite above may have added it
                if (d, h) in includes:
                    continue
                unseen.append((d, h, n))
    by_file = collections.defaultdict(list)
    for d, h, n in unseen:
        if h not in by_file[d]:
            by_file[d].append(h)
    for d, hs in by_file.items():
        for h in hs:
            base = changed.get(d)
            if base is not None:
                tree.lines[d] = base.split('\n')
            text = add_include(tree, d, h)
            if text is None:
                report['left'].append((d, 0, '', 'no include block to add %s to' % h))
            else:
                changed[d] = text
                includes[(d, h)] += 1
    undefined = sorted(n for n in declared if n not in defs)

    print('extern lines removed: %d in %d files' % (report['removed'], len(changed)))
    print('includes added: %d' % len(includes))
    for (f, h), _ in sorted(includes.items()):
        print('  %s  +  %s' % (f, include_name(h)))
    if unseen:
        print('definitions that did not see their declaration (include added above):')
        for d, h, n in unseen:
            print('  %s: %s, declared in %s' % (d, n, h))
    if report['keyword']:
        print('forward declarations of functions, keyword removed:')
        for f, i, l in report['keyword']:
            print('  %s:%d: %s' % (f, i, l))
    if report['comments']:
        print('comment lines above a removed declaration (review):')
        for f, i, l in report['comments']:
            print('  %s:%d: %s' % (f, i, l))
    if twice:
        print('declared in more than one header:')
        for n, hs in twice:
            print('  %s: %s' % (n, ', '.join(sorted(hs))))
    if undefined:
        print('declared in a header, no definition found in an engine source (by this script\'s heuristic):')
        print('  ' + ', '.join(undefined))
    if report['left']:
        print('LEFT ALONE:')
        for f, i, l, why in report['left']:
            print('  %s:%d: %s  [%s]' % (f, i, l, why))

    if args.apply:
        for f, text in changed.items():
            with open(os.path.join(args.repo, f), 'w', encoding='utf-8', newline='') as out:
                out.write(text)
        print('rewritten: %d files' % len(changed))
    return 1 if (report['left'] or twice) else 0


if __name__ == '__main__':
    sys.exit(main())
