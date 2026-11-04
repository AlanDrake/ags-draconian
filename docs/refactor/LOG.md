# Refactor log

What happened while each task of [PLAN.md](PLAN.md) was done: the dated *Status* paragraphs of
finished tasks, moved here verbatim (only the indentation was removed), with what was measured,
what was found on the way and what was left open. Since 2026-11-04 the full definitions of
finished tasks are here too, and the long status paragraphs of open ones (1.2, 7.1, U.3); PLAN.md
keeps a one-line result for finished tasks and a short current status for open ones. Read this
file to look up how a task went, not to start one. One section per task, in the plan's order;
entries are in the order they were written. The plan before that compaction is PLAN.md at
`ec43d3764`.

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
- Moving these checks to "whole tree" in CI is task 2.4 (needs the Linux compile database).

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
