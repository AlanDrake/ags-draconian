#!/usr/bin/env python3
"""
Computes code-health metrics tracked by docs/refactor/PLAN.md.

Run from the repository root:
    python Script/refactor_metrics.py            # markdown report to stdout
    python Script/refactor_metrics.py --json     # machine-readable, for diffing between runs
    python Script/refactor_metrics.py --list-long 40 --list-dead 200

All analysis is heuristic (regex + brace matching, no real C++ parser). Numbers are meant
for tracking trends between runs, not as exact truth. Dead-code candidates in particular
must be verified (linker gc-sections output, grep, tests) before deleting anything.
"""
import argparse
import collections
import json
import os
import re
import subprocess
import sys

# Third-party / generated code: never measured, never reformatted
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude|googletest|_deps|build|bin|obj|packages|References'
                         r'|scintilla|ScintillaNET|AdressBarEx)(/|$)'
                         r'|^Plugins/agslua/agslua/(lualibhelp|zlib)')
CPP_EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')
MODULES = ['Common', 'Engine', 'Compiler', 'Tools', 'Plugins', 'Editor']

SIG_RE = re.compile(r'^(?:static\s+|inline\s+)*[A-Za-z_][\w:<>,\*&\s~]*?\b([A-Za-z_][\w:~]*)\s*\(([^;{}]*)\)\s*'
                    r'(?:const)?\s*(?:override)?\s*(?:\{\s*)?$')
DEF_RE = re.compile(r'^(?:static\s+|inline\s+)*[A-Za-z_][\w:<>,\*&\s]*?[\s\*&]([A-Za-z_]\w*(?:::[A-Za-z_~]\w*)?)\s*\([^;]*$')
TOK_RE = re.compile(r'[A-Za-z_]\w*')
CRYPTIC_WATCHLIST = ['chaa', 'chap', 'chin', 'chinfo', 'char1', 'chi', 'objj', 'guio', 'butt', 'sds', 'eres', 'tqs',
                     'scam', 'texx', 'widd', 'aschar', 'iit', 'vii', 'xxx', 'yyy', 'xx', 'yy', 'fil', 'scv', 'guin',
                     'butid', 'guisl', 'tehtext', 'ddd', 'abuf', 'thisroom', 'croom', 'objs', 'scsystem', 'usetup']
NOT_FUNCS = {'if', 'for', 'while', 'switch', 'return', 'else', 'catch', 'main', 'WinMain', 'operator', 'sizeof'}


def tracked_files():
    out = subprocess.run(['git', 'ls-files'], capture_output=True, text=True, check=True).stdout
    return [f for f in out.splitlines() if not THIRD_PARTY.search(f) and os.path.isfile(f)]


def strip_comments_and_strings(src):
    src = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), src, flags=re.S)
    src = re.sub(r'//[^\n]*', '', src)
    src = re.sub(r'"(\\.|[^"\\\n])*"', '""', src)
    return re.sub(r"'(\\.|[^'\\\n])*'", "''", src)


def scan_functions(path, code_lines):
    """Yields (length, max_nesting, line, name) for each function body found."""
    i = 0
    while i < len(code_lines):
        line = code_lines[i].rstrip()
        m = SIG_RE.match(line)
        opens_here = line.endswith('{')
        opens_next = i + 1 < len(code_lines) and code_lines[i + 1].strip().startswith('{')
        if m and m.group(1).split('::')[-1] not in NOT_FUNCS and (opens_here or opens_next):
            start = i if opens_here else i + 1
            depth = max_depth = 0
            j = start
            while j < len(code_lines):
                for ch in code_lines[j]:
                    if ch == '{':
                        depth += 1
                        max_depth = max(max_depth, depth)
                    elif ch == '}':
                        depth -= 1
                if depth <= 0:
                    break
                j += 1
            yield (j - start + 1, max_depth - 1, i + 1, m.group(1))
            i = j
        i += 1


def count_re(lines, pattern):
    rx = re.compile(pattern)
    return sum(1 for l in lines if rx.search(l))


