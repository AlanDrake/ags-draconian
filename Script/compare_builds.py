#!/usr/bin/env python3
"""
Build-parity harness (docs/refactor/PLAN.md, task 7.1a): builds one game project with the AGS Editor
and with the C++ tools (agsbuild), then compares the results file by file.

    python Script/compare_builds.py <project dir> --editor <path to AGSEditor.exe> [options]

Both builds start from fresh copies of the project (Compiled/ and _Debug/ are left out). The game
packages (*.ags) are unpacked with agspak, so the table shows each packed file: game28.dta, rooms,
sprite file and index, audio, fonts, custom data; a "(package)" row compares each .ags package
itself (part names, asset order, ender). Exit code 0 means every file is identical.
A package split into parts (SplitResources) is compared by its assets only.

The one layout difference allowed: the editor embeds the compiled scripts in game28.dta, while agsbuild
packs them as separate files (GlobalScript.o, __DialogScripts.o and the modules in ScriptModules.lst),
which the engine loads since 3.6.1. The comparison takes the scripts out of the editor's game28.dta
(see unembed_scripts), so each separate file must match the script the editor embedded, and the rest
of game28.dta must match byte for byte.

Options:
  --variants LIST   instead of the project as it is, compare copies of it that use what the project
                    does not (PLAN.md 6.7d-1): a comma-separated list of variant names, or "all";
                    --list-variants prints them. Each variant prints its rows that are not identical,
                    and a summary table follows
  --all-rows        with --variants, print every row
  --build-dir DIR   CMake build with tools (default: .tidy/build); agsbuild, agscc and the other tools
                    are collected from it into one tools directory next to the script API header
  --engine PATH     also run the auto-test on the agsbuild output with this engine (Software driver)
  --work-dir DIR    where copies and outputs go (default: .compare/builds)
  --timeout SEC     per build (default: 900); a build that takes longer is killed and reported

The editor build needs Windows; it also needs acwin.exe next to AGSEditor.exe if the project
builds a Windows target.
"""
import argparse
import glob
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
API_HEADER = os.path.join(REPO, 'Editor', 'AGS.Editor', 'Resources', 'agsdefns.sh')
EXE = '.exe' if os.name == 'nt' else ''
TOOLS = ['agsbuild', 'agf2dta', 'agf2dlgasc', 'agfexport', 'agspak', 'crmbuild', 'crmpak', 'po2tra', 'spriteimport',
         'spritepak']


def run(cmd, log_path, timeout, cwd=None):
    """Runs a command with output to a log file; returns (exit code or None on timeout, seconds)."""
    import time
    start = time.time()
    with open(log_path, 'w', encoding='utf-8', errors='replace') as log:
        proc = subprocess.Popen(cmd, stdout=log, stderr=subprocess.STDOUT, cwd=cwd)
        try:
            code = proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
            code = None
    return code, time.time() - start


def collect_tools(build_dir, sys_dir):
    os.makedirs(sys_dir, exist_ok=True)
    found = {}
    for name in TOOLS + ['agscc']:
        candidates = glob.glob(os.path.join(build_dir, '**', name + EXE), recursive=True)
        candidates = [c for c in candidates if '_deps' not in c]
        if candidates:
            shutil.copy2(candidates[0], sys_dir)
            found[name] = candidates[0]
    shutil.copy2(API_HEADER, sys_dir)
    missing = [t for t in ['agsbuild', 'agscc', 'agspak'] if t not in found]
    if missing:
        sys.exit(f'Tools not found in {build_dir}: {", ".join(missing)} (build with -DAGS_BUILD_TOOLS=1)')
    return found


def copy_project(src, dst):
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    # Build products stay behind: the output folders, and agsbuild's intermediate folder
    shutil.copytree(src, dst, ignore=shutil.ignore_patterns('Compiled', '_Debug', '_obj', '*.lock', '.git'))
    # The editor packs a compiled room that is newer than its sources as it is; both builds must
    # compile AGS 4 rooms (Rooms/<n>/) from their sources, so the copy has none
    rooms_dir = os.path.join(dst, 'Rooms')
    for n in os.listdir(rooms_dir) if os.path.isdir(rooms_dir) else []:
        crm = os.path.join(dst, f'room{n}.crm')
        if os.path.isfile(os.path.join(rooms_dir, n, 'data.xml')) and os.path.isfile(crm):
            os.remove(crm)


