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
- **No silent performance loss:** a change that is not meant to make something faster must not make it
  slower in the builds given to users, the Release ones (Debug builds are internal and do not count).
  On a hot path (script VM, pixel loops, `Common/util`), compare the code of the objects that changed
  or benchmark, before and after, with MSVC and GCC, and put the result in the commit message; a cost
  is reported with its numbers and the user decides (STYLE.md "Performance").
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
  `ci/corpus/run.py` runs a private game through every room. The Editor builds through them too
  (6.7d, 2026-11-07): Build, Run, templates and translations run `agsbuild`, `agspak`, `crmbuild` and
  `po2tra` from `Editor/AGS.Editor/Build/`; the C# data writer, the in-process compile and the C#
  preprocessor are gone, so `docs/context/repository.md`'s `DataFileWriter.cs` steps do not apply: a
  project property the engine needs goes to `Tools/data/agfreader.cpp` and `data_file_writer.cpp` only.
  `Script/compare_builds.py` proves byte parity between the Editor's build and a plain `agsbuild` run
  (CI's `Windows Build Parity` job), and with `--tools-editor` against the reference editor from the
  tag `pre-6.7d`, the last one with the C# build.
- Windows 7+ only; PSP is gone (3.1; `eOS_PSP` and the `"psp"` config name stay for the script ABI
  and engine config).
- Game data from 3.99 on (D1; 3.4-1, 2026-11-07; upstream loads 3.6.0.21+): `kGameVersion_LowSupported =
  kGameVersion_399`, the pre-3.99 upgrade paths and version values are gone, an older game gets a "compiled
  with ... this engine loads games made with AGS 3.99 or later" error. Rooms, saves and the script API are
  still at upstream's minimums (3.4 steps 2 to 4).
- CMake is the only build system for the engine, `agscc` and the tools (D5; 3.2a/b, 2026-11-05): the
  Makefiles, `make.yml` and `dh --buildsystem makefile` are gone, `debian/rules` calls CMake, CI installs
  the tools with `cmake --install`. Adding an engine `.cpp` means editing `Engine/CMakeLists.txt` (and
  the native `Solutions/*.vcxproj` until 3.2c). Two standalone plugin Makefiles stay
  (`Plugins/agslua`, `Plugins/agspalrender`, phase 7).
- Upstream is read-only: `git fetch upstream`, cherry-pick with `-x`, record in `docs/UPSTREAM.md`.

## Where things are (fork)

`docs/context/repository.md` applies, with these differences: `Compiler/script` (new compiler),
`Editor/AGS.Editor/Build/` (the editor's tool runners: `BuildToolRunner` for `agsbuild --json`,
`ToolProcess` for the others, `TemplatePackage`, `TranslationCompiler`; no `DataFileWriter.cs`, no
`ScriptCompiler.cpp` in `AGS.Native`; `AGS.CScript.Compiler` is gone, `FastString` lives in
`Editor/AGS.Editor/Utils/`),
`Tools/crmbuild`, `Tools/data/xml_fields.h` (table-driven `Game.agf` reading, drift guard
`Tools/data/agf_elements.txt`), `Script/compare_objects.py`, `Script/compare_builds.py`,
`Script/refactor_metrics.py` (BASELINE.md numbers), `Script/tidy_fix.py`, `Script/tidy_diff.py` (clang-tidy on changed lines), `ci/corpus/`,
`.github/workflows/verify.yml`, `Script/prepush.py`, `Script/linux_jobs.sh` (the Linux CI jobs, run by
`prepush.py`). Local-only helpers and build trees live in the git-ignored
`.compare/` and `.tidy/`.

## Verify before you report done

`python Script/prepush.py --auto` before a push runs the stages that the changed files need, by the
`AUTO_RULES` table in the script (the editor's C# code: `msbuild` alone; the tools and the compiler: all
but `emscripten`; Common, Engine, Plugins: every stage; an unknown path: every stage), and lists the
changed files that no stage verifies (workflows, packaging, most scripts). `--dry-run` shows the choice.
Without `--auto` every stage runs, which a repository-wide change needs (`--list`, `--only`, `--skip`,
`--base`, `--emsdk <dir>`): `format`, `build`
(+ `ctest`), `tidy` (clang-tidy on changed lines), `headers`, `libcxx`, `emscripten` (engine as GLES2, then
a syntax check of the Linux-only code), `linux` (the CI's GCC job: build, header check, `ctest`), `msbuild`.
On demand with `--only`: `sanitizers` (ASan, UBSan and leaks on the unit tests and the auto-test game),
`coverage`. The Linux stages run `Script/linux_jobs.sh` in a WSL 2 distro (`Ubuntu-24.04`, as CI's
`ubuntu-latest`) on a copy of the working tree, uncommitted files included. CI confirms, it does not
discover. Also: the auto-test game; `compare_objects.py` for mechanical commits; `compare_builds.py` for
tool or `agfreader` changes; the generated code or a benchmark for a change on a hot path (STYLE.md
"Performance").

Workflows: `verify.yml` (every push: format, GCC build + tests + auto-test game + tidy gate, header
checks), `ci.yml` (`ags4draconian*`, PRs: light set, which is the Windows CMake build with Ninja and the
auto-test game plus the MSBuild jobs as x86 Release; tags, weekly, `gh workflow run ci.yml -f full=true`:
every platform, Debug and x64). The CMake jobs keep a ccache between runs (PLAN.md 2.9).
`macos.yml` (tags, weekly, on demand; macOS runners are often unavailable), `analysis.yml` (on demand:
ASan + UBSan 27 min, coverage per directory 14 min).

`ags_warning_free(<target>)` marks a target without warnings (`common` so far); with
`-DAGS_WARNINGS_AS_ERRORS=ON` (`verify.yml`, `prepush.py`; off by default) a warning there is an error.

A sanitizer report in first-party code is fixed, not suppressed (that includes an SDL or OpenAL object we
created and did not destroy); a report whose cause is inside third-party code goes into
`ci/sanitizers/lsan.supp` or `ubsan.supp` with its stack and reason, as narrow as possible. The files are
read by `Script/linux_jobs.sh` and `analysis.yml`, which are kept in step.

**What has broken CI** (13 times; LOG.md 0.4, 2.6, 2.8, 3.2):

- **MSVC-only verification** (7): GCC, Clang, libc++, GLES2 and old glibc reject what MSVC accepts
  (STYLE.md "Portability"). A change to code Windows does not compile (`platform/` other than `windows`,
  `*_posix.*`, non-Windows `#if` branches, GL code shared with GLES2), repo-wide edits included, needs
  the `emscripten` and `linux` stages; report what no local build compiled (macOS, iOS, Android,
  FreeBSD, the Debian 10 builder image).
- **One build description only** (3): a new, moved or deleted `.cpp`, tests included, goes in
  `CMakeLists.txt` and in the `Solutions/` `.vcxproj` + `.filters` (until 3.2c); the `msbuild` stage
  finds a missing source, not a missing test file, whose tests then never run on x86 (LOG.md 3.2,
  2026-11-07). A new test target also needs a `Tests.App` project, `ci.yml` and `prepush.py` `TEST_RUNNERS`.
- **CI-only jobs changed blind** (3): the Linux builder image (Debian 10, GCC 8, glibc 2.28, no downloads
  on i386), Android, macOS, new workflows. Ask the user for a probe branch (a name outside
  `ags4draconian*`, only the affected jobs), then one full run.
- **Red run:** read every failed job, fix in one batch, push once.
