# Refactor log

How each task of [PLAN.md](PLAN.md) went: its definition, the result with dates, commits and numbers, the bugs found
on the way, the decisions taken and what was left open. Read it to look up how a task went, not to start one. One
section per task, in the plan's order; a task that is still open (4.1) gets one entry per step.

Compressed on 2026-11-12 (393 KB to about 125 KB): step-by-step narration, repeated verification lists and the
tracking of upstream PRs that were merged or closed were removed; every bug, measurement and decision was kept. The
full text before that is `docs/refactor/LOG.md` at `84eec2e56`; the sections of the merged upstream offers (U.1, U.2,
U.5, U.7, U.9) are only there.

### 0.1 Tag the fork point

- **Definition:** tag `fork-point` at `4cd88beba` (merge base with `upstream/ags4`); `upstream` remote for reference
  only. *Done when:* the tag exists and the README states the project is a hard fork.

### 0.2 Cherry-pick ledger

- **Definition:** `docs/UPSTREAM.md`, a line for every upstream commit picked, adapted or skipped. *Done when:* the
  file exists and CONTRIBUTING.md describes the process.

### 0.3 Record decisions D1–D6

- **Definition:** record D1–D6 in PLAN.md's table.

### 0.4 Raise the toolchain floor

- **Definition:** C++17 in all `CMakeLists.txt` and `.vcxproj`, CMake 3.16+; for D6 `v143` instead of `v141_xp`,
  `WINVER`/`_WIN32_WINNT` `0x0601`, the Vista check in `acplwin.cpp` gone. *Done when:* CI green everywhere, no XP
  toolset or define left.
- **Result (2026-11-01):** ctest 739/739; `Debug_XP`/`Release_XP` and `WIN_XP_COMPAT` (which disabled D3D11 and the
  D3D shader compiler) removed. CI run 41291607424 (`8771bfdcb`) green on every platform.
- **Found:**
  - Fake gamma (`f6119c4bb`) called `glColor4f`, absent from OpenGL ES2: Android and Emscripten failed. Fixed in
    `e19bc3310`.
  - Windows Packaging: `ags.iss` installs `ikpFlac.dll`, which `AGSEditor.csproj` copied only for `Platform` x86
    while the editor builds as AnyCPU (upstream has the same condition). Fixed 2026-11-02.
- **Not verified / left open:** CodeQL default setup fails to autobuild C/C++ (set build mode "none" or disable).
  Standalone plugin projects (`Plugins/*/msvc`, `agspalrender/Makefile`) stay C++11 until phase 7. The engine
  imports `D3DCOMPILER_47.dll` directly, so Windows 7 without KB4019990 needs the DLL shipped (or lazy loading with
  the precompiled shaders as fallback). `CMAKE_OSX_DEPLOYMENT_TARGET` is 10.9: raise it before using
  `std::variant`/`std::any`/`std::optional::value()` (10.13) or `std::filesystem` (10.15).

### 0.5 Repository hygiene

- **Definition:** ignore or commit the stray artefacts (`Engine/resource/fxc*.exe`, `fxc.zip`,
  `Editor/References/x64/`); `.gitattributes` with `* text=auto`, `eol=crlf` only for `*.sln`, `*.bat`, `*.cmd`,
  `*.vcxproj*`, `*.csproj`. *Done when:* `git status` is clean after a full Windows build.

### 0.6 Agent context docs