def hash_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


# The parts of a split package after the first: <name>.001, .002 as agspak names them, and <name>.1D3,
# .2D3 as the editor did (a format-string slip in DataFileWriter.cs; the engine opens whatever names
# the first part lists)
PART_EXTENSION = re.compile(r'^\.(\d{3}|\d+D3)$')


def flatten_output(out_dir, unpack_dir, agspak):
    """Returns {name: (path, size)} for the deployed files, with *.ags and *.vox packages unpacked in
    place."""
    files = {}
    if os.path.isdir(unpack_dir):
        shutil.rmtree(unpack_dir)  # files of an earlier run would show up as this run's output
    if not os.path.isdir(out_dir):
        return files
    names = sorted(os.listdir(out_dir))
    packages = {os.path.splitext(n)[0].lower() for n in names if n.lower().endswith('.ags')}
    for name in names:
        path = os.path.join(out_dir, name)
        if not os.path.isfile(path):
            continue
        base, ext = os.path.splitext(name)
        if base.lower() in packages and PART_EXTENSION.match(ext):
            continue  # a further part of a split package: its assets are unpacked with the first part
        if name.lower().endswith(('.ags', '.vox')):
            dest = os.path.join(unpack_dir, name)
            os.makedirs(dest, exist_ok=True)
            subprocess.run([agspak, '-e', path, dest], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            for root, _, packed in os.walk(dest):
                for p in packed:
                    full = os.path.join(root, p)
                    rel = os.path.relpath(full, dest).replace(os.sep, '/')
                    files[f'{name}/{rel}'] = (full, os.path.getsize(full))
        else:
            files[name] = (path, os.path.getsize(path))
    return files


def ext_block_header(name):
    """An extension block's id, as Tools/data/data_file_writer.cpp WriteExtension writes it."""
    return b'\x00' + name.encode('ascii').ljust(16, b'\x00')


def separate_scripts(files, package):
    """agsbuild's compiled scripts in the order the editor embeds them (global, dialog, then
    ScriptModules.lst), as [(packed name, bytes)], or None if the package has no global script."""
    names = ['GlobalScript.o', '__DialogScripts.o']
    lst = files.get(package + 'ScriptModules.lst')
    if lst:
        names += [line.strip() for line in open(lst[0], encoding='utf-8').read().splitlines() if line.strip()]
    scripts = []
    for n in names:
        if package + n not in files:
            return None
        scripts.append((n, open(files[package + n][0], 'rb').read()))
    return scripts


def unembed_scripts(ed_dta, scripts):
    """Rewrites the editor's game28.dta as it is with the scripts left out, as agsbuild writes it.
    Returns (data, None), or (None, reason) if the editor's file doesn't hold exactly these scripts.

    The editor writes the scripts after the text parser dictionary (Common/game/main_game_file.cpp,
    ReadGameData), so three other places depend on them: the HasCCScript flag and the offset of the
    extension list, both in the GameSetupStructBase block before the scripts, and the script names at
    the start of the v362_interevent2 extension."""
    modules = scripts[2:]
    section = scripts[0][1] + scripts[1][1] + struct.pack('<i', len(modules)) + b''.join(m[1] for m in modules)
    pos = ed_dta.find(section)
    if pos < 0 or ed_dta.find(section, pos + 1) >= 0:
        return None, 'scripts differ'
    ext_start = ed_dta.find(ext_block_header('v360_fonts'), pos + len(section))
    field = ed_dta.find(struct.pack('<i', ext_start), 0, pos)
    if ext_start < 0 or field < 0 or ed_dta.find(struct.pack('<i', ext_start), field + 1, pos) >= 0:
        return None, 'extension offset not found'
    # GameSetupStructBase: the extension offset, then the dict, globalscript, chars and HasCCScript flags
    flag = field + 16
    if struct.unpack_from('<i', ed_dta, flag)[0] != 1:
        return None, 'HasCCScript flag not found'
    data = bytearray(ed_dta[:pos] + ed_dta[pos + len(section):])
    struct.pack_into('<i', data, field, ext_start - len(section))
    struct.pack_into('<i', data, flag, 0)

    # v362_interevent2: the global and dialog script names, the module count and names
    block = data.find(ext_block_header('v362_interevent2'), ext_start - len(section))
    if block < 0:
        return None, 'v362_interevent2 not found'
    length_at = block + 17
    names_at = length_at + 8

    def read_string(at):
        size = struct.unpack_from('<i', data, at)[0]
        return data[at + 4:at + 4 + size].decode('utf-8', 'replace'), at + 4 + size

    global_name, at = read_string(names_at)
    dialog_name, at = read_string(at)
    names = [global_name, dialog_name]
    count = struct.unpack_from('<i', data, at)[0]
    at += 4
    for _ in range(count):
        name, at = read_string(at)
        names.append(name)
    expected = [os.path.splitext(n)[0] + '.asc' for n, _ in scripts]
    if names != expected:
        return None, f'script names {names} instead of {expected}'
    data[names_at:at] = struct.pack('<iii', 0, 0, 0)
    length = struct.unpack_from('<q', data, length_at)[0]
    struct.pack_into('<q', data, length_at, length - (at - names_at - 12))
    return bytes(data), None


def package_layout(path):
    """What an asset library holds besides its assets (Common/data/multifilelib.cpp, format 30): the
    file names of its parts, the asset names in order, and whether it ends with an ender."""
    with open(path, 'rb') as f:
        head = f.read(1 << 20)
        f.seek(-12, os.SEEK_END)
        has_ender = f.read(12) == b'CLIB\x01\x02\x03\x04SIGE'
    if head[:5] != b'CLIB\x1a' or head[5] < 30:
        return None
    at = 7 + 4  # signature, version, part index; reserved options

    def cstr(at):
        end = head.index(b'\0', at)
        return head[at:end].decode('utf-8', 'replace'), end + 1

    count = struct.unpack_from('<i', head, at)[0]
    at += 4
    parts = []
    for _ in range(count):
        name, at = cstr(at)
        parts.append(name)
    count = struct.unpack_from('<i', head, at)[0]
    at += 4
    assets = []
    for _ in range(count):
        name, at = cstr(at)
        assets.append(name)
        at += 1 + 8 + 8  # part, offset, size
    return parts, assets, has_ender


def compare_packages(ed_path, tool_path):
    """Compares the layout of two packages; returns None if it matches, or what differs."""
    e, t = package_layout(ed_path), package_layout(tool_path)
    if e is None or t is None:
        return 'not a package of format 30'
    if len(e[0]) > 1 and len(t[0]) > 1:
        # Split packages are compared by their assets only (PLAN.md 6.7d-3): the editor named the parts
        # .1D3, .2D3, listed the assets unsorted and filled the parts by another rule than agspak
        return None
    if e[0] != t[0]:
        return f'part names {e[0]} and {t[0]}'
    if e[2] != t[2]:
        return 'ender only in the ' + ('editor\'s' if e[2] else 'agsbuild\'s')
    shared = set(e[1]) & set(t[1])
    if [a for a in e[1] if a in shared] != [a for a in t[1] if a in shared]:
        return 'assets in another order'
    return None


def run_autotest(engine, files_dir, work_dir, timeout):
    run_dir = os.path.join(work_dir, 'run')
    if os.path.isdir(run_dir):
        shutil.rmtree(run_dir)
    shutil.copytree(files_dir, run_dir)
    engine_dir = os.path.dirname(os.path.abspath(engine))
    for dll in glob.glob(os.path.join(engine_dir, '*.dll')):
        shutil.copy2(dll, run_dir)
    games = glob.glob(os.path.join(run_dir, '*.ags'))
    if not games:
        return 'no game package to run'
    code, _ = run([os.path.abspath(engine), '--gfxdriver', 'Software', '--no-message-box', '--user-data-dir', '.',
                   os.path.basename(games[0])], os.path.join(work_dir, 'engine.log'), timeout, cwd=run_dir)
    tap = os.path.join(run_dir, 'agstest.tap')
    if not os.path.isfile(tap):
        return f'engine exit {code}, no agstest.tap'
    text = open(tap, encoding='utf-8', errors='replace').read()
    ok = len(re.findall(r'^ok\b', text, re.M))
    not_ok = len(re.findall(r'^not ok\b', text, re.M))
    plan = re.search(r'^1\.\.(\d+)', text, re.M)
    return f'{ok} ok, {not_ok} not ok, of {plan.group(1) if plan else "?"} planned (engine exit {code})'


def set_element(agf_text, name, value, nth=0):
    """Game.agf with the text of the nth <name> element replaced."""
    found = list(re.finditer('<%s>[^<]*</%s>|<%s ?/>' % (name, name, name), agf_text))
    if len(found) <= nth:
        sys.exit(f'No <{name}> element number {nth} in Game.agf')
    m = found[nth]
    return agf_text[:m.start()] + f'<{name}>{value}</{name}>' + agf_text[m.end():]


def put_file(project, rel, data=b'parity'):
    full = os.path.join(project, *rel.split('/'))
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'wb') as f:
        f.write(data)


