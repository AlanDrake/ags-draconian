#!/usr/bin/env python3
"""
Runs locally what the CI would otherwise discover after a push (docs/refactor/PLAN.md, 2.8).

    python Script/prepush.py                  # every stage but the on-demand ones
    python Script/prepush.py --auto           # the stages that the changed files need
    python Script/prepush.py --auto --dry-run # which those are, and why, without running them
    python Script/prepush.py --only format,build
    python Script/prepush.py --skip msbuild   # the long one
    python Script/prepush.py --only sanitizers
    python Script/prepush.py --list

Stages, in the order they run (a failed stage does not stop the later ones; the summary lists all):
  format   clang-format 19.1.7 --dry-run -Werror over every tracked C/C++ file, as format.yml does
  helpers  the lines that differ from the upstream branch (Script/check_helpers.py, STYLE.md "Shared
           helpers"): a new function named like one of Common/util, and a comment that cites a member
           the Editor's class does not have; no build needed
  buildlists  the source lists of the .vcxproj against the CMake targets of the build stage's tree
           (Script/check_build_lists.py, PLAN.md 3.2c), as the CI's Windows job does; configures the tree
  build    CMake + Ninja (MSVC) Release build with tools and tests, then ctest
  tidy     clang-tidy 19.1 on the lines that differ from the upstream branch (Script/tidy_diff.py, 2.4):
           naming and size rules for new code only; the build's compile_commands.json is the input
  headers  the header_check target with MSVC (the CI's Windows job)
  libcxx   the headers again with clang + libc++ from emsdk, POSIX target, no transitive includes:
           the stand-in for the CI's clang job (Script/header_check_libcxx.py)
  emscripten  the engine built with emcc from emsdk, as Emscripten/build.sh does (the CI's Emscripten job):
           Clang and libc++ compile and link the OpenGL ES 2 renderer and the non-Windows branches that
           MSVC never sees; then the sources with Linux-only branches (acpllnx.cpp, library_posix.h, ...)
           are syntax-checked by the same compiler told that it is Linux. Not covered: macOS, iOS,
           Android and FreeBSD code
  linux    the CI's "Linux (GCC)" job (Script/linux_jobs.sh gcc): GCC build with warnings as errors,
           header check and ctest. On Windows it runs in a WSL 2 distro, on a copy of the working tree
  msbuild  the solutions the CI builds (Compiler.Lib, Tests, Engine, Tools, Compiler.App, Editor.Full;
           x64 Release) with their test runners, into .tidy/prepush/msb so that the PDBs of the
           regular Solutions/.build are not overwritten

--auto (PLAN.md 2.9) runs only the stages that the changed files need: the paths that differ between
the base revision and the working tree (the unpushed commits, the uncommitted and the untracked files;
--base names another revision, which the `tidy` and `helpers` stages then diff against too) are
matched against AUTO_RULES below. A change to the editor's C# code
needs `msbuild` alone, one to the tools everything but `emscripten`, one to Common or Engine every
stage; a path no rule knows needs every stage. It prints the choice with a path that caused it, and
the changed files that no stage verifies (workflows, packaging, most scripts). Without --auto nothing
is left out: use that after a repository-wide change, or when the rules may not fit.

On demand, with --only (analysis.yml; PLAN.md 1.5b):
  sanitizers  the ASan + UBSan build of tools, engine and tests, the unit tests and the auto-test game
           (Script/linux_jobs.sh sanitizers); fails on any finding, leaks included
  coverage    the clang coverage build and the same runs; prints the table per directory

Needs: Visual Studio 2022 with the C++ workload, Python, and for the optional parts clang-format
(--clang-format, $AGS_CLANG_FORMAT, .compare/cfenv or PATH; the version is checked), clang-tidy
(--clang-tidy, $CLANG_TIDY, .compare/cfenv or PATH; pip install clang-tidy==19.1.0), emsdk ($EMSDK or
--emsdk) and C:\\Lib (Windows\\setup_ags_build_deps.bat). A stage whose tool is missing is reported
as NOT RUN and makes the exit code non-zero: "not verified" is not "passed". Build trees live in
the git-ignored .tidy/prepush. Stages that do not apply to the platform are skipped silently.

The Linux stages need a WSL 2 distro (--wsl-distro, $AGS_WSL_DISTRO, default Ubuntu-24.04, the CI's
ubuntu-latest) with what Script/linux_jobs.sh lists. The working tree, uncommitted and untracked
files included, goes there as a commit under refs/prepush/worktree (no branch or index changes),
checked out in ~/ags-draconian; the build trees are in ~/build. On Linux the stages run in place,
with the build trees in .tidy/prepush/linux.

Exit code 0 only if every selected stage passed.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, os.path.dirname(__file__))
import tidy_diff  # noqa: E402

CLANG_FORMAT_VERSION = '19.1.7'
WORK = os.path.join(REPO, '.tidy', 'prepush')
BUILD = os.path.join(WORK, 'build')
EMS = os.path.join(WORK, 'ems')
# The targets marked ags_warning_free() fail on a warning (PLAN.md 1.5), as in the CI
WERROR = '-DAGS_WARNINGS_AS_ERRORS=ON'
MSB = os.path.join(WORK, 'msb')
CHECK_BUILD_LISTS = os.path.join(REPO, 'Script', 'check_build_lists.py')
SOURCE_GLOBS = ['*.c', '*.cc', '*.cpp', '*.cxx', '*.h', '*.hpp', '*.inl']
SOLUTIONS = ['Compiler.Lib', 'Tests', 'Engine', 'Tools', 'AGS.Editor.Full']  # Tools.sln builds agscc too
TEST_RUNNERS = ['Compiler.Lib.Test.exe', 'Common.Lib.Test.exe', 'Engine.App.Test.exe', 'Engine.Full.Test.exe',
                'Tools.Test.exe']
STAGES = ['format', 'helpers', 'buildlists', 'build', 'tidy', 'headers', 'libcxx', 'emscripten', 'linux', 'msbuild']
# Run only when named in --only: long, and a report more than a gate (analysis.yml is on demand too)
ON_DEMAND = ['sanitizers', 'coverage']
# --auto: (pattern of a repository path, the stages a change there needs, what no stage verifies or None).
# The first pattern found in a path decides. Added to what the rule says: `format` for a C or C++ file,
# `tidy` and `helpers` for one that clang-tidy checks (tidy_diff.is_checked). The stage lists rest on what each build
# reads: the Emscripten build reads Common, Engine, Plugins and the libraries, nothing under Compiler or
# Tools (`ninja -t deps`); MSBuild reads no CMake file; only MSBuild builds the editor; the tests of the
# compiler and the tools read the script API header.
SOURCE_EXTENSIONS = tuple(g[1:] for g in SOURCE_GLOBS)
COMPILED = ['build', 'headers', 'libcxx', 'emscripten', 'linux', 'msbuild']
CMAKE_BUILDS = ['build', 'headers', 'libcxx', 'emscripten', 'linux']
NOT_IN_THE_WEB_BUILD = ['build', 'headers', 'libcxx', 'linux', 'msbuild']
AUTO_RULES = [
    (r'^docs/|\.md$|^\.git-blame-ignore-revs$|^\.github/ISSUE_TEMPLATE/|^(License|Copyright|Changes)\.txt$', [], None),
    (r'^\.github/', [], 'workflows: actionlint, and a probe branch for the jobs only CI runs'),
    (r'(^|/)CMakeLists\.txt$|^CMake/|\.cmake$', CMAKE_BUILDS + ['buildlists'], None),
    (r'\.vcxproj$', ['buildlists', 'msbuild'], None),
    (r'(^|/)(libsrc|libinclude)/', ['build', 'emscripten', 'linux', 'msbuild'], None),
    (r'^Editor/AGS\.Editor/Resources/agsdefns\.sh$', NOT_IN_THE_WEB_BUILD, None),
    (r'^(Editor|Solutions)/', ['msbuild'], None),
    (r'^(Common|Engine|Plugins)/', COMPILED, None),
    (r'^(Compiler|Tools)/', NOT_IN_THE_WEB_BUILD, None),
    (r'^Emscripten/', ['emscripten'], None),
    (r'^Script/linux_jobs\.sh$', ['linux'], None),
    (r'^Script/(tidy_diff|refactor_metrics)\.py$', ['tidy'], None),
    (r'^Script/header_check_libcxx\.py$', ['libcxx'], None),
    (r'^Script/check_build_lists\.py$', ['buildlists'], None),
    (r'^Script/check_helpers\.py$', ['helpers'], None),
    (r'^Script/', [], 'scripts: no stage runs this one'),
    (r'^ci/', [], 'CI support files: the on-demand stages, or a CI run'),
    (r'^(Android|iOS|OSX|Windows|debian)/', [], 'platform packaging: a full CI run (ci.yml, macos.yml)'),
    (r'^\.clang-format$', ['format'], None),
    (r'^\.clang-tidy$', ['tidy'], None),
    (r'^\.(gitignore|editorconfig)$', [], None),
]
# The Linux stages on Windows: the distro, the clone in its home directory, the snapshot of the working tree
WSL_DISTRO = 'Ubuntu-24.04'
WSL_CLONE = 'ags-draconian'
WSL_REF = 'refs/prepush/worktree'
# The row of Script/coverage_summary.py with the number for the whole tree
COVERAGE_TOTAL = '| **Total** |'
# The emscripten stage's syntax check of the Linux branches: what marks a file as having one (platform.h
# only defines the macros), a compile statement of build.ninja, and the header that needs desktop OpenGL
LINUX_BRANCH = re.compile(r'AGS_PLATFORM_OS_LINUX|AGS_PLATFORM_IS_XDG_UNIX|__linux__')
LINUX_SKIP = 'Common/platform/platform.h'
COMPILE_BLOCK = re.compile(r'^build (\S+): CXX_COMPILER__(?:engine|common)_\S+ (\S+)[^\n]*\n((?:  [^\n]*\n)+)', re.M)
VARIABLE = re.compile(r'^  (\w+) = (.*)$', re.M)
DESKTOP_GL_HEADER = 'ogl_headers.h'

args = None
_msvc = None
_wsl = None


class NotRun(Exception):
    """A stage cannot run here (missing tool): reported, and fails the script."""


def log_path(stage):
    os.makedirs(WORK, exist_ok=True)
    return os.path.join(WORK, stage + '.log')


def run(stage, cmd, env=None, cwd=None, shell=False):
    """Runs a command with its output in .tidy/prepush/<stage>.log (appended); raises on failure."""
    with open(log_path(stage), 'a', encoding='utf-8', errors='replace') as log:
        log.write('$ %s\n' % (cmd if isinstance(cmd, str) else ' '.join(cmd)))
        log.flush()
        r = subprocess.run(cmd, env=env, cwd=cwd or REPO, stdout=log, stderr=subprocess.STDOUT, shell=shell)
    if r.returncode != 0:
        raise subprocess.CalledProcessError(r.returncode, cmd if isinstance(cmd, str) else cmd[0])


def msvc():
    global _msvc
    if _msvc is None:
        if os.name != 'nt':
            raise NotRun('the MSVC stages need Windows')
        import compare_objects as co
        try:
            env = co.msvc_env()
        except SystemExit as e:
            raise NotRun(str(e))
        env.pop('CFLAGS', None)
        env.pop('CXXFLAGS', None)
        _msvc = (env, co.tool)
    return _msvc


def find_clang_format():
    cands = [args.clang_format, os.environ.get('AGS_CLANG_FORMAT')]
    for sub in ('Scripts/clang-format.exe', 'bin/clang-format'):
        cands.append(os.path.join(REPO, '.compare', 'cfenv', sub))
    cands.append(shutil.which('clang-format'))
    for c in cands:
        if not c or not os.path.isfile(c):
            continue
        out = subprocess.run([c, '--version'], capture_output=True, text=True).stdout
        if CLANG_FORMAT_VERSION in out:
            return c
    raise NotRun('clang-format %s not found: pip install clang-format==%s into a venv and pass '
                 '--clang-format or set AGS_CLANG_FORMAT' % (CLANG_FORMAT_VERSION, CLANG_FORMAT_VERSION))


def tracked_files(*paths):
    """`git ls-files` of paths, without the files deleted in the working tree and not yet staged."""
    names = subprocess.run(['git', 'ls-files', *paths], cwd=REPO, capture_output=True, text=True,
                           check=True).stdout.split()
    return [n for n in names if os.path.isfile(os.path.join(REPO, n))]


def stage_format():
    exe = find_clang_format()
    files = tracked_files(*SOURCE_GLOBS)
    # Whole files, as CI does: git-clang-format checks changed lines only and misses alignment groups
    for i in range(0, len(files), 200):
        run('format', [exe, '--dry-run', '-Werror', *files[i:i + 200]])
    return '%d files' % len(files)


def base_args():
    """--base for the stages that diff (tidy, helpers): the one given to --auto, else their own default."""
    return ['--base', args.base] if args.base else []


def stage_helpers():
    try:
        run('helpers', [sys.executable, os.path.join(REPO, 'Script', 'check_helpers.py')] + base_args())
    except subprocess.CalledProcessError as e:
        e.note = '%s, see %s' % (last_match('helpers', 'check_helpers: '), os.path.relpath(log_path('helpers'), REPO))
        raise
    return last_match('helpers', 'check_helpers: ')


def is_configured(build_dir):
    """True if the tree was configured with the options of this script (WERROR was added later, 1.5)."""
    try:
        with open(os.path.join(build_dir, 'CMakeCache.txt'), encoding='utf-8', errors='replace') as f:
            return WERROR[2:].replace('=', ':BOOL=') in f.read() and os.path.isfile(os.path.join(build_dir, 'build.ninja'))
    except OSError:
        return False


def cmake_configure():
    env, tool = msvc()
    cmake = tool('cmake', env)
    if not is_configured(BUILD):
        run('build', [cmake, '-S', REPO, '-B', BUILD, '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', '-DAGS_BUILD_TOOLS=1',
                      '-DAGS_TESTS=1', '-DAGS_HEADER_CHECK=1', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', WERROR], env=env)
    return env, cmake


def stage_buildlists():
    env, cmake = cmake_configure()
    # The File API answers a query at the next configure: once by hand, then whenever ninja regenerates
    has_reply = glob.glob(os.path.join(BUILD, '.cmake', 'api', 'v1', 'reply', 'index-*.json'))
    run('buildlists', [sys.executable, CHECK_BUILD_LISTS, '--build', BUILD, '--query'])
    if has_reply:
        run('buildlists', [tool_of('ninja', env), '-C', BUILD, 'build.ninja'], env=env)
    else:
        run('buildlists', [cmake, BUILD], env=env)
    try:
        run('buildlists', [sys.executable, CHECK_BUILD_LISTS, '--build', BUILD])
    except subprocess.CalledProcessError as e:
        drift = last_match('buildlists', 'only in ')
        if drift:
            e.note = 'drift (%s and maybe more), see %s' % (drift, os.path.relpath(log_path('buildlists'), REPO))
        raise
    return last_match('buildlists', 'match their CMake targets')


def stage_build():
    env, cmake = cmake_configure()
    run('build', [cmake, '--build', BUILD], env=env)
    run('build', [tool_of('ctest', env), '--test-dir', BUILD, '--output-on-failure', '-j', str(os.cpu_count() or 4)],
        env=env)


def tool_of(name, env):
    return msvc()[1](name, env)


def stage_tidy():
    exe = tidy_diff.find_clang_tidy(args.clang_tidy)
    if exe is None:
        raise NotRun('clang-tidy %s not found: pip install clang-tidy==19.1.0 into a venv and pass '
                     '--clang-tidy or set CLANG_TIDY' % tidy_diff.CLANG_TIDY_VERSION)
    env, _ = cmake_configure()
    # The compile database of the build tree; the diff is the unpushed commits and the working tree (or --base)
    run('tidy', [sys.executable, os.path.join(REPO, 'Script', 'tidy_diff.py'), '--build', BUILD, '--clang-tidy', exe]
        + base_args(), env=env)
    with open(log_path('tidy'), encoding='utf-8', errors='replace') as log:
        return log.read().strip().splitlines()[-1]


def stage_headers():
    env, cmake = cmake_configure()
    run('headers', [cmake, '--build', BUILD, '--target', 'header_check', '--', '-k', '0'], env=env)


def stage_libcxx():
    cmake_configure()  # compile_commands.json is the source of the flags
    cmd = [sys.executable, os.path.join(REPO, 'Script', 'header_check_libcxx.py'), BUILD]
    if args.emsdk:
        cmd += ['--emsdk', args.emsdk]
    with open(log_path('libcxx'), 'w', encoding='utf-8') as log:
        r = subprocess.run(cmd, stdout=log, stderr=subprocess.STDOUT)
    if r.returncode == 2:
        raise NotRun('emsdk not found: pass --emsdk or set EMSDK')
    if r.returncode != 0:
        raise subprocess.CalledProcessError(r.returncode, cmd[1])
    with open(log_path('libcxx'), encoding='utf-8') as log:
        return log.read().strip().splitlines()[-1]


def find_emsdk():
    for d in (args.emsdk, os.environ.get('EMSDK')):
        if d and os.path.isfile(os.path.join(d, 'upstream', 'emscripten', 'cmake', 'Modules', 'Platform',
                                             'Emscripten.cmake')):
            return os.path.abspath(d)
    raise NotRun('emsdk not found: pass --emsdk or set EMSDK (the CI uses 3.1.74)')


def stage_emscripten():
    emsdk = find_emsdk()
    if os.name == 'nt':
        # cmake and ninja come with Visual Studio; the build itself must not see the MSVC environment
        msvc_environ, tool = msvc()
        cmake, ninja = tool('cmake', msvc_environ), tool('ninja', msvc_environ)
    else:
        cmake, ninja = shutil.which('cmake'), shutil.which('ninja')
        if not cmake or not ninja:
            raise NotRun('cmake and ninja are needed on PATH')
    # What emsdk_env sets: emcc finds its configuration, node and python through these
    env = dict(os.environ)
    env['EMSDK'] = emsdk
    env['EM_CONFIG'] = os.path.join(emsdk, '.emscripten')
    tools = [os.path.join(emsdk, 'upstream', 'emscripten')]
    for sub in (('node', '*', 'bin'), ('node', '*'), ('python', '*')):
        tools += sorted(glob.glob(os.path.join(emsdk, *sub)))
    env['PATH'] = os.pathsep.join(tools + [env.get('PATH', '')])
    if not is_configured(EMS):
        toolchain = os.path.join(emsdk, 'upstream', 'emscripten', 'cmake', 'Modules', 'Platform', 'Emscripten.cmake')
        run('emscripten', [cmake, '-S', REPO, '-B', EMS, '-G', 'Ninja', '-DCMAKE_TOOLCHAIN_FILE=' + toolchain,
                           '-DCMAKE_MAKE_PROGRAM=' + ninja, '-DCMAKE_BUILD_TYPE=Release', '-DAGS_BUILD_TOOLS=0',
                           '-DAGS_TESTS=0', WERROR], env=env)
    run('emscripten', [cmake, '--build', EMS], env=env)
    checked = linux_syntax_check(emsdk, ninja, env)
    return 'ags.wasm linked, %d files with Linux branches pass a syntax check' % checked


def linux_branch_files():
    """First-party Common and Engine files with code for Linux only, which neither MSVC nor emcc compiles."""
    result = set()
    for name in tracked_files('Common', 'Engine'):
        if 'libsrc/' in name or not name.endswith(('.h', '.cpp', '.c', '.inl')) or name == LINUX_SKIP:
            continue
        with open(os.path.join(REPO, name), encoding='utf-8', errors='replace') as f:
            if LINUX_BRANCH.search(f.read()):
                result.add(os.path.normcase(os.path.join(REPO, name)))
    return result


def linux_syntax_check(emsdk, ninja, env):
    """
    The Emscripten build leaves out the Linux branches (acpllnx.cpp, library_posix.h, the XDG paths). They are
    syntax-checked here by the same compiler told that it is Linux: emsdk's sysroot is musl, a Linux C library,
    and SDL comes from the Emscripten port. Sources that reach the desktop OpenGL headers are left out (the
    build has the OpenGL ES ones; MSVC compiles that code). macOS, iOS, Android and FreeBSD code stays unchecked.
    """
    def norm(path):
        return os.path.normcase(os.path.normpath(os.path.join(EMS, path.replace('$:', ':'))))

    linux_files = linux_branch_files()
    # {object: [dependency, ...]} as ninja recorded them in the build
    deps, current = {}, None
    out = subprocess.run([ninja, '-C', EMS, '-t', 'deps'], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        if line and not line.startswith(' '):
            current = deps.setdefault(line.split(':')[0], [])
        elif line.strip() and current is not None:
            current.append(norm(line.strip()))
    with open(os.path.join(EMS, 'build.ninja'), encoding='utf-8') as f:
        text = f.read()
    jobs = []
    for m in COMPILE_BLOCK.finditer(text):
        obj, src = m.group(1), os.path.normpath(os.path.join(EMS, m.group(2).replace('$:', ':')))
        files = set(deps.get(obj, [])) | {os.path.normcase(src)}
        if not files & linux_files or any(os.path.basename(p) == DESKTOP_GL_HEADER for p in files):
            continue
        block = dict(VARIABLE.findall(m.group(3)))
        flags = ' '.join(block.get(k, '') for k in ('DEFINES', 'INCLUDES', 'FLAGS')).replace('$:', ':').split()
        flags = [a for a in flags if not a.startswith('-W') or a.startswith(('-Werror=', '-Wno-'))]
        jobs.append((src, flags))
    if not jobs:
        raise subprocess.CalledProcessError(1, 'no source with Linux branches found in the Emscripten build')
    compiler = os.path.join(emsdk, 'upstream', 'emscripten', 'em++.bat' if os.name == 'nt' else 'em++')

    def check(job):
        src, flags = job
        r = subprocess.run([compiler, '-fsyntax-only', '-U__EMSCRIPTEN__', '-D__linux__', *flags, src], env=env,
                           capture_output=True, text=True, encoding='utf-8', errors='replace')
        return src, r.returncode, r.stdout + r.stderr

    failed = 0
    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool, \
            open(log_path('emscripten'), 'a', encoding='utf-8', errors='replace') as log:
        for src, code, output in pool.map(check, jobs):
            log.write('$ syntax check as Linux: %s: %s\n' % (os.path.relpath(src, REPO), 'ok' if code == 0 else 'FAILED'))
            if code != 0:
                log.write(output + '\n')
                failed += 1
    if failed:
        raise subprocess.CalledProcessError(1, 'syntax check as Linux')
    return len(jobs)


def git_output(stage, *cmd, env=None):
    """The output of a git command in the repository; a failure goes to the stage's log."""
    r = subprocess.run(['git', *cmd], cwd=REPO, env=env, capture_output=True, text=True)
    if r.returncode != 0:
        with open(log_path(stage), 'a', encoding='utf-8', errors='replace') as log:
            log.write('$ git %s\n%s%s\n' % (' '.join(cmd), r.stdout, r.stderr))
        raise subprocess.CalledProcessError(r.returncode, 'git ' + cmd[0])
    return r.stdout.strip()


