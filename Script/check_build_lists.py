#!/usr/bin/env python3
"""
The drift guard between the two build descriptions (docs/refactor/PLAN.md, 3.2c): compares the source list
of every first-party native .vcxproj with the CMake targets that build the same program or library, and fails
on a file that is in one and not in the other, unless DELIBERATE below gives it a reason.

    python Script/check_build_lists.py --build <CMake build dir>
    python Script/check_build_lists.py --build <dir> --query   # only write the File API query

The CMake side comes from the CMake File API (codemodel-v2): the compiled sources of each target, as the
configure step resolved them for this platform. The query must exist before CMake configures the tree:
--query writes it, then configure (an existing tree: "cmake <dir>"). Configure on Windows, as CI's Windows
job does (-DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1): the .vcxproj build Windows programs, and CMake lists other
files on other platforms. The .vcxproj side is the ClCompile and ResourceCompile items, paths resolved
from the project's directory.

PROJECTS maps each .vcxproj to CMake targets:
  own      every compiled source of these targets must be in the .vcxproj or in a project it links
  library  static libraries of which the .vcxproj may compile only some files: a file it lists must be in
           an own or a library target, but a library file it leaves out is no drift. The tools' .vcxproj
           compile the libtools files they use; a file a program needs and its .vcxproj lacks fails the
           MSBuild link instead
  links    .vcxproj whose files this one links (Common.lib, Compiler.lib, Engine.App's objects): they count
           as its own for the CMake files it must have, and are checked in their own entries
  prefix   only the sources under this directory are compared (a plugin's DLL project against the copy
           built into CMake's engine)
Sources outside the repository (FetchContent, generated files) are not compared.

A .vcxproj that is in neither PROJECTS nor NOT_IN_CMAKE fails the check, and so does a DELIBERATE entry
that no longer matches a difference: the list stays as short as the differences. Exit code 0: no drift;
1: drift, listed per project; 2: cannot run (no File API reply, a CMake target missing).
"""
import argparse
import fnmatch
import glob
import json
import os
import subprocess
import sys
import xml.etree.ElementTree as ET

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
MSBUILD_NS = '{http://schemas.microsoft.com/developer/msbuild/2003}'
QUERY = os.path.join('.cmake', 'api', 'v1', 'query', 'codemodel-v2')
REPLY = os.path.join('.cmake', 'api', 'v1', 'reply')
ITEMS = ('ClCompile', 'ResourceCompile')

COMMON_LIB = 'Solutions/Common.Lib/Common.Lib.vcxproj'
COMPILER_LIB = 'Solutions/Compiler.Lib/Compiler.Lib.vcxproj'
ENGINE_APP = 'Solutions/Engine.App/Engine.App.vcxproj'
TOOL_LIBRARIES = ['libtools', 'tinyxml2', 'miniz', 'stb']


def entry(own, library=(), links=(), prefix=None):
    return {'own': list(own), 'library': list(library), 'links': list(links), 'prefix': prefix}


def plugin(name):
    return entry(['engine'], prefix='Plugins/%s/' % name)


def tool(name):
    return entry([name], TOOL_LIBRARIES)


PROJECTS = {
    COMMON_LIB: entry(['common', 'allegro', 'freetype', 'aastr', 'miniz', 'stb', 'sdl_ttf']),
    # CMake's compiler library compiles the Common and Allegro files it uses; MSBuild's Compiler.lib does
    # not, its users do (agscc) or link Common.lib: the check of Compiler.Lib is in those of its users
    COMPILER_LIB: entry([], ['compiler']),
    'Solutions/Compiler.App/Compiler.App.vcxproj':
        entry(['agscc', 'compiler'], ['common', 'allegro'], [COMPILER_LIB]),
    'Solutions/Compiler.Lib/Compiler.Lib.Test.vcxproj':
        entry(['compiler_test'], ['compiler', 'common'], [COMPILER_LIB]),
    ENGINE_APP: entry(['ags', 'engine', 'glad', 'apeg', 'mojoal'], links=[COMMON_LIB]),
    'Solutions/Tests.App/Common.Lib.Test.vcxproj': entry(['common_test'], links=[COMMON_LIB]),
    'Solutions/Tests.App/Engine.App.Test.vcxproj': entry(['engine_test'], ['engine'], [COMMON_LIB]),
    # Its <Link> item takes Engine.App's object files but main_sdl2.obj, as engine_full_test links engine
    'Solutions/Tests.App/Engine.Full.Test.vcxproj': entry(['engine_full_test'], links=[ENGINE_APP]),
    # Its ProjectReference to Common.Lib links Common.lib
    'Solutions/Tests.App/Tools.Test.vcxproj':
        entry(['tools_test'], TOOL_LIBRARIES + ['common', 'allegro'], [COMMON_LIB]),
    'Solutions/Tools.App/afg2dlgasc.vcxproj': tool('agf2dlgasc'),
    'Solutions/Tools.App/agf2dta.vcxproj': tool('agf2dta'),
    'Solutions/Tools.App/agfexport.vcxproj': tool('agfexport'),
    'Solutions/Tools.App/agsbuild.vcxproj': tool('agsbuild'),
    'Solutions/Tools.App/agspak.vcxproj': tool('agspak'),
    'Solutions/Tools.App/crmbuild.vcxproj': tool('crmbuild'),
    'Solutions/Tools.App/crmpak.vcxproj': tool('crmpak'),
    'Solutions/Tools.App/po2tra.vcxproj': tool('po2tra'),
    'Solutions/Tools.App/spriteimport.vcxproj': tool('spriteimport'),
    'Solutions/Tools.App/spritepak.vcxproj': tool('spritepak'),
    'Plugins/AGSSpriteFont/msvc/AGSSpriteFont.vcxproj': plugin('AGSSpriteFont'),
    'Plugins/ags_parallax/msvc/ags_parallax.vcxproj': plugin('ags_parallax'),
    'Plugins/ags_snowrain/msvc/ags_snowrain.vcxproj': plugin('ags_snowrain'),
    'Plugins/agsblend/msvc/agsblend.vcxproj': plugin('agsblend'),
    'Plugins/agsflashlight/msvc/AGSflashlight.vcxproj': plugin('agsflashlight'),
    'Plugins/agspalrender/msvc/agspalrender.vcxproj': plugin('agspalrender'),
    'Plugins/agstouch/msvc/agstouch.vcxproj': plugin('agstouch'),
}