def collect(files):
    m = collections.OrderedDict()
    cpp = [f for f in files if f.endswith(CPP_EXT)]
    cs = [f for f in files if f.endswith('.cs') and not f.endswith('.Designer.cs')]

    raw = {}
    for f in cpp + cs:
        with open(f, encoding='utf-8', errors='replace') as fh:
            raw[f] = fh.read()

    loc = collections.Counter()
    for f, txt in raw.items():
        mod = f.split('/', 1)[0]
        if mod in MODULES:
            loc[(mod, 'C#' if f.endswith('.cs') else 'C/C++')] += txt.count('\n')
    m['loc'] = {f'{mod} {lang}': n for (mod, lang), n in sorted(loc.items())}

    src_cpp = [f for f in cpp if '/test' not in f]
    all_lines = {f: raw[f].split('\n') for f in src_cpp}
    flat = [l for f in src_cpp for l in all_lines[f]]
    hdr_lines = [l for f in src_cpp if f.endswith(('.h', '.hpp')) for l in all_lines[f]]
    eng_cpp_lines = [l for f in src_cpp if f.startswith('Engine/') and f.endswith('.cpp') for l in all_lines[f]]
    eng_lines = [l for f in src_cpp if f.startswith('Engine/') for l in all_lines[f]]

    m['globals'] = {
        'extern decls in Engine (all)': count_re(eng_lines, r'^\s*extern\s+(?!"C")'),
        'extern decls in Engine .cpp files': count_re(eng_cpp_lines, r'^\s*extern\s+(?!"C")'),
    }
    m['legacy_c'] = {
        'NULL': count_re(flat, r'\bNULL\b'),
        'sprintf/strcpy/strcat/strncpy': count_re(flat, r'\b(sprintf|strcpy|strcat|strncpy)\s*\('),
        'malloc/calloc/realloc/free': count_re(flat, r'(?<![\w.>])(malloc|calloc|realloc|free)\s*\('),
        'fixed char buffers': count_re(flat, r'\bchar\s+\w+\s*\[\s*[A-Z0-9_]+\s*\]'),
        'new[] / delete[]': count_re(flat, r'\bnew\s+[\w:]+\s*\[|\bdelete\s*\[\s*\]'),
        'goto': count_re(flat, r'\bgoto\s+\w+\s*;'),
        'using namespace in headers': count_re(hdr_lines, r'^\s*using\s+namespace\b'),
        'tab-indented lines': count_re(flat, r'^\t'),
        'old-style include guards': sum(1 for f in src_cpp if f.endswith('.h') and re.search(r'^#ifndef\s+_', raw[f], re.M)),
        '#pragma once headers': sum(1 for f in src_cpp if f.endswith('.h') and '#pragma once' in raw[f]),
    }
    m['markers'] = {
        'TODO': count_re(flat, r'\bTODO\b'),
        'FIXME': count_re(flat, r'\bFIXME\b'),
        'CHECKME': count_re(flat, r'\bCHECKME\b'),
        'deprecated/obsolete/unused mentions': count_re(flat, r'(?i)\b(deprecated|obsolete|unused)\b'),
        'commented-out code lines': count_re(flat, r'^\s*//\s*[A-Za-z_][\w.\[\]>-]*\s*(\(.*\)|=.*);\s*$'),
        'C# TODO/FIXME/HACK': sum(count_re(raw[f].split('\n'), r'\b(TODO|FIXME|HACK)\b') for f in cs),
        'C# catch-all handlers': sum(count_re(raw[f].split('\n'), r'catch\s*(\(\s*Exception[^)]*\))?\s*$') for f in cs),
    }
    m['platforms'] = {
        'AGS_PLATFORM_OS_PSP refs': count_re(flat, r'AGS_PLATFORM_OS_PSP'),
        'allegro.h includes': count_re(flat, r'#include\s*[<"]allegro'),
        'wgt2allg includes': count_re(flat, r'#include\s*"util/wgt2allg\.h"'),
    }
    # Fork supports game data from 3.99 onward only (PLAN.md D1); anything older is dead
    m['data_versions'] = {
        'legacy version checks (pre-3.99)': count_re(flat, r'\bkGameVersion_(2\d\d|3[0-8]\d|39[0-8])\w*'),
        'files with legacy version checks': sum(
            1 for f in src_cpp if f.endswith('.cpp')
            and re.search(r'\bkGameVersion_(2\d\d|3[0-8]\d|39[0-8])\w*', raw[f])),
    }

    funcs = []
    for f in src_cpp:
        if not f.endswith(('.cpp', '.c')):
            continue
        code = strip_comments_and_strings(raw[f]).split('\n')
        for length, nesting, line, name in scan_functions(f, code):
            funcs.append((length, nesting, f, line, name))
    funcs.sort(reverse=True)
    m['functions'] = {
        'functions scanned': len(funcs),
        '> 100 lines': sum(1 for x in funcs if x[0] > 100),
        '> 200 lines': sum(1 for x in funcs if x[0] > 200),
        'nesting depth >= 6': sum(1 for x in funcs if x[1] >= 6),
        'script API wrappers (Sc_*)': sum(1 for x in funcs if x[4].startswith('Sc_')),
    }
    # Known-bad identifiers inherited from the DOS-era code; the rename phase drives these to zero
    tokens = collections.Counter(t for f in src_cpp for t in TOK_RE.findall(strip_comments_and_strings(raw[f])))
    m['cryptic_names'] = {n: tokens[n] for n in CRYPTIC_WATCHLIST if tokens[n]}
    m['cryptic_names']['(total)'] = sum(tokens[n] for n in CRYPTIC_WATCHLIST)

    m['tests'] = {
        'C++ test files': sum(1 for f in cpp if re.search(r'/test/.*_test\.cpp$', f)),
        'Engine C++ test files': sum(1 for f in cpp if f.startswith('Engine/test/') and f.endswith('_test.cpp')),
        'C# test files': sum(1 for f in cs if '.Tests/' in f),
    }
    return m, funcs, raw, cpp


