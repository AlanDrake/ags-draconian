# AGS Draconian refactor plan

A staged plan for making a codebase that grew from a 1990s DOS program consistent, testable and
easy to navigate, **without breaking the games built on it**. Style: [STYLE.md](STYLE.md). Starting
numbers: [BASELINE.md](BASELINE.md) (`python Script/refactor_metrics.py`). How finished tasks went
(dated status, measurements, bugs found on the way, the full definitions): [LOG.md](LOG.md).

## How to use this plan

Phases 0–2 come first, in order (safety net and tooling); phases 3–7 then overlap **module by
module** (`Engine/ac/overlay.cpp` can be in phase 4 while the Editor is in phase 6). Phase 5 needs
3 and 4; 7.1 (the C++ build tools, D7) needs only phase 2, and 6.7d waits for it. Every task has a
**done when** line a machine can check. Each phase is useful on its own.

This fork is a testbed for **AI-assisted refactoring**: a task fits one session and is backed by an
automatic verifier (tests, the 1.2 contract snapshots, 1.6's object comparison, the metrics
script). A session reads `AGENTS.md`, "Next up" and the task's section (not this whole file),
STYLE.md. It ends a task by ticking the box, moving the task's definition and a dated status to
LOG.md, leaving a one-line result here, and updating "Next up". Commits and PRs name the task ID (`3.2`). Upstream's intent is in
`docs/context/` (on par with upstream PR #3185, AGS 4 only); the fork's plan against it is
`ALIGNMENT.md`; the evidence behind the editor split (6.0, 6.7c, U.6) is `EDITOR-SPLIT.md`.

## Ground rules

1. **Hard fork.** Upstream is not merged wholesale; fixes are cherry-picked by hand and recorded in
   `docs/UPSTREAM.md`. That is what makes the reformat, renames and moves affordable.
2. **The compatibility contract is sacred** (below). Anything not on it may change.
3. **Safety net before surgery.** No structural change to a subsystem until phase 1 has given it
   tests that would catch a regression.
4. **One kind of change per commit** (STYLE.md "Commits"): format, rename, move, delete or
   behaviour. Mechanical commits are verified by object-code comparison (1.6).
5. **Ratchets, not big bangs.** CI enforces style; BASELINE.md metrics may only go down, or the PR
   says why.
6. **Boy-scout rule, with limits.** A fix may rename a function's locals and fix its comments in a
   *separate* preceding commit; the surroundings are not refactored in the same PR.
7. **Check upstream before designing.** Before adding structure or an API, read
   `docs/context/decisions.md` and `roadmap.md`, and use upstream's names and shape unless there is
   a stated reason not to (ALIGNMENT.md). Work meant for upstream is upstream style and C++11 from
   the start.
8. **Lessons go into the docs, not the chat:** `AGENTS.md`, `STYLE.md` or `docs/context/`, in the
   same session.

## Compatibility contract

Frozen unless a task explicitly versions it; each row gets a guard test in phase 1.

| # | Interface | Where it lives | Why it's frozen |
|---|---|---|---|
| C1 | Game data: `.ags`, `.crm`, sprite file + index, `.tra`, `.wfn`, **from 3.99 onward** (D1) | `Common/game/*`, `Common/ac/spritefile.*`, `Common/data/*` | Games built with 3.99/4.x editors must keep loading |
| C2 | Savegames written by 3.99+ engines | `Engine/game/savegame*.cpp` | Players' saves |
| C3 | Script ABI: exported API names and signatures, `game.*`/`system.*` field offsets, bytecode | `Register*API()`, `Engine/ac/dynobj/scriptgame.cpp` and friends, `Engine/script/scriptexecutor.cpp`, `Editor/AGS.Editor/Resources/agsdefns.sh` | Compiled scripts link by name and offset |
| C4 | Plugin API: `IAGSEngine` vtable order, `AGS_Engine*` exports | `Engine/plugin/agsplugin.h` | Binary plugins |
| C5 | Editor project format (`Game.agf`, `.user`, room sources) | `Editor/AGS.Types` | Existing projects. *The editor plugin API left this row on 2026-11-04:* a new editor is a big step and the plugin surface (`EditorContentPanel`, `MenuCommand`, `IGUIController`, `IRoomController`, `IAGSEditor`) follows the better design; plugins recompile against the new `AGS.Types` (6.0 step 2, EDITOR-SPLIT.md §3) |
| C6 | Engine command line and `acsetup.cfg` keys | `Engine/main/main.cpp`, `Engine/main/config.cpp` | Launchers, store builds, players' configs |

C3 is defined by offsets and names, **not** C++ layout: `GamePlayState` fields are read through the
offset switch in `CCScriptGame::ReadInt32`, so renaming or reordering them is safe while that table
stays correct.

## Decisions

Recorded 2026-10-31.

| ID | Question | Decision | Consequences | Unblocks |
|---|---|---|---|---|
| D1 | Oldest game version loaded (C1) | **3.99 and later** (`kGameVersion_399`) | All 3.x upgrade paths can go; room versions need care (3.4) | 3.4 |
| D2 | Platforms | **Windows, Linux, macOS, Android, Web, iOS.** PSP goes; FreeBSD stays for now (shares the POSIX code) | iOS can't be tested locally (no Mac): compile-only CI job (7.3) | 3.1, 7.3 |
| D3 | Legacy script compiler | **Removed**; `script2` is the only compiler | 3.3 | 3.3 |
| D4 | Editor UI | **Avalonia is the main candidate**; WinForms goes into maintenance. *2026-11-03:* upstream plans no cross-platform editor but welcomes the core split (#1092, #3177); a-riddle-fan's `ags-for-mac` (3.6.x, .NET 10 + Avalonia, `AGS.Native` replaced by a CMake C library) is studied before 6.7c | Work goes into a UI-free core and replacing the C++/CLI bridge, which Avalonia can't use (phase 6) | 6.x |
| D5 | Native build system | **CMake only**; MSBuild stays for the C# projects | Makefiles and native `Solutions/*.vcxproj` go (3.2) | 3.2 |
| D6 | Minimum Windows | **Windows 7+** | No `v141_xp`; `AGS.Native` targets `0x0601`; no newer APIs without a runtime check | 0.4, 3.1 |
| D7 | How games are built | **One implementation, in C++: the tools in `Tools/`** (`agsbuild` running `agf2dta`, `agscc`, `crmbuild`, `po2tra`, `agspak`, ...). The editor saves and runs `agsbuild` or single tools. The C# build code (`DataFileWriter.cs`, `DialogScriptConverter.cs`, script-header generation, translation writer, C# preprocessor) is deleted at parity. *Revised 2026-11-01 after maintainer review* (replaced a C# build core argued from commit counts) | Builds need only a C++ toolchain (Linux i386, FreeBSD, CI without the editor); the data writer sits next to the engine's reader. Cost: `Game.agf` has a C# and a C++ reader, so a new property needs both; 7.1g makes a missed one fail tests | 1.7, 3.3, 6.3, 6.7d, 7.1 |

## Next up

Kept current as tasks land. Model and effort are recommendations for an AI session.

Done: phases 0, 1 and 2, 7.1a–m, o and p (7.1k and 7.1p: incremental room builds and script compiles, 2026-11-07; 7.1l with 6.7d), 6.7d (the editor builds through the tools, 2026-11-07), 3.1 (PSP, pre-Windows 7; 2026-11-04), 3.3's legacy compiler (the `AGS.CScript.Compiler` project remains), U.1 (merged, #3181), U.2 (#3182, closed
and re-cut by U.3 as #3186–#3190), the agent docs (0.6, upstream PR #3185), 1.2 (contract guard
tests, 2026-11-04), U.5 (the save fix found by 1.2, upstream PR #3191), the 6.0 study (2026-11-04,
EDITOR-SPLIT.md: 6.0 re-scoped, 6.7c decided, U.6 defined), 1.3 (engine unit tests, 2026-11-05,
merged), 3.2a/b (Makefiles, `make.yml` and `dh --buildsystem makefile` replaced by CMake;
2026-11-05, green in a full CI run), 2.4 (clang-tidy gate, 2026-11-05), 1.5 (warnings, sanitizers, coverage; 2026-11-06). The safety net
(1.2, 1.3) is in place. Ground rule 3 applies from here: no deletion (3.4) or restructuring (phase 5) before the safety net that
guards it (1.2, 1.3). Reordered 2026-11-03 after reading upstream's plans (ALIGNMENT.md): upstream
review takes weeks, so the offers (U.3) go out first and the fork-only divergence (3.2, 3.4, 5.3)
comes after they are closed.

| # | Task | Why now | Model / effort |
|---|---|---|---|
| 1 | **6.7d The editor builds through the tools** (phase 6), split into 13 steps on 2026-11-06 (the task's section). Done 2026-11-06: 6.7d-1 to 6.7d-4 (all ten variants have parity), 6.7d-5 (256-colour rooms; a private 8-bit game has parity in every variant, and its dialogs found three defects), 6.7d-6 (`--header`, `--data-only`, `_obj/`), 6.7d-7 (tools and `agsdefns.sh` next to the editor, installer, CI; probe-confirmed), 6.7d-8 (`BuildToolRunner`). Done 2026-11-07: 6.7d-9 (Build, Rebuild, `/compile` and the new-game wizard build through `agsbuild`; the reference editor's `Compiled/Data` on three projects and every variant, Cancel and progress in the busy dialog), 6.7d-10 (F5 and Ctrl+F5 through `agsbuild --data-only`, the engine started from the editor's folder; the auto-test 2414 of 2414 through both; the user's by-hand checks passed, 2026-11-07: breakpoints, step, call stack, watches, Ctrl+F5, the renamed game file; every Run rebuilt every room, the build before a Run of a private AGS 4 game took 8.6 s instead of 1.9 s), 7.1k (incremental room builds in `agsbuild`, 2026-11-07: that Run is 2.7 to 3.1 s, the rest is the 38 script compiles), 7.1p (incremental script compiles and a task manager that wakes on a finished task, 2026-11-07: `agsbuild --data-only` with nothing changed 0.34 s, the Run 1.5 to 2.1 s against the reference editor's 1.3 to 1.7 s; the rest is the editor's work before `agsbuild` starts, 1.0 to 1.1 s). 6.7d-11 (the other users of the build code, 2026-11-07: a room save writes the sources only, templates through `agspak` and `crmbuild`, "Compile translation" through `po2tra`; the auto-test template byte-identical to the reference editor's, build parity kept). 6.7d-12 (the deletions, 2026-11-07: the editor's build, the `.tra` writer, `DataFileWriter`, the room writer, the in-process compile, `DialogScriptConverter`, the C# preprocessor, about 5,900 lines; a test compares the generated headers with `agfexport`'s). 6.7d-13 (2026-11-07: the harness compares the editor's output with a plain `agsbuild` run without un-embedding, which is what CI's parity job runs; AGENTS.md). **6.7d is done.** The state before it is the tag `pre-6.7d`; the reference editor is built from it. The three open questions of 6.7d-12 were decided and done the same day (LOG.md "6.7d-12 follow-up": the compiler setting obsolete, `<SpeechVoxFiles>` gone from `Game.agf`, nothing to restore for plugins; and `RequiresRebuild`, unread since 6.7d-9, reaches `agsbuild --rebuild` again). **Next: 3.3**, what is left of it: delete `Editor/AGS.CScript.Compiler` (only `FastString` has a user outside the project, autocomplete; it moves into the editor) | The editor's build code is gone, so the dead C# compiler project has no reason left | Sonnet 5.5, `medium` |
| 2 | **U.8, open:** offer the 1.5b-2 fixes in `CreateGlobalVars` (`RuntimeScript::CheckFixupInCode`) and `JointRTTI::Join` upstream (the same code there); the `String(ptr, length)` change alters a Common contract, so it is a separate offer if at all. The 1.5b-4 compiler leak fixes (the same code in `script2/`) are offered only when the user confirms | Real defects, small | Sonnet 5.5, `medium` |
| 3 | **U.3, waiting:** answer the reviews of #3186–#3190 (open since 2026-11-03, none yet), #3185 and #3191 when they come; nothing to do until then. `agsbuild` diagnostics on the Common log system (#2530) wait for ericoporto's logging change | Review takes weeks; the tool work is only valuable upstream while #2316 is active | Sonnet 5.5, `medium` for the follow-up rounds |
| 4 | **U.6, waiting:** v2 force-pushed 2026-11-04 (3 commits, editors in `AGS.Controls`, delegates kept); answer further review. The fork port (6.0 step 1, re-run of the scripts) waits until the PR is merged or stable (6.0 step 1; the fork's `AGS.Types` differs from upstream's by Opacity and formatting, so expect conflicts in the attribute lines) | Upstream review takes weeks, so the offer goes out first | user, then Sonnet 5.5, `medium` for the fork port |
| 5 | **U.7, waiting:** PR #3193 open since 2026-11-05; answer the review when it comes | Review takes weeks | Sonnet 5.5, `medium` |
| 6 | **2.9 CI and pre-push time, landed 2026-11-07:** read the first push's runs (the plan job's push filter, which builds Windows alone, runs there for the first time; the ccache of every job starts cold on the main branch). Open: 2.9d x64-only CI (the user's decisions), the apt package cache, the ARM64 retry | The first push after a CI change is the one that finds what the probe could not | Sonnet 5.5, `medium` |

Open items found while doing finished tasks (details in LOG.md): 0.4's CodeQL setting, the
`D3DCOMPILER_47`/Windows 7 question and the macOS 10.9 deployment target; 1.4's unfixed `agfreader`
crash on a project without `<Settings>` and `StrUtil::Unescape` reading past the end; 1.2's
uninitialised `DataUtil::GameSettings`: 47 `bool`/`int` fields have no initializer, so a `Game.agf`
element that is absent (an older Editor's project) leaves garbage in `agsbuild` and
`ReadCustomDataDirectories` instead of the default from `AGS.Types/Settings.cs` (several are
`true`, so `{}` is not the fix); only `PixelPerfect` got its default, after CI failed on it, and
the worktree branch `ags4-agfreader-tests` (#3147) still has the test's wrong `EXPECT_FALSE`; 7.1f's
"left open" notes (one test project measures parity; 8-bit games and old indexed interactions
unexercised); 3.1's unchecked `GetDiskFreeSpaceExW` result in `AGSWin32::GetDiskFreeSpaceMB`; 1.5a's
4-bit pixel write in `Common/gfx/bitmapdata.cpp` (wrong mask and shift, untested).

Waiting: 3.2c (native `.vcxproj` to CMake) on 6.7, since the C++/CLI bridge links those libraries;
the rest of 3.3 (`AGS.CScript.Compiler`) on 6.7d; 5.3 until U.3 is closed, then with upstream's
draft directory names. The next big step after the safety net is 6.7d.

---

## Phase 0: Fork foundation

- [x] **0.1 Tag the fork point** `fork-point` at `4cd88beba`; the README says hard fork.
- [x] **0.2 Cherry-pick ledger** `docs/UPSTREAM.md`, process in CONTRIBUTING.md.
- [x] **0.3 Decisions D1–D6** recorded above.
- [x] **0.4 Toolchain floor:** C++17, CMake 3.16+, `v143`, Windows 7 defines (2026-11-01).
- [x] **0.5 Repository hygiene:** `.gitignore` for local artefacts, `.gitattributes`.
- [x] **0.6 Agent context docs:** `AGENTS.md` and `docs/context/` (≤3k tokens per file, every claim
  dated and sourced). `docs/context/` is kept on par with upstream PR #3185 but covers `ags4`
  only (AGS 3 removed 2026-11-04); fork-only knowledge goes into `AGENTS.md` and `ALIGNMENT.md`. *Refresh:* `roadmap.md` at each upstream milestone
  change, `decisions.md` when a maintainer settles something (`docs/context/README.md`).
- [x] **0.7 Status logs to LOG.md** (2026-11-03: 28.6k → 18.0k tokens; 2026-11-04, finished-task
  definitions and long statuses moved).

## Phase 1: Safety net

Today: 35 C++ test files, **only 3 for the engine** (~430 source files); 22 C# test files cover
`AGS.Types` and a few editor utilities; end-to-end coverage comes from the auto-test game.

- [x] **1.1 Own the auto-test game:** fork `ags-test-games` with tests for fork-only features
  (2026-11-01; since 1.7 built from source).
- [x] **1.2 Contract guard tests**, one per contract row:
  - C1: load fixture games and rooms written by a 3.99 editor, the earliest 4.0 editor available and
    the current one; assert on counts and names (small fixtures in `Common/test/data/`). Record each
    fixture's room-file version: 3.4 needs it.
  - C2: savegame **round trip** (load → write → byte-compare), plus loading saves from a 3.99 and the
    last released 4.0 engine.
  - C3: the exported API table compared with a checked-in snapshot.
  - C4: a minimal test plugin that calls every `IAGSEngine` method.
  - C5: the Editor opens and saves a sample `Game.agf` and compares the XML; `Tools/data/agfreader`
    reads the same sample with no unknown-element warnings (7.1g).

  *Done when:* each contract has a test that fails if it's broken. *Upstream:* C5 and the AGFReader
  tests are #3147, C2 is #2814's 5-step plan; written upstream-style first in `ags4-upstream`, then
  ported here.
  *Status (2026-11-04, details in LOG.md):* done. C1: `Common/test/game_compat_test.cpp` and the
  `compat_restore_*` ctest cases on the 4.0.0.27 and 4.0.0.30 releases' builds of the Sierra-style
  template (`Common/test/data/compat/`, `make_fixtures.py`; room files `kRoomVersion_400_26`); 3.99
  alphas are not covered, by the user's decision. C2: `savegame_test.cpp` plus the releases' own
  saves, restored by the fork's engine and checked by the game's script. C3: the export snapshot.
  C4: `Engine/test/plugin_api_test.cpp`, `agsplugin.h` declarations in order and the `AGS_*` entry
  points against `Engine/test/data/plugin_api.txt` (no plugin that calls every method). C5 as
  before. Found: a save bug (fixed) and the fork's missing 4.0 `Transparency` exports (restored as
  `[OBSOLETE]` exports; `ScriptExports.ReleaseExportsResolve` checks every 4.0.0.30 export). Left: sending
  `ags4-agfreader-tests` (#3147) and `ags4-save-roundtrip` (#2814) upstream after the
  `ags4-tools-*` PRs.
- [x] **1.3 Engine unit tests for the riskiest subsystems** (2026-11-05, branch `1.3-engine-tests`,
  15 commits, merged): 6 → 16 test files, 12 → 162 engine test cases, ctest 1031 of
  1031; seven engine bugs fixed on the way, each with a failing-then-passing test, all present
  upstream too (LOG.md).
- [x] **1.4 Game corpus regression:** `ci/corpus/run.py` builds a private game and runs every room
  (2026-11-02). Game paths come from arguments or a git-ignored file under `.compare/`, never the
  repo; the script works on a copy.
- [x] **1.5 Coverage, sanitizers and warnings** (2026-11-06)**.** A Linux CI job with `-fsanitize=address,undefined`
  running the unit tests and the auto-test game; an `llvm-cov` job publishing per-directory numbers;
  first-party warnings at `/W4` and `-Wall -Wextra`, as errors **per directory** as each gets clean.
  *Done when:* the sanitizer job is green, coverage is published, `Common/` builds with warnings as
  errors. Split 2026-11-05 (LOG.md):
  - [x] **1.5a Warnings** (2026-11-05): `Common/` is warning-free on MSVC `/W4`, Clang and GCC and marked
    `ags_warning_free(common)`; `-DAGS_WARNINGS_AS_ERRORS=ON` in `verify.yml` and `prepush.py`. Open: 87
    narrowing conversions (MSVC, switched off), the other directories.
  - [x] **1.5b Sanitizers** (2026-11-06, green on CI, on demand by the user's decision)**:** `analysis.yml` worked and reported 47 findings: 3 memory errors, 2
    UBSan places, 42 leak sites. 1.5b-1 done (2026-11-05): the job runs locally in a WSL 2 distro,
    `prepush.py --only sanitizers` (also `linux` in every run, `coverage` on demand). 1.5b-2 done
    (2026-11-05): the 3 memory errors are fixed, 46 rows remain; U.8
    (offer the fixes upstream) is open. 1.5b-4 done (2026-11-05): the 35 compiler leaks are fixed, 11
    rows remain. 1.5b-3 done (2026-11-05): the misaligned `memory.h` accesses and the zero-byte stream
    writes are fixed, 8 rows remain. 1.5b-5 done (2026-11-06): the engine leaks are fixed, the third-party
    reports are in `ci/sanitizers/*.supp`, `sanitizers` is green locally and in the
    on-demand CI run 42174071706 (2026-11-06).
  - [x] **1.5c Coverage** (2026-11-05, on demand): total 53.7%, `Common` 61.6%, `Engine` 47.8%, `Compiler`
    89.9%, `Tools` 68.2%.
- [x] **1.6 Mechanical-change verifier** `Script/compare_objects.py` (2026-11-01; STYLE.md "Commits").
- [x] **1.7 Auto-test game built in CI from source** by the tools of the same run (2026-11-02, with 7.1j).

## Phase 2: Mechanical normalisation

Configs: `/.clang-format`, `/.clang-tidy`, `/.editorconfig`, `DisableFormat` stubs in third-party
folders.

- [x] **2.1 CI format gate** (now whole files, `format.yml`).
- [x] **2.2 One-time reformat**, module by module, hashes in `.git-blame-ignore-revs` (2026-11-01).
- [x] **2.3 Mechanical modernisation:** `nullptr`, `override`, `<cmath>`-style headers, `using`,
  redundant `void` (2026-11-01).
- [x] **2.4 clang-tidy gate** (2026-11-05, confirmed on CI): `Script/tidy_diff.py` runs the root
  `.clang-tidy` on changed lines in `verify.yml` and as `prepush.py` stage `tidy`; it flags case styles,
  function size and new variables or parameters with a watchlist name (`chaa`).
- [x] **2.5 `#pragma once`, no `using namespace` in headers** (2026-11-01).
- [x] **2.6 Self-contained headers (`header_check`), sorted includes** (2026-11-02).
- [x] **2.7 Line endings renormalised** (2026-11-01).
- [x] **2.8 CI shape and the pre-push check** (2026-11-04, green on CI after two pushes: see LOG.md):
  workflows renamed (`ci.yml`, `verify.yml`, `make.yml`, `linux-docker.yml`), concurrency, `paths-ignore`,
  `verify.yml` (fast gate on every push) split from `ci.yml` (full build), clang+libc++ header job,
  `Script/prepush.py`.
- [ ] **2.9 CI and pre-push time** (added 2026-11-06). The tests take seconds (`ctest` 8 s, the
  auto-test game a minute); the builds around them take 20 minutes locally and 18 on CI, the same
  code compiled from nothing in every job (measurements in LOG.md).
  **2.9a** a compiler cache (ccache) in the CMake jobs of `verify.yml` and `ci.yml`; the Windows
  CMake job builds with Ninja, as `prepush.py` does. **2.9b** a light run builds x86 Release only in
  the MSBuild jobs (Debug and x64 wait for a full run) and, on a push, leaves Ubuntu x86_64 to
  `verify.yml`, which runs the auto-test game there. **2.9c** `prepush.py --auto` runs the stages
  that the changed files need.
  Open after these: **2.9d** x64 only on CI except what exists only as 32-bit (the user,
  2026-11-06), which needs decisions on the Windows engine the installer ships, the Linux i386
  packages and the editor; **2.9e** LTO stays in the verification builds (the user, 2026-11-07):
  with a warm cache the Windows build step is 15 s of objects and 255 s of `/GL` links, GCC's engine
  link alone 112 s (LOG.md), and turning it off in Verify, the light CMake jobs and `prepush.py`
  would have taken most of that away, at the price that an LTO-only error waits for a full run; a
  retry for the ARM64 auto-test abort; a cache for the apt packages (31 to 192 s, once 20 minutes);
  upstream's workflows, which fail on the port branches; the laptop for the WSL stages.
  *Done when:* CI confirms a push that changes a few files in under 10 minutes, and a session that
  changed only the editor runs one stage.
  *Status (2026-11-07):* 2.9a–c are on the main branch: the eight commits of the probe branch
  `probe-2.9`, fast-forwarded (the branch started at the branch head), green there in Verify, a
  light and a full run (LOG.md: the probe results). A push should be confirmed in about 12.5 minutes
  instead of 18 (estimated from the probe's jobs), Verify in 7 instead of 8 with the auto-test game
  added; the second half of the "done when" holds. The state before it is the tag `pre-2.9`.

## Phase 3: Dead code removal

Delete first, restructure after. Each removal is a `[delete]` commit verified by the test suite and
the corpus run (1.4).

- [x] **3.1 Dead platforms** (2026-11-04): PSP port, plugin PSP builds and branches,
  `AGS_PLATFORM_OS_PSP` (0 refs), pre-Windows 7 checks in the Editor, engine and installer; the
  script enum value and config name stay (LOG.md).
- [ ] **3.2 Duplicate build systems** (D5): delete `Engine/Makefile*`, `Compiler/Makefile`, plugin
  `Makefile`/`Makefile-objs` and `.github/workflows/make.yml`, after checking CMake covers `debian/`
  packaging. Native `Solutions/*.vcxproj` become CMake-generated VS solutions (generator or presets);
  C# and C++/CLI projects stay in MSBuild. Also (found 2026-11-02): the `Linux Tools` and `macOS
  Tools` jobs run the tool Makefiles through `find ... -execdir make`, which exits 0 whatever `make`
  returns, so six tools are silently missing from the archives (`agf2dlgasc`, `agfexport`,
  `agsbuild`: no `xml_fields.cpp`; `agf2dta`: no `libsrc/glm`; `crmpak`, `spriteimport`: list the
  removed `Common/game/interactions.cpp`). Build those jobs with CMake (`-DAGS_BUILD_TOOLS=1
  -DAGS_BUILD_ENGINE=0`, as the auto-test job does).
  *Done when:* adding an engine `.cpp` means editing one file.
  *Status (2026-11-05):* 3.2a (tools, `agscc`) and 3.2b (engine, plugins, `debian/`, `make.yml`) are
  done, pushed and green in a full CI run (three fixes for the Linux builder image on the way,
  LOG.md); what remains is 3.2c. The Debian and Linux tools jobs run only in a full run. Kept:
  `Plugins/agslua/Makefile*`, its only build file; the user decides after the refactor.
- [ ] **3.3 Legacy compiler** (D3). The legacy compiler is gone (2026-11-03, LOG.md).
  Left: **`Editor/AGS.CScript.Compiler`**. Its compiler parts were never finished and are unused;
  its preprocessor and the `PreprocessorTests.cs` cases went with the editor's build (6.7d-12,
  2026-11-07; 7.1c had ported the cases to the C++ tests). Only `FastString` has a user outside the
  project (autocomplete), plus an assembly-resolve entry in `Program.cs`. Order: move `FastString`
  into the editor, delete the project from the solutions, the installer's file list and CI.
  *Done when:* one compiler, one preprocessor (C++), one test folder, no `AGS.CScript.Compiler`.
- [ ] **3.4 Legacy data-version paths** (D1). The largest deletion; one commit per step:
  1. **Game files.** `kGameVersion_LowSupported = kGameVersion_399` (`Common/ac/game_version.h`),
     then delete what only runs for older data: the `Upgrade*()` functions in
     `Common/game/main_game_file.cpp`, comparisons with `kGameVersion_3xx` older than 399 (62
     `legacy version checks`, 22 `.cpp` files in `Common/gui`, `Engine/ac`, `Engine/game`,
     `Engine/main`) and the `loaded_game_file_version` checks they guard; then the enum values.
  2. **Room files: careful.** Room versions are not monotonic: `kRoomVersion_399 = 40` sorts *below*
     `kRoomVersion_363_06 = 36306`, so raising `kRoomVersion_LowSupport` to 399 would make 3.6.x
     rooms look newer. Use the versions recorded by 1.2's fixtures, replace the minimum with an
     explicit check for them, then delete the older `ReadExt_*` and upgrade code.
  3. **Savegames.** Raise `kSvgVersion_LowestSupported` (now `kSvgVersion_Components`, a 3.x version)
     to the oldest a 3.99 engine writes; delete the branches below.
  4. **Script API.** Delete `*_Old` variants (`Character_GetHasExplicitTint_Old`) and registrations
     only pre-3.99 scripts use, checking the C3 snapshot after each: what 3.99+ games call stays.
  5. Savegame-only dead fields (`gscript_timer`, `replay_hotkey_unused`, ...) go **only at the next
     savegame format bump** (C2).

  *Done when:* `legacy version checks` = 0, the C1/C2 fixture tests pass, and a 3.6 game fails with
  a clean "game too old" error.
- [ ] **3.5 Unused functions.** `refactor_metrics.py --list-dead 200` lists 154 *candidates*, many
  false positives (plugin vtable entries, functions called from Java/Obj-C, name-string references).
  Confirm with a Linux build using `-ffunction-sections -fdata-sections
  -Wl,--gc-sections,--print-gc-sections` and `-Wunused-function`; delete in batches per directory.
  *Done when:* the list holds only documented exceptions.
- [ ] **3.6 Comment debris.** Delete commented-out code (83 lines, mostly `Compiler/test`). Triage 735
  `TODO` / 332 `FIXME` / 80 `CHECKME`: resolve, reclassify per STYLE.md, or delete; per directory,
  with phase 4.
  *Done when:* no `CHECKME`/`HACK` in a cleaned directory and every `FIXME` names its wrong case.
- [ ] **3.7 Obsolete API stubs.** `Engine/plugin/plugin_stubs.cpp` (223-line `RegisterPluginStubs`)
  keeps games using dead plugins (snow/rain, ...) running: keep the needed stubs, drop the rest.
  Fold `Common/util/wgt2allg.*` (DOS WGT shim, 6 includes) into `BitmapHelper` and delete it.

## Phase 4: Names and comments

File by file, most-churned first. Each file gets a `[rename]` commit verified by 1.6, then
optionally a `[comments]` commit.

- [ ] **4.1 Parameter and local names.** The `refactor_metrics.py` watchlist to zero (4,855 today:
  `chaa` 652, `eres` 445, `chi` 368, `objj` 335, `sds` 226, `tqs` 224, `guio` 220, `butt` 187, `chin`
  126, `char1` 98, `chap` 88, `scam` 82, `texx` 68, ...). One name per concept: `ch`
  (`CharacterInfo*`), `obj` (`RoomObject*`), `gui` (`GUIMain*`), `btn` (`GUIButton*`), `dst`
  (destination bitmaps), `err` (error results). IDE or clang-based renaming, never search-and-replace.
  *Done when:* cryptic names = 0, except the globals of 5.1.
- [ ] **4.2 Function names.** Engine free functions: `snake_case` 949, `PascalCase` 1,094,
  `Type_Method` 3,119 (script API). Rename snake_case to PascalCase as phase 5 restructures their
  files, not before (many will be deleted or become methods).
- [ ] **4.3 Legacy struct fields** (`GamePlayState`, `GameSetupStruct`, `CharacterInfo`, `RoomStatus`:
  `usedmode`, `disabled_user_interface`, `messagetime`) to the STYLE.md form, per struct. Safe under
  C3 (offsets); keep the script-facing name in a trailing comment when it differs.
- [ ] **4.4 File names** to `snake_case` after their main class (`gamesetupstruct.cpp` →
  `game_setup_struct.cpp`), as `[move]` commits together with 5.3 so files move once.
- [ ] **4.5 Comments.** For each file of 4.1: remove history notes and author tags, fix comments
  describing old behaviour (DOS, palettes in 32-bit paths, removed functions, old versions), add a
  header comment to every public function. Each top-level directory gets a `README.md`: purpose,
  entry points, what must not depend on it.

## Phase 5: Structure

Each task is a series of small PRs.

- [ ] **5.1 Collect the globals.** 704 `extern` declarations in `Engine/`, 596 inline in `.cpp` files.
  Most shared: `game` (67 files), `thisroom` (31), `spriteset` (31), `gfxDriver` (28),
  `displayed_room` (20), `views` (18), `croom` (18), `playerchar` (17), `palette` (15), `platform`
  (13), `scsystem` (10), `objs` (10).
  1. Every global declared once, in its module's header; `extern` in `.cpp` → 0.
  2. Group them into context structs named as upstream's #2058: `GameData` (static data:
     `GameSetupStruct` plus the data lists), runtime `Game` (`GameData` + `GameState` + runtime
     object lists, e.g. `Character` = `CharacterInfo` + `CharacterExtras`), the current room, and
     `EngineServices` (gfx driver, audio, platform, sprite cache; no upstream name yet), each with
     one accessor.
  3. New and refactored functions take their context as a parameter; phase 1 fixtures construct the
     contexts, which makes engine code unit-testable.

  *Done when:* `extern decls in Engine .cpp files` = 0 and the top 12 globals are reached only
  through the accessors.
- [ ] **5.2 Script bindings apart from logic.** 1,598 hand-written `Sc_*` wrappers from 147 `API_*`
  macros (`script_api.h`), interleaved with logic (`character.cpp`, 5,092 lines: the last ~1,400 are
  bindings and a 228-line `RegisterCharacterAPI`).
  1. Move each file's `Sc_*` block and `Register*API` to `Engine/script_api/<type>_api.cpp` (`[move]`).
  2. Replace the macros with a C++17 template binder deducing arguments from the signature
     (`api.Method<&Character_AddInventory>("Character::AddInventory^2")`), parameter-count check
     generated; guarded by the C3 snapshot.

  *Done when:* no `Sc_` function remains and `script_api.h` holds only the binder.
- [ ] **5.3 Reorganise `Engine/ac/`** (145 files, 64k lines; "ac" = *Adventure Creator*). One folder
  per domain, logic apart from bindings; proposed:

  | New folder in `Engine/` | From `Engine/ac/` |
  |---|---|
  | `world/` | character*, object, roomobject, roomstatus, room, hotspot, region, walkablearea, walkbehind, inventoryitem, properties, movelist |
  | `navigation/` | route_finder*, path_helper, pathfinder_script |
  | `dialog/` | dialog, dialogoptionsrendering, parser |
  | `presentation/` | display, speech, overlay, screenoverlay, draw*, screen, viewframe, lipsync, dynamicsprite, drawingsurface |
  | `ui/` | gui, guicontrol, button, label, listbox, slider, textbox, invwindow, guiinv, mouse |
  | `input/` | sys_events, joystick, touch |
  | `script/dynobj/` | `ac/dynobj/*`, scriptcontainers |
  | `audio/` | audiochannel, audioclip, global_audio (next to `Engine/media/audio`) |
  | `script_api/` | all `Sc_*`/`Register*API` (5.2), `global_api.cpp`, `utils_script`, `video_script`, `viewport_script`, `shader_script` |
  | `runtime/` | game, gamestate, event, timer, system, translation, file, datetime, math, string |

  The 14 `global_*.cpp` files (the old flat API: `Display()`, `SetGlobalInt()`) fold into their
  domain or go. `Common/ac/` likewise (`Common/game/`, `Common/assets/`). *Upstream (2026-11-03):*
  the wiki draft "AGS 4 Draft: major engine refactor" proposes
  `system/main/script/data/game/gfx/audio/video/util`, undecided. File moves hurt `port.py` most:
  wait until U.3 is closed, then prefer upstream's names where they overlap (`game/`, `script/`,
  `gfx/`, `audio/`, `util/`); `navigation/`, `dialog/`, `ui/` stay fork-only.
  *Done when:* `Engine/ac/` no longer exists.
- [ ] **5.4 Split the longest functions** (BASELINE.md list), tests first:
  - `display_speech` (489 lines, nesting 8): text layout, portrait/bubble placement, lip-sync setup,
    blocking wait loop.
  - `load_new_room` (408): load data, reset state, run room scripts and events.
  - `ScriptExecutor::Run` (1,161): the long switch is expected; opcode families into inline
    handlers, benchmarked before and after against the corpus run.
  - `GamePlayState::ReadFromSavegame`/`WriteForSavegame` (303/215) and the `RoomObject`,
    `CharacterExtras`, `ScreenOverlay` serialisers: generated from one field list (helper or
    X-macro), guarded by the C2 round trip.
  - `engine_init_game_settings` (265), `DialogOptions::Begin/Run`.

  *Done when:* functions over 200 lines 19 → ≤ 3 (interpreter loop a documented exception);
  nesting ≥ 6: 10 → 0.
- [ ] **5.5 Confine Allegro** (`libsrc/allegro`, ≈15k lines of C, needed only as pixel buffer and
  blitter). `Common/gfx/allegrobitmap.*` and `Common/gfx/blender.*` become the only includers of
  `allegro.h` (38 today); then decide on an own `Bitmap` backend. Guarded by 1.3's
  `bitmapdata_test`, `paletteop_test` and software-renderer screenshots.
  *Done when:* `allegro.h includes` ≤ 3.
- [ ] **5.6 Error handling.** The 179 `quit("...")` calls in subsystems become `HError` returns
  propagated to the main loop, starting with file and asset loading.
- [ ] **5.7 C leftovers.** Fixed `char[N]` buffers (123) and `sprintf`/`strcpy`/`strcat` (63) → `String`;
  `new[]`/`delete[]` (76) and `malloc`/`free` (51) → containers or smart pointers, except at
  commented C API boundaries (plugin API, SDL, OpenAL).

## Phase 6: Editor

~107k lines of C# on .NET Framework 4.7 / C# 6, plus the C++/CLI bridge `AGS.Native`
(`agsnative.cpp` 3,668 lines). Per D4, WinForms gets bug fixes only; the work is a UI-free core for a
future Avalonia front end. Two blockers, removed by 6.0, 6.1, 6.3 and 6.7: `AGS.Types` and the
editor logic target .NET Framework (Avalonia needs modern .NET), and C++/CLI is Windows- and
Framework-only.

- [ ] **6.0 `AGS.Types` UI-free.** *Re-scoped 2026-11-04 by the 6.0 study (EDITOR-SPLIT.md):* 17 files
  use `System.Windows.Forms` (5 only through an unused `using`), 112 `[Editor(typeof(...))]` attributes
  name 10 `UITypeEditor` classes and 11 `[PropertyTab(typeof(PropertyTabEvents))]` a WinForms
  `PropertyTab`; the 91 `[TypeConverter]` attributes are portable `System.ComponentModel`. The model
  stores **no** `System.Drawing.Font` or `Bitmap` (the earlier counts were AGS's own `Font` class); the
  GDI types that remain (`Icon` 5, `Graphics` 6, `Bitmap` 6) sit in the plugin-facing interfaces and
  the templates. `Color`, `Point`, `Size`, `Rectangle` are fine.
  1. **Property grid out** (also U.6): attributes to their string constructors
     (`[Editor("AGS.Editor.ColorUIEditor, AGSEditor", "System.Drawing.Design.UITypeEditor, System.Drawing, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]`:
     the editor assembly is named `AGSEditor`, and the base type name must be fully qualified or the
     grid drops the editor silently; `[PropertyTab("AGS.Editor.PropertyTabEvents, AGSEditor", PropertyTabScope.Component)]`); the 10
     editor classes, `StringListUIEditorControl`, `PropertyTabEvents` + `event.bmp` and
     `Utilities.PaintColorBlock` move to `AGS.Editor`, where the 9 static GUI delegates
     (`GUIController.cs`, `FontsComponent.cs`) become direct calls; the 5 unused usings go. A test in
     `AGS.Editor.Tests` resolves every attribute string with `Type.GetType` (a wrong string makes the
     property grid silently drop the editor). No plugin API change. On the fork this is a re-run of
     the U.6 scripts (`.compare/helpers/u6/`), not a cherry-pick: 14 of the 68 files conflict because
     the fork's Editor files differ from upstream's in every line. Do it once the #1092 PR is stable.
     *Open design note, to evaluate when step 2 is done and before 6.8, not earlier:* the string
     attributes still let the model name its WinForms editors; semantic attributes plus a host-side
     `TypeDescriptionProvider` would remove that (EDITOR-SPLIT.md §5).
  2. **Plugin-facing surface** (`EditorContentPanel : UserControl`, `ContentDocument`, `MenuCommand`'s
     `Keys`, `IDockingContainer`, `IScriptEditorControl`, `IGUIController`, `IRoomController`,
     `IAGSEditor`, the templates' `Icon`), with ags-for-mac's replacements as the template: a plain
     panel class with `View`/`HasFocus`, an `AGS.Types.Keys` with WinForms values, `IImage` and
     `IDrawingContext` abstractions. Breaks the editor plugin API (plugins recompile against the new
     `AGS.Types`; source changes are the type names): *accepted 2026-11-04*, the plugin API is no longer
     part of C5; design the interfaces for the new editor, not for the old plugins.
  3. **.NET Core differences** (shared with 6.1): `Encoding.Default` in `Utilities.cs` and
     `Translation.cs` is UTF-8 on .NET Core (Windows-1252 via the code-pages provider), the XML
     encoding name casing, `Path.GetInvalidFileNameChars()` off Windows, `Environment.NewLine`; the C5
     round-trip test covers each.

  Same `Game.agf` as before (C5). *Done when:* no reference to `System.Windows.Forms`,
  `System.Drawing.Common` or `System.Drawing.Design`, tests pass on Linux.
- [ ] **6.1 SDK-style projects** with `PackageReference` instead of `packages.config`. The WinForms app stays `net48` (Windows 7
  SP1, D6), `LangVersion` 7.3; UI-free libraries (`AGS.Types`, 6.3's core) multi-target
  `netstandard2.0` and `net8.0`. The .NET Core differences 6.0 step 3 lists (`Encoding.Default`,
  encoding-name casing, invalid path characters, line breaks) are the known traps; ags-for-mac's
  compatibility rules (EDITOR-SPLIT.md §3) are the checklist.
  *Done when:* `dotnet build` builds them, tests pass, `AGS.Types` builds for `net8.0` (after 6.0).
- [ ] **6.2 Format and analyzers.** `dotnet format` once per project (`[format]`,
  `.git-blame-ignore-revs`), `.editorconfig` severities from `suggestion` to `warning`,
  `EnforceCodeStyleInBuild`.
- [ ] **6.3 Logic out of the big UI classes** (`RoomsComponent.cs` 3,113 lines, `ScintillaWrapper.cs`
  3,040, `GUIController.cs` 2,120, `SpriteSelector.cs` 2,045, `AGSEditor.cs` 1,920) into a new
  `AGS.Editor.Core` (`netstandard2.0` + `net8.0`, no WinForms or `System.Drawing`), tested there.
  Start with project load/save/upgrade (`UpgradeGameManager`). The build does **not** move there: the
  editor runs the tools (D7, 6.7d), so `Tasks.cs` and `BuildTargets/` shrink to "save, run
  `agsbuild`, show results".
  *Done when:* load, save and upgrade work from a headless test with no WinForms loaded, and the
  dependency direction is enforced, not assumed: `AGS.Types` and `AGS.Editor.Core` reference no UI
  assembly, `AGS.Controls` references no editor assembly (upstream's rule from the U.6 review:
  controls must not know `GUIController` or editor components), checked by a test over the project
  references.
- [ ] **6.4 Constructor injection over `Factory`** (965 references); `Factory` stays as the
  composition root only.
- [ ] **6.5 Exceptions.** Triage the 122 catch-all handlers: narrow, log or rethrow (silently
  swallowed I/O errors can corrupt projects).
- [ ] **6.6 Editor tests:** load/save round trip (C5) and old-project upgrade (`UpgradeGameManager`).
  `DataFileWriter` (2,551 lines) needs none: the 7.1a harness compares its output until 6.7d.
- [ ] **6.7 Replace the C++/CLI bridge with C# plus a small C library.** `AGS.Native` (~6,800 lines)
  is **not** converted wholesale. Font/GUI rendering (pixel-identical previews) and room/sprite file
  I/O (shared with engine and tools, which the fork changes) stay native; compiling leaves the
  editor (`agscc`, D7); the rest goes or becomes C#.

  | Part | ~Lines | Fate |
  |---|---:|---|
  | Glue between C# and C++ types (bitmap, GUI, room, interaction, custom-property conversion, `NativeMethods`, `NativeRoom`) | 2,400 | Disappears; what's needed is rewritten in C# against the C library |
  | Old compiled-game import (`ImportOldGameFile`, `import_compiled_game_dta`, `load_dta_file_into_thisgame`, `init_game_after_import`, SCI font import) | 650 | Deleted (D1) |
  | Editor-only utilities (template extraction, ini, exe icons `agsicons.cpp`, native constants) | 550 | Ported to C# |
  | Room and sprite file I/O wrappers (over ~4k lines in `Common/`) | 1,100 | Native, behind the C library |
  | Font and GUI rendering wrappers (over ~8k lines in `Common/font`, `Common/gui`) | ~500 | Native, behind the C library |
  | View loop and sprite drawing (`doDrawViewLoop`, `drawSprite*`) | ~150 | **To the UI:** it redraws the whole loop on every paint, uncached; the UI paints cached sprite bitmaps frame by frame and scales/flips itself |
  | Script compiler glue (`ScriptCompiler.cpp`) | 300 | Goes (`agscc`, D7); an in-process entry point only if an in-editor check needs one |

  Separate PRs, the bridge working until the last:
  - [ ] **6.7a Delete the old-game import** and its menu entries.
  - [ ] **6.7b Editor-only utilities to C#.** Templates are AGS multi-file libraries: test the C#
    reader against templates the C++ code wrote. Icons via `BeginUpdateResource`/`UpdateResource`
    P/Invoke, Windows-only.
  - [ ] **6.7c The C library `ags_editor_native`.** *Decided 2026-11-04 (EDITOR-SPLIT.md §4):* reuse
    `ags-for-mac`'s `libagsnative` **conventions** (UTF-8, `int` results with a thread-local last-error
    string, opaque handles with matching free, caller-owned buffers with stride, an API version, hidden
    symbols, one process-wide recursive mutex because `Common` is not thread-safe) and its **image,
    sprite-file, font and GUI entry points** with the `agsn_` names where the semantics match, so a later
    merge on `ags4` is mechanical; say so on #3177 when the branch is cut. Not reused: its 38 room
    functions (`ags4` rooms are XML + PNG; only the 3.99 upgrade reads `.crm`, so a read-only loader with
    fork-defined structs), its CMake-generated constants (6.7b ports them to C#), its templates (6.7b), its
    3.6 struct layouts (the fork's GUI model has Opacity and `ags4` fields). About 30–40 `extern "C"`
    functions in a CMake `SHARED` target linking `Common` and `Tools/data`: images (BMP/PCX/PNG with
    palettes), sprite file slot info/read **as RGBA** and a slot-by-slot writer with raw copy (the sprite
    set lives in C#, not in a native cache), `agsn_sprite_convert` from `sprite_utils` (ags-for-mac runs
    `spriteimport` as a subprocess instead), fonts (set, metrics, draw text, TTF size, SCI import), GUI
    preview from host-flattened descriptions **into RGBA** (not today's `drawGUIAt(HDC ...)` and
    `System::Drawing::Bitmap^`), `.crm` read for the upgrade. Ownership documented per function; C++
    tests in `Editor/native/test/`, plus a pixel comparison of the GUI and font output against the
    C++/CLI bridge before 6.7e switches (ags-for-mac's GUI test checks only sizes).
  - [x] **6.7d The editor builds through the tools** (done 2026-11-07, 13 steps below; D7; needs 7.1f). Build saves the project, runs
    `agsbuild` (or single tools), shows progress, lists errors from `--json` (7.1h) with
    jump-to-line, cancels by stopping `agsbuild`; tools ship with the editor. Plugin build events
    still fire around the run, but nothing patches data mid-build. Then delete `DataFileWriter.cs`,
    `Utils/DialogScriptConverter.cs`, the `.tra` writer in `TranslationsComponent.cs` (`po2tra` writes
    them), the in-process script and room compile and, with 3.3, the C# preprocessor.
    *Done when:* Build output is byte-identical to the last C# build (7.1a) on the two test projects
    and on every variant of 6.7d-1, tests pass, the C# build code is gone.

    *Split 2026-11-06* (LOG.md 6.7d: three inventories, a measured baseline). The tools match the
    editor on both test projects (59 of 59 and 114 of 114 files; `agsbuild` takes 3 s where the
    editor takes 12 s on the private game), but both projects are debug-mode games without voice,
    split packages or extra files: **9 of 10 variants of the auto-test project that use such things
    lose parity, and 256-colour games do not build at all.** So the tools' gaps close first (ground
    rule 3), then the editor switches, then the code goes. The reference for parity is the editor
    built from the tag `pre-6.7d`.
    - [x] **6.7d-1 Parity matrix** (2026-11-06). `compare_builds.py --variants all` builds ten
      variants of a project both ways (`--list-variants`); only `audiovox` has parity today.
    - [x] **6.7d-2 Tool gaps: game data and configuration** (2026-11-06). `agf2dta` left the room names
      out of a release-mode `game28.dta` (the editor always writes them, the engine always reads them:
      such a game did not start); `agfexport game-cfg` ignored an `acsetup.cfg` in the project folder
      (now `--base <file>`, passed by `agsbuild`), wrote `OpenGL` where the engine knows `OGL`, and
      rounded the mouse speed; an empty `GameFileName` gave `.ags` instead of the folder's name. The
      variants `release`, `opengl`, `customcfg`, `noname` have parity; a release build runs the
      auto-test (2414 of 2414).
    - [x] **6.7d-3 Tool gaps: what is packed** (2026-11-06, LOG.md 6.7d; all ten variants have parity
      now)**.** `preload.*`, `flic*.fl?` and `*.ogv` were not packed;
      custom data folders fail when nested or with sub-folders, and lack the editor's checks
      (restricted folders, `.`/`..`, duplicates; warnings); `SplitResources` is ignored (`agspak -p`);
      voice packs lose the sub-folder prefix the engine looks up, keep the folder's letter case, leave
      out FLAC, and fail on a folder without voice files; an `audio.vox` no clip needs stays in the
      output. Split packages are compared by their assets only: the editor names the parts `.1D3`,
      `.2D3` (a format-string bug, `.001` was meant) and fills them by another rule.
      *Done when:* the variants `packed`, `customdirs`, `split`, `speech` have parity.
    - [x] **6.7d-4 UTF-8 command lines on Windows** (2026-11-06, LOG.md 6.7d)**.** The tools took a narrow `main(argv)`: a project
      path or game name outside the ANSI code page does not arrive (`agsbuild` fails at "project file
      not found"; the harness cannot unpack either, as it uses `agspak`). Users' folders are often not
      ASCII, so this blocks the switch. One shared entry point for every tool, with a test.
      `ags4` has no ASCII mode any more (user, 2026-11-06): arguments, paths and game text are UTF-8,
      and the legacy-encoding paths (`agscc --ascii`, `$Encoding=ASCII` translations) get no parity work.
      *Done when:* the variant `unicode` has parity.
    - [x] **6.7d-5 256-colour rooms in `crmbuild`** (2026-11-06, LOG.md 6.7d)**.** It refused them.
      The background of a room saved as 8-bit takes the game-wide colours of the game palette, up to
      the length of the image's own palette, and keeps its palette in the room file, as the editor
      does. Checked on a private 8-bit game (ten rooms, two with several backgrounds), which was
      also the first project with real dialogs and found three defects outside 8-bit: the dialog
      converter did not escape quotes in speech and renumbered the lines (the editor keeps the
      dialog script's line numbers), `agf2dlgasc` never reported a conversion error, and an empty
      `<ScriptModule />` of a character or an item became `GlobalScript.asc`.
      *Done when:* the 8-bit game has parity in every variant.
    - [x] **6.7d-6 Script headers from outside the project, run layout, `_obj/`** (2026-11-06, LOG.md
      6.7d; the F5 command line is there)**.** `agsbuild --header
      <file>` for the headers of editor plugins (they exist only inside the plugin, so the editor
      writes them out; the C# build passes them between the generated and the module headers).
      A run layout for F5, `agsbuild --data-only`: only export, scripts, game data and rooms, a package
      with `game28.dta` and the scripts, the rooms left on disk for `--runfromide`; nothing else is
      packed, as today.
      `--temp-dir` defaults to `<project>/_obj` (this is 7.1k; `_temp` was never removed after a
      successful build either: `DeleteDirectory` removes empty folders only). Found by 6.7d-8, for the
      editor to rely on `--json`: unknown options are accepted silently (so `--header` would become a
      third positional argument today), bad arguments give no JSON, a task that succeeds turns its
      `Error:` lines into errors (`spriteimport`: a build can end in success with errors), a failed
      task without an error line gives an empty message, and the tools inherit every handle
      `agsbuild` may inherit (`bInheritHandles`).
      *Done when:* the engine runs the auto-test from the run layout (2414 of 2414), and a failed
      build leaves nothing outside `_obj/` and the output folder.
    - [x] **6.7d-7 Tools next to the editor** (2026-11-06, probe-confirmed; the installer itself is
      built only by a full run)**.** `crmbuild` had no MSBuild project (`Tools.sln` builds the
      others into the editor's output folder; done 2026-11-06); `agsdefns.sh` existed only as an embedded resource and
      must ship as a file (decision T2; the editor's build copies it next to `AGSEditor.exe` now, where
      `agsbuild` finds it without options, measured 2026-11-06); the installer's file list and CI's
      `windows-editor-binaries` had no tool (both changed 2026-11-06: the editor job takes the x86
      tools from `windows-msvc-tools`; not run yet). CI and installer changes need a probe branch.
    - [x] **6.7d-8 `BuildToolRunner`** (2026-11-06, LOG.md 6.7d) in the editor, without UI types (it moves to `AGS.Editor.Core`
      in 6.3): starts a tool, turns the `--json` lines into progress and `CompileMessages` (file and
      line as the editor names scripts, duplicate warnings dropped), cancels through stdin and kills
      after a timeout. NUnit tests against the real `agsbuild` and against canned lines.
    - [x] **6.7d-9 Build through `agsbuild`** (2026-11-07, LOG.md 6.7d)**:** Build EXE, Rebuild all, `/compile`, the new-game
      wizard. Order: repair object IDs, save, `PreCompileGame` (open dialogs flushed, `AudioCache`
      refreshed, lip sync compiled into `Speech/`: these prepare sources and stay), the editor's
      checks, `agsbuild` into `Compiled/Data`, then the other targets, which only copy from there.
      Progress and a Cancel button in the busy dialog.
      *Done when:* this editor's `Compiled/Data` equals the reference editor's on both projects and
      every variant; an error in a module, a room and a dialog script jumps to its line.
    - [x] **6.7d-10 Run through `agsbuild`** (2026-11-07, LOG.md 6.7d)**:** F5 and Ctrl+F5 use the run layout; breakpoints, call
      stack and watch variables checked by hand in a game (the user), the auto-test run by script.
      The editor's engine (`acwin.exe` next to it) runs on `_Debug/<name>.ags` with `_Debug/Rooms` as the first
      data folder; nothing is copied into `_Debug` any more. The auto-test passes 2414 of 2414 through both
      commands (the reference editor's Run: 2411); the engine gets byte-identical game data, scripts and rooms.
      **Open, for the user:** every Run rebuilds every room (the build before a Run of a private AGS 4
      game: 1.9 s → 8.6 s); incremental room builds in `agsbuild` would win it back.
    - [x] **6.7d-11 The other users of the build code** (2026-11-07, LOG.md 6.7d)**:** saving a room writes
      its sources only (no script compile, no `room<n>.crm`; the errors come from the build, also after
      speech numbering and text import); a game template is staged and packed by `agspak`, a room
      template's `.crm` built by `crmbuild`; "Compile translation" runs `po2tra`; the lip-sync writer has
      its own string helper; `Build/ToolProcess` runs a tool that does not write JSON. The auto-test
      game's template is byte-identical to the reference editor's; the build parity holds.
    - [x] **6.7d-12 Deletions, one commit each** (2026-11-07, LOG.md 6.7d; 11 steps, about 5,900 lines out, every
      step built and tested; the auto-test template byte-identical and build parity in every variant, the Run
      2414 of 2414)**:** `DataFileWriter.cs`; `DialogScriptConverter.cs`
      (`IsRealScriptLineInDialog` moves to its one caller); the `.tra` writer; the in-process compile
      (`AGSEditor.CompileScript*`, `IScriptCompiler`, `ScriptCompiler.cpp` in `AGS.Native`, the
      compiler setting's list, `Script.CompiledData`, `Game.ScriptsToCompile`,
      `ExtraCompilationStep`, `GetScriptModuleList`, `_GlobalVariables.asc`); the editor's build, unused
      since 6.7d-10 (`AGSEditor.CompileGame`, `CreateMiniEXEForDebugging`, `CreateCompiledFiles`,
      `BuildTargetDebug.Build`, `PreCompileGameEventArgs.BuildsWithTools` and the branches it guards,
      `ForceRebuildTime` and `Utilities.DoesFileNeedRecompile(DateTime, string)`, unused since 6.7d-11); the
      room writer in `AGS.Native` (`NativeRoom.SaveToFile`, without a caller since 6.7d-11); the C# preprocessor and its tests (`FastString` stays for autocomplete).
      **Kept, against the first plan:** `Tasks.RegenerateScriptHeader` and the `_GlobalVariables.ash`
      generator, which feed autocomplete, go-to-definition and the plugin API; a test compares them
      with `agfexport`'s output so the two cannot drift.
    - [x] **6.7d-13 Harness, CI, docs** (2026-11-07, LOG.md 6.7d)**.** `compare_builds.py` compares the editor's
      output with a plain `agsbuild` run byte for byte when the editor's package has the scripts as
      separate files (every editor since 6.7d-9); the un-embedding stays for the reference editor
      (`--tools-editor`). CI's parity job, which had failed since the 6.7d-9 push for that reason, runs
      unchanged. AGENTS.md says how the editor builds; `docs/context` is upstream knowledge and keeps
      `DataFileWriter.cs`, which upstream still has.

    Stays in the editor after 6.7d, as open items for 7.1: the pre-build checks
    (`RunPreCompilationChecks`, missing event handlers), so a command-line build does not get them;
    the lip-sync compiler (Pamela, Papagayo); `AudioCache` upkeep.
  - [ ] **6.7e Switch to the C library** via a C# P/Invoke wrapper in `AGS.Editor.Core` (`net48` and
    `net8.0`), one area at a time (sprites, rooms, fonts, GUI previews), each covered by 6.6 tests;
    view and sprite drawing move to the UI at the same time.
  - [ ] **6.7f Delete `AGS.Native`.**

  *Done when:* no C++/CLI left; the native dependency is one C library built by CMake for Windows,
  Linux and macOS, and `AGS.Editor.Core` loads it on Windows and Linux in CI.
- [ ] **6.8 Avalonia front end** (after 6.3 and 6.7), on a branch over `AGS.Editor.Core`, read-only
  views first (project tree, room viewer). Planned in its own document once the core exists.

## Phase 7: Build tools, plugins and ports

- [ ] **7.1 Make the C++ tools the only build path** (D7). The starting point (2026-11-01: five gaps,
  `agsbuild` couldn't build an AGS 4 project) is in LOG.md.
  - [x] **7.1a Build-parity harness** `Script/compare_builds.py`: editor vs `agsbuild`, file by file,
    optional auto-test (2026-11-01). Windows-only until 6.7d.
  - [x] **7.1b Unblock the pipeline:** version-string projects, AGS 4 API levels in `agscc` with a
    drift test, `agsbuild` cancels on failure (2026-11-01). Project upgrades stay in the editor: the
    tools say "open and save this project in the editor first".
  - [x] **7.1c One preprocessor:** the C++ one has `#module` and the C# tests (2026-11-01).
  - [x] **7.1d Table-driven XML reading** (`Tools/data/xml_fields.h`, 2026-11-02). Defaults live in
    member initializers; a missing element leaves the member alone.
  - [x] **7.1e AGS 4 room sources:** `crmbuild` compiles `Rooms/<n>/`, byte-identical (2026-11-01).
  - [x] **7.1f Content parity:** every file identical, auto-test 2414 of 2414 on `agsbuild` output
    (2026-11-02).
  - [x] **7.1g Drift guard:** unknown elements reported; `AGS.Types` property list
    (`Tools/data/agf_elements.txt`) checked against the tables (2026-11-02).
  - [x] **7.1h Diagnostics:** `agsbuild --json`, clean cancel (2026-11-02).
  - [x] **7.1i `BuildWithTaskManager` split** per build stage (2026-11-02).
  - [x] **7.1j CI:** Linux job builds the auto-test with `agsbuild` for the platform jobs; a Windows
    job runs the parity harness (2026-11-02).
  - [x] **7.1k Incremental room builds in `agsbuild`** (redefined 2026-11-07; the intermediate
    directory, the task's first half, landed with 6.7d-6: `_obj/` next to `Compiled/` and `_Debug/`,
    `--temp-dir` defaults to it, cleared before each build, left after a failure for inspection, one
    `.gitignore` line; the editor's `game28.dta` in the project folder went with 6.7d-9). The old Run
    compiled only the rooms whose script had changed; `agsbuild` builds every room each time, so the
    build before a Run of a private AGS 4 game (53 rooms, 36 scripts) takes 8.6 s instead of 1.9 s,
    5 to 8 s of it `agsbuild --data-only` (LOG.md 6.7d-10). A room whose inputs did not change keeps
    its `room<n>.crm` in `_obj/`. The inputs are what `crmbuild` and `agscc` read for that room: the
    `Rooms/<n>/` sources and `room<n>.asc`, the generated headers (the room's own `_AutoGenerated.ash`
    included), the module and `--header` headers, the agscc flags, the part of `Game.agf` the room
    build uses (to be listed from `crmbuild`'s code), the tools' versions; compared by content (a
    manifest with hashes in `_obj/room<n>/`), never by `Game.agf`'s time stamp, which every save
    changes. `_obj/` is no longer cleared by every build: `--rebuild` clears it (the editor's Rebuild
    all passes it), and a room build that fails removes its own `.crm` and manifest, so nothing stale
    is ever recorded as up to date. Scripts, `game28.dta` and the packs stay full builds (fast today).
    *Done when:* on the private AGS 4 game a Run with nothing changed is back near the reference
    editor's 1.9 s and a Run after one room-script change rebuilds that room only (measured); a change
    to each input kind rebuilds the room (a test per kind); the incremental build's output is
    byte-identical to a clean build's on all three projects and every variant (`compare_builds.py`);
    the auto-test passes; a build failing halfway still leaves nothing new outside `_obj/` and the
    output folder, and the next build succeeds.
    *Landed 2026-11-07* (LOG.md 7.1k): a manifest of hashes per room (`_obj/room<n>/manifest.txt`), the
    `Game.agf` part written by `crmbuild --game-key`, `--rebuild` (Rebuild all, and a Run or Build the
    editor marks as needing a full rebuild); the rest of `_obj/` is still cleared. The Run with nothing
    changed takes 2.7 to 3.1 s against the reference editor's 1.6 to 1.7 s (8.4 to 8.8 s before): the
    rooms now cost nothing, the gap is the 38 script compiles, which stay full builds by this task's scope.
  - [x] **7.1p Incremental script compiles in `agsbuild`** (added 2026-11-07: 7.1k left the Run with
    nothing changed at 2.7 to 3.1 s against the reference editor's 1.6 to 1.7 s on a private AGS 4 game,
    and the 38 script compiles, about 0.28 s each, 8 at a time, are the whole gap; the user's criterion
    is the old editor's speed). As 7.1k for the scripts: a module whose inputs did not change keeps its
    `.o` in `_obj/` with a manifest of content hashes. The inputs are the script source (`<Module>.asc`
    in the project, or the generated `_GlobalVariables.asc` and `__DialogScripts.asc`, hashed as
    generated, so `Game.agf`'s time stamp plays no part), every header agscc gets in the order it
    gets them (API, `_AutoGenerated.ash`, `_GlobalVariables.ash`, `--header` files, module headers),
    the agscc flags and the agscc binary. Modules compile independently of one another's `.o`, so a
    changed module recompiles itself alone; a changed header recompiles every script. `--rebuild`
    clears them with the rooms; a failed compile leaves no `.o` recorded as up to date;
    `ScriptModules.lst`, `game28.dta` and the packs stay full builds. First measure where the time of a
    build with nothing changed goes (the export tasks, `agf2dta`, `check-rooms`, the pack, process
    start-up) and put the profile in LOG.md; if something other than the script compiles is a large
    share, report it rather than widen the task.
    *Done when:* on the private AGS 4 game the Run with nothing changed is as fast as the reference
    editor's (1.6 to 1.7 s from the key to the "compiled" box, same method, several runs; if agsbuild's
    remaining tasks make that impossible, the profile says what stands in the way) and a one-module
    change recompiles that module only; a change to each input kind recompiles what depends on it
    (a test per kind); the output is byte-identical to a clean build's on all three projects and every
    variant (`compare_builds.py --incremental`, extended with a script change); the auto-test passes on
    an incremental build; a build failing halfway leaves nothing new outside `_obj/` and the output
    folder, and the next build succeeds.
    *Landed 2026-11-07* (LOG.md 7.1p): a kept script is `_obj/<script>.o` with `_obj/scripts/<script>.o.manifest`,
    which also records the compiler's output, so a kept script's warnings are reported again; the compile task
    checks itself (`TaskCompileScript`); `room_cache` generalised as `build_cache`; and, asked for after the
    profile, the task manager wakes on a finished task instead of every 8 ms. `agsbuild --data-only` with nothing
    changed 0.34 s against 1.74 s; the Run 1.5 to 2.1 s against the reference editor's 1.3 to 1.7 s, the rest of
    the gap being the editor's work before `agsbuild` starts (1.0 to 1.1 s).
  - [x] **7.1l Scripts out of `game28.dta` in the editor too** (done by 6.7d-9 and 6.7d-10, 2026-11-07:
    the editor's `game28.dta` is `agf2dta`'s; 6.7d-13 made the harness compare without `unembed_scripts`). `agsbuild` packs
    `GlobalScript.o`, `__DialogScripts.o`, `ScriptModules.lst` separately (7.1f); `DataFileWriter.cs`
    still embeds them. Clear `HasCCScript`, write no script names in `v362_interevent2`, write the
    scripts to `_obj/` and pack them, include them in the debug run (`CreateDebugMiniEXE` packs only
    `game28.dta`). Room scripts stay in the `.crm`; the engine keeps reading embedded scripts (C1).
    *Done when:* no scripts in the editor's `game28.dta`, `compare_builds.py` all identical without
    `unembed_scripts`, auto-test passes on both builds.
  - [x] **7.1m PO translation sources** compiled like the editor's (2026-11-02).
  - [ ] **7.1n MO files:** dropped 2026-11-03 unless upstream asks (#1780 keeps `.tra`); if it comes
    back, propose it on #1780 first.
  - [x] **7.1o `trac` → `po2tra`**, `-u` decompiles to PO (2026-11-03).
  - **Thin format tools** (`agscc`, `crmpak`, `spritepak`, `agspak`, `po2tra`) stay, with tests for
    their command-line options.

  *Done when:* games build through the tools on Windows, Linux and macOS, byte-identical to the
  editor's former output; the editor builds through them (6.7d); no C# build code remains.
- [ ] **7.2 Bundled plugins.** `agslua` (unchanged since 2013, own Lua and zlib) goes unless a shipped
  game uses it; `agspalrender`'s 745-line `Raycast_Render` only if it's kept; the rest build with
  CMake only (3.2).
- [ ] **7.3 Ports** (D2): each kept port (Android, macOS, Web, iOS, Linux packaging) CMake-driven and
  documented in its README. **iOS has no CI job and no Mac to test on**, and its Xcode project
  (`iOS/xcode/ags`) keeps its own source list, which 4.4 and 5.3 would break: **before phase 5**, build it with CMake
  (`-DCMAKE_SYSTEM_NAME=iOS`, Xcode generator) in a compile-only `macos-latest` job (simulator, no
  signing), and treat a red iOS build as blocking.
  *Done when:* CI builds every kept port from CMake, iOS included.

---

## Upstream contributions

Work upstream (`ags4`) can use goes back as PRs on branches off `upstream/ags4`, in upstream's style
and without fork-only features, built in the worktree `E:\Projects\ags\ags4-upstream` with
`.compare/helpers/upstream_build.py` into `.compare/up-build`, recorded in `docs/UPSTREAM.md`.

- [x] **U.1 `ags4-bugfixes`** merged as #3181 (2026-11-02).
- [x] **U.2 `ags4-buildtools`** (#3182, 2026-11-03): the 7.1 work in upstream's formats, parity with
  the upstream editor; superseded by U.3's five branches.
- [ ] **U.3 Pieces for #2316.** Split #3182 along two of #2316's open items (4.0 room compiler,
  PO translation compiler) into PRs of reviewable size. The PO compiler goes upstream as the new
  tool `po2tra` (decided 2026-11-03); upstream's `trac` keeps compiling TRS and is removed only if
  the maintainers agree. `agsbuild`'s diagnostics move onto the Common log system (`debug::out`,
  #2530, ivan-mogilko's preference), so `--json` is a log sink. Follow-ups for the same milestone:
  `--version` (#3068), `agscc -I` (#3070).
  *Done when:* the pieces have no open review comments about structure.
  *Status (2026-11-03, details in LOG.md):* #3182 closed; re-cut as five branches on
  `upstream/ags4` (1 fixes, 2 table-driven `agfreader` + drift guard, 3 `agf2dta` parity, 4
  `crmbuild`, each on the previous; 5 `po2tra`), rebased after the `master` → `ags4` merge
  `e709f84a0`, tips verified. Opened as #3186–#3190 (2026-11-03; no reviews by 2026-11-04).
  Dropped until #2530: `--json`, cancel, the
  stage split. Not done: MSBuild and NUnit on the new branches.
- [x] **U.4 Agent docs upstream** (PR #3185, base `master`). Review changes are made in the
  worktree first and carried over here; the fork's copy leaves out AGS 3 (`master`, 3.x).
- [x] **U.5 Save fix** (PR #3191, 2026-11-04, `ags4-fixcharinvsave`): 4.0 saves before 4.0.0.28
  load again (LOG.md 1.2, U.5).
- [ ] **U.6 `AGS.Types` property grid out (#1092, part 1).** 6.0 step 1 on `upstream/ags4`, in C# 7.3
  / .NET Framework 4.7 as upstream builds: string-form `[Editor]`/`[PropertyTab]` attributes, the UI
  editor classes and `PropertyTabEvents` into `AGS.Editor`, direct calls instead of the static GUI
  delegates, unused usings removed, the attribute-resolution guard test. No behaviour change in the
  editor and no plugin API change, which is what ericoporto asked for in #1092 and ivan-mogilko
  confirmed in #3177. Step 2 (the plugin-facing interfaces) is posted on #1092 as a design question,
  not as a PR. *Decided 2026-11-04 (EDITOR-SPLIT.md §5).*
- [ ] **U.7 The 1.3 engine fixes** (PR #3193, branch `ags4-fixes-1.3`, 2026-11-05): the seven bugs
  the fork's engine tests found, one commit each, two tests ported into `common_test`; the five
  engine-side tests stay in the fork until upstream has a full-engine test target (LOG.md U.7).
  *Status (2026-11-04, details in LOG.md):* branch `ags4-types-propertygrid` cut by a Sonnet 5.5
  subagent, Release x86 builds, NUnit 382 of 382; the user checked the property grid in the running
  editor (properties panel and Settings) and opened the PR. First review the same day: the classes go
  to `AGS.Controls`, not `AGS.Editor`, and controls must not call `GUIController` directly (upstream's
  static delegates stay). Reworked as 3 commits on `de5508362` with the editors in
  `AGS.Controls/PropertyGridExtras`; awaiting the force-push and a re-check of the grid.

---

## Progress targets

`python Script/refactor_metrics.py` after each phase; update with actual values.

| Metric (BASELINE.md) | Baseline | After P2 | After P3 | After P4 | After P5 |
|---|---:|---:|---:|---:|---:|
| `NULL` | 276 | 0 | | | |
| old-style include guards | 372 | 0 | | | |
| using namespace in headers | 54 | 0 | | | |
| tab-indented lines | 5,511 | 0 | | | |
| AGS_PLATFORM_OS_PSP refs | 8 | | 0 (done, 3.1) | | |
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
| Build parity, `agsbuild` vs editor (7.1a) | 17 of 17 (scripts as separate files matched to the editor's embedded ones); auto-test 2414 of 2414 (7.1f) | | | | (7.1) all |

## Definition of done for any refactor PR

- One kind of change per commit, labelled (`[format]`, `[rename]`, `[move]`, `[delete]`, or a
  behaviour title); mechanical commits verified by 1.6 or the message says why not.
- Builds on all CI platforms; unit tests and the auto-test game pass.
- No contract snapshot changed, unless intentional and called out; no BASELINE metric worse.
- Comments in the touched code match the code.