def worktree_snapshot(stage):
    """
    The working tree as a commit under WSL_REF: HEAD, or a child of it with the uncommitted and the
    untracked files (ignored ones excluded). Made through a copy of the index: nothing the user sees changes.
    """
    git_dir = git_output(stage, 'rev-parse', '--absolute-git-dir')
    index = os.path.join(git_dir, 'prepush-index')
    shutil.copyfile(os.path.join(git_dir, 'index'), index)
    env = dict(os.environ, GIT_INDEX_FILE=index)
    try:
        git_output(stage, 'add', '-A', env=env)
        tree = git_output(stage, 'write-tree', env=env)
    finally:
        os.remove(index)
    commit = git_output(stage, 'rev-parse', 'HEAD')
    if tree != git_output(stage, 'rev-parse', 'HEAD^{tree}'):
        commit = git_output(stage, 'commit-tree', tree, '-p', 'HEAD', '-m', 'prepush: the working tree')
    git_output(stage, 'update-ref', WSL_REF, commit)
    return commit


def wsl(stage):
    """
    The prefix of a command run in the home directory of the WSL distro, without a shell in between.
    The first call of a run copies the working tree into the distro's clone: building on the Windows
    drive through /mnt is slow, and the files there have CRLF line ends.
    """
    global _wsl
    if _wsl is not None:
        return _wsl
    distro = args.wsl_distro or os.environ.get('AGS_WSL_DISTRO') or WSL_DISTRO
    exe = shutil.which('wsl')
    if not exe or subprocess.run([exe, '-d', distro, '--exec', 'true'], capture_output=True).returncode != 0:
        raise NotRun('WSL 2 distro %s not found (wsl --install -d %s, then what Script/linux_jobs.sh needs); '
                     'pass --wsl-distro or set AGS_WSL_DISTRO for another one' % (distro, distro))
    drive, path = os.path.splitdrive(REPO)
    if len(drive) != 2:
        raise NotRun('the repository must be on a drive with a letter for WSL to read it')
    source = '/mnt/%s%s' % (drive[0].lower(), path.replace('\\', '/'))
    prefix = [exe, '-d', distro, '--cd', '~', '--exec']
    commit = worktree_snapshot(stage)
    if subprocess.run(prefix + ['test', '-d', WSL_CLONE + '/.git']).returncode != 0:
        run(stage, prefix + ['git', 'clone', '-q', '--no-checkout', source, WSL_CLONE])
    run(stage, prefix + ['git', '-C', WSL_CLONE, 'fetch', '-q', source, '+%s:%s' % (WSL_REF, WSL_REF)])
    run(stage, prefix + ['git', '-C', WSL_CLONE, '-c', 'advice.detachedHead=false', 'checkout', '-q', '-f',
                         '--detach', commit])
    _wsl = prefix
    return _wsl