def variant_release(project, agf):
    return set_element(agf, 'DebugMode', 'False')


def variant_split(project, agf):
    # Parts of 1 MB; the padding makes the package large enough for three of them
    put_file(project, 'CustomData/padding1.bin', bytes(range(256)) * 3000)
    put_file(project, 'CustomData/padding2.bin', bytes(range(255, -1, -1)) * 3000)
    return set_element(agf, 'SplitResources', '1')


def variant_opengl(project, agf):
    return set_element(agf, 'GraphicsDriver', 'OpenGL')


def variant_customcfg(project, agf):
    put_file(project, 'acsetup.cfg', b'[misc]\r\nparity_key=1\r\n[graphics]\r\nvsync=1\r\n[mouse]\r\nspeed=1.5\r\n')
    return agf


def variant_packed(project, agf):
    for name in ('preload.pcx', 'flic1.flc', 'flic2.fli', 'intro.ogv'):
        put_file(project, name)
    return agf


def variant_speech(project, agf):
    for name in ('Speech/ego1.ogg', 'Speech/ego2.flac', 'Speech/ego3.wav', 'Speech/ego4.mp3', 'Speech/notes.txt',
                 'Speech/French/ego1.ogg', 'Speech/French/ego2.flac', 'Speech/Notes/readme.txt'):
        put_file(project, name)
    return agf


