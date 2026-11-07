# Refactor log

What happened while each task of [PLAN.md](PLAN.md) was done: the dated *Status* paragraphs of
finished tasks, moved here verbatim (only the indentation was removed), with what was measured,
what was found on the way and what was left open. Since 2026-11-04 the full definitions of
finished tasks are here too, and the long status paragraphs of open ones (1.2, 7.1, U.3); PLAN.md
keeps a one-line result for finished tasks and a short current status for open ones. Read this
file to look up how a task went, not to start one. One section per task, in the plan's order;
entries are in the order they were written. The plan before that compaction is PLAN.md at
`ec43d3764`. A second trim on 2026-11-07 moved the long texts of
finished tasks and the open-items list here ("Moved from PLAN.md on 2026-11-07"); the plan before it
is PLAN.md at `4acf72ef5` plus that day's SDL2 note in 3.2c.

### 0.1 Tag the fork point

*Definition, moved from PLAN.md on 2026-11-04:*

**0.1 Tag the fork point.** Tag `fork-point` at `4cd88beba` (the current merge base with
`upstream/ags4`). Keep the `upstream` remote for reference only.
*Done when:* the tag exists and the README states the project is a hard fork.

### 0.2 Cherry-pick ledger

*Definition, moved from PLAN.md on 2026-11-04:*

**0.2 Cherry-pick ledger.** Create `docs/UPSTREAM.md`: a table of upstream commits that were
picked, adapted or deliberately skipped. Every cherry-pick gets a line.
*Done when:* the file exists and the cherry-pick process is described in CONTRIBUTING.md.

### 0.3 Record decisions D1–D6

*Definition, moved from PLAN.md on 2026-11-04:*

**0.3 Record decisions D1–D6** in the table above.

### 0.4 Raise the toolchain floor

*Definition, moved from PLAN.md on 2026-11-04:*

