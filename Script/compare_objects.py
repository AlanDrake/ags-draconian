#!/usr/bin/env python3
"""
Verifies that a commit is purely mechanical (formatting, local renames, moving code around inside
a function) by building the code before and after it and comparing the object files.

    python Script/compare_objects.py <base-rev> [<new-rev>]

<new-rev> defaults to the current working tree (tracked files, including uncommitted changes),
so a change can be checked before it is committed. Typical uses:

    python Script/compare_objects.py HEAD            # uncommitted changes vs HEAD
    python Script/compare_objects.py HEAD~1 HEAD     # the last commit

How it works (see docs/refactor/STYLE.md, "Commits"):
  * Both revisions are built from the same git worktree and into the same build directory
    (default: .compare/ in the repository root), so __FILE__ and include paths are identical,
    and the second build only recompiles what the change touches.
  * The build is MinSizeRel: optimized, NDEBUG (asserts compile away), no debug info and no LTO.
    MSVC gets /Brepro so object files carry no timestamps.
  * Every object listed in compile_commands.json is hashed in both builds and compared.

Exit code 0 means no unexpected difference. A differing object in a GoogleTest file
(*_test.cpp) is reported but tolerated, because the test macros record __LINE__.
An object that differs only in its debug-info section or in the order of its functions (which is what
reordering includes does) is told apart by comparing the disassembly, function by function (needs
dumpbin, so MSVC only); it is reported as "code identical" and tolerated. The disassembly is compared with
MSVC's lambda names (<lambda_hash>, a hash of the lambda's body) read as one name, <lambda>: renaming a local
that a lambda uses changes the hash, not the code.
Expected, by design, to differ: renamed functions, types or globals (their symbol names are in
the objects), and changed string literals.
A [delete] commit of unreferenced functions changes objects by exactly the functions it removes: when no
object was added, the differing and removed objects (tests aside) are compared function by function, and
if functions were only removed, none changed or added, the result is "deletion: N functions removed, 0
changed, 0 added (mechanical)", exit code 0. Data (string literals, tables) is not compared by this step.

Moves (MSVC only, needs dumpbin):
  * Moved and renamed source files. An object whose source moved is paired with its old object: the pairs
    come from `git diff --name-status -M` between the two revisions (any similarity, since the moved files'
    #include lines change too), and an explicit --map <tsv> (old path, a tab, new path per line; lines
    starting with # are comments) wins over git's guess for the paths it names. A pair is compared as an
    unmoved object is (bytes, else the disassembly). What an MSVC object records of its own path: the object
    file's path (the S_OBJNAME record of .debug$S, outside the code, so it only makes the bytes differ) and the
    hash in the names of anonymous-namespace symbols (?A0x<hash>, a hash of the source path), which the
    disassembly reads as ?A0x. First-party code built here has no __FILE__; GoogleTest objects record it, and
    a moved test object that differs is tolerated as a test object in place is. When every pair has the same
    code and nothing else differs: "move: K objects moved, code identical (mechanical)", exit code 0.
  * Functions moved between objects. When objects were added, moved with a code difference, or a [delete]
    comparison found functions added somewhere, the functions of every object that differs, was added or
    was removed (test objects in place aside, as above), moved ones included, are pooled per side. A
    function is its symbol name and its disassembly, normalised as above. Call targets and data references
    print as symbol names, so a callee now in another object, or a static function made external, reads the
    same; string literals are named by their content (??_C@); jump tables and local labels are $LN labels,
    read without their number. How the copies are counted follows the COMDAT selection MSVC gives the
    function's section (every function is a COMDAT under /O1, which implies /Gy): an ordinary function,
    static ones included, is "no duplicates" and must keep the same number of copies with the same code;
    an inline or template function is "pick any", has a copy in each object that calls it and is not
    inlined there, and moving a caller moves that copy along, so only the set of its codes is compared,
    not the number of copies. A function dropped, duplicated (the linker refuses an external duplicate
    anyway), changed or added is printed with the objects it is in, exit code 1; otherwise "move: N
    functions moved between objects, 0 changed, 0 added, 0 removed (mechanical)", exit code 0, prefixed by
    "K objects moved, code identical; " when files moved too.
  * Not compared by the move check: data. The Register*API tables (pairs of a script name and a function
    pointer) are data built at run time from the moved code; the engine tests ScriptExports.* check them.
    String literals, globals and their initial values are not compared either.
  * Without dumpbin (GCC and Clang builds) the tool says that it cannot give a move verdict.

    python Script/compare_objects.py --self-test   # the pool comparison and the --map parser
"""
import argparse
import collections
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
# No precompiled header (PLAN.md 2.10b): a missing #include must fail, and the objects are the ones of
# the files as written
CMAKE_ARGS = ['-DCMAKE_BUILD_TYPE=MinSizeRel', '-DAGS_BUILD_TOOLS=1', '-DAGS_TESTS=1',
              '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON']


