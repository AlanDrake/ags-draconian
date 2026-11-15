#!/usr/bin/env python3
"""
Computes code-health metrics tracked by docs/refactor/PLAN.md.

Run from the repository root:
    python Script/refactor_metrics.py            # markdown report to stdout
    python Script/refactor_metrics.py --json     # machine-readable, for diffing between runs
    python Script/refactor_metrics.py --list-long 40 --list-dead 200
    python Script/refactor_metrics.py --ninja-log <build dir>/.ninja_log   # + compile time per directory

All analysis is heuristic (regex + brace matching, no real C++ parser). Numbers are meant
for tracking trends between runs, not as exact truth. Dead-code candidates in particular
must be verified (linker gc-sections output, grep, tests) before deleting anything.
"""
import argparse
import collections
import json
import os
import posixpath
import re
import statistics
import subprocess
import sys

# Directories below the top level that --ninja-log reports besides the top-level ones (phase 5 works there)
COMPILE_TIME_DIRS = ['Engine/game', 'Engine/game/script_api']
# Third-party / generated code: never measured, never reformatted
THIRD_PARTY = re.compile(r'(^|/)(libsrc|libinclude|googletest|_deps|build|bin|obj|packages|References'
                         r'|scintilla|ScintillaNET)(/|$)'
                         r'|^Plugins/agslua/agslua/(lualibhelp|zlib)')
CPP_EXT = ('.cpp', '.c', '.h', '.hpp', '.inl')
MODULES = ['Common', 'Engine', 'Compiler', 'Tools', 'Plugins', 'Editor']

SIG_RE = re.compile(r'^(?:static\s+|inline\s+)*[A-Za-z_][\w:<>,\*&\s~]*?\b([A-Za-z_][\w:~]*)\s*\(([^;{}]*)\)\s*'
                    r'(?:const)?\s*(?:override)?\s*(?:\{\s*)?$')
DEF_RE = re.compile(r'^(?:static\s+|inline\s+)*[A-Za-z_][\w:<>,\*&\s]*?[\s\*&]([A-Za-z_]\w*(?:::[A-Za-z_~]\w*)?)\s*\([^;]*$')
TOK_RE = re.compile(r'[A-Za-z_]\w*')
CRYPTIC_WATCHLIST = ['chaa', 'chap', 'chin', 'chinfo', 'char1', 'chi', 'objj', 'guio', 'butt', 'sds', 'eres', 'tqs',
                     'scam', 'texx', 'widd', 'aschar', 'iit', 'vii', 'xxx', 'yyy', 'xx', 'yy', 'fil', 'scv', 'guin',
                     'butid', 'guisl', 'tehtext', 'ddd', 'abuf', 'thisroom', 'croom', 'objs', 'scsystem', 'usetup',
                     'charid', 'objid', 'invi', 'tehobj', 'newval', 'newwid', 'newhit']
NOT_FUNCS = {'if', 'for', 'while', 'switch', 'return', 'else', 'catch', 'main', 'WinMain', 'operator', 'sizeof'}

# Dead-code candidates: tokens in the code (not comments) of these files count as uses too: Objective-C(++)
# calls C and C++ functions directly. Java does not count: a native method's C side is named
# Java_<package>_<class>_<method> (JNI), never <method>, and the Android methods named like the iOS port's
# functions (writeConfigFile, ...) would hide them. C# does not count either: it reaches first-party C++
# only through AGS.Native (C++/CLI, read with the C++ files) and has no P/Invoke into AGS code.
REF_EXT = ('.mm', '.m')
RAW_STRING_RE = re.compile(r'R"([^(\s]*)\(.*?\)\1"', re.S)
VIRTUAL_RE = re.compile(r'\b(?:virtual|AGSIFUNC\s*\([^()]*\))\s+[^;(){}]*?\b([A-Za-z_]\w*)\s*\(')
OVERRIDE_RE = re.compile(r'\b([A-Za-z_]\w*)\s*\([^;{}()]*(?:\([^;{}()]*\)[^;{}()]*)*\)\s*(?:const\s*)?'
                         r'(?:noexcept\s*)?(?:override|final)\b')
# Dead by every local measure (PLAN.md 3.5: the gc-sections builds with GCC and MSVC, the grep) and kept on
# purpose: pattern (searched in "file:name"), reason. `--list-dead` prints them apart from the candidates.
DEAD_CODE_EXCEPTIONS = [
    (r'^Engine/platform/ios/', 'built only on iOS (inside #if AGS_PLATFORM_OS_IOS), not verified locally; the iOS '
                               'settings UI that called it is not in the tree'),
    (r'^Common/util/stream\.cpp:ReadAndConvertArrayOfUInt24$',
     "the big-endian half of Stream's array reads (BITBYTE_BIG_ENDIAN, util/stream.h); no supported target is "
     'big-endian, and stream.h calls it as ReadAndConvertArrayOfInt24, so that half does not compile'),
]


def tracked_files():
    out = subprocess.run(['git', 'ls-files'], capture_output=True, text=True, check=True).stdout
    return [f for f in out.splitlines() if not THIRD_PARTY.search(f) and os.path.isfile(f)]


