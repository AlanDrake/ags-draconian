# AGENTS.md

AGS Draconian: a **hard fork** of Adventure Game Studio (`adventuregamestudio/ags`, branch
`ags4`, fork point `4cd88beba`) used as a testbed for AI-assisted refactoring. The engine and
IDE for adventure games, open-sourced in 2011 after 12 years of closed development; large parts
are old C++ and C# with ties that are not obvious.

Entry points, in reading order:

| File | What | Read when |
|---|---|---|
| this file | rules, facts, verification | always |
| [docs/refactor/PLAN.md](docs/refactor/PLAN.md) "Next up" and the task's section | the refactor plan: phases, decisions D1-D7, task status | starting a plan task (read only the task, not the whole file: it is ~10k tokens) |
| [docs/refactor/STYLE.md](docs/refactor/STYLE.md) | fork code style and commit rules | writing or formatting code |
| [docs/refactor/LOG.md](docs/refactor/LOG.md) | finished tasks' definitions and dated status: measurements, bugs found on the way | looking up how a task went |
| [docs/context/](docs/context/README.md) | upstream knowledge about `ags4` (AGS 3 left out): repository map, workflows, decisions, roadmap, sources | changing engine/editor/tools code, designing, or preparing an upstream PR |
| [docs/UPSTREAM.md](docs/UPSTREAM.md) | cherry-pick ledger and the branches sent upstream | taking or sending upstream changes |

## Rules

- **The user decides.** Do not commit, push, open PRs/issues or post comments unless asked each
  time; no `Co-Authored-By` or tool trailers in commits. Work one PLAN.md task at a time; end a
  task by updating its status and "Next up", then stop.
- **One kind of change per commit** (`[format]`, `[rename]`, `[move]`, `[delete]`, or behaviour),
  titles `Program: [kind] what (task id)`; mechanical commits are verified with
  `python Script/compare_objects.py` (STYLE.md "Commits").
- **The compatibility contract C1-C6 in PLAN.md is frozen** (3.99+ game data, 3.99+ saves,
  script ABI, engine plugin API, project format, engine config). Everything else may change, the
  editor plugin API included (2026-11-04): better design wins over backward compatibility.
- **Safety net before surgery:** no deletion or restructuring of a subsystem before the tests
  that would catch a regression exist (PLAN.md ground rule 3).
- **Measure, don't guess:** build both ways and compare; run the tests; say what was not verified.
- Private game projects used as a corpus never appear in the repo (paths, names, content).
- Upstream work (`U.x` tasks) is done in the worktree `../ags4-upstream` on branches off
  `upstream/ags4`, in **upstream's style and C++11**, without fork-only features
  (`docs/context/decisions.md`, `docs/context/workflows.md` "Submit").

## Facts that differ from upstream

- C++17 everywhere first-party (upstream: C++11). `#pragma once`, no `using namespace` in
  headers, self-contained headers (`header_check` CMake target), sorted includes, Allman braces:
  all enforced by CI (`verify.yml`, clang-format 19.1.7 on whole files). Third-party code under
  `libsrc/`, `Common/libsrc`, `Engine/libsrc`, `Editor/scintilla` is never reformatted.
- The fork's default branch is **`ags4draconianr21`** (`git ls-remote --symref origin HEAD`); the
  old `draconian` branch is unused, whatever the session's "main branch" hint says. `gh` without
  `-R AlanDrake/ags-draconian` resolves to upstream.
- The legacy script compiler is gone: `Compiler/script/` **is the new compiler** (upstream keeps
  the old one in `script/` and the new one in `script2/`). `Solutions/Compiler.Lib` builds it.
- Opacity 0-255 replaces Transparency 0-100, tint ranges are 0-255, PO translation sources with
  references/contexts, D3D11 pixel-shader blenders, fake gamma, `DrawingSurface` primitives:
  fork features the upstream auto-test does not cover. The fork's test game is
  `AlanDrake/ags-test-games` branch `ags4-draconian`; CI builds it from source with `agsbuild`.
- Games build through the C++ tools (D7): `agsbuild` → `agf2dta`, `agscc`, `crmbuild` (rooms from
  `Rooms/<n>/`), `po2tra`, `agspak`; `agsbuild --json` gives machine-readable diagnostics;
  `Script/compare_builds.py` proves byte parity with the Editor's build; `ci/corpus/run.py` runs
  a private game through every room. The Editor still builds with its C# code until task 6.7d.
