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
| D1 | Oldest game version loaded (C1) | **3.99 and later** (`kGameVersion_399`) | All 3.x upgrade paths can go; room versions need care (3.4). *2026-11-08:* the editor follows: the one `.crm` reader is Common's, so after 3.4-2 the editor opens and imports **AGS 4.0 projects and rooms only**; a 3.x project is upgraded with upstream's 4.0 editor first. Legacy importers are rebuilt, if wanted, after the planned phases (L.1), not kept through them | 3.4 |
| D2 | Platforms | **Windows, Linux, macOS, Android, Web, iOS.** PSP goes; FreeBSD stays for now (shares the POSIX code) | iOS can't be tested locally (no Mac): compile-only CI job (7.3) | 3.1, 7.3 |
| D3 | Legacy script compiler | **Removed**; `script2` is the only compiler | 3.3 | 3.3 |
| D4 | Editor UI | **Avalonia is the main candidate**; WinForms goes into maintenance. *2026-11-03:* upstream plans no cross-platform editor but welcomes the core split (#1092, #3177); a-riddle-fan's `ags-for-mac` (3.6.x, .NET 10 + Avalonia, `AGS.Native` replaced by a CMake C library) is studied before 6.7c | Work goes into a UI-free core and replacing the C++/CLI bridge, which Avalonia can't use (phase 6) | 6.x |
| D5 | Native build system | **CMake**; MSBuild for the C# projects and the hand-written Windows solutions, which stay (re-decided 2026-11-07, 3.2c) | Makefiles go; the `Solutions/*.vcxproj` are checked against the CMake targets (3.2c) | 3.2 |
| D6 | Minimum Windows | **Windows 7+** | No `v141_xp`; `AGS.Native` targets `0x0601`; no newer APIs without a runtime check | 0.4, 3.1 |
| D7 | How games are built | **One implementation, in C++: the tools in `Tools/`** (`agsbuild` running `agf2dta`, `agscc`, `crmbuild`, `po2tra`, `agspak`, ...). The editor saves and runs `agsbuild` or single tools. The C# build code (`DataFileWriter.cs`, `DialogScriptConverter.cs`, script-header generation, translation writer, C# preprocessor) is deleted at parity. *Revised 2026-11-01 after maintainer review* (replaced a C# build core argued from commit counts) | Builds need only a C++ toolchain (Linux i386, FreeBSD, CI without the editor); the data writer sits next to the engine's reader. Cost: `Game.agf` has a C# and a C++ reader, so a new property needs both; 7.1g makes a missed one fail tests | 1.7, 3.3, 6.3, 6.7d, 7.1 |

## Next up

Kept current as tasks land. Model and effort are recommendations for an AI session. Done: phases
0–2, 3.1, 3.3, 3.4-1, 3.4-2, 3.4-3, 3.4-4, 3.5, 3.7, 6.7d, 7.1 (but 7.1n), U.1, U.2, U.4, U.5, U.9 (LOG.md); 3.2 with 3.2c
(CI-confirmed 2026-11-08). The fork does not wait for upstream review (the user, 2026-11-09): a fork-only
move that would conflict with an open offer is preceded by a tag of the pre-move tree for `port.py`.

| # | Task | Why now | Model / effort |
|---|---|---|---|
| 1 | **4.1 + 4.5 + 3.6 per file:** parameter and local names, comments, comment debris, `Engine/ac/` most-churned files first; one `[rename]` commit per file checked by `compare_objects.py`, then a `[comment]` commit | Phase 5 needs phase 4; mechanical and verifiable | Sonnet 5.5, `medium`, one file set per session |
| 2 | **5.2 step 1, then 5.3:** `Sc_*` blocks and `Register*API` out of the logic files (`[move]`, C3 snapshot); tag `pre-5.3`, then reorganise `Engine/ac/` with upstream's draft names where they overlap | Files move once (4.4 rides along) | Fable 5.1, `high` for the move map, subagents for the moves |
| 3 | **Upstream, when convenient:** U.8 (the 1.5b-2 fixes; `String(ptr, length)` separately if at all; 1.5b-4 leaks only if the user confirms); U.3 (#3186–#3190, #3185, #3191), U.6, U.7 (#3193) under review | Real defects, small | Sonnet 5.5, `medium` (U.6: the user first) |

Open items from finished tasks (details in LOG.md "Open items"):
- 0.4: the CodeQL setting, `D3DCOMPILER_47` on Windows 7, the macOS 10.9 deployment target.
- 1.4: `agfreader` crashes on a project without `<Settings>`; `StrUtil::Unescape` reads past the end.
- 1.2: 47 `DataUtil::GameSettings` fields have no initializer, so an absent `Game.agf` element leaves
  garbage instead of `Settings.cs`'s default (`{}` is not the fix); #3147's test has a wrong `EXPECT_FALSE`.
- 7.1f: parity measured on one test project; 8-bit games and old indexed interactions unexercised.
- 3.1: unchecked `GetDiskFreeSpaceExW` result in `AGSWin32::GetDiskFreeSpaceMB`.
- 1.5a: the 4-bit pixel write in `Common/gfx/bitmapdata.cpp` (wrong mask and shift, untested).
- 3.4-1: constants left behind (`GUI::DataVersion`, always-on behaviour switches, three dialog option
  offsets, `FindNearestWalkablePoint`'s old method).
- 3.4-2: the editor's project upgrade steps below the 3.99.99.01 floor, unreachable now and kept for L.1
  (`UpgradeGameCommonTask`, `ScriptsComponent`'s `SetRestartPoint()` step).
- 3.4-3: upstream's readers misread GUI saves 4000000-4000009 (no control refs; FIXME in `GUIMain::SkipSavestate`)
  and keep `dialog_options_zorder` for Game State 4000000-4000023; the floors could rise to 1765a7451's (2024-02).
- 3.4-4, a decision for the user: a script API level floor for 4.0 projects (the editor and `agscc` refuse levels
  below v399 or v400). It would free the gated registration, the four runtime switches and the `SCRIPT_API_v3xx` /
  `SCRIPT_COMPAT_v3xx` gates in `agsdefns.sh` (298 mentions below v399, 324 below v400); it costs a C3 change (C3 as frozen covers games compiled at those
  levels) and needs a clear error for an existing project or game that uses one. The way up for such a
  project: L.2.
- 3.5: `CCStaticObjectArray` and the `kScValStaticArray` branches of the script VM are unreachable (nothing makes such a
  value) but need a Release codegen comparison to go; `stream.h`'s big-endian branch does not compile
  (`ReadAndConvertArrayOfInt24`); the `DEBUG_SPRITECACHE` and `DEBUG_PATHFINDER` builds fail; `= delete` or keep for the
  unused copy/move members (`RoomData`, `OpenAlSource`, `SDLDecoder`, `Bitmap::operator=`); leftovers constant now
  (`RunEventInModules`' last parameter, the write-only `SPRCACHEFLAG_LOCKED` and `DebugManager::_freeGroupID`).
- Scripts in a git worktree: `refactor_metrics.py`, `prepush.py`'s `linux` and `msbuild` stages fail there.
- Stale: `Tools/test/directory_test.cpp`'s googletest comment; `FetchGoogleTest.cmake`'s pin for GCC < 5.

Ignored until the refactor is done: the iOS port (7.3, 2026-11-09).

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

- [x] **1.1 Own the auto-test game:** fork `ags-test-games` with tests for fork-only features
  (2026-11-01; since 1.7 built from source).
- [x] **1.2 Contract guard tests**, one per contract row C1–C5 (2026-11-04; LOG.md). Left: sending
  `ags4-agfreader-tests` (#3147) and `ags4-save-roundtrip` (#2814) upstream.
- [x] **1.3 Engine unit tests** (2026-11-05): 12 → 162 engine test cases, seven engine bugs fixed
  (LOG.md; upstream: U.7).
- [x] **1.4 Game corpus regression:** `ci/corpus/run.py` builds a private game and runs every room
  (2026-11-02). Game paths come from arguments or a git-ignored file under `.compare/`, never the
  repo; the script works on a copy.
- [x] **1.5 Coverage, sanitizers and warnings** (2026-11-06; LOG.md): `Common/` warning-free
  (`ags_warning_free`), sanitizer and coverage jobs on demand (`analysis.yml`, `prepush.py --only`).
  Open: 87 narrowing conversions (MSVC), the other directories' warnings.
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
- [x] **2.8 CI shape and the pre-push check** (2026-11-04): `verify.yml`, `ci.yml`, `Script/prepush.py`.
- [ ] **2.9 CI and pre-push time.** 2.9a–c done 2026-11-07 (ccache, light runs, `prepush.py
  --auto`; LOG.md). **2.9d** done 2026-11-08, landed 2026-11-09 (LOG.md): every Windows build
  is x64 (editor, engine, tools, installer), no x86 build is left where an x64 one exists, the Linux i386
  packages stay in full runs; the plugin DLLs are x64 with nothing to ship (the user, 2026-11-09). Open:
  upstream's workflows failing on the port
  branches; the laptop for the WSL stages. LTO stays in the verification builds (the user, 2026-11-07),
  and so does the auto-test game in the Windows CMake job (the user, 2026-11-09).
  *Done when:* CI confirms a push that changes a few files in under 10 minutes. The first push measured
  12 min 15 s on a fast runner, the usual one gives about 13 (LOG.md 2.9, 2026-11-09); with LTO and the
  game where they are, the 10 minutes need another saving or a new target.

## Phase 3: Dead code removal

Delete first, restructure after. Each removal is a `[delete]` commit verified by the test suite and
the corpus run (1.4).

- [x] **3.1 Dead platforms** (2026-11-04): PSP port, plugin PSP builds and branches,
  `AGS_PLATFORM_OS_PSP` (0 refs), pre-Windows 7 checks in the Editor, engine and installer; the
  script enum value and config name stay (LOG.md).
- [x] **3.2 Duplicate build systems** (D5; 3.2c confirmed by CI 2026-11-08). 3.2a/b done 2026-11-05: the Makefiles are gone, CMake
  builds the engine, tools, plugins and `debian/` (LOG.md; `Plugins/agslua/Makefile*` stays until the
  user decides).
  **3.2c, re-decided 2026-11-07** (LOG.md): the hand-written `Solutions/*.vcxproj` stay, so the two build
  descriptions must not drift: (1) a script compares every `.vcxproj`'s source list with its CMake
  target, with a short checked-in list of deliberate differences, run by `prepush.py` and CI; (2)
  `AGS.Native` stops linking `Compiler_md.lib` and `AGS.Editor.Full.sln` stops building `Compiler.Lib`
  (unused since 6.7d-12). SDL2 stays dynamically linked in the release builds.
  *Done when:* a source file in one build description and not the other fails `prepush.py` and CI;
  the editor builds without `Compiler.Lib`.
  *Status (2026-11-07, LOG.md):* done locally: `Script/check_build_lists.py` (26 `.vcxproj`, 15 deliberate
  differences, no drift), the `buildlists` stage, the step in CI's Windows build job; the editor builds
  without `Compiler.Lib`, build parity holds. CI (2026-11-08): both steps green, "26 .vcxproj match their
  CMake targets (15 deliberate differences)".
- [x] **3.3 Legacy compiler** (D3; 2026-11-07): one compiler, one preprocessor (C++), no
  `AGS.CScript.Compiler`; `FastString` lives in `Editor/AGS.Editor/Utils/` (LOG.md).
- [ ] **3.4 Legacy data-version paths** (D1). The largest deletion; one commit per step:
  1. **Game files.** Done 2026-11-07 (LOG.md): games older than 3.99 get a clean error;
     `legacy version checks` 62 → 0.
  2. **Room files.** Done 2026-11-08 (LOG.md): `kRoomVersion_LowSupport = kRoomVersion_400`, a 3.6.3
     `.crm` gets "Required format version: 36315, supported 4000000 - 4000034"; the 3.x branches and
     enum values are gone (`ReadExt_363_Objects` and LZW backgrounds stay: 4.0 rooms use them); the
     editor refuses projects older than 3.99.99.01 ("open and save it with the AGS 4.0 editor first")
     and says which `.crm` files it imports; `UpgradeGameRoomsOpenFormatTask` is gone. Pre-4.0 room
     version checks 3 → 0; ctest 1166 of 1166, the auto-test game 2414 of 2414.
  3. **Savegames.** Done 2026-11-08 (LOG.md): `kSvgVersion_LowestSupported = kSvgVersion_400` (no save that
     loaded is lost: older ones failed the Game State floor already), five component floors raised to what the first
     4.0 format wrote; the pre-4.0 branches and 66 enum values are gone; pre-4.0 save version checks 82 → 10 (the
     rest are 3.6-numbered versions 4.0 engines wrote); ctest 1169 of 1169, the auto-test game 2414 of 2414.
  4. **Script API.** Done 2026-11-08 (LOG.md): the API level is a project setting (v321 to v400_33 in a 4.0
     project), so the level gate and the four runtime switches stay; four wrappers nothing registered are gone
     (`_Old` among them), the snapshot is unchanged, a test pins the gate; ctest 1170 of 1170, auto-test 2414 of 2414.
  5. Savegame-only dead fields (`gscript_timer`, `replay_hotkey_unused`, ...) go **only at the next
     savegame format bump** (C2).

  *Done when:* `legacy version checks` = 0, the C1/C2 fixture tests pass, a 3.6 game fails with
  a clean "game too old" error, a 3.6 project and a 3.6 `.crm` get the editor's clean message, and a
  4.0 `.crm` still imports.
- [x] **3.5 Unused functions** (2026-11-08): confirmed by gc-sections links (GCC, MSVC, AGS.Native) and deleted:
  283 production functions (Engine 80, Common 189, Tools 7, Compiler 6, editor native 1) and 10 test helpers;
  `dead-code candidates` 151 → 0, the 8 documented exceptions in `DEAD_CODE_EXCEPTIONS` (LOG.md).
- [ ] **3.6 Comment debris.** Delete commented-out code (83 lines, mostly `Compiler/test`). Triage 735
  `TODO` / 332 `FIXME` / 80 `CHECKME`: resolve, reclassify per STYLE.md, or delete; per directory,
  with phase 4.
  *Done when:* no `CHECKME`/`HACK` in a cleaned directory and every `FIXME` names its wrong case.
- [x] **3.7 Obsolete API stubs** (2026-11-09): `wgt2allg.*` folded into `BitmapHelper::BlitAllegroBitmap`; the
  stubs of the five built-in plugins dropped (272 → 198 lines), those of the closed-source and defunct plugins
  (Steam, Galaxy, Wadjet, shell, URL, joystick) kept with their reasons in the file (LOG.md).

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
  tag the pre-move tree `pre-5.3` so ports have a fixed base (no waiting for U.3; the user,
  2026-11-09), and prefer upstream's names where they overlap (`game/`, `script/`, `gfx/`, `audio/`,
  `util/`); `navigation/`, `dialog/`, `ui/` stay fork-only.
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

- [ ] **6.0 `AGS.Types` UI-free.** Re-scoped 2026-11-04 by the 6.0 study (counts and evidence:
  EDITOR-SPLIT.md).
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
  | Script compiler glue (`ScriptCompiler.cpp`) | 300 | Gone (6.7d-12) |

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
    functions (`ags4` rooms are XML + PNG; only "Import existing room" and room templates read `.crm`,
    4.0 versions only after 3.4-2, so a read-only loader with fork-defined structs), its CMake-generated constants (6.7b ports them to C#), its templates (6.7b), its
    3.6 struct layouts (the fork's GUI model has Opacity and `ags4` fields). About 30–40 `extern "C"`
    functions in a CMake `SHARED` target linking `Common` and `Tools/data`: images (BMP/PCX/PNG with
    palettes), sprite file slot info/read **as RGBA** and a slot-by-slot writer with raw copy (the sprite
    set lives in C#, not in a native cache), `agsn_sprite_convert` from `sprite_utils` (ags-for-mac runs
    `spriteimport` as a subprocess instead), fonts (set, metrics, draw text, TTF size, SCI import), GUI
    preview from host-flattened descriptions **into RGBA** (not today's `drawGUIAt(HDC ...)` and
    `System::Drawing::Bitmap^`), `.crm` read for the upgrade. Ownership documented per function; C++
    tests in `Editor/native/test/`, plus a pixel comparison of the GUI and font output against the
    C++/CLI bridge before 6.7e switches (ags-for-mac's GUI test checks only sizes).
  - [x] **6.7d The editor builds through the tools** (2026-11-07, 13 steps; LOG.md): Build, Run,
    templates and translations run `agsbuild`, `agspak`, `crmbuild`, `po2tra`; the C# build code is
    gone; the output is byte-identical to the reference editor's (tag `pre-6.7d`) in every variant.
  - [ ] **6.7e Switch to the C library** via a C# P/Invoke wrapper in `AGS.Editor.Core` (`net48` and
    `net8.0`), one area at a time (sprites, rooms, fonts, GUI previews), each covered by 6.6 tests;
    view and sprite drawing move to the UI at the same time.
  - [ ] **6.7f Delete `AGS.Native`.**

  *Done when:* no C++/CLI left; the native dependency is one C library built by CMake for Windows,
  Linux and macOS, and `AGS.Editor.Core` loads it on Windows and Linux in CI.
- [ ] **6.8 Avalonia front end** (after 6.3 and 6.7), on a branch over `AGS.Editor.Core`, read-only
  views first (project tree, room viewer). Planned in its own document once the core exists.

## Phase 7: Build tools, plugins and ports

- [ ] **7.1 Make the C++ tools the only build path** (D7). Done 2026-11-01 to 11-07 (LOG.md): 7.1a–m,
  o, p: the parity harness `Script/compare_builds.py`, AGS 4 projects in every tool, one preprocessor,
  table-driven `Game.agf` reading with a drift guard, `crmbuild`, content parity, `agsbuild --json`, the
  auto-test game built by the tools in CI, incremental room and script builds, `po2tra`. 7.1n (MO
  files) dropped 2026-11-03 unless upstream asks (#1780 keeps `.tra`).
  Open: tests for the thin format tools' options (`agscc`, `crmpak`, `spritepak`, `agspak`, `po2tra`);
  what stayed in the editor after 6.7d: the pre-build checks (`RunPreCompilationChecks`, missing event
  handlers), the lip-sync compiler (Pamela, Papagayo), `AudioCache` upkeep.
  *Done when:* games build through the tools on Windows, Linux and macOS; no C# build code remains.
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

## Later: after the planned phases

Wanted, but not before phases 3–7 are done; nothing is kept in the code to make them easier
(decided 2026-11-08: clean up first, rebuild legacy paths afterwards if still wanted).

- [ ] **L.1 Legacy project import.** Open a 3.5.0.8–3.6.3 project (or import one of its rooms) in the
  fork's editor without a detour through upstream's 4.0 editor. Design chosen 2026-11-08: a tool,
  `crmimport`, in `Tools/` (D7 pattern, run by the editor like `crmbuild`): reads a `.crm` of any
  supported version with Common's reader plus a frozen copy of the 3.x code that 3.4-2 deleted (the
  3.x format is final, so the copy never drifts), and writes `Rooms/<n>/` sources: `data.xml` (a
  C++ writer, the mirror of `Tools/data/room_xml.cpp`, table-driven), backgrounds and masks as PNG.
  The editor keeps the C# half (`Game.agf` upgrade: the 78 `[Obsolete]` shims in `AGS.Types`, the
  `UpgradeGame*Task` classes, the version-index checks; `room<n>.asc` and `.crm.user` moves). Not a
  C# port of the reader (a second implementation of the block format, with LZW, and no fixture) and
  not a copy inside the editor's native library (6.7c stays small). Safety net first: a 3.6.3 room
  fixture built with the 3.6.3 release editor as `make_fixtures.py` builds the 4.0 ones, and the
  reference conversion output taken from the pre-3.4-2 editor (tag it before deleting).
  *Done when:* the 3.6.3 fixture project opens, builds and runs; the converted sources match the
  reference output; the tool runs in the Linux CI job.
- [ ] **L.2 Script API compatibility** (2026-11-09; with the API level floor, open item 3.4-4; not
  designed yet). Two candidates, alone or together:
  - *Script shims* (the user): AGS script modules on the side. A removed or renamed API gets a drop-in
    with the old name; one whose meaning or range changed (Transparency 0-100, tint ranges) gets a new
    name and the user does a replace-all. Source projects only.
  - *A compatibility plugin* (ivan-mogilko): registers the old functions through the plugin API (C4).
    Could also serve games already compiled at an old level; native code on every port.

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
  *Status:* opened as #3186–#3190 (2026-11-03), awaiting review; `--json`, cancel and the stage split
  wait for #2530 (LOG.md).
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
  not as a PR. *Decided 2026-11-04 (EDITOR-SPLIT.md §5).* *Status:* v2 force-pushed 2026-11-04
  (editors in `AGS.Controls`, static delegates kept), awaiting review (LOG.md).
- [ ] **U.7 The 1.3 engine fixes** (PR #3193, branch `ags4-fixes-1.3`, 2026-11-05): the seven bugs
  the fork's engine tests found, one commit each, two tests ported into `common_test`; the five
  engine-side tests stay in the fork until upstream has a full-engine test target (LOG.md U.7).
- [x] **U.9 Sprite import overrun** merged as #3196 into `master` (2026-11-07); `NDEBUG` for upstream's
  tool projects dropped (LOG.md U.9).

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
