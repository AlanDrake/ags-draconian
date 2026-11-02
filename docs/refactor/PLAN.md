# AGS Draconian refactor plan

A staged plan for turning a codebase that grew from a 1990s DOS program into one that is
consistent, testable and easy to navigate, **without breaking the games built on it**.

- **Style rules:** [STYLE.md](STYLE.md)
- **Starting numbers:** [BASELINE.md](BASELINE.md), recomputed with `python Script/refactor_metrics.py`
- **Status:** tick the boxes in this file as tasks land, and reference the task ID (`3.2`) in commit
  messages and PRs.

## How to use this plan

The work is split into phases. Each phase has numbered tasks, and every task has a **done when**
line you can check. Phases 0–2 must be done in order and before anything else: they build the
safety net and the tooling. After that, phases 3–7 can overlap and be done **module by module**.
For example, `Engine/ac/overlay.cpp` can go through phase 4 while the Editor is still in phase 6.

```mermaid
graph LR
  P0[0 Fork foundation] --> P1[1 Safety net]
  P1 --> P2[2 Mechanical normalisation]
  P2 --> P3[3 Dead code]
  P2 --> P4[4 Names & comments]
  P3 --> P5[5 Structure]
  P4 --> P5
  P2 --> P6[6 Editor]
  P2 --> B[7.1 Build tools]
  B --> P6
  P3 --> P7[7.2-7.3 Plugins, ports]
```

Task 7.1 (the C++ build tools, D7) doesn't depend on phases 3–5 and can start right after phase 2.
The editor's switch to the tools (6.7d) waits for it.

Rough sizing, for one developer part-time: phases 0–2 take weeks, phases 3–4 take a few months,
and phases 5–6 are ongoing over a year or more. Each phase is useful on its own, so stopping
after any of them still leaves the code better than before.

## Ground rules

1. **It's a hard fork.** Upstream `adventuregamestudio/ags` is no longer merged wholesale. Fixes
   are cherry-picked by hand and recorded in `docs/UPSTREAM.md` (task 0.2). Being free of merges is
   what makes the reformat, renames and moves in this plan affordable, so make full use of it.
2. **The compatibility contract is sacred** (see below). Anything not on it may change freely.
3. **Safety net before surgery.** No structural change to a subsystem until phase 1 has given it
   tests that would catch a regression.
4. **One kind of change per commit** (STYLE.md, "Commits"): format, rename, move, delete, or
   behaviour. Mechanical commits are verified by object-code comparison (task 1.6).
5. **Ratchets, not big bangs.** CI enforces style on changed lines from day one. Metrics in
   BASELINE.md may only go down. A PR that makes a metric worse needs a stated reason.
6. **Boy-scout rule, with limits.** When you touch a function for a fix, you may rename its locals
   and fix its comments in a *separate* preceding commit. Don't refactor the surroundings in the
   same PR as the fix.

## Compatibility contract

These interfaces are frozen unless a task explicitly versions them. Each one gets a guard test in
phase 1.

| # | Interface | Where it lives | Why it's frozen |
|---|---|---|---|
| C1 | Game data: `.ags`, `.crm`, sprite file + index, `.tra`, `.wfn`, **from 3.99 onward** (D1) | `Common/game/*`, `Common/ac/spritefile.*`, `Common/data/*` | Games built with 3.99/4.x editors must keep loading |
| C2 | Savegame format, for saves written by 3.99+ engines | `Engine/game/savegame*.cpp` | Players' saves of those games |
| C3 | Script ABI: exported API names and signatures, `game.*`/`system.*` field offsets, bytecode | `Register*API()`, `Engine/ac/dynobj/scriptgame.cpp` and friends, `Engine/script/scriptexecutor.cpp`, `Editor/AGS.Editor/Resources/agsdefns.sh` | Compiled game scripts link by name and offset |
| C4 | Plugin API: `IAGSEngine` vtable order, `AGS_Engine*` exports | `Engine/plugin/agsplugin.h` | Binary plugins built against the SDK |
| C5 | Editor project format (`Game.agf`, `.user`, room `.crm` source) and editor plugin API | `Editor/AGS.Types` | Existing projects and editor plugins |
| C6 | Engine command line and `acsetup.cfg` keys | `Engine/main/main.cpp`, `Engine/main/config.cpp` | Launchers, store builds, players' configs |

Note that C3 is defined by offsets and names, **not** by C++ struct layout. For example,
`GamePlayState` fields are read through the offset switch in `CCScriptGame::ReadInt32`, so
renaming or reordering the C++ fields is safe as long as that table stays correct.

## Purpose of the fork

This fork is a testbed for **AI-assisted refactoring**. That affects how the tasks are written:
each one should fit in a single session, have a "done when" line a machine can check, and be
backed by an automatic verifier (tests, the contract snapshots from 1.2, the object-code comparison
from 1.6, the metrics script). When a session starts a task, it should read this file, STYLE.md and
the task's section, and finish by updating the checkbox and the progress table.

## Decisions

Recorded 2026-10-31.

| ID | Question | Decision | Consequences | Unblocks |
|---|---|---|---|---|
| D1 | Oldest game version the fork must load (C1) | **3.99 and later** (`kGameVersion_399`) | All 3.x upgrade paths can go (task 3.4). Room versions need care, see 3.4. | 3.4 |
| D2 | Which platforms to ship on | **Windows, Linux, macOS, Android, Web, iOS.** Drop PSP. FreeBSD stays for now (small, shares the POSIX code). | iOS can't be tested locally (no Mac), so it gets a compile-only CI job (7.3) | 3.1, 7.3 |
| D3 | Keep the legacy script compiler? | **Remove `Compiler/script`** | `script2` becomes the only compiler (3.3) | 3.3 |
| D4 | Future of the Editor UI | **Avalonia is the main candidate.** The WinForms editor goes into maintenance mode. | Editor work concentrates on a UI-free core shared by both UIs, and on replacing the C++/CLI bridge, which Avalonia can't use (phase 6) | 6.x |
| D5 | Single build system for native code | **CMake only.** MSBuild stays for the C# editor projects. | Hand-written Makefiles and the native `Solutions/*.vcxproj` go (3.2) | 3.2 |
| D6 | Minimum Windows version | **Windows 7+** | Drop `v141_xp`; `AGS.Native` targets `0x0601` instead of XP's `0x0501`; no APIs newer than Windows 7 without a runtime check | 0.4, 3.1 |
| D7 | How are games built, by the editor and headless? | **One build implementation, in C++: the tools in `Tools/`**, with `agsbuild` running `agf2dta`, `agscc`, `crmpak`, `trac`, `agspak` and the rest. The editor saves the project and runs `agsbuild`, or individual tools where it needs finer control. The C# build code (`DataFileWriter.cs`, `DialogScriptConverter.cs`, the C# script-header generation and translation writer, the C# preprocessor) is deleted once the tools reach parity. *Revised 2026-11-01 after maintainer review; replaces "one C# build core plus a C# command-line builder", which was argued from commit counts, a poor proxy for parity.* | Builds need only a C++ toolchain, so they run where .NET doesn't (Linux i386, which CI packages; FreeBSD, D2) and in CI without the editor. The game-data writer sits next to the engine's reader: same structs, testable with C++ round trips. Cost: `Game.agf` is read by both C# (editor) and C++ (`agfreader`), so a new project property needs a C++ change too; 7.1g makes a missed one fail tests instead of silently doing nothing. Starting point, measured 2026-11-01: the tools **can't build an AGS 4 project yet** (five gaps, listed in 7.1). | 1.7, 3.3, 6.3, 6.7d, 7.1 |

## Next up