def linux_job(stage, job):
    """Runs a job of Script/linux_jobs.sh: in the WSL distro on Windows, in place on Linux."""
    script = 'Script/linux_jobs.sh'
    if os.name == 'nt':
        run(stage, wsl(stage) + ['bash', '%s/%s' % (WSL_CLONE, script), job])
    elif sys.platform.startswith('linux'):
        run(stage, ['bash', os.path.join(REPO, script), job],
            env=dict(os.environ, AGS_JOBS_WORK=os.path.join(WORK, 'linux')))
    else:
        raise NotRun('the Linux stages need Linux, or Windows with a WSL 2 distro')


def last_match(stage, pattern):
    """The last line of the stage's log that has the pattern, or ''."""
    with open(log_path(stage), encoding='utf-8', errors='replace') as log:
        found = [line.strip() for line in log if pattern in line]
    return found[-1] if found else ''


def stage_linux():
    linux_job('linux', 'gcc')
    return last_match('linux', 'tests passed')


def stage_sanitizers():
    try:
        linux_job('sanitizers', 'sanitizers')
    except subprocess.CalledProcessError as e:
        found = last_match('sanitizers', 'distinct sanitizer findings')
        if found:
            e.note = '%s see %s' % (found, os.path.relpath(log_path('sanitizers'), REPO))
        raise
    return last_match('sanitizers', 'No sanitizer findings')


