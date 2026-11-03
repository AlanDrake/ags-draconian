# Refactor log

What happened while each task of [PLAN.md](PLAN.md) was done: the dated *Status* paragraphs of
finished tasks, moved here verbatim (only the indentation was removed), with what was measured,
what was found on the way and what was left open. [PLAN.md](PLAN.md) keeps each task's definition
and a one-line result; read this file to look up how a finished task went, not to start a task.
One section per task, in the plan's order; entries are in the order they were written.
Tasks that are still open keep their latest status in PLAN.md.

### 0.4 Raise the toolchain floor

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

### 1.1 Own the auto-test game

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

### 1.4 Game corpus regression

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

*Status (2026-11-02):* done, green in CI (run 41442221520): the `Auto-test game` job builds it with
`agsbuild --strict` and every platform job runs it. Hand-made releases are no longer needed.

### 2.2 One-time reformat, module by module

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

*Status (2026-11-01):* done: 457 files (mostly C# editor sources), content identical ignoring CR.

### 3.3 Legacy compiler

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

### 7.1a Build-parity harness

*Status (2026-11-01):* done. `python Script/compare_builds.py <project> --editor <AGSEditor.exe>
[--engine <acwin.exe>]` collects the tools from `.tidy/build`, builds fresh copies both ways under
`.compare/builds/`, prints the distinct `agsbuild` errors and the per-file table, and exits 0 only
on full parity. First result on the auto-test project: the editor build gives 17 files; `agsbuild`
stops at 40 `#module` errors (7.1c) and the missing room sources (7.1e).

### 7.1b Unblock the pipeline

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

*Status (2026-11-02):* done. `BuildWithTaskManager` is 45 lines now: it reads the project
(`ReadProject`), then calls one function per stage on a `BuildContext` (the lists the stages share):
`AddExportTasks`, `AddScriptTasks`, `AddGameDataTask`, `AddRoomSourceTasks`, `AddCompiledRoomTasks`,
`AddTranslationTasks`, `AddSpriteTasks`, `AddAudioTasks`, `AddProjectFileTasks`, `AddGamePackTask`,
`AddAudioVoxTask`, `AddSpeechVoxTasks`, `AddDeployTask`, `AddCleanupTask`. The order is the old order
(it decides the order of files in the package); measured: still 17 of 17 identical.

### 7.1j CI

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
  stays as it is (decided 2026-11-03, until further notice). U.3's PO ticket and any port of
  7.1m/7.1o add `po2tra`; removing `trac` upstream needs the maintainers' agreement.

### U.1 `ags4-bugfixes`

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