def variant_customdirs(project, agf):
    for name in ('Extra/Deep/a.txt', 'Extra/Deep/Sub/b.txt', 'CustomData/Nested/c.txt', 'Speech/Loose/d.txt'):
        put_file(project, name)
    return set_element(agf, 'CustomDataDir', 'Extra/Deep,CustomData,CustomData/Nested,Speech/Loose,../outside,.')


def variant_audiovox(project, agf):
    return set_element(agf, 'BundlingType', 'InSeparateVOX')


def variant_noname(project, agf):
    return set_element(agf, 'GameFileName', '')


def variant_unicode(project, agf):
    # A game name and a project folder (see VARIANT_DIRS) outside ASCII and outside the ANSI code pages,
    # and a translation, which gets the game name on po2tra's command line
    put_file(project, 'Parity.po', ('msgid ""\nmsgstr ""\n"Content-Type: text/plain; charset=UTF-8\\n"\n\n'
                                    'msgid "Hello"\nmsgstr "Ciao"\n').encode('utf-8'))
    agf = set_element(agf, 'GameName', 'Parit\u00e9 \u0436\u0434 "\u00f8"')
    return set_element(agf, 'Translations', '<Translation><Name>Parity</Name></Translation>')


VARIANTS = {
    'release': (variant_release, 'release mode (DebugMode off)'),
    'split': (variant_split, 'SplitResources: a package in parts'),
    'opengl': (variant_opengl, 'OpenGL as the default graphics driver'),
    'customcfg': (variant_customcfg, 'an acsetup.cfg in the project folder'),
    'packed': (variant_packed, 'preload.pcx, flic and ogv files in the project folder'),
    'speech': (variant_speech, 'a Speech folder with a sub-folder: voice packs'),
    'customdirs': (variant_customdirs, 'custom data folders: nested, with sub-folders, restricted, outside'),
    'audiovox': (variant_audiovox, 'an audio clip in audio.vox'),
    'noname': (variant_noname, 'an empty GameFileName'),
    'unicode': (variant_unicode, 'a game name and a project path outside ASCII, with a translation'),
}
# Variants whose project copies are built in a folder of this name: both builds in one of the same
# name, where the name counts
VARIANT_DIRS = {'noname': 'folder-name', 'unicode': 'proj\u00e9 \u0436\u0434'}


