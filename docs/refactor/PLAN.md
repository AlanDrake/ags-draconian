# AGS Draconian refactor plan

A staged plan for making a codebase that grew from a 1990s DOS program consistent, testable and
easy to navigate, **without breaking the games built on it**. Style: [STYLE.md](STYLE.md). Starting
numbers: [BASELINE.md](BASELINE.md) (`python Script/refactor_metrics.py`). How finished tasks went
(dated status, measurements, bugs found on the way, the full definitions): [LOG.md](LOG.md).

## How to use this plan

Phases 0–2 come first, in order (safety net and tooling); phases 3–7 then overlap **module by
module** (`Engine/game/render/overlay.cpp` can be in phase 4 while the Editor is in phase 6). Phase 5 needs
3 and 4; 7.1 (the C++ build tools, D7) needs only phase 2, and 6.7d waits for it. Every task has a
**done when** line a machine can check. Each phase is useful on its own.

This fork is a testbed for **AI-assisted refactoring**: a task fits one session and is backed by an
automatic verifier (tests, the 1.2 contract snapshots, 1.6's object comparison, the metrics
script). A session reads `AGENTS.md`, "Next up" and the task's section (not this whole file),
STYLE.md. It ends a task by ticking the box, moving the task's definition and a dated status to
LOG.md, leaving a one-line result here, and updating "Next up". Commits and PRs name the task ID (`3.2`). Upstream's intent is in
`docs/context/` (on par with upstream PR #3185, AGS 4 only); the fork's plan against it is
`ALIGNMENT.md`; the evidence behind the editor split (6.0, 6.7c, U.6) is `EDITOR-SPLIT.md`; the design of the
engine's contexts, definitions and live objects (5.1, 5.1a, 5.8 to 5.12) is `RUNTIME-SPLIT.md`.

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
| C1 | Game data: `.ags`, `.crm`, sprite file + index, `.tra`, `.wfn`, **from 3.99 onward** (D1) | `Common/game/*`, `Common/gfx/sprite_file.*`, `Common/data/*` | Games built with 3.99/4.x editors must keep loading |
| C2 | Savegames written by 3.99+ engines | `Engine/game/save/*` | Players' saves |
| C3 | Script ABI: exported API names and signatures, `game.*`/`system.*` field offsets, bytecode | `Register*API()` in `Engine/game/script_api/`, `Engine/game/dynobj/script_game.cpp` and friends, `Engine/script/scriptexecutor.cpp`, `Editor/AGS.Editor/Resources/agsdefns.sh` | Compiled scripts link by name and offset |
| C4 | Plugin API: `IAGSEngine` vtable order, `AGS_Engine*` exports | `Engine/plugin/agsplugin.h` | Binary plugins |
| C5 | Editor project format (`Game.agf`, `.user`, room sources) | `Editor/AGS.Types` | Existing projects. *The editor plugin API left this row on 2026-11-04:* a new editor is a big step and the plugin surface (`EditorContentPanel`, `MenuCommand`, `IGUIController`, `IRoomController`, `IAGSEditor`) follows the better design; plugins recompile against the new `AGS.Types` (6.0 step 2, EDITOR-SPLIT.md §3) |
| C6 | Engine command line and `acsetup.cfg` keys | `Engine/main/main.cpp`, `Engine/main/config.cpp` | Launchers, store builds, players' configs |

C3 is defined by offsets and names, **not** C++ layout: `GamePlayState` fields are read through the
offset switch in `CCScriptGame::ReadInt32`, so renaming or reordering them is safe while that table
stays correct.

C2 constrains **reading** only (the user, 2026-11-15): the engine may write a new save format, and saves of
3.99+ engines in the current one load through a legacy routine (5.12).

## Decisions

Recorded 2026-10-31.

| ID | Question | Decision | Consequences | Unblocks |
|---|---|---|---|---|
| D1 | Oldest game version loaded (C1) | **3.99 and later** (`kGameVersion_399`) | All 3.x upgrade paths can go; room versions need care (3.4). *2026-11-08:* the editor follows: the one `.crm` reader is Common's, so after 3.4-2 the editor opens and imports **AGS 4.0 projects and rooms only**; a 3.x project is upgraded with upstream's 4.0 editor first. Legacy importers are rebuilt, if wanted, after the planned phases (L.1), not kept through them | 3.4 |
| D2 | Platforms | **Windows, Linux, macOS, Android, Web, iOS.** PSP goes; FreeBSD stays for now (shares the POSIX code) | iOS has no CI, no Mac and no project file on `ags4`: ignored until the refactor is done (7.3, 2026-11-09) | 3.1, 7.3 |
| D3 | Legacy script compiler | **Removed**; `script2` is the only compiler | 3.3 | 3.3 |
| D4 | Editor UI | **Avalonia is the main candidate**; WinForms goes into maintenance. *2026-11-03:* upstream plans no cross-platform editor but welcomes the core split (#1092, #3177); a-riddle-fan's `ags-for-mac` (3.6.x, .NET 10 + Avalonia, `AGS.Native` replaced by a CMake C library) is studied before 6.7c. *2026-11-13:* the room layer tree (6.9) is the last WinForms UI change, done before 6.8 to avoid designing the room pane twice | Work goes into a UI-free core and replacing the C++/CLI bridge, which Avalonia can't use (phase 6) | 6.x |
| D5 | Native build system | **CMake**; MSBuild for the C# projects and the hand-written Windows solutions, which stay (re-decided 2026-11-07, 3.2c) | Makefiles go; the `Solutions/*.vcxproj` are checked against the CMake targets (3.2c) | 3.2 |
| D6 | Minimum Windows | **Windows 7+** | No `v141_xp`; `AGS.Native` targets `0x0601`; no newer APIs without a runtime check | 0.4, 3.1 |
| D7 | How games are built | **One implementation, in C++: the tools in `Tools/`** (`agsbuild` running `agf2dta`, `agscc`, `crmbuild`, `po2tra`, `agspak`, ...). The editor saves and runs `agsbuild` or single tools. The C# build code (`DataFileWriter.cs`, `DialogScriptConverter.cs`, script-header generation, translation writer, C# preprocessor) is deleted at parity. *Revised 2026-11-01 after maintainer review* (replaced a C# build core argued from commit counts) | Builds need only a C++ toolchain (Linux i386, FreeBSD, CI without the editor); the data writer sits next to the engine's reader. Cost: `Game.agf` has a C# and a C++ reader, so a new property needs both; 7.1g makes a missed one fail tests | 1.7, 3.3, 6.3, 6.7d, 7.1 |

## Next up

Kept current as tasks land. Model and effort are recommendations for an AI session. Done: phases
0–2, 3.1, 3.3, 3.4-1, 3.4-2, 3.4-3, 3.4-4, 3.5, 3.7, 4.1, 5.1 steps 1 and 2 (2026-11-15), 2.10a (2026-11-15), 5.2, 5.3 (2026-11-14), 6.7d,
6.9, 7.1 (but 7.1n) (LOG.md); 3.2 with 3.2c
(CI-confirmed 2026-11-08). The fork does not wait for upstream review (the user, 2026-11-09): a fork-only
move that would conflict with an open offer is preceded by a tag of the pre-move tree for `port.py`.

| # | Task | Why now | Model / effort |
|---|---|---|---|
| 1 | **The bugs found in 4.1, by upstream branch** (the user, 2026-11-12; the list is "4.1's bugs by upstream branch" below): all fixed in the fork (2026-11-12, `d90b789ab`..`98a7dae87`, `4c07f1e11`; lip-sync and the push button still want a run in a game); ported to `master-41-fixes` and `ags4-41-fixes`, trial merge of the first into the second on `ags4-41-mergecheck` (`docs/UPSTREAM.md`, worktrees `../ags-up-master`, `../ags-up-ags4`; the trial merge's worktree is gone, its branch stays); double-checked, fixed, rebased and pushed to `origin` 2026-11-13 (`53a55451d` on `upstream/master` `fc4c47a7f`, `9d32c9654`; what the check found is in `docs/UPSTREAM.md`); left: open the two PRs (texts in `.compare/pr-description.md` of each worktree), the trial merge is from the first version and not redone, and the fork's reader still gives `RunGameLoopsWhileDialogOptionsDisplayed` true for an absent element where `Settings.FromXml` gives false (no effect on 4.0 projects) | 4.1 is done; the PRs were waiting for it | Fable 5.1, `high` for the review; the user opens the PRs |
| 2 | **5.1 and 5.1a, by the design of 2026-11-15** (`RUNTIME-SPLIT.md`, reviewed with the user; LOG.md "5.1 step 2 and 5.1a: the design" lists what was decided and what each task still has to design). The order: **(a)** done 2026-11-15: 2.10a, the compile-time metric (BASELINE.md "compile time"), and the three save defects of RUNTIME-SPLIT.md 11 with a test each (`1dc82bb6c`); **(b)** done 2026-11-15 (`b8ac51d20`..`59b5d7092`, not pushed): 5.1 step 2, `EngineServices` (LOG.md "5.1 step 2": baseline 143 to 94; the sprite cache is left to (d)); **next: (c)** 5.8 step 1, Common's save code into the engine, and 6.7a, which takes the old-game import out of `AGS.Native` before the renames reach it; **(d)** 5.1 step 3, `GameDef`, `Game`, `Room`; **(e)** 5.1a step 1 (`Character`), 5.9 on it, then 5.1a steps 2 to 4; **(f)** 5.11 and 5.12, the restore into a second `Game` and the new save format; **(g)** 5.1a step 5 (GUI) with 6.7c. Any time after (d): 5.8 step 2. After (e): 5.10. Any time: 2.10b. Small follow-ups left by 5.2 and 5.3: the host half of `asset_helper.h` (that of `runtime_defines.h` is done, `platform/base/engine_defines.h`); the signature fixes that 5.2 step 2's tags mark (LOG.md "5.2 step 2", each a behaviour change to decide). | Steps 1 and 2 are done: each remaining global has one declaration, the process-wide ones are out of the way, and `Script/use_context.py` converts the uses of a context; the design settles what the globals become before 1,860 uses of `game` are touched | Fable 5.2, `high` to supervise and for 5.9, 5.11 and 5.12's designs; Sonnet 5.5 or Opus 5.5 subagents for the conversions, in batches built once per batch |
| 3 | **4.5 + 3.6 on what had no comment pass** (770 first-party files, ~280k lines): batch 8 done (2026-11-13, LOG.md: `game`, `gamestate`, `room`, `dialog`, `savegame`, `game_init`, `script`, `guimain`, `engine`); next, the untouched files most-churned first (a ranking by commits since 2024, the batch-6 stray-rename files among them), then the D3D/OpenGL drivers with the `emscripten` and `linux` stages, and the compiler (7,440-line parser: its own plan, by function groups). Brief `.compare/briefs/brief-45-fileset.md` (agents never run `prepush.py`; the supervisor runs it once on the combined branch) | The files have their final paths since 5.3 (map: `docs/refactor/moves-5.3.tsv`; the brief's file lists use the old ones), and a comment pass is cheaper before 5.1 and 5.2 step 2 edit them again | Sonnet 5.5, `medium`, parallel subagents; the parser plan Fable 5.1 |
| 4 | **Upstream, when convenient:** U.8 (the 1.5b-2 fixes; `String(ptr, length)` separately if at all; 1.5b-4 leaks only if the user confirms); under review: U.3 (#3188, #3189; #3187 merged), U.4 (#3185), U.6 (#3192) | Real defects, small | Sonnet 5.5, `medium` (U.6: the user first) |

Open items from finished tasks (details in LOG.md "Open items"):
- 0.4: the CodeQL setting, `D3DCOMPILER_47` on Windows 7, the macOS 10.9 deployment target.
- 2.5, found by 5.1 step 1 (2026-11-15): no script or workflow checks `#pragma once`; nine headers lacked it and
  have it now (`9078a8c83`), the two `resource.h` files of Visual Studio's resource editor are left. A check in
  the `format` stage would make "enforced" true.
- 5.1's design (2026-11-15, RUNTIME-SPLIT.md 11): the three save defects are fixed, a test each (a character's
  animation volume lost on load, an animated overlay's wait counter not restored, `RoomStatus` reading the
  walk-area and walk-behind counts in the other order than it writes them; no existing save loads differently).
  Open, found on the way: `move_object` on an object that is already moving leaks its move list and leaves
  `Object.MotionPath` on the earlier path; `GetObjectIDAtRoom` tests against the scaled size; `RoomStatus::
  ReadFromSavegame` does not check its counts against the fixed arrays. Upstream `master` and `ags4` not
  checked for any of the six.
- 5.1 step 2 (2026-11-15): `ApplyBehaviorOptions` no longer uses its `GameSetupStruct &` parameter (its name is
  commented out; three callers pass `game`); `util/timer.cpp` is frame pacing that needs the host (five baseline
  lines) and belongs in the host ring; `Game_ChangeSpeechVox` and `ResourcePaths::VoiceAvail` keep two
  meanings of "voice available" (`EngineServices::VoiceAvail` is the one the game reads).
- 1.4: the `agfreader` items are closed: a project without `<Settings>` is refused by `AGFReader::Open` and `ReadFields`
  refuses a missing element (2026-11-12, test `AGFReader.OpenReportsMissingSettings`); `StrUtil::Unescape`'s read past
  the end went with its rewrite in `28badf14b` (U.3). Left: the `ReadAllData` wrappers drop `ReadFields`' error, so an
  invalid value or an unknown element under `UnknownElements::Fail` goes unreported there.
- 1.2: the `DataUtil::GameSettings` scalars start at `Settings.cs`'s defaults (`1bfcbfbb6`, `GameFPS` 60 in
  `5edb9f059`); upstream `ags4` (#3187, merged 2026-11-12) has the wrong `EXPECT_FALSE(s.PixelPerfect)` (should be
  `EXPECT_TRUE`) in `AGFReaderBlocks.GameSettingsDefaults`: fixed by the `ags4` PR of task 1.
- The editor writes the last-opened room's colours into the game palette (2026-11-12, LOG.md 7.1f): `RoomsComponent.
  CopyGamePalette` copies the `PaletteEntry` references (a class), so `CopyToAGSBackgroundPalette` sets the game's
  Background slots, and a save puts them in `Game.agf`; an 8-bit game's `game28.dta` then depends on the room opened
  last (by the reference editor at build time too). Upstream `ags4` has the same line. Fixed in the fork only, for build
  parity (the user, 2026-11-12: low priority upstream, no effect on a real game): `PaletteEntry.Clone`, tests
  `PaletteEntryTests`. A project saved before the fix keeps the room colours it already has in its Background slots.
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
- **4.1's bugs by upstream branch** (checked 2026-11-12 by reading each function on `upstream/master` and
  `upstream/ags4`; details in LOG.md "4.1 + 4.5 + 3.6 per file"). A bug on both goes to `master` (upstream merges it up);
  one in an AGS 4 feature to `ags4`. The fork's commits use the fork's names: a port renames back.
  - `master`, fixed in the fork: `is_pos_in_sprite` with a zero size (`df791c714`), `File.ReadRawChar` (`f04c500e3`),
    `DynamicSprite.Tint` / `Width`, `Height`, `ColorDepth` / `GetDrawingSurface` on a deleted sprite (`9ce810048`,
    `2b1389a2e`, `964ffb429`), `DisplayTopBar`'s time and font (`ea271ea5d`, `0becab61f`),
    `InventoryItem.CursorHotspotAlignment` (`a3e21a6e3`), older saves' `audio_speed` (the half of `8a25d5732` without
    the view anchor), the `FaceDown` turn (`63da172bd`, test `818449240`), a point above the room (`6e8d72c93`), voice
    lip-sync with a pack other than `speech.vox` (`405cb0086`: `init_voicepak` reloads the sync data, the bare clip name
    is compared; `4c07f1e11`: a named pack's `syncdata.dat` is under its folder), the dialogs' push button follows the mouse (`14c94fcc9`), 4-bit `BitmapData` pixels (`d90b789ab`),
    `GetDiskFreeSpaceMB`'s failed call (`96493c8ce`), `lzwcompress`'s little-endian store (`a0085f0fa`), the
    `DataUtil::GameSettings` defaults (the tools, part of #3187's code: `1bfcbfbb6`, `5edb9f059`); cosmetic: the
    `SetPixels` and `Overlay.Height` warning names (`599acd6fd`, `6b718c3d1`), `|`/`&` for `||`/`&&` (`6c237db78`,
    `c2c23c177`, `0fd70e037`'s `CSCIWaitMessage`, `a8ee0fa4b`), `ListBox_AddItem`'s dead check (`f21086076`).
  - `ags4`, fixed in the fork: `DrawingColor` after `Release()` (`1c83f49aa`; `master` guards it), the scanner's
    `IsHexDigit` (`b59fd4b94`), `SetScale` registered twice (`9846f97d4`, `ac3bc8730`), the pivot of older saves
    (`d228d1baa`), `Joystick.GetHat` (`4b1ce87fb`), `Overlay.Tint` on a removed overlay (`9d81c631c`),
    `UpdateOverlayState`'s assert (`578f83fb2`), the save prescan of views (`edadb75aa`), older saves' view anchor (the
    other half of `8a25d5732`), `Speech.SkipStyle`'s read-back with the gamepad/touch flags (`e036d4ade`), the tools'
    `WriteGlobalMessagesBlock` (`f161028ac`: not dead but harmful: the engine does not skip the block for 400_33+, so
    a `Game.agf` with any global message gave a game that did not start; the tools only, `ags4`'s editor no longer
    writes it).
  - Fork only, all fixed: the editor's palette leak and `agfreader` without `<Settings>` (2026-11-12, below), Ctrl+D's
    "trnsp" and the Opacity range checks (`603365067`, `c81711ba6`, `e903548c7`).
- Bugs found by the comment pass (task 3; LOG.md "Batch 8"), open in the fork, all on upstream `master` and `ags4` too
  (so for the `master` PR of task 1, or one after it; the user decides): the viewport/camera renumbering in
  `DeleteRoomViewport`/`DeleteRoomCamera` (`[index]` for `[i]`), `Game.SetAudioTypeVolume`'s unchecked `change_type`
  (out-of-bounds read), the dialog options' text-input key running the key-press handler instead of
  `dialog_options_text_input`, `get_room_bg_surface` never finding background 0, `MakeSaveGameDir`'s empty "remapped
  to" path; smaller ones listed there.
- Sprite placement (2026-11-12, LOG.md "Sprite placement and the Direct3D 9 half-pixel offset"), in long-term
  testing by the user: the software renderer's scaling, hit tests on the truncated scaled size, truncated frame
  offsets, and a camera stretch by integer maths (exact agreement of all renderers at any zoom) are left. Characters
  and room objects use their own origin rule since 2026-11-13 (`Common::SpriteOriginRule`; an unflipped even-width
  one stands a pixel right of earlier versions). The auto-test's expectations are in `ags-test-games` tag
  `draconian-auto-test-3`, which must be pushed with the engine commits. The D3D9 fix is prepared for upstream
  (`docs/UPSTREAM.md`, `ags4-d3d9-halfpixel`).
- 6.9 (LOG.md), both on `upstream/ags4` too: every loaded room's hotspots and regions subscribe to
  `InteractionSchema.Instance.Changed` and are never detached; `IRoomController.GetAreaMaskPixel` checks
  `x > mask.Width`/`y > mask.Height` (should be `>=`; one mask column past the edge reads the next row, on the last
  row past the array) and copies the whole mask for each pixel.
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
  `ags4-save-roundtrip` (#2814) upstream (#3147's tests went into #3187).
- [x] **1.3 Engine unit tests** (2026-11-05): 12 → 162 engine test cases, seven engine bugs fixed
  (LOG.md).
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
- [ ] **2.10 Compile time as a metric, then the cost per file** (2026-11-15, RUNTIME-SPLIT.md 9). A first
  reading of the ninja log of a full MSVC MinSizeRel build: a file of `Engine/game` costs about 2.4 s plus
  1.3 ms per line (no precompiled header, no unity build), so the 46 small files of `script_api/` take 122 s of
  the directory's 446 s.
  a. **Done 2026-11-15.** `Script/refactor_metrics.py` reads a ninja log and reports the total
     compile time and the median per first-party file; both go into BASELINE.md (MSVC and GCC), and each batch
     of phase 5 reports them before and after. Before 5.1 step 2. Result: all 558 first-party objects take
     1676 to 1731 s with MSVC and 1585 to 1604 s with GCC (median 2.6 to 2.7 s and 2.4 s per object); the
     run-to-run spread is 1 to 3%, 4% on the MSVC median (BASELINE.md "compile time", LOG.md 2.10a).
  b. Lower the fixed cost: a precompiled header for the engine targets, unity builds where
     `compare_objects.py` does not need one object per file (it compares per file, so not in its build).
     Measured both ways; the user decides. The fixed cost is about 2.4 s of a file's 2.7 s (2.10a), so first
     a trace of what it is made of (`-ftime-trace`, `/d1reportTime`): standard and SDL headers argue for a
     precompiled header (stable content only: std, SDL, gtest for the tests; one CI job built with
     `CMAKE_DISABLE_PRECOMPILE_HEADERS=ON` so missing includes still fail; phase 5's readings taken without
     it), game headers for the `game.h` design of 5.1 step 3. Unity builds only for `script_api/`, after
     phase 5 and against the precompiled header's result. Not pursued (the user, 2026-11-15): C++20 modules
     (the Debian 10 image's GCC 8, the hand-written projects, include-based tools and ports), ThinLTO (Clang
     only and already CMake's choice there; the slow links are MSVC `/GL` and GCC, which runs `-flto=auto`
     on 11+), a precompiled header for the vendored libraries (almost all C, built once, ccache).

  *Done when:* the two numbers are in BASELINE.md and in `refactor_metrics.py`; (b) is decided with its
  measurements.

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

- [x] **4.1 Parameter and local names** (2026-11-12; LOG.md "4.1 + 4.5 + 3.6 per file"). The `refactor_metrics.py` watchlist to zero (4,855 today:
  `chaa` 652, `eres` 445, `chi` 368, `objj` 335, `sds` 226, `tqs` 224, `guio` 220, `butt` 187, `chin`
  126, `char1` 98, `chap` 88, `scam` 82, `texx` 68, ...). One name per concept: `ch`
  (`CharacterInfo*`), `obj` (`ScriptObject*`; `room_obj` for a `RoomObject` beside it), `gui` (`GUIMain*`), `btn` (`GUIButton*`), `dst`
  (destination bitmaps), `err` (error results). IDE or clang-based renaming, never search-and-replace.
  *Done when:* cryptic names = 0, except the globals of 5.1.
  *Result:* 909 names, all of them the 5.1 globals; 7 batches, each file set in LOG.md with the naming conventions
  it set (`eval_result` and `qualifiers` in the compiler).
- [ ] **4.2 Function names.** Engine free functions: `snake_case` 949, `PascalCase` 1,094,
  `Type_Method` 3,119 (script API). Rename snake_case to PascalCase as phase 5 restructures their
  files, not before (many will be deleted or become methods).
- [ ] **4.3 Legacy struct fields** (`GamePlayState`, `GameSetupStruct`, `CharacterInfo`, `RoomStatus`:
  `usedmode`, `disabled_user_interface`, `messagetime`) to the STYLE.md form, per struct. Safe under
  C3 (offsets); keep the script-facing name in a trailing comment when it differs. Done with 5.1a, per struct
  (its point 3). *Started 2026-11-15:* `audio_speed` → `AudioSpeed` in `RoomObject` and `CharacterExtras`
  (`632e3fec5`), ahead of the rest because the tidy gate asked for it on a changed line; a legacy field whose
  declaration line a commit changes gets its STYLE.md name in that commit or a `[rename]` beside it.
- [ ] **4.4 File names** to `snake_case` after their main class (`gamesetupstruct.cpp` →
  `game_setup_struct.cpp`), as `[move]` commits together with 5.3 so files move once. *Part done 2026-11-14:* 5.3
  renamed 154 of its 278 moved files (`docs/refactor/moves-5.3.tsv`). Left: files in place that keep a glued name,
  for instance `Common/gui/guibutton.cpp`, `Common/util/textstreamreader.cpp`, `Common/data/assetmanager.cpp`,
  `Engine/script/scriptexecutor.cpp`, `Engine/gfx/gfxdriverbase.cpp`, `Engine/media/audio/audioplayer.cpp`,
  `Engine/platform/base/agsplatformdriver.cpp`, `Tools/agsbuild/taskmanager.cpp`: at least 65 files by a scan for
  names of ten or more lowercase letters with no underscore (shorter ones, such as `guibutton`, are missed).
- [ ] **4.5 Comments.** For each file of 4.1: remove history notes and author tags, fix comments
  describing old behaviour (DOS, palettes in 32-bit paths, removed functions, old versions), add a
  header comment to every public function. Each top-level directory gets a `README.md`: purpose,
  entry points, what must not depend on it. *Part done 2026-11-14:* `Engine/README.md` (the rings, entry points,
  one line per directory) and `Engine/game/README.md` (the subfolders, where new code goes, the rules for script
  bindings) carry the engine; the other top-level directories stay open.

## Phase 5: Structure

Each task is a series of small PRs.

- [ ] **5.1 Collect the globals.** 704 `extern` declarations in `Engine/`, 596 inline in `.cpp` files.
  Most shared: `game` (67 files), `thisroom` (31), `spriteset` (31), `gfxDriver` (28),
  `displayed_room` (20), `views` (18), `croom` (18), `playerchar` (17), `palette` (15), `platform`
  (13), `scsystem` (10), `objs` (10).
  1. [x] Every global declared once, in its module's header; `extern` in `.cpp` → 0. *Done 2026-11-15*
     (LOG.md "5.1 step 1"): 552 `extern` lines out of 107 source files (533 without the tests) by
     `Script/collect_externs.py`, 86 `extern` and 49 function declarations added to 26 headers by hand, each
     defining file includes its declaration. `Script/check_layers.py` now fails on an `extern` in an engine
     source file. Proved with `compare_objects.py` (MSVC `/O1`: no function changed) and a GCC object comparison
     (`-Os`: one libstdc++ function inlined differently; `-O3` with LTO: 33 of 14,530 functions of `ags`, none
     on a hot path); the first comparison caught `TRUE` turning into -1 in the Direct3D drivers through a new
     include, fixed. The layering baseline went from 128 to 143: the 15 dependencies on a higher ring that the
     `extern` lines hid.

  The design for steps 2 to 4, 5.1a and 5.8 to 5.12 is `RUNTIME-SPLIT.md` (2026-11-15, reviewed with the user).
  Globals are placed by when they die: `GameDef`
  (the loaded game file, never written after load), `Game` (one play-through: everything a save holds), `Room`
  (the loaded room, owned by `Game`), `EngineServices` (the process). `GameDef` replaces this plan's earlier
  `GameData`: one suffix for every definition, and the Tools already have `DataUtil::GameData`. Each context has
  one accessor that returns the current instance through a pointer, so a test or a restore can make another
  instance current: `Game::Current()`, `GameDef::Current()`, `Room::Current()`, `EngineServices::Current()`. A
  function that uses one more than once takes a local reference at its top (`Game &game = Game::Current();`),
  which step 4 turns into the parameter.

  2. [x] **`EngineServices`**: the graphics driver, platform, asset manager, sprite cache, the engine config
     (`usetup`, its class renamed `EngineConfig`), `ResPaths`, the loop flags (`want_exit`, `abort_engine`,
     `switched_away`, `game_update_suspend`, `proper_exit`), the `main.cpp` flags the game ring reads. It lives
     in the host ring and names the higher rings' types by forward declaration only, so every ring can include
     it; `EngineConfig` moves down from `main/` with it. `scriptExecutor` moves down to the `script` ring, the
     crash counters `eip_guinum` and `eip_guiobj` to `debug`. The palette is game state (a savestate holds it)
     and stays with `Game`: the three graphics drivers stop reading the global and are given the palette when
     it changes. The fields of `GamePlayState` that outlive a restore (`voice_avail`, `separate_music_lib`,
     `enable_antialiasing`, the text language) come here; those that describe the loaded game's packs and
     translation are reset by `RunAGSGame`. No name in it is in dispute, so it goes first. The pointer load of
     the accessor is measured on the draw loops before the first batch lands (STYLE.md "Performance").
     *Done when:* the layering baseline is 110 or less: 143 less the 21 includes of `main/game_setup.h` and 12
     of the 15 that step 1 exposed (`debug/debug.cpp` keeps its three until step 3: it reads the game's options
     and the room number, which are game state); these globals are reached only through
     `EngineServices::Current()`.
     *Done 2026-11-15* (LOG.md "5.1 step 2"; `b8ac51d20`..`59b5d7092`): `platform/base/engine_services.h`, a struct with
     the config (`EngineConfig`, by value), `ResPaths`, the engine version, the platform and graphics drivers,
     the asset manager, the loop flags, the two script-log flags, and the voice, music-pack and text-language
     values that were in `GamePlayState`; 656 uses of 14 globals converted by `Script/use_context.py` (148
     direct, 508 through a `services` local in 113 functions), which step 3 runs again for `Game` and `Room`.
     `scriptExecutor` is a global of the script ring, the crash marks are in `debug`, the drivers are given a
     pointer to the palette once (`IGraphicsDriver::SetIndexedPalette`). The layering baseline is 94 (143
     before). **Not done as written: the sprite cache stays a global** until step 3: it is built on a reference
     to `game.SpriteInfos` and `draw.cpp`'s texture cache binds to it during static initialization. The two
     base-ring files that read the loop flags (`util/timer.cpp`, `debug/debug.cpp`) include the host's
     `engine_services.h`: two new baseline lines, beside the ones they had for the platform driver. Measured
     (MSVC `/O2` objects before and after): `draw.cpp` +1.0% instructions, `screen.cpp` +1.3%, the per-object
     draw functions byte-identical, the instance pointer loaded once per function that has the local; two loops
     reloaded it per iteration (the GUI loop of `draw_gui_and_overlays`, `ShakeScreen`'s condition), about 0.2 ns
     each by a micro-benchmark, and were given the services from outside the loop (the user, 2026-11-15).
  3. **`GameDef`, `Game` and `Room`.** `GameSetupStruct` and its base become `GameDef`, with `views`,
     `dialog`, the GUI lists and the compiled scripts as loaded (a `[rename]`; `AGS.Native` compiles the
     struct, so with the `msbuild` stage). The member `GameInfo` (the game's descriptive texts, also the
     script type of that name) keeps its name: it is why the suffix is `Def` (5.1a). The globals of one
     play-through become members of `Game`, a batch per domain
     (`world`, `ui`, `dialog`, `audio`, `render`, the script heap: managed pool, script global data, handle
     arrays); `thisroom`, `croom`, `objs` and the room-loaded globals become `Room`; the room fields of
     `GamePlayState` go to `Room` and `RoomState`, its viewports and cameras to `Game` (RUNTIME-SPLIT.md 5).
     `game/game.h` is included by about 335 files and must not become the header that includes every entity:
     the mechanism is chosen here and judged by 2.10's numbers. Left by step 2: the sprite cache
     (`spriteset`, 165 uses) goes to `EngineServices` once the sprite table has its owner (the loaded
     `SpriteInfos` in `GameDef`, the live ones, which the cache resizes for dynamic sprites, in `Game`), and
     the texture cache of `draw.cpp` stops binding to it during static initialization.
     *Done when:* no `extern` of mutable state is left in an `Engine/` header but the contexts and a list of
     exceptions kept in `Script/check_layers.py` (constants, stateless managers); the median compile time per
     file is not higher than before the step.
  4. New and refactored functions take their context as a parameter; phase 1 fixtures construct the
     contexts, which makes engine code unit-testable.

  *Done when:* `extern decls in Engine .cpp files` = 0 and the top 12 globals are reached only
  through the accessors.
- [ ] **5.1a Definitions apart from live objects** (the user, 2026-11-09; redesigned 2026-11-15,
  RUNTIME-SPLIT.md 2 and 3). Nearly every field the game file holds is also changed during play and saved (37 of
  `CharacterInfo`'s 40), so the split is not by field: the **definition** is kept as loaded and never written,
  and the **live object** is built from it. Three words, confirmed by the user:

  | Word | What | Lives in | Example |
  |---|---|---|---|
  | `XDef` | the definition: the record the game or room file holds; a plain struct that reads and writes itself in that file; no save code, no engine include | `Common/game`, linked by tools and engine | `CharacterDef`, `RoomObjectDef` (today `CharacterInfo`, `RoomObjectInfo`) |
  | `XState` | the fields that exist only during play; a plain struct | `Engine` | `CharacterState` (today's `CharacterExtras` and the play-only fields of `CharacterInfo`) |
  | `X` | the live object: `class Character : public CharacterDef, public CharacterState`, plus behaviour | `Engine` | `Character`, `RoomObject` |

  Fields are declared once, in a struct, never twice. The stem is the Editor's class name; room entities carry
  the `Room` prefix; the project side keeps `AGF::X` and `DataUtil::XData`. `Def` and not `Info`: `GameInfo` is
  already the script type and the member that hold the game's descriptive texts (C3), and `Info` means facts
  about a thing elsewhere too (`SpriteInfo`, `PluginInfo`, which keep the suffix); `CharacterInfo`,
  `InventoryItemInfo`, `RoomObjectInfo` and `FontInfo` are renamed. The full name table is RUNTIME-SPLIT.md 3.

  1. **`Character`**: one live list in `Game` replaces `game.chars` + `charextra` (171 uses); the `Character_*`
     functions take `Character *`; `GameDef`'s characters are not written after load. First an alias
     (`using Character = CharacterInfo`) and the signatures, a `[rename]` that `compare_objects.py` proves, then
     the class; `CharacterInfo` becomes `CharacterDef` once the engine no longer names it (about 25 uses in
     Common and 5 in the Tools are left by then). Both structs have a member `flags` with different meanings (the `CHF_` bits and the
     `kCharf_` turning bits): the second is renamed before the two become bases of one class.
  2. **`RoomObject` and the room entities**, `RoomStatus` → `RoomState`, `RoomData` → `RoomDef`, `RoomObjectInfo` → `RoomObjectDef`
     (`RoomStruct`, its subclass with the Allegro bitmaps, is the bitmap half of the live `Room` and merges
     into it once `AGS.Native` no longer compiles it, 6.7e): region light and tint, walk-area scaling and the
     face ratio leave `thisroom` for the live objects; masks and backgrounds the script draws on are working
     copies in `Room`. Unlike the character, today's `RoomObject` repeats its record's fields under other names
     and types (`x`/`X` as `int`/`int32_t`, `num`/`Sprite` as `uint16_t`/`int32_t`, `baseline`/`Baseline` as
     `int16_t`/`int32_t`, four `blocking_` fields against a `Rect`): deriving from `RoomObjectInfo` removes the
     copies, and each type difference is a behaviour commit with a test. A field group that `Character` already
     has with the same meaning is declared once here, as a shared struct (RUNTIME-SPLIT.md 7: graphic
     properties, view animation, movement, blocking rectangle, frame sound); the logic stays per type until
     5.10.
  3. **The plain entities**: inventory item, cursor, audio clip, audio type, view, dialog, the options and
     palette, and the sprite infos. Each gets a live copy in `Game`; none has a `State` struct. The audio clip
     never changes but scripts hold a handle to it and the engine writes its id, so it has a live object too.
     The sprite cache is constructed on a reference to `game.SpriteInfos` and writes it (dynamic sprites,
     `SPF_SURFACEACQUIRED`): the live list belongs to the sprite cache, the loaded one stays in `GameDef`.
     The fix-ups the engine applies to the loaded data today (`OPT_GLOBALTALKANIMSPD`, the cursors' views in
     `engine_init_game_settings`) move into the loader or onto the live copies. After this step
     `GameDef::Current()` returns a `const GameDef &`.
  4. **What is left of `GamePlayState`**: the per-object property lists go to the live objects; the rest is the
     game's own state and is renamed `GameState`, the blocking-loop base class of that name renamed in the same
     commit.
  5. **GUI**: the data of `GUIMain` and the controls as plain records (`GUIDef`, `GUIButtonDef`, one struct
     per level of the hierarchy), the drawing as functions over the records with an interface for sprites and
     fonts in place of the hooks each host implements (`draw_gui_sprite`, `wouttext_outline`), the live classes
     in the engine. The editor's C library draws previews with the same functions (6.7c). After 6.7a. Until
     this step the definition list and the live list hold the same class, which is enough for steps 3 and
     5.11.
  6. 4.3 (struct field names) is done here, per struct, so each field is renamed once, in its final struct.
     Variables: a live object is `ch`/`obj`/`gui`, a definition `def` (STYLE.md "Reserved names").

  The script API sees the same handles (C3); plugins get the same opaque `AGSCharacter *` with a stable
  address (C4). Saves made before the split load through the legacy reader (5.8 step 1, 5.12).
  *When:* after 5.1 step 3 and 5.8 step 1; 5.9 starts after step 1.
  *Guard:* a CMake target that compiles `Common/game` with no `Engine/` on the include path, the save
  round-trip tests and the `compat_restore` fixtures, the auto-test game.
  *Done when:* `GameDef::Current()` is `const` and nothing casts it away, `charextra` is gone, no `Common` struct has
  a field the engine writes during play, and `agf2dta`/`crmbuild` link no Engine object.
- [x] **5.2 Script bindings apart from logic.** 1,598 hand-written `Sc_*` wrappers from 147 `API_*`
  macros (`script_api.h`), which sat interleaved with the logic.
  1. [x] **Each file's `Sc_*` block and `Register*API` in `Engine/game/script_api/<type>_api.cpp`** (2026-11-14,
     LOG.md "5.2 step 1 and 5.3"): 44 files split, 1,547 functions moved byte for byte by
     `Script/split_bindings.py` (`--repo`, so it can be replayed on an upstream tree), proved by
     `compare_objects.py`'s move verdict and `ScriptExports.*`. Left: `script/runtimescriptvalue.h` still
     includes `script_api.h`, so nearly every engine file gets it (`HEADER_RULES` in `check_layers.py` lets
     only `script/` and `game/script_api/` include it directly).
  2. [x] **The macros replaced by a C++17 template binder** (2026-11-14, LOG.md "5.2 step 2"):
     `Engine/script/script_binder.h` generates each wrapper from the function's signature, a row is
     `ScMethod<&Character_AddInventory>("Character::AddInventory^2")`; 1,644 of 1,645 rows converted by
     `Script/convert_bindings.py`, each generated wrapper proved identical to the macro wrapper it replaced by
     `Script/compare_bindings.py` (1,600 of 1,600 pairs, MSVC objects). `script_api.h` went from 840 to 81 lines
     and is no longer included by `runtimescriptvalue.h`. 69 rows carry a tag that keeps a contract differing
     from the signature; each is a candidate for a signature fix (listed in LOG.md).

  *Done* (2026-11-14), with two things kept on purpose: `Sc_System_Log` is hand-written (it formats without
  translating, which no row form does), and `script_api.h` keeps `ScriptSprintf` and the two macros of the plugins'
  C-variadic functions (contract C4).
- [x] **5.3 Reorganise `Engine/ac/`** (2026-11-14; LOG.md "5.2 step 1 and 5.3"; the full definition is there).
  `Engine/ac/` and `Engine/gui/` are gone and `Common/ac/` holds `def_version.h` alone: 278 files moved by
  `docs/refactor/moves-5.3.tsv` (154 renamed to `snake_case`; also the path table for ports), the thirteen `global_*` file pairs folded into
  their domain files (`docs/refactor/fold-5.3.tsv`). `Engine/` is four rings, checked by `Script/check_layers.py`
  (prepush stage `layers`, `verify.yml`; the ring table is in its docstring): host (`util`, `debug`, `platform`,
  `device`, `gfx`, `media`, `resource`), the script VM (`script`), the game module (`game`) and composition
  (`main`, `plugin`). `Engine/game/` has `world`, `navigation`, `dialog`, `ui`, `render`, `audio`, `video`,
  `input`, `save`, `lib`, `dynobj` and `script_api` beside its root files. `Engine/README.md` and
  `Engine/game/README.md` describe the layout. Tag `pre-5.3` is the last tree with the old paths.
  Left open from it:
  - `Common/ac/def_version.h` stays: CI and the packaging scripts of every platform name it (the user, 2026-11-14:
    it stays; phase 7 may move it when the packaging scripts are reworked).
  - The layering baseline, `Script/check_layers_baseline.txt`, 129 includes (128 since 5.2 step 2; 143 since 5.1
    step 1, which made 15 hidden ones visible; 94 since 5.1 step 2), only shrinks. 5.1 step 2 took
    out the ones that carried `usetup` (`main/game_setup.h`, 21); the rest (`main/game_run.h` 22,
    `plugin/plugin_engine.h` 12, the `debug` and `util` files) is for later tasks. Two small follow-ups: split `runtime_defines.h` and `asset_helper.h` into their host and game parts, and cut
    `script/script_api.h` from `script/runtimescriptvalue.h` (the cut is done, 5.2 step 2; the host's part of
    `runtime_defines.h` is `platform/base/engine_defines.h` since 5.1 step 2, and no host file includes
    `runtime_defines.h` any more; `asset_helper.h` is open: `media/audio/sound.h` and `media/video/flic_player.cpp`).
  - Per-folder READMEs (4.5) are two files for the engine, as above.
- [ ] **5.4 Split the longest functions** (BASELINE.md list), tests first:
  - `display_speech` (489 lines, nesting 8): text layout, portrait/bubble placement, lip-sync setup,
    blocking wait loop.
  - `load_new_room` (408): load data, reset state, run room scripts and events.
  - `ScriptExecutor::Run` (1,161): the long switch is expected; opcode families into inline
    handlers, benchmarked before and after against the corpus run.
  - `GamePlayState::ReadFromSavegame`/`WriteForSavegame` (303/215) and the `RoomObject`,
    `CharacterExtras`, `ScreenOverlay` serialisers: not split here any more. They become the legacy reader
    as they are (5.12), and the new save format is written from the property rows (5.9, 5.12).
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
- [ ] **5.8 Each format described once, in its layer** (2026-11-15, RUNTIME-SPLIT.md 4). A component describes
  itself in the formats that live with it: the game file through the record's own methods, as today, and the
  new save format through its property rows (5.9, 5.12). Today's save code is the exception: it becomes the
  record of a dead format, so it is gathered in one place. No step here adds a file (a file costs about 2.4 s
  to compile, 2.10).
  1. **Common's save code into `Engine/game/save/savegame_components.cpp`**: the `...Savegame` methods of the
     six `Common/game` files (`character_info`, `inventory_item_info`, `mouse_cursor`, `dialog_topic`,
     `audio_clip_type`, `game_setup_struct`), whose fields are public. First the definitions as they are (a
     `[move]` that `compare_objects.py` proves), then as functions of the save file, their declarations out of
     the Common headers (the save round-trip tests). The tools compile this code today without using it.
     Before 5.1a: `CharacterInfo` and `CharacterExtras` both have `ReadFromSavegame` and `WriteToSavegame`,
     which one class cannot inherit twice. The eight `Common/gui` files keep theirs until 5.1a step 5 (private
     members). The engine's own structs (`GamePlayState`, `RoomStatus`, `RoomObject`, `CharacterExtras`,
     `ScreenOverlay`) keep theirs until 5.12, when what is left of them joins the legacy reader.
  2. **The game file's missing writers into Common**, beside their readers, as methods on the records (as
     characters, inventory items and cursors have them): views first, then the header, written by hand in the
     tools today. `Tools/data/data_file_writer.cpp` becomes the conversion from the project structs to a
     `GameDef`. Proved by `compare_builds.py`; a new read-then-write test in Common guards C1 without the
     engine. After 5.1 step 3; the GUI part after 5.1a step 5.
  3. Optional: layout rows for the game file (RUNTIME-SPLIT.md 6), when a record's code is touched anyway.

  *Done when:* no `Savegame` method is left in `Common/game` (in `Common/gui` after 5.1a step 5);
  `data_file_writer.cpp` writes no stream; build parity holds; the engine's and Common's file counts are not
  higher than before.
- [ ] **5.9 Property rows** (2026-11-15, RUNTIME-SPLIT.md 6). One row per field beside its struct
  (`Prop<&CharacterDef::Baseline>("Baseline")`), in the style of `xml_fields.h` and `script_binder.h`. From the
  rows: comparison and dump of two objects (tests, the check of a restore, later an inspector), the new save
  format (5.12), the script API's plain getters and setters, a drift check of `agsdefns.sh`. `Character` first,
  after 5.1a step 1; then each struct as 5.1a splits it. The task opens with a design pass on how far to take
  data-driven code (the user, 2026-11-15: left for this session): what a row carries beyond name and member
  (range, default, script exposure), whether a row can drive behaviour (a change hook), and whether a schema
  file should generate the rows; decided against a real `Character` and `CharacterState`.
  *Done when:* every `Info` and `State` struct has its rows (a test fails for a field without one), and a test
  compares two `Game` objects built from the same `GameDef` as equal.
- [ ] **5.10 Shared parts, the logic** (2026-11-15, RUNTIME-SPLIT.md 7). After 5.1a step 2 has made the shared
  structs: one implementation each of the view animation tick and its start, the move along a path, the graphic
  property setters (tint, light, opacity, scaling, rotation), the blocking rectangle and the
  `Run...Interaction` dispatch, used by characters, room objects, and where they have the part animated
  buttons, overlays and GUI objects. The types differ today (scale as percent or factor, flag polarity, the
  "none" values, default anchors, which setters mark the object changed), so each part starts with tests that
  pin today's behaviour per type (ground rule 3), and each difference removed is a behaviour commit the user
  decides. The script functions stay per type (C3) and call the shared code.
  *Done when:* each operation above has one implementation; the pinned tests pass or their change is recorded.
- [ ] **5.11 Restore into a second `Game`; a new game from `GameDef`** (2026-11-15, RUNTIME-SPLIT.md 8; the
  reason for the split, the user). After 5.1 step 3 and 5.1a steps 1 to 3.
  1. `Game(const GameDef &)` builds the whole starting state: what `InitGameState`,
     `engine_init_game_settings` and the `GamePlayState` constructor do in three places today.
  2. A restore reads into a second `Game`, validates, and makes it the live one; a refused
     `validate_restored_save` goes back to the first. One managed pool per `Game`. Two things are designed
     here, not settled: how the second `Game` becomes live (exchanging the contents element by element keeps
     the addresses plugins hold, C4, but the pool that was filled during the read maps handles to the second
     instance's addresses and must be rebound; switching the current pointer keeps the pool right and moves the
     addresses), and who owns the dynamic sprites while two instances exist.
  3. `RetryWithClearGame`, the `IsGameClear` checks, `PreservedParams::GameOptions` and
     `DialogOption::OriginalText` go. For the user: whether `RestartGame()` without a restart point starts a
     new game from the `GameDef` (it fails today).

  *Done when:* a test restores a truncated save and the game goes on; a test builds two `Game` objects, runs
  one, restores a save into the other and compares; `RunAGSGame` is the only path that reloads the game file.
- [ ] **5.12 A new save format, the current one read by a legacy routine** (the user, 2026-11-15:
  the save format need not be kept; RUNTIME-SPLIT.md 6a). After 5.9, and 5.11 step 1. Written from the property
  rows: per type the saved field names once, then the values; the reader matches names, so a field the save
  lacks keeps the definition's value and a field the engine dropped is skipped, with no version number per
  field. Only what differs from the definition is written (today each save holds every frame of every view and
  every GUI in full). What rows cannot describe keeps a writer of its own inside the new container: managed
  objects (their managers serialise them already), script global data, plugin data, bitmaps, the lists that
  grow during play (overlays, move lists, room states). The code of 5.8 step 1 and the engine structs' save
  methods become the legacy reader, never extended; the old writers are deleted. To decide here: matching lists by script name, so that a save survives a game patch that adds a
  character or a GUI.
  *Done when:* the engine writes the new format only; the `compat_restore` fixtures and one written by the last
  engine with the old format load; save size and time on the corpus game are recorded against today's.

## Phase 6: Editor

~107k lines of C# on .NET Framework 4.7 / C# 6, plus the C++/CLI bridge `AGS.Native`
(`agsnative.cpp` 3,668 lines). Per D4, WinForms gets bug fixes only (after 6.9, the last planned UI change, so
the Avalonia port starts from a settled design); the work is a UI-free core for a
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
  - [ ] **6.7a Delete the old-game import** and its menu entries. Before 5.1a step 5, and best before 5.1
    step 3: `AGS.Native` compiles `GUIMain`, `RoomStruct` and a `GameSetupStruct` (`thisgame`, 189 uses), part
    of it for this import, so every rename of those types has to build there until it is gone.
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
- [ ] **6.8 Avalonia front end** (after 6.3, 6.7 and 6.9), on a branch over `AGS.Editor.Core`, read-only
  views first (project tree, room viewer). Planned in its own document once the core exists.
- [x] **6.9 Room editor layer tree** (the user, 2026-11-13; done and merged 2026-11-13, LOG.md).
  The last planned WinForms UI change, done before 6.8 so the Avalonia front end ports one settled design: the
  breadcrumb bar became a tree on the left of the room pane (`RoomLayerTree` without UI, `RoomLayerTreeView`), the
  unused area slots in a collapsed "Unused (n)" group (`RoomAreaUsage`), the bar and `AdressBarEx` deleted.

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
  documented in its README. **iOS is ignored until the refactor is done** (no
  `project.pbxproj` on `ags4`, a DIY port with a forum guide, 2026-11-09). The macOS Xcode project
  `OSX/xcode/AGSKit` keeps its own source list, which no CI job builds (`macos.yml` uses CMake): it is
  kept in step by `Script/check_build_lists.py` (the `buildlists` stage), against the CMake targets.
  *Done when:* CI builds every kept port from CMake, iOS excepted.

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
- [ ] **L.3 What the contract keeps out of phase 5** (2026-11-15, RUNTIME-SPLIT.md 10; recorded, not designed):
  identity by a stable id instead of the array index in the script ABI (C3), handles instead of addresses for
  plugins (C4), blocking calls as script coroutines instead of nested game loops, one schema that generates
  both the C++ property rows and the C# properties of `AGS.Types`.

---

## Upstream contributions

Work upstream (`ags4`) can use goes back as PRs on branches off `upstream/ags4`, in upstream's style
and without fork-only features, built in the worktree `E:\Projects\ags\ags4-upstream` with
`.compare/helpers/upstream_build.py` into `.compare/up-build`, recorded in `docs/UPSTREAM.md`.

- [ ] **U.3 Pieces for #2316**: the 7.1 tools as PRs of reviewable size, along #2316's open items (4.0 room
  compiler, PO translation compiler). Upstream's `trac` stays unless the maintainers agree to remove it.
  `agsbuild`'s diagnostics move onto the Common log system (`debug::out`, #2530, ivan-mogilko's preference), so
  `--json` is a log sink. Follow-ups for the same milestone: `--version` (#3068), `agscc -I` (#3070).
  *Done when:* the pieces have no open review comments about structure.
  *Status:* #3187 merged 2026-11-12 (`140b7d7de`); #3188, #3189 open, review round 3 answered and force-pushed
  2026-11-12 (`docs/UPSTREAM.md`); rebased on `140b7d7de` and pushed again 2026-11-13 (`d8cf4ef42`, `b75cf43e4`):
  `agfexport` writes `acsetup.cfg` with `IniUtil::WriteToString` again (ivan-mogilko's review: `Merge` on a deleted file
  merges nothing), the game info texts unescaped as upstream's tool has them, the dialog option texts too;
  `--json`, cancel and the stage split wait for #2530.
  *Later (the user, 2026-11-13; agreed with ivan-mogilko on #3188):* one line format in `IniUtil`, a separate change off
  `master`: `IniUtil::Write` and `WriteToString` give `key = value` (`WriteToString` with `\n`), `IniFile`, which `Merge`
  and so the Editor's `acsetup.cfg` go through, gives `key=value` with the platform's line break. Until then the tools'
  `acsetup.cfg` differs from the Editor's in spaces and line breaks only.
- [x] **U.4 Agent docs upstream** (PR #3185, base `master`). Review changes are made in the
  worktree first and carried over here; the fork's copy leaves out AGS 3 (`master`, 3.x).
- [ ] **U.6 `AGS.Types` property grid out (#1092, part 1).** 6.0 step 1 on `upstream/ags4`, in C# 7.3
  / .NET Framework 4.7 as upstream builds: string-form `[Editor]`/`[PropertyTab]` attributes, the UI
  editor classes and `PropertyTabEvents` into `AGS.Editor`, direct calls instead of the static GUI
  delegates, unused usings removed, the attribute-resolution guard test. No behaviour change in the
  editor and no plugin API change, which is what ericoporto asked for in #1092 and ivan-mogilko
  confirmed in #3177. Step 2 (the plugin-facing interfaces) is posted on #1092 as a design question,
  not as a PR. *Decided 2026-11-04 (EDITOR-SPLIT.md §5).* *Status:* #3192 open (editors in
  `AGS.Controls`, static delegates kept).

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
| extern decls in Engine headers (5.1 step 3) | 203 (2026-11-15) | | | | the contexts and the listed exceptions |
| layering baseline includes (5.1 step 2) | 143 (2026-11-15) | | | | 94 (2026-11-15, step 2 done; the target was ≤ 110) |
| `Savegame` methods in `Common/` (5.8) | 14 files | | | | 0 |
| compile time, total and median per file (2.10) | MSVC 1676 to 1731 s, 2.6 to 2.7 s; GCC 1585 to 1604 s, 2.4 s (2026-11-15, two readings each) | | | | not higher |
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
