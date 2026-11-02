#!/usr/bin/env python3
"""
Game corpus regression (docs/refactor/PLAN.md, task 1.4): builds each game project with agsbuild, then
runs the engine in every room of it and reports pass or fail per room.

    python ci/corpus/run.py [<project dir> ...] --engine <path to ags.exe> [options]

Projects come from the arguments, or else from .compare/corpus.txt (one path per line, # for comments),
which is git-ignored: game paths are personal and stay out of the repository. Each project is built
from a fresh copy (Script/compare_builds.py's copy_project), with agsbuild --strict.

Each room runs in its own engine process (--startr <room>) for a few seconds, with the Software driver,
no sound and a full log, and fails if:
  - the engine exits before the time is up (a crash or a fatal error),
  - the log has no "Loading room <room>",
  - the log has an [ERROR] or [FATAL] message.
Warnings are listed per room, and fail it with --fail-on-warnings.

Options:
  --build-dir DIR     CMake build with the tools (default: .tidy/build)
  --work-dir DIR      where copies, builds and logs go (default: .compare/corpus-run)
  --seconds N         how long each room runs (default: 10)
  --jobs N            rooms run at the same time (default: 4)
  --fail-on-warnings  a warning fails the room too
"""
import argparse
import concurrent.futures
import glob
import os
import re
import shutil
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, os.path.join(REPO, 'Script'))
import compare_builds  # noqa: E402  (collect_tools, copy_project, run)

CORPUS_LIST = os.path.join(REPO, '.compare', 'corpus.txt')


def read_corpus_list():
    if not os.path.isfile(CORPUS_LIST):
        return []
    with open(CORPUS_LIST, encoding='utf-8') as f:
        return [line.strip() for line in f if line.strip() and not line.lstrip().startswith('#')]


def read_rooms(agf_path):
    """The room numbers listed in Game.agf."""
    text = open(agf_path, encoding='utf-8', errors='replace').read()
    rooms = re.search(r'<Rooms\b.*?</Rooms>', text, re.S)
    if not rooms:
        return []
    return sorted({int(n) for n in re.findall(r'<UnloadedRoom>.*?<Number>(\d+)</Number>', rooms.group(0), re.S)})


def run_room(engine, game_file, room, room_dir, seconds):
    """Runs the engine in one room; returns (failure or None, warnings)."""
    if os.path.isdir(room_dir):
        shutil.rmtree(room_dir)
    os.makedirs(room_dir)
    cmd = [engine, '--gfxdriver', 'Software', '--windowed', '--no-message-box', '--nosound',
           '--user-data-dir', room_dir, '--log-file=all', '--log-file-path=' + room_dir,
           '--startr', str(room), game_file]
    proc = subprocess.Popen(cmd, cwd=os.path.dirname(game_file), stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    try:
        code = proc.wait(timeout=seconds)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
        code = None
    log_path = os.path.join(room_dir, 'ags.log')
    log = open(log_path, encoding='utf-8', errors='replace').read() if os.path.isfile(log_path) else ''
    errors = sorted(set(re.findall(r'^.*\[(?:ERROR|FATAL)\].*$', log, re.M)))
    warnings = sorted(set(re.findall(r'^.*\[WARN\].*$', log, re.M)))
    if code is not None:
        last = [line for line in log.splitlines() if line.strip()][-3:]
        return f'engine exited with {code}: ' + ' / '.join(last)[:300], warnings
    if not re.search(rf'Loading room {room}\b', log):
        return 'the room was not loaded', warnings
    if errors:
        return '; '.join(errors)[:300], warnings
    return None, warnings


def run_project(project, args, sys_dir):
    name = os.path.basename(os.path.normpath(project))
    work = os.path.join(args.work_dir, name)
    copy = os.path.join(work, 'project')
    out = os.path.join(work, 'output')
    compare_builds.copy_project(project, copy)
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    code, secs = compare_builds.run([os.path.join(sys_dir, 'agsbuild' + compare_builds.EXE),
                                     os.path.join(copy, 'Game.agf'), out, '--sys-dir', sys_dir, '--strict'],
                                    os.path.join(work, 'agsbuild.log'), args.timeout)
    print(f'\n## {name}\n\nagsbuild: exit {code} in {secs:.0f}s (log: {work}/agsbuild.log)')
    games = glob.glob(os.path.join(out, '*.ags'))
    if code != 0 or not games:
        print('RESULT: BUILD FAILED')
        return False

    # The engine's libraries next to the game, which all room runs share
    engine_dir = os.path.dirname(os.path.abspath(args.engine))
    for dll in glob.glob(os.path.join(engine_dir, '*.dll')):
        shutil.copy2(dll, out)

    rooms = read_rooms(os.path.join(copy, 'Game.agf'))
    results = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_room, os.path.abspath(args.engine), os.path.abspath(games[0]), r,
                               os.path.join(work, 'rooms', str(r)), args.seconds): r for r in rooms}
        for future in concurrent.futures.as_completed(futures):
            results[futures[future]] = future.result()

    print(f'\n| room | result | warnings |\n|---:|---|---|')
    failed = 0
    for r in rooms:
        failure, warnings = results[r]
        if failure is None and warnings and args.fail_on_warnings:
            failure = 'warnings'
        failed += failure is not None
        shown = '; '.join(w.strip() for w in warnings)[:200]
        print(f'| {r} | {"FAIL: " + failure if failure else "pass"} | {shown} |')
    print(f'\n{len(rooms) - failed} of {len(rooms)} rooms pass (logs: {work}/rooms/<room>/ags.log)')
    return failed == 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('projects', nargs='*', help='game project directories (default: .compare/corpus.txt)')
    ap.add_argument('--engine', required=True, help='engine executable')
    ap.add_argument('--build-dir', default=os.path.join(REPO, '.tidy', 'build'))
    ap.add_argument('--work-dir', default=os.path.join(REPO, '.compare', 'corpus-run'))
    ap.add_argument('--seconds', type=int, default=10)
    ap.add_argument('--jobs', type=int, default=4)
    ap.add_argument('--timeout', type=int, default=900, help=argparse.SUPPRESS)  # per build
    ap.add_argument('--fail-on-warnings', action='store_true')
    args = ap.parse_args()
    args.work_dir = os.path.abspath(args.work_dir)

    projects = args.projects or read_corpus_list()
    if not projects:
        sys.exit(f'No projects given, and none listed in {CORPUS_LIST}')
    sys_dir = os.path.join(args.work_dir, 'tools')
    compare_builds.collect_tools(os.path.abspath(args.build_dir), sys_dir)

    ok = all([run_project(os.path.abspath(p), args, sys_dir) for p in projects])
    print('\nRESULT:', 'PASS' if ok else 'FAIL')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