def stage_coverage():
    linux_job('coverage', 'coverage')
    return last_match('coverage', COVERAGE_TOTAL)


def find_msbuild():
    vswhere = os.path.join(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
                           'Microsoft Visual Studio', 'Installer', 'vswhere.exe')
    if os.path.isfile(vswhere):
        out = subprocess.run([vswhere, '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild',
                              '-find', r'MSBuild\**\Bin\MSBuild.exe'], capture_output=True, text=True).stdout.split('\n')
        if out and out[0].strip():
            return out[0].strip()
    raise NotRun('MSBuild not found (Visual Studio 2022)')


def stage_msbuild():
    if os.name != 'nt':
        raise NotRun('MSBuild solutions need Windows')
    if not os.path.isdir(r'C:\Lib') and not os.environ.get('AGS_SDL_LIB'):
        raise NotRun(r'dependencies missing: run Windows\setup_ags_build_deps.bat (C:\Lib) or set the AGS_*_LIB variables')
    msbuild = find_msbuild()
    out_dir = MSB + '\\'
    built = []
    for sln in SOLUTIONS:
        run('msbuild', [msbuild, os.path.join(REPO, 'Solutions', sln + '.sln'), '/m', '/nologo', '/v:minimal',
                        '/p:MultiProcessorCompilation=true', '/p:PlatformToolset=v143', '/p:Configuration=Release',
                        '/p:Platform=x64', '/p:AgsSolutionsDir=' + out_dir])
        built.append(sln)
        if sln in ('Tests',):
            for exe in TEST_RUNNERS:
                path = os.path.join(MSB, '.test', 'x64', 'Release', exe)
                if not os.path.isfile(path):
                    raise subprocess.CalledProcessError(1, 'missing test runner ' + exe)
                run('msbuild', [path])
    # The editor's NUnit tests, when NuGet has restored the runner
    runners = glob.glob(os.path.join(REPO, 'Solutions', 'packages', 'NUnit.ConsoleRunner.3.*', 'tools', 'nunit3-console.exe'))
    dll = os.path.join(MSB, '.test', 'x64', 'Release', 'AGS.Editor.Tests.dll')
    if runners and os.path.isfile(dll):
        run('msbuild', [runners[0], dll, '--noresult'])
        return '%d solutions, Editor NUnit' % len(built)
    return '%d solutions (Editor NUnit runner not restored: not run)' % len(built)


