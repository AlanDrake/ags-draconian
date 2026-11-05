#!/usr/bin/env python3
"""
Runs locally what the CI would otherwise discover after a push (docs/refactor/PLAN.md, 2.8).

    python Script/prepush.py                  # every stage
    python Script/prepush.py --only format,build
    python Script/prepush.py --skip msbuild   # the long one
    python Script/prepush.py --list

Stages, in the order they run (a failed stage does not stop the later ones; the summary lists all):
  format   clang-format 19.1.7 --dry-run -Werror over every tracked C/C++ file, as format.yml does
  build    CMake + Ninja (MSVC) Release build with tools and tests, then ctest
  tidy     clang-tidy 19.1 on the lines that differ from the upstream branch (Script/tidy_diff.py, 2.4):
           naming and size rules for new code only; the build's compile_commands.json is the input
  headers  the header_check target with MSVC (the CI's Windows job)
  libcxx   the headers again with clang + libc++ from emsdk, POSIX target, no transitive includes:
           the stand-in for the CI's GCC and clang jobs (Script/header_check_libcxx.py)
  emscripten  the engine built with emcc from emsdk, as Emscripten/build.sh does (the CI's Emscripten job):
           Clang and libc++ compile and link the OpenGL ES 2 renderer and the non-Windows branches that
           MSVC never sees; then the sources with Linux-only branches (acpllnx.cpp, library_posix.h, ...)
           are syntax-checked by the same compiler told that it is Linux. Not covered: macOS, iOS,
           Android and FreeBSD code, and linking anything for Linux
  msbuild  the solutions the CI builds (Compiler.Lib, Tests, Engine, Tools, Compiler.App, Editor.Full;
           x86 Release) with their test runners, into .tidy/prepush/msb so that the PDBs of the
           regular Solutions/.build are not overwritten

Needs: Visual Studio 2022 with the C++ workload, Python, and for the optional parts clang-format
(--clang-format, $AGS_CLANG_FORMAT, .compare/cfenv or PATH; the version is checked), clang-tidy
(--clang-tidy, $CLANG_TIDY, .compare/cfenv or PATH; pip install clang-tidy==19.1.0), emsdk ($EMSDK or
--emsdk) and C:\\Lib (Windows\\setup_ags_build_deps.bat). A stage whose tool is missing is reported
as NOT RUN and makes the exit code non-zero: "not verified" is not "passed". Build trees live in
the git-ignored .tidy/prepush. Stages that do not apply to the platform are skipped silently.

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
SOURCE_GLOBS = ['*.c', '*.cc', '*.cpp', '*.cxx', '*.h', '*.hpp', '*.inl']
SOLUTIONS = ['Compiler.Lib', 'Tests', 'Engine', 'Tools', 'Compiler.App', 'AGS.Editor.Full']
TEST_RUNNERS = ['Compiler.Lib.Test.exe', 'Common.Lib.Test.exe', 'Engine.App.Test.exe', 'Tools.Test.exe']
STAGES = ['format', 'build', 'tidy', 'headers', 'libcxx', 'emscripten', 'msbuild']
# The emscripten stage's syntax check of the Linux branches: what marks a file as having one (platform.h
# only defines the macros), a compile statement of build.ninja, and the header that needs desktop OpenGL
LINUX_BRANCH = re.compile(r'AGS_PLATFORM_OS_LINUX|AGS_PLATFORM_IS_XDG_UNIX|__linux__')
LINUX_SKIP = 'Common/platform/platform.h'
COMPILE_BLOCK = re.compile(r'^build (\S+): CXX_COMPILER__(?:engine|common)_\S+ (\S+)[^\n]*\n((?:  [^\n]*\n)+)', re.M)
VARIABLE = re.compile(r'^  (\w+) = (.*)$', re.M)
DESKTOP_GL_HEADER = 'ogl_headers.h'

args = None
_msvc = None


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


def stage_format():
    exe = find_clang_format()
    files = subprocess.run(['git', 'ls-files', *SOURCE_GLOBS], cwd=REPO, capture_output=True, text=True,
                           check=True).stdout.split()
    # Whole files, as CI does: git-clang-format checks changed lines only and misses alignment groups
    for i in range(0, len(files), 200):
        run('format', [exe, '--dry-run', '-Werror', *files[i:i + 200]])
    return '%d files' % len(files)


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
    # The compile database of the build tree; the diff is the unpushed commits and the working tree
    run('tidy', [sys.executable, os.path.join(REPO, 'Script', 'tidy_diff.py'), '--build', BUILD, '--clang-tidy', exe],
        env=env)
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
    names = subprocess.run(['git', 'ls-files', 'Common', 'Engine'], cwd=REPO, capture_output=True, text=True,
                           check=True).stdout.split()
    result = set()
    for name in names:
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
                        '/p:Platform=x86', '/p:AgsSolutionsDir=' + out_dir])
        built.append(sln)
        if sln in ('Tests',):
            for exe in TEST_RUNNERS:
                path = os.path.join(MSB, '.test', 'x86', 'Release', exe)
                if not os.path.isfile(path):
                    raise subprocess.CalledProcessError(1, 'missing test runner ' + exe)
                run('msbuild', [path])
    # The editor's NUnit tests, when NuGet has restored the runner
    runners = glob.glob(os.path.join(REPO, 'Solutions', 'packages', 'NUnit.ConsoleRunner.3.*', 'tools', 'nunit3-console.exe'))
    dll = os.path.join(MSB, '.test', 'x86', 'Release', 'AGS.Editor.Tests.dll')
    if runners and os.path.isfile(dll):
        run('msbuild', [runners[0], dll, '--noresult'])
        return '%d solutions, Editor NUnit' % len(built)
    return '%d solutions (Editor NUnit runner not restored: not run)' % len(built)


RUNNERS = {'format': stage_format, 'build': stage_build, 'tidy': stage_tidy, 'headers': stage_headers,
           'libcxx': stage_libcxx, 'emscripten': stage_emscripten, 'msbuild': stage_msbuild}


def main():
    global args
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--only', help='comma-separated stages to run')
    ap.add_argument('--skip', help='comma-separated stages to leave out')
    ap.add_argument('--list', action='store_true', help='list the stages')
    ap.add_argument('--clang-format', help='path of clang-format %s' % CLANG_FORMAT_VERSION)
    ap.add_argument('--clang-tidy', help='path of clang-tidy %s' % tidy_diff.CLANG_TIDY_VERSION)
    ap.add_argument('--emsdk', help='emsdk directory (default: $EMSDK)')
    ap.add_argument('--clean-logs', action='store_true', help='delete the old logs first')
    args = ap.parse_args()

    if args.list:
        print('\n'.join(STAGES))
        return 0
    chosen = args.only.split(',') if args.only else list(STAGES)
    chosen = [s for s in chosen if s not in (args.skip.split(',') if args.skip else [])]
    for s in chosen:
        if s not in RUNNERS:
            sys.exit('unknown stage %s (stages: %s)' % (s, ', '.join(STAGES)))
    chosen.sort(key=STAGES.index)
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
            status, note = 'FAIL', 'see %s' % os.path.relpath(log_path(s), REPO)
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
