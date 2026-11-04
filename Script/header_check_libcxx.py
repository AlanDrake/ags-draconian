#!/usr/bin/env python3
"""
Header self-containment with a strict standard library: clang + libc++ from emsdk, as a POSIX target
(PLAN.md 2.6, 2.8). MSVC hides missing includes; GCC and libc++ do not, and this reproduces what the
CI's GCC and clang jobs find, without a Linux machine.

    python Script/header_check_libcxx.py <build-dir> [--emsdk <dir>]

<build-dir> is a CMake build tree made with a Ninja generator and -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
(Script/prepush.py makes one). The flags (-D, -I) of each module come from one source file of it.
The emsdk is found from --emsdk, then $EMSDK; any version with upstream/bin/clang++ will do.

Exit code 0: every header compiles on its own; 2: the emsdk is missing (nothing was checked);
1: some header fails.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

# One translation unit per module, to take the compiler flags from
MODULES = {
    'Common': 'Common/ac/spritecache.cpp',
    'Engine': 'Engine/ac/character.cpp',
    'Compiler': 'Compiler/main.cpp',
    'Tools': 'Tools/agsbuild/main.cpp',
}
# Defines of the Windows build that mean nothing to a POSIX target
DROP_DEFINES = ('WIN32', '_WINDOWS', '_CRT_NONSTDC_NO_DEPRECATE', '_CRT_SECURE_NO_DEPRECATE', 'AGS_HAS_CD_AUDIO')
# What CMake/HeaderCheck.cmake leaves out, as seen from a POSIX platform (emscripten)
EXCLUDE = re.compile(r'^(Common|Engine|Tools|Compiler)/(libsrc/|libinclude/|test[0-9]*/)'
                     r'|^Compiler/script/'
                     r'|/util/library_(windows|posix|psp|dummy)\.h$'
                     r'|^Common/util/smart_ptr\.h$'
                     r'|^Engine/gfx/gfxfilter_(aa)?d3d'
                     r'|^Engine/platform/(android|bsd|ios|linux|osx|windows)/')


def find_emsdk(arg):
    for d in (arg, os.environ.get('EMSDK')):
        if d and os.path.isfile(os.path.join(d, 'upstream', 'bin', 'clang++' + ('.exe' if os.name == 'nt' else ''))):
            return d
    return None


def flags_of(entries, src, sysroot):
    entry = entries.get(os.path.normcase(os.path.join(REPO, src)))
    if entry is None:
        sys.exit(f'{src} is not in compile_commands.json: build the Common, Engine, Compiler and tools targets')
    args = re.findall(r'(?:[^\s"]|"[^"]*")+', entry['command'])[1:]
    out = []
    for a in args:
        a = a.replace('"', '')
        if a.startswith(('-D', '/D')):
            if a[2:].split('=')[0] not in DROP_DEFINES:
                out.append('-D' + a[2:])
        elif a.startswith('-I'):
            path = a[2:]
            if '_deps' in path and 'sdl2' in path.lower():
                continue  # SDL configured for the host; the sysroot has its own
            if path.replace('\\', '/').endswith('Engine/libsrc/glad/include'):
                out.append('-I' + os.path.join(REPO, 'Engine', 'libsrc', 'glad-gles2', 'include'))
            out.append('-I' + path)
    out.append('-I' + os.path.join(sysroot, 'include', 'SDL2'))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('build_dir')
    ap.add_argument('--emsdk')
    ap.add_argument('--jobs', type=int, default=os.cpu_count() or 4)
    ap.add_argument('--keep-transitive', action='store_true',
                    help='do not define _LIBCPP_REMOVE_TRANSITIVE_INCLUDES (a laxer standard library)')
    args = ap.parse_args()

    emsdk = find_emsdk(args.emsdk)
    if not emsdk:
        print('emsdk not found: pass --emsdk or set EMSDK (the CI uses 3.1.74)')
        return 2
    exe = '.exe' if os.name == 'nt' else ''
    clang = os.path.join(emsdk, 'upstream', 'bin', 'clang++' + exe)
    sysroot = os.path.join(emsdk, 'upstream', 'emscripten', 'cache', 'sysroot')

    with open(os.path.join(args.build_dir, 'compile_commands.json'), encoding='utf-8') as f:
        entries = {os.path.normcase(e['file']): e for e in json.load(f)}
    flags = {m: flags_of(entries, s, sysroot) for m, s in MODULES.items()}
    extra = [] if args.keep_transitive else ['-D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES']

    headers = []
    for m in MODULES:
        for root, _, files in os.walk(os.path.join(REPO, m)):
            for f in files:
                if f.endswith(('.h', '.hpp')):
                    rel = os.path.relpath(os.path.join(root, f), REPO).replace('\\', '/')
                    if not EXCLUDE.search(rel):
                        headers.append((m, rel))

    with tempfile.TemporaryDirectory() as tmp:
        def check(item):
            m, rel = item
            stub = os.path.join(tmp, re.sub(r'[^A-Za-z0-9]', '_', rel) + '.cpp')
            with open(stub, 'w') as f:
                f.write('#include "%s/%s"\n' % (REPO.replace('\\', '/'), rel))
            r = subprocess.run([clang, '--target=wasm32-unknown-emscripten', '--sysroot=' + sysroot, '-std=c++17',
                                '-fsyntax-only', '-w', '-ferror-limit=5'] + extra + flags[m] + [stub],
                               capture_output=True, text=True)
            return rel, r.returncode, r.stdout + r.stderr

        with ThreadPoolExecutor(args.jobs) as ex:
            results = list(ex.map(check, headers))

    bad = sorted((rel, out) for rel, rc, out in results if rc != 0)
    for rel, out in bad:
        print('FAIL', rel)
        print('\n'.join(l for l in out.splitlines() if 'error' in l)[:600])
    print(f'{len(headers)} headers, {len(bad)} fail (clang + libc++, POSIX)')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