Kept current as tasks land. Model and effort are recommendations for an AI session (see "Purpose
of the fork").

| # | Task | Why now | Model / effort |
|---|---|---|---|
| 1 | **1.7:** hand the game that the Linux job builds to the platform jobs, in place of the downloaded release | Keeps the test game in step with the compiler; wait until the Linux job has stayed green for a few runs | Sonnet 5.5, `medium` |
| 2 | **7.1h:** machine-readable diagnostics and clean cancel for `agsbuild` | The editor (6.7d) needs them to list tool errors | Sonnet 5.5, `high` |
| 3 | **7.1i:** split `BuildWithTaskManager` into one function per stage | Easier to change while 7.1h touches it | Sonnet 5.5, `medium` |

Other tracks that can run in between: 2.6 (self-contained headers, Sonnet 5.5 `medium`) and 1.3 (engine
unit tests, Opus 5.5 `xhigh`).

---

## Phase 0: Fork foundation

- [x] **0.1 Tag the fork point.** Tag `fork-point` at `4cd88beba` (the current merge base with
  `upstream/ags4`). Keep the `upstream` remote for reference only.
  *Done when:* the tag exists and the README states the project is a hard fork.
- [x] **0.2 Cherry-pick ledger.** Create `docs/UPSTREAM.md`: a table of upstream commits that were
  picked, adapted or deliberately skipped. Every cherry-pick gets a line.
  *Done when:* the file exists and the cherry-pick process is described in CONTRIBUTING.md.
- [x] **0.3 Record decisions D1–D6** in the table above.
- [x] **0.4 Raise the toolchain floor.** `CXX_STANDARD 17` in all `CMakeLists.txt` (currently 11 in
  `Engine/` and `Common/`), `LanguageStandard stdcpp17` in all `.vcxproj`, `cmake_minimum_required`
  3.16+. For D6 (Windows 7+): replace the `v141_xp` toolset in `Solutions/Common.Lib` and the other
  projects with `v143`, and raise `WINVER`/`_WIN32_WINNT` in `Editor/AGS.Native/stdafx.h` from
  `0x0501` (XP) to `0x0601` (Windows 7). The Vista check in `Engine/platform/windows/acplwin.cpp`
  becomes always true and can go.
  *Done when:* CI is green on every platform with C++17, and no XP toolset or define remains.
  *Status (2026-11-01):* implemented, not yet committed. Verified on Windows only: `Engine.sln`,
  `Tests.sln` and `AGS.Editor.Full.sln` (Release x86, MSBuild) and the CMake build (x64, tools + tests)
  build with 0 errors; unit tests pass (MSBuild: Common 96, Engine 3, Tools 76; CMake ctest: 739/739);
  `ags.exe` still links the static runtime. CI (run 41287882653 and the re-run of its failed jobs):
  green on Windows, MinGW x64/arm64, Ubuntu x64/arm64, macOS, and the Linux packaging jobs (Debian
  Buster image, glibc 2.28). Android and Emscripten failed because the fork's fake-gamma code
  (`f6119c4bb`) called `glColor4f`, which OpenGL ES2 doesn't have; fixed in `e19bc3310`. Verified
  locally with emsdk 3.1.74 (the pre-fix file fails with exactly that error, the fixed tree builds);
  Android compiles the same GLES code path; CI run 41291607424 (`8771bfdcb`) confirms it: all build
  and test jobs pass on every platform.
  Left over, not part of this task (need the job logs, so a GitHub token):
  - **Windows Packaging** (`Run Inno Setup`) failed. *Fixed (2026-11-02):* `ags.iss` installs
    `ikpFlac.dll`, which the editor never copied to its output: `AGSEditor.csproj` copied it only for
    `Platform` x86, but the editor builds as AnyCPU (upstream has the same condition). It was the only
    file of `ags.iss` missing from the `windows-editor-binaries` artifact.
  - **CodeQL default setup** (repo settings, not a workflow file) fails to autobuild C/C++. Set C/C++
    to build mode "none" in Settings > Code security, or disable it.
  Also done: the `Debug_XP`/`Release_XP` configurations and `WIN_XP_COMPAT` (which disabled D3D11 and
  the D3D shader compiler) are removed; the hand Makefiles and legacy macOS Xcode projects are at
  C++17 too. Left for phase 7: the standalone plugin projects (`Plugins/*/msvc`, `agspalrender/Makefile`)
  are still C++11.
  Follow-ups found:
  - The engine imports `D3DCOMPILER_47.dll` directly, so it won't start on a Windows 7 without update
    KB4019990 unless the game ships that DLL. This was already true before; D6 makes it worth deciding
    (ship the DLL, or load it lazily and fall back to the precompiled shaders).
  - `CMAKE_OSX_DEPLOYMENT_TARGET` is 10.9. C++17 language features work there, but the library types
    `std::optional::value()`, `std::variant`, `std::any` need macOS 10.13 and `std::filesystem` 10.15.
    Raise the target before code starts using them.
- [x] **0.5 Repository hygiene.** Add the stray local artefacts (`Engine/resource/fxc*.exe`, `fxc.zip`,
  `Editor/References/x64/`) to `.gitignore` or commit them deliberately. Add a `.gitattributes` with
  `* text=auto` and `eol=crlf` only for `*.sln`, `*.bat`, `*.cmd`, `*.vcxproj*` and `*.csproj`.
  *Done when:* `git status` is clean after a full Windows build.

## Phase 1: Safety net

Today: 35 C++ test files, but **only 3 for the engine** (~430 source files). The 22 C# test files
cover `AGS.Types` and a few editor utilities. The end-to-end coverage comes from the upstream
auto-test game that CI downloads from `latest`.

- [x] **1.1 Own the auto-test game.** CI downloads `ags-test-games/releases/latest` from upstream.
  After the fork this will start failing for reasons that have nothing to do with your code. Fork
  `ags-test-games`, pin a tag in `build.yml`, and add tests there for every Draconian-only feature
  (D3D11 driver, custom shaders, full ranges, dirty rects, ...).
  *Done when:* CI pins a fork-owned test-game version that covers fork-specific features.
  *Status (2026-11-01):* fork is `AlanDrake/ags-test-games`, branch `ags4-draconian`. `build.yml` now
  downloads `AUTO_TEST_REPO`/`AUTO_TEST_TAG` (`draconian-auto-test-1`) instead of upstream `latest`.
  The game must be built with the **Draconian editor** (fork-only script API), so the test repo's
  `build-ags4` job is disabled and releases are uploaded by hand until 1.7 exists. Headless build:
  `AGSEditor.exe /compile <absolute path>\Game.agf` (from Git Bash set `MSYS_NO_PATHCONV=1`, or it
  rewrites `/compile` into a path). Covered fork features: Opacity/Tint ranges (34 checks), the new
  DrawingSurface primitives (32), void-returns-void in the compiler (2); local run 2414/2414 pass with
  the Software driver. Not covered yet: fake gamma, D3D11 pixel-shader blenders, slider focus outside
  GUI bounds (all need rendering or input checks), PO translations (belongs in editor tests, 6.6).
  Done: release `draconian-auto-test-1` published; CI run 41286020888 passes the auto-test on all six
  platform jobs (Windows, MinGW x64/arm64, Ubuntu x64/arm64, macOS), hardware and Software drivers.
- [ ] **1.2 Contract guard tests** (one per row of the compatibility contract):
  - C1: load fixture game files and rooms written by a 3.99 editor, the earliest 4.0 editor you can
    get, and the current one, and assert on counts and names (fixtures go in `Common/test/data/`, as
    small as possible). Record each fixture's room-file version number; task 3.4 needs it.
  - C2: savegame **round trip**: load save → write save → byte-compare, plus loading saves produced by
    a 3.99 engine and the last released 4.0 engine.
  - C3: a test that dumps the exported API table (`--print-scripttoc` already exists) and compares it
    with a checked-in snapshot. Any change to the snapshot must be deliberate.
  - C4: a minimal test plugin that calls every `IAGSEngine` method.
  - C5: Editor: open → save a sample `Game.agf` and compare the XML. `Game.agf` has two readers
    (D7), so the same sample is also loaded by `Tools/data/agfreader` with no unknown-element
    warnings (7.1g).
  *Done when:* each contract has a test that fails if it's broken.
- [ ] **1.3 Engine unit tests for the riskiest subsystems**, in this order: script VM
  (`ScriptExecutor` with bytecode produced by `script2`), route finder, overlay/GUI layout
  arithmetic, savegame components, string formatting (`scsprintf` already started), sprite cache
  eviction. Where a subsystem reads globals, add a test fixture that sets them up. That fixture is
  the first step towards phase 5's context object.
  *Done when:* each listed subsystem has a test file. Engine test files ≥ 15.
- [ ] **1.4 Game corpus regression.** A script (`ci/corpus/run.py`) that, for each game you own the
  sources to: compiles all scripts with `script2` (before 3.3 removes the legacy compiler, run it
  once with both and confirm every game builds with `script2`); runs the engine with
  `--gfxdriver Software --no-message-box --log-stdout` through `--startr N` for every room;
  and fails on warnings or crashes. Run it locally before risky merges and in CI nightly if the game
  data can live in a private repo.
  *Done when:* one command runs the whole corpus and reports pass/fail per room.
- [ ] **1.5 Coverage, sanitizers and warnings.** Add a Linux CI job with `-fsanitize=address,undefined`
  running all unit tests and the auto-test game, and a coverage job (`llvm-cov`) that publishes
  per-directory numbers. Raise first-party warning levels to `/W4` and `-Wall -Wextra`, warnings as
  errors **per directory** as each directory gets clean.
  *Done when:* the sanitizer job is green, the coverage report is published, and `Common/` builds with
  warnings as errors.
- [x] **1.6 Mechanical-change verifier.** `Script/compare_objects.py`: build Release without debug
  info into two trees (before and after a commit), strip the objects and hash them. Report
  translation units that differ. Format-only and rename-only commits must come out identical except
  for files that use `__LINE__`/`__FILE__`. This makes phases 2 and 4 nearly risk-free.
  *Done when:* the script exists and is documented in STYLE.md, "Commits".
  *Status (2026-11-01):* done. Both revisions build in one worktree and build directory (`.compare/`,
  ignored by git), so paths and `__FILE__` match and the second build is incremental. Build type
  MinSizeRel: optimized, `NDEBUG`, no debug info, and no LTO (the `MATCHES Release` checks in
  CMakeLists turn LTO on only for Release); MSVC also gets `/Brepro`. First-party code uses no
  `__LINE__`/`__FILE__`/`__DATE__` directly and asserts compile away, so only GoogleTest files are
  line-sensitive; they're reported and tolerated. Checked on Windows/MSVC: clang-format on 4 files
  (1,600+ changed lines, incl. `character.cpp`) gives 588 identical objects plus the tolerated test
  file; a parameter rename gives 589 identical; changing `return 1` to `return 2` is reported as a
  code change (exit code 1). First run ~3.5 min, later runs seconds to a few minutes.
  Not yet tried on Linux/macOS (the script uses `-g0` there; GCC and Clang objects carry no timestamps).
- [ ] **1.7 Build the auto-test game in CI, from source.** Done as part of 7.1j: once `agsbuild`
  builds AGS 4 projects, a Linux job compiles `AlanDrake/ags-test-games` (pinned commit) with the tools
  built in the same run and hands the result to the platform jobs. That replaces the hand-made
  releases from 1.1, keeps the test game in step with this commit's compiler, and doesn't tie every
  platform job to the Windows editor build (the earlier plan: compile with the CI-built editor).
  Until then, keep publishing `draconian-auto-test-N` releases by hand.
  *Done when:* the platform jobs run a test game compiled by tools of the same CI run.

## Phase 2: Mechanical normalisation

The configs are already in the repo: `/.clang-format`, `/.clang-tidy`, `/.editorconfig`, plus
`DisableFormat` stubs in the third-party folders. Measured clang-format 19 churn is about 15% of
lines (BASELINE.md). Most of it is real fixes: K&R braces → Allman, `if (x) y;` split onto two
lines, tab-indented code.

- [x] **2.1 CI format gate.** A job that runs pinned `clang-format 19` via `git clang-format --diff`
  against the PR base and fails if anything changes. Changed lines only, from day one.
  *Done when:* a PR with a badly formatted line fails CI.
- [x] **2.2 One-time reformat, module by module.** Commits like `Common: [format] whole module`,
  then Compiler, Tools, Engine (split by subfolder), Plugins (first-party only), Editor/AGS.Native.
  Verify each with 1.6. Add every hash to `.git-blame-ignore-revs` and run
  `git config blame.ignoreRevsFile .git-blame-ignore-revs`.
  *Done when:* `clang-format --dry-run` reports nothing on first-party code, and CI switches to
  checking whole files.
  *Status (2026-11-01):* done. Formatted Common, Compiler, Tools, Engine/ac, the rest of Engine and the
  first-party Plugins (~58,000 changed lines in 7 commits, hashes in `.git-blame-ignore-revs`); every
  commit checked with `Script/compare_objects.py`: all non-test objects identical. clang-format needed
  two passes to become idempotent (trailing-comment alignment). `.github/workflows/format.yml` now
  checks all tracked C/C++ files with clang-format 19.1.7. Not formatted by design (`DisableFormat`):
  third-party code (FreeType's own `.clang-format` replaced by the stub), `Editor/AGS.Native` and
  `Editor/Native` (C++/CLI, going away in 6.7), `PSP/` (removed in 3.1), `OSX/` (Objective-C). Only
  Windows/MSVC objects are compared; code for other platforms is formatted but not compiled by that
  check (formatting does not change tokens). Each developer should run
  `git config blame.ignoreRevsFile .git-blame-ignore-revs` once.
- [x] **2.3 Mechanical modernisation** (each item is its own commit, using the clang-tidy `--fix`
  of the named check): `NULL` → `nullptr` (276), `modernize-use-override`,
  `modernize-deprecated-headers` (`<math.h>` → `<cmath>`), `modernize-use-using`,
  `modernize-redundant-void-arg`.
  *Done when:* those checks are clean, so they move from "changed lines" to "whole tree" in CI.
  *Status (2026-11-01):* done, with exceptions; four commits, each checked with
  `Script/compare_objects.py` (all non-test objects identical). `Script/tidy_fix.py` runs a check over
  all TUs in parallel and applies the merged fixes. Results:
  - `modernize-use-nullptr`: 157 places. `NULL` still appears 147 times: comments, `sdl2_stubs.c`
    (C code), and Android/iOS/Linux/macOS-only code that the MSVC compile database doesn't cover.
    One `BROWSEINFO bi = {0}` stays, since `{nullptr}` compiles differently.
  - `modernize-use-override`: 91 places.
  - `modernize-deprecated-headers`: 122 includes in 89 files. **Seven files left on `<math.h>` /
    `<stdlib.h>`** (`Common/gui/guiinv.cpp`, `Compiler/script2/cc_compiledscript.cpp`,
    `Engine/ac/character.cpp`, `Engine/main/update.cpp`, three plugins): with `<cmath>`/`<cstdlib>` the
    compiler picks different `abs`-family overloads, which can change behaviour. Review by hand.
  - `modernize-redundant-void-arg`: 32 places. `Engine/main/engine.h` and `Engine/ac/sys_events.*`
    left alone (they changed `game.cpp`'s object; cause not investigated).
  - `modernize-use-using`: **skipped.** The fixer drops parameter names from function-type typedefs
    and conflicts on nested ones, which broke the build. Purely cosmetic; do by hand if wanted.
  - Moving these checks to "whole tree" in CI is task 2.4 (needs the Linux compile database).
- [ ] **2.4 clang-tidy gate.** Generate `compile_commands.json` in the Linux CI job and run
  `clang-tidy-diff.py` on the PR diff with the root `.clang-tidy`. This enforces naming and
  function-size rules on new code without touching the old code.
  *Done when:* a PR that introduces a `chaa`-style name or a 300-line function gets flagged.
- [x] **2.5 Header guards → `#pragma once`** (372 headers use `__AGS_...` guards, which are reserved
  identifiers) and **remove `using namespace` from headers** (54). The latter isn't purely mechanical:
  every `.cpp` that relied on it needs its own `using` or qualification. Do it one header at a time.
  *Done when:* both metrics are 0.
  *Status (2026-11-01):* done. (1) 380 headers converted to `#pragma once` (`52cba7906`); ten
  headers without a real guard left alone, since a guardless header may be meant for repeated
  inclusion. (2) All 42 file/namespace-scope `using namespace` lines removed from first-party headers
  (`5e6da0633`): headers now qualify names outside `namespace AGS`, and ~50 .cpp files got their own
  using-directives after the includes. Verified with the CMake/MSVC build, all MSBuild solutions
  (incl. `AGS.Native`), the Emscripten build, 828 unit tests and `Script/compare_objects.py`.
  Android/iOS/Linux/macOS-only sources were checked by reading; CI is the real test for those.
  The metric still shows 12: one function-local `using` in `characterinfo.h` (harmless) and 11 in
  `Editor/AGS.Native` (C++/CLI, removed in 6.7).
- [ ] **2.6 Self-contained headers, then sorted includes.** Add a CMake target that compiles each
  first-party header on its own (`header_check_<name>.cpp` that includes only that header). Fix
  failures with includes or forward declarations, then set `SortIncludes: CaseSensitive` with
  `IncludeCategories` matching STYLE.md and reformat once more.
  *Done when:* the header-check target builds and include order is enforced. This also helps
  compile times: `character.cpp` includes 57 headers and `game.cpp` 74.
- [x] **2.7 Line endings.** Renormalise with the `.gitattributes` from 0.5 (`git add --renormalize .`)
  in one commit, listed in `.git-blame-ignore-revs`.
  *Status (2026-11-01):* done: 457 files (mostly C# editor sources), content identical ignoring CR.

## Phase 3: Dead code removal

Remove first and restructure after: deleting code first makes everything later smaller. Each
removal is a `[delete]` commit, verified by the full test suite and the corpus run (1.4).

- [ ] **3.1 Dead platforms** (D2, D6): the `PSP/` folder, `Engine/Makefile.psp`, every plugin
  `Makefile.psp` and `psp.c`, `Engine/util/library_psp.h`, the 8 `AGS_PLATFORM_OS_PSP` branches, and
  any pre-Windows 7 code paths and toolsets. iOS and FreeBSD stay.
  *Done when:* `AGS_PLATFORM_OS_PSP refs` = 0 and the folders are gone.
- [ ] **3.2 Duplicate build systems** (D5): delete `Engine/Makefile*`, `Compiler/Makefile`, plugin
  `Makefile`/`Makefile-objs`, and `.github/workflows/makefile-build.yml`. Make sure the CMake build
  covers what `debian/` packaging needs. Move the native `Solutions/*.vcxproj` to CMake-generated VS
  solutions (CMake's VS generator, or CMake presets) so a source file is listed in exactly one place.
  The C# and C++/CLI editor projects stay in MSBuild.
  *Done when:* adding a `.cpp` file to the engine means editing one file.
- [ ] **3.3 Legacy compiler** (D3; run 1.4 once first): remove `Compiler/script/` (≈6.5k lines, including the
  1,477-line `__cc_compile_file`), `Compiler/test/` tests that only target it,
  `Solutions/Compiler.Lib`, `ccCompileText` in `AGS.Native/ScriptCompiler.cpp`, the
  `ExtendedCompiler` setting and editor UI, and fold `test2/` into `test/`.
  Then remove **`Editor/AGS.CScript.Compiler`**. Its compiler parts (tokenizer, code generator,
  `ScriptCompiler.cs`, most of `Entities/`) were never finished and nothing uses them. Its
  **preprocessor is used** by the editor's build today (`AGSEditor.cs`, `CompilerFactory.CreatePreprocessor`),
  and it has drifted from the C++ one in `Compiler/preproc` (it knows `#module`, the C++ one doesn't).
  So the order is: port its C#-only features to the C++ preprocessor (7.1c), switch the editor's
  build to the tools (6.7d), move the few types the editor uses (`CompilerMessage`, `FastString` for
  autocomplete, `ScriptAnnotations`) into the editor, move `PreprocessorTests.cs` cases to C++
  tests, then delete the project.
  *Done when:* one compiler, one preprocessor (C++), one test folder, and no `AGS.CScript.Compiler`.
- [ ] **3.4 Legacy data-version paths** (D1: 3.99+ only). This is the largest single deletion in the
  plan. Work in this order, one commit per step:
  1. **Game files.** Set `kGameVersion_LowSupported = kGameVersion_399` in
     `Common/ac/game_version.h`, then delete every branch that can now only run for older data: the
     `Upgrade*()` functions in `Common/game/main_game_file.cpp`, comparisons against
     `kGameVersion_3xx` constants older than 399 (62 `legacy version checks` in BASELINE.md, in 22 `.cpp` files across
     `Common/gui`, `Engine/ac`, `Engine/game` and `Engine/main`), and the
     `loaded_game_file_version` checks they guard. Then remove the obsolete enum values themselves.
  2. **Room files: careful.** Room versions are not monotonic. `kRoomVersion_399 = 40` sorts *below*
     the 3.6.x versions (`kRoomVersion_363_06 = 36306`), so raising `kRoomVersion_LowSupport` to
     `kRoomVersion_399` would make 3.6.x rooms look newer than 3.99 ones. Use the fixture versions
     recorded in 1.2 to see which values 3.99/4.0 editors actually wrote. Then replace the numeric
     minimum with an explicit check for those versions, and only after that delete the `ReadExt_*`
     and upgrade code for older rooms.
  3. **Savegames.** Raise `kSvgVersion_LowestSupported` (currently `kSvgVersion_Components`, a 3.x
     version) to the oldest version a 3.99 engine writes, and delete the branches below it.
  4. **Script API.** Delete `*_Old` variants (e.g. `Character_GetHasExplicitTint_Old`) and
     compatibility registrations that exist only for pre-3.99 scripts. Check the C3 snapshot after
     each deletion: anything a 3.99+ game can still call must stay.
  5. Deprecated savegame-only fields (`gscript_timer`, `replay_hotkey_unused`, ...) go **at the next
     savegame format bump only**, since C2 still applies to 3.99+ saves.
  *Done when:* `legacy version checks` = 0, the C1/C2 fixture tests pass, and loading a 3.6 game
  fails cleanly with a "game too old" error instead of crashing.
- [ ] **3.5 Unused functions.** `refactor_metrics.py --list-dead 200` lists 154 *candidates*. Many are
  false positives: plugin-API vtable entries, functions called from Java/Obj-C on mobile, or functions
  only referenced by name strings. Confirm each with a Linux build using
  `-ffunction-sections -fdata-sections -Wl,--gc-sections,--print-gc-sections` and `-Wunused-function`,
  then delete in batches per directory.
  *Done when:* the candidate list contains only documented intentional exceptions.
- [ ] **3.6 Comment debris.** Delete commented-out code (83 lines, mostly in `Compiler/test2`).
  Triage the 735 `TODO` / 332 `FIXME` / 80 `CHECKME` markers: either resolve, reclassify as
  `FIXME` / `TODO` per STYLE.md, or delete if stale. Do this per directory alongside phase 4.
  *Done when:* no `CHECKME` / `HACK` remain in a cleaned directory, and every `FIXME` says what
  case is wrong.
- [ ] **3.7 Obsolete API stubs.** `Engine/plugin/plugin_stubs.cpp` (223-line `RegisterPluginStubs`)
  keeps games that reference long-dead plugins (snow/rain, etc.) running. Keep the stubs your games
  need and drop the rest. `Common/util/wgt2allg.*` (the DOS WGT graphics shim, 6 includes) should be
  folded into `BitmapHelper` and deleted.

## Phase 4: Names and comments

This phase is done **file by file**, starting with the files that change most often (high churn
gives the best return). Each file gets a `[rename]` commit verified by 1.6, then optionally a
`[comments]` commit.

- [ ] **4.1 Parameter and local names.** Drive the watchlist in `refactor_metrics.py` to zero
  (4,855 occurrences today): `chaa` 652, `eres` 445, `chi` 368, `objj` 335, `sds` 226, `tqs` 224,
  `guio` 220, `butt` 187, `chin` 126, `char1` 98, `chap` 88, `scam` 82, `texx` 68... Use one name per
  concept across the codebase (`ch` for `CharacterInfo*`, `obj` for `RoomObject*`, `gui` for
  `GUIMain*`, `btn` for `GUIButton*`, `ds` → `dst` for destination bitmaps, `err` for error results).
  Use IDE rename or clang-based renaming, never search-and-replace.
  *Done when:* the cryptic-names total is 0, except for the globals handled in 5.1.
- [ ] **4.2 Function names.** Engine free functions are a mix of `snake_case` (949),
  `PascalCase` (1,094) and `Type_Method` (3,119, the script API). Rename the snake_case ones to
  PascalCase as their files are restructured in phase 5; don't do it earlier, since many of them
  will be deleted or moved into classes.
- [ ] **4.3 Field names of legacy structs.** `GamePlayState` (`play`), `GameSetupStruct` (`game`),
  `CharacterInfo`, `RoomStatus`, etc. use DOS-era lower-case names (`usedmode`,
  `disabled_user_interface`, `messagetime`). Rename to the STYLE.md form per struct. The script ABI
  maps by offset (C3), so this is safe. Keep each renamed field's original script-facing name in a
  trailing comment if it differs.
- [ ] **4.4 File names.** Rename files to `snake_case` matching their main class
  (`gamesetupstruct.cpp` → `game_setup_struct.cpp`, or whatever the type becomes in phase 5). Use
  `[move]` commits (rename only, no content change, so git tracks history) and do them together with
  the directory moves in 5.3 so files move only once.
- [ ] **4.5 Comments.** For each file touched in 4.1: remove history notes and author tags, fix
  comments that describe old behaviour (watch for references to DOS, palettes in 32-bit paths,
  removed functions and old version numbers), and add a header comment to every public function.
  Each top-level directory gets a short `README.md`: what it's for, its main entry points, and what
  must not depend on it.

## Phase 5: Structure

This is where the maze gets straightened out. Each task here is a series of small PRs, not one
big one.

- [ ] **5.1 Collect the globals.** 704 `extern` declarations in `Engine/`, 596 of them written inline
  in `.cpp` files. The most-shared globals are `game` (67 files), `thisroom` (31), `spriteset` (31),
  `gfxDriver` (28), `displayed_room` (20), `views` (18), `croom` (18), `playerchar` (17),
  `palette` (15), `platform` (13), `scsystem` (10) and `objs` (10). Steps:
  1. Every global is declared in exactly one header owned by its module. `extern` in `.cpp` files
     → 0.
  2. Group them into a few context structs (`GameData` for static loaded data, `GameState` for
     runtime state, `RoomContext` for the current room, `EngineServices` for the gfx driver, audio,
     platform and sprite cache), each with a single accessor.
  3. New and refactored functions take the context they need as a parameter. The phase 1 fixtures
     construct these contexts directly, which is what makes engine code unit-testable.
  *Done when:* `extern decls in Engine .cpp files` = 0 and the top 12 globals are reached only
  through the context accessors.
- [ ] **5.2 Separate script bindings from logic.** There are 1,598 hand-written `Sc_*` wrappers,
  built from 147 `API_*` macros in `script_api.h`, and they're interleaved with game logic (for
  example `character.cpp`, 5,092 lines, whose last ~1,400 lines are bindings plus a 228-line
  `RegisterCharacterAPI`). Steps:
  1. Move each file's `Sc_*` block and `Register*API` into `Engine/script_api/<type>_api.cpp`
     (a `[move]` commit).
  2. Replace the macros with a C++17 template binder that deduces arguments from the
     implementation's signature, e.g.
     `api.Method<&Character_AddInventory>("Character::AddInventory^2")`. Generate the
     parameter-count check from the signature. Guarded by the C3 snapshot test (1.2).
  *Done when:* no `Sc_` function remains, and `script_api.h` only has the binder.
- [ ] **5.3 Reorganise `Engine/ac/`.** "ac" (the original program name, *Adventure Creator*) is a
  64k-line catch-all holding 145 files. Proposed target layout (adjust as you go; the
  principle is one folder per domain, with logic separate from script bindings):

  | New folder | Moves from `Engine/ac/` |
  |---|---|
  | `Engine/world/` | character*, object, roomobject, roomstatus, room, hotspot, region, walkablearea, walkbehind, inventoryitem, properties, movelist |
  | `Engine/navigation/` | route_finder*, path_helper, pathfinder_script |
  | `Engine/dialog/` | dialog, dialogoptionsrendering, parser |
  | `Engine/presentation/` | display, speech, overlay, screenoverlay, draw*, screen, viewframe, lipsync, dynamicsprite, drawingsurface |
  | `Engine/ui/` | gui, guicontrol, button, label, listbox, slider, textbox, invwindow, guiinv, mouse |
  | `Engine/input/` | sys_events, joystick, touch |
  | `Engine/script/dynobj/` | `ac/dynobj/*` (managed script objects, next to the VM that owns them), scriptcontainers |
  | `Engine/audio/` | audiochannel, audioclip, global_audio (next to existing `Engine/media/audio`) |
  | `Engine/script_api/` | all `Sc_*`/`Register*API` (5.2), `global_api.cpp`, `utils_script`, `video_script`, `viewport_script`, `shader_script` |
  | `Engine/runtime/` | game, gamestate, event, timer, system, translation, file, datetime, math, string |

  The 14 `global_*.cpp` files are the old flat script API (`Display()`, `SetGlobalInt()` and friends).
  Fold whatever is still used into its domain folder and delete the rest. `Common/ac/` gets the
  same treatment (`Common/game/`, `Common/assets/`).
  *Done when:* `Engine/ac/` no longer exists.
- [ ] **5.4 Split the longest functions** (top of the BASELINE.md list), each with tests first:
  - `display_speech` (489 lines, nesting 8): split into text layout, portrait/bubble placement,
    lip-sync setup, and the blocking wait loop.
  - `load_new_room` (408): split into loading data, resetting state, and running room scripts and
    events.
  - `ScriptExecutor::Run` (1,161): an interpreter loop, so the long switch is expected. Move each
    opcode family into an inline handler, and benchmark before and after; the corpus run (1.4) is
    the reference.
  - `GamePlayState::ReadFromSavegame`/`WriteForSavegame` (303/215), `RoomObject`, `CharacterExtras`
    and `ScreenOverlay` serialisers: generate them from one field list (a serialisation helper or an
    X-macro) so reading and writing can't drift apart. Guarded by the C2 round-trip test.
  - `engine_init_game_settings` (265), `DialogOptions::Begin/Run`. (`BuildWithTaskManager` in
    `agsbuild`, 535 lines, is split as part of 7.1i.)
  *Done when:* functions over 200 lines: 19 → 3 at most (with the interpreter loop as a documented
  exception), and nesting ≥ 6: 10 → 0.
- [ ] **5.5 Confine Allegro.** The trimmed Allegro 4 fork (`libsrc/allegro`, ≈15k lines of C) is
  only needed as a pixel-buffer and blitter backend. Make `Common/gfx/allegrobitmap.*` and
  `Common/gfx/blender.*` the only files that include `allegro.h` (38 includes today), then decide
  whether to replace the backend with a small `Bitmap` implementation of your own. The tests from
  1.3 (`bitmapdata_test`, `paletteop_test`) and software-renderer screenshots guard this.
  *Done when:* `allegro.h includes` ≤ 3.
- [ ] **5.6 Error handling.** Replace the 179 `quit("...")` calls in subsystems with `HError` returns propagated to
  the main loop (STYLE.md). Start with file and asset loading, where a bad game file should produce a
  diagnostic instead of an abort.
- [ ] **5.7 Common C leftovers.** Replace fixed `char[N]` buffers (123) and
  `sprintf`/`strcpy`/`strcat` (63) with `String`, and raw `new[]`/`delete[]` (76) and
  `malloc`/`free` (51) with containers or smart pointers, except at C API boundaries (plugin API,
  SDL, OpenAL), which must be commented.

## Phase 6: Editor

The editor is ~107k lines of C# on .NET Framework 4.7 with C# 6, plus the C++/CLI bridge
`AGS.Native` (`agsnative.cpp` is 3,668 lines).

Per D4, the WinForms editor goes into **maintenance mode** and Avalonia is the planned UI. So the
editor work is about preparing a UI-independent core that a future Avalonia front end can use.
WinForms panes get bug fixes but no polish. Two things block Avalonia today:

- The project model (`AGS.Types`) and editor logic target .NET Framework 4.7, and Avalonia needs
  modern .NET.
- `AGS.Native` is C++/CLI, which only runs on Windows and is tied to .NET Framework.

Tasks 6.0, 6.1, 6.3 and 6.7 remove those two blockers. The remaining tasks improve quality along the way.

- [ ] **6.0 Make `AGS.Types` UI-free.** It's meant to be the project model, but today it depends on
  the WinForms UI: 17 files use `System.Windows.Forms`, 121 `[Editor(typeof(...))]` /
  `[TypeConverter(typeof(...))]` property-grid attributes reference WinForms editor classes, and the
  model stores `System.Drawing` `Font` (55 references) and `Bitmap` (15) objects. Those two types
  are Windows-only on .NET 6+. `Color`, `Point`, `Size` and `Rectangle` are fine, since they're
  cross-platform in .NET 8. Steps:
  1. Switch property-grid attributes to their string constructors
     (`[Editor("AGS.Editor.SpriteSelectUIEditor, AGS.Editor", "System.Drawing.Design.UITypeEditor")]`),
     which removes the compile-time dependency, and move the editor classes into the WinForms assembly.
  2. Replace `Font`/`Bitmap` fields with plain data (font file + size + style, sprite number, image
     path), and create the `System.Drawing` objects in the UI layer.
  3. Move anything else that needs WinForms out of `AGS.Types`.
  The model must load and save the same `Game.agf` as before (C5 round-trip test, 1.2).
  *Done when:* `AGS.Types` has no reference to `System.Windows.Forms`, `System.Drawing.Common` or
  `System.Drawing.Design`, and its tests pass on Linux.
- [ ] **6.1 SDK-style projects.** Convert the `.csproj` files to SDK style. Use `PackageReference`
  instead of `packages.config`. The WinForms application keeps targeting `net48` (still supported on
  Windows 7 SP1, D6), with `LangVersion` 7.3, the highest C# version officially supported on .NET
  Framework. Libraries that hold no UI code (`AGS.Types` and the new core from 6.3; not
  `AGS.CScript.Compiler`, which 3.3 deletes) **multi-target `netstandard2.0` and `net8.0`**, so both
  the WinForms editor and a future Avalonia editor can reference them.
  *Done when:* `dotnet build` builds the C# projects, the existing tests pass, and `AGS.Types` builds
  for `net8.0` (needs 6.0 first).
- [ ] **6.2 Format and analyzers.** Run `dotnet format` once per project (`[format]` commits,
  `.git-blame-ignore-revs`), then raise the `.editorconfig` severities from `suggestion` to
  `warning`, and enable `EnforceCodeStyleInBuild`.
- [ ] **6.3 Pull logic out of the big UI classes.** `RoomsComponent.cs` (3,113 lines),
  `ScintillaWrapper.cs` (3,040), `GUIController.cs` (2,120), `SpriteSelector.cs` (2,045) and
  `AGSEditor.cs` (1,920) mix UI with project logic. Move the non-UI parts into a new
  `AGS.Editor.Core` assembly (`netstandard2.0` + `net8.0`, no `System.Windows.Forms` or
  `System.Drawing` reference), which the Avalonia editor will build on, and test them there. Start
  with project load/save and upgrade (`UpgradeGameManager`), because those have no UI and are what an
  Avalonia editor needs first. The build itself does **not** move here: per D7 the editor runs the
  C++ tools (6.7d), so `Tasks.cs` and `BuildTargets/` shrink to "save, run `agsbuild`, show results".
  *Done when:* loading, saving and upgrading a project works from a headless test with no WinForms
  loaded.
- [ ] **6.4 Dependency injection over `Factory`.** 965 references to the static `Factory` service
  locator. Introduce constructor parameters for new and refactored classes and keep `Factory` as the
  composition root only.
- [ ] **6.5 Exceptions.** Triage the 122 catch-all handlers: narrow the exception type, log, or
  rethrow. An editor that silently swallows I/O errors can corrupt projects.
- [ ] **6.6 Editor tests.** Extend `AGS.Editor.Tests`: project load/save round trip (C5) and the
  upgrade path for old projects (`UpgradeGameManager`). `DataFileWriter` (2,551 lines) needs no new
  unit tests: until 6.7d deletes it, the build-parity harness (7.1a) compares its output with the
  tools' output file by file.
- [ ] **6.7 Replace the C++/CLI bridge with C# plus a small C library.** `AGS.Native` (~6,800 lines
  of C++/CLI) is **not** converted to C# wholesale. Two of its parts must stay native: font and GUI
  rendering (previews must be pixel-identical to the engine), and room/sprite file I/O for editing
  (shared with the engine and the tools; the fork changes these formats, so a C# copy would need
  every change made twice). Compiling leaves the editor process entirely: scripts compile in `agscc`
  as part of the build (D7). Everything else either goes away or becomes C#. Current contents:

  | Part | ~Lines | Fate |
  |---|---:|---|
  | Glue between C# and C++ types: bitmap, GUI, room, interaction and custom-property conversion, the `NativeMethods` managed API, `NativeRoom` | 2,400 | Disappears; what's still needed is rewritten in C# against the C library |
  | Import of old compiled games: `ImportOldGameFile`, `import_compiled_game_dta`, `load_dta_file_into_thisgame`, `init_game_after_import`, SCI font import | 650 | Deleted (D1) |
  | Editor-only utilities: template extraction, ini read/write, exe icon writing (`agsicons.cpp`), native constants | 550 | Ported to C# |
  | Room and sprite file I/O wrappers (over ~4k lines in `Common/`) | 1,100 | Stays native, behind the C library |
  | Font and GUI rendering wrappers (over ~8k lines in `Common/font`, `Common/gui`) | ~500 | Stays native, behind the C library |
  | View loop and sprite drawing (`doDrawViewLoop`, `drawSprite*`) | ~150 | **Moves to the UI.** `doDrawViewLoop` redraws the whole loop into one surface on every paint, with no caching, which also keeps the view editor from redrawing one frame. Frames are plain sprites: the UI paints them frame by frame from cached sprite bitmaps and does scaling and flipping itself. |
  | Script compiler glue (`ScriptCompiler.cpp`) | 300 | Goes away: the build compiles with `agscc` (D7). Add an in-process entry point to the C library only if an in-editor check turns out to need one. |

  Steps, each a separate PR, with the C++/CLI bridge kept working until the last one:
  - [ ] **6.7a Delete the old-game import** and the editor menu entries that use it.
  - [ ] **6.7b Port the editor-only utilities to C#.** Template files are AGS multi-file libraries, so
    the C# reader needs tests against templates written by the C++ code. Icon writing uses the Win32
    `BeginUpdateResource`/`UpdateResource` API through P/Invoke. It only matters for Windows builds of
    the game and can stay Windows-only.
  - [ ] **6.7c Define the C library (`ags_editor_native`).** About 20–30 `extern "C"` functions in a
    new CMake target that links `Common` and `script2`, with opaque handles and caller-owned buffers.
    Rough groups: open/save room (backgrounds, masks, properties as plain structs), sprite file
    open/read/write/replace/delete, get a sprite **as an RGBA buffer** (the UI scales, flips and
    composes it), render GUI and font text **into an RGBA buffer** (instead of today's
    `drawGUIAt(HDC ...)` and `System::Drawing::Bitmap^` arguments, which tie it to Windows), plus a
    last-error string. A header comment documents ownership for every function. Unit-test it from C++
    in `Editor/native/test/`.
  - [ ] **6.7d The editor builds through the tools** (D7; needs 7.1f). The Build command saves the
    project, runs `agsbuild` (or individual tools where it needs finer control), shows its progress,
    and lists errors and warnings from the tools' machine-readable output (7.1h) with jump-to-line.
    Cancelling the build stops `agsbuild`. Ship the tools with the editor (installer, 7.x packaging).
    Define what editor plugins that hook build events see: the events still fire around the
    `agsbuild` run, but no plugin can patch data in the middle of it. Then delete the C# build code:
    `DataFileWriter.cs`, `Utils/DialogScriptConverter.cs`, the C# script-header generation, the binary
    writer in `TranslationsComponent.cs` (`trac` writes `.tra` files), and, with 3.3, the C#
    preprocessor.
    *Done when:* the editor's Build output is byte-identical to what it produced before the switch
    (7.1a harness, run once more against the last C# build), all tests pass, and the C# build code is
    gone.
  - [ ] **6.7e Switch the editor to the C library** through a C# P/Invoke wrapper in `AGS.Editor.Core`
    (works from both `net48` and `net8.0`), one feature area at a time (sprites, rooms, fonts, GUI
    previews), each covered by the 6.6 tests before and after. In the same step, view and sprite
    drawing move to the UI (see the table above): the view editor paints frame by frame from cached
    sprite bitmaps instead of calling `DrawViewLoop`.
  - [ ] **6.7f Delete `AGS.Native`** once nothing references it.

  *Done when:* there's no C++/CLI left in the repo, the editor's native dependency is one plain C
  library built by CMake for Windows, Linux and macOS, and `AGS.Editor.Core` loads it on Windows and
  Linux in CI.
- [ ] **6.8 Avalonia front end** (follows 6.3 and 6.7). Build it on a separate branch on top of
  `AGS.Editor.Core`, starting with read-only views (project tree, room viewer) before editors. Plan
  this as its own document when the core is ready, because by then the shape of the core will
  decide most of the design.

## Phase 7: Build tools, plugins and ports

- [ ] **7.1 Make the C++ tools the only build path** (D7). Can start right after phase 2.

  **Starting point**, measured 2026-11-01 by building `AlanDrake/ags-test-games` (`ags4-draconian`)
  with the fork editor and with `agsbuild`, using tools built from the current tree. The editor
  build works; `agsbuild` can't build an AGS 4 project. Each gap was found by patching the previous
  one in scratch only:

  | # | Gap | Since |
  |---|---|---|
  | 1 | `Tools/data/agfreader.cpp` accepts only the old project tag `3.0.3.2` plus a version index, so it rejects every project saved since the editor switched to version strings (`Unsupported Game.agf format: 4.0.0.34`, then `format index: 0`) | July 2025, `ca79f81c5` |
  | 2 | No AGS 4 room compiler: `agsbuild` expects `room<n>.crm` and `room<n>.asc` in the project root, but AGS 4 projects keep rooms as sources in `Rooms/<n>/` (`data.xml`, `background0.png`, mask PNGs, `room<n>.asc`). Only the editor turns them into `.crm` today, through `AGS.Native`. | AGS 4 room format |
  | 3 | `agscc` lists script API levels only up to `v363` (`Compiler/compiler.cpp`); every AGS 4 level (`v399` … `v400_33`, which the engine and editor have) fails with `invalid API version` | AGS 4 |
  | 4 | The C++ preprocessor doesn't know `#module`, which the script API header uses (`agsdefns.sh` line 14), so no script compiles. The C# preprocessor learned it in `48fb2ec66`. | Jan 2025 |
  | 5 | `agsbuild` hung after its tasks failed: idle, no child process, until killed | bug |

  `agf2dta` did write a `game28.dta`, but content parity can't be measured until scripts compile, so
  more gaps may show up in 7.1f. The steps, in order:

  - [x] **7.1a Build-parity harness** (`Script/compare_builds.py`). Copies a project twice, builds one
    copy with the editor (`AGSEditor.exe /compile <absolute path>\Game.agf`; from Git Bash with
    `MSYS_NO_PATHCONV=1`) and the other with `agsbuild`, unpacks both packages with `agspak -e`, and
    compares them file by file: `game28.dta`, rooms, sprite file and index, audio, translations, fonts,
    custom data. It prints a table (identical / different / missing on one side) and can run the
    auto-test on the `agsbuild` output. This is the measuring stick for the rest of 7.1, as
    `compare_objects.py` was for phase 2. It needs the editor, so it's Windows-only until 6.7d.
    *Done when:* one command prints the per-file table for the auto-test project.
    *Status (2026-11-01):* done. `python Script/compare_builds.py <project> --editor <AGSEditor.exe>
    [--engine <acwin.exe>]` collects the tools from `.tidy/build`, builds fresh copies both ways under
    `.compare/builds/`, prints the distinct `agsbuild` errors and the per-file table, and exits 0 only
    on full parity. First result on the auto-test project: the editor build gives 17 files; `agsbuild`
    stops at 40 `#module` errors (7.1c) and the missing room sources (7.1e).
  - [x] **7.1b Unblock the pipeline** (gaps 1, 3 and 5; small, separate commits):
    - `agfreader`: accept the version-string format, comparing versions rather than exact strings.
      **Project upgrades stay in the editor** (`UpgradeGameManager`): the tools read the current
      format, and older ones only where reading them needs no upgrade step (for example Transparency
      0–100 becoming Opacity 0–255 in 4.0.0.34 does need one). For anything older they say "open and
      save this project in the editor first". A C++ copy of the upgrade code would be one more thing
      to keep in sync.
    - `agscc`: add the AGS 4 API and compatibility levels, plus a test that fails when the engine's
      `kScriptAPI_*` list (`Common/ac/gamestructdefines.h`) has a level `agscc` doesn't, so they can't
      drift apart again.
    - `agsbuild`: when a task fails, cancel what depends on it, finish or stop the rest, report, and
      exit non-zero instead of hanging. Add a test that runs a build with a failing task.
    *Done when:* `agsbuild` on the auto-test project gets past project loading and API checks and
    always terminates, on success and on failure.
    *Status (2026-11-01):* done, with tests for each part:
    - `agfreader` accepts project formats from 4.0.0.34 up to the newest it knows (also 4.0.0.34),
      and tells the user to open and save older projects in the Editor first.
    - The script API levels are now one table, `kScriptAPINames` next to the enum in
      `Common/ac/gamestructdefines.h`, used by `agscc` and `agf2dta`. `agf2dta` had a third copy that
      also stopped at `v363`, so it wrote the wrong API level into AGS 4 games: a content-parity bug
      fixed early. `Common/test/scriptapi_test.cpp` fails if the enum gets a level the table lacks.
    - The hang wasn't in the task manager but in `PipedProcess` on Windows: a read could ask for more
      bytes than the pipe held (an unsigned underflow), and then blocked forever because `agsbuild` kept
      its own copy of the pipe's write end, so the pipe never reported end-of-file. Fixed by bounded
      reads, closing our write ends right after starting the process, non-inheritable read ends, and
      starting processes under one lock. `Tools/test/pipedprocess_test.cpp` runs six chatty processes
      in parallel; on the old code it aborts after 120 s. Left open: the POSIX branch reads stdout to
      end-of-file before it reads stderr, so a child that fills the stderr pipe could still deadlock it.
    Result: `agsbuild` loads the auto-test project and exits after 2 s, failing only on `#module` (7.1c)
    and the room sources (7.1e).
  - [x] **7.1c One preprocessor** (gap 4). Diff `Editor/AGS.CScript.Compiler/Preprocessor.cs`
    against `Compiler/preproc/preprocessor.cpp` (directives, predefined macros, the
    `"__NEWSCRIPTSTART_..."` markers), port everything the C++ side lacks, starting with `#module`, and
    move the cases from `Editor/AGS.Editor.Tests/ScriptCompiler/PreprocessorTests.cs` into C++ tests.
    *Done when:* every auto-test script compiles with `agscc`, and the former C# preprocessor tests pass
    against the C++ preprocessor (prerequisite for deleting the C# one in 3.3).
    *Status (2026-11-01):* done. `#module` was the only missing directive (7.1e later found that
    `agscc` also lacked three of the editor's predefined macros). As in the C# version, the
    first `#module <name>` turns the script's marker into `"__NEWSCRIPTSTART_<script>;<module>"`,
    which the new compiler's scanner already reads. All 21 C# preprocessor tests already had C++
    copies. `Compiler/test/preprocessor_test.cpp` (now 31 tests) used to build only with the legacy
    compiler; it now also runs in the default `compiler_test`, in CMake and MSBuild. Two more bugs
    surfaced once scripts got past the preprocessor:
    - `agscc` dropped the headers with the new compiler, so no script saw `agsdefns.sh`. It now
      joins the headers and the script into one text, as the editor does.
    - `agfreader` read `<Name>` for dialogs, inventory items, views and GUI controls, which AGS 4
      saves as `<ScriptName>`, so `_AutoGenerated.ash` left them out. A content-parity bug fixed
      early, with a test.
    Result: all 40 auto-test scripts compile with `agscc`; `agsbuild` now stops only on the room
    sources (7.1e).
  - [x] **7.1d Table-driven XML reading** (maintainers' design). C++ has no reflection, so emulate it
    with one table per data struct: each row names a member, its XML element and the function that
    reads the value, and a single generic function fills a struct from an XML element using the
    table:

    ```cpp
    const XmlField<GameSettingsData> kGameSettingsFields[] = {
        {"GameName", Read<&GameSettingsData::GameName>},  // reader chosen from the member's type
        {"GameFileName", Read<&GameSettingsData::GameFile>},
        {"GameFPS", Read<&GameSettingsData::GameFPS>},
    };
    const XmlField<CharacterData> kCharacterFields[] = {
        {"BlockingRectangle", Read<&CharacterData::Blocking, ReadRect>},  // custom reader where needed
        // ...
    };
    template <typename T, size_t N>
    HError ReadFields(const XMLElement *elem, T &obj, const XmlField<T> (&fields)[N], UnknownPolicy policy);
    ```

    How to build it:
    - C++17 `auto` template parameters (`Read<&T::Member>`) turn each row into a plain function pointer
      and pick the reader from the member's type: `int`, `bool` (`True`/`False`, as `AGS.Types` writes
      it), `float`, `String`, enums through a name table each, rectangles.
    - **Defaults live in the struct's member initializers** (`int Opacity = 255;`), and a missing
      element leaves the member alone. That gives one source of truth instead of today's default
      argument on every `ReadInt` call.
    - The tables fill tool-side mirror structs that follow the XML (as `agf2dta` already does with
      `DataUtil::*Data`). Meaning goes into separate conversion code: inverted flags, derived values,
      nested engine structs such as `RoomStruct::Edges`.
    - `ReadFields` walks the element's children and looks each one up in the table. **Unknown
      elements fall out for free**, which is most of the drift guard (7.1g).
    - Lists (`<Characters>` with `<Character>` items, editor folders) get one generic list reader
      that takes the item table.
    - What tables don't fit (order-dependent values, upgrades of old values) stays as code, after
      `ReadFields`.

    Pilot it on the room tool (7.1e, new code). Then move `agfreader` over one entity at a time
    (`Character`, `InventoryItem`, `Cursor`, ...); each move must leave `game28.dta` byte-identical
    (7.1a), so the migration is mechanical and checked.
    *Done when:* the room tool and all of `agfreader` read through tables, and no hand-written
    `ValueParser::Read*` call remains.
    *Status (2026-11-01):* the table layer and the pilot are done; moving `agfreader` over is left
    for 7.1f. `Tools/data/xml_fields.h` has `XmlField<T>`, three row readers (`Read<&T::M>` picks the
    parser from the member type, `ReadWith<&T::M, Parse>` for a custom parser,
    `ReadElement<&T::M, Fn>` for nested elements), and `ReadFields` / `ReadList`. A row with a null
    reader marks an element that is known but deliberately not read (obsolete properties). An unknown
    element is an error, as in the editor's own deserializer; `UnknownElements::Skip` is for tables
    that read part of an element on purpose (the cursors' interaction schema, read from `Game.agf`).
    The parsers accept what `AGS.Types` writes, the way it reads it back: bools ignore case, enums
    take the exact name or a number, floats are invariant. `CompareXmlVersions` moved there from
    `agfreader`. Tests: `Tools/test/xml_fields_test.cpp`.
    *Migration of `agfreader` (2026-11-02):* done. Every entity and list of `Game.agf` is read through
    a table: characters, items, cursors, views, fonts, audio clips and types, GUIs and their controls,
    dialogs, the game settings and runtime setup, custom property schemas, global variables, scripts,
    plugins, translations, sprites, parser words and lip sync, and the root `<Game>` element. Each
    step was checked by the harness staying at 17 of 17. What stays as code, after `ReadFields`:
    entity references (`ID`, `ScriptName`), the interaction events, the audio folders' inherited
    defaults, the palette and the sprites' attributes (attributes, not elements), and the dialog
    files' lookup. The readers take `UnknownElements::Report`, not `Fail`: they cannot return errors
    yet (`ReadAllData` is `void`), so unknown elements and bad values go to `XmlProblems` and the
    caller decides (7.1g).
  - [x] **7.1e AGS 4 room sources** (gap 2). Compile `Rooms/<n>/` to `.crm` in C++, following the same
    principle as `agf2dta`; room data is simpler than game data. Read `data.xml` through 7.1d tables,
    load the background and mask PNGs (`Common/gfx/image_png.cpp`), compile `room<n>.asc` with
    `agscc`, and write the room with `Common/game/room_file.cpp`, in `crmpak` or a new `crmbuild` tool
    that `agsbuild` calls. Most of the logic already exists in C++: `convert_room_to_native` and
    `convert_room_interactions_to_native` in `AGS.Native/agsnative.cpp` (~330 lines) map every room
    property onto `RoomStruct`, only reading from the managed `Room^` object instead of XML. Port that
    mapping to read from the 7.1d structs. That also takes it out of `AGS.Native` (6.7). The details
    that decide byte parity (mask colours to area numbers, background colour depth and compression)
    are in the same file.
    *Done when:* the rooms `agsbuild` produces are byte-identical to the editor's (7.1a).
    *Status (2026-11-01):* done: all three auto-test rooms are byte-identical to the editor's.
    - **Reading** (`Tools/data/room_xml.{h,cpp}`): mirror structs and tables for `Room`,
      `RoomObject`, `RoomHotspot`, `RoomWalkableArea`, `RoomWalkBehind` and `RoomRegion`. Room files
      are only upgraded when the editor saves the room, so unlike `Game.agf` the tool reads every
      room format from 3.99.99.01 on. The test project's rooms are 4.0.0.27. It applies the editor's
      load-time upgrades: `Name`/`Description` became `ScriptName`/`DisplayName`, `Transparency`
      0–100 became `Opacity`, region light and tint moved to 0–255 below 4.0.0.34, and old indexed
      interaction events are converted.
    - **Building** (`Tools/data/room_build.{h,cpp}`): a port of `convert_room_to_native` and
      `convert_room_interactions_to_native`, plus the editor's fixups. Properties are synced with the
      schema and the GameID is fixed up. A room from another game with a different mask resolution is
      an error ("open and save it in the editor"): resizing masks stays in the editor. Cursor
      interactions become a table indexed by cursor ID, built from the game's interaction schema.
      Text properties are unescaped and encoded as UTF-8 or Windows-1252. Backgrounds go through
      `spriteimport`'s port of the editor's bitmap conversion (`ConvertRoomBackgroundForGame`). Masks
      must be 8-bit, and out-of-range pixels are cleared with a warning. Walkable-area extents are
      computed as `calculate_walkable_areas` does. 256-colour games are refused for now: their
      backgrounds need the game palette.
    - **`crmbuild`** (new tool, CMake only, D5): `crmbuild Game.agf <n> -o room<n>.crm --script
      room<n>.o` builds the room. `--header` writes the room's declarations after the game's
      `_AutoGenerated.ash`, as the editor does. For rooms with `Rooms/<n>/data.xml`, `agsbuild` runs
      header, `agscc`, then room, and packs the room with its script inside. Older projects with
      `room<n>.crm` files keep the old path.
    - **Script parity** needed fixes outside the room tool, all of which also affect 7.1f:
      - `scriptgen` now ports the editor's `RegenerateScriptHeader` (order, `import readonly X *`).
        The old order and format changed the compiled scripts' type info and TOC.
      - `agscc` now defines `SCRIPT_EXT_*`, `NEW_KEYINPUT_API` (with `-foldkeyinput`) and `UNICODE`,
        as the editor does. Without `SCRIPT_EXT_MANAGEDBASE`, `agsdefns.sh` declared every managed
        type without its base. It adds the script TOC with `-g`, takes `--name` for the script's
        name (`Rooms\1\room1.asc`, as the Windows editor names it), and `--ascii` now reaches the
        new compiler.
      - `agsbuild` passes the editor's fixed values for the obsolete script settings (it used to
        read `-fforce...=0` from elements the editor no longer saves), and `--ascii` for ANSI
        games. It also names the API header `_BuiltInScriptHeader.ash`, the editor's name.
    Tests: `Tools/test/room_build_test.cpp`. Found on the way, not fixed:
    - `crmpak -e <room> script <file>` crashes (`ReadOnlyCompiledScript` passes a null room).
    - `ImageFile::LoadPNG` copies only `width*height` bytes of an RGB(A) image when no channel swap
      is needed.
    - Possible `game28.dta` gaps for 7.1f: `agf2dta` indexes interaction events by the XML `Index`
      instead of the cursor schema, doesn't unescape or re-encode text properties, and writes
      `"4.00.00.34 "` (trailing space) as "compiled with", where the editor writes `"4.00.00.34"`.
      (7.1f fixed the last two.)
  - [x] **7.1f Content parity.** Work through the 7.1a table until every file matches: game data
    (`agf2dta`, including fork features such as Opacity 0–255), dialog scripts (`agf2dlgasc` against
    `DialogScriptConverter.cs`), script headers (`scriptgen`), sprites (`spriteimport`), the audio
    cache, translations (`trac`), custom data and fonts. If a difference is harmless (ordering, for
    example), make both sides agree anyway, so parity stays a plain byte comparison.
    *Done when:* every file is identical and the auto-test passes on `agsbuild` output (2414 of 2414,
    as with the editor).
    *Status (2026-11-02):* done for the auto-test project. All 17 files `agsbuild` produces are
    byte-identical to the editor's, and the auto-test passes on them (2414 of 2414). What it took:
    - **Layout:** `agf2dta --script-dir` embeds the compiled scripts in `game28.dta`, as the editor
      does (global, dialog, then the modules in compile order). `agsbuild` now runs it after every
      compile and no longer packs `.o` files or `ScriptModules.lst`. Every game script is compiled
      against all module headers: the editor's compile tasks share one header list, which keeps
      growing. Fonts come from the `Fonts` folder (`<FontFiles>` in `Game.agf`), packed without the
      folder.
    - **AGS 4 project format** that `agfreader` didn't know:
      - dialogs live in `Dialogs/<name>.xml` and `.dasc` (`AGFReader::GetDialogData`,
        `ReadDialogScript`);
      - characters and items use `DisplayName`, and their own `ScriptModule` (default
        `GlobalScript.asc`) sets the interactions' module;
      - frames have `Flip` (four values), offsets and event names (now read through a 7.1d table);
      - items have a hotspot alignment;
      - cursors have `CreateEvent` and `StandardRole`;
      - colours are 32-bit hex. `StringToInt`'s `strtol` saturated them to `0x7FFFFFFF` on Windows;
        `ParseXmlValue` now reads them as the editor's `Convert.ToInt32(s, 16)`.
    - **The nine AGS 4 extension blocks** were missing from `data_file_writer`: `ext_ags399` and
      `v400_gameopts`, `customprops`, `fontfiles`, `guictrlgfx`, `eventtables`, `viewevents`,
      `charopts2` and `charinv`. All are ported from `DataFileWriter.cs`. The event tables follow the
      editor's reflection order. The GUI controls' legacy event tables are now empty, as the editor
      writes them.
    - **Values written differently:**
      - inventory item 0 was added twice (the editor's list already has it);
      - views are by ID with empty slots;
      - the palette is 8-bit, not 6-bit;
      - text properties are unescaped;
      - the deprecated score sound and maximum score fields are 0;
      - the dot colour is written only with a marker style;
      - "compiled with" lost its trailing space;
      - `dev_error_text` was missing from the game info.
    - **Dialog scripts:** the converter dropped every dialog's last line.
    - **`acsetup.cfg`:** written with `IniUtil::Merge` into a fresh file, as the editor does. The
      `show_fps` condition was inverted.
    - **Harness:** `compare_builds.py` now clears its unpack folders, which had shown files left by
      earlier runs.
    Left open: one test project measures parity. Translations (`trac`), 8-bit games and old-format
    indexed interactions aren't exercised by it, and `agf2dta` still takes interaction slots from the
    XML `Index` instead of the cursor schema (the same here). Another sample project would widen
    the check.
  - [x] **7.1g Drift guard.** Two halves:
    - **Unknown elements:** with the tables from 7.1d, `ReadFields` reports XML elements it doesn't
      recognise: a warning in normal use, an error in tests. A test loads a project saved by the
      current editor (the auto-test project) and expects none.
    - **Properties no sample project uses:** a C# test reflects over the `AGS.Types` classes the
      tools read, and writes the serialized property names per class to a checked-in list. A C++ test
      checks that every name appears either in a table or in an explicit "editor-only, not needed for
      building" list. This catches a new property even before any test project uses it, and turns
      "the tools ignore this on purpose" into a reviewed decision.
    *Done when:* adding a project property in `AGS.Types` without teaching the tools, or listing it
    as editor-only, makes a test fail.
    *Status (2026-11-02):* done.
    - **Unknown elements:** `UnknownElements::Report` records unknown elements and bad values in
      `XmlProblems` and goes on. `agf2dta` prints them as warnings, and as errors with `--strict`
      (`agsbuild` runs `agf2dta`, so it shows them too). The auto-test project loads with none, and
      `--strict` passes. `Fail` stays for the room tool, which returns errors.
    - **Properties no sample uses:** `AgfElementsTests` (C#, `Editor/AGS.Editor.Tests/Types`)
      serializes a default object of every class that the tools read and checks the child element
      names against `Tools/data/agf_elements.txt`; run it with `AGS_UPDATE_AGF_ELEMENTS=1` to
      regenerate the list. The C++ test `AgfElements` (`Tools/test/agfreader_test.cpp`) checks that every
      element on the list has a row in a table (`GetKnownElements()` maps class names to tables). A
      row without a reader means "the build does not need it" (editor-only), with the reason in a
      comment, so that decision is now reviewed. 37 classes are covered.
    - **What it found at once** (all fixed): characters' `Enabled` and `Visible` were never read, and
      the writer set both flags always; the sprites' `ImportColorDepth` (which sets `SPF_KEEPDEPTH`) was
      not read or written; global variables' `ArrayType` and `ArraySize` were ignored, and default
      values were not lower-cased as the editor does (`bool b = True;`).
    - **Left open:** `spriteimport` does not re-import a sprite as 8-bit for `ImportColorDepth` 8
      (only the flag is written, which is all a build from `acsprset.spr` needs). The classes outside
      `Game.agf` (rooms, translations) are not on the list: rooms are read by the room tool, which
      fails on unknown elements. CI should build with `--strict` (7.1j).
  - [ ] **7.1h Diagnostics for the editor.** Behind a flag, `agsbuild` and the tools it runs print
    errors, warnings and progress in a machine-readable form (one JSON object per line: severity,
    tool, file, line, message), and `agsbuild` stops cleanly when cancelled.
    *Done when:* the editor (6.7d) can list tool errors with jump-to-line and cancel a build.
  - [ ] **7.1i Split `BuildWithTaskManager`** (535 lines, `Tools/agsbuild/main.cpp`) into one function
    per build stage, while doing the steps above. It stays.
  - [ ] **7.1j CI.** A Linux job builds the auto-test project with `agsbuild` from the tools of the same
    run and hands it to the platform jobs (this is 1.7). A Windows job runs the parity harness against
    the editor on every PR until 6.7d deletes the C# build code.
    *Status (2026-11-02):* the Linux half is done and green in CI (run 41432710133). `agsbuild` takes
    `--strict` (passed on to `agf2dta`). `Script/agsbuild_autotest.sh <build-dir> <project> <work-dir>`
    collects the tools from a CMake build, builds the project in strict mode, and runs the engine on the
    result (checked with `tappy`); locally it gives 2414 of 2414. The `Ubuntu-x86_64` job in `build.yml`
    checks out the test game at `AUTO_TEST_TAG` and runs it after the downloaded auto-test. Still to do:
    handing the built game to the platform jobs in place of the downloaded one (1.7), once this job
    has been green for a while.
    *Status (2026-11-02, later):* the Windows half is done and green in CI (commit 2a43f3092). The `Windows`
    matrix job uploads its tools and `ags.exe` as `parity-tools-windows`; the new `Windows Build
    Parity` job downloads them with the editor, the x86 engine and SDL2.dll, puts `acwin.exe` next to
    `AGSEditor.exe`, and runs `Script/compare_builds.py` on the auto-test at `AUTO_TEST_TAG`
    (with `--engine`). Logs upload as `parity-logs`. Only 1.7 is left of 7.1j.
    Getting CI green on the way (2026-11-02):
    - `Engine/util/library_posix.h` still used `Path` unqualified after the using-directives left the
      headers; it broke every POSIX build (Linux, macOS, Android).
    - The MSBuild `Tools.Test` project did not compile `scriptgen.cpp`, which the 7.1g tests use.
    - Android: Gradle configures CMake eight times (four ABIs, two configs), and each configure
      downloaded the five dependencies from github.com; a few minutes of DNS failure on the runner
      failed the job. `Script/fetch_deps.sh` now downloads them once, with retries, into a cached
      directory, and the `Fetch*.cmake` scripts take them from `AGS_FETCH_MIRROR_DIR` (hashes still
      checked). `CMAKE_WARN_DEPRECATED=OFF` for Android hides the ~2200 warnings that NDK r25's
      toolchain file causes under CMake 3.31; the other platforms still show deprecation warnings.
    - Windows Packaging: see 1.1 (`ikpFlac.dll`).
    The failures on `ags4-fullranges` are Docker image names that must be lowercase (fixed on this
    branch) and the auto-test calling `Character.Transparency`, which that branch no longer has.
  - **Thin format tools:** `agscc`, `crmpak`, `spritepak`, `agspak` and `trac` stay as they are, and
    get tests for their command-line options (`Tools/test` exists).

  *Done when:* games build through the tools on Windows, Linux and macOS, byte-identical to the
  editor's former output; the editor builds through them (6.7d); and no C# build code remains.
- [ ] **7.2 Bundled plugins.** `agslua` hasn't changed since 2013 and bundles its own Lua and zlib.
  Remove it unless a game you ship uses it. `agspalrender` has a 745-line `Raycast_Render`: refactor
  only if you keep maintaining it. Check that the remaining plugins build with CMake only (3.2).
- [ ] **7.3 Ports** (D2). For each kept port (Android, macOS, Web, iOS, Linux packaging), make sure
  its build entry point is CMake-driven and documented in its README.
  - **iOS has no CI job today, and you have no Mac to test on**, so every engine refactor risks
    breaking it silently. Its Xcode project (`iOS/xcode/ags`) also keeps its own list of source files,
    which the file moves in 4.4 and 5.3 would break. Do this **before phase 5**: build iOS with CMake
    (`-DCMAKE_SYSTEM_NAME=iOS`, Xcode generator) and add a compile-only GitHub Actions job on a
    `macos-latest` runner that builds the simulator target without code signing. That job is the
    only automated iOS check, so treat a red iOS build as blocking.
  *Done when:* every kept port is built by CI from CMake, iOS included.

---

## Progress targets

Run `python Script/refactor_metrics.py` after each phase and update this table with the actual
values.

| Metric (BASELINE.md) | Baseline | After P2 | After P3 | After P4 | After P5 |
|---|---:|---:|---:|---:|---:|
| `NULL` | 276 | 0 | | | |
| old-style include guards | 372 | 0 | | | |
| using namespace in headers | 54 | 0 | | | |
| tab-indented lines | 5,511 | 0 | | | |
| AGS_PLATFORM_OS_PSP refs | 8 | | 0 | | |
| legacy version checks (pre-3.99) | 62 | | 0 | | |
| dead-code candidates | 154 | | < 20 | | |
| TODO + FIXME + CHECKME | 1,147 | | no CHECKME / HACK | | |
| commented-out code lines | 83 | | 0 | | |
| cryptic names (total) | 4,855 | | | ~0 (globals excluded) | 0 |
| extern decls in Engine .cpp files | 596 | | | | 0 |
| functions > 200 lines | 19 | | | | ≤ 3 |
| nesting depth ≥ 6 | 10 | | | | 0 |
| script API wrappers (Sc_*) | 1,598 | | | | 0 |
| allegro.h includes | 38 | | | | ≤ 3 |
| Engine C++ test files | 3 | ≥ 15 (P1) | | | |
| C# catch-all handlers | 122 | | | | (P6) < 20 |
| C++/CLI lines (`Editor/AGS.Native`) | ~6,800 | | | | (P6) 0 |
| Build parity: files identical, `agsbuild` vs editor (7.1a) | 17 of 17; auto-test 2414 of 2414 on `agsbuild` output (7.1f) | | | | (7.1) all |

## Definition of done for any refactor PR

- [ ] One kind of change per commit, labelled (`[format]`, `[rename]`, `[move]`, `[delete]`, or a
  normal behaviour title).
- [ ] Builds on all CI platforms, and unit tests plus the auto-test game pass.
- [ ] Mechanical commits verified with the object-code comparison (1.6), or the commit message says
  why they can't be.
- [ ] No compatibility contract snapshot changed, or the change is intentional and called out.
- [ ] No BASELINE metric got worse.
- [ ] Comments in the touched code match the code.