- **Definition:** `AGENTS.md` (fork rules, facts) and `docs/context/` (upstream knowledge, every claim dated and
  sourced, ≤3k tokens per file), kept identical to the upstream offer (PR #3185); fork-specific knowledge goes to
  `AGENTS.md` and `ALIGNMENT.md` only. *Done when:* a session can answer "what does upstream intend here" without
  searching issues. *Refresh:* `roadmap.md` at upstream milestone changes, `decisions.md` when a maintainer settles
  something.

### 0.7 Move status logs out of this file

- **Definition:** move finished tasks' status paragraphs from PLAN.md to `LOG.md`. *Done when:* PLAN.md < 8k tokens.
- **Result (2026-11-03):** 1372 lines, 114240 bytes (~28.6k tokens) -> 910 lines, 72.1k bytes (~18.0k tokens).

### 1.1 Own the auto-test game

- **Definition:** fork `ags-test-games`, pin it in CI, test every Draconian-only feature.
- **Result (2026-11-01):** `AlanDrake/ags-test-games` branch `ags4-draconian`, release `draconian-auto-test-1`
  (later built in CI, 1.7). Covers Opacity/Tint ranges (34 checks), DrawingSurface primitives (32),
  void-returns-void (2); 2414/2414 locally; CI run 41286020888 green on all six platform jobs.
- **Lessons:** headless build `AGSEditor.exe /compile <absolute path>\Game.agf`, with `MSYS_NO_PATHCONV=1` in Git Bash.
- **Not verified / left open:** fake gamma, D3D11 pixel-shader blenders, slider focus outside GUI bounds (need
  rendering or input checks), PO translations (editor tests, 6.6).

### 1.2 Contract guard tests

- **Definition:** a test per contract that fails if it breaks: C1 fixture games and rooms from old editors, C2 save
  round trip plus released engines' saves, C3 export snapshot, C4 plugin API, C5 `Game.agf` round trip in the Editor
  and `agfreader`.
- **Result (2026-11-04):**
  - C1: `Common/test/data/compat/<release>/` (`game.ags`, `agssave.001`) for 4.0.0.27 (Alpha 31) and 4.0.0.30,
    built by `make_fixtures.py` with each release's editor and engine; `Common/test/game_compat_test.cpp`; ctest
    `compat_restore_<release>` (`Engine/test/compat_restore.cmake`) restores each save in the fork's engine and
    needs `PASS` in `c1result.txt`. Room format `kRoomVersion_400_26`, game data `kGameVersion_400_27`/`_400_29`.
  - C2: `Engine/test/savegame_test.cpp` (target `engine_full_test`): byte-identical rewrite, each of 14 components
    alone, truncated save refused (upstream-first `196a0fadb`, #2814).
  - C3: `Engine/test/data/script_exports.txt` (1635 lines); `ScriptExports.ReleaseExportsResolve` checks the 1552
    names exported at tag v4.0.0.30. C4: `Engine/test/plugin_api_test.cpp` against `plugin_api.txt` (324 lines).
    Snapshots regenerate with `AGS_UPDATE_SNAPSHOTS=1`.
  - C5: `GameXmlRoundTripTests.cs` on `Tools/test/data/Game.agf` (`AGS_UPDATE_GAME_SAMPLE=1`), read by
    `AGFReaderSample.*` with no problems. AGFReader per-block tests (#3147, 47 upstream-first, `26db161ed`) ported
    to `Tools/test/agfreader_test.cpp`; they found the upstream reader's drift from the editor (`Name`/`ScriptName`,
    `Description`/`DisplayName`, `RealName`, `Flipped`/`Flip`, `ScriptModule`, audio folder defaults,
    `LATEST_XML_VERSION` 3.0.3.2), fixed on `ags4-tools-2-agfreader`.
  - ctest 881/881; MSBuild Common 103, Tools 167, Compiler 594, Engine 3.
- **Found:**
  - `CharacterExtras::ReadFromSavegame` read every 4.x save's inventory in the new format; saves before
    `kCharSvgVersion_400_28` use the legacy one, so no 4.0.0.27 or older save loaded ("Expected: 1894, actual:
    46235"). Cause: upstream `6ec11f244` gates on `kCharSvgVersion_36310` only. Fixed; upstream ags4 has it too.
  - The fork's Opacity left 10 exports of compiled 4.0 games unresolved (`get_/set_Transparency` of Character, GUI,
    GUIControl, Object, Overlay). Restored as `[OBSOLETE]` engine exports (user's choice over script shims) with
    4.0's checks; `GfxDef::Trans100ToOpacity`/`OpacityToTrans100` reproduce 4.0's values and drift (37 reads back as
    38; opacity 251-254 clamps to 0); declarations under `SCRIPT_COMPAT_v400_24`.
- **Decisions:** 3.99 not covered (user, 2026-11-04: a transition phase). 3.99.102 scripts don't link in the fork or
  upstream ags4 (`IsVoxAvailable`): upstream `5e852ea37` (2025-02) removed 186 of its exports.
- **Lessons:** `cc_format_error`/`cc_get_callstack` stubs in `common_stubs.cpp` collide with
  `Tools/data/cc_script_stubs.cpp` in MSBuild `Tools.Test`; they live in `game_compat_test.cpp`.
- **Not verified / left open:** C2 skips the script component and non-default rooms, characters, GUIs, views and
  `DoBefore/AfterRestore`; C3 skips `game.*`/`system.*` offsets and signatures beyond `^N`; C4 has no plugin calling
  every method; `savegame_internal.h` isn't self-contained; the new C++ tests are CMake-only.

### 1.4 Game corpus regression

- **Definition:** `ci/corpus/run.py` builds each owned game and runs every room; one command reports pass/fail per
  room. Corpus: one private AGS 4 game (53 rooms, 36 scripts, 1.6 GB); paths come from arguments or the git-ignored
  `.compare/corpus.txt`.
- **Result (2026-11-02):** `agsbuild --strict` on a fresh copy, each room 10 s (`--startr N`, Software, four at a
  time); fails on early exit, no `Loading room N`, `[ERROR]`/`[FATAL]`; warnings only with `--fail-on-warnings`.
  53/53 rooms pass; builds with `script2` (legacy compiler not tried). Parity: 114/114 files and the translation;
  auto-test 59/59, 2414/2414.
- **Found:**
  - With `--no-message-box` the engine exits with code 0 after a fatal error; only the early exit shows it.
  - `agsbuild`: clip folder settings (a child element) not read, clips lost repeat, volume, priority; GUIs written
    in folder order, not by ID (`gui[N]` wrong); FLAC clips had no file type; escapes like `\n` in GUI texts,
    properties and the game name not resolved. Also the PO translation gap (7.1m).
  - Byte differences matched: clip declaration order (now by ID, in the editor), interaction tables, two
    deprecated hotspot colours, room order, package asset order, part name and ender (`audio.vox`).
  - Not fixed: `agfreader` crashes on a `Game.agf` without `<Settings>`; `StrUtil::Unescape` reads past a trailing
    single `\`.
- **Lessons:** the parity harness skips `.git`, compiles AGS 4 rooms from sources on both sides, compares the
  `.ags` packages themselves.

### 1.5 Coverage, sanitizers and warnings

- **Definition:** a sanitizer job (ASan + UBSan on unit tests and the auto-test game), per-directory `llvm-cov`
  numbers, first-party warnings as errors per directory once clean. *Done when:* sanitizers green, coverage
  published, `Common/` builds with warnings as errors. Split into 1.5a-c (2026-11-05).
- **Result (2026-11-06):** done; sanitizers green locally and in on-demand CI run 42174071706 (auto-test 2414 passed).
- **Decisions:** `analysis.yml` on demand only (user, 2026-11-05: 27 and 14 minutes); `sanitizers` on demand in
  `prepush.py` too (user, 2026-11-06: slow, rarely actionable). Debug builds do not count for performance (user,
  2026-11-06; the origin of STYLE.md "Performance").

**1.5a Warnings (2026-11-05)**
- `ags_warning_free(common)`, `option(AGS_WARNINGS_AS_ERRORS)` (set in `verify.yml`, `prepush.py`). GCC sites before
  → after: `Common` 122 → 4 (`-Wfloat-equal`, never an error), `Engine` 164 → 144, `Compiler` 71 → 33, `Tools` 69 →
  28. `-Winline` dropped. MSVC `/W4` without C4100, C4018, C4245, C4389, C4244, C4267.
- **Found** (upstream's code, fixed):
  - `room_file.cpp` (twice): `data_ver >= kRoomVersion_363_15 && data_ver < kRoomVersion_399` never true (36315 vs
    40), so 3.6.3.15+ backgrounds were read as LZW; now `< kRoomVersion_400`. No test room.
  - `guimain.cpp` `ReadGUI`: `GameGuiVersion < GameGuiVersion`; now `< kGuiVersion_LowSupported`.
  - `directory.cpp`: `GetCurrentDirectory` used the buffer after a failed `getcwd`.
  - Not fixed: `bitmapdata.cpp` 4-bit pixel writing uses mask `4`, shift 0 or 1 (needs `0xF`, 0 or 4).
- **Lessons:** `/WX`/`-Werror` in the compile database broke clang-tidy; `tidy_diff.py` passes `-Wno-error`.
- **Left open:** 87 narrowing conversions in `Common` (C4244/C4267 off); the other directories.

**1.5b Sanitizers, 1.5c Coverage** (`analysis.yml`, `Script/sanitizer_summary.py`, `Script/coverage_summary.py`)
- Coverage: total 53.7% of 101,506 lines; `Common` 61.6%, `Engine` 47.8% (`gfx` 18.6%, `gui` 8.9%, `plugin` 3.4%),
  `Compiler` 89.9%, `Tools` 68.2% (`po2tra` 0%), `Plugins` 0%.
- First sanitizer run: 47 findings (3 ASan, 2 UBSan, 42 LSan: 35 compiler, 7 engine at exit).
- **1.5b-1:** `Script/linux_jobs.sh gcc|sanitizers|coverage` copies the CI commands (keep in step); `prepush.py`
  stages `linux` (always), `sanitizers`, `coverage` (`--only`) run in WSL `Ubuntu-24.04` on the working tree via
  `refs/prepush/worktree`. Lessons: clang-cl has no leak detection, hence WSL; WSLg opened the game fullscreen, so
  the script unsets `DISPLAY`, `WAYLAND_DISPLAY`, `XDG_RUNTIME_DIR`, `PULSE_SERVER`; summary scripts need `--repo`
  outside the checkout.
- **Found, 1.5b-2 (memory errors):**
  - `String(const char *, size_t)`/`SetString(ptr, length)` ran `strlen` past `length` on unterminated buffers
    (`Utf8ToCp1252`, `data_helpers.cpp`, `readlink`, `DialogTopic`, Editor template description); now `memchr`.
    +1 ns on short texts, a 20000-character `Left`/`Mid`/`Right` 400 ns → 4 ns.
  - `CreateGlobalVars` read `_code[_fixups[i]]` before checking it; `RuntimeScript::CheckFixupInCode`.
  - `JointRTTI::Join` (upstream's code) compared names through pointers into the string table it grows; uses
    `name_stri`.
  - Seen, not touched: `FIXUP_DATADATA` reads `globaldata[_fixups[i]]` unchecked; `JoinLocation`/`JoinType`
    `if (uid <= size) resize(uid + 1)` can shrink the table (upstream too).
- **Found, 1.5b-3 (UBSan):**
  - `Memory::Read/WriteInt16/32/64` dereferenced cast pointers at odd offsets (`WFNFont::ReadFromFile`). Fixed with
    `UnalignedInt16/32/64` in `platform/types.h` (`__unaligned`, plain type on MSVC x86, `aligned(1), may_alias` on
    GCC/Clang): the 28 objects that see `memory.h` keep the code of before a50cf4f7a in MSVC and GCC builds. An
    earlier `memcpy` fix cost 0.9 → 2.1 ns per read at MSVC `/O1` and 4.0 → 13.6 ns in x64 Debug, and changed
    `scriptexecutor.cpp` with GCC `-O3`.
  - `VectorStream::Write` called `memcpy` from null with size 0; zero-byte stream reads and writes return early.
  - Not fixed: the uncompiled strict-alignment branch's `ReadInt64` sign-extends bit 31.
  - Not verified: ARMv7 clang reads 64 bits with two `ldr` where the cast got `ldrd` (code read only).
- **Found, 1.5b-4 (compiler leaks, 46 → 11 rows):** `SymbolTable` never freed its `new CTF_...` (now owned by
  `unique_ptr`; `CompileTimeFunc` got a virtual destructor); `SymbolTableEntry::operator=` leaked the old `...D`
  pointers on every scope exit; `Scanner::SymstringToSym` made a second `LiteralDesc` for a known literal. Seen:
  no move constructor on `SymbolTableEntry` (deep copies on growth).
- **Found, 1.5b-5 (engine leaks):**
  - `AGSDeSerializer::Unserialize` made a never-freed `ScriptUserObject` manager per restored struct; uses
    `globalDynamicStruct` now.
  - `OpenAlSource` never deleted pooled AL buffers, so `alcCloseDevice` refused; `ReleaseBuffers()` in
    `audio_core_shutdown`.
  - `agscc`'s `Compile()` never deleted its `ccScript`; the summary missed it (indirect leaks only) and now reports
    such logs.
  - Third-party reports suppressed in `ci/sanitizers/lsan.supp` (`SDL_EGL_InitializeOffscreen`, `ALSA_OpenDevice`,
    `GL_RunCommandQueue`) and `ubsan.supp` (`nonnull-attribute:SDL_stdlib.c`, mojoAL); a plain SDL program leaks
    the same, each entry shown necessary.
- **Not verified / left open:** macOS, iOS, Android, FreeBSD, Debian 10 image; drivers other than Software at exit;
  video playback at exit. The auto-test game reports 2414 tests on CI and 2345 in the local distro (same tag, not
  looked into). U.8 (offer the 1.5b-2 fixes upstream).

### 1.6 Mechanical-change verifier

- **Definition:** `Script/compare_objects.py` builds before and after a commit and reports translation units that
  differ; format and rename commits must come out identical.
- **Result (2026-11-01):** one worktree and build dir in `.compare/` (paths and `__FILE__` match), MinSizeRel, MSVC
  `/Brepro`; GoogleTest files are line-sensitive and tolerated. A clang-format of 4 files gave 588 identical, a
  rename 589, `return 1` → `return 2` is flagged. Not tried on Linux/macOS.

### 1.7 Build the auto-test game in CI, from source

- **Definition:** done with 7.1j: CI builds `AlanDrake/ags-test-games` with the same run's tools for every platform job.
- **Result (2026-11-02):** green in run 41442221520 (`agsbuild --strict`); hand-made releases no longer needed.

### 2.1 CI format gate

- **Definition:** pinned `clang-format 19` via `git clang-format --diff` against the PR base. *Done when:* a badly
  formatted line fails CI.

### 2.2 One-time reformat, module by module

- **Definition:** `[format]` commits per module, verified with 1.6, hashes in `.git-blame-ignore-revs`.
- **Result (2026-11-01):** ~58,000 changed lines in 7 commits, all non-test objects identical; CI checks all tracked
  C/C++ files with clang-format 19.1.7. Not formatted (`DisableFormat`): third-party code, `Editor/AGS.Native`,
  `Editor/Native`, `PSP/`, `OSX/`.
- **Lessons:** clang-format needed two passes to be idempotent (trailing comments). Run `git config
  blame.ignoreRevsFile .git-blame-ignore-revs` once. Only MSVC objects were compared.

### 2.3 Mechanical modernisation

- **Definition:** one commit per clang-tidy fix: `use-nullptr`, `use-override`, `deprecated-headers`, `use-using`,
  `redundant-void-arg`.
- **Result (2026-11-01):** four commits, objects identical; `Script/tidy_fix.py`. 157, 91, 122 (89 files) and 32
  places.
- **Left open:** seven files stay on `<math.h>`/`<stdlib.h>` (`guiinv.cpp`, `cc_compiledscript.cpp`,
  `character.cpp`, `update.cpp`, three plugins) because `<cmath>` picks other `abs` overloads; `engine.h` and
  `sys_events.*` skipped for `redundant-void-arg` (changed `game.cpp`'s object); `use-using` skipped (the fixer
  broke the build). Whole-tree gating dropped: 2.4 gates changed lines.

### 2.4 clang-tidy gate

- **Definition:** naming and function-size rules on changed lines. *Done when:* a `chaa`-style name or a 300-line
  function is flagged.
- **Result (2026-11-05):** `Script/tidy_diff.py` (`clang-tidy-diff.py` is not in the pip wheel), in `verify.yml` and
  `prepush.py` stage `tidy`; exit 0/1/2. Abbreviations via `readability-identifier-length` and `CRYPTIC_WATCHLIST`
  (`ags-cryptic-name`; functions and members not covered). CI runs 42032914043 (green), 42034663373 (7 planted
  findings).
- **Found:** on Windows `--line-filter` needs backslashes; with `/` the gate passed everything.
- **Lessons (2026-11-09):** a parameter `[rename]` brought old functions under the rules; findings on functions that
  exist in the base (`BaseFunctions`) are dropped.

### 2.5 Header guards → `#pragma once`

- **Result (2026-11-01):** 380 headers to `#pragma once` (`52cba7906`; ten unguarded left alone); 42 header
  `using namespace` removed (`5e6da0633`). 12 remain: one function-local in `characterinfo.h`, 11 in
  `Editor/AGS.Native`. Non-Windows sources checked by reading only.

### 2.6 Self-contained headers, then sorted includes

- **Result (2026-11-02):** `CMake/HeaderCheck.cmake` target `header_check`; 28 of 366 headers fixed on MSVC. Sorted
  includes (`IncludeBlocks: Regroup`, `platform/windows/windows.h` before other Windows headers) over 570 files.
  Final: MSVC 366/366, GCC 350/350, clang + libc++ 350/350.
- **Found** (by CI after the push): `fmem.h` used `size_t`, legacy `cc_symboldef.h`/`cc_variablesymlist.h` lacked
  standard includes, `delegate.h`/`indexedobjectpool.h` used `assert`, `std::find_if`, `uint32_t`, `SIZE_MAX`
  without headers; `gfxfilter_d3d*.h` now skipped off Windows.
- **Lessons:** for repo-wide changes build the MSBuild solutions too and check headers with libc++
  (`-D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES`), which reproduced GCC; MSVC with `/permissive-` did not.

### 2.7 Line endings

- **Result (2026-11-01):** renormalised 457 files (mostly C#), content identical ignoring CR.

### 2.8 CI shape and the pre-push check

- **Definition:** added 2026-11-04 after 2.6 needed four pushes: CI should confirm, not discover.
- **Result (2026-11-04):** green on the second run (`7afe0be15`). `verify.yml` on every push (format, GCC build,
  `ctest`, header checks), `ci.yml` light or full (a `plan` job; full on tags, Mondays, `-f full=true`),
  `concurrency`, `paths-ignore` for docs. `Script/prepush.py` stages `format`, `build`, `headers`, `libcxx`,
  `msbuild`; `emscripten` added 2026-11-05 (GLES2 and Linux-branch syntax check). Old workflow registrations
  disabled before the first push.
- **Found:** Ubuntu's libc++ 21 rejected `time_t` and `atoi` without headers once the libc++ flags applied to the
  whole build; `AGS_HEADER_CHECK_FLAGS` now applies them to the stubs, includes added to 10 files.
- **Decisions:** macOS in its own `macos.yml` (2026-11-04): runners are often unavailable and held up the parity job.
- **Lessons:** upstream `build.yml` changes no longer cherry-pick. 7 of 13 CI breaks were code MSVC accepts.
- **Not verified:** `emscripten` covers no macOS, iOS, Android, FreeBSD code, Linux linking, GCC or tools.

### 2.9 CI and pre-push time

- **Definition:** added 2026-11-06 (the user named testing and CI as the bottleneck). **2.9a** ccache in the CMake
  jobs of `verify.yml` and `ci.yml`, the Windows CMake job on Ninja; **2.9b** a light run builds only one MSBuild
  configuration and, on a push, leaves Ubuntu x86_64 to `verify.yml`, which runs the auto-test game; **2.9c**
  `prepush.py --auto`. *Done when:* CI confirms a push that changes a few files in under 10 minutes, and a session
  that changed only the editor runs one stage. The state before it is the tag `pre-2.9`.
- **Baseline (2026-11-06,** light run 42281919227, Verify 42281919108): `ctest` 1106 tests in 8 s, the auto-test
  game about a minute per renderer. A push cost Verify 8 min and light CI 18 min, about 70 runner-minutes; longest
  chain auto-test game 81 s, Windows 907 s (configure 176, build 466), Parity 100 s. Nothing cached but the
  downloaded sources; 403 of the 945 compile steps were SDL2, SDL_sound, freetype, allegro, vorbis, theora and gtest;
  the engine compiled 8 times per push. Locally a full `prepush.py` after a Common header change was 20 to 25 min,
  `sanitizers` 13 min.
- **Result, 2.9a-c** (landed 2026-11-07, the eight commits of `probe-2.9` fast-forwarded):
  - ccache through `CMAKE_C_COMPILER_LAUNCHER`/`CMAKE_CXX_COMPILER_LAUNCHER`; `.ccache` (at most 500 MB, 1 GB in
    the `build` jobs) kept by `actions/cache` under a key with the commit hash, restored by prefix, never
    overwritten. Jobs: Verify's GCC and clang header jobs, `ci.yml`'s five `build` platforms (MSVC: ccache release
    4.14.2, checked against GitHub's SHA-256). Left out: the auto-test game job (90 s with the cache, 81 s
    without), Emscripten, Android, the Linux image jobs, `macos.yml`, `analysis.yml`.
  - The Windows CMake job builds with Ninja (the Visual Studio generators take no launcher), `CC`/`CXX` set to `cl`
    (a MinGW gcc is first on the runner's `PATH`); binaries in `build/`, not `build/Release/`. `/MP` only for the
    Visual Studio generators. ccache works with MSVC's `/GL`, `/Fd`, `/FS`: 1276 of 1318 calls cacheable, warm hit
    1275 of 1276; one MSVC build is 0.41 GB of cache, a GCC build 0.13 GB. `compile_commands.json` commands start
    with `ccache`; clang tooling drops it (checked with clang-tidy 19.1.0). The FetchContent cache key is
    `fetchcontent-2-` (a tree configured by the VS generator stops a Ninja configure).
  - 2.9b: on a push the `build` matrix is Windows alone; Verify's "Linux (GCC)" job builds and runs the auto-test
    game with both renderers; PRs and manual light runs keep Ubuntu-x86_64. `Script/linux_jobs.sh gcc` does not
    run the game.
  - 2.9c: changed paths (merge base with upstream to working tree, or `--base`) are matched against `AUTO_RULES`,
    first rule decides, unknown path means every stage; rules from what each build reads (`ninja -t deps` of the
    Emscripten tree: 0 files under `Compiler/` and `Tools/`, 54 under `Plugins/`). `--dry-run` prints the choice.
  - Probe, warm (ac4ab0650): Verify 6.9 min (GCC 408 s, clang headers 151 s, was 289); Windows 569 s (was 907),
    MSBuild Engine 156 (474), Tests 190 (456), Tools 104 (418), Editor 134 (212). Full run (8a17b00d0): 26 jobs green
    in 20.7 min.
- **Result, 2.9d x64 only on Windows** (2026-11-08, landed 2026-11-09): the MSBuild test, engine, tools and editor
  jobs build x64 Release on every run and x64 Debug in full runs; parity, PDB, tools archive and packaging take x64;
  `ags.iss` installs in 64-bit mode (64-bit Windows only) with `vc_redist.x64.exe` (14.42.34433) and
  `Magick.NET-Q8-x64.Native.dll`; `prepush.py`'s `msbuild` builds x64. The editor already had x64 configurations, so
  no port was needed. Measured: editor NUnit x64 537 passed, 25 ignored, 0 failed; `compare_builds.py` 59 identical,
  auto-test 2414 of 2414, PARITY; full run 42526395741 green, installer 75.6 MB, Windows tools zip 2.8 MB.
- **Result, first push measured** (2026-11-09, `525ddd44d`): Verify 6 min 35 s, CI 12 min 15 s (estimate was
  12.5). The push filter chose the Windows CMake job alone; caches warm (GCC 1015 of 1016 hits, clang 560 of 561,
  MSVC 1274 of 1275). Critical path, serial: auto-test game 110 s, Windows 504 s (configure 91, build 214, tests 30,
  auto-test runs 114), parity 112 s.
- **Result, follow-up** (2026-11-09, `probe-2.9-followup`, four commits): the MSBuild Tests job runs in full runs
  only (305 s there); an apt cache (~79 MB) cuts the install step by 15 to 40 s per Ubuntu job (e.g. Ubuntu-x86_64
  52 s → 21 to 22 s); an ARM64 auto-test retry (`continue-on-error` on the first attempt); every CMake job takes its
  dependencies from the `ags-deps` cache through `Script/fetch_deps.sh` (mirror 12 MB, 7 archives; download 13 to
  15 s cold, 0 to 1 s warm). The light run is not shorter: the configure (76 to 101 s) dominates, not downloads.
- **Result, the push after landing** (`a04697491..edb87dc88`): Verify 6 min 53 s, CI light 13 min 44 s; critical path
  game 89 s, Windows 619 s (configure 89, build 281), parity 108 s. Runners come in two speeds (by the cache-free
  auto-test runs: 63 to 64 s and 50 s on fast ones, 74 to 75 s and 57 to 58 s usually; build step 208 to 214 s
  against 261 to 284 s). Time to expect for a light run: about 13 minutes, 12 on a fast runner.
- **Found:**
  - Every CMake job downloaded its dependencies at every configure, cache hit or not (FetchContent runs its download
    step each time); the runs of 2026-11-09 failed there on HTTP 504. Fixed in the follow-up by the `ags-deps`
    cache and `fetch_deps.sh` (Android had it since 2026-11-02); googletest now goes through `ags_fetch_url`.
  - 2.9d's first full run (42524209246) failed in Windows Packaging: `build.ps1` checks `Source/Redist` against
    `checksums.sha256`, which still named the x86 redistributable; the survey missed it. Fixed; 42526395741 green.
  - Both Verify jobs save the apt and `ags-deps` caches under the same keys; the clang job saves first and the GCC
    job's save is refused, so the apt archive is the clang job's package set. Not changed (the job name in the key
    would end it).
  - On a fresh `_deps` extraction the Linux jobs compile 288 more calls (all hits); not looked into.
- **Decisions:**
  - 2026-11-07 (the user): LTO stays in the verification builds (2.9e). With a warm cache the Windows build step is
    15 s of objects and 255 s of `/GL` links (`tools_test` 66, `ags` 36), GCC's `ags` link 112 s; turning LTO off
    there would have taken most of it away.
  - 2026-11-08 (the user): the installer ships the x64 `acwin.exe` and editor; every x86 build with an x64
    alternative goes; what exists only as 32-bit stays; the Linux i386 packages stay, in full runs and tags only.
  - 2026-11-09 (the user): plugin DLLs are x64 from now on, nothing more needed (the seven `Plugins/*/msvc` projects
    have x64 configurations; no CI job built or shipped them). An x64 engine cannot load a 32-bit plugin DLL.
  - 2026-11-09 (the user): no 32-bit test run on a push is accepted and `ctest` is not added to the i386 package
    build; the MSBuild Tests job moves to full runs; the auto-test game stays in the Windows CMake job (a job of its
    own would have given about 10.5 min).
- **Lessons:** a branch's caches are not visible to another branch, so a probe's caches do not count for the main
  branch. The follow-up's slower times were first put down to a GitHub incident; reading 13 runs showed they are the
  usual runner speed (the cache-free auto-test runs tell the two speeds apart).
- **Not verified / left open:**
  - The "under 10 minutes" of the done-when is not met (about 13 min); the second half holds. The time left is LTO
    linking, kept by decision.
  - Still open from the plan's list: upstream's workflows, which fail on the port branches; the laptop for the WSL
    stages. Whether the `_deps` caches go: the user decides.
  - No 32-bit build runs a test anywhere; three shipped targets are 32-bit and built in full runs only (wasm32, the
    Linux i386 packages, Android `armeabi-v7a` and `x86`). `ctest` in the i386 build would need googletest in the
    builder image, a local-googletest path, `-DAGS_TESTS=1` in `debian/rules` and a probe.
  - The ARM64 retry is untested (the first attempt passed). A cold push was not observed. The Linux package and
    tools jobs in containers do not get the mirror; the i386 container still does not resolve `github.com`.
  - `setup_ags_build_deps.bat` still builds the x86 libraries and downloads the x86 redistributable.
  - 2.9d: the installer installed and run, the editor window opened by hand, an upgrade over an x86 install, the 25
    ignored NUnit tests compared with x86.

### 3.1 Dead platforms

- **Definition** (D2, D6): remove `PSP/`, `Engine/Makefile.psp`, the plugins' `Makefile.psp` and `psp.c`,
  `Engine/util/library_psp.h`, the 8 `AGS_PLATFORM_OS_PSP` branches, pre-Windows 7 code paths and toolsets; iOS and
  FreeBSD stay; delete the `ags4-framescript` branch. *Done when:* `AGS_PLATFORM_OS_PSP refs` = 0 and the folders
  are gone.
- **Result (2026-11-04):** done; tag `pre-3.1` (local) marks the commit before. Removed the PSP port, its Makefiles,
  `psp.c` and `exports.exp`, the `PSP_VERSION` blocks, parallax's `dummy()`, the 8 defines and their references.
  Pre-Windows 7: no toolset to remove (all 124 `PlatformToolset` entries are `v143`); removed the editor's
  `IsWindowsXPOrHigher`/`IsWindowsVistaOrHigher`, old Windows names in the About box, the debugger's Win9x check,
  the Win9x workaround in `AGSWin32::GetDiskFreeSpaceMB`; the installer warns below Windows 7. Verified:
  `compare_objects.py` 610 objects identical, only `ags_parallax.cpp` differs; `prepush.py` all stages; PSP refs 0;
  corpus run 53 of 53 rooms.
- **Found:**
  - `GetDiskFreeSpaceMB` ignores the `GetDiskFreeSpaceExW` result, so a failed call returns an uninitialised value
    (a TODO marks it). Returning 0 is not the fix: both callers read it as "disk full" and would refuse saves when
    the save directory does not exist yet. Open.
  - Corpus runner (1.4): `--startr` only sets the start room of a new game, so a room's script can send the player
    on (two of 54 rooms do, 25 → 503 and 500 → 502), and `run.py` charges the rest of the run to the start room
    because it only checks for `Loading room <n>`. It should report the rooms loaded.
- **Decisions:** kept by the contract: `eOS_PSP`/`eOSPSP` in the script enum (C3: removing renumbers `eOSWeb`,
  `eOSFreeBSD`), `"psp"` in the `override_os` names (C6), the savegame reader's skip of the Vista rich-media header
  (C2; later removed in 3.4 step 3).
- **Not verified / left open:** the installer, the Xcode project; the save-time call of `GetDiskFreeSpaceMB`.
  `ags4-framescript` (local, commit `5fb51ad7d`) not deleted: the agent's `git branch -D` was refused, left to the
  user.

### 3.2a/b Makefiles to CMake

- **Definition** (D5): **3.2a** the tool Makefiles and `Compiler/Makefile` go, the `Linux Tools` and macOS tools jobs
  build and install with CMake; **3.2b** `Engine/Makefile*`, the plugins' `Makefile`/`Makefile-objs` and `make.yml`
  go, `debian/` builds through CMake. **3.2c** was re-defined (below) as "two build descriptions that cannot drift".
- **Result, 3.2a/b (2026-11-05):** full CI run and macOS run green on `32a687432`. Removed 9 tool Makefiles,
  `Compiler/Makefile`, the engine Makefiles, five plugins' Makefiles, `make.yml`. `linux-tools` runs CMake
  (`RelWithDebInfo`) in the builder image, so a failed tool fails the job; the tools archives now hold 11 programs
  instead of 4. `debian/rules` configures with CMake (`AGS_USE_LOCAL_{SDL2,SDL2_SOUND,OGG,THEORA,VORBIS}=ON`,
  `FETCHCONTENT_FULLY_DISCONNECTED=ON`, `CMAKE_INSTALL_RPATH=$ORIGIN/<rpath>`), `debian/control` needs
  `cmake (>= 3.16)`. Measured: `agscc` 34 sources both ways; engine Makefile 396 files, CMake 390, every difference
  explained (11 `apeg` MPEG files empty under `DISABLE_MPEG_AUDIO`, FreeType duplicates, guarded platform files);
  the `.deb` binary 5,418,616 bytes instead of 6,562,432 (about 1 MB was `-rdynamic`'s `.dynsym`/`.dynstr`), same
  ten `NEEDED` libraries as `e22c990b0`; Windows `ctest` 1031 passed.
- **Result, 3.2c (2026-11-07, CI-confirmed on `dd514dad9` 2026-11-08):** `NativeDLL.vcxproj` no longer links
  `Compiler_md(_d).lib`, `AGS.Editor.Full.sln` no longer holds `Compiler.Lib` (`AGS.Native.dll` 2,829,312 bytes before
  and after, `compare_builds.py` 59 identical); the unused `Release_MD`/`Debug_MD` configurations of `Compiler.Lib`
  and `Compiler.App` removed (358 lines). Drift guard `Script/check_build_lists.py` (CMake File API against each
  `.vcxproj`'s `ClCompile`/`ResourceCompile`, 15 deliberate differences with reasons), run by `prepush.py` stage
  `buildlists` and CI's Windows `build` job; first CI run: "26 .vcxproj match their CMake targets". A source added to
  one side only fails with the file named.
- **Found:**
  - The `Linux Tools`/`macOS Tools` jobs ran the Makefiles through `find ... -execdir make`, which exits 0 whatever
    `make` returns, so six tools were silently missing from the archives (`agf2dlgasc`, `agfexport`, `agsbuild`,
    `agf2dta`, `crmpak`, `spriteimport`; `crmbuild` never had a Makefile). Fixed by the CMake jobs.
  - `AGS_CASE_SENSITIVE_FILESYSTEM` was missing for `Engine/ac/file.cpp` in CMake Linux builds (set for `common`
    only; the Makefile set it for every file, so the shipped Debian engine had it). Added for the engine on Linux;
    Android and FreeBSD still do not get it, as before.
  - New in the package: `_FILE_OFFSET_BITS=64` (CMake defines it everywhere), so the i386 engine gets 64-bit file
    offsets and the `libc6` dependency goes from `>= 2.14` to `>= 2.28` (most likely FreeType's `fcntl64`; read from
    sources, not checked in the binary).
  - First full run (`672e584a7`), six builder-image jobs failed: SDL_sound's CMake package names a shared library the
    image deletes (now found through pkg-config first; an installed SDL2 sets `SDL2_LIBRARY`); `agsbuild` used
    `std::thread` without libpthread, separate before glibc 2.34 (`agsbuild` and `tools_test` link
    `Threads::Threads`); the i386 container cannot resolve `github.com` (the tools job uses the image's libraries with
    downloads off).
  - The 1.2 and 1.3 tests never reached the MSBuild test projects (`game_compat_test.cpp`, `movelist_test`,
    `plugin_api_test`, `route_finder_test`, the whole `engine_full_test`). Fixed: added, new
    `Tests.App/Engine.Full.Test.vcxproj`, the googletest submodule moved from 1.8.1 to `52204f78` (the commit
    `FetchGoogleTest.cmake` takes). MSBuild tests 990 → 1146 run, test names identical to CMake's (1147 with the
    disabled one). Still CMake-only: the two `compat_restore_*` ctest cases.
  - The shipped tools ran with asserts on: no `Tools.App/*.vcxproj` defined `NDEBUG` in Release. Fixed; every tool
    but `agscc` 20-46 KB smaller (`agsbuild` 776,704 → 730,624 bytes), auto-test package byte-identical.
  - `Convert8BitARGBTo32` (`Tools/data/sprite_utils.cpp`) stepped its `uint32_t` row pointer by the stride in bytes,
    so any 8-bit image with palette alpha taller than one row was written past its buffer; its
    `assert(dst.GetBytesPerPixel() == 32)` could never hold. Uncovered by the `NDEBUG` fix. Upstream has the same code
    (`2a7b4f1f7`, still in `upstream/ags4` `4d4b2c9cd`). Fixed 2026-11-07 (byte pointer per row, assert on
    `GetColorDepth() == 32`); test `SpriteImport.DirectImportConvert8WithAlphaTo32` crashed with
    `STATUS_HEAP_CORRUPTION` before the fix, passes after (MSVC, MSBuild, GCC).
  - `debian/make_ags+libraries.sh` read the version from a moved file and built in Debian 8 chroots (no C++17); now
    Debian 10, but Debian 10 has CMake 3.13 and no SDL_sound for SDL2, so the chroots need both installed. Not run.
- **Decisions:**
  - 2026-11-07 (the user, after seeing a CMake-generated solution with 46 projects): the hand-written solutions
    stay; 3.2c becomes a drift guard plus dropping the editor's dead compiler dependency; `Engine.Full.Test`'s
    wildcard over Engine.App's objects stays. *Done when (rewritten):* a source file in one build description and
    not the other fails `prepush.py` and CI; the editor builds without `Compiler.Lib`.
  - 2026-11-07 (the user, after talking to the maintainers): release builds keep SDL2 dynamically linked
    (`acwin.exe` + `SDL2.dll`, the Linux bundle's `libSDL2`); only CMake builds that fetch SDL2 link it statically.
    The agent's static-link change was reverted in full (it had measured `acwin.exe` 4,079,104 → 5,404,160 bytes;
    the editor copies `SDL2.dll` into every game's `Compiled/Windows`).
  - Kept: `Plugins/agslua/Makefile*` and `Plugins/agspalrender/Makefile` (standalone plugin builds, phase 7; the
    user decides after the refactor).
- **Lessons:** CI-only jobs (the Debian 10 builder image) were fixed on a temporary branch whose workflow ran only
  those six jobs, then one full run.
- **Not verified / left open:** the Debian and Linux tools jobs run only in full runs now, so a break shows late; the
  engine was not started from the new package; no dpkg-buildflags hardening in either build (read, not run);
  3.2c: the x64 editor builds at the time, a deliberately mismatched file on CI (done locally only).

### 3.3 Legacy compiler

- **Definition** (D3): remove `Compiler/script/` (the AGS 3 compiler), its tests, `Solutions/Compiler.Lib`,
  `ccCompileText`, the `ExtendedCompiler` UI, fold `test2/` into `test/`; then remove `Editor/AGS.CScript.Compiler`
  after its preprocessor's users moved to C++ (7.1c, 6.7d). *Done when:* one compiler, one preprocessor (C++), one
  test folder, no `AGS.CScript.Compiler`.
- **Result:** done 2026-11-07.
  - 2026-11-03: removed `Compiler/script` (legacy), `fmem`, their tests, `LEGACY_COMPILER`,
    `AGS_BUILD_LEGACY_COMPILER`, the editor's "AGS SCOM 3 EXT" and the legacy CI steps. Move-only renames:
    `script2` → `script`, `test2` into `test`, `Compiler2.Lib` → `Compiler.Lib`, `ccCompileText2`/`ccGetExtensions2`
    lose the `2`. A project selecting "AGS SCOM 3 EXT" (or none) gets the available compiler with a warning;
    `ExtendedCompiler` stays, unused, because the loader rejects unknown elements. Verified: CMake compiler tests
    594, editor NUnit 391, parity 59 of 59, auto-test 2414 of 2414.
  - 2026-11-07, three commits: `FastString` moved to `Editor/AGS.Editor/Utils/FastString.cs` (namespace
    `AGS.Editor`); the project deleted (50 files, about 3,200 lines) with its references, solution entries, the
    `Program.cs` assembly-resolve case and the `ags.iss` entry. Editor NUnit 547 of 547 after each commit.
- **Found:** `Compiler/Makefile` still built `agscc` with the legacy compiler (the Makefile workflow's Linux binaries
  had it); switched to `script2` at the time.
- **Not verified / left open:** the installer, the README text, `AGS.Editor.NoNative.sln` (its native project fails
  to link `Common_md_d.lib` in a fresh output directory; not compared against the parent commit).

### 3.4 Legacy data-version paths

- **Definition** (D1): five steps: game files, room files, savegames, the script API, savegame-only fields.
  Decided 2026-11-08 (D1): no legacy reader is carried; L.1 may rebuild one later.
- **Step 1, game files (2026-11-07, four commits):** `kGameVersion_LowSupported = kGameVersion_399`; older data gets
  "Game was compiled with 3.6.0.21 (data format 3060021). This engine loads games made with AGS 3.99 or later (data
  formats 3999999 - 4000034)." (still `kMGFErr_FormatVersionNotSupported`). Deleted the pre-3.99 branches in Common
  (57 lines; no `Upgrade*()` function went) and Engine (158 lines, 17 files, 29 checks), the enum values
  `kGameVersion_360_21` to `_363_16` and `_362_07`. Tests `GameDataVersion.*`. Measured: `legacy version checks`
  62 → 0 (files 22 → 0); Engine lines 125744 → 125616; ctest 1150 of 1150, auto-test 2414 of 2414, corpus 53 of 53
  with an identical per-room table.
  - Left, constant now: `GUI::DataVersion` has no reader; the four runtime behaviour switches are always on, so their
    C6 config overrides change nothing; `DialogOptions`' `line_x_off`, `normal_area_counter_align_pad`,
    `fixed_padding` are always 0; `FindNearestWalkablePoint`'s old method is called by `route_finder_test` only;
    `InventoryItemInfo::ReadFromFile`'s version parameter is unused. GUI version checks stay (3.99 data carries
    `kGuiVersion_350`). The editor's `ImportOldGame` now refuses pre-3.99 games too (6.7 deletes it).
- **Step 2, room files (2026-11-08, six commits):** `kRoomVersion_LowSupport = kRoomVersion_400`; an older room gets
  `kRoomFileErr_FormatNotSupported` with its version (pre-4.0 rooms wrote a bare 16-bit version). Deleted the
  `< kRoomVersion_400` step of `UpdateRoomData` and four enum values (17 lines). The editor refuses projects saved
  before version index 3999901 ("open and save it with the AGS 4.0 editor first"; `ProjectVersion.Check`,
  `ProjectVersionTests`); `UpgradeGameRoomsOpenFormatTask` and `ConvertAllRoomsFromCrmToOpenFormat` deleted (123
  lines); "Import existing room" and room templates read the file first and refuse an old `.crm` with one message.
  Measured: comparisons with a pre-4.0 room version 3 → 0, enum values below `kRoomVersion_400` 4 → 0; ctest 1166 of
  1166; MSBuild tests and NUnit 556 of 562 (6 skipped); the headless editor refuses copies claiming 3060316 and
  3999900; auto-test 2414 of 2414.
- **Step 3, savegames (2026-11-08, five commits):** `kSvgVersion_LowestSupported = kSvgVersion_400` (4000000; was 9),
  based on a scan of every first-parent commit from upstream's 3000d69de (2023-10-02): engines wrote 4000000 from
  there and 4000018 from 1b1d79d31; earlier 3.99.99 saves have "Game State" version 10, below that component's floor
  since 3000d69de, so no save that loads now is lost. Five component floors raised to what 3000d69de wrote (Audio
  0 → 2, GUI → `kGuiSvgVersion_400`, Mouse Cursors 0 → 1, Overlays → `kOverSvgVersion_400`, Managed Pool →
  `kManagedPoolSvgVersion_400`). The Vista rich-media header skip went (its writer was removed in 018d8790f,
  2022-04-05): such a file is now `kSvgErr_SignatureFailed`. Deleted Engine +226 −441, Common +105 −161, 66 enum
  values; values above the floors that 4.0 engines wrote are kept (Audio 3060130, 3060303, Inventory Items 3060304,
  3060314, Script Modules 3060200, Plugin Data 1, `kDialogTopicSvgVer_363`). Tests `SavegameFile.*`,
  `Savegame.ComponentOlderThan400IsRefused`; with the old floors back they fail (Overlays throws `bad_alloc`).
  Measured: comparisons with pre-4.0 save version names 82 → 10, enum values 76 → 16; Engine lines 125748 → 125629,
  Common 51000 → 50944; ctest 1169 of 1169; auto-test 2414 of 2414; `compat_restore_4.0.0.27`/`_30` pass; the
  4.0.0.30 fixture's save is identical between engines but for Game State bytes 1740-1741, which also differ between
  two runs of one engine.
- **Step 4, script API (2026-11-08, three commits):** deleted 4 unregistered wrappers (29 lines):
  `Sc_Character_GetHasExplicitTint_Old` with `Character_GetHasExplicitTint_Old` (unbound since upstream's 1295db018),
  `Sc_free_dynamic_sprite` (registration removed by upstream's 5e852ea37), `Sc_Walkarea_GetDrawingSurface` and
  `Sc_Walkbehind_GetDrawingSurface` (added unregistered by upstream's b4f32fba4). Test
  `ScriptExports.OldApiLevelsKeepTheirSaveFunctions` checks the level gate. Measured: `script_exports.txt` unchanged;
  `compare_objects.py` 637 objects identical, the four `Engine/ac` objects differ by exactly the five deleted
  functions; ctest 1170 of 1170; auto-test 2414 of 2414.
- **Found:**
  - The plan's list for step 2 was wrong twice: `ReadExt_363_Objects` stays (`WriteRoomData` writes the
    `v363_objects` block in every room; the 4.0.0.27/30 fixtures carry it), and the LZW background branch stays
    (rooms 4000000 to 4000032 have LZW backgrounds; condition now `>= kRoomVersion_400_33`).
  - `AGS.Native`'s `load_room_script` passed a `String` to `%s` (undefined behaviour, worked on MSVC because `_cstr`
    is the first member); fixed with `.GetCStr()`.
  - Audio's pre-3.6.0.9 save branch left `total_channels` uninitialised in its `else` (deleted with the branch).
  - Upstream's, kept as they were: GUI saves 4000000 to 4000009 have no control refs and `ReadGUI` leaves the old ref
    list empty (the deleted `< 36200` copy should have been `< 40010`), while `GUIMain::SkipSavestate` reads refs from
    them (a FIXME marks it); Game State saves 4000000 to 4000023 keep the running game's `dialog_options_zorder`
    (the reset was under `< 363_02`); `ReadDescription` accepts 4000001 to 4000017 without a file format block.
  - The save floors are conservative: since the Room States floor went to 40003 (1765a7451, 2024-02-14) every floor
    could rise to what that commit wrote and the `< 400_03` branches go; not done. `load_game`'s match by data file
    name is now reached only by a game with neither GUID nor legacy ID (a TODO says so).
  - The script API level is a project setting, not a data version, so it has no 3.99 floor: a 4.0 project chooses
    any level from v321 to v400_33. Kept: the gate in `RegisterGlobalAPI` (below v362 it binds `SaveGameSlot`,
    `RestoreGameDialog`, `SaveGameDialog` to the old functions) and the four runtime switches on
    `OPT_BASESCRIPTAPI`. The plan's "registrations only pre-3.99 scripts use" has no instance.
  - 20 snapshot names that `agsdefns.sh` declares at no level (e.g. `AudioChannel::SetSpeed^1`,
    `DrawingSurface::SetPixel^3`, `InterfaceOff`, `InterfaceOn`, `SystemInfo::get_Version`); the 4.0.0.30 release
    registers them, so removing them is a C3 change. `RegisterSpeechAPI` names an unused `base_api` parameter.
- **Decisions:** 2026-11-08 (the user): the editor's now-unreachable C# upgrade steps (`UpgradeGameCommonTask`'s
  thresholds below 3999901, the 3.6 halves of seven conditions, `ScriptsComponent`'s `< 3060110`,
  `UpgradeGameFontsToFontFilesTask`'s 3.0.3.2 check) are kept: L.1's C# half, or still reachable above the floor.
- **Lessons:** `.clang-tidy` got `bugprone-signed-char-misuse.CharTypdefsToIgnore: 'int8_t'` (de-indented
  `ReadInt8()` lines became changed lines and were flagged even with `static_cast<int>`). In a worktree
  `refactor_metrics.py` stops with "Run from the repository root."; GitHub release downloads gave 504, worked around
  with `AGS_FETCH_MIRROR_DIR`.
- **Not verified / left open:** step 5 (savegame-only fields) waits for the next savegame format bump (C2). For steps 1-4: macOS, iOS,
  Android, FreeBSD, the Debian 10 builder image; real 3.6 games, `.crm` files and saves, and 3.99.99 saves (refusals
  checked with written or patched headers); the auto-test game with hardware drivers; each commit on its own (only
  final trees built). Step 2: the `linux` stage, the corpus run, the editor's import and template dialogs by hand,
  opening a 3.99.99.01 project in the editor window. Step 3: the Linux clang-tidy gate with the new option. Step 4:
  a game compiled below v362. `compare_objects.py` does not apply to steps 1-3 (runtime conditions).

### 3.5 Unused functions

- **Definition:** confirm the dead-code candidates of `refactor_metrics.py --list-dead` (154 at the time, many false
  positives) with linker-verified builds and delete in batches. *Done when:* the list holds only documented
  exceptions.
- **Result (2026-11-08):** done, 14 commits (all "(3.5)"); 283 production functions and 10 test helpers deleted.
  - Method: the linker decides. (1) GCC in WSL at `-O0` with `-ffunction-sections -fdata-sections`,
    `--gc-sections --print-gc-sections`, once with `NDEBUG` and once Debug, `nm` over 526 objects against 17
    binaries (an optimised build cannot tell inlined from unused, and CMake forces LTO on "Release" types);
    (2) MSVC x64 `/Od /Ob0 /Gy`, `/OPT:REF /OPT:NOICF /MAP`; (3) `AGS.Native.dll` Debug x86 map (38 Linux-dead Common
    functions are live there only). Helpers `.compare/helpers/dead-functions/gc_build.sh`, `collect.py`. Blind spots covered by reading
    every word hit inside `#if` blocks, the macOS/iOS/Android files, a Debug build with the debug switches on, and
    the callers of virtual families.
  - Deleted: Engine 80 functions (+1 −891 lines, 89 files), including the whole `FileBasedAGSDebugger` and
    `D3D11GfxFilter`/`AAD3D11GfxFilter`, `get_eip_guiobj`, `AGSPlatformDriver::GetIllegalFileChars` and overrides;
    Common 189 (+2 −2858, 100 files), mostly the game-data, translation and GUI writers no program uses (the tools
    have their own writer), then the GUI controls' virtual `WriteToFile` family; Tools 7 and 3 test print helpers;
    Compiler 6; test stubs 7. Test-only functions went only when their whole family was dead; 4 test cases and one
    Tools test went.
  - Measured: `dead-code candidates (unverified)` 151 → 0, new metric `dead-code exceptions (documented)` 8;
    `functions scanned` 8935 → 8695; lines Common 50944 → 47958, Engine 125665 → 124742, Tools 29032 → 28909,
    Compiler 34948 → 34883, Editor 6591 → 6586; `-Wunused-function` warnings 12 → 1; MSVC function symbols
    112929 → 112088. `compare_objects.py`: pure-deletion batches "1929 removed, 0 changed, 0 added"; with the two
    virtual families 1946 removed, 62 changed (each only by vtable displacements one x64 slot down), 1 added (a
    renamed vcall thunk). Verified: prepush all stages, ctest 1165 of 1165, NUnit 556 of 562 (6 skipped), auto-test
    2414 of 2414.
- **Found:**
  - The gc builds were wrong three times, caught by reading `#if` hits: `CfgWriteFloat` is called under
    `AGS_PLATFORM_MOBILE` (`mobile_base.cpp:149`); `Stream`'s `ReadAndConvertArrayOf*`/`WriteAndConvertArrayOf*` (10)
    and `SwapBytesFloat32`/`IntFloatSwap` serve the big-endian path; `String::FillString` is called by the
    `String(char, size_t)` constructor. All kept.
  - `stream.h`'s big-endian branch does not compile (it calls `ReadAndConvertArrayOfInt24`, the function is
    `...UInt24`).
  - The `DEBUG_SPRITECACHE` and `DEBUG_PATHFINDER` builds fail at HEAD (`spritecache.cpp`'s `LoadSprite` log uses the
    private `ResourceCache::_cacheSize`; `route_finder_impl.cpp:119` names undeclared `Debug`, `xx`, `yy`).
  - Not touched: `RunEventInModules`' last parameter is always `false`; `SPRCACHEFLAG_LOCKED` is set and never read;
    `DebugManager::_freeGroupID` is only reset; 36 IAGSEngine members no plugin in the tree calls (C4 decides);
    `currentline` in `cc_internallist.cpp`, an unused variable, left for 3.6.
  - `compare_objects.py` had two bugs, fixed on the way: dumpbin's closing "Summary" line counted as code of the
    object's last function, and MSVC's local `$LN<n>` labels taken as functions. It now tells a deletion from a code
    change.
  - `prepush.py`'s format and emscripten stages opened files deleted in the working tree; it now lists tracked files
    that exist (`tracked_files`).
- **Decisions:** exceptions kept and listed in `DEAD_CODE_EXCEPTIONS`: the 7 iOS functions of `acplios.cpp`,
  `ReadAndConvertArrayOfUInt24`, `engine_set_pre_init_callback` and `pl_register_builtin_plugin` (Xcode project),
  copy/move special members (`= delete` is a decision), everything under `Plugins/` (phase 7),
  `cc_bytecode_test_lib.cpp`'s `WriteOutput*` (3.6), `CCStaticObjectArray`. `CCStaticObjectArray` is an open item: no
  value of its kind is made any more, but the script VM names it in `kScValStaticArray` branches on the hot path, so
  deleting it needs a Release codegen comparison first.
- **Not verified / left open:** macOS (`acplmac.cpp`'s override, the Xcode project lines), iOS, Android, FreeBSD, the
  Debian 10 builder image, a big-endian build; AGS.Native x64 or Release; MSVC Debug asserts in Windows-only code;
  header inline functions and templates never instantiated; virtual members other than the two families; each
  commit on its own beyond the per-batch build and ctest. `native_build.py` copies its Debug DLL over the
  git-ignored `Editor/References/AGS.Native.dll`.

### 3.7 Obsolete API stubs

- **Definition:** keep the plugin stubs games still need, drop the rest; fold `Common/util/wgt2allg.*` into
  `BitmapHelper` and delete it.
- **Result (2026-11-09):** done on branch `task-3.7`, three commits, 16 files, +30 −170.
  - `wgt2allg.*` gone (`[move]`): `wputblock_raw` is `BitmapHelper::BlitAllegroBitmap(dst, x, y, src, transparent)`
    in `Common/gfx/bitmap.*`, used by `IAGSEngine::BlitBitmap` only; the file-scope wrapper `Bitmap` is a local.
    `compare_objects.py` (MSVC x64): 634 objects identical, `bitmap.cpp` and `agsplugin.cpp` differ as expected.
    `allegro.h includes` 41 → 40.
  - Stubs dropped (`[delete]`, 272 → 198 lines) for the five built-in plugins (`ags_snowrain`, `ags_SnowRain20`,
    `agsblend`, `agsflashlight`, `agsspritefont`). They were reached only by an engine without `BUILTIN_PLUGINS`
    with the DLL missing; such a game now stops at "N unresolved imports" instead of running without the effect.
    Kept, reasons in the file header: `agsteam*`/`agsgalaxy*`, `agswadjetutil` (closed source), `ags_shell`/
    `agsshell`, `agsappopenurl`, `agsjoy` (defunct).
  - Verified: prepush all stages (linux ctest 1165/1165), `--only sanitizers` clean in 1060 s.
- **Lessons:** the agent writing this entry truncated PLAN.md to 0 bytes (a Python script opened it for writing, then
  failed on a non-ASCII character); restored from HEAD plus the pending diff. Scripts that rewrite a tracked file
  read and write UTF-8 and write to a temporary file first.
- **Not verified / left open:** the "DLL missing, no built-ins" case itself; whether Android's build sets
  `BUILTIN_PLUGINS`. After the tidy fix only format, buildlists, tidy and build were re-run.

### 7.1 Make the C++ tools the only build path

- **Definition:** (D7) games build through the C++ tools on Windows, Linux and macOS, byte-identical to the
  editor's former output; the editor builds through them (6.7d); no C# build code remains.
- **Starting point (2026-11-01):** `agsbuild` could not build an AGS 4 project. Five gaps: `agfreader` rejected
  version-string projects (since `ca79f81c5`); no AGS 4 room compiler for `Rooms/<n>/`; `agscc` knew API levels
  only up to `v363`; the C++ preprocessor lacked `#module`; `agsbuild` hung after failed tasks.
- **Result:** 7.1a-7.1j, 7.1m, 7.1o done 2026-11-01 to 2026-11-03; 7.1l done by 6.7d-9/6.7d-10 (2026-11-07: the
  editor's `game28.dta` is `agf2dta`'s, the harness compares without `unembed_scripts`). Room scripts stay in
  the `.crm`; the engine keeps reading embedded scripts (C1).
- **Decisions:** 7.1n (MO files) dropped 2026-11-03 unless upstream asks (#1780 keeps `.tra`; propose it there
  first). The thin format tools (`agscc`, `crmpak`, `spritepak`, `agspak`, `po2tra`) stay, with tests for their
  options.

### 7.1a Build-parity harness

- **Definition:** `Script/compare_builds.py` builds a project with the editor (`AGSEditor.exe /compile`, from Git
  Bash with `MSYS_NO_PATHCONV=1`) and with `agsbuild`, unpacks both (`agspak -e`) and prints a per-file table.
- **Result (2026-11-01):** `compare_builds.py <project> --editor <AGSEditor.exe> [--engine <acwin.exe>]`, tools
  from `.tidy/build`, builds under `.compare/builds/`, exits 0 only on full parity. First run: editor 17 files,
  `agsbuild` stopped on `#module` (7.1c) and room sources (7.1e).

### 7.1b Unblock the pipeline

- **Definition:** version-string projects, AGS 4 API levels in `agscc` with a drift test, `agsbuild` cancels on
  failure. Project upgrades stay in the editor; the tools say "open and save this project in the editor first".
- **Result (2026-11-01):** `agfreader` accepts formats from 4.0.0.34 on. One API level table,
  `kScriptAPINames` in `Common/ac/gamestructdefines.h`, used by `agscc` and `agf2dta`;
  `Common/test/scriptapi_test.cpp` guards it.
- **Found:**
  - `agf2dta` had its own API level list ending at `v363`, so it wrote the wrong API level into AGS 4 games.
  - The hang was in `PipedProcess` on Windows: a read could ask for more bytes than the pipe held (unsigned
    underflow) and block forever, because `agsbuild` kept its own copy of the write end. Fixed by bounded reads,
    closing our write ends after start, non-inheritable read ends, process start under one lock;
    `Tools/test/pipedprocess_test.cpp` aborts after 120 s on the old code. Upstream had it too.
- **Left open:** the POSIX branch reads stdout to EOF before stderr; a child filling stderr could deadlock it.

### 7.1c One preprocessor

- **Definition:** port what the C++ preprocessor lacks from `Preprocessor.cs`, move the C# tests to C++.
- **Result (2026-11-01):** `#module` was the only missing directive (marker
  `"__NEWSCRIPTSTART_<script>;<module>"`); all 21 C# tests already had C++ copies;
  `Compiler/test/preprocessor_test.cpp` (31 tests) now runs in `compiler_test`. All 40 auto-test scripts
  compile with `agscc`.
- **Found:**
  - `agscc` dropped the headers with the new compiler (no script saw `agsdefns.sh`); it now joins headers and
    script, as the editor does.
  - `agfreader` read `<Name>` for dialogs, items, views and GUI controls, which AGS 4 saves as `<ScriptName>`,
    so `_AutoGenerated.ash` left them out. Fixed with a test.

### 7.1d Table-driven XML reading

- **Definition:** (maintainers' design) one table per struct, rows naming member, element and reader; one generic
  `ReadFields`; defaults in member initializers, a missing element leaves the member alone. *Done when:* no
  hand-written `ValueParser::Read*` call remains.
- **Result:** `Tools/data/xml_fields.h` (2026-11-01): `XmlField<T>`, `Read<&T::M>`, `ReadWith<&T::M, Parse>`,
  `ReadElement<&T::M, Fn>`, `ReadFields`/`ReadList`; a null reader marks a known element deliberately not read;
  `UnknownElements::Skip` for tables that read part of an element. Tests `Tools/test/xml_fields_test.cpp`.
  All of `agfreader` migrated 2026-11-02, harness at 17 of 17 at each step. Left as code: entity references,
  interaction events, audio folder defaults, palette and sprite attributes, dialog file lookup. Readers use
  `UnknownElements::Report` into `XmlProblems` (`ReadAllData` is `void`).

### 7.1e AGS 4 room sources

- **Definition:** compile `Rooms/<n>/` to `.crm` in C++ (port of `convert_room_to_native` and
  `convert_room_interactions_to_native`). *Done when:* rooms byte-identical to the editor's.
- **Result (2026-11-01):** all three auto-test rooms byte-identical. `Tools/data/room_xml.*` reads every room
  format from 3.99.99.01 with the editor's load-time upgrades; `Tools/data/room_build.*` builds. New tool
  `crmbuild` (`crmbuild Game.agf <n> -o room<n>.crm --script room<n>.o`, `--header`); `agsbuild` runs header,
  `agscc`, room. Different mask resolution is an error (resizing stays in the editor); 256-colour games refused
  for now. Tests `Tools/test/room_build_test.cpp`.
- **Found:**
  - `scriptgen` did not follow the editor's `RegenerateScriptHeader` (order, `import readonly X *`), changing
    the compiled scripts' type info and TOC.
  - `agscc` lacked `SCRIPT_EXT_*`, `NEW_KEYINPUT_API` and `UNICODE`; without `SCRIPT_EXT_MANAGEDBASE` every
    managed type lost its base. Also added: TOC with `-g`, `--name`, `--ascii` reaching the new compiler.
  - `agsbuild` read obsolete `-fforce...=0` settings from elements the editor no longer saves.
  - `crmpak -e <room> script <file>` crashed (`ReadOnlyCompiledScript` passed a null room). Fixed later; upstream
    too.
  - `ImageFile::LoadPNG` copied only `width*height` bytes of RGB(A) images when no channel swap was needed. Fixed
    later with `Common/test/imagefile_test.cpp`; upstream too.
  - `agf2dta` indexes interaction events by the XML `Index` instead of the cursor schema (left open).

### 7.1f Content parity

- **Definition:** every file identical and the auto-test passing on `agsbuild` output.
- **Result (2026-11-02):** the editor's 17 files identical; compiled scripts packed separately (40 `.o` and
  `ScriptModules.lst`, loaded by the engine since 3.6.1, upstream `4de348c79`), each matching the editor's
  embedded copy; auto-test 2414 of 2414. Every script compiles against all module headers (upstream PR #1408).
  Added the AGS 4 project format to `agfreader` and the nine AGS 4 extension blocks to `data_file_writer`
  (`ext_ags399`, `v400_gameopts`, `customprops`, `fontfiles`, `guictrlgfx`, `eventtables`, `viewevents`,
  `charopts2`, `charinv`).
- **Found:**
  - Hex colours: `strtol` saturated them to `0x7FFFFFFF` on Windows; now read as `Convert.ToInt32(s, 16)`.
  - Written differently from the editor: inventory item 0 twice; views not by ID; 6-bit palette; text
    properties not unescaped; score sound and maximum score not 0; dot colour without marker style; trailing
    space in "compiled with" (`"4.00.00.34 "`); `dev_error_text` missing.
  - The dialog script converter dropped every dialog's last line (upstream too).
  - `acsetup.cfg`: `show_fps` condition inverted (upstream too); now written with `IniUtil::Merge`.
  - `compare_builds.py` did not clear its unpack folders.
- **Decisions:** `agf2dta --script-dir` removed after maintainer review (the build does not need it; it made the
  game data wait for every compile).
- **Left open:** one test project measures parity; translations, 8-bit games and old indexed interactions are
  not exercised.

### 7.1g Drift guard

- **Definition:** unknown elements reported; a property added in `AGS.Types` without teaching the tools (or
  listing it as editor-only) fails a test.
- **Result (2026-11-02):** `agf2dta` prints `XmlProblems` as warnings, errors with `--strict` (also via
  `agsbuild`). `AgfElementsTests` (C#) checks serialized names against `Tools/data/agf_elements.txt`
  (regenerate with `AGS_UPDATE_AGF_ELEMENTS=1`); `AgfElements` (`Tools/test/agfreader_test.cpp`) checks every
  name has a row; a row without reader means editor-only, reason in a comment. 37 classes.
- **Found:**
  - Characters' `Enabled` and `Visible` never read; the writer always set both.
  - Sprites' `ImportColorDepth` (`SPF_KEEPDEPTH`) not read or written.
  - Global variables' `ArrayType`/`ArraySize` ignored; defaults not lower-cased (`bool b = True;`).
- **Left open:** `spriteimport` does not re-import as 8-bit for `ImportColorDepth` 8; rooms and translations not
  on the list.

### 7.1h Diagnostics for the editor

- **Definition:** machine-readable diagnostics and clean cancel, so the editor (6.7d) can list errors and cancel.
- **Result (2026-11-02):** `agsbuild` parses the tools' existing output (`Tools/agsbuild/diagnostics.cpp`)
  instead of each tool printing JSON. `--json`: stdout carries only JSON lines (format in
  `Tools/agsbuild/diagnostics.h`). Cancel by SIGINT/SIGTERM or `cancel` on stdin: tools killed
  (`PipedProcess::Terminate`, process group on POSIX), exit code 2, no tool processes left. `tools_test` 109
  pass; parity 17 of 17, auto-test 2414 of 2414.
- **Not verified:** the POSIX side of `Terminate` locally.

### 7.1i Split `BuildWithTaskManager`

- **Result (2026-11-02):** 535 → 45 lines: `ReadProject`, then one `Add*` function per stage on a `BuildContext`.
  Order unchanged (it decides file order in the package); 17 of 17 identical.

### 7.1j CI

- **Definition:** a Linux job builds the auto-test with `agsbuild` for the platform jobs (1.7); a Windows job runs
  the parity harness until 6.7d.
- **Result (2026-11-02):** `Script/agsbuild_autotest.sh` (strict mode, 2414 of 2414 locally); the `Auto-test
  game` job builds tools only and uploads `auto-test-game` for every `build` job; `Windows Build Parity` runs
  `compare_builds.py` (green at 2a43f3092). Green in runs 41442221520 and 41461852760.
- **Lessons:**
  - `Engine/util/library_posix.h` used `Path` unqualified after using-directives left the headers: every POSIX
    build broke.
  - MSBuild `Tools.Test` did not compile `scriptgen.cpp`.
  - Android configures CMake eight times, each downloading five dependencies; DNS blips failed the job.
    `Script/fetch_deps.sh` downloads once with retries into `AGS_FETCH_MIRROR_DIR`.

### 7.1m PO translation sources

- **Definition:** (found by 1.4) `agsbuild` asked for `<Name>.trs`, so AGS 4 projects with a `.po` failed.
- **Result (2026-11-02):** `ReadPO` follows `Translation.LoadDataImpl`, `WriteTRA` follows
  `TranslationsComponent.CompileTranslation` (`Tools/data/tra_utils.*`). The private game's `Italian.tra` is
  byte-identical to the editor's.
- **Found:** `StrUtil::GetKeyValue` ignored its separator and split on `=`: TRS annotations and PO contexts
  (`PARSERWORD:5`) were never read.
- **Not verified:** font overrides in a `.tra`; non-UTF-8 translations with escapes above 127.

### 7.1o `trac` → `po2tra`, decompiling to PO

- **Result (2026-11-03):** tool renamed `po2tra`; `WritePO` replaces `WriteTRS` (editor's layout, entries
  sorted, parser words as `??? (word <id>)`). PO → TRA → PO → TRA tests; on the private game's PO (783 texts,
  79 with escapes) both `.tra` decompile to identical POs.
- **Decisions (2026-11-03):** upstream gets `po2tra` next to its own `trac`, which stays; removing `trac` needs
  the maintainers' agreement.

### U.3 Pieces for #2316

- **Open:** branches 2 (`agfreader` reading the AGS 4 format as plain `ValueParser` reads, with the AGFReader
  tests; no tables or drift guard, per the review), 3 (`agf2dta` parity) and 4 (`crmbuild`), #3187-#3189, stacked
  on `upstream/ags4` (ledger in `docs/UPSTREAM.md`).
- **Decisions:** `--json`, cancel and the stage split stay fork-only until #2530. The game info stays unescaped,
  as the 4.0 editor writes it.
- **Lessons:** upstream is C++11; `.compare/helpers/port.py` ports hunks (and converts `xml_fields` rows to plain
  reads), `.compare/helpers/cxx11.py` checks `-std=c++11`. `verify_strict.sh` once reported "ok" with a failed
  test (now fails on any `FAILED`).

### U.4 Agent docs upstream

- PR #3185 (base `master`, `066cb1152`), open: `AGENTS.md` and `docs/context/`.
- **Decision:** the fork's `docs/context/` is a copy; review changes are made in the worktree and copied here,
  never edited here first.

### 6.0 study: ags-for-mac, #1092, #3177

- **Result (2026-11-04):** evidence in `EDITOR-SPLIT.md`. Measured on the fork: 17 `AGS.Types` files use WinForms
  (5 only through an unused `using`); 112 `[Editor]` on 10 `UITypeEditor` classes, 11 `[PropertyTab]`, 91
  portable `[TypeConverter]`; no `System.Drawing.Font` or `Bitmap` in the model (the earlier "55 Font / 15
  Bitmap" were AGS's own `Font`); `Icon`/`Graphics`/`Bitmap` only in plugin interfaces and templates; 9 static
  GUI delegates. ags-for-mac's `libagsnative`: 3,072 lines, 72 C functions, 38 of them rooms.
- **Decisions (2026-11-04):** 6.0 step 1 goes upstream as U.6. 6.7c reuses the library's conventions and its
  image, sprite-file, font and GUI entry points, not the room half. The user: breaking editor plugins is
  acceptable, the editor plugin API leaves C5, 6.0 step 2 needs no further gate.
- **Not verified:** nothing was built.

### U.6 AGS.Types property grid out

- **Status:** PR #3192 open, branch `ags4-types-propertygrid` on `de5508362`: `b622391cd` move (16 renames into
  `AGS.Controls/PropertyGridExtras`), `553a83a4a` usings, `11936b3ef` tests. NUnit 382 of 382; the user checked
  the grid in the running editor.
- **Decisions (2026-11-04):** (review, ivan-mogilko) the classes go to `AGS.Controls`, controls must not call
  `GUIController`, upstream's static delegates stay; the fork's 6.0 step 1 uses the same layout.
  `PaintColorBlock` stays in `AGS.Types.Utilities` (the user). Next step for #1092: semantic attributes in the
  model and a host-side `TypeDescriptionProvider`.
- **Lessons:** the editor assembly is `AGSEditor`; the `UITypeEditor` base name must be fully qualified or
  `ReflectTypeDescriptionProvider` drops the editor silently; upstream's `Utilities.cs` mixes LF and CRLF, so
  stage it with `git -c core.autocrlf=false add`.
- **Not verified:** the Debug configuration; a clean checkout without `Editor/References`.

### 1.3 Engine unit tests for the riskiest subsystems

- **Definition:** tests for the script VM, route finder, overlay/GUI layout, savegame components, `scsprintf`,
  sprite cache eviction, with fixtures for the globals they read. *Done when:* engine test files >= 15.
- **Result (2026-11-05):** 15 commits. `Engine/test/*.cpp` 6 → 16,
  `engine_test` 4 → 51, `engine_full_test` 8 → 111 (it now builds `Compiler/script/*.cpp`), ctest 881 → 1031 of
  1031. Mutation checks fail the expected tests. `compare_objects.py`: 602 objects identical.
- **Found (fixed, each with a test that fails without it; all in upstream `ags4` too):**
  - `RuntimeScript` destructor compared the signed link index with `0u`: slot 0 never freed, `_linkedScripts[-1]`
    read on a failed creation.
  - Link index stored as `uint8_t`: the 257th script got index 0.
  - `CalcPartsFromProgress` gave a backward run 0 steps: progress inf/NaN, a speed change jumped the walker back.
  - `GetGUIAt(kHit_Any)` used the unscaled rect.
  - `ReadComponent` read through a null handler for an unselected component (crash via
    `OPT_SAVECOMPONENTSIGNORE` or a retry).
  - Character inventory quantities saved in `unordered_map` order: save-restore-save bytes differed.
  - `ResourceCache::DisposeFreeItems` disposed the locked section, kept the free items, and erased nodes while
    iterating: `clear_cache_on_room_change` freed nothing.
- **Found (not fixed):** `FindNearestWalkablePoint` (3.6.2) skips the bottom row near the top edge (disabled
  test; a fix moves characters in existing games); `ccCompileText` section-name condition inverted (upstream
  too); `String.Format("%*d", ...)` passes `*` to `snprintf` (UB); `kSaveRestore_AllowMismatchExtra` would write
  past `game.chars` (never set); no opcode-range or code-end check in Release; joint RTTI matches types without a
  layout check and never resets; `GUIControl::IsOverControlImpl` margin only left/top; the one-argument
  `SpriteCache` constructor leaves callbacks empty; scaled-graphic AABB spans corner pixel origins;
  `WorldToLocal` truncates toward zero; the Repeat flow never shows the last point.
- **Not verified:** GCC/Linux/macOS, a Debug build, ASan, the auto-test game, the private corpus; the tests are
  CMake-only at the time (added to the MSBuild test projects in 3.2c).

### 6.7d The editor builds through the tools

- **Definition:** Build runs `agsbuild` (D7) with progress, `--json` errors with jump-to-line and cancel; tools ship with the editor; then the C# build code goes (`DataFileWriter.cs`, the dialog converter, the `.tra` writer, the in-process compile, the C# preprocessor). *Done when:* output byte-identical to the last C# build on the two test projects and every 6.7d-1 variant, tests pass, the C# build code is gone. Split 2026-11-06 into 13 steps: tool gaps first (ground rule 3), then the switch, then the deletions.
- **Result:** done 2026-11-07 (6.7d-1 to 6.7d-8 on 2026-11-06, 6.7d-9 to 6.7d-13 on 2026-11-07). Reference editor: tag `pre-6.7d`, MSBuild Release x86 (locally `.compare/msb-pre67d`). Baseline before the switch: auto-test 59 files all identical (auto-test 2414 of 2414; editor 4 s, `agsbuild` 1 s); a private AGS 4 game (53 rooms, 36 scripts, a translation) 114 files identical (editor 12 s, `agsbuild` 3 s). Final: this editor against `pre-6.7d` with `compare_builds.py --tools-editor`: auto-test 59 of 59 identical, private AGS 4 game 208 of 208, private 8-bit game 53 of 53, parity in all ten variants of each; auto-test template byte-identical (119 assets); Ctrl+F5 2414 of 2414. 6.7d-12 removed about 5,900 lines.
- **6.7d-1 Parity matrix** (2026-11-06): `compare_builds.py --variants all` / `--list-variants` builds ten variants (`release`, `split`, `opengl`, `customcfg`, `packed`, `speech`, `customdirs`, `audiovox`, `noname`, `unicode`; a variant can build in a non-ASCII folder); unpacks `*.vox`; a failed build is no parity. Only `audiovox` had parity at first.
- **6.7d-2 Game data and configuration** (2026-11-06): four tool fixes (Found); `agfexport game-cfg --base <file>` merges the project's `acsetup.cfg` (`IniUtil::Merge`); mouse speed `%.7g`. `release`, `opengl`, `customcfg`, `noname` have parity; a release build runs the auto-test 2414 of 2414. 170 tools tests (`Tools.Test.vcxproj` did not compile `game_utils.cpp` before).
- **6.7d-3 What is packed** (2026-11-06): `Tools/agsbuild/package_files.{h,cpp}` (22 tests): loose files, custom data folders with the editor's checks and warnings, voice packs as `SpeechComponent` makes them, split packages with `agspak -p` (parts `.001`). Stale parts, an unneeded `audio.vox` and removed voice packs are deleted after a successful pack. `packed`, `customdirs`, `split`, `speech` have parity (63, 62, 61, 67 files); split build runs 2414 of 2414; `tools_test` 174 → 199.
- **6.7d-4 UTF-8 command lines** (2026-11-06): `CmdLineOpts::InitProgramArgs` in all eleven programs (`CommandLineToArgvW` as UTF-8, Allegro `set_filename_encoding`); `agsbuild` quotes each argument by platform rules (`ProcessArgs`). `unicode` has parity. `common_test` 110 → 113, `tools_test` +4.
- **6.7d-5 256-colour rooms** (2026-11-06): `crmbuild`'s `LoadRoomImages` remaps an 8-bit room's indexed background as the editor does (`RemapBackground`): game-wide colours of the game palette, only in the slots the image's palette has (length from the PNG `PLTE` chunk); each 8-bit frame keeps its palette, the first is the room's. Private 8-bit game (9 rooms, 6 dialogs): 53 of 53 identical, parity in all ten variants; a 20-colour-palette copy 53 of 53. `tools_test` 218 → 225 (`dialogscriptconv_test.cpp` new).
- **6.7d-6 Headers from outside, run layout, `_obj/`** (2026-11-06): `--header <file>` (repeatable, between the global variables header and module headers, section named by file name); `--data-only` (package with only `game28.dta` and scripts, rooms in `<out>/Rooms/`); `--temp-dir` defaults to `<project>/_obj`, cleared at start (only the default), `-c` deletes it after success; `Directory::DeleteDirectoryTree` (links deleted, never followed); `agsbuild`'s own option parser (`commandline.cpp`). Measured: folders always win over packages (`kAssetPriorityDir`). Command line: `<engine> [--windowed|--fullscreen|<log groups> --enabledebugger <id>] --runfromide "<proj>\_Debug" "<proj>\_Debug\Rooms":"","<proj>":"","<proj>\Fonts":"","AudioCache":"audio","Speech":"voice" "<proj>\_Debug\<name>.ags"`. 2414 of 2414 with a broken stale `room1.crm`; a halfway failure leaves nothing new outside `_obj/` and the output folder. `common_test` 113 → 117, `tools_test` 199 → 218; all ten variants parity; private game 208 of 208.
- **6.7d-7 Tools next to the editor** (2026-11-06): `crmbuild` got an MSBuild project (from upstream port branch `4b17e4b9c`, toolset v143) in `Tools.sln`, 59 of 59 and 2414 of 2414; the editor build copies `agsdefns.sh` next to it (decision T2); the editor CI job takes the tools from `windows-msvc-tools` and fails if one is missing; `ags.iss` lists them. Confirmed on probe branch `probe-67d7` (run 42213937471).
- **6.7d-8 `BuildToolRunner`** (2026-11-06): `Editor/AGS.Editor/Build/`, C# 6, no UI types: parses `--json` lines into progress and `CompileMessages` (duplicate warnings dropped); cancel writes `cancel`, after 5 s ends the tool's job object (kill-on-close, so the tools die with the editor). Editor tests 396 → 522.
- **6.7d-9 Build through `agsbuild`** (2026-11-07): `AGSEditor.BuildGame(forceRebuild)` for Build EXE, Rebuild all, `/compile` and the new-game wizard: IDs repaired, save, `PreCompileGame`, checks, one busy dialog with progress and Cancel running `agsbuild` into `Compiled/Data`, then the other targets copy from there. Plugin headers written by `Build/ScriptHeaderFiles.cs` to `%TEMP%`, passed as `--header`. Every build scans rooms for unlinked event handlers (`ScanAllRoomsForMissingEventHandlers`). `compare_builds.py --tools-editor <AGSEditor.exe>`. Parity: 59 of 59, 208 of 208, 53 of 53, all ten variants each. Errors in a module, room and dialog get the reference's names and lines. Cancel ends the build 0.25 to 0.8 s after the press, no tool left running. Room compiler warnings now reach the output.
- **6.7d-10 Run through `agsbuild`** (2026-11-07): F5/Ctrl+F5 call `AGSEditor.BuildGameForRun`, `agsbuild --data-only` into `_Debug`, the editor's own `acwin.exe` started with 6.7d-6's command line; nothing copied into `_Debug`; a Run needs no earlier Build EXE. Ctrl+F5 2414 of 2414 (reference editor 2411: three `FillDirList` tests); F5 with a broken stale `room1.crm` 2414 of 2414; an injected array index error breaks into the editor at line 11 of `room1.asc`. The engine gets 45 of 45 files identical to the reference Run's (`game28.dta`, 40 scripts, 3 rooms, `acsetup.cfg`). Cost: build before a Run of the private AGS 4 game 1.9 s, 1.8 s (reference) → 8.4 s, 8.8 s (this editor); led to 7.1k.
- **6.7d-11 The other users of the build code** (2026-11-07): `Build/ToolProcess` runs text-output tools (18 tests); a room save writes `data.xml` and modified images only (room script errors now come from the build, not Ctrl+R or closing a room); "Compile translation(s)" run `po2tra` (`Build/TranslationCompiler`); templates packed by `agspak -c` from `_obj/template/` (`Build/TemplatePackage`), a room template's `.crm` built by `crmbuild`; nothing written into the project folder. Editor NUnit 567 of 567; 59 of 59; template byte-identical (119 assets).
- **6.7d-12 Deletions** (2026-11-07): eleven commits, each built and tested: the editor's C# build (`CompileGame`, `CreateMiniEXEForDebugging`, the C# voice packs and others), the C# `.tra` writer, `DataFileWriter.cs` (2,557 lines), the `AGS.Native` room writer, the in-process compile (`IScriptCompiler`, `ScriptCompiler.cpp`, `Script.CompiledData`, `GetScriptModuleList` and others), `DialogScriptConverter.cs`, the C# preprocessor and its 22 tests, the compiler setting's list; two moves (EXE resource functions to `NativeMethods.cpp`, `_GlobalVariables.ash` text to `Tasks.MakeGlobalVariablesHeader`); `AgfexportHeaderTests` compares both editor headers with `agfexport autoash` and `glvar`. Native warnings 25 → 24; NUnit 547 of 547; 59 of 59, all ten variants parity, template identical, Ctrl+F5 2414 of 2414.
- **6.7d-13 Harness, CI, docs** (2026-11-07): `compare_builds.py` un-embeds scripts only when the editor's package lacks separate script files; otherwise plain byte comparison. CI's `Windows Build Parity` job unchanged but its comment (guards that the editor adds and loses nothing against a plain `agsbuild` run). CI's mode 59 of 59, 2414 of 2414, all 10 variants parity; old mode against the reference still 59 of 59. AGENTS.md updated; PLAN.md: 6.7d and 7.1l done, 3.3 rewritten for `AGS.CScript.Compiler`.
- **Found:**
  - `agf2dta` wrote the room names block only in debug mode; the engine reads it from every game of data version 4.00.00.13+ (`read_room_names`), so a release-mode `game28.dta` was misread. Fixed in 6.7d-2.
  - `agfexport game-cfg` ignored a project `acsetup.cfg`, wrote `OpenGL` instead of `OGL`, rounded the mouse speed (`%.f`); an empty `GameFileName` gave `.ags`. Fixed in 6.7d-2.
  - Editor bug: split package parts named `.1D3`, `.2D3` (`i.ToString(".D3")`; `.001` was meant). Not fixed in the editor.
  - `agsbuild` did not pack `preload.*`, `flic*.fl?`, `*.ogv`, ignored `SplitResources`, failed on a voice folder without voice files and on nested custom data folders, lost the voice sub-folder prefix, kept folder case, skipped `*.flac`. Fixed in 6.7d-3.
  - `agspak -p` wrote broken split packages (first part's table written before later offsets known; later parts checked as if they had one). Fixed in 6.7d-3.
  - Include patterns had no root anchor (`file1.txt` matched `CustomData/file1.txt`) and `ClipMid` stood for `Mid`, breaking every `[...]` class. Fixed in 6.7d-3.
  - `AGF::ReadAudioClips` did not read the clip's name, so a clip with no source or cache was skipped silently. Fixed in 6.7d-3.
  - Narrow `argv` on Windows: a non-ASCII project path gave "project file not found"; `agspak` could not unpack either. Fixed in 6.7d-4.
  - `StrUtil::ConvertUtf8ToWstr`/`ConvertWstrToUtf8` wrote the terminator one past the buffer and handled no surrogate pairs (an emoji folder name reached the child tools mangled); `Path::WidePathToUTF8` freed an array with `delete` (above 1024 bytes). Fixed in 6.7d-4.
  - tinyxml2's narrow `fopen` did not find a project in a non-ASCII folder. Fixed in 6.7d-4 (`ags_fopen`).
  - A game name with quotes (`--gamename` of `po2tra`) broke `agsbuild`'s child command lines. Fixed in 6.7d-4 (`ProcessArgs`).
  - `crmbuild` refused 8-bit rooms. Fixed in 6.7d-5.
  - Dialog converter: double quotes in speech not escaped (a `TODO` since the port; `agscc` failed); the function opening put on its own line and empty lines dropped, so line numbers did not match the dialog script (first statement line 2 instead of 25). Fixed in 6.7d-5.
  - `agf2dlgasc` printed messages only when there were none (`if (errors.empty())`), so a bad line was dropped and the build went on. Now prints `Error: at Dialog <id>, line <n> : <text>` and fails. Fixed in 6.7d-5.
  - `this` next to a non-ASCII byte went through `std::isalnum` with a negative value; character `CAnn` not found as `ann`. Fixed in 6.7d-5.
  - `<ScriptModule />` (empty) on a character or item became `GlobalScript.asc`; the editor reads an empty name. Fixed in 6.7d-5; a GUI's default is `GlobalScript.asc`.
  - `agsbuild --json`: unknown options accepted silently, no JSON for bad arguments, a succeeded task's `Error:` lines reported as errors (`spriteimport`), a failed task could have an empty message (`Task::Run` never stored its result), tools inherited every handle, `_temp` never removed. Fixed in 6.7d-6, each with a failing-without-fix test.
  - `agsbuild --json` did not exit after the build while stdin stayed open (thread in `std::getline(std::cin)` held the stream lock the C runtime takes at exit); `cancel` after the end crashed it. Fixed by the supervisor (6.7d-8): read with `ReadFile`/`read`; exit 0 3 of 3, `cancel` exit 2 3 of 3.
  - `agsbuild` reported a missing `agscc.exe` as "Process failed with err code: 2" per script (user's first Rebuild all; `agscc.exe` was built only by `Compiler.App.sln`). Fixed in 6.7d-9: `Tools.sln` holds `Compiler.App` and `Compiler.Lib`; `CheckBuildOptions` checks every tool before starting any.
  - Editor: "Import text" (Make default language) changed hotspot/object names without marking the room modified, so since 6.7d-9 those changes never reached disk. Fixed in 6.7d-11 (`AGSEditor_ProcessAllGameTexts`).
  - `GetPatternsForStandardGameFiles` listed its excludes before `AudioCache/*`, so templates and the upgrade backup took `_obj/AudioCache/*` (first template had 124 assets). Fixed in 6.7d-11.
  - `SaveCrm` wrote every temporary room's `.crm` with the open room's images (single image cache); gone with it in 6.7d-11.
  - CI's `Windows Build Parity` job red since the 6.7d-9 to 6.7d-11 push (run 42381421451 on f4fd487a4): the harness un-embedded scripts from a package that already had them separate, and reported an identical `game28.dta` as different. Fixed in 6.7d-13.
  - `WorkspaceState.RequiresRebuild` not read since 6.7d-9, so an upgraded project no longer got a full build. Fixed in the 6.7d-12 follow-up: `BuildGame` ORs it in, reaching `agsbuild --rebuild`.
  - Found by the user (2026-11-08): deleting the `RequiredRebuildTime` property made every `Game.agf.user` from a 3.6.3.13+ editor fail to open the project (`DeserializePropertiesFromXML` throws on an unknown element; `WorkspaceState.FromXml` was outside the try/catch). Fixed: property back as `[Obsolete]`; `LoadUserDataFile` catches any read failure, warns, resets and deletes the file; `WorkspaceStateTests` added. Editor NUnit 550 of 550 (544 passed, 6 skipped).
  - `Tasks.HandleObjectIDErrors` repaired IDs after the project was saved. Fixed for the build in 6.7d-9, for the Run in 6.7d-10.
  - `IScriptCompiler.h` put `using namespace AGS::Types` inside `AGS::Native`; `NativeMethods.cpp` relied on it. Fixed in 6.7d-12 (`AGS::Types::FontMetrics`).
  - 6.7d-12's first step dropped `<SpeechVoxFiles>`, which is in `Game.agf`, not the user file; the template check caught the 77 bytes.
  - `wchar_t` is signed on Linux: one 6.7d-4 line failed the Linux clang-tidy gate (`bugprone-signed-char-misuse`), which the Windows tidy stage cannot see. Fixed.
- **Found, not fixed:**
  - `BuildTargetBase.DeleteCommonGameFiles` deletes `filename` instead of the loop's `fileName`, so old split parts are never removed in `Compiled/Windows` (in `Compiled/Data` `agsbuild` removes them).
  - `RoomsComponent.CopyGamePalette` copies references, so opening a room changes the game palette; the editor writes the last-loaded room's background colours into `game28.dta` and `Game.agf`. On an 8-bit-background copy of the auto-test, `game28.dta` differs in those 214 slots only. Left so.
  - `File::DeleteFile` on Windows tests `errno` after `DeleteFileW`, which does not set it, so most failures look like success.
  - `String::FindAnyChar(const String &)` searches for the whole string.
  - `Path::IsSameOrSubDir` takes a parent without a trailing slash for a file.
  - On POSIX the tools still inherit every descriptor.
  - `po2tra` accepts any text as a `.po` (`ReadPO` never errors; an unterminated string gives exit code 0).
  - `NativeRoom(Room)`, `SetBackground`, `SetAreaMask`, `convert_room_to_native` have no caller outside `NativeRoomTests`; `AGS.Native` no longer needs the `Compiler_md.lib` link and include path; the rest of `AGS.CScript.Compiler` outside `FastString` has no user outside the project.
- **Decisions:**
  - 2026-11-06, user: `ags4` has no ASCII mode; arguments, paths and game text are UTF-8; legacy encodings (`agscc --ascii`, `$Encoding=ASCII`) get no parity work.
  - `Tasks.RegenerateScriptHeader` and the `_GlobalVariables.ash` generator stay (autocomplete, go-to-definition, `IAGSEditor.GetAllScriptHeaders`), with a drift test against `agfexport`. `AGS.CScript.Compiler` stays for `FastString`.
  - Run install folder is `_Debug`, `_Debug/Rooms` searched first (supervisor, 6.7d-6): the Run needs no earlier full build and `$INSTALLDIR$` writes go to `_Debug`.
  - `agsbuild` deletes stale voice packs, unlike the editor (subagent, accepted by the supervisor).
  - 2026-11-07, user: the Run's cost is fixed by 7.1k (incremental room builds), done before the C# Run is deleted.
  - 2026-11-07, user: the script compiler setting is `[Obsolete]` (read, not written; the upgrade step only warns for a compiler other than "AGS SCOM 4"); `<SpeechVoxFiles>` is no longer read or written (the two templates then differ by 149 bytes of `Game.agf` only); nothing to restore for `GetScriptModuleList` and the removed types (none in `IAGSEditor`); `RequiredRebuildTime` dead.
  - 2026-11-08, user: the user file is ancillary and must never keep a project from opening.
  - Stays in the editor, open for 7.1: the pre-build checks (`RunPreCompilationChecks`, missing event handlers), the lip-sync compiler, `AudioCache` upkeep.
- **Lessons:**
  - `**/build` in `.gitignore` (for Xcode) hid `Editor/AGS.Editor/Build/` on Windows (`core.ignorecase`); two negations added.
  - Locally only `--tools-editor` had been run, so the plain-mode CI parity job broke unseen; run the mode CI runs.
  - `/compile` needs a desktop session and exits 0 when the project path does not exist.
  - Open: `prepush.py` could run the Linux tidy gate in its `linux` stage.
- **Not verified / left open:**
  - Not reproduced by `agsbuild`: the editor's incremental pack checks, 8.3-name wildcard matches, culture-specific lower case for `sp_` names.
  - `--build-debug`/`--build-release` reach only `agscc`, not `agf2dta` or `agfexport` (open, 7.1).
  - A cmd window not on a UTF-8 code page shows the tools' output as mojibake.
  - A character or item with no `<ScriptModule>` element at all: editor writes an empty name for cursor events, the tool `GlobalScript.asc` (the engine takes empty as global). Left so.
  - The installer itself was not built (needs a full run). The output panel click was not tried.
  - Checked by the user on 2026-11-07: breakpoints, step, call stack, watches, Ctrl+F5, renamed game file, Ctrl+R, closing a room, room templates, "Compile translation". Not checked: a game with engine plugins, a plugin that adds template files or used the removed events or types.
  - A Debug/Release switch now relies on the cache's inputs instead of the time stamp (not verified).
  - Linux and macOS (the harness needs the editor).

### 7.1k Incremental room builds in `agsbuild`

- **Definition** (redefined 2026-11-07 after 6.7d-10's Run cost): a room whose inputs did not change keeps its `room<n>.crm` in `_obj/`, compared by content hashes, never time stamps. *Done when:* a Run with nothing changed on the private AGS 4 game is near the reference editor's 1.9 s, a one-room change rebuilds that room only, a test per input kind, incremental output byte-identical to a clean build everywhere, a halfway failure leaves nothing stale.
- **Result** (2026-11-07):
  - Per room: `_obj/room<n>.crm` and `_obj/room<n>/manifest.txt`. Task `check-rooms` deletes a stale room's `.crm` and manifest before building, so a failed build records nothing; the room's tasks get a skip condition (`Task::SetSkipCondition`).
  - Manifest (`Tools/agsbuild/room_cache.*`): lines `<kind> <hash> <name>` (`tool`, `game`, `command`, `header` in order, `source` for every top-level file in `Rooms/<n>/`), 64-bit FNV-1a over contents.
  - `Game.agf` part: struct `RoomBuildGame`, keyed by `MakeRoomBuildGameKey` with structured bindings (an added member is a compile error until keyed); `crmbuild --game-key` writes it, since agsbuild's MSBuild project does not compile `room_build.cpp`.
  - `--rebuild` deletes all of `_obj/` (editor: `BuildToolOptions.Rebuild` on Rebuild all and a marked full rebuild); the rest of `_obj/` is still cleared each build (`ClearTempDirKeepingRooms`). `compare_builds.py --incremental`.
  - `tools_test` 225 → 247. Parity with `pre-6.7d`: 33 of 33 (three projects, as-is and ten variants); auto-test 2414 of 2414; halfway failure leaves the other rooms kept and the next build equals a clean one.
  - Private AGS 4 game, `agsbuild --data-only`, median of 5, two rounds: `--rebuild` 6.18 s, 3.90 s → 6.22 s, 3.97 s; nothing changed 2.12 s, 1.65 s; one room script changed 2.12 s, 1.61 s. Editor Ctrl+F5: 3.1, 2.7, 3.0 s against the reference's 1.7, 1.6 s (8.4, 8.8 s in 6.7d-10). The gap is the 38 script compiles (about 0.28 s each, 8 at a time), out of scope: led to 7.1p.
- **Found:** a kept room no longer repeats its compiler warnings (as the reference editor on a warm Run). `--threads 16` on 8 CPUs gave 1.2 to 1.4 s against 1.5 to 1.9 s (not changed).
- **Not verified:** Linux and macOS incremental runs; a case-sensitive file system with two room files differing only in case; the editor's Rebuild all passing `--rebuild` (read from code and NUnit, not run).

### 7.1p Incremental script compiles in `agsbuild`

- **Definition** (added 2026-11-07; the user's criterion is the old editor's speed): as 7.1k for scripts (source as given or generated, every header in order, agscc flags and binary); profile a nothing-changed build first. *Done when:* the Run with nothing changed on the private AGS 4 game is as fast as the reference editor's (1.6 to 1.7 s) or the profile says what stands in the way; the rest as 7.1k.
- **Result** (2026-11-07):
  - Profile before (nothing changed, 1.43 s): the 38 `agscc` compiles (about 0.15 s each, 0.15 to 0.97 s) were the critical path; the 212 skipped room tasks took 0.95 to 1.38 s because `TaskManager::RunAll` polled every 8 ms.
  - A kept script is `_obj/<script>.o` with `_obj/scripts/<script>.o.manifest`; `TaskCompileScript` checks it when it starts and records it after `agscc` succeeds. The manifest keeps `agscc`'s output, so a kept script's warnings are reported again. Format line `agsbuild manifest 2`. `room_cache` became `build_cache`.
  - Task manager wakes on a finished task (condition variable) instead of every 8 ms; the 8 ms limit stays for cancel.
  - `tools_test` 247 → 266. End to end on the auto-test copy (40 scripts): a script change recompiles that script alone, a header or tool change all 40, the game name alone none. Halfway failure: the other 39 recorded, the next build compiles the broken one alone and equals a clean build.
  - Parity with `pre-6.7d` (`--incremental`, now with a room script and a module changed): 33 of 33 with the script cache; with the task manager change 13 of 13 (three projects as-is and the auto-test's ten variants). Auto-test 2414 of 2414.
  - `agsbuild --data-only`, private AGS 4 game, interleaved medians: nothing changed 1.74 s → 0.88 s (scripts kept) → 0.34 s (and the task manager); `--rebuild` 5.41 s → 5.32 s → 5.28 s.
  - Editor Ctrl+F5: 2.1, 1.7, 1.9, 1.5 s against the reference's 1.7, 1.3, 1.7, 1.5 s. The rest of the gap is the editor's work before `agsbuild` starts (1.0 to 1.1 s: save, `PreCompileGame`, checks, script scan, plugin headers, busy dialog); `agsbuild` takes 0.34 to 0.38 s.
- **Lessons:** the machine slowed by a third during the session (3.8 s → 5.4 s for the same build), so tool sets are compared only in interleaved runs or within a round.
- **Not verified:** Linux and macOS incremental runs (the `linux` stage runs the unit tests only); a script module on a case-sensitive file system; the editor's Run with a module changed (only `agsbuild` timed).

### 4.1 + 4.5 + 3.6 per file

- **Definition:** PLAN.md 4.1, 4.5 and 3.6 applied to one file set per session, `Engine/ac/` most-churned first: a
  `[rename]` commit for parameter and local names (checked by `compare_objects.py`), then a `[comment]` commit (comment
  debris, stale comments, a header comment on every public function). The task stays open until the watchlist is at
  zero; each file set is logged here.
- **Method:** Sonnet 5.5 subagents at `medium`, reviewed by the supervisor; from 2026-11-11 four sets at once, one
  worktree each, brief `.compare/briefs/brief-41-fileset.md`. Every mechanical commit `compare_objects.py`-identical
  against its parent (637, later 638 objects); `prepush.py --auto` every selected stage per set (linux ctest
  1165/1165 at first, 1182/1182 at batch 5). Not run for any set: macOS, iOS, Android, the auto-test game.
- **Status (2026-11-12):** 1,826 watchlist names in all, 917 without the 5.1 globals (`thisroom`, `croom`, `objs`,
  `scsystem`, `usetup`). Next: the engine tail `gui/mylistbox.cpp` 7, `listbox.cpp` 6, `speech.cpp` 5, `invwindow.cpp`
  5, `plugin/agsplugin.cpp` 4, `gui/mytextbox.cpp`, `gui/mypushbutton.cpp` 4 each; outside it
  `Tools/data/data_file_writer.cpp` (`chinfo` 40), `Common/gfx/image_pcx.cpp` 7, `Common/util/lzw.cpp` 6,
  `Common/game/main_game_file.cpp` 5 and the compiler (`eres`/`tqs` 669, `char1` 13).

**Naming conventions set here** (STYLE.md "Naming"):
- One name per type: `ch` `CharacterInfo *` (also `CharacterInfo &`), `chex` `CharacterExtras`, `obj` `ScriptObject *`,
  `room_obj` `RoomObject`, `other` for a second character or object, `control` `GUIControl *` (`guio` → `control`, the
  name Engine/ and Common/gui already used), `btn` `GUIButton *` (PLAN.md 4.1's name), `surface`
  `ScriptDrawingSurface *` (`target`/`source` where a function has two), `item` `ScriptInvItem *`, `slider`
  `GUISlider *`, `gui` `GUIMain *`, `sgui` `ScriptGUI *`, `script_cam`/`script_view` for `ScriptCamera *`/
  `ScriptViewport *` (`cam`/`view` are the `Camera`/`Viewport` behind them), `dynsprite` `ScriptDynamicSprite *`,
  `file` (was `fil`), `script_over` (was `scover`/`sco`), `format` for a printf format (was `texx`).
- Indices are `<thing>_id`: `char_id`, `obj_id`, `btn_id` (the button's index in its GUI), `gui_id`, `item_id`,
  `ctrl_id`, `loc_id`.
- A playing `SoundClip` is `playback` (2026-11-09, the user's choice: `ch` and `clip` were misuses; `voice` is speech
  voice-over in ~150 engine identifiers, `channel` is a `ScriptAudioChannel *` 40 times and a slot index; `playback`
  agrees with `PlaybackState`, `IsPlaybackDone`).
- Words spelled out, since run-together names pass clang-tidy as `snake_case` (`newval` → `new_value`, `toret` →
  `result`, `bwid` → `blocking_width`); camelCase → `snake_case`; header declarations synced with their definitions.
- Reserved (`9512441d7`, the user: `ch` must be "an exact thing, not something that could be three different
  things"): `ch`, `chex`, `btn`; `tidy_diff.py` reports `[ags-reserved-name]` when one is declared on a changed line
  with another explicit type (`auto` not checked). `control` and `surface` stay unreserved (the tools and the graphics
  drivers use them for their own things).
- `CRYPTIC_WATCHLIST` gained `charid`, `objid`, `invi`, `tehobj`, `newval`, `newwid`, `newhit`.

**Lessons:**
- A rename puts old code on the changed lines under the tidy gate: braces (`ShortStatementLines: 0`; STYLE.md leaves
  them optional), C-style downcasts → `static_cast`, unused parameters. Each set has a `[format]` commit; they are in
  `.git-blame-ignore-revs`.
- A function-local static's name is part of its mangled symbol: renaming it is not object-identical (`offsetxWas`/
  `offsetyWas` in `game_run.cpp` stay).
- clang-rename was impractical for ~1,100 occurrences; a per-function name map applied by a scratch script, skipping
  string literals and member accesses, with `compare_objects.py` as the proof.
- `compare_objects.py`'s first run in an idle `.compare/` tree can report false differences (stale objects); re-run.
- Parallel worktrees: a fresh worktree lacks the googletest submodule (`msbuild` cannot find `gtest/gtest.h`) and the
  Editor's NuGet packages; agents sharing one scratchpad overwrote each other's scripts (give each its own folder); the
  `linux` stages share one WSL clone (agents skip it, the combined run covers it); four parallel builds make
  `compat_restore` tests time out (they pass alone).
- An agent reported `git reset` as denied but the reflog shows it ran `git reset HEAD~2`, unwinding a supervisor commit
  (recommitted as `5d9fd70b4`; nothing lost).

**`Engine/ac/character.cpp` + `character.h`** (2026-11-09; `b65708b52`, `0ba2ef3fc`, `6efc3bd1c`, `54f3e0173`):
- Watchlist to 0: `chaa` 642, `chi` 129, `chin` 89, `chap` 88, `char1` 84, `vii` 41, `texx` 27, `xx` 27, `yy` 23,
  `aschar` 19, `widd` 10, `iit` 10, `chinfo` 9. Comments: a TODO (a pathfinder route ends on a walkable pixel, a
  script-given path need not); the `walkwaitcounter` FIXME names its case (`int16_t`, negative after 32767 steps). The
  `Sc_*` signature comments were renamed, not audited (5.2).
- **Found:** `start_character_turning` decided the 180° tie-break of `kScTurnOrder_FaceDown` with
  `is_loop_in_left_half(fromidx) ? 1 : -1`: `fromidx` is an index into `turnlooporder`, not a loop, and `+1` leads a
  left-half loop up; DownLeft, UpLeft and UpRight starts turned through Up (checked for all eight starts). Fixed in
  `63da172bd` (`is_loop_in_left_half(turnlooporder[fromidx]) ? -1 : 1`, test on `start_character_turning`; checked
  visually by the user). Upstream `8d0654ddd`, same line on `ags4`.

**`Engine/ac/object.cpp` + `object.h`** (2026-11-09; `209dba696`, `b23b5506c`, `17c8226bc`):
- Watchlist to 0: `objj` 337, `xx`/`yy` 10 each, `xxx`/`yyy` 1 each. Comments: a "thing" of `AreThingsOverlapping`
  is a character ID or an object ID + 1000 (C3); `GetThingRect`: a disabled character counts as overlapping, a
  disabled object does not, neither checks visibility (a FIXME).
- **Found:** `GetObjectIDAtRoom` computed `obj_x`/`obj_y` and never used them (deleted in `ac3bc8730`);
  `is_pos_in_sprite` with a 0 width or height fell back to the bitmap size minus 1, excluding the last column and row
  from the hit test (a sprite whose recorded size is 0 while its image exists; `df791c714`).
- Not bugs: `Object.Baseline` returns the stored value, 0 meaning "use the object's Y" (intended, confirmed by the
  user); `AreThingsOverlapping`'s `+1` after the comparison (same minimum either way).

**`Engine/ac/guicontrol.cpp` + `guicontrol.h`** (2026-11-09; `4d69ebf58`, `612aee72e`, `b2b8ac185`, `351392e6e`):
- Watchlist to 0: `guio` 227, `xx`/`yy` 11 each (`guio` → `control`). Every function of the header documented.
- **Found:** `RegisterGUIControlAPI` registered `"GUIControl::SetScale"` twice on consecutive lines (the repeat
  deleted in `ac3bc8730`, table 71 → 70 entries); `GUIControl_SetOpacity` tested its range with a bitwise `|`
  (`||` in `c81711ba6`).

**`Engine/ac/drawingsurface.cpp` + `drawingsurface.h`** (2026-11-09; `24c36d4ea`, `bc90b54ee`, `fad87c64f`):
- Watchlist to 0: `sds` 162, `xx` 16, `yy` 15, `texx` 3. The pixel loops' code unchanged. The empty
  `if (src == ds) {}` became the reason drawing a surface onto itself is allowed (upstream `aeb65ccbf` dropped the
  error on purpose).
- **Found (upstream `ags4` too):** `ScriptDrawingSurface::SetDrawingColor` dereferenced `GetBitmapSurface()` without a
  check, so setting `DrawingColor` after `Release()` (or after the source sprite was deleted) crashed the engine; it
  warns and keeps the old colour (`1c83f49aa`). `DrawingSurface_SetPixelsImpl` warned as `"DrawingSurface.GetPixels"`
  (`599acd6fd`). No test added. The `linux` stage for these ran with the button set.
- Not fixed, minor: `Sc_DrawingSurface_DrawStringWrapped` names `DrawingSurface_DrawString` in its parameter-count
  assertion.

**`Engine/ac/button.cpp` + `button.h`** (2026-11-09; `b94b5235b`, `68e9a925c`, `92a1ad6f1`):
- Watchlist to 0: `butt`, `guin` and the unlisted `guil`. `UpdateAnimatingButton`'s "returns 1 if animation finished"
  was inverted (it returns `true` while the animation goes on), now correct.
- Noted, not changed: `AddButtonAnimation` (the save loader's path) does not drop a duplicate for the same button, as
  `Button_Animate` does; `StopButtonAnimation` has no range check (callers pass valid indices); `Button_GetText` copies
  at most 199 characters (`MAX_MAXSTRLEN`). `Button_GetGraphic` treating only a negative image as "none" is right.

**`Engine/ac/characterinfo_engine.cpp`** (2026-11-09; `ce2466d9a`, `b56d9cff1`, `a6a139eb7`, `e03971a33`, `6d9c3736d`):
- Watchlist to 0: `chi` 186; `followingAsSheep` → `following_as_sheep`. Both CHECKMEs answered (`UpdateCharacterMoving`
  is the only place that sets the standing frame after a walk; `ch->room++` is the follower's room-change countdown);
  a TODO: a script's `StopMoving` resets the frame but not `animwait`. Comments that said "visible" where the code tests
  "enabled" corrected.
- `e03971a33` `[delete]`s an unreachable branch of `UpdateCharacterFollower` (dead since before the 2024
  Enabled/Visible split); `6d9c3736d` turns the empty branches of `UpdateCharacterIdle` and `UpdateCharacterFollower`
  into early returns (code identical).
- Noted: in `FixupCharacterLoopAndFrame`, `int loop = ch->loop;` is overwritten at once and `loop = 0;` after a
  `quitprintf` is unreachable.

**Reserved names** (2026-11-09): `9512441d7` (rule), `5d9fd70b4`; misuses renamed in `92d3d5a2a` Engine (`SoundClip`,
~150 uses in 10 files; `btn` → `btn_id` in `process_interface_click`), `72858b28e` Common, `1d5a7130d` Compiler
(`cs_scanner.cpp` is not UTF-8: its 2 non-ASCII bytes kept), `28f894c0f` Tools; tidy follow-ups `fd0a4bbe2`,
`817678acd`, `f0f7e8dba`.
- **Found:** `Compiler/script/cs_scanner.h` `IsHexDigit` accepted every letter (`'Z'`/`'z'` where `'F'`/`'f'` is
  meant): `0x1'g'` swallowed the quote and failed on an unterminated char literal. Fixed in `b59fd4b94`, test
  `Scan.LiteralIntHexSeparatorNeedsHexDigits` (fails without the fix).
- Not a bug: the `audio.cpp` crossfade (`crossfading_in_channel` is reset to 0 two lines earlier when nothing plays).

**Naming review of the six file sets** (2026-11-09; `0b05b0300`, `fad65bcf6`, `1a4a283da` `[rename]`, `e2b767951`
`[format]`, `c32fc1041` the watchlist):
- Held: one name per type (`ch` 496, `obj` 197, `control` 120, `btn` 88, `surface` 65, `chex` 26). Renamed where it
  did not: index names to `<thing>_id`, `RoomObject` to `room_obj`, run-together names, `voice` → `playback` (145 lines
  in 10 files), the `byte` locals of `72858b28e` → `value`. Watchlist total 2,641 after the additions.
- **Open, tool:** prepush's `tidy` stage crashed with `WinError 206` (command line too long): it always compares with
  the upstream merge base, and the `--line-filter` for 3,758 unpushed changed lines exceeds Windows' 32k limit. It works
  again after a push; the fix is to pass each file only its own lines plus the changed headers', or to honour `--base`.
- **Left:** the three `AGSViewFrame *ch` typedef parameters in `agspalrender` and `ags_snowrain` (phase 7); `inum` in
  `Character_AddInventory` (`item_id` is taken there); script-API parameter names `sframe`, `ignwal`, `rept`, `sppd`,
  `spdd`, and `distaway`, `xwas`, `ywas`, `oldxp`, `bestshotwas`, `sppic`, `tdxp` and the like, which need a closer
  read; the `Sc_*` signature comments still show old names (5.2).

**`viewport_script.cpp`, `dynamicsprite.cpp/.h`, `file.cpp/.h`** (2026-11-11; the first parallel batch):
- `viewport_script` (`9e09c881a`, `83ae040bf`): `scam` 78, `scv` 60 to 0. `dynamicsprite` (`7036f0198`, `838984bb9`):
  `sds` 91 to 0; seven commented-out `HasFreeSlots` blocks deleted, `add_dynamic_sprite`'s "returns 0" corrected (it
  passes on -1, `NO_SPRITE_INDEX`). `file` + `Engine/main/update.cpp` (`c4403392d`, `4af9514ed`, `b54332ca9`,
  `a8c32ba14`): `fil` 86 to 0, `followingAsSheep` and two `CharacterInfo *chi` in `update.cpp`.
- **Found, one commit each:**
  - `cc7e218d4`: the three AGFReader version tests shared one file, `agfreader_test_version.agf`, and raced under
    `ctest -j` (`XML_ERROR_EMPTY_DOCUMENT`); the file is named after the running test.
  - `f04c500e3`: `File.ReadRawChar` returned a `char`: with a signed `char` a byte above 127 came back negative and 255
    as -1, the end-of-file value; with an unsigned one (ARM) the end came back as 255. It returns `Stream::ReadByte()`
    (0-255, -1 at the end). `862284d0c` `[delete]`s the unregistered `FileReadRawChar`. Upstream `ags4` has it.
  - `50510bb9f` a stray `;` in `File_ReadStringBack`; `6c237db78` `||` for a bitwise `|` in `FileRead`.
  - `9ce810048`: `DynamicSprite.Tint` on a deleted sprite (slot 0) tinted sprite 0 and passed it to
    `add_dynamic_sprite(0, ...)` (an assert in Debug, a silent no-op in Release); it quits now.
  - `2b1389a2e`: `Width`, `Height`, `ColorDepth` of a deleted sprite returned sprite 0's; they warn and return 0.
  - `964ffb429`: `DynamicSprite.GetDrawingSurface` on a deleted sprite made a surface on slot 0, so the script drew on
    the game's sprite 0 in memory; it quits now. Upstream `ags4` has it.
  - `1c25551d1`: `free_dynamic_sprite`'s `notify_all` chose between two calls identical since upstream `fcc42eb6a`; the
    parameter is gone.
  - Not added: tests for the `File` and `DynamicSprite` fixes (no test reaches the script file handles or
    `ScriptDynamicSprite` yet).
- Noted, not changed: `DynamicSprite_Rotate`'s condition mixes `&&` and `||` without parentheses (right by
  precedence); `File_ReadRawFloat`'s read stream is named `out`.

**`slider.cpp/.h`, `display.cpp/.h`, `gui.cpp/.h`, `draw.cpp/.h`** (2026-11-11; worktrees from `beca7d561`):
- `slider` (`eb5744d86`, `ecd8850d4`): `guisl` 62 to 0; its `Sc_*` signature comments deleted (they repeated the
  declarations). `display` (`4e0c2f807`, `ae433f1ef`): `xx` 29, `yy` 33, `charid` 3, `butid` 3 to 0. `gui` (`fc73c4f99`,
  `b68925a80`, `0b4c85249`): `guin` 20, `guio` 11, `newwid`/`newhit` 16, `iit` 6, `xx`/`yy` 11, `widd` 2 to 0. `draw`
  (`0a987d72b`, `85da7164a`, `d4aef55f2`): `objid` 20, `charid` 18, `chin` 18, `xx`/`yy` 6 to 0; `d4aef55f2` passes on
  `add_to_sprite_list`'s unused `id` (its one caller passes `0u`, the default: same behaviour).
- **Found, one commit each:** `f04f88996` an agent's wrong comment on `gui_on_mouse_move` (the loop breaks on a popup
  and while an overlay disables the interface); `284fddc89` that function's empty `if (...) ;` → early return;
  `e903548c7` `||` in `GUI_SetOpacity`; `9846f97d4` `"GUI::SetScale"` registered twice in a row; `a656390f3`
  `CalcLipsyncFrameDuration` `static`; `0235fa2ea` `[delete]` the write-only `ObjectCache::in_use`; `001940372`
  `MflUtils`' two library tests shared `mfl_test_a.txt`/`b.txt` (`StandaloneLibraryHasNoEnder` failed in a race);
  named after the running test.
- Noted, not changed: `global_display.cpp` reads `source_text_length` and writes it back (an "ugly hack" no-op;
  `ea271ea5d` below); `prepare_and_add_object_gfx` adds `thisroom.Height` to a baseline, assuming no baseline exceeds
  the room height (a TODO).

**`characterextras.cpp/.h`, `global_game.cpp/.h`, `walkablearea.cpp/.h`, `global_display.cpp/.h`** (2026-11-11;
worktrees from `67e417ebc`):
- `characterextras` (`c9da42c0a`, `a6ea00c77`, `d116a98b6`, `62db2030b` drops `WriteToSavegame`'s unused
  `CharacterInfo *`, save format the same): `chi`/`chin` → `ch`; stale `kRoomStatSvgVersion_*` tags →
  `kCharSvgVersion_400_26`/`_29`. `global_display` (`029b0bbe6`, `b95b229df`): `texx`, `xx`/`yy`, `widd`, `wii`,
  `aschar`, `charid` to 0. `walkablearea` (`2d7c8061d`, `508bc862d`, `80187ed97` `[delete]` an unused
  `CharacterInfo *`): `xx`/`yy` 29, `chin`, `char1` to 0. `global_game` (`6b0b5e484`, `cdd438b00`, `e9c3be85d`
  `[delete]` a duplicate `MoveSaveSlot` declaration, `9c7d8077e` `emplace_back`): `xx`/`yy` 34, `newval` 7 to 0.
- **Found, one commit each:**
  - `ea271ea5d`: `DisplayTopBar` timed a message by its title's length. The script wrapper's translation leaves the
    text's source length in `source_text_length` for `GetTextDisplayTime`; Ivan's hotfix `101ec0f7e` (2019) kept it
    across the title's `get_translation`, upstream `3ab558f98` (2024-02) moved that translation down and left the save
    and restore adjacent, doing nothing. Upstream `ags4` has it.
  - `0becab61f`: the same 2024 commit passed `FONT_NORMAL` for the bar's title where the font from
    `game.top_bar_font` was meant. Upstream `ags4` has it.
  - `d228d1baa`: characters and room objects from a save before `kCharSvgVersion_400_29` / `kRoomStatSvgVersion_40029`
    got the pivot `Point(0.5f, 0.5f)`, the int point (0,0), the top-left corner, not the centre. From upstream
    `5c5b9cd38` (2026-06); upstream `ags4` has both.
  - `c2c23c177` `&&`/`||` for bitwise `&`/`|` between comparisons (`get_area_scaling`, `RemoveWalkableArea`,
    `RestoreWalkableArea`, `SetMultitasking`); `8e69deccf` `prepare_walkable_areas`' empty `if (...) ;`; `52deb978f`
    `StartCutscene`'s error says 1 to 6, the range it checks; `7f178e148` `GetSaveSlotDescription` `static`.
  - Open items raised here, decided 2026-11-12 (see batch 5): old saves' `CharacterExtras`/`RoomObject` fields,
    `get_area_scaling`'s unread `x`, `get_walkable_area_at_location` reading mask row 1 above the room.
- Noted, not changed: `Display()` translates its format and `DisplayAtYImpl` translates the formatted text again;
  `WaitImpl` is non-static and undeclared; `game.h`'s `GetLocationTypeImpl` comment was not touched.

**`inventoryitem.cpp/.h`, `gui/cscidialog.cpp/.h`, `joystick.cpp/.h`, `overlay.cpp/.h`** (2026-11-12; worktrees from
`79e0d4a86`):
- `cscidialog` (`da073204e`, `3028c7818`, `4a975eeb0`): `xx`/`yy` 24 to 0; `__my_wbutt` (a reserved name) →
  `DrawWindowFrame`. `joystick` (`932fd6412`, `4925730a7`, `4aa50d396`): `butt` 15 to 0 (`button`). `inventoryitem`
  (`5d8dced46`, `59cbeb90a`, `83809e607`): `invi` 11, `iit` 9, `guio` 5 to 0. `overlay` (`a0d7c3856`, `f85f8d0c3`):
  `charid` 11, `texx` 2 to 0; the `get_overlays()` FIXME became a TODO naming the five files that write through the
  non-const reference.
- **Found, one commit each** (the first three in upstream `ags4` too):
  - `4b1ce87fb`: `Joystick.GetHat` passed the hat index through `Joystick_Hat_AGStoSDL`, a hat direction mapping:
    indices 5, 7, 10, 11 and 13 on became 0 and read hat 0 (since `ac83c3e86`, 2023). `89c2c1d6f` `[delete]`s the
    mapping.
  - `a3e21a6e3`: `InventoryItem.CursorHotspotAlignment` returned `hotx`, the X hotspot (since upstream `11e52329b`,
    2026-09).
  - `9d81c631c`: `Overlay.Tint` on a removed overlay called `SetTint` through null.
  - `6b718c3d1` `Overlay.Height`'s warning said "SetWidth", `AnimatedOverlay.Animating` reported errors as "Animate";
    `0fd70e037` `&&`/`||` for bitwise tests (`CSCIWaitMessage` called `finddefaultcontrol` for every key;
    `Overlay_SetOpacity`); `578f83fb2` `UpdateOverlayState`'s assert allowed the ID one past the end.
  - `d16516831` `[delete]` cscidialog's write-only `windowcount` and unused `curswas`; `516989d26` `[delete]`
    `set_inv_item_pic`'s second store of the same sprite; `3482377ff` four file-local helpers `static`.
- Noted, not changed: `checkcontrols` ignores the window level; `CSCISendControlMessage`/`CSCIDeleteControl` do not
  range-check `ctrl_id`; `add_joystick` skips every later device for good once one fails to open
  (`_joysticks.size() != device_index`); `Joystick_IsGamepadButtonDown`/`GetGamepadAxis` call SDL with a null
  controller for a plain joystick (SDL returns 0); `overlay.h` uses `std::unique_ptr` without `<memory>`;
  `Overlay_CreateGraphicCore`'s `SIZE_MAX` check is dead.

**`global_api.cpp`, `game/savegame_components.cpp/.h`, `main/game_run.cpp/.h`, `global_audio.cpp/.h`** (2026-11-12;
worktrees from `a241df4d1`):
- `game_run` (`56e42b338`, `08277469f` `[delete]` a duplicate `cur_mode` extern, `17dcedcff`): `objid`, `charid`,
  `guin` to 0. `global_api` (`4b33e4137`, `9c5784b48`, `3fb808e7d` `[delete]` the undefined `Sc_Said` extern and a
  doubled `MoveSaveSlot` row): `texx` → `format` in the `ScPl_*` functions; the remaining `texx`/`xx`/`yy` are in `Sc_*`
  signature comments (5.2). `global_audio` (`57e9b345c`, `b48210153`): `charid` 9 to 0, `sndid` → `cue_id`.
  `savegame_components` (`6e8cd9cf7`, `c729d942e`, `a2d3fe098`): `chi` 11 to 0; names in the
  `#if (DEBUG_SAVECOMPONENTS)` blocks (no compiler sees them) checked by reading; `ReadRTTI`'s `kType_Generated` in a
  field's flags → `kField_Generated` (same value); no old-save field gap here (`RestoredData` defaults them). Tidy
  follow-ups `09b8cd946`, `7eef32c49`.
- **Found:**
  - `edadb75aa`: the save prescan (`PrescanViews`) skipped 18 bytes per frame whatever the component version, and not
    the frame event names `WriteViews` writes after each loop's frames since 4000024: with a two-loop view it read a
    frame count from a name and failed, so `Game.ScanSaveSlots` left compatible saves out. Test
    `SavegameContent.PrescanStepsOverViewFrames` (failed before: frame count 1885697139, the bytes "step"). From
    `dcd2870cc`; upstream `ags4` has it.
  - `603365067` the Ctrl+D object list labelled opacity "trnsp"; `a8ee0fa4b` `||` in `SetSpeechVolume`; `[delete]`s
    `a1e9f1ef7` `GameLoopUntilState::GetDisabledFor`, `fb692f132` an unused `scrAudioChannel` extern, `84fd23f6e`
    `ReadAudio`'s `Speed = 1000` overwritten on the next line.
  - Voice lip-sync works only with the default `speech.vox`: `LoadLipsyncData` reads `syncdata.dat` once at startup,
    and `play_voice_speech` compares the clip's asset name, which carries a voice pack's path (`<name>/EGO.1` for
    `sp_<name>.vox`), with the bare names the editor writes; after `Game.ChangeSpeechVox` no line matches and the
    pack's own sync data is never read (older than `71aba62e4`; upstream too). Not fixed (PLAN.md open items).
  - A pre-3.6.1 script's `DisplayAtY(y, text)` goes through `ScriptSprintf` with the text as the format (a `%` is read
    as a format spec; not tested).
- Noted, not changed: `ReadScriptModules`/`PrescanScriptModules` repeat ~40 lines; `kSaveRestore_AllowMismatchExtra`
  is never set (with it the readers would index past the game arrays); `check_gamepad_controls` sends no gamepad event
  to scripts; `stop_voice_nonblocking` ignores a voice played with `as_speech` false.
- **Decisions (the user, 2026-11-12) on the open items of batches 3-5:**
  - `8a25d5732`: characters from saves before `kCharSvgVersion_400_28`/`_26` get the default `audio_speed` (1000) and
    view anchor/offset (as `SetUnlockedView`), room objects before `kRoomStatSvgVersion_40028` the default
    `audio_speed` (0, normal speed in `SoundClipWaveBase`), instead of keeping the running game's
    (`CharacterExtras::ReadFromSavegame` read into the live `charextra[i]`). No test (the component tests write the
    current version only).
  - `fb0b14838`: `get_area_scaling` takes the row only: its `x` was converted and never read since the source was
    opened (`ac.cpp`, 2012).
  - `6e8d72c93`: a point above the room reads mask row 0, not 1 (row 1 was the old engine's 1..height design).
  - The pre-3.6.1 `DisplayAtY` formatting does not matter; the lip-sync limitation is checked (PLAN.md open items).
  - One upstream PR with all the bugs found in this stage, once 4.1 is finished.

### 7.3 Ports

- **Result (2026-11-09; a slice of 7.3, the other ports open):** `Script/check_build_lists.py` reads
  `OSX/xcode/AGSKit/AGSKit.xcodeproj/project.pbxproj` (PBXBuildFile → PBXFileReference → PBXGroup paths, by regular
  expressions) and compares its compiled sources with the CMake targets `common`, `engine`, the libraries and the
  built-in plugins, in the Windows tree. 18 deliberate differences with reasons (other platforms' drivers,
  `alplmac.mm`, Allegro's `win/` and `unix/ufile.c`, FreeType's `builds/windows` and `builds/unix/ftsystem.c`, the MPEG
  audio files `DISABLE_MPEG_AUDIO` empties, the four git-ignored libraries `libsrc/download.sh` unpacks: ogg, vorbis,
  theora, SDL_sound). `OSX/xcode/ags/ags.xcodeproj` (`plugin_registration.cpp`) is in `NOT_IN_CMAKE`; every compiled
  file must be tracked by git, by exact case.
- The project was far out of step: 82 sources missing (all of `Common/data/`, 4.0's script-API files, FreeType 2.x and
  SDL_ttf, `stb_image.c`) and 71 compiled files that no longer exist (`Common/core/asset*.cpp`, the old `global_*.cpp`,
  `cc_instance.cpp`, `freetype-2.1.3`, `alfont`): 118 references removed, 82 added, 480 compiled sources (469 before).
  Three case mismatches fixed (`AAROT.c` is `aarot.c`, the `AGSflashlight` group is `agsflashlight`), five empty groups
  dropped.
- **Decisions:** iOS is ignored until the refactor is done (no `project.pbxproj` on `ags4`, a DIY port with a forum
  guide); the plan's compile-only iOS job is dropped from 7.3.
- **Not verified / left open:** an Xcode build (no Mac). Build settings untouched and probably need a pass on a Mac:
  `HEADER_SEARCH_PATHS` still names `freetype-2.1.3`, `alfont-2.0.9` and a misspelled `Common/lbsrc/aastr-0.1.1`, not
  `libsrc/freetype/include`, `libsrc/sdl_ttf` and `libsrc/stb`, nor FreeType's `FT2_BUILD_LIBRARY`-style definitions.
  The check compares with a Windows CMake tree, so macOS-only CMake sources (what `if(MACOS)` adds besides
  `alplmac.mm`) are not seen.

### Sprite placement and the Direct3D 9 half-pixel offset (outside the plan)

- **Definition (the user, 2026-11-12):** find why a maintainer's viewport test is off by a pixel at some zooms and
  window sizes, and why animation frames of different sizes jump at the game resolution; bring both fixes into the
  fork for long-term testing.
- **Result (2026-11-12, done; two engine commits, one in `ags-test-games`):**
  - `4e94f0c5f` Direct3D 9: its half-pixel offset came from the game-to-window ratio and was added before the batch
    transform, so it was wrong on the native surface and render textures, depended on the window size and grew with
    the camera's zoom (29 of 320 columns off at 200% with a window of the game's size). Now set per render target with
    the viewport and applied after the batch transform; D3D9 picks the same texels as D3D11. Ported from the branch
    prepared for upstream, `ags4-d3d9-halfpixel` (`docs/UPSTREAM.md`).
  - `c232e1aab` sprite placement: the hardware renderers rounded the origin on each frame's truncated scaled size and
    mirrored even-width flips around half a pixel, so frames of different sizes drifted (up to 4 game pixels under a
    x3 camera). `Common::GetSpriteQuadEdge` puts the origin texel on the position, draws at the exact zoom
    (`IDriverDependantBitmap::SetScaleRatio`, characters and room objects) and mirrors flips around that texel, moving
    off texel-edge ties by half the edges' grid; `GraphicSpace` takes the view frame's flip, so the software renderer
    and hit tests agree for unscaled flips. 5 unit tests (`SpritePlacement.*`).
  - Measured with a probe in the auto-test (frames 31x50 to 48x77, zoom 50-150%, camera x1-x3, flip; 40 cases per
    driver), cases that move / worst game pixels: D3D9 37/4 -> 6/1, D3D11 37/4 -> 6/1, OpenGL 36/4 -> 3/1.
  - `ags-test-games` `d8d301b` (branch `ags4-draconian`, not pushed): the h-flipped character frames (even width) are
    expected one pixel right; push it with the engine commits, or CI's OpenGL and Software auto-test runs fail 4
    tests. Auto-test 2414 of 2414 on D3D9, D3D11, OpenGL and Software; `ctest` 1187.
- **Not done:** the software renderer still scales with Allegro's stretch (scaled frames still drift there);
  `GraphicSpace` uses the truncated scaled size for hit tests (up to a pixel off the drawn sprite); frame offsets are
  scaled with truncating division (`draw.cpp`); the camera's own nearest-texel ties at x1.5 and x2 (1 pixel). Not run:
  the `linux`, `emscripten` and other prepush stages.

### Open items from finished tasks

*Moved from PLAN.md "Next up" on 2026-11-07* (details in the tasks' sections):

- 0.4: the CodeQL setting, the `D3DCOMPILER_47`/Windows 7 question and the macOS 10.9 deployment target.
- 1.4: the unfixed `agfreader` crash on a project without `<Settings>`; `StrUtil::Unescape` reading past the end.
- 1.2: the uninitialised `DataUtil::GameSettings`: 47 `bool`/`int` fields have no initializer, so a `Game.agf` element
  that is absent (an older Editor's project) leaves garbage in `agsbuild` and `ReadCustomDataDirectories` instead of
  the default from `AGS.Types/Settings.cs` (several are `true`, so `{}` is not the fix); only `PixelPerfect` got its
  default, after CI failed on it. The open upstream PR #3187 (branch `ags4-tools-2-agfreader`, which took #3147's tests)
  still has the wrong `EXPECT_FALSE(s.PixelPerfect)` in `AGFReaderBlocks.GameSettingsDefaults` (checked 2026-11-12).
- 7.1f's "left open" notes: one test project measures parity; 8-bit games and old indexed interactions unexercised.
- 3.1: the unchecked `GetDiskFreeSpaceExW` result in `AGSWin32::GetDiskFreeSpaceMB`.
- 1.5a: the 4-bit pixel write in `Common/gfx/bitmapdata.cpp` (wrong mask and shift, untested).
- 3.4-1's leftovers that are constant now: the write-only `GUI::DataVersion`, the always-on runtime behaviour
  switches, three dialog option offsets that are always 0, `FindNearestWalkablePoint`'s old method used by tests only.
- The scripts in a git worktree (2026-11-07): `refactor_metrics.py` refuses to run (`.git` is a file there),
  `prepush.py`'s `linux` stage cannot fetch from one and its `refs/prepush/worktree` ref is shared with the main
  checkout, and the `msbuild` stage needs the git-ignored NuGet packages, irrKlang DLLs and the googletest submodule a
  fresh worktree lacks.
- `Tools/test/directory_test.cpp`'s comment on the MSBuild googletest being older is out of date;
  `FetchGoogleTest.cmake` still pins 1.8.1 for GCC older than 5, which no builder uses.