def make_variant(project, dst, name):
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    shutil.copytree(project, dst, ignore=shutil.ignore_patterns('Compiled', '_Debug', '*.lock', '.git'))
    agf_path = os.path.join(dst, 'Game.agf')
    with open(agf_path, encoding='utf-8', newline='') as f:
        agf = f.read()
    agf = VARIANTS[name][0](dst, agf)
    with open(agf_path, 'w', encoding='utf-8', newline='') as f:
        f.write(agf)


def status(code):
    if code is None:
        return 'timed out (killed)'
    return f'exit {code - (1 << 32) if code >= (1 << 31) else code}'  # Windows reports -1 as 4294967295


def compare(project, work, sys_dir, args, all_rows=True, engine=None, dir_name=None):
    """Builds the project both ways under the work folder and prints the comparison. Returns
    (parity, one-line summary)."""
    os.makedirs(work, exist_ok=True)
    agspak = os.path.join(sys_dir, 'agspak' + EXE)

    # Editor build
    ed_proj = os.path.join(work, 'editor-project', dir_name) if dir_name else os.path.join(work, 'editor-project')
    copy_project(project, ed_proj)
    ed_code, ed_time = run([os.path.abspath(args.editor), '/compile', os.path.join(ed_proj, 'Game.agf')],
                           os.path.join(work, 'editor.log'), args.timeout)
    ed_out = os.path.join(ed_proj, 'Compiled', 'Data')

    # agsbuild
    tool_proj = os.path.join(work, 'tools-project', dir_name) if dir_name else os.path.join(work, 'tools-project')
    copy_project(project, tool_proj)
    tool_out = os.path.join(work, 'tools-output')
    if os.path.isdir(tool_out):
        shutil.rmtree(tool_out)
    os.makedirs(tool_out)
    tool_code, tool_time = run([os.path.join(sys_dir, 'agsbuild' + EXE), os.path.join(tool_proj, 'Game.agf'),
                                tool_out, '--sys-dir', sys_dir], os.path.join(work, 'agsbuild.log'), args.timeout)

    ed_files = flatten_output(ed_out, os.path.join(work, 'unpacked-editor'), agspak)
    tool_files = flatten_output(tool_out, os.path.join(work, 'unpacked-tools'), agspak)

    print(f'editor:   {status(ed_code)} in {ed_time:.0f}s, {len(ed_files)} files  (log: {work}/editor.log)')
    print(f'agsbuild: {status(tool_code)} in {tool_time:.0f}s, {len(tool_files)} files  (log: {work}/agsbuild.log)')
    if tool_code not in (0, None):  # print the distinct errors
        log = open(os.path.join(work, 'agsbuild.log'), encoding='utf-8', errors='replace').read()
        errors = sorted(set(re.findall(r'^(?:Error|Failure): .*$', log, re.M)))
        for e in errors[:12]:
            print(f'  {e[:160]}')
        if len(errors) > 12:
            print(f'  ... {len(errors) - 12} more distinct errors')

    # The scripts agsbuild packs separately are compared with those the editor embeds in game28.dta
    unembedded = {}  # game28.dta -> (the editor's file without the scripts, or None; the reason)
    embedded = set()  # separate script files found in the editor's game28.dta
    for n in tool_files:
        if not n.endswith('/game28.dta') or n not in ed_files:
            continue
        package = n[:-len('game28.dta')]
        scripts = separate_scripts(tool_files, package)
        if scripts is None:
            continue
        unembedded[n] = unembed_scripts(open(ed_files[n][0], 'rb').read(), scripts)
        if unembedded[n][0] is not None:
            embedded.update(package + s for s, _ in scripts)
            embedded.add(package + 'ScriptModules.lst')

    names = sorted(set(ed_files) | set(tool_files))
    counts = {'identical': 0, 'different': 0, 'only editor': 0, 'only agsbuild': 0}
    rows = []
    for n in names:
        e, t = ed_files.get(n), tool_files.get(n)
        note = ''
        if e and t and n in unembedded:
            data, reason = unembedded[n]
            result = 'identical' if data is not None and data == open(t[0], 'rb').read() else 'different'
            note = f' ({reason})' if reason else ' (scripts separate)'
        elif e and t:
            result = 'identical' if e[1] == t[1] and hash_file(e[0]) == hash_file(t[0]) else 'different'
        elif t and n in embedded:
            result, note = 'identical', ' (embedded in the editor\'s game28.dta)'
        else:
            result = 'only editor' if e else 'only agsbuild'
        counts[result] += 1
        rows.append((result, f'| {n} | {result}{note} | {e[1] if e else ""} | {t[1] if t else ""} |'))
    # The packages themselves, besides the files in them: part names, asset order, ender
    for n in sorted(os.listdir(ed_out) if os.path.isdir(ed_out) else []):
        ed_pkg, tool_pkg = os.path.join(ed_out, n), os.path.join(tool_out, n)
        if not n.lower().endswith(('.ags', '.vox')) or not os.path.isfile(tool_pkg):
            continue
        reason = compare_packages(ed_pkg, tool_pkg)
        result = 'different' if reason else 'identical'
        names.append(n + ' (package)')
        counts[result] += 1
        rows.append((result, f'| {n} (package) | {result}{f" ({reason})" if reason else ""} | | |'))
    shown = [row for result, row in rows if all_rows or result != 'identical']
    if shown:
        print('\n| file | result | editor bytes | agsbuild bytes |\n|---|---|---:|---:|')
        print('\n'.join(shown))
    summary = ', '.join(f'{v} {k}' for k, v in counts.items())
    print('\n' + summary)

    if engine:
        print('auto-test on agsbuild output:', run_autotest(engine, tool_out, work, args.timeout))

    ok = bool(names) and counts['identical'] == len(names) and ed_code == 0 and tool_code == 0
    if ed_code != 0 or tool_code != 0:
        summary += f'; editor {status(ed_code)}, agsbuild {status(tool_code)}'
    print('RESULT:', 'PARITY' if ok else 'NO PARITY')
    return ok, summary


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('project', nargs='?', help='game project directory (containing Game.agf)')
    ap.add_argument('--editor', help='path to AGSEditor.exe')
    ap.add_argument('--variants', help='comma-separated variant names, or "all"')
    ap.add_argument('--list-variants', action='store_true')
    ap.add_argument('--all-rows', action='store_true')
    ap.add_argument('--build-dir', default=os.path.join(REPO, '.tidy', 'build'))
    ap.add_argument('--engine', help='engine executable for the auto-test run')
    ap.add_argument('--work-dir', default=os.path.join(REPO, '.compare', 'builds'))
    ap.add_argument('--timeout', type=int, default=900)
    args = ap.parse_args()
    sys.stdout.reconfigure(errors='replace')  # paths outside the console's code page

    if args.list_variants:
        for name, (_, what) in VARIANTS.items():
            print(f'{name:12}{what}')
        return 0
    if not args.project or not args.editor:
        ap.error('the project and --editor are required')
    project = os.path.abspath(args.project)
    if not os.path.isfile(os.path.join(project, 'Game.agf')):
        sys.exit(f'No Game.agf in {project}')
    work = os.path.abspath(args.work_dir)
    os.makedirs(work, exist_ok=True)
    sys_dir = os.path.join(work, 'tools')
    collect_tools(os.path.abspath(args.build_dir), sys_dir)

    if not args.variants:
        return 0 if compare(project, work, sys_dir, args, engine=args.engine)[0] else 1

    chosen = list(VARIANTS) if args.variants == 'all' else args.variants.split(',')
    unknown = [v for v in chosen if v not in VARIANTS]
    if unknown:
        sys.exit(f'Unknown variants: {", ".join(unknown)} (see --list-variants)')
    results = []
    for name in chosen:
        print(f'\n== {name}: {VARIANTS[name][1]}')
        variant_work = os.path.join(work, 'variants', name)
        source = os.path.join(variant_work, 'source')
        make_variant(project, source, name)
        results.append((name,) + compare(source, variant_work, sys_dir, args, all_rows=args.all_rows,
                                         dir_name=VARIANT_DIRS.get(name)))
    print('\n| variant | what | result | files |\n|---|---|---|---|')
    for name, ok, summary in results:
        print(f'| {name} | {VARIANTS[name][1]} | {"parity" if ok else "NO PARITY"} | {summary} |')
    return 0 if all(ok for _, ok, _ in results) else 1


if __name__ == '__main__':
    sys.exit(main())