**0.4 Raise the toolchain floor.** `CXX_STANDARD 17` in all `CMakeLists.txt` (currently 11 in
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

### 0.5 Repository hygiene

*Definition, moved from PLAN.md on 2026-11-04:*

**0.5 Repository hygiene.** Add the stray local artefacts (`Engine/resource/fxc*.exe`, `fxc.zip`,
`Editor/References/x64/`) to `.gitignore` or commit them deliberately. Add a `.gitattributes` with
`* text=auto` and `eol=crlf` only for `*.sln`, `*.bat`, `*.cmd`, `*.vcxproj*` and `*.csproj`.
*Done when:* `git status` is clean after a full Windows build.

### 0.6 Agent context docs

*Definition, moved from PLAN.md on 2026-11-04:*

**0.6 Agent context docs.** `AGENTS.md` (fork rules and facts) and `docs/context/`
(upstream's repository map, workflows, decisions, roadmap, sources; every claim dated and
sourced), written for tools, ≤3k tokens per file. The same `docs/context/` is offered upstream
(PR #3185, 2026-11-03) and is kept identical to it; fork-specific knowledge goes into
`AGENTS.md` and `ALIGNMENT.md`, never into `docs/context/`.
*Done when:* a session can answer "what does upstream intend here" from the docs without
searching issues. *Refresh:* `roadmap.md` at each upstream milestone change, `decisions.md`
when a maintainer settles something (rules in `docs/context/README.md`).

### 0.7 Move status logs out of this file

*Definition, moved from PLAN.md on 2026-11-04:*

**0.7 Move status logs out of this file.** The "*Status*" paragraphs of finished tasks go to
`LOG.md` (one dated entry per task, same text), leaving each task here as description,
"done when" and a one-line result. Keep the "Found on the way" bugs and lessons: they are the
useful part, move them with the entry.
*Done when:* this file is under 8k tokens and `LOG.md` holds every removed paragraph verbatim.
*Status (2026-11-03):* done; 1372 lines, 114240 bytes (~28.6k tokens) -> 910 lines, 72.1k bytes (~18.0k tokens); LOG.md holds the removed paragraphs (574 lines).

### 1.1 Own the auto-test game

*Definition, moved from PLAN.md on 2026-11-04:*

**1.1 Own the auto-test game.** CI downloads `ags-test-games/releases/latest` from upstream.
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

### 1.2 Contract guard tests

*Status of 2026-11-03, moved from PLAN.md on 2026-11-04:*

*Status (2026-11-03):* C2, C3 and C5 done; C1 and C4 open. Nothing committed yet.
- **C3:** `Engine/test/script_exports_test.cpp` (new CMake target `engine_full_test`, since
  `engine_test` stubs `cc_error`/`debug_script_event` and can't link the whole engine) registers the
  current API with `setup_script_exports` and compares every exported symbol (`name kind plugin|-`,
  1635 lines: 518 functions, 1114 methods, 3 objects) with `Engine/test/data/script_exports.txt`;
  regenerate with `AGS_UPDATE_SNAPSHOTS=1`. `--print-scripttoc` dumps a compiled script, not the
  API, so it wasn't reused. Not covered: `game.*`/`system.*` field offsets, signatures beyond the
  `^N` argument count.
- **AGFReader per-block tests** (= #3147): written upstream-first on worktree branch
  `ags4-agfreader-tests` (`26db161ed`, 47 tests, C++11, XML string → parser, no files; upstream
  `tools_test` 124 of 124), checked on `ags4-tools-2-agfreader` too (139 of 139), then ported to
  `Tools/test/agfreader_test.cpp` (AGS 4 element names, `Opacity`). They document the upstream
  reader's drift from the editor (`Name`/`ScriptName`, `Description`/`DisplayName`, `RealName`,
  `Flipped`/`Flip`, interactions' `ScriptModule`, audio folder defaults, `LATEST_XML_VERSION`
  3.0.3.2), all fixed on `ags4-tools-2-agfreader`.
- **C5:** `Editor/AGS.Editor.Tests/Types/GameXmlRoundTripTests.cs` loads `Tools/test/data/Game.agf`
  (the auto-test project trimmed to ~80 KB, plus properties, a plugin, array variables; nothing
  private), saves it and compares the `<Game>` XML (only `<BuildTargets>` normalised: the editor
  filters it by registered targets); regenerate with `AGS_UPDATE_GAME_SAMPLE=1`. The same sample is
  read by `AGFReaderSample.*` in `tools_test` with `XmlProblems` empty (`agf2dta --strict`'s
  check) and entity counts. Mutations (a deleted element, an injected `<Unknown>`) fail both sides.
- **C2:** upstream-first on worktree branch `ags4-save-roundtrip` (`196a0fadb`, target
  `savegame_test`, 6 tests), ported to `Engine/test/savegame_test.cpp` (`engine_full_test`): ~16
  `play` fields written with `SavegameComponents::WriteAllCommon`, cleared, restored with `ReadAll`,
  written again: state restored and the two saves byte-identical; plain and compressed agree; each
  of the 14 components round-trips alone; a truncated save is refused. **Gaps:** the script
  component is skipped (`gamescript` is null without a loaded game: access violation), rooms,
  characters, GUIs and views only in their default state, `DoBeforeRestore`/`DoAfterRestore` not
  exercised; no real 3.99/4.0 save fixtures exist yet. `savegame_internal.h` isn't self-contained.
- Measured: fork ctest 874 of 874 (`tools_test` 167, `engine_full_test` 7), NUnit 394 of 394.
  Not verified: GCC/Clang builds and the header check for the new tests, MSBuild `Tests.sln` (the
  new C++ tests are CMake-only), CI, macOS/Linux.
- Left for the second part: C1 (fixtures from a 3.99 and an early 4.0 editor), C4 (the test
  plugin), real save fixtures for C2, and sending `ags4-agfreader-tests` (#3147) and
  `ags4-save-roundtrip` (#2814) upstream after the `ags4-tools-*` PRs.

*Status (2026-11-04):* second part done; 1.2 is closed.
- **Fixtures:** `Common/test/data/compat/<release>/game.ags` and `agssave.001` for the 4.0.0.27
  (Alpha 31, the earliest AGS 4 release at hand) and 4.0.0.30 releases, ~320 KB each. Built by
  `make_fixtures.py` from each release's own Sierra-style template (flat room background, so the
  room stays small; `fixture.asc` added to the global script) with the release's editor
  (`AGSEditor.exe /compile`, headless) and saved by the release's engine. The script, with no
  save, sets known state (a global int, an inventory item, the player's position, a `String`, a
  dynamic array, a disabled hotspot), saves to slot 1 and quits; with a save, it restores it,
  compares the state and writes `PASS` or `FAIL <what>` to `c1result.txt`. Each release restores
  its own save with `PASS`.
- **C1:** `Common/test/game_compat_test.cpp` (`common_test`) opens each package, reads the game
  data (data version, editor version string, names of the character, inventory items, GUIs, script
  module; counts of views, fonts, cursors) and `room1.crm` (size, hotspot name, compiled script).
  Room format of both releases: `kRoomVersion_400_26` (task 3.4 must keep loading it); game data
  `kGameVersion_400_27` and `_400_29`. The `compat_restore_<release>` ctest cases
  (`Engine/test/compat_restore.cmake`) run the fork's engine (Software driver, `--nosound`,
  `--user-data-dir` with the save) and fail without `PASS`. They run in every CI job that runs
  ctest; not on Android/Emscripten.
- **Bug found (C2), fixed:** `CharacterExtras::ReadFromSavegame` read the inventory of every 4.x
  save in the new format; 4.0 saves before `kCharSvgVersion_400_28` have the legacy one ("Characters,
  version 4000026 ... Component data size mismatch. Expected: 1894, actual: 46235"), so no save of
  4.0.0.27 or earlier loads. Cause: upstream `6ec11f244` (master's 3.6.3 change merged into ags4)
  gates on `kCharSvgVersion_36310` only, where the audio-speed block below it already has the
  ags4 range. Upstream ags4 has the same bug (not reported yet). With the fix reverted the
  4.0.0.27 case fails, with it both pass.
- **3.99 not covered** (user's decision, 2026-11-04: 3.99 was a transition phase before 4.0). A
  3.99.102 build loads its data, but its scripts don't link in the fork or in upstream ags4
  (`unresolved imports: IsVoxAvailable`): upstream removed deprecated exports in `5e852ea37`
  (2025-02); 186 of the 3.99.102 engine's non-plugin exports don't resolve.
- **Fork-only script-API breaks for 4.0 games (C3), fixed the same day (see the next status):** comparing
  the exports registered at the v4.0.0.30 tag with the fork's snapshot (by the fork's lookup rules,
  `systemimports.cpp`), these don't resolve: `get_/set_Transparency` of Character, GUI,
  GUIControl, Object, Overlay (the fork's Opacity). (`GUI::GetByName` and `Game::SimulateKeyPress`
  looked missing only because the release registered them without `^N`; the fork has the `^N`
  names that compiled scripts import.) The fixture game uses none, so the tests pass; a C3 test
  against the release's export list would catch them.
- **C4:** `Engine/test/plugin_api_test.cpp` (`engine_test`) dumps `Engine/plugin/agsplugin.h` with
  comments removed, one declaration per line in order, plus the `AGS_*` names that
  `agsplugin.cpp` looks up, and compares with `Engine/test/data/plugin_api.txt` (324 lines;
  `AGS_UPDATE_SNAPSHOTS=1`). Swapping two `IAGSEngine` methods fails at the right declaration. Not
  done: a plugin that calls every method (the layout, not the behaviour, is the contract).
- Measured: ctest 879 of 879 (5 new); MSBuild `Tests.sln` x86 Release: Common 102, Tools 167,
  Compiler 594, Engine 3, all pass (the new C++ tests are CMake-only); clang-format clean on all
  tracked files; the new files compile with the emsdk clang/libc++ check. `cc_format_error` and
  `cc_get_callstack` stubs live in `game_compat_test.cpp`: in `common_stubs.cpp` they collide with
  `Tools/data/cc_script_stubs.cpp` in MSBuild `Tools.Test`. Not verified: GCC, macOS, Linux runs of
  the engine cases (CI will be the first).

*Status (2026-11-04, later): the 4.0 Transparency compat exports.* The user chose engine exports
over script shims: compiled 4.0 games import `Character::get_Transparency^0` and friends by name, so
only the engine can satisfy them, and recompiled projects need just declarations.
- **Engine:** `[OBSOLETE] Transparency => Opacity` registrations of `get_/set_Transparency` on
  Character, GUI, GUIControl, Object and Overlay (file-local functions over the opacity field), with
  4.0's range checks and messages. Conversions `GfxDef::Trans100ToOpacity`/`OpacityToTrans100` in
  `gfx_def.h` compose 4.0's own legacy-255 helpers, so values equal 4.0's, including its drift
  (37 reads back as 38); opacity 251-254, which 4.0 could not store, clamps to 0 instead of -1.
- **Script header:** `import attribute int Transparency;` on the five types under
  `SCRIPT_COMPAT_v400_24` (4.0.0.30's API level): `agscc -C v400_24` compiles all five, the default
  level rejects them.
- **Tests:** `ScriptExports.ReleaseExportsResolve` checks every name of
  `Engine/test/data/script_exports_4.0.0.30.txt` (1552 names from tag v4.0.0.30, no plugin stubs,
  no `game_init.cpp` entity arrays) against the fork's table; names the release registered without
  `^N` count when any overload exists. Without the new exports it lists exactly the 10. The fixture
  script sets `player.Transparency = 37` before saving and expects 38 after the restore: both
  releases pass on their own engines, the fork passes, and without the exports the fork engine
  refuses the game (`unresolved import 'Character::get_Transparency^0'`); fixtures regenerated.
  `GfxDef.Trans100OverOpacity` compares the conversions with 4.0's for 0-100 and checks the range
  for every opacity. The export snapshot gained the 10 lines.
- Measured: ctest 881 of 881; MSBuild `Tests.sln` (Common 103, Tools 167, Compiler 594, Engine 3)
  and `AGS.Editor.Full.sln` build; clang-format clean. Not run: NUnit, GCC/macOS/Linux.
- Upstream: ags4 still has Transparency natively, so these exports only belong in an upstream PR
  that offers the fork's Opacity, in the same `[OBSOLETE]` + `SCRIPT_COMPAT` form.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 1.4 Game corpus regression

*Definition, moved from PLAN.md on 2026-11-04:*

**1.4 Game corpus regression.** A script (`ci/corpus/run.py`) that, for each game you own the
sources to: compiles all scripts with `script2` (before 3.3 removes the legacy compiler, run it
once with both and confirm every game builds with `script2`); runs the engine with
`--gfxdriver Software --no-message-box --log-stdout` through `--startr N` for every room;
and fails on warnings or crashes. Run it locally before risky merges and in CI nightly if the game
data can live in a private repo.
*Done when:* one command runs the whole corpus and reports pass/fail per room.
*Corpus (2026-11-02):* one private AGS 4 game built on this branch (project format 4.0.0.34,
53 rooms, 36 scripts, 1.6 GB); another can be prepared on request. Game paths are personal: the
script takes them from its arguments or a git-ignored file under `.compare/`, never from the repo,
and works on a copy (the editor's `/compile` writes into the project). For AGS 4 projects the
`script2`-vs-legacy step is likely moot (the legacy compiler predates AGS 4 syntax); confirm once.

*Status (2026-11-02):* done. `python ci/corpus/run.py --engine <ags.exe>` builds each game of the
corpus (arguments, or the git-ignored `.compare/corpus.txt`) from a fresh copy with
`agsbuild --strict`, then runs every room in its own engine process (`--startr N`, Software
driver, no sound, full log) for 10 seconds, four at a time, and prints pass or fail per room. A
room fails if the engine exits before the time is up, if the log has no `Loading room N`, or if
it has an `[ERROR]` or `[FATAL]` message; warnings are listed, and fail the room only with
`--fail-on-warnings`, because the private game logs one by design in every room (its script
looks for a data file in the save folder before the package). Result: 53 of 53 rooms pass; a room that doesn't exist is reported as a failure. The game builds with
`script2` (`agscc` uses it unless built with `LEGACY_COMPILER`), which is what 3.3 needs; the
legacy compiler was not tried on AGS 4 syntax. Found on the way: the engine exits with code 0
after a fatal error with `--no-message-box` ("Unable to load the room file"), so only the early
exit tells it apart from a normal quit.
Build parity with the editor: `compare_builds.py` on the game reports 114 of 114 files identical
(with the scripts as separate files, 7.1f) and its translation; the auto-test project stays at
59 of 59 and 2414 of 2414. The first parity run found:
- Runtime bugs in `agsbuild` builds: a clip folder's settings were not read (they are in a child
  element of the folder), so clips lost their inherited repeat, volume and priority; GUIs were
  written in folder order instead of by ID, so `gui[N]` could be the wrong GUI; FLAC clips had no
  file type; `\n` and other escapes in GUI texts, property values and the game name were not
  resolved.
- The PO translation gap (7.1m).
- Byte differences, made to match: the editor declared audio clips in folder order (now by ID,
  in the editor), interaction tables, two deprecated hotspot colours, room order, the packages'
  asset order, part name and ender (`audio.vox` named itself `Compiled\Data\audio.vox`).
- The harness had to change: copies leave out `.git`, AGS 4 rooms are compiled from their sources
  on both sides (the editor packs a newer `room<n>.crm` as it is), and the `.ags` packages are
  compared themselves, not only the files in them.
- Found, not fixed: `agfreader` crashes on a `Game.agf` without `<Settings>`;
  `StrUtil::Unescape` reads past the end of a text ending in a single `\`.

### 1.5 Coverage, sanitizers and warnings

*Definition: in PLAN.md; split into 1.5a to 1.5c on 2026-11-05.*

*Status (2026-11-06):* done. 1.5a and 1.5c done (2026-11-05), confirmed by a CI probe run (42050934970 Verify, 42050935242
Analysis); 1.5b: the job works and reported 47 findings; 1.5b-1 done, the job runs locally; 1.5b-2 done, the
memory errors are fixed; 1.5b-3 done, the first-party UBSan rows are fixed; 1.5b-4 done, the compiler leaks are fixed;
1.5b-5 done (2026-11-06), the engine leaks are fixed or suppressed, `sanitizers` is green locally and
in the on-demand CI run 42174071706.

**1.5a Warnings**
- Before, distinct first-party sites: GCC 15.2 `Common` 122, `Engine` 164, `Compiler` 71, `Tools` 69,
  `Plugins` 48, `Compiler/test` 755 (747 unused variables), `Tools/test` 61; Clang (emcc) `Common` 68,
  `Engine` 77; MSVC `/W4` `Common` 174 (87 narrowing conversions, 46 unused parameters, 14 signed with
  unsigned).
- Warning set: `-Winline` removed (122 GCC sites; it reports optimizer choices). `-Wfloat-equal` stays
  but is never an error. MSVC first-party level: `/W4` without C4100, C4018, C4245, C4389 (off on GCC
  too) **and without C4244, C4267 (narrowing; GCC needs `-Wconversion`): 87 sites in `Common`, open.**
- CMake: `ags_warning_free(<target>)` (`/W4` on MSVC through the `AGS_WARNING_FREE` property) and
  `option(AGS_WARNINGS_AS_ERRORS)`, off by default, set by `verify.yml` and `prepush.py`. `common` is
  marked.
- `Common/`: 95 sites, 39 files, mechanical (parentheses, `[[maybe_unused]]` on stream reads, unused
  code, shadowed names, `std::move` on returns, initializer order). 4 `-Wfloat-equal` sites stay.
- **Bugs found by the warnings, fixed** (upstream's code):
  - `room_file.cpp`, twice: `data_ver >= kRoomVersion_363_15 && data_ver < kRoomVersion_399` is never
    true (36315 against 40), so the backgrounds of 3.6.3.15+ rooms were read as LZW. Now
    `< kRoomVersion_400`. No test: no such room in the repository.
  - `guimain.cpp`: `GameGuiVersion < GameGuiVersion` in `ReadGUI`; now `< kGuiVersion_LowSupported`.
  - `directory.cpp`: `GetCurrentDirectory` used the buffer after a failed `getcwd`.
- **Seen, not fixed:** the 4-bit case of `BitmapData` pixel writing (`bitmapdata.cpp`): mask `4` and
  shift 0 or 1 where a nibble needs `0xF` and 0 or 4.
- Verified: all seven `prepush.py` stages (1031 tests); `common` warning-free with `/W4 /WX` (MSVC) and
  `-Werror` (Clang through emcc; GCC 15.2 on CI). GCC now: `Common` 4 (`-Wfloat-equal`), `Engine` 144,
  `Compiler` 33, `Tools` 28, `Plugins` 48. `compare_objects.py HEAD`: 591 identical, 38 differ, all
  explained (MSVC names a closure by a hash of its text, so a renamed lambda parameter changes the
  object).
- Found: with `/WX` or `-Werror` in the compile database clang-tidy failed on every `common` source
  (its "argument unused" warnings became errors); `tidy_diff.py` passes `-Wno-error`.

**1.5b Sanitizers, 1.5c Coverage** (`analysis.yml`, `Script/sanitizer_summary.py`,
`Script/coverage_summary.py`)
- Both jobs build tools, engine and tests, run `ctest`, then `Script/agsbuild_autotest.sh`: the game is
  built by the instrumented tools and run in the instrumented engine (Software driver).
- Sanitizers: GCC, `-fsanitize=address,undefined -fsanitize-recover=address`, RelWithDebInfo,
  `halt_on_error=0`, leaks on, `LSAN_OPTIONS=exitcode=0`: one run lists every finding, and the summary
  script fails the job if there is any. Coverage: clang source-based, `%m` profiles, `llvm-cov export
  -summary-only`, a table per module and directory.
- **On demand only** (user, 2026-11-05: 27 and 14 minutes): `gh workflow run analysis.yml --ref <branch>
  [-f sanitizers=false] [-f coverage=false]`, possible only once the file is on the default branch.
- **Coverage, first numbers:** total 53.7% of 101,506 lines; `Common` 61.6%, `Engine` 47.8% (`ac` 49.3%,
  `gfx` 18.6%, `gui` 8.9%, `plugin` 3.4%), `Compiler` 89.9%, `Tools` 68.2% (`crmpak`, `spriteimport`,
  `spritepak` under 3%, `po2tra` 0%), `Plugins` 0%.
- **Sanitizer findings, 47** (the run: 27 minutes, 21 of them the build):
  - ASan, each in a unit test: `Tools/data/room_build.cpp:156` (`Utf8ToCp1252` gives `String` an
    unterminated buffer and `String::SetString` calls `strlen`; `RoomBuild.TextProperties`);
    `Engine/script/runtimescript.cpp:452` (`CreateGlobalVars` reads `_code[_fixups[i]]` unchecked;
    `ScriptBytecode.FixupOutsideTheCodeIsRefused`); `Common/script/cc_reflect.cpp:459` (`JointRTTI::Join`
    compares names through pointers into a string table the loop has reallocated;
    `ScriptVM.CallsFunctionsOfAnotherScript`; upstream's code).
  - UBSan: `Common/util/memory.h:67` (misaligned `int16_t` read), `Common/util/memorystream.cpp:185`
    (`memcpy` from null).
  - LSan, 42 sites: 35 in the script compiler (27 `new` in `SymbolTable::SymbolTable()`; every compiler
    test), 7 in the engine at exit (software renderer, audio driver and core, managed objects).
- Fixed after the run, not re-run: UBSan writes to stderr, not to `log_path`, and ctest hides the output
  of passing tests, so the summary had no UBSan row. The tests now run with `ctest -V` into a file, the
  game step through `tee`, and the summary reads both (47 findings on the downloaded logs, 45 before).
- **1.5b-1 Local runs (2026-11-05), through WSL 2 instead of clang-cl:** Visual Studio's clang-cl 19.1.5
  has ASan, UBSan and `llvm-cov` but no leak detection, and 42 of the 47 findings are leaks; the variant
  was dropped untried once the user enabled Hyper-V.
  - `Script/linux_jobs.sh gcc|sanitizers|coverage` holds the commands of `verify.yml` "Linux (GCC)" and of
    the two `analysis.yml` jobs (a copy: keep in step). `prepush.py` has the stage `linux` (every run) and
    the on-demand stages `sanitizers` and `coverage` (`--only`). On Windows they run in the distro
    `Ubuntu-24.04` (GCC 13.3, clang 18.1, CMake 3.28: the CI's `ubuntu-latest`; `--wsl-distro`): the
    working tree, uncommitted and untracked files included, is committed under `refs/prepush/worktree`
    through a copy of the index and checked out in the distro's `~/ags-draconian`. On Linux they run in
    place.
  - Measured (i7-4790K, 8 threads), clean tree and rerun: `linux` 7 min and 17 s (1031 tests);
    `sanitizers` 18 min and 3 min (CI: 27 min); `coverage` 7.5 min (CI: 14 min), total 53.4%.
  - `sanitizers` reports 49 rows: the 3 memory errors, 42 leak sites and 2 UBSan places above, the
    `memorystream.cpp:185` report twice (argument 1 and 2), and one not in the CI list: mojoAL's `_alcCreateContext`
    (`libsrc/mojoAL/mojoal.c:2163`) calls `SDL_memcpy` with a null pointer. The stage fails on findings,
    which is why it is on demand until 1.5b-5.
  - Checked: an uncommitted `#error` in `Common/util/directory.cpp` fails `linux`; `prepush.py --only
    linux` started inside the distro passes too (the in-place path). WSLg gives the distro a
    display and a sound server, so the first game run opened a fullscreen window on the Windows desktop
    and stopped at test 801 of 2345 when it was closed; the script unsets `DISPLAY`, `WAYLAND_DISPLAY`,
    `XDG_RUNTIME_DIR` and `PULSE_SERVER` (SDL finds Wayland through `XDG_RUNTIME_DIR` alone).
    `sanitizer_summary.py` and `coverage_summary.py` need `--repo` when not run from the checkout: without
    it the leaks collapsed into 4 allocator frames.
  - Not done: Docker runs in the distro and the builder images `ags4-amd64` and `ags4-i386` are public,
    but `debian/build.sh` was not run in them; no Android, MinGW or clang + libc++ header job locally.
    The workflows do not call `linux_jobs.sh`.
- **1.5b-2 Memory errors (2026-11-05):** the three ASan rows are gone
  (`sanitizers` on the final tree, 22 min: 49 rows to 46, the 42 leak sites and 4 UBSan rows, no new row, no
  `ERROR: AddressSanitizer` in the 628 logs, the three named tests and the three new ones pass in the
  instrumented run), each fix has a test.
  - `String(const char *, size_t)` and `SetString(ptr, length)` took `strlen` of the buffer and then the
    smaller number, so they read past `length`; the callers `Utf8ToCp1252`, the text conversion in
    `data_helpers.cpp` (a `std::vector` without a terminator), `file.cpp` (`readlink`, Linux), `DialogTopic`
    (a 150-byte field of game data) and the Editor's template description pass buffers without one. Fixed in
    `String`: it scans with `memchr(cstr, 0, length)` (still stops at a terminator; `NoIndex` keeps
    `strlen`). Cost, measured with a micro-benchmark (g++ -O2, glibc): +1 ns on short texts (2.9 to 3.9 ns),
    equal at 200 characters, and `Left`/`Mid`/`Right` of a long text no longer scan to its end (20000
    characters: 400 ns to 4 ns); the one-argument path is unchanged. Test `String.FromBufferWithLength`.
  - `CreateGlobalVars` read `_code[_fixups[i]]` for GLOBALDATA fixups before `CreateRuntimeCodeFixups`
    checked the position; the check is now `RuntimeScript::CheckFixupInCode`, used by both, same message.
    `FixupOutsideTheCodeIsRefused` passed before only because the second pass reported the error; new
    `FixupBeforeTheCodeIsRefused` (negative position).
  - `JointRTTI::Join` compared names through `Location::name`, quick-ref pointers into `_strings`, which
    `JoinLocation` grows inside the loop; it compares through `name_stri` now. Local to `Join`: no RTTI
    structure or serialized data changed. Test `JointRTTI.FindsExistingLocationAfterTheStringTableGrew`.
  - Checked: `format`, `build` (1034 tests), `tidy`, `linux` (1034 tests), `sanitizers` (above). With
    the fixes reverted and the new tests kept, `sanitizers` gives 49 rows again, each of the three ASan
    rows in 2 logs (the old test and the new one); `JointRTTI.FindsExistingLocationAfterTheStringTableGrew`
    fails there, the other two new tests pass (ASan runs with recovery, so a report does not fail a test).
    Not verified: macOS, iOS, Android, FreeBSD, Debian 10 image; the stages `headers`, `libcxx`,
    `emscripten` and `msbuild` were not run (no new file, `string.h` changed in comments only); the
    micro-benchmark was not run on MSVC.
  - Found by reading, not touched and not reproduced: `FIXUP_DATADATA` in `CreateGlobalVars` reads
    `globaldata[_fixups[i]]` unchecked; `JoinLocation` and `JoinType` start with `if (uid <= size)
    resize(uid + 1)`, which shrinks the table when a "generated" entry that is not the last one is replaced
    (the same in upstream; no test reaches either).
  - Commits, one each: `Common: String(ptr, length) never reads past length (1.5b-2)` (`util/string.*`,
    `test/string_test.cpp`); `Engine: CreateGlobalVars checks the fixup position (1.5b-2)`
    (`script/runtimescript.*`, `test/script_bytecode_test.cpp`); `Common: JointRTTI::Join compares locations
    by name offsets (1.5b-2)` (`script/cc_reflect.cpp`, its test is in `Engine/test/script_vm_test.cpp`).
- **1.5b-3 UBSan (2026-11-05, by a subagent, reviewed by the overseeing session):** the two first-party UBSan rows are gone (`sanitizers` on the final tree, 5 min
  incremental: 11 rows to 8, exactly the 7 engine leaks and the SDL_stdlib.c row; no new row).
  - `Memory::ReadInt16` (and its `Int32`/`Int64` siblings and the three `Write` functions) dereferenced a cast
    pointer, undefined for an address not aligned for the type; UBSan caught the 16-bit read in
    `WFNFont::ReadFromFile` (the glyph table of a WFN font sits at odd offsets in the loaded buffer). Chose the
    whole family: same construction, same defect, a fixed-size `memcpy` keeps the native byte order and the
    `MEMORY_STRICT_ALIGNMENT` and big-endian branches are untouched (the byte-wise branch is now redundant
    next to the `memcpy` one on every target: dropping it would be a separate `[delete]`, not done).
  - `VectorStream::Write` called `memcpy(vec.data() + pos, null, 0)`: `GamePlayState::WriteForSavegame` writes
    `parsed_words` with `WriteArrayOfInt16`, and the vector is empty in every test and in a fresh game, so
    the array is null with a count of 0 (legitimate). `Write` of 0 bytes now returns 0 before the resize and
    the copy; `MemoryStream::Write` and `MemoryStream::Read` got the same `size == 0` guard (they had the same
    memcpy with a possible null buffer).
  - Script VM cost, measured: MSVC `/O2` (what the Release projects use: `MaxSpeed`, intrinsics) gives
    identical disassembly for `scriptexecutor.cpp`, `runtimescriptvalue.cpp`, `wfnfont.cpp` and `lzw.cpp`
    (the objects that changed through the header). `compare_objects.py` (MinSizeRel, `/O1`) reports 618 objects
    identical, 11 different: `memorystream.cpp` (intended), the four above, tests; at `/O1` MSVC does not
    expand `memcpy` and calls it (`ReadInt32` becomes a call). GCC 13 `-O1/-O2/-O3`: byte-identical code for
    all six functions against the dereference (`movzwl/movl/movq (%rdi)`). A Debug engine calls `memcpy` in
    each read; nobody ships or profiles one, so no workaround.
  - Tests: `Memory.ReadWriteMisaligned` (odd offsets 1 to 7, all widths, LE and BE), `Stream.ZeroSizeIo`
    (`Write(nullptr, 0)` on an empty vector, `Read(nullptr, 0)`, `MemoryStream::Write(nullptr, 0)`); they
    pass without the fix too (UBSan recovers), they put the path under the sanitizer. 1040 tests.
  - Not verified: macOS, iOS, Android, FreeBSD, Debian 10 image (the `memcpy` branch is compiled by
    every target but only the ones built locally ran). `sanitizers` ran before a last edit that only added
    braces in `VectorStream::Write`; the other stages ran after it. `MemoryStream::Seek` clamps the position
    to the length, so the early return of a zero-byte `VectorStream::Write` skips no resize.
  - Commits: `Common: Memory read and write functions use memcpy, not a cast pointer (1.5b-3)`, `Common:
    zero-byte reads and writes of memory streams skip memcpy (1.5b-3)`.
  - **Reviewed 2026-11-06**, after the `memcpy` was called slower than the strict-alignment branch. Measured
    on the 28 objects that see `memory.h` and with a micro-benchmark through `ReadInt32LE` and
    `WriteInt32LE` (MSVC 19.44, GCC 13.3, clang 18.1; i7-4790K; ns per call, read / write):

    | Build | cast (before) | `memcpy` (now) | from bytes | unaligned type |
    |---|---|---|---|---|
    | MSVC x64 Debug (`/Od /Ob0 /RTC1`) | 4.0 / 3.9 | 13.6 / 4.8 | 5.8 / 5.6 | 4.0 / 3.9 |
    | MSVC x86 Debug | 7.3 / 4.6 | 13.3 / 7.2 | 8.9 / 6.4 | 7.1 / 4.6 |
    | MSVC x64 MinSizeRel (`/O1`) | 0.9 / 0.4 | 2.1 / 1.8 | 1.0 / 1.0 | 0.9 / 0.4 |
    | MSVC x64 Release (`/O2`) | 0.9 / 0.4 | 0.9 / 0.4 | 0.9 / 1.0 | 0.9 / 0.4 |

    - MSVC Release (x64, `/GL` off to have code to read): all 28 objects are the code of before, as stated
      above. MSVC Debug: 4 objects differ, `ReadInt16/32` go from 6 to 21 instructions and call `memcpy`
      and `_RTC_CheckStackVars` (the local `v`), `WriteInt16/32` from 8 to 12 with the call. The user, the
      same day: Debug builds are internal, never given to users, and do not count. What a running Debug
      game loses was not measured. With `/Oi` MSVC expands the `memcpy` at `/Od` too (a read is 4.4 ns).
    - GCC `-O3` (the flags of the `linux` stage, `-fno-lto` to have code to read): 26 objects are the code
      of before; `scriptexecutor.cpp` is not (`Run` 3154 to 3169 instructions, `PushValueToStack` 61 to 64:
      other registers and block order, no call), nor `wfnfont.cpp` (one compare, operands swapped).
      "Byte-identical" above was the six helpers alone. The speed of the VM was not measured. GCC and
      clang expand the `memcpy` at `-O0` (a read is 5.9 instead of 5.3 ns with GCC: the stack protector
      for `v`). With Ubuntu's `_FORTIFY_SOURCE=3` a `memcpy` into a buffer of known size is a
      `__memcpy_chk` call (0.3 to 3.1 ns in the benchmark's write loop over a global array); the engine
      objects have none.
    - From bytes, as the strict-alignment branch does: MSVC does not merge them (`/O2`: 10 instructions
      for a 32-bit read or write, 24 for a 64-bit read), so it is slower than the `memcpy` in Release (a
      32-bit write 1.0 instead of 0.4 ns), faster in Debug, and slower than the cast in both. The
      branch's `ReadInt64` is wrong when bit 31 is set: `b[3] << 24` is an `int` and sign-extends, so
      `0x80000000` reads as `0xFFFFFFFF80000000`. No build defines `AGS_STRICT_ALIGNMENT` or
      `TEST_STRICT_ALIGNMENT`: nothing compiles the branch, tests included.
    - **Applied the same day, by the user's decision:** the access goes through an unaligned integer type,
      not `memcpy`. `UnalignedInt16/32/64` in `platform/types.h` (C++ only: the header is compiled as C
      too): `__unaligned` with MSVC, the plain type for MSVC x86 (the keyword is error C4235 there and
      the target needs no alignment, as the Windows SDK's `UNALIGNED`), an alias with
      `__attribute__((aligned(1), may_alias))` with GCC and Clang and a `static_assert` on `alignof`. In
      a Windows Release build it is the code the `memcpy` gave; the gain is the code of before 1.5b-3 in
      every build looked at, with nothing left to the optimizer. The strict-alignment branch is untouched.
    - Checked on the final tree: all 28 objects are the code of before a50cf4f7a with MSVC x64 and x86,
      Release and Debug, and with GCC `-O3`; `compare_objects.py` against the tree with the cast pointer:
      638 objects identical; a program using every helper at odd addresses is clean under UBSan with GCC
      13 and clang 18 and gets 6 reports with the cast pointer; `sanitizers`: no findings (unit tests and
      the 2345 tests of the auto-test game); `prepush.py`: the eight stages pass (1106 tests in `linux`;
      `tidy` wanted `using`, not `typedef`). Not verified: macOS, iOS, Android, FreeBSD, the Debian 10
      image (GCC 8), clang-cl. For ARMv7 clang reads 64 bits with two `ldr` where the cast got `ldrd`,
      which needs a word-aligned address (generated code only, not run).
    - Commit: `Common: Memory reads and writes go through unaligned types (1.5b-3)`.
    - The lesson is STYLE.md "Performance" and the rule in AGENTS.md: the builds that count are the
      Release ones.
- **1.5b-4 Script compiler leaks (2026-11-05, by a subagent, reviewed by the overseeing session):** the 35 compiler rows are gone
  (`sanitizers`: 46 rows to 11, exactly the 7 engine leaks and 4 UBSan rows of 1.5b-3 and 1.5b-5; no
  `ERROR: AddressSanitizer`; the 11 rows are in 52 log files, the 46 were in 628). Three causes, no suppression:
  - The 27 rows of `SymbolTable::SymbolTable()`: `new CTF_...` handed to `OperatorCtFunctions`, which stored raw
    pointers in `OperatorDesc`; nobody freed them. The table now owns them (`std::vector<std::unique_ptr<
    CompileTimeFunc>>`, `OperatorCtFunctions` takes `unique_ptr`; the entries' pointers stay, not owning).
    `CompileTimeFunc` had virtual functions and no virtual destructor: it has one now (deleting through the base
    was undefined). The compile time functions hold a reference to their table, so `SymbolTable` is no longer
    copyable (nothing copied it; the implicit copy would have shared functions bound to the original).
  - The rows of `MakeEntryVariable`, `MakeEntryLiteral`, `MakeEntryConstant` and `SymbolTableEntry::operator=`:
    `operator=` overwrote the nine `...D` pointers without deleting the old ones. The parser uses it to restore
    the symbol of an outer scope when a block ends (`RestoreLocalsFromSymtable`), so every local variable's
    description leaked there. It now deletes what the entry owned (copy first, then delete; self-assignment
    returns early).
  - The 3 rows of `Scanner::SymstringToSym`: `new LiteralDesc` over a literal that was already in the table (the
    same number or string scanned twice); it calls `MakeEntryLiteral` now, which keeps the existing one.
  - Raw pointers stay (no `unique_ptr` members): the parser steals `FunctionD` into a `unique_ptr` and the
    `...D` pointers are passed to functions in many places; the entry already owned them, only the assignment
    broke the rule.
  - Tests (`Compiler/test/cc_symboltable_test.cpp`): `EntryCopyOwnsItsDescriptions`,
    `EntryAssignReplacesOwnedDescriptions`, `EntryAssignSelfKeepsDescriptions`,
    `OperatorCompileTimeFuncsAreOwnedByTable` (4 new, 1038 in all). The leak itself shows only in `sanitizers`.
  - Checked: `format`, `build` (1038 tests), `tidy` (it asked for braces and for unnamed unused lambda
    parameters on the changed lines), `headers`, `libcxx`, `msbuild` (the Editor links the compiler), `linux`
    (GCC 13, warnings as errors, 1038 tests), `sanitizers` (above, unit tests and the auto-test game ok).
    `compare_builds.py` on the auto-test game (test games repository, branch `ags4-draconian`), the Editor from
    before the change against `agsbuild` with the change: 59 identical, PARITY. The bytecode of all compiler tests
    is compared by the tests themselves.
  - Not verified: macOS, iOS, Android, FreeBSD, Debian 10 image (the change is plain C++17: `make_unique`,
    `unique_ptr`); `emscripten` was not run; no private game corpus.
  - Seen, not touched: `SymbolTableEntry` has a copy constructor and no move, so `entries` growth copies
    every description (deep) instead of moving them; a move constructor would speed up the compiler.
    `cc_symboltable.cpp` assigns `entry.VartypeD = new ...` (and `OperatorD`, `DelimeterD`) in the `Add...` and
    `VartypeWith...` functions without a check that none exists; they only run on fresh entries today.
  - Commits, one per cause: `Compiler: SymbolTableEntry assignment deletes what it owned (1.5b-4)`,
    `Compiler: the symbol table owns its compile time functions (1.5b-4)`, `Compiler: the scanner reuses an
    existing literal description (1.5b-4)`. The stages ran on the final tree; the two intermediate commits
    were not built on their own.
- **1.5b-5 Engine leaks at exit (2026-11-06, by a subagent, reviewed by the overseeing session):** `sanitizers` is green
  locally: "No sanitizer findings", exit 0 (8 rows in 52 log files before; after, 6 log files, 1041 unit tests,
  the auto-test game 2345 passed; no `ERROR: AddressSanitizer`, no `runtime error`; 92 s incremental). The rows
  by cause:
  - The 4 managed-object rows. `cc_serializer.cpp:91` is the engine (the auto-test game restores a save):
    `AGSDeSerializer::Unserialize` did `new ScriptUserObject()` for every restored script struct and made that
    object the pool's manager, but the manager is stateless and the created structs use `globalDynamicStruct`;
    the 40-byte objects were never freed. Fixed: `globalDynamicStruct.Unserialize(...)`, as the dynamic array case
    next to it does. The other three (`CCDynamicArray::CreateImpl`, `ScriptString::CreateBuffer`,
    `ScriptUserObject::Create`) are the test's own fault: `ScriptVM` tests (`engine_full_test`) leave objects in
    the pool; its `TearDown` calls `pool.Reset()` now. Not one cause, as the task guessed: two.
  - `audio_core.cpp:82` (80 bytes, mojoAL's `ALCdevice`), the one run that plays sound: `alcCloseDevice` refuses
    to close a device while AL buffers exist, and `OpenAlSource` keeps the buffers it generated in a free list
    (`g_oalint.freeBuffers`) for reuse and never deleted them. Fixed: `OpenAlSource::ReleaseBuffers()`, called by
    `audio_core_shutdown` after the slots are destroyed and with the context still current (`alDeleteBuffers`
    needs it). It also stops a second `audio_core_init` in the same process from reusing buffer names of the
    closed device.
  - `ali3dsw.cpp:146` and `sys_main.cpp:194`: third party, suppressed (`ci/sanitizers/lsan.supp`, the stacks and
    reasons are in the file). A plain SDL program built against the same SDL (window and OpenGL renderer, a texture
    copy, audio device opened and closed, destroyed, `SDL_Quit`) leaks the same blocks. `SDL_EGL_InitializeOffscreen`:
    Mesa's EGL display (2680 bytes) on SDL's offscreen video driver, which SDL picks without a display. `ALSA_OpenDevice`:
    libasound's configuration (2049 blocks, 66666 bytes) after `snd_pcm_open`. With the display hidden, two
    56-byte blocks from the first draw call of that renderer (Mesa, they point to each other, so LSan reports them
    as indirect only) showed as a new row: `GL_RunCommandQueue`. The engine destroys the renderer and the window and
    calls `SDL_Quit`, as the plain program does.
  - The UBSan row (`SDL_stdlib.c:527`, mojoAL's `_alcCreateContext` copies a null attribute list of size 0 with
    `SDL_memcpy`; `alcCreateContext(device, nullptr)` is legal): `ci/sanitizers/ubsan.supp`, `nonnull-attribute:SDL_stdlib.c`
    (UBSan matches the file of the failing place, nothing narrower exists). GCC 13's UBSan run-time honours the
    file: measured with a call to `SDL_memcpy(buf, NULL, 0)`, the report is gone, and a suppression for another file
    leaves it.
  - Found on the way, not in the table: **an `agscc` leak that the summary could not show.** 43 of the 52 logs were
    `agscc` runs of the auto-test build whose leaked blocks (the whole compiled script) only point to each other, so
    LSan reports them as indirect leaks without any direct one, and `sanitizer_summary.py` read only direct leaks. They
    were there before 1.5b-4 too (that task's 46 to 11 counts rows, not these). Cause: `Compile()` in
    `Compiler/compiler.cpp` never deleted the `ccScript` that `ccCompileText` returned. Fixed with a
    `std::unique_ptr`; and the summary now reports a log with indirect leaks and no direct one as one finding, named by
    the outermost frame in our code (the 43 logs showed as "under main at Compiler/main.cpp:324"; the GL pair above as
    "under ags_entry_point"). Other tools of `agsbuild` ran the same way and showed no such logs.
  - Wiring: `Script/linux_jobs.sh` sets `LSAN_OPTIONS`/`UBSAN_OPTIONS` `suppressions=` (the path in the checkout);
    `.github/workflows/analysis.yml` the same two variables with `${{github.workspace}}/ci/sanitizers/...`
    (**not run**: the workflow is on demand and only the user pushes). LSan prints "Suppressions used" at the end of
    each log, and every entry is used (`ALSA_OpenDevice` 2049, `SDL_EGL_InitializeOffscreen` 1,
    `GL_RunCommandQueue` 2). Needed: the auto-test game rerun with one entry removed at a time returns exactly its
    row (no LSan file: the `SetDisplayMode` and `try_init_audio_driver` rows; without `GL_RunCommandQueue`: the
    indirect-only row; without `ALSA_OpenDevice`: `try_init_audio_driver`; without the EGL entry: `SetDisplayMode`;
    without the UBSan file: `SDL_stdlib.c:527`).
  - Shutdown order read, not changed: `quit()` runs `video_shutdown`, `shutdown_sound` (`audio_core_shutdown`),
    `unload_game` (`ccUnregisterAllObjects`), `engine_shutdown_gfxmode`, `sys_main_shutdown`. The change in
    `audio_core_shutdown` is between "slots destroyed" and "context destroyed", inside the audio core, which is
    driver independent (the graphics drivers are not involved: Direct3D 9/11, OpenGL and GLES2 do not touch the AL
    buffers). The restore fix is the pool's own data, on every platform.
  - Tests: `ManagedPool.RestoredStructKeepsItsDataAndTheSharedManager` (a struct through `ccSerializeAllObjects`,
    `ccUnregisterAllObjects`, `ccUnserializeAllObjects`: data and `globalDynamicStruct` as manager; fails without
    the fix); 1041 tests. The audio and compiler fixes show only in `sanitizers`.
  - Checked: `sanitizers` (above; a first run with the GL entry missing failed on exactly that row), `format`, `build`
    (MSVC, 1041 tests), `tidy` (asked for braces in `ReleaseBuffers`, done), `headers`, `linux` (GCC 13, warnings as
    errors, 1041 tests). The Windows engine from the `build` stage ran the auto-test project (a local export, 2414
    tests, 0 failed) and exited with code 0 after "AudioCore: shutdown"; `tappy` was not on the PATH of that shell,
    so the TAP verdict itself was not run.
  - Not verified: macOS, iOS, Android, FreeBSD, Debian 10 image; `analysis.yml` was not run (on the CI runner the
    SDL libraries differ from the distro's: a new third-party row there goes into the suppression files with its
    stack); Direct3D and OpenGL drivers at exit (no run exercised them; the change does not touch them); a game with
    video playback at exit.
  - Seen, not touched: a plain SDL video program also leaks `SDL_DBus_Init` blocks (SDL's D-Bus support) in this
    distro; the engine's logs have none.
  - Decided (user, 2026-11-06): `sanitizers` stays on demand in `prepush.py` and on CI: it is slow and rarely
    gives something to act on.
  - Commits, one per cause: `Engine: restored script structs use the shared manager (1.5b-5)`, `Engine: the script VM
    tests remove the managed objects they leave (1.5b-5)`, `Engine: the audio core deletes its AL buffers before it
    closes the device (1.5b-5)`, `Compiler: agscc deletes the script it compiled (1.5b-5)`, `Script: the sanitizer
    summary reports leaks that only point to each other (1.5b-5)`, `CI: suppressions for the third-party sanitizer
    reports (1.5b-5)`.
- **1.5b confirmed on CI (2026-11-06):** pushed after a full `prepush.py` run (8 stages, 1041 tests); `gh workflow
  run analysis.yml -R AlanDrake/ags-draconian --ref ags4draconianr21 -f coverage=false`, run 42174071706, 21 minutes:
  "No sanitizer findings (5 log files)", the auto-test game 2414 passed, 0 failed. The suppression files as
  written for the local distro were enough on the runner: no new third-party row. Verify and CI of the same
  push are green. Seen, not looked into: the game reports 2414 tests on CI and 2345 in the local distro, from
  the same tag of the test project, 0 failed in both.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 1.6 Mechanical-change verifier

*Definition, moved from PLAN.md on 2026-11-04:*

**1.6 Mechanical-change verifier.** `Script/compare_objects.py`: build Release without debug
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

### 1.7 Build the auto-test game in CI, from source

*Definition, moved from PLAN.md on 2026-11-04:*

**1.7 Build the auto-test game in CI, from source.** Done as part of 7.1j: once `agsbuild`
builds AGS 4 projects, a Linux job compiles `AlanDrake/ags-test-games` (pinned commit) with the tools
built in the same run and hands the result to the platform jobs. That replaces the hand-made
releases from 1.1, keeps the test game in step with this commit's compiler, and doesn't tie every
platform job to the Windows editor build (the earlier plan: compile with the CI-built editor).
Until then, keep publishing `draconian-auto-test-N` releases by hand.
*Done when:* the platform jobs run a test game compiled by tools of the same CI run.

*Status (2026-11-02):* done, green in CI (run 41442221520): the `Auto-test game` job builds it with
`agsbuild --strict` and every platform job runs it. Hand-made releases are no longer needed.

### 2.1 CI format gate

*Definition, moved from PLAN.md on 2026-11-04:*

**2.1 CI format gate.** A job that runs pinned `clang-format 19` via `git clang-format --diff`
against the PR base and fails if anything changes. Changed lines only, from day one.
*Done when:* a PR with a badly formatted line fails CI.

### 2.2 One-time reformat, module by module

*Phase 2's introduction, moved from PLAN.md on 2026-11-04:*

The configs are already in the repo: `/.clang-format`, `/.clang-tidy`, `/.editorconfig`, plus
`DisableFormat` stubs in the third-party folders. Measured clang-format 19 churn is about 15% of
lines (BASELINE.md). Most of it is real fixes: K&R braces → Allman, `if (x) y;` split onto two
lines, tab-indented code.

*Definition, moved from PLAN.md on 2026-11-04:*

**2.2 One-time reformat, module by module.** Commits like `Common: [format] whole module`,
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

### 2.3 Mechanical modernisation

*Definition, moved from PLAN.md on 2026-11-04:*

**2.3 Mechanical modernisation** (each item is its own commit, using the clang-tidy `--fix`
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
- Moving these checks to "whole tree" in CI is task 2.4 (needs the Linux compile database). *Superseded:*
  2.4 gates changed lines only; the whole tree stays as it is until a module is cleaned up.

### 2.4 clang-tidy gate

*Definition, moved from PLAN.md on 2026-11-05:* `compile_commands.json` in the Linux CI job and
`clang-tidy-diff.py` on the PR diff with the root `.clang-tidy`: naming and function-size rules for new
code only. *Done when:* a PR that adds a `chaa`-style name or a 300-line function is flagged.

*Status (2026-11-05):* done; confirmed on CI by a probe branch.
- **`Script/tidy_diff.py`** stands in for `clang-tidy-diff.py` (not in the pip `clang-tidy==19.1.0` wheel).
  `git diff -U0 <base>` against the working tree gives the changed lines, passed as one `--line-filter`;
  the whole root `.clang-tidy` runs. Checked: first-party `.cpp` in the compile database, and headers
  unless their `.cpp` is in the diff; never `libsrc/`, `libinclude/`, `_deps/`. Exit 0 clean, 1 findings,
  2 could not analyse. Size and naming findings sit on the declaration line, so an edit inside an old
  long function is not flagged.
- **Banned abbreviations:** the naming check knows case styles only (a new `int chaa` passed). `.clang-tidy`
  sets `readability-identifier-length` to 99 (not in `Checks`); the script turns it on to learn every
  variable and parameter declared on a changed line and reports those in `CRYPTIC_WATCHLIST`
  (`refactor_metrics.py`) as `ags-cryptic-name`. A line that only uses a legacy name passes. Functions
  and data members are not covered.
- **Runs** as a step of `verify.yml`'s Linux job (base: the push's `before`, else the merge base with
  `origin/ags4draconianr21`; `fetch-depth: 0`) and as `prepush.py` stage `tidy` (base: merge base with
  upstream).
- **Verified:** locally a parameter `objj`, a local `chaa`, a loop counter `xx`, wrong case styles and a
  306-line function are flagged; clean code in a file with 34 old findings and a new line using a legacy
  `chaa` pass. On CI (runs 42032914043 green, 42034663373 red with the 7 planted findings): same output,
  both base choices, 5 to 6 s for one or two files. Local: 31 s per `.cpp` (MSVC database), 74 s for
  30 files.
- **Found:** on Windows the `--line-filter` name needs backslashes (with `/` nothing matched and the gate
  passed everything). The fork's last 60 commits hold 36 findings (braces, C-style casts, test names),
  reported when those lines are next touched.
- **Not verified:** a large diff on the runner.

### 2.5 Header guards → `#pragma once`

*Definition, moved from PLAN.md on 2026-11-04:*

**2.5 Header guards → `#pragma once`** (372 headers use `__AGS_...` guards, which are reserved
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

### 2.6 Self-contained headers, then sorted includes

*Definition, moved from PLAN.md on 2026-11-04:*

**2.6 Self-contained headers, then sorted includes.** Add a CMake target that compiles each
first-party header on its own (`header_check_<name>.cpp` that includes only that header). Fix
failures with includes or forward declarations, then set `SortIncludes: CaseSensitive` with
`IncludeCategories` matching STYLE.md and reformat once more.
*Done when:* the header-check target builds and include order is enforced. This also helps
compile times: `character.cpp` includes 57 headers and `game.cpp` 74.

*Status (2026-11-02):* done, in two commits.
- **Self-contained headers.** `CMake/HeaderCheck.cmake` adds the `header_check` target
  (`-DAGS_HEADER_CHECK=1`): one generated source per header of `Common/`, `Engine/`, `Tools/` and
  `Compiler/` that includes only that header, with the flags of the owning target; platform
  headers are checked on their platform only. 28 of 366 headers failed on MSVC (missing includes
  or forward declarations: `Stream`, `String`, `Rect`, the gfx filter classes in `ali3d*.h`,
  `SavegameDescription`, ...); all fixed. Left out on purpose: `Compiler/script/` (the legacy compiler,
  gone in 3.3) and `util/library_*.h` (parts of `library.h`, chosen by platform). CI runs the target on
  the Windows (MSVC) and Ubuntu (GCC) jobs; it fails when a header is broken (tried).
- **Sorted includes.** `.clang-format` now has `SortIncludes: CaseSensitive`, `IncludeBlocks: Regroup`
  and four categories: own header, `platform/windows/windows.h` (must precede other Windows
  headers: `<shlwapi.h>` and `<Psapi.h>` broke without it), C/C++ standard, other `<...>`, then
  `"..."`. One mechanical commit over 570 files. Measured: the whole tree builds on MSVC, 806 unit
  tests pass, and `compare_objects.py` finds identical code in every non-test object (354 byte-identical;
  229 differ only in the debug-info section or function order, which include order changes; the
  21 test objects differ as always). Not built here: the Linux/macOS/Android/Emscripten-only sources,
  where CI is the test. One manual edit: `AGSSpriteFont.cpp` included `<WinBase.h>` before
  `<windows.h>`, which sorting reversed; `windows.h` includes it anyway.
- `Script/compare_objects.py` learned to tell "bytes differ" from "code differs" (needs `dumpbin`).
- **What CI found after the push** (fixed in three follow-up commits), and why the local MSVC check
  missed it: `Compiler/fmem.h` used `size_t` with no include (MSVC declares `size_t` implicitly);
  the legacy `Compiler/script/cc_symboldef.h` and `cc_variablesymlist.h` lacked `<string>`,
  `<vector>`, `<cstdint>` (only the MSBuild `Compiler.Lib.sln` builds them, not the CMake build that
  was checked); `delegate.h` and `indexedobjectpool.h` used `assert`, `std::find_if`, `uint32_t` and
  `SIZE_MAX` without their headers (GCC's libstdc++ pulls in less than MSVC's library, and checks
  template bodies). The Direct3D filter headers (`gfxfilter_d3d*.h`, which `#error` off Windows) are
  now skipped outside Windows. Final count: MSVC 366/366, GCC 350/350 (run 41457809129 plus
  `indexedobjectpool.h`), clang with libc++ 350/350.
- Lesson for repo-wide changes: build the MSBuild solutions too, and check headers with a strict
  standard library before pushing. Locally, emsdk's clang with libc++ (POSIX target,
  `-D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES`) reproduced GCC's findings; MSVC, even with
  `/permissive-`, did not.

### 2.7 Line endings

*Definition, moved from PLAN.md on 2026-11-04:*

**2.7 Line endings.** Renormalise with the `.gitattributes` from 0.5 (`git add --renormalize .`)
in one commit, listed in `.git-blame-ignore-revs`.

*Status (2026-11-01):* done: 457 files (mostly C# editor sources), content identical ignoring CR.

### 2.8 CI shape and the pre-push check

*Added 2026-11-04 (not in the original plan): "CI should confirm, not discover" (2.6 needed four pushes).*

*Status (2026-11-04):* done and pushed; the second run (`7afe0be15`) is green in all four workflows,
with 340 header stubs compiled by clang + libc++ 21 on Ubuntu and the heavy jobs skipped.
- **Workflows renamed:** `build.yml` → `ci.yml` ("CI"), `format.yml` → `verify.yml` ("Verify"),
  `makefile-build.yml` → `make.yml`, `build-linux-docker.yml` → `linux-docker.yml`. Cost: an upstream
  change to `build.yml` no longer cherry-picks onto it; port it by hand.
- **Split:** `verify.yml` runs on every push (clang-format; Linux GCC build + `ctest` + header check;
  clang + libc++ header check with `-D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES`). `ci.yml` (full) and
  `make.yml` run on pushes to `ags4draconian*` branches (the old `draconian` branch is unused), `v*` tags (`ci.yml` only), pull requests and on demand. The
  GCC header check moved from `ci.yml` to `verify.yml`; MSVC's stays in `ci.yml`.
- **Full or light (`ci.yml`):** a first `plan` job outputs `full` = tag push, or `schedule` (Mondays
  04:00 UTC), or `workflow_dispatch` with input `full` true (default): `gh workflow run ci.yml --ref
  <branch> -f full=true`. Other runs build only the light set: the Windows MSVC jobs, the auto-test
  game, and the `build` matrix reduced to Windows and Ubuntu-x86_64 (the plan job emits the matrix as
  JSON; `fromJSON` in the strategy). Heavy jobs (Emscripten, Android, Linux binaries and bundle, macOS
  and Linux make tools, PDB, tools and Windows packaging, release) have `if: needs.plan.outputs.full ==
  'true'`; their dependants skip by themselves. The mingw and Arm64 `build` entries are heavy.
  **macOS is its own workflow, `macos.yml`** (decided 2026-11-04, after a day as a `-f macos=true`
  opt-in in `ci.yml`): hosted macOS runners are often unavailable for long stretches (the 2026-11-04
  13:45 run waited 18 min for one before a newer run replaced it), and inside `ci.yml` a waiting macOS
  entry held up the parity job, which needs the whole `build` matrix. `macos.yml` runs on `v*` tags,
  Mondays 05:00 UTC and on demand, with its own auto-test-game job, the macOS engine build and
  auto-test (step for step the former matrix entry) and the macOS tools archive; `cancel-in-progress`
  lets next week's run replace one still queued. Nothing in `ci.yml` consumes its artifacts, so a
  release completes without it. The `schedule` runs on the default branch, which is
  `ags4draconianr21`, so the weekly runs work as is.
- **`concurrency`** per workflow, event and ref cancels a superseded run (never for a release tag);
  the event in the group keeps a manual or weekly full run alive through the pushes that follow it.
  **`paths-ignore`** `docs/**`, `**.md`, `.git-blame-ignore-revs`, issue templates (nothing in the
  builds reads them; tag pushes are not filtered by GitHub).
- **`Script/prepush.py`** stages `format`, `build` (CMake/Ninja/MSVC + `ctest`), `headers` (MSVC),
  `libcxx` (`Script/header_check_libcxx.py`, emsdk clang + libc++, 340 headers), `msbuild` (the six CI
  solutions, x86 Release, test runners and the editor's NUnit, into `.tidy/prepush/msb`). A missing tool
  is `NOT RUN` and fails the script. Measured here: format 1988 files 14 s, build + ctest 327 s (cold),
  headers 36 s, libcxx 54 s, msbuild 293 s, all pass; the libcxx stage failed on a planted header
  without `<string>`.
- **Not verified:** the YAML parses, but the Linux jobs, including the new clang job (apt packages,
  configure with libc++), were written without a Linux machine. Debug configurations of the solutions and
  macOS/Android/Emscripten are not in the script.
- **Old workflow registrations** disabled with `gh workflow disable` on 2026-11-04, before the first
  push (otherwise the next push would start the old files' runs too): 368431699 (Build Linux Docker
  Image), 68872654 (GitHub Actions Build), 404365706 (Format check), 368431697 (Makefile Build).
  CodeQL (93435167) is left alone (see 0.4).
- **First run (12:41 UTC, 2026-11-04):** every job passed except `Headers (clang, libc++)`. It set
  the libc++ flags for the whole build, so `common` failed to compile (Ubuntu's libc++ 21 is stricter
  than emsdk 3.1.74's: `file.h` used `time_t`, `string.cpp` `atoi`, without their headers) and no header
  stub ran. Fix: `AGS_HEADER_CHECK_FLAGS` applies the flags to the stubs only; the missing includes were
  added to `file.h`, `directory.h`, `assetmanager.h`, `scriptdatetime.h`, `sdldecoder.h`,
  `graphicspace.h`, `scripteventtable.h`, `transformstream.h`, `movelist.h`, `string.cpp`, found with a
  text scan for C library names used without their header. Not verifiable without libc++ 21 on glibc.
- **First push:** expect problems in the Linux and clang jobs; read all of them, fix in one batch, push once.
- **Stage `emscripten` (2026-11-05):** 7 of our 13 CI breaks were code MSVC accepts and others do not.
  The stage builds the engine with emcc as `Emscripten/build.sh` does (`.tidy/prepush/ems`), which covers
  the GLES2 renderer and the generic non-Windows branches, then syntax-checks the sources with Linux
  branches (found through `ninja -t deps`; 4 files today) with `-fsyntax-only -U__EMSCRIPTEN__
  -D__linux__`, since that build uses `library_dummy.h` and compiles `acpllnx.cpp` to nothing. Sources
  reaching `ogl_headers.h` are skipped (desktop GL headers). Measured: 73 s configure + 234 s build cold,
  2 s unchanged, about 80 s after an engine change (LTO link), 7 s syntax check. Verified by restoring
  two old breaks: `glColor4f` in GLES2 code (0.4) and an unqualified `Path` in `library_posix.h` (2.5)
  both fail. Not covered: macOS, iOS, Android, FreeBSD code; linking for Linux (the 7.1o and 3.2
  breaks); GCC; the tools.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

- [x] **2.8 CI shape and the pre-push check** (2026-11-04, green on CI after two pushes: see LOG.md):
  workflows renamed (`ci.yml`, `verify.yml`, `make.yml`, `linux-docker.yml`), concurrency, `paths-ignore`,
  `verify.yml` (fast gate on every push) split from `ci.yml` (full build), clang+libc++ header job,
  `Script/prepush.py`.

### 2.9 CI and pre-push time

*Added 2026-11-06 (not in the original plan): the user named testing and CI as the bottleneck.*

*Status (2026-11-07):* 2.9a–c are on the main branch: the eight commits of `probe-2.9`, green in Verify, a light
and a full run there ("The probe" below), fast-forwarded onto `ags4draconianr21`; the branch and its caches are
deleted. LTO stays in the verification builds (the user, 2026-11-07; 2.9e below). The "done when" is not reached: a push
should now be confirmed in about 12.5 minutes (estimated from the job times; the light run on a push has
not happened yet), not 10, and the time left is the link-time code generation (2.9e).

- **Where the time went, before the change.** Sources: the light CI run 42281919227 and the Verify run
  42281919108 of 2026-11-06, the logs of that day in `.tidy/prepush`, the stage times above (2.8).
  - The tests are not it: `ctest` runs 1106 tests in 8 s, the auto-test game takes about a minute per
    renderer on a runner.
  - A push costs Verify 8 min and light CI 18 min of waiting, about 70 runner-minutes. The longest
    chain: Auto-test game 81 s, then Windows 907 s (configure 176, build 466, header check 65, tests 31,
    auto-test 132), then Parity 100 s. Ubuntu-x86_64 659 s (packages 79, configure 83, build 376);
    Verify's GCC job 472 s (build 244); MSBuild Tests 456 s, Tools 418 s, Engine 474 s, Editor 212 s.
  - Nothing was cached but the downloaded sources. 403 of the 945 compile steps of the CMake build
    (header stubs not counted) are SDL2, SDL_sound, freetype, allegro, vorbis, theora and gtest.
  - The engine was compiled 8 times per push: Verify GCC, Ubuntu-x86_64 (the same compiler and
    options), Windows CMake, MSBuild x86 Debug, x86 Release, x64 Release, the test runners x86 and x64.
  - Locally a full `prepush.py` run after a change to a Common header is 20 to 25 minutes (the sum of
    the stage times, not one timed run), and `sanitizers` 13 minutes. 19 different Common headers
    changed in the three days before.
- **2.9a, the compiler cache.** ccache, through the environment variables
  `CMAKE_C_COMPILER_LAUNCHER` and `CMAKE_CXX_COMPILER_LAUNCHER` (CMake 3.17+), so the configure
  commands stay as they are and the downloaded dependencies are cached too. The cache directory
  (`.ccache` in the workspace, at most 500 MB) is kept by `actions/cache` under a key with the commit
  hash and restored by prefix: a cache is never overwritten, so each commit saves its own and GitHub
  drops the oldest when the repository's 10 GB are used. A step prints the statistics, with the reason
  for whatever could not be cached, counted from the build step on (the configure step's compiler
  checks live in scratch directories with random names: some 200 calls per job that never hit). Jobs:
  Verify's GCC and clang header jobs, and in `ci.yml` the five `build` platforms (ccache from apt,
  from MSYS2, and for MSVC the release archive 4.14.2 checked against the SHA-256 that GitHub publishes
  for it). Left out: the auto-test game job (tried: 90 s with the cache, 81 s without; its 120 objects
  come back in seconds, installing ccache costs as much, the rest is configure and eleven links),
  Emscripten, Android, the Linux image jobs, `macos.yml`, `analysis.yml`.
  - **The Windows CMake job builds with Ninja**, as `prepush.py` always did: the Visual Studio
    generators take no compiler launcher. A step writes what `vcvars64.bat` changes into the job's
    environment, with `CC` and `CXX` set to `cl` (CMake would otherwise take the first compiler on
    `PATH`, a MinGW gcc on the runner). The binaries are in `build/`, no longer in `build/Release/`.
  - **`/MP` only for the Visual Studio generators** (root `CMakeLists.txt`): Ninja hands one source
    to each compiler call, where the option does nothing, and a cache has to treat it somehow. SDL2's
    own `CMakeLists.txt` still adds it to its 196 sources (third-party, left alone).
  - **ccache with MSVC works** with `/GL`, `/Fd` and `/FS`, which the manual does not mention and
    which could not be tried locally (downloading and running the binary was not permitted in the
    session): 1276 of the 1318 compiler calls of the cold run were cacheable (37 "unsupported compiler
    option", 5 "unsupported source encoding", in the dependencies), and the warm run hit 1275 of 1276.
    One MSVC build is 0.41 GB of cache (the `/GL` objects), a GCC build 0.13 GB: the `build` jobs'
    limit is 1 GB.
  - `compile_commands.json` now starts each command with `ccache`. clang's tooling drops known
    wrappers and `tidy_diff.py` reads only the file names: checked in WSL with clang-tidy 19.1.0 on the
    GCC tree's database, with and without the prefix (`Common/util/string.cpp`, 260 diagnostics and no
    error both ways). The probe does not show it: its diff has no C++ line.
  - The key of the `build` jobs' FetchContent cache has a version now (`fetchcontent-2-`): the cache
    holds the dependencies' build trees as well, and one that the Visual Studio generator configured
    stops a Ninja configure ("does not match the generator used previously").
- **2.9b, the light run.** The MSBuild jobs build x86 Release only: it is what the editor ships with,
  what the parity job compares and what `prepush.py`'s `msbuild` stage builds, so a light run confirms
  exactly what was verified locally. The Debug builds (Engine, Tools, Compiler, Editor with its NUnit
  run) and everything x64 are steps with `if: needs.plan.outputs.full == 'true'`. Cost: an error only
  a Debug or an x64 MSBuild build has is found by the weekly or a manual full run, not by the push.
  On a push the `build` matrix is Windows alone: Verify's "Linux (GCC)" job is the same build as
  Ubuntu-x86_64, so it now builds the auto-test game with its own tools and runs it with both
  renderers, as that entry did. Pull requests and manual light runs keep Ubuntu-x86_64 (`"verify":
  true` in the plan job's list; the filter is `map(select(.light and (.verify | not)))`).
  `Script/linux_jobs.sh gcc` does not run the game: it would add two minutes to every local run.
- **2.9c, `prepush.py --auto`.** The paths that differ between the merge base with the upstream branch
  and the working tree (`--base` for another revision) are matched against `AUTO_RULES`; the first
  rule decides, and a path no rule knows needs every stage. The lists rest on what each build reads:
  `ninja -t deps` of the Emscripten tree has 0 files under `Compiler/` and `Tools/` (54 under
  `Plugins/`, which the engine builds in); MSBuild reads no CMake file; only MSBuild builds the
  editor; the compiler's and the tools' tests read `agsdefns.sh`. `format` is added for a C or C++
  file and `tidy` for one `tidy_diff.py` checks, unless the file is deleted. Changed files that no
  stage verifies are listed with what does (workflows: actionlint and a probe; packaging: a full
  run). `--dry-run` prints the choice and stops. Without `--auto` nothing changed.
- **Checked locally:** `actionlint` on both workflows; the platform list is valid JSON and selects
  Windows on a push, Windows and Ubuntu-x86_64 on other light runs, all five on a full run (the three
  filters restated in Python: there is no `jq` on this machine); the MSVC environment step in
  PowerShell 5.1 writes 46 variables, with `cl` and `ninja` on the new `Path`; `auto_plan` on 34 paths,
  one per rule and kind of file, and on a mixed change. The `/MP` change: `prepush.py --only
  build,headers` on the branch passes (a new tree: 492 s with the configure step and the downloads,
  1106 tests, header check 48 s; our targets compile without `/MP`). The other stages were not run:
  the change is inside `if(MSVC)`, and MSBuild does not read it.
- **Not verified:** everything the workflows do on a runner. The first push to the main branch is also
  the first run of the push filter (the plan job prints the matrix it chose).
- **The probe.** `ci.yml` does not run on a branch outside `ags4draconian*`, but it can be started
  there by hand, so no probe workflow is needed:

  ```sh
  git push origin probe-2.9        # Verify runs by itself: cold caches
  gh workflow run ci.yml -R AlanDrake/ags-draconian --ref probe-2.9 -f full=false
  # when both are green, again for the warm numbers, then the full set once
  gh workflow run verify.yml -R AlanDrake/ags-draconian --ref probe-2.9
  gh workflow run ci.yml -R AlanDrake/ags-draconian --ref probe-2.9 -f full=false
  gh workflow run ci.yml -R AlanDrake/ags-draconian --ref probe-2.9 -f full=true
  ```

  The caches a probe branch saves are visible to that branch only (`gh cache list`, `gh cache delete`).
- **Probe results (2026-11-06, 22:00 to 23:10 UTC).**
  - *Cold* (d0420e98b): Verify 42301637833 green, GCC job 848 s with 192 s of apt (a slow mirror that
    evening). Light run 42301639300: Windows green on Ninja and ccache (801 s), the MSBuild jobs
    146 to 183 s; Ubuntu-x86_64 was cancelled at its 20-minute limit inside `apt-get install`, the
    mirror serving at about 40 KB/s, before any step of ours, so the parity job was skipped.
  - *Warm* (ac4ab0650, only `ci.yml` changed): Verify 42304899184, 6.9 min: GCC 408 s (configure 87,
    build 159, header check under 20, the game 86), clang headers 151 s (was 289). Light run
    42304898565, 16 min only because Ubuntu-x86_64 was cold (766 s): Windows 569 s (configure 88,
    build 271, header check 4, tests 30, game 132; was 907), MSBuild Engine 156 (was 474), Tests 190
    (456), Tools 104 (418), Editor 134 (212), Parity 101.
  - *Full* (8a17b00d0): 42307397759, all 26 jobs green in 20.7 min (Android 689 s is the longest);
    Win-mingw-x86_64 1068 s and Win-mingw-arm64 1006 s cold with 923 cacheable calls each,
    Ubuntu-Arm64 634 s cold (its auto-test passed this time), Ubuntu-x86_64 warm 482 s (build 204 s,
    672 of 673 hits; was 659), Windows 571 s, the Debug and x64 MSBuild steps ran. Verify 42307398238
    7.3 min.
  - *A push to the main branch*, estimated from these jobs: game 69 to 90 s, Windows 570 s, Parity
    100 s, about 12.5 min instead of 18, and about 22 runner-minutes instead of 56; Verify 7 min instead
    of 8 with the game added. The warm case is the best case: a commit that changes a Common header
    recompiles what includes it.
  - **What is left is linking.** Warm Windows build step, from the log's timestamps: the 924 objects
    are back in 15 s, then the 40 links take 255 s (`tools_test` 66, `ags` 36, `agf2dlgasc` 33,
    `crmpak` 32, `engine_test` 25): `/GL` moves the code generation into every link. GCC the same with
    `-flto`: `ags` 112 s, `engine_full_test` 109, `tools_test` 81, `common_test` 67, in parallel from
    +79 to +236 s. The test targets set `INTERPROCEDURAL_OPTIMIZATION FALSE`, but they link libraries
    whose objects are LTO objects. That is 2.9e, a decision: the binaries of these jobs are not
    shipped, so LTO off there (an option, on by default, off in Verify, the light CMake jobs and the
    `build` and `linux` stages of `prepush.py`) would take most of the 270 and 159 s away, at the price
    that an LTO-only error waits for a full run. Decided 2026-11-07 (the user): LTO stays.
  - Also seen: the apt step varies between 31 and 192 s and once ran 20 minutes; a package cache is
    a possible follow-up.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 3.1 Dead platforms

*Definition, moved from PLAN.md on 2026-11-04:*

**3.1 Dead platforms** (D2, D6): `PSP/`, `Engine/Makefile.psp`, plugin `Makefile.psp` and
`psp.c`, `Engine/util/library_psp.h`, the 8 `AGS_PLATFORM_OS_PSP` branches, pre-Windows 7 code
paths and toolsets. iOS and FreeBSD stay. Also: delete the superseded `ags4-framescript` branch
(upstream has `ViewFrame.EventName`/`OnFrameEvent`).
*Done when:* `AGS_PLATFORM_OS_PSP refs` = 0 and the folders are gone.

*Status (2026-11-04):* done, tag `pre-3.1` (local) marks the commit before it.
- Removed: `PSP/` (the port, its prebuilt libraries and patches), `Engine/Makefile.psp`,
  `Engine/util/library_psp.h` (never included: `library.h` selects windows, posix or dummy), each
  plugin's `Makefile.psp`, `psp.c`, `exports.exp` (PRX exports, PSP only), `Plugins/Makefile.psp`,
  the `PSP_VERSION` blocks in snowrain and flashlight, parallax's `dummy()` (a PSP linker
  workaround), the 8 `AGS_PLATFORM_OS_PSP` defines (always 0, never tested), and the references:
  README, Xcode project, header-check and clang-tidy excludes, Debian orig-tarball filter.
- **Kept, by the contract:** `eOS_PSP`/`eOSPSP` in the script enum (C3: removing it renumbers
  `eOSWeb`, `eOSFreeBSD`) and `"psp"` in the `override_os` names (C6, and the array is indexed by
  the enum). Credits in `Copyright.txt`, `Changes.txt` and the agsblend readme stay as history.
- Pre-Windows 7: no toolset to remove (all 124 `PlatformToolset` entries are `v143`, `AGS.Native`
  already targets `0x0601`, the editor manifest lists Windows 7 and later only). Removed: the
  editor's unused `IsWindowsXPOrHigher`/`IsWindowsVistaOrHigher`, the About box's names for
  Windows 2000 to Vista, the file-based debugger channel's Win9x check, the Win9x out-parameter
  workaround in `AGSWin32::GetDiskFreeSpaceMB`; the installer now warns below Windows 7. Kept: the
  savegame reader's skip of the Vista rich-media header (C2: older saves have it).
- **Open:** `GetDiskFreeSpaceMB` ignores the `GetDiskFreeSpaceExW` result, so a failed call returns
  an uninitialised value (a TODO marks it). Returning 0 is not the fix: both callers read it as "disk
  full" and would refuse saves if the call fails on a save directory that does not exist yet.
- Not done: `ags4-framescript` (local only, one experiment commit `5fb51ad7d`) is not deleted; the
  agent's `git branch -D` was refused as irreversible, so it is left to the user.
- Verified: `compare_objects.py` on the plugin and `platform.h` changes, 610 objects identical and
  only `ags_parallax.cpp` differing (the removed `dummy()`); `prepush.py` all stages (format 1969
  files, CMake build + ctest, header check, libc++ 340 headers, 6 MSBuild solutions with the Editor's
  NUnit tests); `AGS_PLATFORM_OS_PSP refs` 0 (`refactor_metrics.py`); the corpus run (1.4) with the
  new engine, 53 of 53 rooms pass (the engine's startup write check calls `GetDiskFreeSpaceMB`; no
  save is made, so the save-time call is not exercised). Not verified: the installer (not built
  locally), the Xcode project (no Mac).
- Found on the way, in the corpus runner (1.4): `--startr` only sets the player's start room of a
  new game, so a room's own script can send the player on (in the private game two of 54 rooms do,
  25 → 503 and 500 → 502), and `run.py` charges the rest of the run, errors included, to the start
  room, because it only checks that `Loading room <n>` appears. It should report the rooms loaded.

### 3.2a/b Makefiles to CMake

*Definition:* split of 3.2 (D5) chosen when the task was done. **3.2a, tools:** `Tools/*/Makefile` and
`Compiler/Makefile` go; the `Linux Tools` job (`ci.yml`) and the macOS tools job (`macos.yml`) build
and install with CMake. **3.2b, engine and Debian:** `Engine/Makefile*`, the plugins' `Makefile` and
`Makefile-objs`, `make.yml` go; `debian/` builds through CMake. 3.2c (the native `.vcxproj`) is not
part of it.

*Status (2026-11-05):* done and pushed; the full CI run and the macOS run are green on `32a687432`.
- Removed: 9 tool Makefiles, `Compiler/Makefile`, `Engine/Makefile`, `Makefile-defs.{linux,osx,allegro,freetype}`,
  `Makefile-objs`, `Makefile` and `Makefile-objs` of `AGSSpriteFont`, `ags_parallax`, `ags_snowrain`,
  `agsblend`, `agsflashlight`, and `.github/workflows/make.yml` (its artifact `ubuntu-make-binaries-*` had no
  consumer). **Kept:** `Plugins/agslua/Makefile*` (its only build file: no `msvc/` project, not in the
  engine) and `Plugins/agspalrender/Makefile` (above): standalone plugin builds, phase 7. The five
  deleted plugin Makefiles built standalone `.so` files; the plugins are built into the engine by CMake
  and the standalone Windows projects remain.
- Changed: `linux-make-tools` is `linux-tools` and runs `cmake` configure, build, `--install` in the builder
  image (`RelWithDebInfo` = the Makefiles' `-O2 -g`, no LTO), so a failed tool fails the job; the macOS job
  does the same with `Release`. Archive names and the layout inside (`bin/` contents) are unchanged. The
  archives now hold 11 programs instead of the 4 that built (`agscc`, `agspak`, `po2tra`, `spritepak`): new
  are `agf2dlgasc`, `agf2dta`, `agfexport`, `agsbuild`, `crmpak`, `spriteimport` (their Makefiles failed,
  silently with the earlier `find -execdir make`) and `crmbuild` (it never had one). `debian/rules` configures, builds and installs with
  `cmake` (`RelWithDebInfo`, `AGS_USE_LOCAL_{SDL2,SDL2_SOUND,OGG,THEORA,VORBIS}=ON`,
  `FETCHCONTENT_FULLY_DISCONNECTED=ON`, `DEB_BUILD_OPTIONS=rpath=` becomes `CMAKE_INSTALL_RPATH=$ORIGIN/<rpath>`),
  `debian/ags.install` takes `usr/bin/ags` from `debian/tmp`, `debian/control` needs `cmake (>= 3.16)`.
  `Engine/CMakeLists.txt` defines `AGS_CASE_SENSITIVE_FILESYSTEM` for the engine target on Linux (below).
  READMEs (`CMAKE.md`, `debian/`, `OSX/`), `CONTRIBUTING.md` and `AGENTS.md` follow.
- Measured, source lists (read statically from both sides, `.compare/helpers/srcdiff.py`; the CMake
  configure for a Linux target cannot run on Windows): `agscc` 34 files in both. Engine, Linux and
  macOS (Windows-only files left out on both sides): the Makefile compiles 396 files, CMake 390; the
  Makefile-only 13 are the 11 `apeg` MPEG audio files (each is `#ifndef DISABLE_MPEG_AUDIO`,
  defined for `apeg` by CMake, so empty), FreeType's generic `src/base/ftsystem.c` (a second
  `FT_Stream_Open` next to `builds/unix/ftsystem.c`, which `ar` never pulled in) and `psnames.c`
  (`#include "psmodule.c"`, which CMake compiles directly). CMake-only 7: `psmodule.c`, the other systems'
  platform files (`acpland`, `acplbsd`, `acplios`, `acplmac` or `acpllnx`, `acpemscripten`; each
  guarded) and `agstouch` (iOS code, a no-op elsewhere). The Makefile's
  `BASE_subdirs` listed `ac/statobj`, `font`, `setup`, which have no sources. Tools: every Makefile-only
  file is `miniz`, `stb` or `tinyxml2`, which CMake links as libraries; each tool's CMake target compiles 23
  to 44 files its Makefile did not (`xml_fields.cpp`, `data_file_writer.cpp`, ...).
- Measured, defines and flags: all carried but one, and one is new. `ALLEGRO_STATICLINK`, `AL_LIBTYPE_STATIC`,
  `FT2_BUILD_LIBRARY`, `HAVE_UNISTD_H`/`HAVE_FCNTL_H`, `BUILTIN_PLUGINS`, `HAVE_FSEEKO`, `AGS_HAS_CD_AUDIO`,
  `DISABLE_MPEG_AUDIO`, `BUILD_STR`, `-fsigned-char -fno-strict-aliasing -fwrapv`,
  `-Werror=write-strings`, `-Werror=delete-non-virtual-dtor` are set by CMake on the same targets.
  **Missing: `AGS_CASE_SENSITIVE_FILESYSTEM` for `Engine/ac/file.cpp`** (CMake set it for `common` only; the
  Linux Makefile for every file, so the shipped Debian engine had the case-sensitive branch and the CMake
  Linux builds did not): added for the engine on Linux. Not carried, no reader or only a diagnostic:
  `LINUX_VERSION`, `MAC_VERSION`, `RTLD_NEXT` (no use anywhere), the `USE_TREMOR` switch (apeg only,
  off), `-rdynamic` and `--as-needed` (engine uses `dlsym` on plugin handles only), `-Werror=format`,
  `-Werror=format-security`, `-Werror=implicit-function-declaration`, `-Wno-unused-value`. Android and
  FreeBSD do not get the case-sensitive define either, as before. **New in the package:
  `_FILE_OFFSET_BITS=64`**, which CMake defines for everything and the engine Makefiles never set: the
  i386 engine gets 64-bit file offsets, and the package's `libc6` dependency goes from `>= 2.14` to
  `>= 2.28`, the image's glibc and the floor `ci/linux/Dockerfile` states (most likely FreeType's
  `fcntl` binding `fcntl64`; read from the sources, the symbol was not looked up in the binary).
- Measured, install (`cmake --install` on Windows, Ninja Release, a scratch prefix): `bin/` holds
  `ags`, `agscc` and the 10 tools, nothing else (no library from the `EXCLUDE_FROM_ALL` subprojects).
  The rules were already there; names and `bin/` are the Makefiles'. Also on Windows: full build and
  `ctest` (1031 tests passed). `AGS_USE_LOCAL_SDL2=ON` with a real SDL2 install configures and links
  `SDL2.lib` (the installed config sets `SDL2_LIBRARIES`, not `SDL2_LIBRARY`; SDL2 reaches the link
  through SDL2_sound's static target either way).
- CI, first full run (`672e584a7`): everything green but the six jobs in the Linux builder image
  (Debian 10, GCC 8, glibc 2.28), which nothing local could run. Three causes: SDL_sound's CMake
  package names the shared library the image deletes, so `find_package(SDL2_sound)` is a configure
  error (the four package jobs); `agsbuild` uses `std::thread` and did not link libpthread, a library
  of its own before glibc 2.34, which Ubuntu hides (Linux Tools x64); the configure step downloads
  SDL2 and the i386 container did not resolve `github.com` (Linux Tools x86). Fixed: an installed
  SDL_sound is taken through pkg-config first, as the Makefile build did, and an installed SDL2 sets
  `SDL2_LIBRARY`; `agsbuild` and `tools_test` link `Threads::Threads`; the Linux tools job uses the
  image's libraries with downloads off, as `debian/rules`.
- The fixes were tried on a temporary branch whose workflow ran only those six jobs (with
  `MAKEFLAGS=-k` and steps printing the results), not through the whole matrix: green on the first
  push, then one full run on `32a687432` (26 jobs) and the macOS run, both green. Branch and workflow
  are deleted.
- Measured in the image: both tools archives hold the 11 programs; the `.deb` holds `usr/bin/ags`,
  stripped, the bundles `ags64`/`ags32` with `RPATH $ORIGIN/lib64`/`lib32` and the libraries. Against
  the last Makefile-built amd64 package (`e22c990b0`): the same ten `NEEDED` libraries; the binary is
  5,418,616 bytes instead of 6,562,432, of which about 1 MB is the `.dynsym`/`.dynstr` that
  `-rdynamic` exported (nothing reads them: no `backtrace`, `dlsym` on plugin handles only); `libc6`
  as above. Read, not run: no dpkg-buildflags (hardening) in either build (the old rules ran `make`
  directly and the Makefile set `CFLAGS :=`; the new ones run `cmake` directly).
- Left: the Debian and Linux tools jobs run only in a full CI run now that `make.yml` (every push)
  is gone, so a break there shows late; the engine was not started from the new package (the
  auto-test game runs the Ubuntu CMake build, not the Debian one).
- Stale leftovers fixed with it: the `.gitignore` block for tools built in their folders,
  `build_linux_make_task` in `.disabled.cirrus.yml`, `libfreetype6-dev` in `debian/control` (FreeType
  is built from `libsrc`, as the Makefile did), and `debian/make_ags+libraries.sh` reading the version
  from `Common/core/def_version.h` (now `Common/ac/`) and building in Debian 8 chroots (GCC 4.9, no
  C++17), now Debian 10 as the CI image. Not run, and not enough by itself: Debian 10 packages
  CMake 3.13 (3.16 needed) and no SDL_sound for SDL2 (the Makefile build needed that too), so the
  chroots need both installed, as `ci/linux/Dockerfile` does. CI builds the bundle with
  `debian/build.sh` in that image. `Plugins/agslua/Makefile*`: the user decides after the refactor.

*Found by the 3.2c investigation (2026-11-07), fixed before 3.2c:* two places where the hand-made
`Solutions/*.vcxproj` had drifted from CMake ("one build description only").
- **The 1.2 and 1.3 tests never reached the MSBuild test projects**, and those are CI's only x86 tests
  (the CMake Windows job is x64). Missing: `Common/test/game_compat_test.cpp` (1.2), `Engine/test/movelist_test`,
  `plugin_api_test`, `route_finder_test` (1.3) and the whole `engine_full_test` target (ten files). Fixed: the
  four files are in `Common.Lib.Test` and `Engine.App.Test`, with their path defines (`COMPAT_FIXTURES_DIR`,
  `PLUGIN_API_*`) made absolute from `$(MSBuildProjectDirectory)`, so a runner works from any directory (CI runs
  them from the repository root); `Engine.App.Test` links `Common.lib`, as CMake's `engine_test` links
  `common`, instead of its seven inlined Common and Allegro copies. New `Tests.App/Engine.Full.Test.vcxproj`:
  the ten test files and the eight compiler sources CMake builds in, linked with the object files of
  `Engine.App` (all but `main_sdl2.obj`; the MSBuild engine is no library), `Common.lib` and Engine.App's
  libraries, SDL2.dll copied next to it; `Tests.sln` now builds `Engine.App` first; `ci.yml`'s Tests jobs and
  `prepush.py` run it. The new tests use `GTEST_SKIP` and `INSTANTIATE_TEST_SUITE_P`, which the googletest
  submodule (release 1.8.1, 2018) lacks: the submodule is now at `52204f78`, the commit `FetchGoogleTest.cmake`
  takes, so both builds use one googletest.
  Measured, Release x86 from `Solutions\` and from the root: before 990 tests (Compiler 598, Common 113, Engine
  13, Tools 266); after 1146 run, all pass but 2 skipped as in CMake's Release (Compiler 598, Common 117, Engine
  51, Engine.Full 112 + 2 skipped, Tools 266) and 1 disabled (`route_finder_test`). The test names
  (`--gtest_list_tests`, disabled included: 1147) are identical to those of the CMake test executables of the
  same commit. Still CMake-only: the two `compat_restore_*` ctest cases (the engine restores the 1.2 fixtures'
  saves; a CMake script, not a gtest). No other test source is in a CMakeLists.txt and no `.vcxproj`.
- **The shipped x86 tools ran with asserts on:** no `Tools.App/*.vcxproj` defined `NDEBUG` in Release
  (`Compiler.App`, `agscc`, does). Fixed for Release Win32 and x64. Measured, Release x86: every tool but
  `agscc` is 20-46 KB smaller (`agsbuild` 776,704 -> 730,624 bytes), and assert texts are gone from the binaries
  (`max_threads > 0` from `agsbuild.exe`, `dst.GetBytesPerPixel() == 32` from `spriteimport.exe`); `agscc.exe`
  is byte-identical. The auto-test project built by the MSBuild `agsbuild` before and after: the package and
  `acsetup.cfg` are byte-identical (and identical between two runs of the old tools). No first-party assert in
  `Tools/` or `Common/` has a side effect (scanned). That `spriteimport` assert is always false (bytes, not bits)
  and guards `Convert8BitARGBTo32` in `Tools/data/sprite_utils.cpp`, which also steps its `uint32_t` row
  pointer by a byte stride: with asserts on, the old x86 tools stopped there; now they write past the buffer, as
  the CMake tools already did. Open, not fixed here (fixed since, below).
- Also built and run: `Tests.sln` Release x64 (the same counts), Debug x86 (all pass; the two `NDEBUG`-only
  cases skip there and the two Release skips run; `Compiler.Lib.sln` not built in Debug), `Tools.sln` Release x64 (builds). Not verified: Debug x64,
  the tools in Debug, the editor with the new tools, a CI run.

*Fixed after the 3.2c defects (2026-11-07):* the `spriteimport` overrun found above.
- **What:** `Convert8BitARGBTo32` (`Tools/data/sprite_utils.cpp`) converts an 8-bit image whose palette has alpha
  into a 32-bit sprite that keeps it (`ImportAlphaChannel`, a 32-bit game; `spriteimport`, which `agsbuild` runs
  when the sprite file is rebuilt). It stepped its `uint32_t` row pointer by the destination's stride in bytes,
  four rows per row, so any image taller than one row was written past its buffer; its
  `assert(dst.GetBytesPerPixel() == 32)` could never hold (4 bytes). The code is upstream's port of the
  `AGS.Native` conversion (`2a7b4f1f7`) and is the same in `upstream/ags4` (`4d4b2c9cd`, 2026-11-07).
- **Fixed:** the row pointer is a byte pointer cast per row, as in the other conversions of the file, and the
  assert checks `GetColorDepth() == 32` (bits). The neighbours were checked and are correct:
  `Convert8BitToHiColorImpl` (both overloads), `FixMaskColorImpl` and `ConvertAndFixMaskColor` walk byte pointers
  by byte strides, and their asserts compare bits.
- **Test:** `SpriteImport.DirectImportConvert8WithAlphaTo32` (3x4, palette alphas 255, 128, 64, 200, 32 and a
  zero-alpha entry, through `ConvertSpriteForGame`) compares every pixel row by row. Before the fix (CMake, MSVC
  x64) Release crashed with `STATUS_HEAP_CORRUPTION` (`0xC0000374`) before any result, 3 runs of 3, and Debug
  stopped on the assert; after it, it passes in both, and in the MSBuild `Tools.Test` and GCC (`linux_jobs.sh gcc`).
- **Measured:** the auto-test project's build does not reach the conversion (it keeps its sprite file, and its only
  8-bit sprite is 1x1 without a source): `compare_builds.py` gives parity, 59 identical rows, with the old and the
  new tools, and the two packages are byte-identical. With `agsbuild --import-sprites` the two sprite files differ
  only in `SpriteFileIDCheck` (`time()`, one second apart), in the sprite file, the index and the package. Not a hot
  path (a build tool), so no benchmark.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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
  `Plugins/agslua/Makefile*`, its only build file; the user decides after the refactor. The 3.2c
  investigation (2026-11-07) found the `.vcxproj` drifted: the 1.2/1.3 tests missing from the MSBuild test
  projects (CI's only x86 tests) and no `NDEBUG` in the tools' Release builds; both fixed (LOG.md), and the
  `NDEBUG` fix uncovered the `Convert8BitARGBTo32` overrun in `Tools/data/sprite_utils.cpp` (fixed, LOG.md;
  upstream has the same code: U.9).
  *3.2c re-decided (the user, 2026-11-07, after seeing a CMake-generated solution; evidence in LOG.md "3.2c
  investigation"):* **the hand-written solutions stay.** The generated one has 46 projects, four configurations,
  builds SDL2, SDL_sound and the codecs from source where ours link the prebuilt libraries, and cannot hold the
  C# editor anyway; our MSBuild jobs are CI's only x86 coverage. So 3.2c is no longer "the native `.vcxproj`
  become CMake-generated" but **"two build descriptions that cannot drift"**, in four pieces: (1) a drift guard,
  a script that compares every `.vcxproj`'s source list with its CMake target (from the CMake build's
  `compile_commands.json` or File API), with a short checked-in list of deliberate differences (the builtin
  plugins and other platforms' files `acwin.exe` never built, `sdl2_stubs.c`), run by `prepush.py` and CI; (2)
  the dead compiler dependency of the editor goes: `AGS.Native` links `Compiler_md.lib` and `AGS.Editor.Full.sln`
  builds `Compiler.Lib` for it, but nothing in the bridge uses the compiler since 6.7d-12 (the `_MD`
  configurations of `Compiler.Lib.vcxproj` go with it); (3) `Engine.Full.Test`'s wildcard over Engine.App's
  objects stays as it is while it builds (no Engine.Lib split now); (4) ~~SDL2 statically linked into
  `acwin.exe`~~ **withdrawn the same day (the user, after talking to the maintainers): the release builds keep
  SDL2 dynamically linked**, Windows (`acwin.exe` + `SDL2.dll`, which also lets a newer SDL2 revision be tested
  by swapping the DLL) and above all Linux (the bundle's `libSDL2-2.0.so.0` from `debian/rules`'
  `AGS_USE_LOCAL_SDL2=ON`). Both already are; nothing changes. Only CMake builds that fetch SDL2
  (`CMake/FetchSDL2.cmake`: developers, CI's auto-test engine) link it statically. Kept from the first decision:
  `acwin.exe` stays the shipped engine. Dropped: CMake presets for VS solutions (the user opens ours), CMake x86
  CI jobs (the MSBuild jobs stay), matching CMake's `ags` to `acwin.exe` (CMake's engine is CI's auto-test
  engine, not the shipped one).
  *Done when (rewritten):* a source file in one build description and not the other fails `prepush.py` and CI;
  the editor builds without `Compiler.Lib`.

*3.2c done locally (2026-11-07; an Opus 5.5 subagent under supervision, checked by the supervisor):*
- **The editor's dead compiler dependency:** `NativeDLL.vcxproj` no longer links `Compiler_md(_d).lib` nor has
  `Compiler/` on its include path (no include of `AGS.Native` resolves there); `AGS.Editor.Full.sln` no longer
  holds `Compiler.Lib`. Measured: `AGS.Editor.Full.sln` builds Release and Debug x86 from a fresh output folder
  with no `Compiler*.lib` built; `AGS.Native.dll` Release x86 is 2,829,312 bytes before and after;
  `compare_builds.py` with the editor before and after (same tools and engine): 59 identical, parity.
  Found: `agscc` links the `/MT` `Compiler.lib` in every configuration a solution builds; the `Release_MD` and
  `Debug_MD` configurations of `Compiler.Lib` and `Compiler.App` are built by nothing now (ci.yml, scripts,
  props and docs searched). Left for the user: remove them.
- **The drift guard** `Script/check_build_lists.py` (stdlib only): the compiled sources of each CMake target from
  the File API (codemodel-v2) against the `ClCompile`/`ResourceCompile` items of each `.vcxproj`, with a table
  of which targets a project builds (`own`), may take files from (`library`: the tools from `libtools`), and
  which projects it links (`links`: `Common.lib`, `Compiler.lib`, Engine.App's objects for `Engine.Full.Test`'s
  wildcard); the plugin DLL projects are compared with the engine's built-in copies (`prefix`). 15 deliberate
  differences with a reason each (other platforms' files, the built-in plugins, googletest, `sdl2_stubs.c`,
  freetype's two `ftdebug.c`); the editor's C++/CLI bridge is the one `.vcxproj` without a CMake target. It
  fails on a drift, on a deliberate entry that matches nothing, and on a `.vcxproj` it does not know. Run by
  `prepush.py` as the stage `buildlists` (3 s on a configured tree; `--auto` picks it for `.vcxproj`, CMake
  files and the script) and by CI's Windows `build` job (the query step before the configure, the check
  after it). Measured: passes on the tree (26 projects, no drift); a source added to `Engine/CMakeLists.txt`
  only, and one added to `Engine.App.vcxproj` only, fail with the file named (the agent); a `ClCompile` added to
  `Common.Lib.Test.vcxproj` only fails, exit 1, and passes again once reverted (the supervisor). The agent's
  one "real drift" (25 Common files `tools_test` compiles and `Tools.Test` lacked) was the table missing
  `Tools.Test`'s `ProjectReference` to `Common.Lib`: with the link recorded, nothing differs.
- **SDL2 statically linked into `acwin.exe`:** done by the agent before the user withdrew it, then reverted in
  full. What it measured, for the record: `acwin.exe` Release x86 4,079,104 -> 5,404,160 bytes, the extra system
  libraries `imm32`, `setupapi`, `version`, the auto-test 2414 of 2414 on Software and D3D9 with no `SDL2.dll`
  on the search path. Two facts if it comes back: the editor copies `SDL2.dll` into every game's
  `Compiled/Windows` (`BuildTargetWindows.cs`), and a static engine gives plugins that call SDL their own copy.
- `prepush.py --auto`: `buildlists` and `msbuild` pass (5 solutions, Compiler 598, Common 120, Engine.App 51,
  Engine.Full 112, Tools 267 tests, the editor's NUnit tests); actionlint clean.
  Not verified: the x64 builds of the editor; the CI step (the next push runs it).

### 3.3 Legacy compiler

*Definition, moved from PLAN.md on 2026-11-04:*

**3.3 Legacy compiler** (D3; run 1.4 once first): remove `Compiler/script/` (≈6.5k lines, including the
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
*Status (2026-11-03):* the legacy compiler is gone; `AGS.CScript.Compiler` waits for 6.7d.
See LOG.md for the details of the removal and its verification.

*Status (2026-11-03):* the legacy compiler is gone; `AGS.CScript.Compiler` waits for 6.7d.
- Checked first that nothing needs it: the CMake build compiled none of its sources, and linking
  MSBuild's `agscc` and `AGS.Native` without its library left only the preprocessor and
  `version.cpp` (compiled into its project, not part of it) and the five functions of the editor's
  "AGS SCOM 3 EXT" wrapper. The preprocessor took one macro from its header.
- Removed: `Compiler/script` (the AGS 3 compiler), `fmem`, their tests, `Solutions/Compiler.Lib`,
  `LEGACY_COMPILER` and CMake's `AGS_BUILD_LEGACY_COMPILER`, the editor's "AGS SCOM 3 EXT", and
  the legacy test steps in CI. `Compiler/Makefile` still built `agscc` with the legacy compiler,
  so the Makefile Build workflow's Linux binaries had it; it now builds `script2`.
- The editor gives a project that selects "AGS SCOM 3 EXT" (or no compiler) the available one
  when it loads, with a warning; `ExtendedCompiler` stays, unused, because the project loader
  rejects unknown elements.
- Renamed, move-only: `Compiler/script2` to `Compiler/script`, `Compiler/test2` into
  `Compiler/test`, `Solutions/Compiler2.Lib` (and `Compiler2*.lib`) to `Compiler.Lib`; and the
  functions `ccCompileText2` and `ccGetExtensions2` to `ccCompileText` and `ccGetExtensions`.
- Verified: CMake (compiler 594, tools 116, all tests), MSBuild `Compiler.Lib.sln` (594),
  `Compiler.App.sln`, `AGS.Editor.Full.sln` with the editor's NUnit tests (391); the parity
  harness on the auto-test (59 of 59, 2414 of 2414); a project selecting the removed compiler
  loads with the warning, compiles and is saved with "AGS SCOM 4"; the Makefile's source list
  links into a working `agscc` (MSVC objects, no GCC here).

*Status (2026-11-07):* `Editor/AGS.CScript.Compiler` is gone (task 3.3 is done). Three commits:
- `FastString`, the project's one type with a user outside it (`AutoComplete`), moved to
  `Editor/AGS.Editor/Utils/FastString.cs`, namespace `AGS.Editor` (the namespace line is the only text change).
  It is a copy for one commit, because the project's tokenizer and `ScriptAnnotations` still used it; the
  original went with the folder.
- Deleted: the project (50 files, about 3,200 lines: tokenizer, code generator, `ScriptCompiler.cs`, the
  entities), the `ProjectReference` in `AGSEditor.csproj` and `AGS.Editor.Tests.csproj`, the project and its
  configuration lines in `AGS.Editor.Full.sln` and `AGS.Editor.NoNative.sln`, the `Program.cs` assembly-resolve
  case, the unused `using` in `SpeechComponent.cs` and `GameTextEncodingChangedArgs.cs`, the dll in the
  installer's file list (`ags.iss`) and in `Windows/README.md`. Nothing in `.github/` or `ci/` named it.
- Measured: `AGS.Editor.Full.sln` Release x86 builds, the same five C# warnings (CS0168, CS0612) after the move and after the
  deletion, all in files this task did not touch, no `AGS.CScript.Compiler.dll` in the output; editor NUnit 547 of 547
  after each commit, `AutoCompleteTests` among them (they cover `FastString` through `AutoComplete`).
  `grep CScript` finds nothing outside `docs/context/` (upstream knowledge, upstream still has the project) and
  this file and PLAN.md.
- Not verified: the installer (`ags.iss` builds only with Inno Setup and a prepared `Source\`), the README text,
  and `AGS.Editor.NoNative.sln` (its native project fails to link `Common_md_d.lib` in a fresh output
  directory, in a project this change does not touch; not compared against the parent commit, and the log shows no C# error).

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

- [x] **3.3 Legacy compiler** (D3). Done 2026-11-07: the legacy compiler went on 2026-11-03,
  `Editor/AGS.CScript.Compiler` on 2026-11-07 (`FastString` lives in `Editor/AGS.Editor/Utils/`; LOG.md).
  What was left: **`Editor/AGS.CScript.Compiler`**. Its compiler parts were never finished and are unused;
  its preprocessor and the `PreprocessorTests.cs` cases went with the editor's build (6.7d-12,
  2026-11-07; 7.1c had ported the cases to the C++ tests). Only `FastString` has a user outside the
  project (autocomplete), plus an assembly-resolve entry in `Program.cs`. Order: move `FastString`
  into the editor, delete the project from the solutions, the installer's file list and CI.
  *Done when:* one compiler, one preprocessor (C++), one test folder, no `AGS.CScript.Compiler`.

### 3.4 Legacy data-version paths

*Definition:* PLAN.md 3.4 (D1), five steps: game files, room files, savegames, the script API, savegame-only fields.

*Status (2026-11-07): step 1, game files, done.* Four commits:
- **Behaviour:** `kGameVersion_LowSupported = kGameVersion_399`. Data older than that gets its own message, with
  the editor's version string, the data format and the supported range; the engine shows it in its "Loading game
  failed" box: "Game was compiled with 3.6.0.21 (data format 3060021). This engine loads games made with AGS 3.99
  or later (data formats 3999999 - 4000034)." The error code is still `kMGFErr_FormatVersionNotSupported`.
  `GameDataVersion.*` in `game_compat_test.cpp` write a header as the tools do and open it: 3.6.0.21 and 3.6.3.16
  are refused with that message, 3.99 opens, a version newer than the engine is refused. With the old minimum the
  first test fails.
- **Deleted, Common** (57 lines): the `kGameVersion_362_07` value derived from the "compiled with" string; the
  pre-3.99 branches of `UpgradeGame`, `UpgradeCharacters`, `UpgradeGUI` (no `Upgrade*()` function went: each
  still converts 3.99 and early 4.0 data); the "< 3.6.3.4 or 3.99 to 4.0.0.26" conditions became "< 4.0.0.26",
  the same set for 3.99+ data; the inventory hotspot alignment, button frame, list box selection and slider handle
  rules of 3.6.x data.
- **Deleted, Engine** (158 lines, 17 files): the 29 checks of 3.6.x versions in `Engine/ac`, `Engine/game`,
  `Engine/gui`, `Engine/main` (dialog option layout and fonts, text padding and border colours, crop clamping,
  restart point, room event order, the old nearest-walkable-point scan, ...). `Overlay`'s
  `PreserveTextColor` had a 3.6.3.16..3.99 range: `>= 4.0.0.33` alone gives the same for 3.99+ data.
- **Deleted, the enum values** `kGameVersion_360_21` to `kGameVersion_363_16` and `_362_07`: nothing in the tree
  names them (Editor, Tools, Plugins, Compiler, tests and scripts included). Kept: the format history comment in
  `game_version.h`; three field comments now say "since 3.6.3.14" instead of naming a value.
- Comparisons with `kGameVersion_399` itself, which the metric does not count: 4 (`main_game_file.cpp` 2,
  `overlay.cpp` 2), all gone.
- Measured: `legacy version checks` 62 → 0 (files 22 → 0); Engine C/C++ lines 125744 → 125616. `prepush.py`
  every stage (format 1988 files, CMake build with warnings as errors and ctest 1150 of 1150, tidy on changed
  lines clean, headers, libc++ 343 headers, emscripten, the Linux GCC job 1150 of 1150, the five MSBuild solutions with their tests, `AGS.Native` included, and
  the editor's NUnit 541 of 547 with 6 skipped); no recompiled
  file has more MSVC warnings than before; the auto-test game 2414 of 2414 after the Common and the Engine
  steps; the corpus run (1.4) 53 of 53 rooms pass, with a per-room table identical to that of an engine built
  before the change.
- Left, constant now but more than a deleted branch: `GUI::DataVersion` has no reader (written by `ReadGUI`,
  the engine, `AGS.Native` and `gui_test.cpp`: three programs); the four runtime behaviour switches are always
  on, so their config overrides (C6 keys) change nothing; `DialogOptions`' `line_x_off`,
  `normal_area_counter_align_pad` and `fixed_padding` are always 0; `FindNearestWalkablePoint`'s old method is
  called by `route_finder_test` only; `InventoryItemInfo::ReadFromFile`'s version parameter is unused. GUI versions
  are not pre-3.99-only (3.99 data carries `kGuiVersion_350`), so their checks stay.
- Not verified: macOS, iOS, Android, FreeBSD and the Debian 10 builder image (no local build); a real 3.6 game
  (the refusal was checked with a package holding a 3.6.0.21 header). `compare_objects.py` does not apply: the
  deletions change what older data does, by design. The editor's `ImportOldGame` (`AGS.Native`) shares the
  reader, so it refuses compiled games older than 3.99 too; 6.7 deletes it.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

  1. **Game files.** *Done 2026-11-07 (LOG.md):* `kGameVersion_LowSupported = kGameVersion_399`, a
     3.6 game gets "Game was compiled with 3.6.0.21 (data format 3060021). This engine loads games made
     with AGS 3.99 or later", the pre-3.99 branches of the `Upgrade*()` functions, Common/gui and
     Engine and the enum values are gone: `legacy version checks` 62 → 0 (22 files → 0).

### 7.1 Make the C++ tools the only build path

*Starting point, moved from PLAN.md on 2026-11-04:*

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

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 7.1a Build-parity harness

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1a Build-parity harness** (`Script/compare_builds.py`). Copies a project twice, builds one
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

### 7.1b Unblock the pipeline

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1b Unblock the pipeline** (gaps 1, 3 and 5; small, separate commits):
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

### 7.1c One preprocessor

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1c One preprocessor** (gap 4). Diff `Editor/AGS.CScript.Compiler/Preprocessor.cs`
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

### 7.1d Table-driven XML reading

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1d Table-driven XML reading** (maintainers' design). C++ has no reflection, so emulate it
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

### 7.1e AGS 4 room sources

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1e AGS 4 room sources** (gap 2). Compile `Rooms/<n>/` to `.crm` in C++, following the same
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
Tests: `Tools/test/room_build_test.cpp`. Found on the way, fixed later (U.1):
- `crmpak -e <room> script <file>` crashes (`ReadOnlyCompiledScript` passes a null room).
- `ImageFile::LoadPNG` copies only `width*height` bytes of an RGB(A) image when no channel swap
  is needed.
- Possible `game28.dta` gaps for 7.1f: `agf2dta` indexes interaction events by the XML `Index`
  instead of the cursor schema, doesn't unescape or re-encode text properties, and writes
  `"4.00.00.34 "` (trailing space) as "compiled with", where the editor writes `"4.00.00.34"`.
  (7.1f fixed the last two.)

### 7.1f Content parity

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1f Content parity.** Work through the 7.1a table until every file matches: game data
(`agf2dta`, including fork features such as Opacity 0–255), dialog scripts (`agf2dlgasc` against
`DialogScriptConverter.cs`), script headers (`scriptgen`), sprites (`spriteimport`), the audio
cache, translations (`trac`), custom data and fonts. If a difference is harmless (ordering, for
example), make both sides agree anyway, so parity stays a plain byte comparison.
*Done when:* every file is identical and the auto-test passes on `agsbuild` output (2414 of 2414,
as with the editor).

*Status (2026-11-02):* done for the auto-test project. The 17 files the editor builds are
identical in the `agsbuild` output, except that the compiled scripts are separate files (40 `.o`
files and `ScriptModules.lst`) instead of being embedded in `game28.dta`; each matches the script
the editor embeds. The auto-test passes on the `agsbuild` output (2414 of 2414). What it took:
- **Layout:** the editor embeds the compiled scripts in `game28.dta` (global, dialog, then the
  modules in compile order). `agsbuild` packs them as separate files (`GlobalScript.o`,
  `__DialogScripts.o`, the modules listed in `ScriptModules.lst`), which the engine loads since
  3.6.1 (upstream `4de348c79`). For a while `agf2dta --script-dir` embedded them as well, for byte
  parity; it was removed after maintainer review, because the build doesn't need it and it made the
  game data wait for every compile. `compare_builds.py` instead takes the scripts out of the
  editor's `game28.dta` (the scripts, the `HasCCScript` flag, the extension offset and the script
  names in `v362_interevent2`) and compares the rest byte for byte. Every game script is compiled
  against all module headers, as the editor does: in AGS 4 a module may use what the modules after
  it declare, and the engine links imports in any module order (upstream PR #1408). Fonts come
  from the `Fonts` folder (`<FontFiles>` in `Game.agf`), packed without the folder.
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

### 7.1g Drift guard

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1g Drift guard.** Two halves:
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

### 7.1h Diagnostics for the editor

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1h Diagnostics for the editor.** Behind a flag, `agsbuild` and the tools it runs print
errors, warnings and progress in a machine-readable form (one JSON object per line: severity,
tool, file, line, message), and `agsbuild` stops cleanly when cancelled.
*Done when:* the editor (6.7d) can list tool errors with jump-to-line and cancel a build.

*Status (2026-11-02):* done, with one change to the plan: **`agsbuild` reads the diagnostics from
the tools' output** instead of every tool printing JSON. The tools already print `Error: ...` /
`Warning: ...` lines, and `agscc` adds `at <file>, line <N> : <message>`; one tested parser
(`Tools/agsbuild/diagnostics.cpp`) is cheaper than ten tools each learning a flag, and the
format is still one JSON object per line. Moving it into the tools stays possible later.
- `agsbuild --json`: stdout carries only JSON lines (`start`, `task` with its state, `diagnostic`
  with severity/tool/task/file/line/message, `progress`, `end`); the usual log goes to stderr.
  The format is documented in `Tools/agsbuild/diagnostics.h`. On the auto-test project it gives
  the three real "Code execution cannot reach this point" warnings of the dialog scripts, with
  their `Dialog N` section and line; a script with a syntax error gives
  `{"severity":"error","tool":"agscc","file":"helpers.asc","line":68,...}`.
- Cancel: SIGINT/SIGTERM (Ctrl+C on Windows), or the line `cancel` on stdin in `--json` mode (the
  editor can't signal a hidden console process). Waiting tasks don't start, running tools are
  killed (`PipedProcess::Terminate`, which on POSIX kills the whole process group), `end` says
  `cancelled`, exit code 2. Measured: cancel 0.5 s into the auto-test build leaves no tool
  processes behind.
- Tests (`tools_test`, 109 pass): the line parser and JSON escaping (`diagnostics_test.cpp`), a failed
  task cancelling only its dependents and being reported, and cancelling a build of sleeping tools
  in under a second (`taskmanager_test.cpp`, the failing-task test 7.1b asked for).
- The parity harness is unchanged: 17 of 17 identical, auto-test 2414 of 2414.
- Not checked locally: the POSIX side of `Terminate` (CI's Linux jobs run the new tests).

### 7.1i Split `BuildWithTaskManager`

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1i Split `BuildWithTaskManager`** (535 lines, `Tools/agsbuild/main.cpp`) into one function
per build stage, while doing the steps above. It stays.

*Status (2026-11-02):* done. `BuildWithTaskManager` is 45 lines now: it reads the project
(`ReadProject`), then calls one function per stage on a `BuildContext` (the lists the stages share):
`AddExportTasks`, `AddScriptTasks`, `AddGameDataTask`, `AddRoomSourceTasks`, `AddCompiledRoomTasks`,
`AddTranslationTasks`, `AddSpriteTasks`, `AddAudioTasks`, `AddProjectFileTasks`, `AddGamePackTask`,
`AddAudioVoxTask`, `AddSpeechVoxTasks`, `AddDeployTask`, `AddCleanupTask`. The order is the old order
(it decides the order of files in the package); measured: still 17 of 17 identical.

### 7.1j CI

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1j CI.** A Linux job builds the auto-test project with `agsbuild` from the tools of the same
run and hands it to the platform jobs (this is 1.7). A Windows job runs the parity harness against
the editor on every PR until 6.7d deletes the C# build code.

*Status (2026-11-02):* the Linux half is done and green in CI (run 41432710133). `agsbuild` takes
`--strict` (passed on to `agf2dta`). `Script/agsbuild_autotest.sh <build-dir> <project> <work-dir>`
collects the tools from a CMake build, builds the project in strict mode, and runs the engine on the
result (checked with `tappy`); locally it gives 2414 of 2414. The `Ubuntu-x86_64` job in `build.yml`
checks out the test game at `AUTO_TEST_TAG` and runs it after the downloaded auto-test.
*Status (2026-11-02, 1.7):* handed over, **not yet run in CI**. A new `Auto-test game` job builds
only the tools (`-DAGS_BUILD_ENGINE=0`), runs `agsbuild_autotest.sh ... --build-only`, and uploads
`ags4-auto-test.ags` + `acsetup.cfg` as `auto-test-game`; every `build` matrix job needs it and
downloads that instead of the release zip. The Ubuntu-only step is gone: every platform now runs
the tool-built game.
*Status (2026-11-02, later):* the Windows half is done and green in CI (commit 2a43f3092). The `Windows`
matrix job uploads its tools and `ags.exe` as `parity-tools-windows`; the new `Windows Build
Parity` job downloads them with the editor, the x86 engine and SDL2.dll, puts `acwin.exe` next to
`AGSEditor.exe`, and runs `Script/compare_builds.py` on the auto-test at `AUTO_TEST_TAG`
(with `--engine`). Logs upload as `parity-logs`. Only 1.7 is left of 7.1j.
*Status (2026-11-02, final):* done. 1.7 is green too (run 41442221520), and both halves stayed
green through 2.6 (run 41461852760).
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

### 7.1m PO translation sources

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1m PO translation sources** (found by 1.4). AGS 4 projects keep their translations as
`<Name>.po` (`Translation.TRANSLATION_SOURCE_FILE_EXTENSION`); the editor converts the legacy
`.trs` when it upgrades a project. `agsbuild` asked `trac` for `<Name>.trs`, so the whole build
failed on a project with a translation.
*Done when:* `trac` compiles a PO translation into the same `.tra` as the editor (tests for each
rule, and the parity harness on a project with a PO translation).

*Status (2026-11-02):* done. `trac` compiles PO only (`trac -u` still decompiles to TRS), and
`agsbuild` passes it `<Name>.po`.
- `ReadPO` (`Tools/data/tra_utils.*`) follows the editor's parser (`Translation.LoadDataImpl`):
  multi-line strings, `msgctxt`, the `# $` special tags, obsolete (`#~`) and fuzzy entries, only
  `msgstr[0]` of plural forms, parser words by their `PARSERWORD:<id>` context, and a text found
  twice keeping its first place with the later translation. It fills a `TranslationSource`,
  which keeps the file's order (the engine's `Translation` holds the texts in hash maps).
- `WriteTRA` writes the `.tra` as the editor does (`TranslationsComponent.CompileTranslation`),
  not through Common's `WriteTraData`: texts in the file's order and without a terminator,
  resolved with the editor's `UnescapeText` (octal and hex escapes in the translation's
  encoding), obsolete, fuzzy and untranslated entries left out, always the two string options
  (encoding and language), font overrides in the file's order, and the editor's ending. Only an
  `AutoTranslateParserSaid` of `ON` sets the option, as in the editor.
- `StrUtil::GetKeyValue` ignored its separator and split on `=`; TRS annotations and PO contexts
  (`PARSERWORD:5`) were never read.
- Measured: the private AGS 4 game's `Italian.tra` is byte-identical to the editor's. Not
  covered by that game: font overrides in a `.tra` (the writer follows Common's font block),
  and translations in a non-UTF-8 encoding with numeric escapes above 127 (written as one byte;
  the editor uses the system code page).

### 7.1o `trac` → `po2tra`, decompiling to PO

*Definition, moved from PLAN.md on 2026-11-04:*

**7.1o `trac` → `po2tra`, decompiling to PO** (asked 2026-11-03). After 7.1m the tool kept
upstream's name but no longer read upstream's input (TRS), and `-u` still wrote TRS, a format
AGS 4 projects no longer use.
*Done when:* the tool, its target and every reference are `po2tra`, and `po2tra -u` writes a PO
that compiles back to the same translation.

*Status (2026-11-03):* done. `WritePO` (`Tools/data/tra_utils.*`) replaces `WriteTRS`: the
editor's PO layout (`Translation.SaveDataImpl`), texts escaped as `EscapeText` does but with
`\r` kept (the texts come from the game and must still match it), entries and font overrides
sorted (the engine's `Translation` holds them in hash maps). A TRA has no Text Parser source
words, so each word ID gets one entry with the placeholder `??? (word <id>)`, unique because a
PO reader keeps one entry per text.
- Measured: tests for the PO text and for PO → TRA → PO → TRA giving the same translation
  (texts, parser words, options, font overrides); on a copy of the private game's PO with
  every text translated (783 texts, 79 with escapes), decompiling both compiled files gives
  identical POs. The two `.tra` files differ only in the order of the texts.
- Upstream gets `po2tra` as a new tool for PO sources, next to its own `trac` (TRS), which
  stays as it is (decided 2026-11-03, until further notice). U.3's `po2tra` PR and any port of
  7.1m/7.1o add `po2tra`; removing `trac` upstream needs the maintainers' agreement.

### U.1 `ags4-bugfixes`

*Definition, moved from PLAN.md on 2026-11-04:*

**U.1 `ags4-bugfixes`:** fixes to existing upstream behaviour that the fork found, one commit
each. Checked one by one against `upstream/ags4`: the fork's OGL ES2 fix (fake gamma is fork-only),
the slider focus fix (upstream fixed it its own way) and the version bump (follows Opacity) don't
apply. Fixes already in upstream `master` are left to upstream's usual master-to-ags4 merge.
*Done when:* the branch builds (CMake tools + tests) and its tests pass.

*Status (2026-11-02):* done, 9 commits on `upstream/ags4` (`e70c7c95d`), merged upstream as #3181
(`a83208d33`, with the two U.2 additions below):
- Editor: `Dialog(id, scriptName)` crash, invariant culture in project files, `ikpFlac.dll` missing
  from the AnyCPU build (upstream's Windows Packaging fails on it today: `ags.iss` line 103), stale
  character after `ParserState.Reset()` (false "unterminated literal" in translations). Picked
  from the fork and `avalonia` with `-x` lines and trailers removed.
- Tools: the agsbuild pipe hang, `agf2dlgasc` dropping the last line, inverted `show_fps`. The
  fork's patches only applied in part (`git apply --ignore-whitespace --reject`); the rest was
  rewritten in upstream's style.
- New, also fixed in the fork: `ImageFile::LoadPNG` copied `width * height` bytes of RGB(A) images
  when no component swap is needed (test: `Common/test/imagefile_test.cpp`, a PNG round trip in
  PNG byte order), and `crmpak -e <room> script` passed a null `RoomData` (7.1e found both).
- Checked against the unfixed code: both new tests fail, `crmpak` segfaults, the pipe test aborts
  after 120 s, and 5 of the 7 dialogs of `ags3/auto-test` lose their final `stop`.
- Verified: CMake Ninja (Common 94, Compiler 561, Engine 3, Tools 76 tests) and MSBuild as
  upstream CI builds it (`Tests.sln` and `AGS.Editor.Full.sln`, Release x86, v142; NUnit 370 of
  370). The worktree needs `git submodule update --init Common/libsrc/googletest` and the ignored
  `Editor/References` DLLs and `Solutions/packages` copied in.
- Found, not fixed: `ags3/auto-test/room1.crm` (room version 40, `kRoomVersion_399`) fails to load
  in both upstream and the fork ("Block data overlapping" in the Main block). D1 says 3.99 rooms
  must load, so this belongs to 1.2/1.4.
*Status (2026-11-02, U.2):* 11 commits now; U.2 found two more upstream bugs, neither in the fork:
- `AGS.Types`: since `6148cb840` (2026-11-01) characters and inventory items load their
  interactions into a new object and lose the script module, so the game data gets an empty
  module (the engine then looks only in the global script). Two NUnit tests, which fail on the
  unfixed code; NUnit 372 of 372.
- The `agf2dta`, `crmpak` and `spriteimport` Makefiles have not built since `63f2b8996`
  (`interactions.cpp` gone, `scripteventtable.cpp` and `cc_reflect.cpp` missing). The Makefile
  Build workflow does not notice: `find -execdir make` drops make's exit code. Checked without
  make or GCC by linking exactly the listed sources' MSVC objects
  (`.compare/helpers/mk_linktest.py`): they fail before and link after.

### U.2 `ags4-buildtools`

*Definition, moved from PLAN.md on 2026-11-04:*

**U.2 `ags4-buildtools`:** the 7.1 work (AGS 4 support in the C++ tools: project format,
script API levels, `#module`, `crmbuild`, game data parity, table-driven XML, drift guard,
`--json`), stacked on U.1, with `Script/compare_builds.py` and `Script/agsbuild_autotest.sh` but no
CI wiring. Starts by cherry-picking the four `master` tool commits that `ags4` lacks and that the
fork's agsbuild changes build on (`e5f9eab3a`, `0ddc44b1a`, `89a48f169`, `81f0a2cfd`). Each fork
commit is ported by hand: the fork's code is reformatted, so patches only apply with
`git apply --ignore-whitespace --reject` and the rejected hunks are rewritten. The tools must
write **upstream's** formats: project 4.0.0.33, `Transparency` 0–100 converted as upstream's
`DataFileWriter.cs` does (`Trans100ToLegacyTrans255`), upstream tint ranges and room versions.
*Done when:* `compare_builds.py` reports every file identical between `agsbuild` and an upstream
editor (MSBuild, `PlatformToolset=v142`) on the upstream test game (`ags-test-games` `main`,
`ags4/auto-test`, 4.0.0.33), and the upstream engine passes the auto-test on `agsbuild` output.

*Status (2026-11-02):* done, 24 commits on `ags4-bugfixes`, open as #3182. To change after
maintainer review: drop `agf2dta --script-dir` and pack the scripts separately, as the fork now does
(7.1f, "Layout"), with the `compare_builds.py` comparison that goes with it.
*Status (2026-11-03):* superseded by the five U.3 branches (the old branch is kept as
`backup/ags4-buildtools-3182`).
- The four `master` commits with `-x`; `0ddc44b1a` needed `dst_depth` renamed to ags4's
  `res_depth` (noted in its message). Then every 7.1 commit, one upstream commit each, except
  the CI wiring and the fork-only ones (Opacity, formatting); plus "build crmbuild with MSBuild
  and make too", since upstream builds every tool with `Tools.sln` and a Makefile as well.
- Porting: `.compare/helpers/port.py` matches each fork hunk token by token (whitespace, `NULL`
  vs `nullptr`, `Common::` qualifiers ignored), keeps upstream's own context lines and re-indents
  the new ones; `check` compares each port with its fork commit after clang-format, so only the
  intended adaptations remain. That replaced most of the hand-rewriting of rejected hunks.
- **Upstream stays C++11** for Common, Compiler and Tools, on purpose (`CXX_STANDARD 11`,
  `-std=c++11`, compat `std::make_unique` and `std::size` in `util/memory_compat.h`, a recent
  "remove use of c++17 insert_or_assign"). So the 7.1d rows are `XML_READ(&T::Member)`,
  `XML_READ_WITH`, `XML_READ_ELEMENT` and `XML_READ_CONVERTED` macros over
  `template <typename MP, MP Member, ...>` instead of `Read<&T::Member>`; `inline constexpr`,
  generic lambdas and the fork's `CXX_STANDARD 17` for `crmbuild` went too.
  `.compare/helpers/cxx11.py` checks the changed sources with emsdk clang in `-std=c++11` (MSVC
  has no C++11 mode); tests compile as C++17, which googletest asks for.
- Upstream formats: project format 4.0.0.33 only (the oldest without an upgrade step: it moved the
  dialogs into their own files); room formats 3.99.99.01 to 4.0.0.27, with the `Name` and
  `Description` of rooms saved by older editors; `Transparency` 0-100 everywhere, converted with
  `Trans100ToLegacyTrans255` for characters, GUIs and room objects and written as a percentage
  for GUI controls, as `DataFileWriter.cs` does; region light level and tint in the editor's
  ranges (`LightLevel - 100`, `TintLuminance * 25 / 10`). `agf_elements.txt` regenerated from
  upstream's `AGS.Types`.
- Verified on the final branch: `compare_builds.py` 17 of 17 identical against the upstream editor
  (MSBuild v142 x86, with U.1), with the tools from CMake and from MSBuild (`Tools.sln`,
  `Compiler.App.sln`); the upstream engine (CMake) passes the auto-test on the `agsbuild` output,
  2380 of 2380, also through `agsbuild_autotest.sh` (which passes `--strict`). CMake tests: Common
  97, Compiler 592, Tools 110. MSBuild as upstream CI: `Tests.sln`, `Compiler.Lib.sln` (91),
  `Compiler2.Lib.sln` (592), NUnit 373 of 373. Every one of the 24 commits builds and passes on
  its own (`git rebase --exec .compare/helpers/verify_strict.sh`).
- Not checked: POSIX builds and the Makefiles under real make/GCC (no GCC here; the Makefiles' source
  lists link with MSVC, `mk_linktest.py`); `tappy` isn't installed locally, so the TAP file was
  counted by hand.

### U.3 Pieces for #2316

*Status of 2026-11-03, moved from PLAN.md on 2026-11-04:*

*Status (2026-11-03):* the branches are cut, nothing posted yet. #3182 was not ready: it still
had `agf2dta --script-dir` (the fork dropped it in `461e41138`, after the branch was cut) and
`agsbuild --json`, both objected to by the maintainers, and no PO compiler at all (`agsbuild`
still ran `trac` on `.trs`); five later fork fixes were missing too. Re-cut as five branches in
the worktree, on `upstream/ags4` (ledger in `docs/UPSTREAM.md`): 1 fixes (9 commits), 2
table-driven `agfreader` + drift guard (8), 3 `agf2dta` parity (4), 4 `crmbuild` (2), each on the
previous; 5 `po2tra` (2) on `upstream/ags4`. Rebased 2026-11-03 after `master` was merged into
`ags4` (`e709f84a0`): the six `master` picks dropped out, the fork's unescape commit too (upstream's
`ad38522e2` covers it; the game info stays unescaped, as the 4.0 editor's `WriteExt_363GameInfo`
writes it, where `ad38522e2` applied `TextProperty`). #3182 is closed. Dropped: `--json`, cancel and the stage split (fork-only until
#2530 lands). `po2tra` is a new tool next to `trac`: `tra_utils` keeps the TRS code and gains
`ReadPO`, `WritePO` and `WriteTRA(TranslationSource)`; the two shared helpers got a parameter.
Verified at each tip (CMake MSVC build, C++11 check, Common/Compiler/Tools tests: 97/592/83,
92, 94, 104; PR 5: 95/561/81); PR 4 against the upstream editor on `ags4/auto-test`: 58 of 58
files identical, auto-test 2380 of 2380; the package lists its assets by name where the editor
uses its build order (the fork's editor fix `e69119490` is fork-only). `po2tra` on the private
game's PO gives the same `.tra` and PO as the fork's. Makefile source lists of `po2tra`, `trac`
and `crmbuild` link (`mk_linktest.py`). Every commit of the pre-rebase stack passed `rebase --exec verify_strict.sh` (31 of 31); after
the rebase only the tips were verified (4: 102/592/104, parity 58 of 58, auto-test 2380 of 2380;
5: 99/561/82). Not done: MSBuild (`Tools.sln`, `Tests.sln`) and NUnit on the new branches. Found on the way: `verify_strict.sh`
reported "ok" with a failed test (fixed: it fails on any `FAILED`), and a stale
`mk_linktest/agspak.exe` in the build directory made the harness report a wrong ender.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

  *Status (2026-11-03, details in LOG.md):* #3182 closed; re-cut as five branches on
  `upstream/ags4` (1 fixes, 2 table-driven `agfreader` + drift guard, 3 `agf2dta` parity, 4
  `crmbuild`, each on the previous; 5 `po2tra`), rebased after the `master` → `ags4` merge
  `e709f84a0`, tips verified. Opened as #3186–#3190 (2026-11-03; no reviews by 2026-11-04).
  Dropped until #2530: `--json`, cancel, the
  stage split. Not done: MSBuild and NUnit on the new branches.

### U.4 Agent docs upstream

*Definition, moved from PLAN.md on 2026-11-04:*

**U.4 Agent docs upstream** (PR #3185, 2026-11-03, base `master`, one commit `066cb1152`):
`AGENTS.md` and `docs/context/`, superseding the `AGENTS.md` of #3025. The fork's
`docs/context/` is a copy; changes requested in review are applied in the worktree and copied
here, never edited here first.

### U.5 Save fix upstream

*Definition and status, 2026-11-04:*

**U.5 Save fix** (PR #3191, branch `ags4-fixcharinvsave` off `upstream/ags4` `e709f84a0`, one
commit `5ca3e45ce`): the fix of `CharacterExtras::ReadFromSavegame` found by 1.2 (fork
`d403d7236`), so 4.0 saves before 4.0.0.28 read the legacy inventory. Upstream had the same bug.
Verified on the upstream engine build with the fork's compat fixtures (not committed upstream):
without the fix the 4.0.0.27 save fails with "Component data size mismatch" and 4.0.0.30 passes;
with it both pass. `engine_test` 3/3, `savegame_test` 6/6, `common_test` 99/99. The Transparency
compat exports (fork `587403037`) are not part of it: they belong to the Opacity offer (#3119).

### 6.0 study: ags-for-mac, #1092, #3177

*Definition (PLAN.md "Next up" item 2 of 2026-11-03):* read `ags-for-mac` (`AGS.Types` diff, the C
library that replaced `AGS.Native`) and #1092; decide which 6.0 steps go upstream and whether 6.7c
reuses that library's surface.

*Status (2026-11-04):* done; the evidence is in `EDITOR-SPLIT.md`, the decisions in PLAN.md (6.0
re-scoped, 6.1 note, 6.7c, U.6, "Next up"). Read: the repository at `4c3f45c` (3.6.4 dev, .NET 10,
Avalonia; upstream sources linked in place, 18 `AGS.Types` files forked, 6 shims, 1 regex rewrite;
`libagsnative` 3,072 lines, 72 C functions, 38 of them rooms; 89 test methods), its
`FORKED_FILES.md`, `PHASE1_ISOLATION_PLAN.md`, patches README, the C header and interop; #1092,
#3177 (7 comments to 2026-11-03), #2432. Measured on the fork: 17 `AGS.Types` files use WinForms
(same set as upstream `ags4`; `master` has 19), 5 of them only through an unused `using`; 112
`[Editor]` attributes on 10 `UITypeEditor` classes, 11 `[PropertyTab]`, 91 portable
`[TypeConverter]`; no `System.Drawing.Font` or `Bitmap` in the model (the plan's "55 Font / 15
Bitmap" were AGS's own `Font` class), `Icon`/`Graphics`/`Bitmap` only in the plugin-facing
interfaces and templates; 9 static GUI delegates set in `GUIController.cs` and `FontsComponent.cs`;
`.crm` is read by the editor only in the 3.99 upgrade (`ConvertAllRoomsFromCrmToOpenFormat`) and
written only by the room compile step (`SaveCrm`). Decided: 6.0 step 1 (property grid out) is
upstream-ready as U.6 with no plugin API change; step 2 (interfaces, `EditorContentPanel`,
`MenuCommand`) versions C5's editor plugin API and waits for the user; 6.7c reuses the library's
conventions and the image, sprite-file, font and GUI entry points, not the room half, the
CMake-generated constants or the 3.6 structs, and adds a sprite conversion function instead of
ags-for-mac's `spriteimport` subprocess. Its three bug patches (#3144, #3163, #3164) are already in
`ags4` and the fork; its unpatched `LoadPNG` note does not apply to the fork's code. Not verified:
nothing was built; the string-form attribute resolution on .NET Framework is asserted from the
`EditorAttribute`/`PropertyTabAttribute` string constructors and must be proven by U.6's guard test.
The user decided the same day: breaking editor plugins is acceptable, the editor plugin API leaves C5,
and 6.0 step 2 needs no further gate (better design over backward compatibility).

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

- [ ] **6.0 `AGS.Types` UI-free.** *Re-scoped 2026-11-04 by the 6.0 study (EDITOR-SPLIT.md):* 17 files
  use `System.Windows.Forms` (5 only through an unused `using`), 112 `[Editor(typeof(...))]` attributes
  name 10 `UITypeEditor` classes and 11 `[PropertyTab(typeof(PropertyTabEvents))]` a WinForms
  `PropertyTab`; the 91 `[TypeConverter]` attributes are portable `System.ComponentModel`. The model
  stores **no** `System.Drawing.Font` or `Bitmap` (the earlier counts were AGS's own `Font` class); the
  GDI types that remain (`Icon` 5, `Graphics` 6, `Bitmap` 6) sit in the plugin-facing interfaces and
  the templates. `Color`, `Point`, `Size`, `Rectangle` are fine.

### U.6 AGS.Types property grid out

*Definition (PLAN.md, 2026-11-04):* 6.0 step 1 on `upstream/ags4` as the #1092 fix: string-form
`[Editor]`/`[PropertyTab]` attributes, the UI editor classes and `PropertyTabEvents` into `AGS.Editor`,
direct calls instead of the static GUI delegates, unused usings removed, a guard test that resolves
every attribute string.

*Status (2026-11-04):* done as a branch, not pushed. A Sonnet 5.5 subagent (medium effort, ~11 min
plus a 1 min follow-up, supervised through a progress log) cut `ags4-types-propertygrid` from
`de5508362`: `25604a72d` move (16 files, 94-97 % rename similarity, `git log --follow` works) +
`EditorTypeNames` constants, `b57689b77` direct calls (nine `GUIController` methods and
`FontsComponent.FontImportSize` become `internal`), `9161145dc` eight unused usings and the
`System.Design` reference, `c6c69ab1e` six NUnit tests. 67 files, +438/-313. Measured: NUnit 376 of
376 before, 382 of 382 after; breaking the assembly name on purpose failed 4 of the 6 new tests.
Found on the way: the editor assembly is `AGSEditor`, not `AGS.Editor` (PLAN.md corrected); the
`UITypeEditor` base name must be fully qualified or `ReflectTypeDescriptionProvider` drops the
editor silently; `BuildTargetUIEditor` derives from `StringListUIEditor`, so both moved and
`Settings.cs` got `Utilities.StringListSeparators`; `AGS.Editor.NoNative.sln` still builds the
C++/CLI project and fails to link `Common_md_d.lib` here, `AGS.Editor.Full.sln` builds;
`AGS.Types/Utilities.cs` has LF endings in a CRLF tree. Supervision: one false alarm from me (a
rename check with the wrong flags), cancelled before the agent acted. The user ran the editor built
from the branch the same day: the property grid works in the properties panel and in Settings. Not
verified: the Debug configuration, a clean checkout without `Editor/References`.

*Review round 1 (2026-11-04):* the user pushed v1 and opened the PR; ivan-mogilko asked for the
classes in `AGS.Controls` (where the custom controls already are) and objected to the "direct calls"
commit: controls must not know `GUIController` or editor components, and hardcoding that behaviour in
control classes is bad design. Both accepted: `AGS.Controls` depends on `AGS.Types` and WinForms and
not on `AGS.Editor`, so it enforces the decoupling; the static delegates upstream already had were the
right seam and removing them was our regression. The same subagent reworked the branch in ~2 min:
`backup/ags4-types-propertygrid-v1` keeps v1; v2 is 3 commits on `de5508362` (`2a97d0fad` move to
`AGS.Controls/PropertyGridExtras`, 16 renames, `EditorTypeNames` with `, AGS.Controls`,
`PaintColorBlock` as a static on `ColorUIEditor` because an `AGS.Controls.Utilities` would shadow
`AGS.Types.Utilities`; `a816df141` usings; `59fe1aab6` tests, `AGS.Editor.Tests` now references
`AGS.Controls`). `GUIController`/`FontsComponent` differ from upstream only by `using AGS.Controls;`;
nothing `internal`; `AGS.Controls.csproj` gains `System.Design` for `FileNameEditor`. Verified by the
supervisor: 69 files, +413/-219, no `Factory`/`GUIController` reference under
`AGS.Controls/PropertyGridExtras`, NUnit 382 of 382 (agent's run). Not verified: the running editor
on v2, the Debug build. Lesson for the fork's 6.0 step 1: the same layout (editors in
`AGS.Controls`, delegates kept).

*Design review of v2 (2026-11-04):* the user built and tried v2 (no issue), then asked for a second
design pass. Remaining smell, left for #1092 as the next step rather than this PR: the model still
names its UI editors (`EditorTypeNames`, 112 `[Editor]`, 11 `[PropertyTab]`); the cleaner split is
semantic attributes in the model and a host-side `TypeDescriptionProvider` mapping them to
`UITypeEditor`s, which also removes the `System.Drawing 4.0.0.0` pin in the base-type string. Applied
to the move commit: the constant `EditorAssembly` renamed `ControlsAssembly`; `PaintColorBlock` first
went into its own `AGS.Controls.ColorSwatch` class, then the user asked for it to **stay in
`AGS.Types.Utilities`** as upstream has it (a one-method class is not worth a reviewer's question),
so `Utilities.cs` and `PaletteEditor.cs` are back to upstream's text plus the `StringListSeparators`
property, and `ColorUIEditor` calls `Utilities.PaintColorBlock`. Not done on purpose (pre-existing,
not part of this change): the `MultilineStringUIEditor.cs` file name vs the `MultiLineStringUIEditor`
class. Final commits `b622391cd` move, `553a83a4a` usings, `11936b3ef` tests; 68 files,
+373/-180, 16 renames. Found on the way: upstream's `Utilities.cs` is LF with 14 CRLF lines, and
the worktree has `core.autocrlf=true`, so both the Edit tool (normalising to CRLF) and `git add`
(stripping CRLF) produce a spurious 14-line diff; the file was rebuilt from upstream's bytes with a
script and staged with `git -c core.autocrlf=false add`. The agent's `build.sh` passes a POSIX path
to the NUnit runner under `MSYS_NO_PATHCONV=1` ("Invalid argument"): run `nunit3-console` with a
Windows path separately.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

  *Status (2026-11-04, details in LOG.md):* branch `ags4-types-propertygrid` cut by a Sonnet 5.5
  subagent, Release x86 builds, NUnit 382 of 382; the user checked the property grid in the running
  editor (properties panel and Settings) and opened the PR. First review the same day: the classes go
  to `AGS.Controls`, not `AGS.Editor`, and controls must not call `GUIController` directly (upstream's
  static delegates stay). Reworked as 3 commits on `de5508362` with the editors in
  `AGS.Controls/PropertyGridExtras`; awaiting the force-push and a re-check of the grid.

### 1.3 Engine unit tests for the riskiest subsystems

*Definition, moved from PLAN.md on 2026-11-05:*

**1.3 Engine unit tests for the riskiest subsystems**, in order: script VM (`ScriptExecutor` on
`script2` bytecode), route finder, overlay/GUI layout arithmetic, savegame components, string
formatting (`scsprintf`), sprite cache eviction. A subsystem that reads globals gets a fixture
that sets them up: the first step towards phase 5's context object.
*Done when:* each has a test file; engine test files >= 15.

*Status (2026-11-05):* done on branch `1.3-engine-tests` (15 commits on `5aaa1e65c`), not merged,
not pushed. An Opus 5.5 subagent (xhigh, 2 h 30 min, supervised every 20 minutes through a progress
log kept in `.compare/helpers/t13/`) did it in the plan's order, one tests commit per subsystem:
`script_vm_test.cpp` (25 cases, scripts compiled in memory: `engine_full_test` now builds
`Compiler/script/*.cpp`), `script_bytecode_test.cpp` (19, hand-assembled bytecode; 2 only with
`DEBUG_CC_EXEC`), `managed_pool_test.cpp` (9), `route_finder_test.cpp` (18 + 1 disabled, paths
checked against a test-side Dijkstra), `movelist_test.cpp` (19), `overlay_test.cpp` (14),
`gui_layout_test.cpp` (12), `savegame_components_test.cpp` (7), `savegame_file_test.cpp` (8),
`sprite_cache_test.cpp` (9), `scsprintf_test.cpp` extended 1 → 11. Every suite that reads engine
globals has a fixture named after the subsystem, listing the globals it touches in one place and
resetting them in `TearDown`. Measured: `Engine/test/*.cpp` 6 → 16 (15 with tests), `engine_test`
4 → 51 (+1 disabled), `engine_full_test` 8 → 111 (2 skipped in Release), ctest 881 → 1031 of 1031;
mutation checks (SUBREG as add and the div-zero check off: 5 VM tests fail; diagonal steps allowed:
the corner test fails). Final `prepush.py`, all stages with emsdk: format 1979 files, build + ctest,
headers, libc++ 340 headers, MSBuild 6 solutions + NUnit, all pass; `compare_objects.py
ags4draconianr21 HEAD`: 602 objects identical, only the fixed files' objects differ. Seven engine
bugs fixed, each its own commit with a test that fails without it, all present in upstream `ags4`
(U.7 candidates): `RuntimeScript` destructor compared the signed link index with `0u` (slot 0 never
freed, `_linkedScripts[-1]` read on a failed creation); link index stored as `uint8_t` (257th script
got index 0); `CalcPartsFromProgress` gave a backward run 0 steps (progress inf/NaN, speed change
jumped the walker back); `GetGUIAt(kHit_Any)` used the unscaled rect; `ReadComponent` read through a
null handler for an unselected component (crash via `OPT_SAVECOMPONENTSIGNORE` or a retry);
character inventory quantities saved in `unordered_map` order (save-restore-save bytes differed);
`ResourceCache::DisposeFreeItems` disposed the locked section and kept the free items, erasing nodes
while iterating (`clear_cache_on_room_change` freed nothing). Reported, not fixed: the 3.6.2
`FindNearestWalkablePoint` skips the bottom row near the top edge (disabled test; changes where
existing games place characters); `ccCompileText` section-name condition inverted (compiler,
upstream too, hidden by the preprocessor's section markers); `String.Format("%*d", ...)` passes `*`
to `snprintf` (UB); `kSaveRestore_AllowMismatchExtra` would write past `game.chars` (never set);
no opcode-range or code-end check in Release; joint RTTI matches types by section + name without a
layout check and never resets (a `RunAGSGame` edge case); `GUIControl::IsOverControlImpl` margin
only left/top (callers pass 0); the one-argument `SpriteCache` constructor leaves the callbacks
empty (unused); scaled-graphic AABB spans corner pixel origins; `WorldToLocal` truncates toward
zero; the Repeat flow never shows the last point (pinned as current behaviour). Portability: no
`std::filesystem` (macOS 10.9 target), assert-tripping cases guarded with `NDEBUG`, new files compile
with emsdk clang + libc++. Not verified: GCC/Linux/macOS runs and CI, a Debug build, ASan, the
auto-test game, the private corpus; the new tests are CMake-only like 1.2's (`Tests.sln` unchanged).
Supervision: no correction was needed in seven checks; the agent added `managed_pool_test.cpp`
beyond the six subsystems on its own.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

- [x] **1.3 Engine unit tests for the riskiest subsystems** (2026-11-05, branch `1.3-engine-tests`,
  15 commits, merged): 6 → 16 test files, 12 → 162 engine test cases, ctest 1031 of
  1031; seven engine bugs fixed on the way, each with a failing-then-passing test, all present
  upstream too (LOG.md).

### U.7 The 1.3 engine fixes upstream

*Definition and status, 2026-11-05:* the seven engine bugs fixed by task 1.3 offered on
`upstream/ags4` as branch `ags4-fixes-1.3` (7 commits on `6ba515a06`, ledger in `docs/UPSTREAM.md`),
done by a Sonnet 5.5 subagent (medium, 7 min). None of the fixed files had changed upstream since
the fork's base. Three ports needed hand-application (`movelist.cpp`, `gui.cpp`, `characterinfo.cpp`:
whitespace and include differences); the supervisor compared the branch's and the fork's hunks
ignoring whitespace: identical logic in all six files. Tests: upstream's `engine_test` links a few
engine sources and has no `engine_full_test`, so only the two Common-level tests were ported, written
fresh against upstream's API (`CharacterInfo::WriteToSavegame` into a `VectorStream`; a
`ResourceCache<int,int>` subclass for `DisposeFreeItems`), both checked to fail without the fix; the
five engine-side fixes cite their reproduction in the commit body and keep their tests in the fork.
Verified at the tip only (MSVC Ninja, `AGS_TESTS`): `common_test` 104, `compiler_test` 563,
`tools_test` 77, `engine_test` 3, `savegame_test` 6, run directly, not through `ctest`; `cxx11.py`
clean on every changed file. Not verified: each commit alone, GCC/clang/macOS, a game, clang-format
(upstream does not enforce it). Open question for the PR: whether upstream wants an
`engine_full_test` target (offered as a separate PR in the notes).
Opened as PR #3193 on 2026-11-05 after the user ran a game through the fork's editor and engine built
from `1.3-engine-tests` (`.compare/msb-13`, Release x86, and the CMake `ags.exe` in `.compare/build-13`;
the fresh worktree needed the NuGet `Solutions/packages` folder and the irrKlang DLLs copied from the
main checkout): no evident issue.

### 6.7d The editor builds through the tools

The definition and the 13 steps are in PLAN.md (the task is open).

*Status (2026-11-07): done, 13 steps in two days (6.7d-1 to 6.7d-8 on 2026-11-06, 6.7d-9 to 6.7d-13 on
2026-11-07).* Split 2026-11-06: three read-only subagents took inventory (the
editor's build flow and every subscriber of its build events; what the editor's build does that
`agsbuild` does not; how the tools are built, shipped and run in CI), the supervisor measured.

**Baseline.** The reference editor is the tag `pre-6.7d` built with MSBuild (Release x86; locally
`.compare/msb-pre67d`, with `acwin.exe` next to it).

| Project | Files | Result | Editor | `agsbuild` |
|---|---:|---|---:|---:|
| auto-test (`ags4-draconian`) | 59 | all identical, auto-test 2414 of 2414 | 4 s | 1 s |
| a private AGS 4 game (53 rooms, 36 scripts, a translation) | 114 | all identical | 12 s | 3 s |

A full `agsbuild` run is faster than the editor's build, so the switch needs no incremental build.

**6.7d-1 Parity matrix.** Both projects are debug-mode games without voice files, split packages or
loose files to pack, so `compare_builds.py` got variants: copies of a project that use such things
(`--variants all`, `--list-variants`; a variant can also build in a folder whose name is not ASCII).
It unpacks `*.vox` as well now, and a build that fails is no parity. The auto-test project against
the reference editor:

| Variant | Result | What differs |
|---|---|---|
| `release` (DebugMode off) | no parity | `game28.dta`: `agf2dta` writes the room names only in debug mode, the editor always |
| `split` (SplitResources 1) | no parity | `agsbuild` does not split; the editor's second part is named `.1D3` (`i.ToString(".D3")`, a bug) |
| `opengl` (default driver) | no parity | `acsetup.cfg`: `OpenGL` instead of `OGL`, which is what the engine accepts |
| `customcfg` (`acsetup.cfg` in the project) | no parity | the project's file is not merged |
| `packed` (`preload.pcx`, `flic*`, `*.ogv`) | no parity | the four files are not packed |
| `speech` (voice files, a sub-folder) | `agsbuild` fails | "failed to prepare asset library": a folder without voice files |
| `customdirs` (nested, with sub-folders) | `agsbuild` fails | cannot create the nested temporary folder |
| `audiovox` (a clip in `audio.vox`) | parity | |
| `noname` (empty GameFileName) | no parity | `.ags` instead of `<folder name>.ags` |
| `unicode` (game name and path outside ASCII) | `agsbuild` fails | project file not found: narrow `argv` on Windows; `agspak` cannot unpack the editor's package for the same reason |

**Gaps read from the code, not yet measured** (each belongs to the step named):
- Voice packs (6.7d-3): assets lose the `<sub-folder>/` prefix the engine looks up, pack names keep
  the folder's letter case (`sp_French.vox`, the editor writes `sp_french.vox`), `*.flac` is not in
  the filter.
- Custom data folders (6.7d-3): no check for restricted folders, `.`/`..` or duplicates, no warnings;
  the include file's patterns are `.gitignore`-style, so a file name with `#`, `!` or `[...]` is
  misread.
- `audio.vox` (6.7d-3): one that no clip needs any more stays in the output folder; a clip with
  neither source nor cache file is skipped without a message.
- `agfexport game-cfg` (6.7d-2): the mouse speed is written with `%.f`.
- Script headers of editor plugins (6.7d-6): `__Plugin_<dll>.ash` and the headers C# plugins add
  through `GetScriptHeaderList` are not passed to `agscc`.
- 256-colour games (6.7d-5): `crmbuild` refuses them (`room_build.cpp`, `LoadRoomImages`).
- `--build-debug`/`--build-release` reach only `agscc`, not `agf2dta` or `agfexport`; the editor uses
  the project's setting alone, so the switch does not need them (open, 7.1).
- `NEW_KEYINPUT_API`: `agsbuild` never passes `-foldkeyinput`. Not a gap in practice:
  `Settings.UseOldKeyboardHandling` is `[Obsolete]` and not saved, so it is false in every project
  the tools read.
- Legacy text encodings: no work (the user, 2026-11-06: `ags4` has no ASCII mode, everything is UTF-8).

**What the inventories changed in the plan:**
- `Tasks.RegenerateScriptHeader` cannot be deleted: besides the build it feeds autocomplete,
  go-to-definition and `IAGSEditor.GetAllScriptHeaders`. It stays, with a drift test against
  `agfexport autoash`. `Utils/ScriptGeneration.cs` is not header generation at all (it inserts event
  handlers).
- `DataFileWriter` has users outside the build: templates (`Tasks.CreateTemplateFile`), the `.tra`
  writer and its two menu commands, the lip-sync writer (one helper), `speech.vox`, the end signature
  `BuildTargetWindows` appends.
- Rooms are compiled in-process on every room save, on `PreSaveGame` and `PreCompileGame`, and by text
  processing that makes changes (speech numbering recompiles every room). `Script.CompiledData` and
  `Game.ScriptsToCompile` have no reader outside `DataFileWriter` and the room writer; the debugger
  reads nothing from compiled scripts and matches breakpoints by the script names `agsbuild` already
  uses (`Rooms\<n>\room<n>.asc`, `Dialog <id>`).
- `ExtraCompilationStep` has one subscriber (lip sync), `ExtraOutputCreationStep` one (voice packs);
  no editor plugin API exposes a build event except `GetScriptHeaderList`.
- The F5 run starts `_Debug/<name>.exe` (the engine with only `game28.dta` attached) with
  `--runfromide`: rooms, sprites, fonts, audio and voice come from the project's folders. The engine
  needs a package for the game data (`FindGameData` tests libraries only), and loads scripts as
  separate assets from any registered folder: hence the run layout of 6.7d-6.
- The other build targets (Windows, Linux, Web, Android) only copy from `Compiled/Data`; they stay.
- `AGS.CScript.Compiler` stays for `FastString` (autocomplete); only the preprocessor and its 21
  tests go.
- Shipping: `crmbuild` has no MSBuild project in the fork; `agsdefns.sh` is only an embedded resource;
  the installer lists files one by one and CI's editor artifact holds no tool. `/compile` needs a
  desktop session (splash, main form, busy dialog) and exits 0 when the project path does not exist.

**6.7d-2 Game data and configuration** (2026-11-06, supervisor):
- `agf2dta` wrote the room names block only in debug mode, as 3.x did; the engine reads it from every
  game of data version 4.00.00.13 or later (`GameSetupStruct::read_room_names`), so a release-mode
  `game28.dta` from the tools was misread. The block is now always written; the test that expected
  an empty block in release mode was wrong and now expects the same bytes in both modes. A
  release-mode build runs the auto-test, 2414 of 2414, as the editor's does.
- `agfexport game-cfg --base <file>`: the config starts from a copy of that file and the project's
  settings are merged over it (`IniUtil::Merge`), as `BuildTargetBase.GenerateConfigFile` does with an
  `acsetup.cfg` in the project folder; `agsbuild` passes it when the file exists.
- The graphics driver is written as the engine names it (`OGL` for OpenGL, `MakeGfxDriverConfig`);
  the mouse speed with up to 7 significant digits (`%.7g`, as .NET's `float.ToString()`), not rounded.
- An empty or blank `GameFileName` names the package after the project folder, as
  `AGSEditor.BaseGameFileName` does. The harness builds this variant in a folder of a fixed name on
  both sides, since the name is part of the output.
- Tests: `game_utils_test.cpp` (driver name, mouse speed, `show_fps` only in debug mode); 170 tools
  tests, with CMake and with `Tools.Test.vcxproj` (which did not compile `game_utils.cpp` before).

**6.7d-7, first part** (2026-11-06): `crmbuild` builds with MSBuild. The project is the one of the
upstream port branch (`4b17e4b9c`) with the fork's toolset (v143), added to `Tools.sln`. The MSBuild
tool set (Release x86) gives 59 of 59 identical on the auto-test project and runs it, 2414 of 2414.

**6.7d-4 UTF-8 command lines** (2026-11-06; an Opus 5.5 subagent finished what a Fable 5.1 one began
before a usage limit stopped it; the supervisor reviewed the core and merged it):
- `CmdLineOpts::InitProgramArgs(argc, argv)`, called first by all eleven programs: on Windows the
  arguments come from `CommandLineToArgvW(GetCommandLineW())` as UTF-8 (as the engine's
  `GetCommandArg`), and Allegro's file names are set to Unicode (`set_filename_encoding`, as the
  engine and `AGS.Native` do; without it a relative path from a non-ASCII current folder broke in
  `Path::MakeAbsolutePath`). Elsewhere `argv` as it is. `Parse` takes the resulting list.
- `StrUtil::ConvertUtf8ToWstr`/`ConvertWstrToUtf8` wrote the terminator one past the buffer and
  handled no UTF-16 surrogate pairs: a folder named with an emoji reached the child tools mangled.
  `Path::WidePathToUTF8` freed an array with `delete` (above 1024 bytes; the `agscc -H` list is longer).
- The XML files (`Game.agf`, dialogs, rooms) are opened through `ags_fopen` (`AGF::LoadXmlFile`):
  tinyxml2's narrow `fopen` did not find a project in a non-ASCII folder.
- `agsbuild` builds every command line with `ProcessArgs`, which quotes each value by the platform's
  rules (`QuoteWindowsArg`, the C runtime's and `CommandLineToArgvW`'s; `QuotePosixShellArg`, as the
  children run through `/bin/sh -c`): a game name with quotes (`--gamename` of `po2tra`) broke the
  command line before. `PipedProcess` quotes the program's path itself; `MakeEscapedExePath` and
  `EscapePath` are unused now.
- Measured: the variant `unicode` (a folder `projé жд`, a game `Parité жд "ø"`, a translation) has
  parity; `--json` from a non-ASCII project gives valid UTF-8 with the right script names; tools in a
  non-ASCII folder work. Tests: `common_test` 110 → 113, `tools_test` +4 (quoting round trips, an
  end-to-end child process that prints its arguments, `tools_test --print-args`).
- Not compiled locally at the time: the POSIX branches (the Linux stages run before the push).
- Left as it was: a cmd window not on a UTF-8 code page shows the tools' UTF-8 output as mojibake
  (pipes get the right bytes).

**6.7d-3 What is packed** (2026-11-06; an Opus 5.5 subagent finished what a Fable 5.1 one began, then
ported it onto 6.7d-4's command lines in the main checkout; reviewed by the supervisor):
- `Tools/agsbuild/package_files.{h,cpp}` holds the decisions, tested in `package_files_test.cpp`
  (22 tests): the loose files the editor packs (`preload.*`, `flic*.fl?`, `*.ogv`); custom data
  folders checked as `BuildTargetDataFile` does (absolute, `.`/`..`, restricted folders: the editor's
  warning texts, as `--json` warnings; nested or repeated folders skipped; `Array.Sort`'s order,
  emulated for ASCII names), staged into `_temp/_custom` and packed with their paths; voice packs as
  `SpeechComponent` makes them (`speech.vox` from the top level of `Speech/`, `sp_<lower case>.vox` per
  sub-folder with `<folder>/` in the asset names, which is what the engine looks up; `*.flac` too; a
  folder without voice files makes no pack); split packages with `agspak -p` (MiB parts named `.001`,
  decided above), deployed by reading the part names from the package (`TaskDeployPackage`).
- Stale output is deleted after a successful pack: parts beyond the new count, an `audio.vox` no clip
  needs, voice packs the project no longer has. The editor keeps old voice packs (its changed-file
  check finds nothing to rebuild); deleting them was the subagent's decision, accepted by the
  supervisor, so the engine plays no voice the project does not have.
- A clip with neither source nor cache file fails the build with the editor's message
  (`AGF::ReadAudioClips` did not read the clip's name: one line).
- Two older tool bugs fixed on the way: `agspak -p` wrote broken split packages (the first part's table
  was written before the later parts' offsets were known, and the later parts were checked as if they
  had one); the include patterns had no root anchor (`file1.txt` also matched `CustomData/file1.txt`)
  and `ClipMid` stood for `Mid`, which broke every `[...]` class.
- Measured: `packed`, `customdirs`, `split`, `speech` have parity (63, 62, 61, 67 files); the split
  build runs the auto-test (2414 of 2414; a room and most modules sit in later parts); three builds
  into one folder remove the stale parts and packs. `tools_test` 174 → 199.
- Not reproduced: the editor's incremental checks for the packs (`agsbuild` always rebuilds them, 3 s
  for the private game), its 8.3-name wildcard matches, culture-specific lower case for `sp_` names.

**6.7d-6 Headers from outside, run layout, `_obj/`** (2026-11-06; an Opus 5.5 subagent in its own
worktree, reviewed and merged by the supervisor):
- `--header <file>`, repeatable: after the built-in, auto-generated and global variables headers,
  before the module headers, for game and room scripts, as the editor orders them; a section is named
  by the file's name, so `__Plugin_<dll>.ash` gives the editor's names. Checked end to end with a fake
  plugin header (undeclared without it; the sections in the editor's order).
- `--data-only`, for the editor's Run: exports, scripts, `game28.dta`, rooms, translations and
  `acsetup.cfg`; a package with only `game28.dta` and the scripts; the rooms in `<out>/Rooms/`
  (stale room files there deleted). Measured with the reference engine: folders always win over
  packages (`kAssetPriorityDir`), and the `--runfromide` folders are searched in their order, then the
  install folder. With `Compiled/Windows` as the install folder (as today) a fresh project fails 2 tests
  (`FillDirList` finds no config there) and a stale `room<n>.crm` in the project folder would be loaded;
  so the install folder is `_Debug` and `_Debug/Rooms` comes first (decided by the supervisor:
  the run needs no earlier full build, and a game writing to `$INSTALLDIR$` writes into `_Debug`, not
  into the distribution folder). 2414 of 2414 on a fresh copy with a broken stale `room1.crm`. The
  editor's command line (run in the project folder):
  `<engine> [--windowed|--fullscreen|<log groups> --enabledebugger <id>] --runfromide "<proj>\_Debug"
  "<proj>\_Debug\Rooms":"","<proj>":"","<proj>\Fonts":"","AudioCache":"audio","Speech":"voice"
  "<proj>\_Debug\<name>.ags"`.
- `--temp-dir` defaults to `<project>/_obj`, cleared at the start of a build (only that default, never
  a given `--temp-dir`) and left afterwards; `-c` deletes it after a success. The build refuses to
  start when the output folder or a header lies inside it. `Directory::DeleteDirectoryTree` (new,
  Common): links and junctions are deleted as links, never followed. A build failing halfway leaves
  nothing new outside `_obj/` and the output folder (measured).
- The `--json` defects 6.7d-8 found: agsbuild has its own parser of a table of known options
  (`commandline.cpp`; the other tools keep `CmdLineOpts`), an unknown option, a value glued to an
  option or a missing value is an error naming the argument, as JSON when `--json` is anywhere;
  a task that succeeds reports its `Error:` lines as warnings (`spriteimport`'s non-fatal messages are
  `Warning:` now, with the reason on the line); a message ending in `:` takes the next line; a failed
  task always has a message (`Task::Run` never stored its result); the tools inherit only their two
  pipe ends (`PROC_THREAD_ATTRIBUTE_HANDLE_LIST`). Each with a test that fails without its fix.
- Tests: `common_test` 113 → 117, `tools_test` 199 → 218. All ten variants have parity, the auto-test
  2414 of 2414, the private game 208 of 208 (the harness unpacks the voice packs now).
- Found, not fixed: `File::DeleteFile` on Windows tests `errno` after `DeleteFileW`, which does not set
  it, so most failures look like success; `String::FindAnyChar(const String &)` searches for the whole
  string; `Path::IsSameOrSubDir` takes a parent without a trailing slash for a file; on POSIX the tools
  still inherit every descriptor.
- For the editor (6.7d-9 to 6.7d-11): write plugin headers outside `_obj`, pass no `--temp-dir`, keep
  `AudioCache` refreshed before a run, exclude `_obj` from templates and the upgrade backup
  (`GetPatternsForStandardGameFiles`), let Clean delete it.

**6.7d-8 `BuildToolRunner`** (2026-11-06; an Opus 5.5 subagent, after a Fable 5.1 one stopped at a
usage limit with two files written): `Editor/AGS.Editor/Build/`, namespace `AGS.Editor.Build`, C# 6,
no WinForms, `System.Drawing` or editor singletons. `BuildToolEvent.Parse` (one `--json` line, null for
anything else, never throws); `BuildToolOptions.ToArguments` (one `QuoteArgument` by the
`CommandLineToArgvW` rules, `--header` and `--data-only` for 6.7d-6); `BuildToolRunner.Run`: stdout
and stderr read on their own threads, the progress callback called on the caller's thread in order,
diagnostics into `CompileMessages` (duplicate warnings by file, line and text dropped), outcome
Success/Failed/Cancelled from the `end` event and Crashed/ToolNotFound otherwise, each with an error
message. Cancel writes `cancel`, then after a grace period (5 s) ends the whole Windows job object the
tool was put in at start (kill-on-close, so the tools die with the editor too; `taskkill /T` if
Windows refuses the job). Tests: 396 → 522 editor tests (126 new: events 58, options 49, runner 16
against batch-file processes, 3 against the real `agsbuild` when `AGS_TEST_PROJECT` names a project);
the kill tests were checked to fail with the job and `taskkill` disabled. The repository's
`**/build` ignore line (for Xcode) hid the new folders on Windows (`core.ignorecase`): two negations.
Found in `agsbuild`:
- With `--json` it did not exit after the build while its stdin stayed open: the thread waiting in
  `std::getline(std::cin)` held the stream's lock, which the C runtime takes at exit; `cancel` after
  the end crashed it. Fixed by the supervisor: the thread reads with `ReadFile`/`read`. Measured with
  stdin held open (`.compare/helpers/stdin_open_check.py`): exit 0 after the build, 3 of 3; `cancel`
  still gives exit 2, 3 of 3. The runner's workaround (closing stdin when the last task is done) can go.
- The rest goes to 6.7d-6 (unknown options, no JSON for bad arguments, errors from succeeded tasks,
  empty error messages, inherited handles, `_temp` never removed).

**6.7d-7, CI and installer** (2026-11-06): the editor's build copies `agsdefns.sh` next to it; the
editor job takes the x86 tools from `windows-msvc-tools` and fails if one is missing; `ags.iss` lists
them. Confirmed on the probe branch `probe-67d7` (run 42213937471: tools, editor, and a job checking
that every editor file the installer lists is in the editor artifact). The probe's Verify run first
timed out in `apt-get` (Ubuntu 26's mirror at a few KB/s), then failed the Linux clang-tidy gate on one
6.7d-4 line (`wchar_t` is signed on Linux: `bugprone-signed-char-misuse`), which the Windows tidy stage
of `prepush.py` cannot see; fixed, and the gate run in WSL against the CI's base is clean
(`~/venv` has clang-tidy 19.1.0 now). Open: `prepush.py` could run that Linux gate in its `linux`
stage. The installer itself was not built (it needs the full run's artifacts).

**6.7d-5 256-colour rooms, and what the first real dialogs showed** (2026-11-06, supervisor; the
corpus is a private 8-bit game: 9 rooms, two with several backgrounds, 6 dialogs, 2 modules):
- `crmbuild` built no room of an 8-bit game. `LoadRoomImages` now does what the editor does when it
  loads a background (`RoomsComponent.LoadBackground`, `PaletteUtilities.RemapBackground` with the
  exact palette): in a room saved as 8-bit (`ColorDepth` 8 in `data.xml`) an indexed image's palette
  takes the game palette's game-wide colours, and only in the slots the image's palette has (a PNG
  with 20 colours leaves the game-wide slots 20 to 41 black; the PNG reader gives no palette length, so
  `room_build.cpp` reads the `PLTE` chunk's). An 8-bit background keeps its palette in its frame, and
  the first frame's is the room's (`AGS.Native SetNativeRoomBackground`, `PrepareForWriteToFile`).
  The rule does not depend on the game: a true-colour game converts such a room with those colours.
- The editor reads PNGs with its own loader (`BitmapHelper.LoadBitmap`: GDI+ gives an 8-bit PNG with a
  `tRNS` chunk as 32-bit, so the chunk is taken out and put back as palette alpha); room files hold
  no alpha, so nothing follows from it.
- Dialogs: the auto-test's are bare entry points and the other private game has none, so
  `dialogscriptconv.cpp` had never met a real one. (1) Double quotes in speech were not escaped
  (a `TODO` since the port): `agscc` failed on the line. (2) The editor writes the function's
  opening on the first line and one line per line of the dialog script, empty ones too, so the
  compiled script and the compiler's messages carry the dialog script's line numbers; the tool put
  the opening on its own line and dropped empty lines (the first statement was line 2 instead of 25).
  (3) `agf2dlgasc` printed messages only when there were none (`if (errors.empty())`), so a line with
  an unknown character was dropped and the build went on; it now prints
  `Error: at Dialog <id>, line <n> : <text>` (the editor's script name for a dialog), converts every
  dialog, and fails. (4) `this` next to a non-ASCII byte went through `std::isalnum` with a negative
  value; a character named `CAnn` was not found as `ann`.
- `<ScriptModule />` on a character or an inventory item (the game has it on all 33): the editor
  reads an empty name, the tool kept its default `GlobalScript.asc`. Now an empty element is an empty
  name and only a missing one leaves the default; a GUI's default is `GlobalScript.asc` too, as in
  the editor.
- The harness: `set_element` did not match an element with attributes (`<CustomDataDir
  IsNull="True" />`, a null string), so `customdirs` stopped on this project.
- Measured against the reference editor: the 8-bit game, 53 of 53 files identical, and parity in all
  ten variants; a copy whose room 0 has a 20-colour palette, 53 of 53 (slots 20 to 255 black in both
  room files); the auto-test game, ten variants and the run (2414 of 2414); the other private game,
  208 of 208. `tools_test` 218 → 225 (`dialogscriptconv_test.cpp` is new).
- Not the same as the editor, and left so:
  - The game palette's background slots in `game28.dta`. The editor's room palette is a shallow copy
    of the game's (`CopyGamePalette`), so loading a background overwrites the game's background
    colours with that image's: the editor writes the colours of the room it loaded last, and saves
    them into `Game.agf` with the next save. The tools write what `Game.agf` holds. Measured on a copy
    of the auto-test game with an 8-bit background: the room files are identical, `game28.dta` differs
    in those 214 slots and nowhere else. The 8-bit game has parity because its `Game.agf` was saved
    after a full rebuild.
  - A character or item without any `<ScriptModule>` element (a hand-edited file; formats before
    4.0.0.24 have it inside `<Interactions>`, which is read): the editor writes an empty module name
    for its cursor events and `GlobalScript.asc` for the others, the tool `GlobalScript.asc` for
    both. The engine takes an empty name for the global script (`GetScriptInstance`).
- Helpers, local: `.compare/helpers/mk_8bit_cases.py` (the derived projects), `cmpdta2.py` (the two
  `game28.dta` block by block, scripts taken out), `which_script.py`.

**6.7d-9 Build through `agsbuild`** (2026-11-07; an Opus 5.5 subagent):
- `AGSEditor.BuildGame(forceRebuild)` serves Build EXE, Rebuild all, `/compile` and the new-game wizard (the
  callers no longer save or compile themselves): object IDs repaired, then the project saved (the order
  agsbuild needs, as it reads the files: the editor's build repaired the IDs after the callers' save),
  `PreCompileGame`, `RunPreCompilationChecks`, the script checks (`CheckGameScripts`), then one busy dialog
  that deletes the targets' old game files (also those of an earlier game file name, as before), runs
  `agsbuild` from the editor's folder into `Compiled/Data` (`BuildToolRunner`, default `_obj`, no
  `--temp-dir`) and deploys the other selected targets, which copy from there. User data saved and
  `RequiresRebuild` cleared after a success. F5 still takes the editor's build (`CompileGame(false, true)`,
  6.7d-10); its other branch is unused now and goes in 6.7d-12.
- `PreCompileGameEventArgs.BuildsWithTools` (only until the Run switches): rooms keep the open-room save and
  the missing-file checks but are not compiled, translations are not compiled (`po2tra`). The editor's
  room compile was also its only check of the rooms' event handlers ("looks like an event handler, but is
  not linked on Room N's Event pane"): the agsbuild build loads every room and scans it
  (`ScanAllRoomsForMissingEventHandlers`), so every build reports what only a full rebuild reported.
  The lip-sync data (`Speech/syncdata.dat`, a source of the voice packs) is made in `PreCompileGame`, for both
  builds; `ExtraCompilationStep` has no subscriber left, and `ExtraOutputCreationStep` (the C# voice packs) is
  not raised by the agsbuild build (its check, `CheckSpeechFilesForConsistency`, is empty).
- Plugin headers (`GetScriptHeaderList` without `_GlobalVariables.ash`, which agsbuild makes) are written by
  `Build/ScriptHeaderFiles.cs` to a new folder in `%TEMP%` (each in a sub-folder, under its own name, which
  agscc takes as the section name) and passed as `--header`; deleted after the build (7 NUnit tests).
- The busy dialog has an optional Cancel button (and Esc) and a progress bar when a total is known; the
  build shows `agsbuild`'s task and count ("agscc:Defs.asc (14 / 215)"). The tools' information lines are
  not listed (agspak's "found pattern file" for the lists agsbuild writes); errors and warnings are.
- `!_obj/*` in `GetPatternsForStandardGameFiles` (templates, upgrade backup). Rooms are no longer loaded and
  re-saved in the current format by the rebuild that follows an upgrade of the room format: `crmbuild`
  reads the older room formats itself (`Transparency` before 4.0.0.34).
- `compare_builds.py --tools-editor <AGSEditor.exe>`: the second side is that editor's `/compile`, so the
  reference editor (scripts embedded) is compared with this one (scripts separate) directly.
- Measured, reference editor (`pre-6.7d`) against this editor, both MSBuild Release x86:

  | Project | Default | Ten variants |
  |---|---|---|
  | auto-test | 59 of 59 identical | parity in all ten |
  | a private AGS 4 game | 208 of 208 identical (reference 12 s, this editor 7 s before the room scan) | parity in all ten |
  | a private 8-bit game | 53 of 53 identical | parity in all ten |

  The final binary (with the room scan) was run again on the auto-test game (59 of 59), the private AGS 4
  game (208 of 208; its 12 room event-handler warnings are the reference's 12) and its `unicode` variant;
  the other variants ran with the binary before the scan, which changes no output file. The `unicode`
  variant had never run on that game: the harness could not add a translation to a project that has
  some (`<Translations>` with children), fixed.

- Errors, by `/compile`'s console (one error put into a copy of the auto-test game each time): both editors
  report `GlobalScript.asc` line 5, `Rooms\1\room1.asc` line 4 and `Dialog 0` line 3, the names the output
  panel hands to the same `ZoomToFile` handlers (scripts by file name, rooms by the `Rooms\<n>\room<n>.asc`
  pattern, dialogs by `Dialog <n>`). The click in the output panel was not tried.
- Cancel, by UI Automation (Cancel pressed 0.5, 1, 1.5 and 3 s into the private game's build): the editor
  ends 0.25 to 0.8 s after the press with "The build was cancelled.", exit code 1, no tool left running.
- Messages that differ from the reference editor's: the room compiler's warnings now reach the output
  ("Code execution cannot reach this point" in three rooms of the private game: the editor's room compile
  threw them away, `SaveRoomOnThread` looks at its messages only for errors); "Room room9.crm was saved, but
  there were warnings" is gone with the room compile.
- Left for 6.7d-10 and 6.7d-11: F5 and Ctrl+F5 still build with the C# code (and still repair the IDs after
  the save); saving a room still compiles it and writes `room<n>.crm` into the project folder, which nothing
  reads now; the room event-handler scan moves to `CheckGameScripts` when the editor's room compile goes.
- The user's first Rebuild all showed one "Process failed with err code: 2" per script: their editor folder
  had the tools of `Tools.sln` but not `agscc.exe`, which only `Compiler.App.sln` built, and `agsbuild`
  reported each failed `CreateProcess` with the system's code alone (2, `ERROR_FILE_NOT_FOUND`). Fixed three
  ways: `Tools.sln` now holds `Compiler.App` and `Compiler.Lib` (the `Compiler.App.sln` steps of `ci.yml` and
  of `prepush.py`'s msbuild stage are gone, `Compiler.App.sln` itself stays); `agsbuild` checks every tool it
  may start before it starts any (`CheckBuildOptions`: "not found in the system directory '...' (--sys-dir):
  agscc", one diagnostic, which the editor lists as the build's one error); the editor needs no list of its
  own, `agsbuild` is the one that knows which tools a build takes.

**6.7d-10 Run through `agsbuild`** (2026-11-07; an Opus 5.5 subagent):
- F5 and Ctrl+F5 call `AGSEditor.BuildGameForRun`, which takes `BuildGame`'s steps (IDs repaired before the
  save, `PreCompileGame` with `BuildsWithTools`, the checks, plugin headers, the busy dialog with progress
  and Cancel, user data saved on success) and runs `agsbuild --data-only` into `_Debug` instead of building
  and deploying the targets. Before it, the `_Debug` target deletes `<name>.ags` and the `<name>.exe` that
  earlier editors made there, also for the previous game file name; the editor checks first that the engine
  is next to it.
- The engine is the editor's own `acwin.exe`, started in the project folder with 6.7d-6's command line:
  `--runfromide "<proj>\_Debug" "<proj>\_Debug\Rooms":"","<proj>":"","<proj>\Fonts":"","AudioCache":"audio",
  "Speech":"voice" "<proj>\_Debug\<name>.ags"`, after the same window or debugger options as before. Nothing is
  copied into `_Debug` any more (the engine as `<name>.exe` with `game28.dta` appended, `SDL2.dll`, the
  plugins): the engine looks for plugins in its own folder first, where the editor took them from, and
  reads `acsetup.cfg` next to the game file. `Compiled/Windows` is no longer the install folder of a Run,
  so a Run needs no earlier Build EXE.
- Unused now, deleted in 6.7d-12 (added to its list): `CompileGame`, `CreateMiniEXEForDebugging`,
  `CreateCompiledFiles`, `BuildTargetDebug.Build`, and `PreCompileGameEventArgs.BuildsWithTools`, which only
  that dead path passes as false (its comment says so now).
- Verified with the real editor driven by UI Automation (a local script opens the project, posts F5 or Ctrl+F5
  to its window with Ctrl held in the thread's key state, dismisses the editor's message boxes, and reads the
  engine's command line from the process list), on fresh exports of the auto-test project:

  | Run | Result |
  |---|---|
  | Ctrl+F5, this editor, twice | 2414 of 2414 each time, engine exit code 0 |
  | F5 (`--enabledebugger`), a broken `room1.crm` in the project folder | 2414 of 2414; the stale room is never loaded |
  | Ctrl+F5, the reference editor (`pre-6.7d`) | 2411 of 2414 (the three `FillDirList` tests: `Compiled/Windows` as the install folder, as 6.7d-6 found) |
  | F5 on a copy with an array index error at line 11 of `room1.asc` | the editor goes to `[Break]`, shows "Error running function 'room_AfterFadeIn': Error (line 11): Array index out of bounds" and opens `room1.asc` |

  What the engine is given: the reference editor's run (`game28.dta` in the mini EXE, un-embedded as
  `compare_builds.py` does, the rooms it compiled into the project folder, `_Debug/acsetup.cfg`) against this
  editor's (`_Debug/<name>.ags`, `_Debug/Rooms`): `game28.dta`, the 40 scripts, the 3 rooms and
  `acsetup.cfg`, 45 of 45 files identical. The script section names the debugger matches breakpoints
  against are in those bytes, so breakpoints, the call stack and watches see what they saw before.
- **Cost, for the user to decide:** the old Run compiled only the rooms whose script had changed and wrote
  one `game28.dta`; `agsbuild` builds every room each time. The build before a Run, from the key to the
  "compiled" message, both editors MSBuild Release x86, two runs each on a warm project:

  | Project | Reference editor | This editor | `agsbuild --data-only` alone |
  |---|---|---|---|
  | auto-test (3 rooms) | 1.7 s | 2.0 s | |
  | a private AGS 4 game (53 rooms, 36 scripts) | 1.9 s, 1.8 s | 8.4 s, 8.8 s | 5.3 to 7.6 s (5 runs) |

  Incremental room builds in `agsbuild` (a room whose sources and headers did not change keeps its
  `room<n>.crm`; `_obj/` is meant for it, 7.1k) would win this back. The C# Run stays in the code until 6.7d-12.
- Also seen: the private game now gets "compiled with warnings" on every Run, where the reference editor's
  warm Run said "compiled successfully": the room compiler's warnings and the room event-handler scan
  (6.7d-9) are reported each time, not only when a room is recompiled.
- Not verified here (the user, by hand): a breakpoint set in a script editor, Step into, Pause, the call stack
  panel's contents and watch variables while paused (the injected error proves the engine-to-editor messages
  and the room script's name, not the editor-to-engine `SETBREAK`); a game with engine plugins (the plugin
  lookup now finds them in the editor's folder instead of a copy in `_Debug`). *The user checked them on
  2026-11-07 on their game: breakpoints, step, call stack, watches, Ctrl+F5 and the renamed game file all
  work.* The Run's cost was decided the same day: 7.1k, redefined as incremental room builds in `agsbuild`,
  goes before 6.7d-11, so the Run is as fast as the editor's before 6.7d-12 deletes the C# Run.

**6.7d-11 The other users of the build code** (2026-11-07; a Fable 5.1 supervisor, four Opus 5.5 subagents and a
Sonnet 5.5 one, each on its own files):
- `Build/ToolProcess` runs one tool that writes text, not agsbuild's JSON (po2tra, agspak, crmbuild): hidden
  process in a job object, stdin closed at once, both streams read on their own threads, the `Error:` /
  `Warning:` / `Info:` lines parsed by agsbuild's own rules (`diagnostics.cpp`: a message ending in `:` takes the
  next line, "`<what> at <file>, line <N> : <msg>`" gives a file and line), a non-zero exit without an error
  line reported with the last ten lines of output. 18 NUnit tests with batch files as the tool.
- Rooms (`RoomsComponent`): a save writes `data.xml` and the modified images only (`SaveRoomOnThread` no
  longer regenerates the header, compiles the script or writes `room<n>.crm`; `SaveCrm`, `SaveRoomDataAndCompile`
  and `HaveScriptHeadersBeenUpdatedSinceRoomWasCompiled` are gone); the autocomplete cache and the event-handler
  scan on save stay. `PreCompileGame` saves the open room, checks that every room's sources exist
  (`CheckRoomFilesExist`, what was left of `RecompileAnyRoomsWhereTheScriptHasChanged`) and scans the event
  handlers; `BuildsWithTools` is not read any more. Write-access checks name `data.xml` instead of the `.crm`.
  `IRoomController.Save` (plugin API) saves the sources, as its doc always said. Visible changes: a room script's
  errors no longer show on Ctrl+R or when a room closes, they come from the build; a room that did not change
  writes nothing.
- Translations: "Compile translation" and "Compile all translations" run `po2tra <po> Compiled/Data/<name>.tra
  --gamename --uniqueid` (`Build/TranslationCompiler`, 5 tests against the real tool, a game name with quotes
  and a path outside ASCII among them); the first error names the source, as po2tra's do not. The C# `.tra`
  writer is now called only by the dead `PreCompileGame` branch (6.7d-12).
- Templates (`Build/TemplatePackage`, 15 tests): the files are copied under their stored names into
  `_obj/template/<random>/` and packed with `agspak -c <pkg> <folder> -r` (asset names with `/`, sorted as the
  editor sorted them); a room template's `room<n>.crm` is built by `crmbuild` from the saved sources into
  `_obj/template-room/` (the game is saved first), `rtemplate.dat` is written there too, nothing is written
  into the project folder any more; a plugin's template files are packed from where they are instead of being
  copied into the project folder (the FIXME, which could overwrite a project file of the same name). The
  auto-test project's template (`/maketemplate`) is byte-identical to the reference editor's: 119 assets.
- The lip-sync writer (`SpeechComponent.CompileLipSyncFiles`) has its own `WriteNullTerminatedString`
  instead of `DataFileWriter.FilePutNullTerminatedString`.
- Measured: the editor's NUnit suite 567 of 567 (563 without the test project, which `AGS_TEST_PROJECT` names);
  `compare_builds.py --tools-editor` on the auto-test project, reference editor against this one: 59 of 59
  identical, and the `unicode` (a translation) and `speech` variants have parity.
- Found and fixed on the way: "Import text" (Make default language) set hotspot and object names and text
  properties without marking the room modified, so since 6.7d-9, when the build stopped reading the
  `room<n>.crm` the save wrote, those changes never reached disk (`AGSEditor_ProcessAllGameTexts` marks the room
  modified; the game is saved before, so the image cache is clean); `GetPatternsForStandardGameFiles` listed
  its excludes (`!_Debug/*`, `!Compiled/*`, `!_obj/*`) before `AudioCache/*`, and a later include wins and
  matches at any depth, so a template and the upgrade backup took `_obj/AudioCache/*` (the first template
  had 124 assets).
- Found, not fixed: `po2tra` accepts any text as a `.po` (`ReadPO` never returns an error; an unterminated
  string gives "did not appear to have any translated lines" and exit code 0), where the editor's writer
  refused what `TryLoadData` could not read; `SaveCrm` wrote every temporary room's `.crm` with the open
  room's images (the single image cache), gone with it. Unused now, for 6.7d-12: `PreCompileGameEventArgs.
  ForceRebuildTime`, `Utilities.DoesFileNeedRecompile(DateTime, string)`, `NativeRoom.SaveToFile`.
- Not verified (the user, by hand): Ctrl+R and closing a room in the GUI; "Save room as template" and a new
  room from that template (a `.crm` without a compiled script through `ConvertRoomFromCrmToOpenFormat`);
  the "Compile translation" menu in the real editor; a plugin that adds template files. *The user checked
  the first three by hand on 2026-11-07: all good.* Only the `msbuild`
  stage applies to this change (Editor only); `prepush.py --auto` also lists the stages of the earlier
  unpushed commits.

**6.7d-12 Deletions** (2026-11-07; an Opus 5.5 subagent): eleven commits, in an order where each one builds
and passes the tests (the in-process compile goes after `DataFileWriter`, which read `Script.CompiledData` and
`Game.ScriptsToCompile`; `DialogScriptConverter` and the preprocessor after the compile, their only caller):
1. The editor's build: `CompileGame`, `CreateCompiledFiles`, `CreateMiniEXEForDebugging`,
   `ExtraOutputCreationStep` (raised only there; the C# voice packs with it), `BuildTargetDebug.Build`,
   `BuildTargetDataFile.Build` (only `CreateCompiledFiles` called it), `PreCompileGameEventArgs.BuildsWithTools`
   and `ForceRebuildTime`, the translation branch of `PreCompileGame`, both `Utilities.DoesFileNeedRecompile`.
2. The C# `.tra` writer. 3. `DataFileWriter.cs` (2,557 lines) and an unused copy of `CreateAudioVOXFile`;
   `BuildTargetWindows` (Build EXE with the data attached) keeps the CLIB end signature as its own constant.
4. The room writer in `AGS.Native`: `NativeRoom.SaveToFile`, `save_room_file`, `save_default_crm_file` (without a
   caller at all) and the two helpers only they used.
5. A move: `ScriptCompiler.cpp` also held the live EXE resource functions (`UpdateFileIcon`,
   `UpdateFileVersionInfo`), now at the end of `NativeMethods.cpp`.
6. The script compiler setting has no list (a behaviour change): the list came from the in-process compilers;
   the setting is hidden, still saved, and set to "AGS SCOM 4" on load as before (`agscc` does not read it).
7. The in-process compile: `CompileScript*`, `IScriptCompiler`, `ScriptCompiler.cpp`, `CompiledScript.h`,
   `Script.CompiledData`, `Game.ScriptsToCompile`, `ExtraCompilationStep`, `GetScriptModuleList`,
   `_GlobalVariables.asc`. `IScriptCompiler.h` had put `using namespace AGS::Types` inside `AGS::Native`, which
   `NativeMethods.cpp` relied on (`Native::FontMetrics`); the type is written as `AGS::Types::FontMetrics` now
   (unqualified it is ambiguous with the native `FontMetrics` of `Common/font`), no `using namespace` in a header.
8. `DialogScriptConverter.cs` and `Dialog.CachedConvertedScript`; `IsRealScriptLineInDialog` moved to
   `GameSpeechProcessor`. 9. The C# preprocessor (`Preprocessor`, `IPreprocessor`, `PreprocessorState`,
   `Macro`, `Macros`) and its 22 tests: its one other caller was the in-process compile; autocomplete and the
   script editor never used it.
10. A move: the `_GlobalVariables.ash` text to `Tasks.MakeGlobalVariablesHeader`, next to
    `RegenerateScriptHeader`, so a test can call it. 11. `AgfexportHeaderTests`: both headers against
    `agfexport autoash` and `glvar`, line by line, with `AGS_TEST_PROJECT`. Its first version failed on a private
    AGS 4 game: the editor lists a folder's views before its subfolders, `agfexport` by ID, which the scripts do
    not see; runs of `#define` lines are compared sorted. It passes on the auto-test project,
    `Tools/test/data`, a private AGS 4 game and a private 8-bit game.
- Measured: MSBuild after every step, no warning added (the native code from scratch: 25 warnings before, 24
  after, one moved with its function and one went with `save_default_crm_file`); NUnit 567 of 567 after steps
  1 to 8 less the 22 preprocessor tests, 547 of 547 at the end (541 and 6 ignored without the test project);
  `compare_builds.py --tools-editor` against the reference editor: 59 of 59 identical, parity in all ten
  variants; the auto-test template byte-identical (119 assets); Ctrl+F5 of this editor: 2414 of 2414;
  `prepush.py --auto`: `format` and `msbuild` (five solutions and the editor's NUnit) pass.
- Found and fixed on the way: the first version of step 1 also dropped the C# voice packs' file-time cache, but
  that is written into `Game.agf` (`<SpeechVoxFiles>`, component data), not the user file; the template check
  caught the 77 bytes, and the element stays (read and written back) until the user decides.
- Found, not fixed: `WorkspaceState.RequiredRebuildTime` is saved in the user data and read by nothing;
  `NativeRoom(Room)`, `SetBackground`, `SetAreaMask` and `convert_room_to_native` have no caller outside
  `NativeRoomTests`; `AGS.Native` no longer includes anything from `Compiler/`, so its `Compiler_md.lib` link and
  include path are unneeded; what is left of `AGS.CScript.Compiler` outside `FastString` (autocomplete) has no
  user outside the project.
- Not verified: the intermediate states were built and tested before the `<SpeechVoxFiles>` fix (three lines
  that only keep the element; the final state is); a plugin that used the removed events or types; Linux and
  macOS (the editor is Windows only).

**6.7d-13 Harness, CI, docs** (2026-11-07; the Fable 5.1 supervisor):
- CI's `Windows Build Parity` job had been red since the push of 6.7d-9 to 6.7d-11 (run 42381421451 on
  f4fd487a4: 58 identical, `game28.dta` "different (scripts differ)", both sides 29,203 bytes). The harness's
  plain mode took the scripts out of the editor's `game28.dta` before comparing, and since 6.7d-9 the editor's
  package has them as separate files already, so the search for the embedded section failed and the file was
  reported as different although it was identical. Locally only `--tools-editor` had been run, where the
  reference editor is the embedded side.
- `compare_builds.py`: the un-embedding happens only when the editor's package lacks the separate script files;
  an editor that packs them separately gets the plain byte comparison. The docstring says which editors are
  which. The CI job is unchanged but for its comment: it is the guard that the editor's command line and
  pre-build steps add and lose nothing against a plain `agsbuild` run.
- Measured (the 6.7d-12 editor, `.tidy/build` tools): CI's mode on the auto-test project 59 of 59 identical,
  auto-test 2414 of 2414 on the agsbuild output, parity; every one of the 10 variants parity (`split` 61 files,
  `packed` 63, `speech` 67, `customdirs` 62, `audiovox` 60, `unicode` 60); the old mode against the reference
  editor still 59 of 59, un-embedding as before.
- Docs: AGENTS.md says how the editor builds now and what `docs/context/repository.md`'s `DataFileWriter.cs`
  steps mean for the fork; `docs/context` itself is upstream knowledge and stays. PLAN.md: 6.7d and 7.1l
  done, 3.3 rewritten for what is left (`AGS.CScript.Compiler`).
- Not verified: the CI job itself (the next push confirms it; `actionlint` passed on the edited workflow).

**6.7d-12 follow-up, the three open decisions** (2026-11-07, the user decided, the supervisor did them):
- The script compiler setting is `[Obsolete]`, as `ExtendedCompiler` is: still read, so older projects load, no
  longer written, so it leaves `Game.agf` at the next save; the upgrade step no longer sets it, it only warns
  when a project had selected a compiler other than "AGS SCOM 4" (the legacy "AGS SCOM 3 EXT"), since such
  scripts may need changes for `agscc`. `Tools/data/agf_elements.txt` and the sample `Tools/test/data/Game.agf`
  regenerated through their tests' update switches (`agfreader` keeps the element as known and ignored).
- `<SpeechVoxFiles>`, the deleted C# voice packs' file-time cache, is no longer read or written; it leaves
  `Game.agf` at the next save too. The two templates (reference editor, this editor) differ by those two
  elements alone: 149 bytes of `Game.agf`, the other 118 assets identical.
- `GetScriptModuleList` and the other removed types were hooks into the in-process compile, none of them in
  `IAGSEditor` (its one event, `GetScriptHeaderList`, stays); nothing to restore.
- Found and fixed: `WorkspaceState.RequiresRebuild` (set when a project or its rooms are upgraded on load) had
  not been read since 6.7d-9: the old build OR-ed it into `forceRebuild`, the new build path did not, so an
  upgraded project no longer got the full build it asked for (the content-hash cache of 7.1k and 7.1p covers
  most of it, since upgraded sources hash differently). `BuildGame` ORs it in again, so it reaches
  `agsbuild --rebuild`. `RequiredRebuildTime`, the time stamp that `DoesFileNeedRecompile` compared file
  times with, is dead for good (the manifests replaced the comparison) and is gone from `WorkspaceState` and
  the user file, with the setters that wrote it and the line in `BuildCommandsComponent`.
- Measured: editor NUnit 547 of 547; `tools_test` `AGFReaderSample` 2 of 2 on the new sample;
  `compare_builds.py --tools-editor` against the reference editor 59 of 59 identical; the template as above.
  Not verified: a Debug/Release switch, which used to force a rebuild through the time stamp, now relies on
  the cache's inputs (the flags and `Game.agf` part it hashes).

**Bugs found on the way, not fixed:** `BuildTargetBase.DeleteCommonGameFiles` deletes `filename`
instead of the loop's `fileName`, so old split parts are never removed (still so for `Compiled/Windows`;
in `Compiled/Data` agsbuild removes them); the split part names above;
`RoomsComponent.CopyGamePalette` copies references, so opening a room changes the game palette (6.7d-5);
`AGSEditor.CreateAudioVOXFile` is an unused copy of the one in `BuildTargetDataFile` (both deleted in 6.7d-12);
`Tasks.HandleObjectIDErrors` repaired IDs after the project was saved (fixed for the build in 6.7d-9, for the
Run in 6.7d-10).

Not verified: the gaps listed as read from the code; anything on Linux or macOS (the harness needs
the editor).

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 7.1k Incremental room builds in `agsbuild`

**7.1k Incremental room builds** (2026-11-07; an Opus 5.5 subagent):
- A room built from `Rooms/<n>/` is kept in `_obj/` while nothing it is built from changes. Its entry is
  `_obj/room<n>.crm` and the folder `_obj/room<n>/`, which now holds the room's `_AutoGenerated.ash`, its
  `room<n>.o` (moved there from `_obj/`) and `manifest.txt`. The new task `check-rooms` (after `autoash`, `glvar`
  and `crmbuild:game-key`) makes each room's manifest and compares it with the recorded one; the room's three
  tasks (`crmbuild:…:ash`, `agscc:room<n>.asc`, `crmbuild:room<n>.crm`) get a skip condition
  (`Task::SetSkipCondition`, asked when the task starts: it logs "Up to date" and succeeds), and a new task
  `manifest:room<n>.crm` records the manifest after the room is built. A stale room's `.crm` and manifest are
  deleted by `check-rooms` before anything is built, so a room build that fails or is cancelled leaves nothing
  recorded. The log says "Rooms up to date: X of Y".
- The manifest (`Tools/agsbuild/room_cache.*`) is a text of lines `<kind> <hash> <name>`, compared as a whole: a
  format line; `tool` for `agsbuild`, `agscc` and `crmbuild` (their binaries: the version strings are never bumped);
  `game`, the settings `crmbuild` reads from `Game.agf`; `command` for each of the three command lines without
  `-v` (the agscc flags, the script name, every path, the order of the headers); `header` for every header agscc
  gets, in order (the API header, the generated `_AutoGenerated.ash` and `_GlobalVariables.ash`, `--header`
  files, the module headers); `source` for every file at the top level of `Rooms/<n>/` by name (`data.xml`, the
  images, `room<n>.asc`; an added `background1.png` or a removed mask counts). The room's own `_AutoGenerated.ash`
  is made from the generated header and `data.xml`, both inputs already. Hash: 64-bit FNV-1a over the contents
  (no dependency, the same on every platform; a test checks the published values), each file hashed once per
  build; never a time stamp.
- The `Game.agf` part is the struct `RoomBuildGame` (`ReadRoomBuildGame`: unique ID, default mask resolution,
  colour depth, text code page, palette, cursor events, property schema). `MakeRoomBuildGameKey` writes it as text
  with structured bindings, so a member added to `RoomBuildGame`, `PaletteEntryData` or
  `CustomPropertySchemaItem` is a compile error there until it goes into the key. `crmbuild <GAME.AGF> --game-key
  <OUT>` writes it (`_obj/_RoomGame.key`), so the key comes from the binary that builds the rooms; agsbuild does
  not read it itself because its MSBuild project compiles only `agfreader` and `xml_fields` of `Tools/data`
  (`room_build.cpp` would bring 40 more files). A change of the game's name alone keeps every room (measured).
- `_obj/` is still cleared before a build, but for the room entries that have a manifest
  (`ClearTempDirKeepingRooms`): scripts, `game28.dta`, packs and staged folders start from nothing as before (a
  staged folder is packed with `*`, and a hard link to a project file left there would be written through by a
  later `spriteimport`). `--rebuild` deletes all of `_obj/` (a given `--temp-dir` is never cleared: there it only
  makes every room stale). The run layout's rooms are copied to `_Debug/Rooms` instead of moved (4.4 MB on the
  private game), and `-c` with a given `--temp-dir` deletes them and the manifests too. Old-layout rooms
  (`room<n>.crm` in the project, `AddCompiledRoomTasks`) are unchanged: built every time, their copies in `_obj/`
  are cleared like any other file.
- Editor: `BuildToolOptions.Rebuild` (`--rebuild`); `RunBuildTool` passes `forceRebuild`, so Rebuild all, and a
  Build or Run that the editor marks as needing a full rebuild (debug mode switched, a project upgrade), build every
  room. `Script/compare_builds.py --incremental`: on the agsbuild side the copy is built with one room's script
  changed, the change is undone, then the build that follows (that room rebuilt, the others kept) and one more
  with nothing changed are both compared with the editor's.
- Tests (`tools_test` 225 → 247): `room_cache_test.cpp` (a test per input kind: each
  tool, the game key, each command line, each header kind and their order, room data, images, script, an added,
  removed or renamed room file; a missing input; up to date, stale, missing room or manifest, `--rebuild`; what
  `ClearTempDirKeepingRooms` keeps), `RoomBuild.EverySettingChangesTheGameKey`,
  `TaskManager.SkippedTaskSucceedsWithoutRunning`, `--rebuild` in the command-line tests and in
  `BuildToolOptionsTests`. End to end on a copy of the auto-test (rooms kept of 3): nothing changed 3; room 1's
  `data.xml`, background, an added `background1.png`, script 2; a module header, a `--header` file, the API
  header, `Game.agf`'s unique ID, debug mode, a changed `crmbuild.exe` 0; `--rebuild` 0, then 3.
- Parity (`compare_builds.py --incremental`, the reference editor `pre-6.7d`): the auto-test, the private 8-bit
  game and the private AGS 4 game, each as it is and in all ten variants: 33 of 33 with parity, in both compared
  builds of each (rooms kept 2 of 3 then 3 of 3, 8 of 9 then 9 of 9, 52 of 53 then 53 of 53). The run layout of a
  warm private game copy against a fresh `--rebuild` one: 56 of 56 files identical. The auto-test on an
  incremental build: 2414 of 2414. A build failing halfway (a broken script in room 2 of the auto-test): nothing
  new or changed outside `_obj/` and `_Debug/`, room 2's `.crm` and manifest gone, rooms 1 and 3 kept; with the
  script fixed the next build keeps 2 of 3 and its `_Debug` equals a clean build's.
- Measured on the private AGS 4 game (53 rooms, 36 scripts), MSBuild Release x86 tools, this machine (8 logical
  CPUs). `agsbuild --data-only` alone, median of 5 runs, two rounds (the first right after a long MSBuild):

  | Build | before (6.7d-10's agsbuild) | 7.1k |
  |---|---|---|
  | every room (`--rebuild`) | 6.18 s, 3.90 s | 6.22 s, 3.97 s |
  | nothing changed | | 2.12 s, 1.65 s (53 of 53 kept) |
  | one room's script changed | | 2.12 s, 1.61 s (52 of 53 kept) |

  The editor's Ctrl+F5, from the key to the "compiled" box (UI Automation, warm project): this editor 3.1 s, 2.7 s,
  3.0 s; the reference editor 1.7 s, 1.6 s (8.4 s, 8.8 s in 6.7d-10). The rooms now cost next to nothing:
  `check-rooms` takes about 0.2 s off the critical path, and a build with nothing changed is bound by the 38 script
  compiles (about 0.28 s each, 8 at a time), which stay full builds by this task's scope. Seen while measuring,
  not changed: `--threads 16` on this 8-CPU machine gave 1.2 to 1.4 s against 1.5 to 1.9 s (agscc waits on
  files); incremental scripts or a higher default would close most of the rest.
- Also seen: a room kept no longer reports its compiler warnings again (as the reference editor did on a warm Run);
  the private game still shows "compiled with warnings" for the scripts and the room event-handler scan.
- Not verified: Linux and macOS runs of the incremental path (the `linux` stage runs the unit tests, not a game
  build); a project on a case-sensitive file system with two room files differing only in case; the editor's
  Rebuild all passing `--rebuild` was read from the code and the NUnit test, not run.

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### 7.1p Incremental script compiles in `agsbuild`

**7.1p Incremental script compiles** (2026-11-07; an Opus 5.5 subagent):
- Profile first (`agsbuild --data-only` on a private AGS 4 game, 53 rooms, 38 scripts, nothing changed, MSBuild
  Release x86 tools, median of 5, from the `--json` task events): 1.43 s in all. Start-up to the first task 0.05 s;
  the export tasks (`autoash`, `glvar`, `dlgasc`, `game-cfg`, `agf2dta`, `po2tra`, `crmbuild:game-key`) run side by
  side from 0.05 to 0.16 s, 0.07 to 0.11 s each; the 38 `agscc` compiles, about 0.15 s each 8 at a time, from 0.15
  to 0.97 s: the critical path. `check-rooms` (0.11 s) waits for a free thread until 0.82 s, as every thread is
  compiling; then the 53 kept rooms' four tasks each (212 tasks that only log "Up to date") take 0.95 to 1.38 s:
  the task manager looks at its tasks every 8 ms (`TaskManager::RunAll`) and starts a thread per task, so a skipped
  task costs about a tick. The pack 0.06 s, the deploy 0.04 s.
- A compiled script whose inputs did not change is kept: `_obj/<script>.o` stays where it was (the package names
  its files by their names in `_obj/`) with its manifest `_obj/scripts/<script>.o.manifest`. No check task: the
  compile task (`TaskCompileScript`, a `TaskPipedProcess`) makes the manifest when it starts, after the tasks that
  write its generated inputs (`autoash`, `glvar`, `dlgasc` for the dialog script), compares it, and either keeps the
  `.o` or deletes the `.o` and the manifest, runs `agscc`, and records the manifest after `agscc` succeeds. A failed
  or cancelled compile leaves nothing recorded.
- The manifest's inputs (`<kind> <hash> <name>` lines, 64-bit FNV-1a of the contents, as 7.1k): `tool` for the
  `agsbuild` and `agscc` binaries; `command`, the whole `agscc` command line (the flags, the script's path, every
  header's path in its order, the output; `-v` is never given to it); `header` for every header in the order
  `agscc` gets them (the copy of the API header, the generated `_AutoGenerated.ash` and `_GlobalVariables.ash`, the
  `--header` files, the module headers); `source`, the script: the project's `<Module>.asc`, or
  `_GlobalVariables.asc` and `__DialogScripts.asc` as this build generated them. `agscc` reads nothing else
  (`Compiler/compiler.cpp`: the `-H` headers and the script; no `#include`, no time, no environment).
- The manifest file also records what `agscc` printed after a line `output`; a kept script prints it again, so its
  warnings are reported by every build as before (the editor's "compiled with warnings" does not depend on what
  changed). The same file format for rooms (format line `agsbuild manifest 2`, so 7.1k's room manifests are stale
  once); a room records nothing after `output`.
- `room_cache.{h,cpp}` became `build_cache.{h,cpp}` (and its test): `BuildInputs` (`Sources`, files by name, next
  to `SourceDir`, a folder of which every file counts; `GameKeyFile` optional), `MakeManifest`, `CacheEntry`,
  `CheckCache`, `WriteManifest`, `ClearTempDirKeepingCache` (also keeps `scripts/<x>.manifest` when `_obj/<x>`
  exists, and that `<x>`). One `FileHasher` per build, now thread-safe, is shared by `check-rooms` and the compile
  tasks, so each header and tool is hashed once. `TaskPipedProcess::RunProcess` tells where the tool's output starts
  in the task's log. `--rebuild` (help text), `-c` with a given `--temp-dir` (deletes the manifests too), the
  editor's `BuildToolOptions.Rebuild` comment.
- The task manager wakes when a task finishes instead of every 8 ms (`TaskManager::RunAll` waits on a condition
  variable that the task's thread signals after `Task::Run`; the 8 ms limit stays for a cancel request, which a
  signal handler only flags). Asked for after the profile: with the scripts kept, the 212 skipped room tasks were
  0.43 s of a 0.78 s build. The order in which tasks start, their errors and the cancelling are unchanged.
- Tests (`tools_test` 247 → 266): in `build_cache_test.cpp` a test per input kind of a script (each tool, a
  compiler option, each header kind, the header order, a header added or removed, the script, a generated script; a
  change in another script keeps it), up to date with the recorded output, stale, a manifest with more inputs, the
  `.o` missing, `--rebuild`; `TaskCompileScript` with a stand-in compiler (`cmd` / `sh`): it records the warning,
  the next run keeps the script without running the compiler (which would fail) and reports the same warning with
  its file and line; a failed compile leaves no `.o` and no manifest; what `ClearTempDirKeepingCache` keeps.
  End to end on a copy of the auto-test (40 scripts, 3 rooms; compiled, rooms kept, warnings): nothing changed 0,
  3 of 3, 3; one module's source, the global script, a dialog's script, a global variable's default value: that
  script alone, 3 of 3; a module header, `GlobalScript.ash`, a `--header` file, the API header, a global variable
  added, an inventory item's script name, the order of two modules, debug mode, a changed `agscc.exe` or
  `agsbuild.exe`: 40, 0 of 3; the game's name alone: 0; a renamed character that a script uses: 40 and the build
  fails, then 40 when it is put back; `--rebuild` 40, then 0. The warnings were 3 in every build.
- Failure halfway (a broken module of the auto-test): the build fails, nothing new or changed outside `_obj/` and
  `_Debug/`, that module's `.o` and manifest are gone, the other 39 are recorded with their `.o`; with the module
  fixed the next build compiles that module alone and its `_Debug` equals a clean build's; then nothing is compiled.
- Parity (`compare_builds.py --incremental`, now a room's script and a script module changed and back, then
  nothing; the reference editor `pre-6.7d`): with the script cache, the auto-test, the private 8-bit game and the
  private AGS 4 game, each as it is and in all ten variants: 33 of 33 with parity in both compared builds (scripts
  kept 39 of 40 then 40 of 40, 2 of 3 then 3 of 3, 37 of 38 then 38 of 38; rooms 2 of 3, 8 of 9, 52 of 53, then
  all). With the task manager's change too: the three projects as they are and the auto-test in all ten variants,
  13 of 13 with parity. The auto-test on an incremental build: 2414 of 2414 with the script cache alone and again
  with both changes.
- Measured on the private AGS 4 game, MSBuild Release x86 tools, this machine (8 logical CPUs), `agsbuild --data-only`
  alone, median of 5 runs. The machine slowed down by a third during the session (the same build went from 3.8 s
  to 5.4 s), so the tool sets are compared in interleaved runs (`--rebuild` 7 cycles, nothing changed 5 cycles);
  the rows of two rounds run one tool set after the other and compare only within a round.

  | Build | before (HEAD's agsbuild) | scripts kept | and the task manager wakes |
  |---|---|---|---|
  | everything (`--rebuild`), interleaved | 5.41 s | 5.32 s | 5.28 s |
  | nothing changed, interleaved | 1.74 s | 0.88 s | 0.34 s |
  | nothing changed, two rounds | 1.47 s, 1.76 s | 0.80 s, 0.85 s | 0.35 s, 0.35 s |
  | one module changed (1 of 38 compiled) | 1.47 s, 1.68 s | 0.84 s, 0.85 s | 0.37 s, 0.49 s |
  | that module's header changed (all compiled and built) | 3.81 s, 4.69 s | 4.03 s, 4.84 s | 4.35 s, 5.15 s |
  | everything (`--rebuild`), two rounds | 3.84 s, 4.98 s | 4.48 s, 5.53 s | 4.60 s, 5.41 s |

  A build is not slower for the cache: hashing every input of a full build is lost in its noise (interleaved
  `--rebuild` row). Profile with nothing changed after both changes (0.36 s): start-up 0.05 s; the export tasks
  0.05 to 0.20 s, the longest `crmbuild:game-key` (0.15 s, a process); the 38 kept scripts 0.15 to 0.22 s (hashing
  their inputs); `check-rooms` 0.22 to 0.30 s (hashing the rooms' files); the pack 0.22 to 0.27 s beside it; the 212
  skipped room tasks 0.30 to 0.32 s (0.43 s before the task manager's change); the deploy 0.32 to 0.35 s.
- The editor's Ctrl+F5 on that game, from the key to the "compiled" box (UI Automation as in 6.7d-10; two sessions
  of two Runs each, the editors in turn, the same hour): this editor 2.1, 1.7, 1.9, 1.5 s; the reference editor 1.7,
  1.3, 1.7, 1.5 s (this editor 2.7 to 3.1 s after 7.1k). The first Run of a session is the slower in both. Where
  this editor's Run goes (process times next to the key): 1.0 to 1.1 s from the key to `agsbuild`'s start (the
  editor saves the project, runs `PreCompileGame`, its checks and the script scan, writes the plugin headers, opens
  the busy dialog), `agsbuild` 0.34 to 0.38 s, then 0.45 to 0.5 s until the box is seen (the probe looks every
  0.1 s, and only from 1 s after the key). So `agsbuild` is no longer what keeps the Run 0.2 s behind the reference
  editor's: the editor's work before it is (6.7d-11 and 6.7d-12 change that code).
- Not verified: Linux and macOS runs of the incremental path (the `linux` stage runs the unit tests, the stand-in
  compiler test included, not a game build); a script module in a project on a case-sensitive file system; the
  editor's Run with a module changed (`agsbuild` alone was timed for it).

*Moved from PLAN.md on 2026-11-07 (the plan's text as it stood):*

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

### Open items from finished tasks

*Moved from PLAN.md "Next up" on 2026-11-07:*

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
4-bit pixel write in `Common/gfx/bitmapdata.cpp` (wrong mask and shift, untested); 3.4-1's leftovers that
are constant now (the write-only `GUI::DataVersion`, the always-on runtime behaviour switches, three dialog
option offsets that are always 0, `FindNearestWalkablePoint`'s old method used by tests only); the scripts in a
git worktree (2026-11-07): `refactor_metrics.py` refuses to run (`.git` is a file there), `prepush.py`'s `linux`
stage cannot fetch from one and its `refs/prepush/worktree` ref is shared with the main checkout, and the
`msbuild` stage needs the git-ignored NuGet packages, irrKlang DLLs and the googletest submodule a fresh
worktree lacks; `Tools/test/directory_test.cpp`'s comment on the MSBuild googletest being older is out of date;
`FetchGoogleTest.cmake` still pins 1.8.1 for GCC older than 5, which no builder uses.
