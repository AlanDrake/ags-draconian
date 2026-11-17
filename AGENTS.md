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
| [docs/refactor/RUNTIME-SPLIT.md](docs/refactor/RUNTIME-SPLIT.md) | the design of the engine's contexts (`GameDef`, `Game`, `Room`, `EngineServices`), definitions and live objects (`XDef`, `X`, `XState`), where serialisation goes, the savestate format | working on 5.1, 5.1a, 5.8 to 5.12, or adding a global, a game struct or save code |
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
- **Reuse before writing:** a function that works on text, bytes, paths or files in general is looked
  for first (`grep` in `Common/util` and `data_helpers.h`; the table in STYLE.md "Shared helpers"), then
  used, or extended with a test. No copy with one case added, no second reader or writer of a format.
  `Script/check_helpers.py` (prepush stage `helpers`) reports the new functions named like a shared one.
- Private game projects used as a corpus never appear in the repo (paths, names, content).
- Upstream work (`U.x` tasks) is done in the worktree `../ags4-upstream` on branches off
  `upstream/ags4`, in **upstream's style and C++11**, without fork-only features
  (`docs/context/decisions.md`, `docs/context/workflows.md` "Submit"). A tool there mirrors
  **upstream's** Editor, not the fork's: before a push, `python Script/check_helpers.py --repo
  ../ags4-upstream --base upstream/ags4` (repeated helpers, comments citing Editor members upstream
  does not have; `workflows.md` "Verify before a PR" 7 and 8).

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
- `Game.agf` and `Rooms/<n>/data.xml` are read through tables (`Tools/data/xml_fields.h`: one row per
  element, unknown elements reported, the drift guard `Tools/data/agf_elements.txt` against `AGS.Types`).
  Upstream reads them as one `ValueParser` line per property and has no drift check (ericoporto's review
  of #3187, 2026-11-08). The fork keeps the tables on purpose: here the tools are the only build path, so
  an element they do not read must fail the build, which only a table or a hand-kept element list gives.
  The two readers no longer share structure; port between them with `.compare/helpers/port.py`
  (`docs/UPSTREAM.md` "Taking an upstream change").
- Games build through the C++ tools (D7): `agsbuild` → `agf2dta`, `agscc`, `crmbuild` (rooms from
  `Rooms/<n>/`), `po2tra`, `agspak`; `agsbuild --json` gives machine-readable diagnostics;
  `ci/corpus/run.py` runs a private game through every room. The Editor builds through them too
  (6.7d, 2026-11-07): Build, Run, templates and translations run `agsbuild`, `agspak`, `crmbuild` and
  `po2tra` from `Editor/AGS.Editor/Build/`; the C# data writer, the in-process compile and the C#
  preprocessor are gone, so `docs/context/repository.md`'s `DataFileWriter.cs` steps do not apply: a
  project property the engine needs goes to `Tools/data/agfreader.cpp` and `data_file_writer.cpp` only.
  `Script/compare_builds.py` proves byte parity between the Editor's build and a plain `agsbuild` run
  (CI's `Windows Build Parity` job), and against the reference editor from the tag `pre-6.7d`, the last
  one with the C# build: `--editor <reference> --api-header pre-6.7d`, since an editor compiles with its
  own build's `agsdefns.sh` and a later header changes the compiled scripts (`--tools-editor` cannot be
  compared with the reference any more for that reason).
- Windows 7+ only; PSP is gone (3.1; `eOS_PSP` and the `"psp"` config name stay for the script ABI
  and engine config).
- Game data from 3.99 on (D1; 3.4-1, 2026-11-07; upstream loads 3.6.0.21+): `kGameVersion_LowSupported =
  kGameVersion_399`, the pre-3.99 upgrade paths and version values are gone, an older game gets a "compiled
  with ... this engine loads games made with AGS 3.99 or later" error. Rooms from 4.0 on (3.4-2,
  2026-11-08; upstream reads 3.5.0.8+): `kRoomVersion_LowSupport = kRoomVersion_400`, an older `.crm` gets
  `kRoomFileErr_FormatNotSupported`; the `v363_objects` block and LZW backgrounds stay (4.0 rooms use them).
  The editor opens projects saved by 4.0 or later (version index 3999901, open-format rooms; 3.4-2): an
  older one is refused with "open and save it with the AGS 4.0 editor first", "Import existing room" and room
  templates take 4.0 `.crm` files only. Saves and the script API are still at upstream's minimums (3.4
  steps 3 and 4).