- Windows 7+ only; PSP is gone (3.1; `eOS_PSP` and the `"psp"` config name stay for the script ABI
  and engine config).
- CMake is the only build system for the engine, `agscc` and the tools (D5; 3.2a/b, 2026-11-05): the
  Makefiles, `make.yml` and `dh --buildsystem makefile` are gone, `debian/rules` calls CMake, CI installs
  the tools with `cmake --install`. Adding an engine `.cpp` means editing `Engine/CMakeLists.txt` (and
  the native `Solutions/*.vcxproj` until 3.2c). Two standalone plugin Makefiles stay
  (`Plugins/agslua`, `Plugins/agspalrender`, phase 7).
- Upstream is read-only: `git fetch upstream`, cherry-pick with `-x`, record in `docs/UPSTREAM.md`.

## Where things are (fork)

`docs/context/repository.md` applies, with these differences: `Compiler/script` (new compiler),
`Tools/crmbuild`, `Tools/data/xml_fields.h` (table-driven `Game.agf` reading, drift guard
`Tools/data/agf_elements.txt`), `Script/compare_objects.py`, `Script/compare_builds.py`,
`Script/refactor_metrics.py` (BASELINE.md numbers), `Script/tidy_fix.py`, `Script/tidy_diff.py` (clang-tidy on changed lines), `ci/corpus/`,
`.github/workflows/verify.yml`, `Script/prepush.py`. Local-only helpers and build trees live in the git-ignored
`.compare/` and `.tidy/`.

## Verify before you report done

Before a push run `python Script/prepush.py` (clang-format, CMake build + `ctest`, clang-tidy on the changed lines, header check with
MSVC and clang+libc++, the Emscripten engine build with a syntax check of the Linux-only code, the MSBuild solutions; `--list`, `--only`, `--skip`): CI confirms, it does not
discover. Workflows: `verify.yml` (fast gate, every push), `ci.yml` (`ags4draconian*` branches, PRs: light set;
tags, weekly, `gh workflow run ci.yml -f full=true`: every platform), `macos.yml` (tags, weekly, on demand;
standalone because hosted macOS runners are often unavailable).

CMake build + `ctest`; MSBuild `Tests.sln` and `AGS.Editor.Full.sln` when `Common/` or the
Editor changed; `clang-format --dry-run -Werror` on all tracked C/C++ files; the auto-test game;
`compare_objects.py` for mechanical commits; `compare_builds.py` for tool or `agfreader`
changes; the header check with a strict standard library before repo-wide pushes (CI should
confirm, not discover).

**What has broken CI so far**, most frequent first (LOG.md 0.4, 2.6, 2.8, 3.2; each is a rule now):

- **MSVC accepts what GCC, Clang, libc++, GLES2 or an old glibc reject** (7 breaks). A green local
  build covers Windows only; STYLE.md "Portability" lists the traps. When a change touches code
  Windows does not compile (`platform/` other than `windows`, `*_posix.*`, non-Windows `#if`
  branches, OpenGL code shared with GLES2), mechanical repo-wide changes included, run the
  `emscripten` stage of `prepush.py` (emcc builds the engine as GLES2, then the Linux-only code is
  syntax-checked), and name in the report what no local build compiled (macOS, iOS, Android and
  FreeBSD code; nothing is linked for Linux).
- **A source file or setting in one build description only** (3 breaks). A new, moved or deleted
  `.cpp`, tests included, goes in the `CMakeLists.txt` and in the `Solutions/` `.vcxproj` and
  `.vcxproj.filters` (until 3.2c); the `msbuild` stage of `prepush.py` finds a miss, so run it.
- **Workflow, Docker or CMake changes for jobs with no local equivalent** (3 breaks: the Linux
  builder image with Debian 10, GCC 8, glibc 2.28 and no downloads on i386; the clang job; Android;
  macOS). Do not send them through the full matrix: ask the user for a probe branch (a name outside
  `ags4draconian*`, a workflow with only the affected jobs, `MAKEFLAGS=-k`), then one full run.
- **On a red run** read every failed job, reproduce what can be reproduced, fix in one batch, push
  once. One speculative fix per run is how 2.6 took four pushes.
