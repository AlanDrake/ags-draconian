#!/usr/bin/env python3
"""Builds a compatibility fixture (docs/refactor/PLAN.md, task 1.2) with an installed AGS release.

    python make_fixtures.py --ags-dir "C:/Program Files/Adventure Game Studio 4.0" --version 4.0.0.30
        --agspak <build>/agspak.exe [--keep <work dir>]

Steps: unpack the release's own Sierra-style template, give its room a flat background (the
fixture stays small) and add fixture.asc to the global script, build it headless with the release's
editor (`AGSEditor.exe /compile`), then run the release's engine once: the script sets known state,
saves to slot 1 and quits. The game package and the save go to <version>/ next to this script.
The tests run the game with the fork's engine: the script restores the save and checks the state.
Windows only (the releases are Windows programs).
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))


def patch_project(proj):
    bg = os.path.join(proj, "Rooms", "1", "background0.png")
    im = Image.open(bg)
    flat = Image.new(im.mode, im.size, im.getpixel((0, 0)))
    if im.mode == "P":
        flat.putpalette(im.getpalette())
    flat.save(bg)
    # The template ships a compiled room; remove it so the editor rebuilds it from the sources
    for name in os.listdir(proj):
        if name.lower().endswith(".crm"):
            os.remove(os.path.join(proj, name))

    gs = os.path.join(proj, "GlobalScript.asc")
    src = open(gs, encoding="utf-8-sig").read()
    block = open(os.path.join(HERE, "fixture.asc"), encoding="utf-8").read()
    nl = "\r\n" if "\r\n" in src else "\n"
    block = block.replace("\r\n", "\n").replace("\n", nl)
    m = re.search(r"function repeatedly_execute\(\)\s*\{", src)
    if not m:
        sys.exit("the template's GlobalScript.asc has no repeatedly_execute")
    src = src[:m.start()] + block + src[m.start():m.end()] + nl + "  c1_tick();" + src[m.end():]
    open(gs, "w", encoding="utf-8", newline="").write(src)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--ags-dir", required=True, help="the installed AGS release")
    ap.add_argument("--version", required=True, help="fixture directory name, e.g. 4.0.0.30")
    ap.add_argument("--agspak", required=True, help="agspak built from this repository")
    ap.add_argument("--keep", help="work directory to keep (default: a temporary one)")
    args = ap.parse_args()
    args.agspak = os.path.abspath(args.agspak)

    work = args.keep or tempfile.mkdtemp(prefix="ags-compat-")
    proj = os.path.join(work, "project")
    saves = os.path.join(work, "saves")
    shutil.rmtree(proj, ignore_errors=True)
    shutil.rmtree(saves, ignore_errors=True)
    os.makedirs(proj)
    os.makedirs(saves)

    template = os.path.join(args.ags_dir, "Templates", "Sierra-style.agt")
    subprocess.run([args.agspak, "-e", template, proj], check=True, stdout=subprocess.DEVNULL)
    patch_project(proj)

    editor = os.path.join(args.ags_dir, "AGSEditor.exe")
    r = subprocess.run([editor, "/compile", os.path.join(proj, "Game.agf")], capture_output=True, text=True,
                       timeout=300)
    print(r.stdout.strip())
    data = os.path.join(proj, "Compiled", "Data")
    pkgs = [f for f in os.listdir(data) if f.endswith(".ags")] if os.path.isdir(data) else []
    if len(pkgs) != 1:
        sys.exit("the editor did not build the game")
    game = os.path.join(data, pkgs[0])

    engine = os.path.join(args.ags_dir, "acwin.exe")
    subprocess.run([engine, "--windowed", "--gfxdriver", "Software", "--no-message-box", "--nosound",
                    "--user-data-dir", saves, game], timeout=120, check=False,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    save = os.path.join(saves, "agssave.001")
    if not os.path.isfile(save):
        sys.exit("the engine did not write the save")

    out = os.path.join(HERE, args.version)
    os.makedirs(out, exist_ok=True)
    shutil.copyfile(game, os.path.join(out, "game.ags"))
    shutil.copyfile(save, os.path.join(out, "agssave.001"))
    print(f"wrote {out}: game.ags {os.path.getsize(game)} bytes, agssave.001 {os.path.getsize(save)} bytes")
    if not args.keep:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