def dead_candidates(raw, cpp):
    """Function definitions whose name never appears outside their own definition/declaration."""
    def_roots = ('Engine/', 'Common/', 'Compiler/script2/', 'Compiler/preproc/', 'Tools/')
    tokens = collections.Counter()
    header_tokens = collections.Counter()
    defs = []
    for f in cpp:
        txt = raw[f]
        toks = TOK_RE.findall(txt)
        tokens.update(toks)
        if f.endswith(('.h', '.hpp')):
            header_tokens.update(set(toks))
        if f.endswith(('.cpp', '.c')) and f.startswith(def_roots) and '/test' not in f:
            lines = txt.split('\n')
            for i, line in enumerate(lines):
                dm = DEF_RE.match(line)
                if dm and i + 1 < len(lines) and (line.rstrip().endswith('{') or lines[i + 1].strip().startswith('{')):
                    name = dm.group(1).split('::')[-1]
                    if name not in NOT_FUNCS and not name.startswith(('Sc_', '~')):
                        defs.append((f, i + 1, name))
    seen = set()
    out = []
    for f, line, name in sorted(defs):
        if name in seen:
            continue
        seen.add(name)
        # definition (+ one declaration in a header) is all there is
        if tokens[name] - min(header_tokens[name], 1) <= 1:
            out.append((f, line, name))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--list-long', type=int, default=25, help='how many longest functions to list')
    ap.add_argument('--list-dead', type=int, default=0, help='how many dead-code candidates to list')
    args = ap.parse_args()

    if not os.path.isdir('.git'):
        sys.exit('Run from the repository root.')
    files = tracked_files()
    metrics, funcs, raw, cpp = collect(files)
    dead = dead_candidates(raw, cpp)
    metrics['functions']['dead-code candidates (unverified)'] = len(dead)
    commit = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()

    if args.json:
        metrics['commit'] = commit
        metrics['longest'] = [dict(lines=l, nesting=d, file=f, line=ln, name=n) for l, d, f, ln, n in funcs[:args.list_long]]
        print(json.dumps(metrics, indent=2))
        return

    print(f'# Refactor metrics @ {commit}\n')
    for section, values in metrics.items():
        print(f'## {section}\n')
        print('| metric | value |\n|---|---:|')
        for k, v in values.items():
            print(f'| {k} | {v} |')
        print()
    print(f'## longest functions (top {args.list_long})\n')
    print('| lines | nesting | location |\n|---:|---:|---|')
    for length, nesting, f, line, name in funcs[:args.list_long]:
        print(f'| {length} | {nesting} | `{f}:{line}` {name} |')
    if args.list_dead:
        print(f'\n## dead-code candidates (first {args.list_dead}, verify before deleting)\n')
        for f, line, name in dead[:args.list_dead]:
            print(f'- `{f}:{line}` {name}')


if __name__ == '__main__':
    main()