- CMake and the hand-written Windows solutions are the build systems for the engine, `agscc` and the
  tools (D5; 3.2a/b, 2026-11-05; 3.2c, 2026-11-07): the Makefiles, `make.yml` and `dh --buildsystem
  makefile` are gone, `debian/rules` calls CMake, CI installs the tools with `cmake --install`. Adding an
  engine `.cpp` means editing `Engine/CMakeLists.txt` and `Solutions/Engine.App/Engine.App.vcxproj`
  (+ `.filters`); the `buildlists` stage checks that the two agree. Release builds link SDL2
  dynamically (`SDL2.dll`, the Linux bundle's `libSDL2`). Two standalone plugin Makefiles stay
  (`Plugins/agslua`, `Plugins/agspalrender`, phase 7).
- The engine's layout is the fork's own (5.3, 2026-11-14): upstream keeps the flat `Engine/ac/`, `Engine/gui/`
  and `Common/ac/`, the fork has rings and domain folders (below). A path of upstream's tree maps to the
  fork's through `docs/refactor/moves-5.3.tsv`; a port goes through `port.py`, which reads that map.
- Upstream is read-only: `git fetch upstream`, cherry-pick with `-x`, record in `docs/UPSTREAM.md`.

## Where things are (fork)

`docs/context/repository.md` applies, with these differences: `Compiler/script` (new compiler),
`Editor/AGS.Editor/Build/` (the editor's tool runners: `BuildToolRunner` for `agsbuild --json`,
`ToolProcess` for the others, `TemplatePackage`, `TranslationCompiler`; no `DataFileWriter.cs`, no
`ScriptCompiler.cpp` in `AGS.Native`; `AGS.CScript.Compiler` is gone, `FastString` lives in
`Editor/AGS.Editor/Utils/`),
`Tools/crmbuild`, `Tools/data/xml_fields.h` (table-driven `Game.agf` reading, drift guard
`Tools/data/agf_elements.txt`), `Script/compare_objects.py`, `Script/compare_builds.py`,
`Script/refactor_metrics.py` (BASELINE.md numbers), `Script/tidy_fix.py`, `Script/tidy_diff.py` (clang-tidy on changed lines), `Script/check_helpers.py` (repeated helpers), `Script/rename_fields.py` (compiler-exact rename of struct fields), `ci/corpus/`,
`.github/workflows/verify.yml`, `Script/prepush.py`, `Script/linux_jobs.sh` (the Linux CI jobs, run by
`prepush.py`). Local-only helpers and build trees live in the git-ignored
`.compare/` and `.tidy/`.

**The engine's layout** (`Engine/README.md`, `Engine/game/README.md`): four rings, each a set of directories
that may include only its own ring and the ones below it: host (`util`, `debug`, `platform`, `device`, `gfx`,
`media`, `resource`), script VM (`script`), game (`game`), composition (`main`, `plugin`); Common is under all.
`Script/check_layers.py` checks the includes against `Script/check_layers_baseline.txt`, which only shrinks, and
fails on an `extern` declaration in an engine source file (5.1 step 1, 2026-11-15): a global is declared once, in
the header of its module, and its users include that header (`Script/collect_externs.py` rewrites them).
What lives as long as the process is not a global any more (5.1 step 2, 2026-11-15): the configuration
(`EngineConfig`, upstream's `GameSetup usetup`), `ResPaths`, the platform and graphics drivers, the asset manager,
the engine version and the loop flags (`want_exit`...) are members of `EngineServices`
(`Engine/platform/base/engine_services.h`, host ring), reached through `EngineServices::Current()`; a function
that uses it more than once takes `EngineServices &services = EngineServices::Current();` as its first statement.
A new process-wide value is a member there, not a global; `Script/use_context.py` converts the uses of a global
to a context's member (`--check` fails on a converted global that is named again).
The game's and the room's globals are members of two more contexts (5.1 step 3, 2026-11-15): `Game`
(`Engine/game/game_context.h`; `class Game : public GameDef`, where `GameDef` in `Common/game/game_def.h` is
upstream's `GameSetupStruct` and its base; `Game::Current()`, local `Game &game`): `game.Characters`, `game.Options`,
`game.Play` (a `GameState`: upstream's `GamePlayState play`, less its room fields, its viewports and its per-object
property lists), `game.Viewports` (`GameViewports`: the viewports and cameras that upstream's `GamePlayState`
holds), `game.Views`, `game.Guis` (a member of `GameDef` still: the loader reads into it), `game.DisplayedRoom`,
`game.Palette`,
the script objects and handles, and state structs such as `game.Cursor`, `game.Speech`, `game.Scripts`,
`game.Events`; and `Room` (`Engine/game/world/room_context.h`; `class Room : public RoomStruct`,
`Room::Current()`, local `Room &room`): `room.Width`, `room.Options` (upstream's `thisroom`), `room.State`
(`croom`), `room.Objects` (`objs`), `room.BgFrame`, `room.WalkableAreasOn`, `room.EnteredAtX` (upstream's
`play.bg_frame`...). The sprite cache is `EngineServices::Sprites`, the mouse position
`services.Input.MouseX`. A function that uses a context once takes the direct form (`Game::Current().ViewCount`);
more than once, or once inside a loop, the local as its first statement; a helper called per drawn or updated
entity takes the context from its caller as its first parameter(s), in the order `Game &`, `Room &`,
`EngineServices &`. The context headers stay light (RUNTIME-SPLIT.md 9): a member whose type needs a module header
is a `std::vector` of a forward-declared type or a reference member, never a new include of `game/ui/gui.h` and
its like. A new value of the game or of the room is a member of its context; the few globals that stay are listed
with a reason in `Script/check_layers.py` (`EXTERN_ALLOWED`: constants, stateless script managers, the script
ring's own state, crash handler values), which fails on any other `extern` variable in an engine header.
A definition is kept apart from its live object (5.1a steps 1 to 4, 2026-11-17; RUNTIME-SPLIT.md 2 and 3): `XDef`
is the record the game or room file holds (Common, never written by the engine after load), `XState` the fields
that exist only during play, `X` the live class that derives from both; a live type that holds nothing more than
its definition is an alias of it until it does. The live list has the definition's name and hides it; the
definition is read through `game.Def()` or `room.Def()`.

| Upstream | Definition (`Common/game/*_def.h`) | Live (Engine) | Live list |
|---|---|---|---|
| `CharacterInfo`, `CharacterExtras`; `game.chars`, `charextra` | `CharacterDef` | `class Character : CharacterDef, CharacterState` | `game.Characters` |
| `RoomData`, `RoomStatus` (`thisroom`, `croom`) | `RoomDef` | `RoomState`: the live entities of one room number | `room.State` |
| `RoomObjectInfo` and the engine's `RoomObject` (`objs`) | `RoomObjectDef` | `class RoomObject : RoomObjectDef, RoomObjectState` | `room.Objects` |
| `RoomHotspot`, `RoomRegion`, `WalkArea`, `WalkBehind` | `RoomHotspotDef`, `RoomRegionDef`, `RoomWalkableAreaDef`, `RoomWalkBehindDef` | `RoomHotspot`, `RoomRegion`, `RoomWalkableArea`, `RoomWalkBehind` | `room.Hotspots`, `Regions`, `WalkAreas`, `WalkBehinds` |
| `InventoryItemInfo` (`game.invinfo`) | `InventoryItemDef` | `class InventoryItem : InventoryItemDef, InventoryItemState` | `game.InventoryItems` |
| `DialogTopic` and its option | `DialogDef`, `DialogOptionDef` | `class Dialog`, alias `DialogOption` | `game.Dialogs` |
| `ScriptAudioClip`, `AudioClipType` | `AudioClipDef`, `AudioClipTypeDef` | `class AudioClip` (its own script object), alias `AudioClipType` | `game.AudioClips`, `AudioClipTypes` |
| `MouseCursor` (`game.mcurs`) | `MouseCursorDef` | alias `MouseCursor` | `game.MouseCursors` |
| `ViewStruct`, `ViewLoopNew`, `ViewFrame` | `ViewDef`, `ViewLoopDef`, `ViewFrameDef` | aliases `View`, `ViewLoop`, `ViewFrame` | `game.Views` |
| `FontInfo` | `FontDef` | the font module's own copy | `game.Fonts` (the definition's) |
| `game.options`, `playercharacter`, `SpriteInfos`... | members of `GameDef` | members of `Game` | `game.Options`, `PlayerCharacterID`, `SpriteInfos` (the table the sprite cache writes) |
| `GamePlayState play`; the loops' base `GameState` | | `GameState` (`Engine/game/game_state.h`); `GameLoopState` (`game_loop_state.h`) | `game.Play` |

- **Fields have STYLE.md names**, named for what they do, the script's or the Editor's name in a trailing comment
  where it differs: `ch->NormalView`, `ch->MoveListID`, `obj.Sprite`, `item.CursorSprite`, `frame.Delay`,
  `game.ViewCount`, `game.Play.MessageTimeLeft` (upstream's `defview`, `walking`, `num`, `cursorPic`, `speed`,
  `numviews`, `messagetime`). A field is renamed with `Script/rename_fields.py` (a map of `Struct::old<TAB>New`;
  it resolves each reference through libclang, never by spelling), as a `[rename]` commit of its own. The script
  reaches the state by offset (`Engine/game/dynobj/script_game.cpp`), whatever the C++ names; a script switch with
  a negative name stays the script's `int` and the logic asks a positive predicate (`IsDialogParserEnabled()`).
- **The game's live copies** are made by `StartFromDefinitions` (`Engine/game/game_init.cpp`, the first statement
  of `InitGameState`) and cleared by `unload_game`; what runs before the game starts reads `game.Def()`. The
  engine writes no member of `GameDef` after load but the GUI lists (5.1a step 5).
- **The room's live entities** have two kinds of definition field: the ones a room state keeps between visits and
  a save holds (an object's position and sprite, a hotspot's `Name`, a walk-behind's `Baseline`, a walkable area's
  `FaceDirectionRatio`), and the ones the room file gives again at every load (`RebindDefinition`, called by
  `BindRoomState`: a region's light and tint, a walkable area's scaling, script names, events, properties). An
  object has no second copy of its definition's fields (`obj.X`, `obj.Baseline`, one `Rect BlockingRect`).
- **Shared parts**: the play fields a character and a room object both have are declared once, in
  `Engine/game/world/entity_state.h` (`GraphicState`, `ViewAnimState`, `MoveState` with `kNoMoveList` and
  `IsMoving()`, `FrameSoundState`), as bases of `CharacterState` and `RoomObjectState`; the logic is still each
  type's own (5.10).
- **Custom properties**: the values a script sets during play are the live object's `PropertyOverrides`
  (character, inventory item, dialog, audio clip); the game file's are `GameDef`'s lists
  (`game.CharacterProperties[i]`...); the GUI's play values are still `game.Play.GuiProperties` and
  `GuiControlProperties`, the room's are lists of `RoomState`.
- **Field tables** (5.9, RUNTIME-SPLIT.md 6; `Common/util/field_table.h`): every data member of an `XDef` or
  `XState`, and of `GameState`, has one row in the struct's table, in its source file (`kCharacterDefFields` in
  `character_def.cpp`); a field added without a row does not build (`static_assert(FieldsCoverStruct(...))`), so
  these structs stay aggregates (no private data, no constructor written out). Not yet: `RoomDef`, `RoomState`,
  `GameDef`. A row is a name, a member and a role, nothing more: `Saved`; `FromDefinition` (the definition gives it
  again, a save does not hold it); `Derived` (computed); `Transient` (play state that a save does not hold and a
  restore does not give back); `Opaque`. A base that has a table is one `BaseFields` row; `DiffFields`,
  `PrintFields` and `AlterFields` work on any struct with a table and descend into its bases and members that have
  one, and a test that checks a save uses them rather than naming fields.
`Engine/game/` holds the game logic in `world`, `navigation`, `dialog`, `ui`, `render`, `audio`, `video`,
`input`, `save`, `lib` and `dynobj`, and the script bindings in `script_api/`. `docs/context/repository.md`
describes upstream's tree, so its `Engine/ac/` and `Common/ac/` paths map through
`docs/refactor/moves-5.3.tsv` (old path = upstream's, new path = the fork's; the functions of the old
`global_*.cpp` files are in `docs/refactor/fold-5.3.tsv`). A new script API function: the logic in the
domain file under `Engine/game/` and its declaration in that file's header; one table row in `Engine/game/script_api/<type>_api.cpp`,
`ScMethod<&Type_Name>("Type::Name^2")` or `ScStatic<...>`: the wrapper is generated from the function's signature
(`Engine/script/script_binder.h`, 5.2 step 2; no `Sc_*` wrappers and no `API_*` macros any more, a tag in the row
where the script's contract differs from the signature). The tools that did the move, with `--repo`:
`Script/split_bindings.py`, `Script/move_functions.py`, `Script/apply_moves.py`.

## Verify before you report done

`python Script/prepush.py --auto` before a push runs the stages that the changed files need, by the
`AUTO_RULES` table in the script (the editor's C# code: `msbuild` alone; the tools and the compiler: all
but `emscripten`; Common, Engine, Plugins: every stage; an unknown path: every stage), and lists the
changed files that no stage verifies (workflows, packaging, most scripts). `--dry-run` shows the choice.
Without `--auto` every stage runs, which a repository-wide change needs (`--list`, `--only`, `--skip`,
`--base`, `--emsdk <dir>`): `format`, `helpers` (`Script/check_helpers.py` on the changed lines: functions
named like a shared helper, comments citing an Editor member that is not there), `layers`
(`Script/check_layers.py`: an include against the engine's ring rules that is not in the baseline, an `extern` in an engine source file, an
`extern` variable in an engine header that its `EXTERN_ALLOWED` table does not list), `buildlists` (`Script/check_build_lists.py`: every `.vcxproj`'s and the macOS Xcode project's
(`OSX/xcode/AGSKit`) sources against its CMake targets, deliberate differences listed with reasons in the script), `build`
(+ `ctest`), `tidy` (clang-tidy on changed lines), `headers`, `libcxx`, `emscripten` (engine as GLES2, then
a syntax check of the Linux-only code), `linux` (the CI's GCC job: build, header check, `ctest`), `msbuild`.
On demand with `--only`: `sanitizers` (ASan, UBSan and leaks on the unit tests and the auto-test game),
`coverage`. The Linux stages run `Script/linux_jobs.sh` in a WSL 2 distro (`Ubuntu-24.04`, as CI's
`ubuntu-latest`) on a copy of the working tree, uncommitted files included. CI confirms, it does not
discover. Also: the auto-test game; `compare_objects.py` for mechanical commits; `compare_builds.py` for
tool or `agfreader` changes; the generated code or a benchmark for a change on a hot path (STYLE.md
"Performance"). A game run on the user's desktop (the auto-test game, a corpus or parity run) passes
`--windowed --background` to the engine: never fullscreen, and it keeps running when its window loses the focus
(`compare_builds.py` and `ci/corpus/run.py` do; a brief for an agent that runs a game says so).

Workflows: `verify.yml` (every push: format, GCC build + tests + auto-test game + tidy gate, header
checks), `ci.yml` (`ags4draconian*`, PRs: light set, which is the Windows CMake build with Ninja and the
auto-test game plus the MSBuild Engine, Tools and Editor jobs as x64 Release, the Tests job in full runs; tags, weekly, `gh workflow run ci.yml -f full=true`:
every platform and the MSBuild Debug builds). No Windows build is x86 any more: the editor, engine, tools and
installer are x64 (PLAN.md 2.9d, 2026-11-08); the Linux i386 packages stay, in full runs. The CMake jobs keep a ccache between runs (PLAN.md 2.9).
`macos.yml` (tags, weekly, on demand; macOS runners are often unavailable), `analysis.yml` (on demand:
ASan + UBSan 27 min, coverage per directory 14 min).

A local CMake build without a compiler launcher uses a precompiled header of stable content (2.10b, 2026-11-16;
`CMake/PrecompiledHeaders.cmake`: standard headers, four Common basics, glm for the engine, gtest for the tests;
MSVC, and GCC 11+ or Clang on Linux). Nothing that verifies does: `prepush.py`, `compare_objects.py`,
`linux_jobs.sh`, `verify.yml` and `analysis.yml` pass `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`, and a ccache job gets
none, so a missing `#include` that the header hides fails there, not in the local build. A compile database for
clang-tidy comes from a build without it (`tidy_fix.py`'s `.tidy/build`), and the compile-time readings of
BASELINE.md are taken without it. A header added to it must be stable and included by nearly every file of the
target: never a game header.

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
  `CMakeLists.txt` and in the `Solutions/` `.vcxproj` + `.filters` (both stay, 3.2c), and a file the macOS
  build compiles also in `OSX/xcode/AGSKit` (7.3; nothing builds that project, the check is its only guard); the `buildlists`
  stage and CI's Windows build job fail when the two differ, test files included. A new `.vcxproj` goes
  in the script's `PROJECTS` table; a new test target also needs a `Tests.App` project, `ci.yml` and
  `prepush.py` `TEST_RUNNERS`.
- **CI-only jobs changed blind** (3): the Linux builder image (Debian 10, GCC 8, glibc 2.28, no downloads
  on i386), Android, macOS, new workflows. Ask the user for a probe branch (a name outside
  `ags4draconian*`, only the affected jobs), then one full run.
- **Red run:** read every failed job, fix in one batch, push once.