def changed_paths(base):
    """
    The repository paths that differ between the base revision and the working tree: committed, staged,
    unstaged and untracked files, deleted ones included; a rename gives both names.
    """
    paths = tidy_diff.git('diff', '--name-only', '--no-renames', base).splitlines()
    paths += tidy_diff.git('ls-files', '--others', '--exclude-standard').splitlines()
    return sorted(set(p for p in paths if p))


def auto_plan(paths):
    """({stage: [the paths that need it]}, [(path, what no stage verifies)]) by AUTO_RULES."""
    needed = {stage: [] for stage in STAGES}
    uncovered = []
    for path in paths:
        stages, note = list(STAGES), None  # a path no rule knows: every stage
        for pattern, rule_stages, rule_note in AUTO_RULES:
            if re.search(pattern, path):
                stages, note = list(rule_stages), rule_note
                break
        # A deleted file is neither formatted nor analysed, but what compiled it must still build
        if os.path.isfile(os.path.join(REPO, path)):
            if path.endswith(SOURCE_EXTENSIONS) and 'format' not in stages:
                stages.append('format')
            if tidy_diff.is_checked(path):
                stages += [stage for stage in ('tidy', 'helpers') if stage not in stages]
        for stage in stages:
            needed[stage].append(path)
        if note:
            uncovered.append((path, note))
    return needed, uncovered