def run(cmd, env=None, cwd=None, capture=False, check=True):
    kw = dict(env=env, cwd=cwd, text=True)
    if capture:
        kw.update(stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    res = subprocess.run(cmd, **kw)
    if check and res.returncode != 0:
        if capture:
            sys.stderr.write(res.stdout[-4000:])
        sys.exit(f'Command failed ({res.returncode}): {cmd if isinstance(cmd, str) else " ".join(cmd)}')
    return res


def git(*args, cwd=REPO):
    return run(['git', *args], cwd=cwd, capture=True).stdout.strip()


def msvc_env():
    """Environment of a VS developer prompt (x64), so that Ninja finds cl.exe, cmake and ninja."""
    vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
                           'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if not os.path.isfile(vswhere):
        sys.exit('vswhere.exe not found: install Visual Studio with the C++ workload')
    vs = run([vswhere, '-latest', '-products', '*', '-requires',
              'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
             capture=True).stdout.strip()
    vcvars = os.path.join(vs, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat')
    # one string, so the nested quotes reach cmd.exe unchanged
    out = run(f'cmd /s /c ""{vcvars}" x64 >nul && set"', capture=True).stdout
    env = dict(line.split('=', 1) for line in out.splitlines() if '=' in line)
    env['CFLAGS'] = env['CXXFLAGS'] = '/Brepro'
    return env


def build_env():
    if os.name == 'nt':
        return msvc_env()
    env = dict(os.environ)
    env['CFLAGS'] = env['CXXFLAGS'] = '-g0'
    return env


def resolve_new(rev):
    if rev:
        return git('rev-parse', '--verify', rev + '^{commit}'), rev
    snapshot = git('stash', 'create')  # commit object of the working tree, leaves it untouched
    if snapshot:
        return snapshot, 'working tree'
    return git('rev-parse', 'HEAD'), 'working tree (clean, same as HEAD)'


def checkout(worktree, commit):
    if not os.path.isdir(worktree):
        os.makedirs(os.path.dirname(worktree), exist_ok=True)
        git('worktree', 'add', '--detach', worktree, commit)
    else:
        git('checkout', '--quiet', '--detach', '--force', commit, cwd=worktree)
        git('clean', '-fdq', cwd=worktree)


def tool(name, env):
    # Windows looks programs up in the parent's PATH, not in the PATH of the env we pass
    path = shutil.which(name, path=env.get('PATH') or env.get('Path'))
    if not path:
        sys.exit(f'{name} not found')
    return path


def build(src, bdir, env):
    cmake = tool('cmake', env)
    if not os.path.isfile(os.path.join(bdir, 'build.ninja')):
        run([cmake, '-S', src, '-B', bdir, '-G', 'Ninja', *CMAKE_ARGS], env=env, capture=True)
    run([cmake, '--build', bdir], env=env, capture=True)


def hash_objects(src, bdir, keep_dir=None):
    """Returns {object path: (sha256, source path relative to the worktree)}.
    With keep_dir, also copies each object there (for the disassembly comparison)."""
    with open(os.path.join(bdir, 'compile_commands.json'), encoding='utf-8') as fh:
        entries = json.load(fh)
    result = {}
    src_norm = os.path.normcase(os.path.abspath(src))
    for e in entries:
        obj = e.get('output')
        if not obj:
            sys.exit('compile_commands.json has no "output" field: CMake 3.20+ is required')
        obj_path = os.path.join(e['directory'], obj)
        source = os.path.abspath(os.path.join(e['directory'], e['file']))
        # normcase only for matching (it lowercases on Windows), keep the real spelling for output
        if not os.path.normcase(source).startswith(src_norm + os.sep):
            continue  # generated or external source
        rel = source[len(src_norm) + 1:].replace('\\', '/')
        with open(obj_path, 'rb') as fh:
            data = fh.read()
        result[os.path.normcase(obj)] = (hashlib.sha256(data).hexdigest(), rel)
        if keep_dir:
            os.makedirs(keep_dir, exist_ok=True)
            with open(os.path.join(keep_dir, hashlib.sha1(os.path.normcase(obj).encode()).hexdigest()), 'wb') as out:
                out.write(data)
    return result


_dumpbin_cache = {}


def dumpbin_text(dumpbin, env, obj_path, option):
    key = (option, obj_path)
    if key not in _dumpbin_cache:
        text = subprocess.run([dumpbin, '/nologo', option, obj_path], env=env, capture_output=True,
                              text=True, errors='replace').stdout
        _dumpbin_cache[key] = normalise_names(text)
    return _dumpbin_cache[key]


def normalise_names(text):
    # MSVC names a lambda's type after a hash of its body (<lambda_1f0c...>), so renaming a local that a lambda
    # uses renames the lambda and everything instantiated for it: compare the code under one name, <lambda>
    text = re.sub(r'<lambda_[0-9a-f]{32}>', '<lambda>', text)
    # an anonymous namespace is named after a hash of the source path (?A0x1f0c2d3e), which moving the file
    # changes: a name, not code
    return re.sub(r'\?A0x[0-9a-f]{8}', '?A0x', text)


def disassembly_functions(dumpbin, env, obj_path):
    """The code of an object as a list of (function label, instructions), without addresses."""
    text = dumpbin_text(dumpbin, env, obj_path, '/disasm:nobytes')
    found, label = [], '<start>'
    for line in text.splitlines():
        if line.startswith('Dump of file') or not line.strip():
            continue
        line = line.strip()
        if line == 'Summary':  # the section sizes follow; not code of the last function
            break
        # a compiler-generated local label inside a function ($LN<n>, numbered per object, so removing a function
        # renumbers the labels of the functions after it): part of the function, without its number
        if not found:
            found.append((label, []))
        if re.match(r'^\$LN\d+:$', line):
            found[-1][1].append('$LN:')
            continue
        line = re.sub(r'\$LN\d+\b', '$LN', line)
        if re.match(r'^[^\s:]+:$', line) or re.match(r'^\S+ \(.*\):$', line):  # a symbol's label
            label = line
            found.append((label, []))
            continue
        if re.match(r'^[0-9A-F]+ \.[\w$]+$', line):  # the section sizes of the summary: layout, not code
            continue
        line = re.sub(r'^[0-9A-F]{8}:\s*', '', line)
        line = re.sub(r'\b[0-9A-F]{8}\b', 'ADDR', line)  # jump targets and offsets inside the object
        found[-1][1].append(line)
    return found


def disassembly_blocks(dumpbin, env, obj_path):
    """The code of an object as {function label: instructions}, without addresses, so that the order of
    the functions and the debug-info section do not matter."""
    found = disassembly_functions(dumpbin, env, obj_path)
    # Labels that occur more than once (the lambdas of one function, now under one name) keep their code apart:
    # sorted, so that the order the compiler put them in does not matter
    grouped = {}
    for label, lines in found:
        grouped.setdefault(label, []).append(lines)
    blocks = {}
    for label, bodies in grouped.items():
        for i, lines in enumerate(sorted(bodies)):
            blocks[label if i == 0 else f'{label} #{i + 1}'] = lines
    return blocks


def same_code(dumpbin, env, old_path, new_path):
    return disassembly_blocks(dumpbin, env, old_path) == disassembly_blocks(dumpbin, env, new_path)


def function_changes(dumpbin, env, old_path, new_path):
    """The functions of an object that a change removed, added and changed, as three sorted lists of
    labels; new_path None means the object is gone, so all its functions were removed."""
    old = disassembly_blocks(dumpbin, env, old_path)
    new = disassembly_blocks(dumpbin, env, new_path) if new_path else {}
    return (sorted(k for k in old if k not in new), sorted(k for k in new if k not in old),
            sorted(k for k in old if k in new and old[k] != new[k]))


def deletion_verdict(dumpbin, env, objects):
    """For a [delete] commit: compares each (source, old object, new object or None) function by function.
    Prints what each object lost, gained or changed; returns (True when functions were only removed,
    the number of functions added)."""
    totals = [0, 0, 0]
    print('\nfunction by function:')
    for source, old_path, new_path in objects:
        changes = function_changes(dumpbin, env, old_path, new_path)
        for i, items in enumerate(changes):
            totals[i] += len(items)
        print(f'  {source}: removed {len(changes[0])}, added {len(changes[1])}, changed {len(changes[2])}')
        for title, items in zip(('removed', 'added', 'changed'), changes):
            for label in items:
                print(f'    {title}: {label}')
    removed, added, changed = totals
    if removed and not added and not changed:
        print(f'\nRESULT: deletion: {removed} functions removed, 0 changed, 0 added (mechanical)')
        return True, 0
    print(f'\n{removed} functions removed, {changed} changed, {added} added')
    return False, added


def function_kinds(dumpbin, env, obj_path):
    """{symbol name: True when the function must keep its number of copies} from the symbol table: False for a
    function in a COMDAT section other than "no duplicates" (pick any: inline and template functions), and
    for a funclet (.text$x) whose parent section is one."""
    selection, parent, functions = {}, {}, {}
    section = None
    for line in dumpbin_text(dumpbin, env, obj_path, '/symbols').splitlines():
        m = re.match(r'^[0-9A-F]+ [0-9A-F]+ SECT([0-9A-F]+) +notype +Static +\| \.', line)
        if m:
            section = int(m.group(1), 16)
            continue
        m = re.match(r'^ +Section length .*selection +(\d+)(?: \(pick associative Section 0x([0-9A-F]+)\))?', line)
        if m and section is not None:
            selection[section] = int(m.group(1))
            if m.group(2):
                parent[section] = int(m.group(2), 16)
            section = None
            continue
        section = None
        m = re.match(r'^[0-9A-F]+ [0-9A-F]+ SECT([0-9A-F]+) +notype \(\) +(?:External|Static) +\| (\S+)', line)
        if m:
            functions[m.group(2)] = int(m.group(1), 16)

    def counted(sec, depth=0):
        if parent.get(sec) is not None and depth < 8:
            return counted(parent[sec], depth + 1)
        return selection.get(sec, 1) == 1  # no selection: not a COMDAT, one copy

    return {name: counted(sec) for name, sec in functions.items()}


def pool_functions(dumpbin, env, objects):
    """The functions of [(object name, object path)] as a list of (object name, label, code, counted)."""
    items = []
    for name, path in objects:
        kinds = function_kinds(dumpbin, env, path)
        for label, lines in disassembly_functions(dumpbin, env, path):
            if label == '<start>' and all(line.startswith('File Type:') for line in lines):
                continue  # dumpbin's header, not a function
            symbol = label[:-1].split(' (', 1)[0]
            items.append((name, label, tuple(lines), kinds.get(symbol, True)))
    return items


def compare_pools(old, new):
    """Compares two pools of (object, label, code, counted) items. A label is counted when any of its copies is:
    its codes must match as a multiset, else (inline functions) as a set. Returns a dict of lists:
    'moved' [(label, old objects, new objects)] for counted functions found in other objects than before,
    'copies' [label] for inline functions found in other objects than before, and 'changed', 'added',
    'removed' [(label, old objects, new objects)] for the differences."""
    def by_label(items):
        found = {}
        for obj, label, code, counted in items:
            found.setdefault(label, []).append((obj, code, counted))
        return found
    before, after = by_label(old), by_label(new)
    result = {key: [] for key in ('moved', 'copies', 'changed', 'added', 'removed')}
    for label in sorted(set(before) | set(after)):
        a, b = before.get(label, []), after.get(label, [])
        counted = any(c for _, _, c in a + b)
        codes_a = collections.Counter(code for _, code, _ in a)
        codes_b = collections.Counter(code for _, code, _ in b)
        objs_a = sorted(obj for obj, _, _ in a)
        objs_b = sorted(obj for obj, _, _ in b)
        entry = (label, objs_a, objs_b)
        if (codes_a == codes_b) if counted else (set(codes_a) == set(codes_b)):
            if counted and objs_a != objs_b:
                result['moved'].append(entry)
            elif not counted and set(objs_a) != set(objs_b):
                result['copies'].append(label)
        elif a and b and set(codes_a) != set(codes_b):
            result['changed'].append(entry)
        elif len(b) > len(a):
            result['added'].append(entry)
        else:
            result['removed'].append(entry)
    return result


def move_verdict(dumpbin, env, old_objects, new_objects, files_moved):
    """For a [move] commit: pools the functions of [(object name, object path)] on each side and compares the
    pools. Prints the moves and the differences; returns True when functions only moved."""
    pools = compare_pools(pool_functions(dumpbin, env, old_objects), pool_functions(dumpbin, env, new_objects))
    print('\nfunctions between objects:')
    groups = collections.Counter((', '.join(a), ', '.join(b)) for _, a, b in pools['moved'])
    for (a, b), n in sorted(groups.items()):
        print(f'  {a or "-"} -> {b or "-"}: {n} functions moved')
    if pools['copies']:
        print(f'  {len(pools["copies"])} inline functions have their copies in other objects than before '
              '(same code; inline copies are compared by code, not by number)')
    for title in ('changed', 'added', 'removed'):
        for label, a, b in pools[title]:
            print(f'  {title}: {label}')
            print(f'    before: {", ".join(a) or "-"}; after: {", ".join(b) or "-"}')
    moved, changed, added, removed = (len(pools[k]) for k in ('moved', 'changed', 'added', 'removed'))
    if not changed and not added and not removed:
        files = f'{files_moved} objects moved, code identical; ' if files_moved else ''
        print(f'\nRESULT: move: {files}{moved} functions moved between objects, 0 changed, 0 added, 0 removed '
              '(mechanical)')
        return True
    print(f'\n{moved} functions moved, {changed} changed, {added} added, {removed} removed')
    return False


def parse_map(text):
    """{old path: new path} from a --map file: one 'old<TAB>new' per line, # comments, blank lines; a third
    column (a note, as in docs/refactor/moves-5.3.tsv) is ignored."""
    result = {}
    for n, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        fields = [f.strip() for f in line.split('\t')]
        if len(fields) not in (2, 3) or not all(fields[:2]):
            sys.exit(f'--map line {n}: expected "old path<TAB>new path", got {line!r}')
        result[fields[0].replace('\\', '/')] = fields[1].replace('\\', '/')
    return result


def moved_sources(base, new, map_path):
    """{old source path: new source path} for the files moved or renamed between the revisions: git's
    rename detection, then the --map file, whose lines replace git's pairs for the paths they name."""
    renames = {}
    for line in git('diff', '--name-status', '-M', base, new).splitlines():
        fields = line.split('\t')
        if fields[0].startswith('R') and len(fields) == 3:
            renames[fields[1]] = fields[2]
    if map_path:
        with open(map_path, encoding='utf-8') as fh:
            explicit = parse_map(fh.read())
        targets = set(explicit.values())
        renames = {o: n for o, n in renames.items() if o not in explicit and n not in targets}
        renames.update(explicit)
    return renames


def target_of(obj):
    """The CMake target directory of an object path (engine.dir), to pair objects of the same target."""
    m = re.match(r'^(.*?\.dir)[\\/]', obj)
    return m.group(1) if m else ''


def pair_moved(before, after, renames):
    """[(old object, new object)] for the objects whose source file moved."""
    pairs, taken = [], set()
    added = [o for o in sorted(after) if o not in before]
    for o in sorted(before):
        if o in after or before[o][1] not in renames:
            continue
        wanted = renames[before[o][1]]
        candidates = [a for a in added if a not in taken and after[a][1] == wanted]
        same_target = [a for a in candidates if target_of(a) == target_of(o)]
        if len(same_target) == 1 or len(candidates) == 1:
            pairs.append((o, (same_target or candidates)[0]))
            taken.add(pairs[-1][1])
    return pairs


def self_test():
    a, b, c = ('mov eax,1', 'ret'), ('mov eax,2', 'ret'), ('call ?f@@YAXXZ', 'ret')
    # a move: Sc_F leaves x.cpp for x_api.cpp, the inline helper gets a second copy
    old = [('x.cpp', 'F:', a, True), ('x.cpp', 'Sc_F:', c, True), ('x.cpp', 'inl:', b, False)]
    new = [('x.cpp', 'F:', a, True), ('x_api.cpp', 'Sc_F:', c, True), ('x.cpp', 'inl:', b, False),
           ('x_api.cpp', 'inl:', b, False)]
    r = compare_pools(old, new)
    assert [m[0] for m in r['moved']] == ['Sc_F:'] and r['copies'] == ['inl:'], r
    assert not r['changed'] and not r['added'] and not r['removed'], r
    # a constant changed in the moved function
    r = compare_pools(old, [new[0], ('x_api.cpp', 'Sc_F:', b, True), new[2]])
    assert [m[0] for m in r['changed']] == ['Sc_F:'], r
    # dropped, and left behind as a duplicate
    r = compare_pools(old, [new[0], new[2]])
    assert [m[0] for m in r['removed']] == ['Sc_F:'] and not r['changed'], r
    r = compare_pools(old, new + [('x.cpp', 'Sc_F:', c, True)])
    assert [m[0] for m in r['added']] == ['Sc_F:'] and not r['moved'], r
    # an inline function whose code changed, one that disappeared
    r = compare_pools(old, [old[0], old[1], ('x.cpp', 'inl:', a, False)])
    assert [m[0] for m in r['changed']] == ['inl:'], r
    r = compare_pools(old, old[:2])
    assert [m[0] for m in r['removed']] == ['inl:'], r
    m = parse_map('# comment\n\nEngine/ac/a.cpp\tEngine/game/b.cpp\r\nEngine\\ac\\c.h\tEngine/x/c.h\n')
    assert m == {'Engine/ac/a.cpp': 'Engine/game/b.cpp', 'Engine/ac/c.h': 'Engine/x/c.h'}, m
    before = {'e/engine.dir/ac/a.cpp.obj': ('1', 'Engine/ac/a.cpp'), 'e/t.dir/ac/a.cpp.obj': ('1', 'Engine/ac/a.cpp')}
    after = {'e/engine.dir/game/b.cpp.obj': ('1', 'Engine/game/b.cpp'),
             'e/t.dir/game/b.cpp.obj': ('1', 'Engine/game/b.cpp')}
    assert pair_moved(before, after, m) == [('e/engine.dir/ac/a.cpp.obj', 'e/engine.dir/game/b.cpp.obj'),
                                            ('e/t.dir/ac/a.cpp.obj', 'e/t.dir/game/b.cpp.obj')]
    print('self-test passed')
    return 0


def is_third_party(path):
    return any(part in ('libsrc', 'libinclude', '_deps') for part in path.split('/'))


def is_line_sensitive(path):
    return path.endswith('_test.cpp') or '/test/' in path


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('base', nargs='?', help='revision before the change')
    ap.add_argument('new', nargs='?', help='revision after the change (default: working tree)')
    ap.add_argument('--work-dir', default=os.path.join(REPO, '.compare'),
                    help='where the worktree and build directory live (default: %(default)s)')
    ap.add_argument('--clean', action='store_true', help='delete the build directory first')
    ap.add_argument('--map', help='file of moved sources, "old path<TAB>new path" per line; wins over git')
    ap.add_argument('--self-test', action='store_true', help='test the pool comparison and the --map parser')
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not args.base:
        ap.error('the base revision is required')

    base = git('rev-parse', '--verify', args.base + '^{commit}')
    new, new_label = resolve_new(args.new)
    worktree = os.path.join(args.work_dir, 'src')
    bdir = os.path.join(args.work_dir, 'build')
    if args.clean and os.path.isdir(bdir):
        shutil.rmtree(bdir)
    env = build_env()

    print(f'base: {args.base} ({base[:10]})')
    print(f'new:  {new_label} ({new[:10]})')
    changed = git('diff', '--name-only', base, new).splitlines()
    print(f'{len(changed)} file(s) changed')

    print('building base...', flush=True)
    checkout(worktree, base)
    build(worktree, bdir, env)
    kept = os.path.join(args.work_dir, 'base-objects')
    if os.path.isdir(kept):
        shutil.rmtree(kept)
    before = hash_objects(worktree, bdir, kept)

    print('building new...', flush=True)
    checkout(worktree, new)
    build(worktree, bdir, env)
    after = hash_objects(worktree, bdir)

    def old_copy(o):
        return os.path.join(kept, hashlib.sha1(o.encode()).hexdigest())

    differ_objs = [o for o in after if o in before and before[o][0] != after[o][0]]
    # Objects that differ in bytes but not in code (debug-info section, function order)
    dumpbin = shutil.which('dumpbin', path=env.get('PATH') or env.get('Path'))
    code_identical = []
    if dumpbin:
        for o in differ_objs:
            if same_code(dumpbin, env, old_copy(o), os.path.join(bdir, o)):
                code_identical.append(o)
    # Objects whose source file moved, paired with their old object and compared the same way
    pairs = pair_moved(before, after, moved_sources(base, new, args.map))
    paired = {o for pair in pairs for o in pair}
    moved_same = [(o, n) for o, n in pairs if before[o][0] == after[n][0] or
                  (dumpbin and same_code(dumpbin, env, old_copy(o), os.path.join(bdir, n)))]
    moved_differ = [p for p in pairs if p not in moved_same]
    moved_unexpected = [p for p in moved_differ if not is_line_sensitive(after[p[1]][1])]
    differ = sorted(after[o][1] for o in differ_objs if o not in code_identical)
    added = sorted(after[o][1] for o in after if o not in before and o not in paired)
    removed = sorted(before[o][1] for o in before if o not in after and o not in paired)
    same = sum(1 for o in after if o in before and before[o][0] == after[o][0])
    same_code_list = sorted(after[o][1] for o in code_identical)

    unexpected = [s for s in differ if not is_line_sensitive(s)]
    tolerated = [s for s in differ if is_line_sensitive(s)]

    moved_note = f', {len(pairs)} moved ({len(moved_same)} code identical)' if pairs else ''
    print(f'\n{same} object(s) identical, {len(code_identical)} differ in bytes but not in code, '
          f'{len(differ)} differ, {len(added)} added, {len(removed)} removed{moved_note}')
    if same_code_list:
        print(f'\ncode identical (debug info or function order differs): {len(same_code_list)} object(s)')
    for title, items in (('DIFFER (code generation changed)', unexpected),
                         ('differ in test files (expected: test macros record __LINE__)', tolerated),
                         ('added', added), ('removed', removed)):
        if items:
            print(f'\n{title}:')
            for s in items:
                note = ' [third-party]' if is_third_party(s) else ''
                note += '' if s in changed else ' [source unchanged: affected through a header]'
                print(f'  {s}{note}')
    for title, items in (('moved, code identical', moved_same),
                         ('moved, code differs', moved_unexpected),
                         ('moved test files that differ (expected: test macros record __LINE__ and __FILE__)',
                          [p for p in moved_differ if p not in moved_unexpected])):
        if items:
            print(f'\n{title}:')
            for o, n in items:
                print(f'  {before[o][1]} -> {after[n][1]}')

    if unexpected or added or removed or moved_unexpected:
        functions_added = 0
        # A [delete] commit changes the objects by exactly the functions it removes
        if dumpbin and not added and not pairs:
            objects = [(after[o][1], old_copy(o), os.path.join(bdir, o)) for o in sorted(differ_objs)
                       if o not in code_identical and not is_line_sensitive(after[o][1])]
            objects += [(before[o][1], old_copy(o), None) for o in sorted(before)
                        if o not in after and not is_line_sensitive(before[o][1])]
            deleted_only, functions_added = deletion_verdict(dumpbin, env, objects)
            if deleted_only:
                return 0
        # A [move] commit moves functions between objects: the same functions, in other objects
        if added or pairs or functions_added:
            if not dumpbin:
                print('\nno move verdict: comparing functions between objects needs dumpbin, so an MSVC build; '
                      'this build cannot prove a move')
            else:
                in_place = [o for o in sorted(differ_objs)
                            if o not in code_identical and not is_line_sensitive(after[o][1])]
                old_objects = [(before[o][1], old_copy(o)) for o in in_place]
                old_objects += [(before[o][1], old_copy(o)) for o in sorted(before)
                                if o not in after and o not in paired]
                # a moved object goes by its new path on both sides: its functions did not move between objects
                old_objects += [(after[n][1], old_copy(o)) for o, n in moved_unexpected]
                new_objects = [(after[o][1], os.path.join(bdir, o)) for o in in_place]
                new_objects += [(after[o][1], os.path.join(bdir, o)) for o in sorted(after)
                                if o not in before and o not in paired]
                new_objects += [(after[n][1], os.path.join(bdir, n)) for _, n in moved_unexpected]
                if move_verdict(dumpbin, env, old_objects, new_objects, len(moved_same)):
                    return 0
        print('\nRESULT: NOT a purely mechanical change')
        return 1
    if pairs:
        print(f'\nRESULT: move: {len(moved_same)} objects moved, code identical (mechanical)')
        return 0
    print('\nRESULT: mechanical change: no code generation differences')
    return 0


if __name__ == '__main__':
    sys.exit(main())