# (.vcxproj pattern, source pattern) -> why the descriptions differ there on purpose (fnmatch patterns)
GTEST = 'Common/libsrc/googletest/googletest/src/*'
DELIBERATE = {
    (COMMON_LIB, 'Common/util/aasset_stream.cpp'): 'Android only: the file is inside #if AGS_PLATFORM_OS_ANDROID',
    (COMMON_LIB, 'Common/util/android_file.cpp'): 'Android only: the file is inside #if AGS_PLATFORM_OS_ANDROID',
    (COMMON_LIB, 'libsrc/freetype/src/base/ftdebug.c'):
        'CMake compiles it next to builds/windows/ftdebug.c, its Windows variant, which defines the same '
        'functions; the .vcxproj compiles the variant only',
    (ENGINE_APP, 'Plugins/*'):
        'the built-in plugins (AGS_BUILTIN_PLUGINS): acwin.exe loads them as DLLs, built by Plugins/*/msvc, '
        'whose entries compare them with the engine\'s copy',
    (ENGINE_APP, 'Engine/platform/android/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_ANDROID',
    (ENGINE_APP, 'Engine/platform/bsd/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_FREEBSD',
    (ENGINE_APP, 'Engine/platform/emscripten/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_EMSCRIPTEN',
    (ENGINE_APP, 'Engine/platform/ios/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_IOS',
    (ENGINE_APP, 'Engine/platform/linux/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_LINUX',
    (ENGINE_APP, 'Engine/platform/osx/*'): 'another platform\'s driver, inside #if AGS_PLATFORM_OS_MACOS',
    (ENGINE_APP, 'Engine/platform/base/agsplatform_xdg_unix.cpp'):
        'XDG Unix platforms only, inside #if AGS_PLATFORM_IS_XDG_UNIX',
    (ENGINE_APP, 'Engine/platform/base/mobile_base.cpp'): 'Android and iOS only, inside #if AGS_PLATFORM_MOBILE',
    (ENGINE_APP, 'Engine/platform/util/libc.c'): 'libc stand-ins for old mobile SDKs, all inside #if (0)',
    ('Solutions/*Test.vcxproj', GTEST):
        'googletest: the .vcxproj compile the submodule\'s sources; CMake builds the same commit '
        '(CMake/FetchGoogleTest.cmake) as its gtest targets, outside the repository',
    ('Solutions/Tests.App/Common.Lib.Test.vcxproj', 'Common/test/sdl2_stubs.c'):
        'stands in for SDL2, which Common.Lib.Test does not link; CMake\'s common_test links SDL2',
}

# .vcxproj with no CMake counterpart -> why
NOT_IN_CMAKE = {
    'Editor/AGS.Native/NativeDLL.vcxproj': 'the editor\'s C++/CLI bridge, built by MSBuild only, as the C# editor is',
}


def fail(message):
    print(message, file=sys.stderr)
    sys.exit(2)


def repo_path(path):
    """The path relative to the repository with forward slashes, or None if it is outside."""
    rel = os.path.relpath(os.path.normpath(path), REPO)
    if rel.startswith('..') or os.path.isabs(rel):
        return None
    return rel.replace(os.sep, '/')


def vcxproj_sources(project):
    """The repository paths of a .vcxproj's ClCompile and ResourceCompile items."""
    full = os.path.join(REPO, project)
    root = ET.parse(full).getroot()
    sources = set()
    for kind in ITEMS:
        for item in root.iter(MSBUILD_NS + kind):
            include = item.get('Include')
            if include is None:
                continue  # the settings of an ItemDefinitionGroup
            if item.find(MSBUILD_NS + 'ExcludedFromBuild') is not None:
                fail('%s: "%s" has ExcludedFromBuild, which this script does not resolve' % (project, include))
            for name in include.split(';'):
                if '$(' in name or '%(' in name or '*' in name:
                    fail('%s: "%s" uses a property or a wildcard, which this script does not resolve' % (project, name))
                path = repo_path(os.path.join(os.path.dirname(full), name.replace('\\', '/')))
                if path is None:
                    fail('%s: "%s" is outside the repository' % (project, name))
                sources.add(path)
    return sources


def linked_sources(project, seen=None):
    """The files of the projects that a project links, recursively."""
    seen = seen if seen is not None else set()
    result = set()
    for linked in PROJECTS[project]['links']:
        if linked not in seen:
            seen.add(linked)
            result |= vcxproj_sources(linked) | linked_sources(linked, seen)
    return result


def write_query(build):
    path = os.path.join(build, QUERY)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, 'w').close()


def cmake_targets(build):
    """{target name: set of the repository paths it compiles} from the File API reply of a build tree."""
    reply = os.path.join(build, REPLY)
    indexes = sorted(glob.glob(os.path.join(reply, 'index-*.json')))
    if not indexes:
        fail('no CMake File API reply in %s: run this script with --query, then configure the tree' % build)
    with open(indexes[-1], encoding='utf-8') as f:
        index = json.load(f)
    codemodel = next((o for o in index['objects'] if o['kind'] == 'codemodel'), None)
    if codemodel is None:
        fail('the File API reply in %s has no codemodel: configure the tree again after --query' % build)
    with open(os.path.join(reply, codemodel['jsonFile']), encoding='utf-8') as f:
        model = json.load(f)
    source_dir = model['paths']['source']
    targets = {}
    for target_ref in model['configurations'][0]['targets']:
        with open(os.path.join(reply, target_ref['jsonFile']), encoding='utf-8') as f:
            target = json.load(f)
        paths = set()
        for source in target.get('sources', []):
            if 'compileGroupIndex' not in source or source.get('isGenerated'):
                continue  # headers and other files that are listed but not compiled
            path = repo_path(os.path.join(source_dir, source['path']))
            if path is not None:
                paths.add(path)
        targets[target['name']] = paths
    return targets


def deliberate(project, path):
    """The DELIBERATE key that covers a difference, or None."""
    return next((k for k in DELIBERATE if fnmatch.fnmatchcase(project, k[0]) and fnmatch.fnmatchcase(path, k[1])),
                None)


def compare(targets):
    """[(project, [lines])] of the drifts, and the DELIBERATE keys that matched no difference."""
    drifts = []
    used = set()
    for project, spec in sorted(PROJECTS.items()):
        missing = [t for t in spec['own'] + spec['library'] if t not in targets]
        if missing:
            fail('%s: no CMake target %s in the build tree (configure it with -DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1)'
                 % (project, ', '.join(missing)))
        prefix = spec['prefix'] or ''
        own = {p for t in spec['own'] for p in targets[t] if p.startswith(prefix)}
        library = {p for t in spec['library'] for p in targets[t] if p.startswith(prefix)}
        listed = vcxproj_sources(project)
        differences = []
        for path in sorted(own - listed - linked_sources(project)):
            differences.append(('only in CMake (%s)' % ', '.join(t for t in spec['own'] if path in targets[t]), path))
        for path in sorted(listed - own - library):
            differences.append(('only in the .vcxproj', path))
        lines = []
        for where, path in differences:
            key = deliberate(project, path)
            if key is None:
                lines.append('  %s: %s' % (where, path))
            else:
                used.add(key)
        if lines:
            drifts.append((project, lines))
    return drifts, sorted(k for k in DELIBERATE if k not in used)


def unmapped_projects():
    listed = subprocess.run(['git', 'ls-files', '*.vcxproj'], cwd=REPO, capture_output=True, text=True,
                            check=True).stdout.split()
    return sorted(p for p in listed if p not in PROJECTS and p not in NOT_IN_CMAKE)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0], formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument('--build', required=True, help='CMake build directory, configured with tools and tests')
    ap.add_argument('--query', action='store_true', help='only write the File API query into the build directory')
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    if args.query:
        write_query(build)
        return 0
    drifts, stale = compare(cmake_targets(build))
    unmapped = unmapped_projects()
    for project, lines in drifts:
        print('%s:' % project)
        print('\n'.join(lines))
    for project, pattern in stale:
        print('DELIBERATE entry that matches no difference (remove it): %s, %s' % (project, pattern))
    for project in unmapped:
        print('%s: in neither PROJECTS nor NOT_IN_CMAKE (Script/check_build_lists.py)' % project)
    if drifts or stale or unmapped:
        print('\nThe build descriptions differ: add the file to the other one (the CMakeLists.txt, or the .vcxproj '
              'and its .filters), or give the difference a reason in DELIBERATE (Script/check_build_lists.py).')
        return 1
    print('%d .vcxproj match their CMake targets (%d deliberate differences; not in CMake: %s)'
          % (len(PROJECTS), len(DELIBERATE), ', '.join(sorted(NOT_IN_CMAKE))))
    return 0


if __name__ == '__main__':
    sys.exit(main())