def print_auto_plan(base, paths, needed, uncovered):
    print('prepush --auto: %d path(s) differ from %s (the unpushed commits and the working tree)' % (len(paths), base[:12]))
    for stage in STAGES:
        because = needed[stage]
        if because:
            more = ' and %d more' % (len(because) - 1) if len(because) > 1 else ''
            print('  %-10s run   %s%s' % (stage, because[0], more))
        else:
            print('  %-10s skip' % stage)
    notes = {}
    for path, note in uncovered:
        notes.setdefault(note, []).append(path)
    for note, group in notes.items():
        more = ' and %d more' % (len(group) - 3) if len(group) > 3 else ''
        print('  no stage verifies %s%s (%s)' % (', '.join(group[:3]), more, note))


RUNNERS = {'format': stage_format, 'helpers': stage_helpers, 'buildlists': stage_buildlists, 'build': stage_build, 'tidy': stage_tidy,
           'headers': stage_headers, 'libcxx': stage_libcxx, 'emscripten': stage_emscripten, 'linux': stage_linux,
           'msbuild': stage_msbuild,
           'sanitizers': stage_sanitizers, 'coverage': stage_coverage}


def main():
    global args
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--only', help='comma-separated stages to run')
    ap.add_argument('--skip', help='comma-separated stages to leave out')
    ap.add_argument('--list', action='store_true', help='list the stages')
    ap.add_argument('--auto', action='store_true', help='run the stages that the changed files need (AUTO_RULES)')
    ap.add_argument('--base', help='with --auto: the revision to compare the working tree with\n'
                                   '(default: the merge base with the upstream branch)')
    ap.add_argument('--dry-run', action='store_true', help='print the stages that would run, and stop')
    ap.add_argument('--clang-format', help='path of clang-format %s' % CLANG_FORMAT_VERSION)
    ap.add_argument('--clang-tidy', help='path of clang-tidy %s' % tidy_diff.CLANG_TIDY_VERSION)
    ap.add_argument('--emsdk', help='emsdk directory (default: $EMSDK)')
    ap.add_argument('--wsl-distro', help='WSL distro of the Linux stages (default: $AGS_WSL_DISTRO, %s)' % WSL_DISTRO)
    ap.add_argument('--clean-logs', action='store_true', help='delete the old logs first')
    args = ap.parse_args()

    if args.list:
        print('\n'.join(STAGES + ['%s (on demand: --only)' % s for s in ON_DEMAND]))
        return 0
    if args.auto and args.only:
        sys.exit('--auto chooses the stages itself: leave out --only (--skip still applies)')
    if args.base and not args.auto:
        sys.exit('--base goes with --auto')
    if args.auto:
        base = args.base or tidy_diff.default_base()
        paths = changed_paths(base)
        needed, uncovered = auto_plan(paths)
        print_auto_plan(base, paths, needed, uncovered)
        chosen = [s for s in STAGES if needed[s]]
    else:
        chosen = args.only.split(',') if args.only else list(STAGES)
    chosen = [s for s in chosen if s not in (args.skip.split(',') if args.skip else [])]
    for s in chosen:
        if s not in RUNNERS:
            sys.exit('unknown stage %s (stages: %s)' % (s, ', '.join(STAGES + ON_DEMAND)))
    chosen.sort(key=(STAGES + ON_DEMAND).index)
    if args.dry_run:
        print('would run: ' + (', '.join(chosen) or 'no stage'))
        return 0
    if not chosen:
        print('prepush: no stage to run')
        return 0
    if args.clean_logs:
        shutil.rmtree(WORK, ignore_errors=True)
    for s in chosen:
        if os.path.isfile(log_path(s)):
            os.remove(log_path(s))

    results = []
    for s in chosen:
        print('== %s ...' % s, flush=True)
        t0 = time.time()
        try:
            note = RUNNERS[s]() or ''
            status = 'PASS'
        except NotRun as e:
            status, note = 'NOT RUN', str(e)
        except subprocess.CalledProcessError as e:
            status, note = 'FAIL', getattr(e, 'note', None) or 'see %s' % os.path.relpath(log_path(s), REPO)
        results.append((s, status, note, time.time() - t0))
        print('   %s (%.0fs) %s' % (status, results[-1][3], note), flush=True)

    print('\nprepush summary:')
    for s, status, note, secs in results:
        print('  %-10s %-8s %5.0fs  %s' % (s, status, secs, note))
    left_out = [s for s in STAGES if s not in chosen]
    if left_out:
        print('  not selected: ' + ', '.join(left_out))
    ok = all(status == 'PASS' for _, status, _, _ in results)
    print('prepush: ' + ('all selected stages passed' if ok else 'NOT READY TO PUSH'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