def strip_comments_and_strings(src):
    # raw string literals first (the shaders' source in the graphics drivers): they may hold // and /*
    src = re.sub(r'(?<![\w])(?:u8|u|U|L)?R"([^()\\\s]{0,16})\((.*?)\)\1"',
                 lambda m: '""' + '\n' * m.group(0).count('\n'), src, flags=re.S)
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


def virtual_members(raw, cpp):
    """Names declared virtual (`virtual`, the plugin API's `AGSIFUNC`) or `override` anywhere: a member of
    that name is called through its vtable (by plugins, for the plugin interfaces), which names it nowhere."""
    names = set()
    for f in cpp:
        txt = strip_comments_and_strings(raw[f])
        names.update(m.group(1) for m in VIRTUAL_RE.finditer(txt))
        names.update(m.group(1) for m in OVERRIDE_RE.finditer(txt))
    return names


def dead_candidates(raw, cpp, files):
    """Function definitions whose name never appears outside their own definition/declaration.
    Returns (listed, kept): kept are those DEAD_CODE_EXCEPTIONS names, as (file, line, name, reason)."""
    def_roots = ('Engine/', 'Common/', 'Compiler/script/', 'Compiler/preproc/', 'Tools/')
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
            # a raw string holds shader code, whose functions are not C++ (rgb2hsv in ali3dogl.cpp)
            lines = RAW_STRING_RE.sub(lambda m: '\n' * m.group(0).count('\n'), txt).split('\n')
            for i, line in enumerate(lines):
                dm = DEF_RE.match(line)
                if dm and i + 1 < len(lines) and (line.rstrip().endswith('{') or lines[i + 1].strip().startswith('{')):
                    name = dm.group(1).split('::')[-1]
                    if name not in NOT_FUNCS and not name.startswith(('Sc_', '~')):
                        defs.append((f, i + 1, name, '::' in dm.group(1)))
    for f in files:
        if f.endswith(REF_EXT):
            with open(f, encoding='utf-8', errors='replace') as fh:
                tokens.update(TOK_RE.findall(strip_comments_and_strings(fh.read())))
    virtual = virtual_members(raw, cpp)
    seen = set()
    listed, kept = [], []
    for f, line, name, member in sorted(defs):
        if name in seen:
            continue
        seen.add(name)
        # definition (+ one declaration in a header) is all there is
        if tokens[name] - min(header_tokens[name], 1) > 1 or (member and name in virtual):
            continue
        reason = next((r for pattern, r in DEAD_CODE_EXCEPTIONS if re.search(pattern, f'{f}:{name}')), None)
        if reason:
            kept.append((f, line, name, reason))
        else:
            listed.append((f, line, name))
    return listed, kept


def read_ninja_log(path):
    """{output path: milliseconds} of the compile steps in a ninja log (v5 and v6: start, end, mtime, output,
    hash, tab-separated). A log has one line per run of a step; the last one of an output wins."""
    times = {}
    with open(path, encoding='utf-8', errors='replace') as fh:
        for line in fh:
            parts = line.rstrip('\n').split('\t')
            if line.startswith('#') or len(parts) < 4 or not (parts[0].isdigit() and parts[1].isdigit()):
                continue
            times[parts[3].replace('\\', '/')] = int(parts[1]) - int(parts[0])
    return times


def object_sources(build_dir):
    """{object path relative to the build directory: (source path relative to the source directory)} from
    the compile_commands.json CMake wrote there (-DCMAKE_EXPORT_COMPILE_COMMANDS=ON). A source outside the
    source directory (generated, fetched into the build tree) maps to None. Plain string work, so that a log
    and a database made on another system (a WSL build read on Windows) read the same. Returns None without
    the database."""
    db = os.path.join(build_dir, 'compile_commands.json')
    if not os.path.isfile(db):
        return None
    root = top = ''
    cache = os.path.join(build_dir, 'CMakeCache.txt')
    if os.path.isfile(cache):
        with open(cache, encoding='utf-8', errors='replace') as fh:
            for line in fh:
                if line.startswith('CMAKE_HOME_DIRECTORY:'):
                    root = line.split('=', 1)[1].strip().replace('\\', '/').rstrip('/') + '/'
                elif line.startswith('CMAKE_CACHEFILE_DIR:'):
                    top = line.split('=', 1)[1].strip().replace('\\', '/').rstrip('/') + '/'
    root = root or os.getcwd().replace('\\', '/').rstrip('/') + '/'

    def under(path, base):
        # a drive letter means a Windows path: its case does not count
        windows = ':' in base[:3]
        if path.lower().startswith(base.lower()) if windows else path.startswith(base):
            return path[len(base):]
        return None

    with open(db, encoding='utf-8') as fh:
        entries = json.load(fh)
    sources = {}
    for e in entries:
        if 'output' not in e:
            return None
        base = e['directory'].replace('\\', '/').rstrip('/') + '/'
        out = e['output'].replace('\\', '/')
        out = under(out, base) or out
        src = e['file'].replace('\\', '/')
        if not (src.startswith('/') or ':' in src[:3]):
            src = base + src
        # fetched and generated sources live under the build directory, which may sit inside the source one
        sources[out] = None if under(src, top or base) is not None else under(src, root)
    return sources


