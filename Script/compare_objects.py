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
dumpbin, so MSVC only); it is reported as "code identical" and tolerated.
Expected, by design, to differ: renamed functions, types or globals (their symbol names are in
the objects), and changed string literals.
"""
import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
CMAKE_ARGS = ['-DCMAKE_BUILD_TYPE=MinSizeRel', '-DAGS_BUILD_TOOLS=1', '-DAGS_TESTS=1',
              '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']


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


def disassembly_blocks(dumpbin, env, obj_path):
    """The code of an object as {function label: instructions}, without addresses, so that the order of
    the functions and the debug-info section do not matter."""
    text = subprocess.run([dumpbin, '/nologo', '/disasm:nobytes', obj_path], env=env, capture_output=True,
                          text=True, errors='replace').stdout
    blocks, label = {}, '<start>'
    for line in text.splitlines():
        if line.startswith('Dump of file') or not line.strip():
            continue
        line = line.strip()
        if re.match(r'^[^\s:]+:$', line) or re.match(r'^\S+ \(.*\):$', line):  # a symbol's label
            label = line
            blocks.setdefault(label, [])
            continue
        if re.match(r'^[0-9A-F]+ \.[\w$]+$', line):  # the section sizes of the summary: layout, not code
            continue
        line = re.sub(r'^[0-9A-F]{8}:\s*', '', line)
        line = re.sub(r'\b[0-9A-F]{8}\b', 'ADDR', line)  # jump targets and offsets inside the object
        blocks.setdefault(label, []).append(line)
    return blocks


def same_code(dumpbin, env, old_path, new_path):
    return disassembly_blocks(dumpbin, env, old_path) == disassembly_blocks(dumpbin, env, new_path)


def is_third_party(path):
    return any(part in ('libsrc', 'libinclude', '_deps') for part in path.split('/'))


def is_line_sensitive(path):
    return path.endswith('_test.cpp') or '/test/' in path


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('base', help='revision before the change')
    ap.add_argument('new', nargs='?', help='revision after the change (default: working tree)')
    ap.add_argument('--work-dir', default=os.path.join(REPO, '.compare'),
                    help='where the worktree and build directory live (default: %(default)s)')
    ap.add_argument('--clean', action='store_true', help='delete the build directory first')
    args = ap.parse_args()

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

    differ_objs = [o for o in after if o in before and before[o][0] != after[o][0]]
    # Objects that differ in bytes but not in code (debug-info section, function order)
    dumpbin = shutil.which('dumpbin', path=env.get('PATH') or env.get('Path'))
    code_identical = []
    if dumpbin:
        for o in differ_objs:
            old_obj = os.path.join(kept, hashlib.sha1(o.encode()).hexdigest())
            if same_code(dumpbin, env, old_obj, os.path.join(bdir, o)):
                code_identical.append(o)
    differ = sorted(after[o][1] for o in differ_objs if o not in code_identical)
    added = sorted(after[o][1] for o in after if o not in before)
    removed = sorted(before[o][1] for o in before if o not in after)
    same = sum(1 for o in after if o in before and before[o][0] == after[o][0])
    same_code_list = sorted(after[o][1] for o in code_identical)

    unexpected = [s for s in differ if not is_line_sensitive(s)]
    tolerated = [s for s in differ if is_line_sensitive(s)]

    print(f'\n{same} object(s) identical, {len(code_identical)} differ in bytes but not in code, '
          f'{len(differ)} differ, {len(added)} added, {len(removed)} removed')
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

    if unexpected or added or removed:
        print('\nRESULT: NOT a purely mechanical change')
        return 1
    print('\nRESULT: mechanical change: no code generation differences')
    return 0


if __name__ == '__main__':
    sys.exit(main())