def compile_times(log_path):
    """Compile time of the first-party objects in a ninja log, from the CMake build directory next to it.
    A step counts when it is an object of compile_commands.json whose source is under the source directory
    and not third-party code (THIRD_PARTY); links, archives and generated steps are not objects of it. Without
    the database, the path of the object decides: CMake puts the object of <dir>/<file>.cpp, for a target
    defined in <tdir>, at <tdir>/CMakeFiles/<target>.dir/<dir>/<file>.cpp.<obj|o> (a source outside <tdir>,
    `../Common/x.cpp`, as `__/Common/x.cpp`); a target of a fetched dependency is under _deps/. The source is
    then <tdir>/<dir>/<file>.cpp, right for the source tree CMake builds here and not otherwise (a source
    given by an absolute path elsewhere has no relative path: it is left out). Only sources under the
    MODULES directories count, which leaves out the targets of fetched dependencies (`<name>_content-build/`).
    A source compiled into several targets counts once per object: the unit is the object."""
    times = read_ninja_log(log_path)
    sources = object_sources(os.path.dirname(os.path.abspath(log_path)))
    objects = []  # (source path, milliseconds)
    if sources is None:
        sys.stderr.write('no usable compile_commands.json next to the log: sources guessed from object paths\n')
        for out, ms in times.items():
            m = re.match(r'(?:(.*)/)?CMakeFiles/[^/]+\.dir/(.+)\.(?:obj|o)$', out)
            if not m or out.startswith('_deps/'):
                continue
            # the target's directory + the source's path in it, `__/` standing for `../`
            src = posixpath.normpath(posixpath.join(m.group(1) or '', m.group(2).replace('__/', '../')))
            if src.split('/', 1)[0] in MODULES and not THIRD_PARTY.search(src):  # not a fetched dependency's target
                objects.append((src, ms))
    else:
        objects = [(src, times[out]) for out, src in sources.items()
                   if out in times and src and out.endswith(('.obj', '.o')) and not THIRD_PARTY.search(src)]
    seconds = collections.defaultdict(list)
    for src, ms in objects:
        seconds[src].append(ms / 1000)

    def row(values):
        return {'objects': len(values), 'total_s': round(sum(values), 1),
                'median_s': round(statistics.median(values), 2)} if values else None

    allv = [v for vs in seconds.values() for v in vs]
    result = {'all': row(allv), 'directories': {}}
    # every top-level directory, and the ones phase 5 works on
    names = sorted({p.split('/', 1)[0] for p in seconds if '/' in p}) + COMPILE_TIME_DIRS
    for d in names:
        r = row([v for p, vs in seconds.items() if p.startswith(d + '/') for v in vs])
        if r:
            result['directories'][d] = r
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--json', action='store_true')
    ap.add_argument('--ninja-log', help='.ninja_log of a CMake/Ninja build (with its compile_commands.json beside '
                                        'it): report the compile time of the first-party objects')
    ap.add_argument('--list-long', type=int, default=25, help='how many longest functions to list')
    ap.add_argument('--list-dead', type=int, default=0, help='how many dead-code candidates to list')
    args = ap.parse_args()

    if not os.path.isdir('.git'):
        sys.exit('Run from the repository root.')
    files = tracked_files()
    metrics, funcs, raw, cpp = collect(files)
    dead, kept = dead_candidates(raw, cpp, files)
    metrics['functions']['dead-code candidates (unverified)'] = len(dead)
    metrics['functions']['dead-code exceptions (documented)'] = len(kept)
    commit = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True).stdout.strip()

    ctime = compile_times(args.ninja_log) if args.ninja_log else None

    if args.json:
        if ctime:
            metrics['compile_time'] = ctime
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
    if ctime:
        print('## compile time (first-party objects of the ninja log, seconds of each step)\n')
        print('| directory | objects | total s | median s |\n|---|---:|---:|---:|')
        for name, r in [('all', ctime['all']), *ctime['directories'].items()]:
            print(f"| {name} | {r['objects']} | {r['total_s']} | {r['median_s']} |")
        print()
    print(f'## longest functions (top {args.list_long})\n')
    print('| lines | nesting | location |\n|---:|---:|---|')
    for length, nesting, f, line, name in funcs[:args.list_long]:
        print(f'| {length} | {nesting} | `{f}:{line}` {name} |')
    if args.list_dead:
        print(f'\n## dead-code candidates (first {args.list_dead}, verify before deleting)\n')
        for f, line, name in dead[:args.list_dead]:
            print(f'- `{f}:{line}` {name}')
        if kept:
            print(f'\n## dead-code exceptions ({len(kept)}, DEAD_CODE_EXCEPTIONS)\n')
            for f, line, name, reason in kept:
                print(f'- `{f}:{line}` {name}: kept, documented: {reason}')


if __name__ == '__main__':
    main()
