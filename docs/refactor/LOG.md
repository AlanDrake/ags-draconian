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

### 2.10a Compile time as a metric

- **Definition:** added 2026-11-15 (PLAN.md 2.10, RUNTIME-SPLIT.md 9). `Script/refactor_metrics.py --ninja-log
  <path>` reports the compile time of the first-party objects of a ninja log: the total, the median per object,
  and the same per top-level directory, `Engine/game` and `Engine/game/script_api`; `--json` carries it too.
  2.10 stays open for 2.10b (a precompiled header, unity builds).
- **How it reads:** only objects (`.obj`, `.o`) of the CMake `compile_commands.json` beside the log count; a
  step's time is its end minus its start, and the last entry of an output wins. The source of an object comes
  from that database (its `output` and `file` fields, relative to the source directory in `CMakeCache.txt`);
  sources under `libsrc`, `_deps` and the like, and anything under the build directory, are left out. Without
  the database the object's path decides (checked equal on a copy of a log: same 558 objects, same totals).
- **Result (2026-11-15):** numbers, configuration and compilers in BASELINE.md "compile time". 558 objects:
  MSVC 1731 and 1676 s (median 2.70 and 2.59 s), GCC 1604 and 1585 s (2.37 and 2.36 s). `Engine/game`: MSVC 444 and
  434 s, GCC 465 and 460 s; `script_api`: MSVC 125 and 122 s, GCC 108 and 109 s. Builds took 3.3 to 3.5 minutes
  each. The first reading of PLAN.md (446 s and 122 s, MinSizeRel MSVC) is confirmed. A fit over the files of
  `Engine/game` gives 2.4 s fixed plus 1.06 ms per line with MSVC (the first reading said 1.3 ms) and 2.3 s plus
  2.0 ms with GCC.
- **Found on the way:** the resource compile of `Engine/resource/version.rc` is a compile edge in the database
  with a `.res` output; the script counts objects only. A build directory inside the source tree (as in
  `.compare/`) makes its fetched sources look first-party unless the build directory is excluded.
- **Not verified:** the numbers come from one machine with a parallel build, so a step's time includes load from
  its neighbours; one MSVC and one GCC version; no other configuration (Release with LTO moves the compile work into
  the link and is not measured by this).

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

### 6.9 Room editor layer tree

- **Definition** (the user, 2026-11-13): the room editor's breadcrumb bar (a drop-down per level, with eye and lock
  toggles) becomes a tree on the left of the room pane, like the sprite manager's; the many empty hotspot and area
  slots must not fill it. The last planned WinForms UI change, done before 6.8 so the Avalonia front end ports one
  settled design. *Done when:* model tests, `prepush` `msbuild`, and the user's check in the running editor.
- **Result** (2026-11-13, branch `room-layer-tree`, 12 commits `46de67761`..`ec0365375` and the fixes below;
  merged into `ags4draconianr21` the same day):
  - Model without UI: `RoomLayerTree` / `RoomLayerNode` over `IRoomLayer` (the part of `IRoomEditorFilter` the tree
    uses). Nodes keyed by layer `Name` + item ID (the bar keyed them by display name and guessed renames). One layer
    with a mask visible at a time; a hidden layer selected is shown while it stays selected, then hidden again with
    the mask layers it hid shown again, unless the user toggled them (the bar left them on: the user's first look).
    The `.user` file keeps the user's visibility and the same `BreadcrumbPath` selection path.
  - Empty slots: `RoomAreaUsage`. Unused = no pixel in the 8-bit mask and properties equal to a new area's (compared
    through `ToXml`, so a new property is compared too); listed in a collapsed "Unused (n)" group at the end of the
    layer; properties without pixels are listed in italics ("nothing drawn"); area 0 always listed. Older editors named
    every hotspot slot `hHotspot<n>` / `Hotspot <n>` (`HOTSPOT<n>` in an older project): on a private AGS 4 game 2560
    of 2597 hotspots, on a private 8-bit game 475 of 490, so these names count as defaults. Over the two games and the
    auto-test game the "nothing drawn" slots left are real (a baseline, scaling or an event on an undrawn area).
  - View: `RoomLayerTreeView`, a native `TreeView` with `OwnerDrawText`; eye and lock in two columns at the right
    edge (the user chose right over a Photoshop-like left column, which would need `OwnerDrawAll`); a click there is
    taken in `WndProc` before the native tree selects or expands. The room is a leaf beside its layers. With the tree
    focused its navigation keys stay in it (the pane takes keys through the main form's key preview), Enter gives the
    focus back to the room. Theme keys `room-editor/tree/*`, falling back to the bar's drop-down colours.
  - Area usage: a stroke or fill lists a first-used area at once; the count over the mask runs at most every 4 s
    (`TickOnceTimer`, waits during a stroke; the user's suggestion); undo, mask import, background import, mask
    resolution and a mask file changed on disk count at once. Costs (Release): a scan 0.2 ms at 320x200 to 5 ms at
    3840x2160; a rebuild's property check of the 98 areas about 20 ms.
  - Deleted: `AGS.Controls/AdressBarEx` (listed as third-party), `ToolStripDropDownMenuEx` and
    `ToolStripDropDownButtonEx` (added for the bar), `RoomEditNode`, `RoomNodeControl`, `VisibleLayerRadioGroup`.
  - Tests: 65 NUnit (model, area usage, the view through window messages); `prepush` `msbuild` passes.
- **Found:**
  - Creating a hotspot or region subscribes it to the global `InteractionSchema.Instance.Changed`; the default check
    would have kept every area it made (98 per tree rebuild), so it detaches them (test). Every loaded room's own
    hotspots and regions are never detached either: open, on `upstream/ags4` too.
  - `IRoomController.GetAreaMaskPixel` checks `x > mask.Width` and `y > mask.Height` (should be `>=`: one column past
    the mask reads the next row, on the last row past the array) and copies the whole mask for each pixel it reads:
    open, on `upstream/ags4` too.
  - The bar's "design modified" test compared boxed strings by reference, so a rebuild alone marked the room's design
    data modified.
  - `TVS_EX_DOUBLEBUFFER` with the owner-drawn tree showed striped garbage for a moment in the rows a collapse left
    empty (the user's video); removed in `ec0365375`, whose message wrongly says the sprite manager's tree does
    without it: the project tree and the sprite folders have it (`TreeViewWithDragDrop`,
    `TreeViewExtensions.EnableDoubleBuffering`). Without it the tree flickered; `9dda6781c` turned it back on through
    that helper (posting the width-change repaint instead of requesting it during painting) and the garbage came
    back on expanding, so `2ae49b08e` turned it off and repaints only what changes: the eye and lock columns for a
    visibility or lock change, the shown nodes updated in place when a rebuild moves none. The icons still
    flickered; the garbage came only with the expand and collapse animation (the user), which no `TVS_EX` style turns
    off, so `83fa72596` keeps the buffer and freezes drawing from `BeforeExpand` to `AfterExpand` (`BeginUpdate`, a
    posted thaw as a safety net); the user saw neither garbage nor flicker. `352d8cf10` then drops the in-place update
    (74 lines, not needed under the buffer: a rebuild is drawn once off screen). `d400d0637` drops the freeze too:
    the double-buffered native tree leaves `WM_ERASEBKGND` unpainted, so the tree fills its background itself there
    (swallowing the message kept the garbage, the user's tests); the user sees no garbage, and a flicker only while
    a node expands, which they accept. Fallback if the garbage shows again: the freeze (`83fa72596`), or
    drawing whole rows (`OwnerDrawAll`). The vertical scroll bar changes the client width without a resize, so the
    tree repaints when the width it drew for changes.
  - Moving the canvas into the bar's row put it under the 45-pixel transparency slider (now 26, no ticks).
- **Lesson:** a command chain that greps the build output for a summary hides a failed build; check the build's exit
  code (`ec0365375` was committed after such a failure and verified by a second run).
- **Not verified:** the editor's room pane at high DPI and with a dark theme file that defines the new
  `room-editor/tree/*` keys (none exists); flicker on a rebuild by the user's eye (an area property edit, an area
  going in or out of "Unused"; toggles, layer switches and expansion were checked on `d400d0637`'s code).

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
- **Status (2026-11-12): 4.1 done** (batch 7): 909 watchlist names, all of them the 5.1 globals (`thisroom`, `croom`,
  `objs`, `scsystem`, `usetup`). 4.5 and 3.6 stay open for the files that had a rename-only pass (the compiler, the
  stray names of batches 6 and 7) and the files no batch touched: 770 files on 2026-11-12, 9 pairs fewer after batch 8
  (2026-11-13).

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

**Batch 6, the engine's tail, Common and the data writer** (2026-11-12; four worktrees from `3ce6340df`; from this
batch the agents prove each commit with `compare_objects.py` only and the supervisor runs `prepush.py` once on the
combined branch, the user's rule):
- GUI controls (`bd41c4301`, `fd5fe1234`): `gui/mylistbox`, `mylabel`, `mypushbutton`, `mytextbox`, `listbox` (`guisl`
  → `listbox`), `invwindow` (`chaa` → `ch`), `guio` → `control` in `game_init` and `cc_guicontrol`. The `my*`
  constructors take `left`/`top`/`width`/`height` (`x`/`y` would shadow `NewControl`'s members).
- Speech, strings, plugin API (`d287298f7`, `81ceef5d6`): `speech`, `string` (camelCase parameters), `agsplugin.cpp`
  (definitions only; `agsplugin.h`, C4, untouched), stray names in `hotspot`, `system`, `cc_character`, `cc_serializer`,
  `route_finder_impl` (the `DEBUG_PATHFINDER` block's undeclared `xx`/`yy` → the call's `dstx`/`dsty`; no compiler sees it).
- Codecs, fonts, video (`61e318149`, `8eff99e08`, `f6937d690`, `ca2b78320`, `e854aa59d`, `29f899b4c`): `image_pcx`
  (`xx` → `pixel_x`, beside the byte column `x`), `lzw`, `fonts`, `script.cpp`, `scriptcamera` (`scam` → `script_cam`),
  `abuf` → `audio_buf` on `VideoPlayer::NextAudioFrame` and its override.
- Data writer (`c8e0f5cc5`, titled `Common` though it renames `Tools/data/data_file_writer.cpp` too; `25bb5eb98`):
  `chinfo`/`chin` → `ch`; the writer's comments name the readers that exist (`ScriptEventHandlers::Read_v361`/`Read`,
  `GUIControl::ReadFromFile*`), not the deleted `DataFileWriter.cs`, `interactions.cpp`, `guiobject.cpp`.
- Tidy follow-ups: `fb4b004d2` (`textheight_` → `text_height`; the plugin hook fields' parameter names back to the
  base, since the camelCase fields beside them mirror `InbuiltPluginDetails` and are 4.3's), `8816ae27e` (named casts,
  `lzw.cpp`'s `min` macro → `std::min`, `emplace_back`; measured: `insert()` identical,
  `AddManagedObjectReader` changed). `9b26aaa95`: `check_helpers.py` took `String_Trim` and five other script API
  functions whose parameters were renamed for new helpers; a function counts as new now only if its file lacked it
  at the base.
- **Found, not fixed (PLAN.md open items):**
  - `Speech.SkipStyle` does not read back what was set: `user_to_internal_skip_speech` adds `SKIP_GAMEPAD` to the key
    styles and `SKIP_TOUCH` to the mouse ones, `internal_skip_speech_to_user` compares the flags with `==` without
    them, so every style with key or mouse reads back as `kSkipSpeechTime` or `kSkipSpeechNone` (`speech.cpp:61-90`;
    since the gamepad and touch flags, `c5c1c83a7`, `3ee7cf33b`; upstream too). Read, not run.
  - `MyPushButton::pressedon` tracks the press position it was given, not the mouse, so dragging off a button of the
    built-in dialogs never releases it (`mypushbutton.cpp:78`; the parameters shadowed the globals before `aa2a418b4`).
  - `WriteGlobalMessagesBlock` writes `Game.agf`'s `<GlobalMessages>` after the lip-sync block, which the reader skips
    only before `kGameVersion_400_33`. Unreachable: neither `AGS.Types` nor the `pre-6.7d` C# writer has global
    messages, and `agfreader` refuses pre-4.0 projects; a `[delete]` of the read and the write.
  - `lzwcompress` stores a `short` through a cast into a byte buffer (unaligned, host byte order: a big-endian host
    writes other bytes; `lzw.cpp:188`). `ListBox_AddItem` tests `AddItem(...) < 0`, which never happens.

**Batch 7, the compiler, the editor's native layer, `agspalrender`** (2026-11-12; two worktrees from `9b26aaa95`):
- `d8941fa41`: the parser's `EvaluationResult`s `eres` → `eval_result` (`lhs_result`, `rhs_result`, `term1_result`...,
  `unused_result` for `eres_dummy`), `TypeQualifierSet`s `tqs` → `qualifiers` (`vardecl_qualifiers`...), 683 names in
  `cs_parser.cpp/.h` and `cc_symboltable.h`; `5237f2a4e`: the scanner's `char0`..`char3` → `byte0`..`byte3` (the bytes
  of one UTF-8 sequence). Upstream changed `cs_parser.cpp` 8 times in the past year: a cherry-pick there may need the
  names mapped by hand.
- The parser commit was not object-identical at first: MSVC names a lambda after a hash of its body, so three
  functions with `[&]` lambdas came out as 120 functions removed, 120 added. `f05423b49`: `compare_objects.py` compares
  with `<lambda_hash>` read as `<lambda>`; the commit is identical then (636 + 2 identical in code), and the
  `emplace_back` commit still shows its change.
- `21dcce05b`: `agsnative.cpp` (`xx`/`yy` → `x`/`y`, `newwid`/`newhit`, `charid`); `f80252e5d`: `agspalrender`'s
  `xx`/`xxx` → `x_squared`/`x_cubed`. Not covered by `compare_objects.py`: proved by `dumpbin /disasm` of the MSVC x64
  Release objects before and after (identical; `agspalrender` built without `/GL` for the comparison).
- `b5a19fa06`: `refactor_metrics.py` strips raw string literals: the shaders' `.xxx` swizzles counted as 4 names and
  their `main()`s as 8 functions. **The watchlist is at 0 outside the 5.1 globals (909): 4.1 done.**
- `43a7d79a6`: the rename put five `qualifiers[(error_tq = TQ::k...)] || ...` chains under tidy
  (`bugprone-assignment-in-if-condition`); `TypeQualifierSet::HasAnyOf(list, found)` replaces them (three parser
  functions change, compile time only).
- Verification of the combined branch (`3ce6340df..43a7d79a6`, base `3ce6340df`): `prepush.py --auto` every stage
  (format, helpers, build + ctest, tidy, headers, libcxx 341 headers, linux ctest 1187/1187, msbuild 5 solutions +
  Editor NUnit); emscripten linked at `f05423b49` (the two commits after it touch no engine code). Runs took 15-18
  minutes for all stages. Not run: macOS, iOS, Android, the auto-test game.
- Left for later: the `Sc_*` signature comments keep the old names (5.2 moves them; the metric skips comments); the
  compiler's comment pass (4.5, 3.6) and the camelCase parameters of files that had no full pass.

**Batch 8, 4.5 + 3.6 on the most-churned engine files** (2026-11-12/13; PLAN.md "Next up" 3; four worktrees from
`3a88a5fb6`, brief `.compare/briefs/brief-45-fileset.md`: 4.1 is done, so a `[rename]` commit now takes the camelCase
and run-together names and the comment pass is the larger part). 770 first-party files (~280k lines) never had a
comment pass; this batch takes the top of a churn ranking (commits since 2024), leaving out the compiler (its own
plan) and the D3D/OpenGL drivers (they need the GL and emscripten builds to verify).
- `game`, `gamestate` (`5db5878ca`, `7f0207fb9`): 497 renames (`slotNum`/`slotn`/`slnum` → `slot`, `view_index` →
  `viewport_id`, `charat`/`hsat`/`objat` → `char_id`/`hotspot_id`/`obj_id`, `kgn`/`mbut`/`gbut` → `key`/`mouse_button`/
  `gamepad_button`); `GamePlayState` documents which fields `Read`/`WriteForSavegame` leave out and which other
  components save.
- `room`, `dialog` (`59acb1933`, `128fc241e`, `ba1766fe8`, `e198805b5`): 703 renames (`dlg`/`dlgnum`/`dialogID` →
  `dialog_id`, `sd` → `script_dialog` beside the global `dialog`, `write_dialog_options`' parameters spelled out); the
  `DialogOptions` members stay (4.3), so some parameters now differ from the field they feed. The parser's F3 CHECKME
  answered (it appends the missing tail of the previous entry).
- `savegame`, `game_init`, `script` (`8036cddd0`..`b1d6fdf73`): 351 renames; the save file's format block, bitmap
  block, header and plugin data documented against the readers; `b1d6fdf73` deletes the unused `TempEip`, the
  undefined `post_script_cleanup` declaration and a `SaveGameState` declaration with no definition.
- `guimain`, `engine` (`7205218c9`..`0d9a1d745`): 331 renames; the `RebuildGUI` CHECKMEs answered (controls cannot
  update their graphic space while game data is read, sprite info may not be loaded yet).
- Every `[rename]`, `[comment]` and `[delete]` commit `compare_objects.py`-identical (640 objects; the delete commit 2
  in bytes, not in code). The agents ran one comparison per stage and split it into per-file commits afterwards.
- Tooling (`58b6da252`, `fb2836de7`): `check_helpers.py` read `Game.StopAudio` in a comment as a missing member of the
  Editor's `Game` class (9 findings); it now accepts what `agsdefns.sh` declares on a struct of that name or as its
  extender. `tidy_diff.py` leaves out a rewritten line whose code without its `//` comment and spacing is that of a
  line the hunk removes: a fixed comment on a legacy field put the field's name under the gate.
- `c04c4a073`: the gate's 14 findings on renamed lines (named casts, `make_unique`, `empty()`, unnamed unused
  parameters, `(-3053)`, `{false}`) and the first first-party `NOLINT`: `RunScriptFunction`'s `cached_error` is a copy
  on purpose (`cc_get_error()` returns the global the run changes; the copy is what is restored).
- Verification (`3a88a5fb6..c04c4a073`): `prepush.py --auto` format, helpers, build + ctest, tidy, headers, libcxx
  341, emscripten, linux ctest 1196/1196; msbuild (5 solutions + Editor NUnit) passed at `0d9a1d745`, not rerun after
  `c04c4a073` (orphaned MSBuild nodes of the interrupted run held its log). Not run: macOS, iOS, Android, the auto-test
  game, the `BUILD_STR` branch of `get_engine_version_and_build` (renamed by reading).
- **Found, not fixed** (all on upstream `master` and `ags4`, checked 2026-11-13; PLAN.md open items):
  - `GameState::DeleteRoomViewport`/`DeleteRoomCamera` renumber the remaining script objects through
    `_scViewportHandles[index]`/`_scCameraHandles[index]`, not `[i]` (`gamestate.cpp:326`, `:406`): with two or more
    after the deleted one, their script objects get wrong IDs.
  - `Game_SetAudioTypeVolume` indexes `change_str[change_type - VOL_CHANGEEXISTING]` before checking `change_type`
    (`game.cpp:301`): an invalid value from script reads past the array and hands it to `printf` as a string.
  - `DialogOptions::RunKey`'s text-input branch fills `runDialogOptionTextInputHandlerFunc`'s parameters and then runs
    `runDialogOptionKeyPressHandlerFunc` (`dialog.cpp:1524`): `dialog_options_text_input` is never called from there and
    the key handler runs twice.
  - `get_room_bg_surface` tests `bg_index >= 0 && bg_index && ...` (`room.cpp:1226`): background 0's attached surface
    (only a restored save attaches one) is never found.
  - `MakeSaveGameDir`'s "remapped to" warning prints the path after clearing it (cosmetic).
  - Smaller: `unload_old_room`'s `croom == nullptr` test after `croom` was used; `DialogOptions::End` sets the overlay
    id to 0 where "none" is -1; `inv_item_wid` has no initializer; `FreeProperties` leaves `guicontrolProps`;
    `GUIMain::SkipSavestate` without `ctrl_refs` leaves the stream misaligned (no caller does that); `InitGameState`
    `std::move`s from a `const` (copies); `moduleRepExecAddr` is written, never read.
  - Not a bug, though reported as one: `WriteBitmap`'s `Seek(end_pos, kSeekEnd)` (`savegame.cpp:234`) would leave a
    hole, but the save is a `BufferedStream`, whose `Seek` clamps to the end; `kSeekBegin` would say what is meant.

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
  - `ags-test-games` `f71a8e0` (branch `ags4-draconian`, tag `draconian-auto-test-2`, which CI checks out): the
    h-flipped character frames (even width) are expected one pixel right; with the older tag CI's OpenGL and Software
    auto-test runs fail 4 tests. Auto-test 2414 of 2414 on D3D9, D3D11, OpenGL and Software; `ctest` 1187.
- **Follow-up (2026-11-13, the user testing in a game with scaled characters):** with `c232e1aab`, odd-width frames
  stood a pixel off even-width ones in flipped loops. Tried: the flip of the original placement for every sprite
  (flipped loops right, unflipped off the other way), then the per-flip rule below for every sprite (both right).
  Kept, `e9a465e20`: `Common::SpriteOriginRule`. `kOriginRule_Classic`, the default, rounds the origin half up and
  mirrors a flip inside the same quad, as AGS always did; overlays, GUIs and the rest keep it, so `x - width / 2`
  centres exactly and a flipped overlay does not move (`c232e1aab` had moved those on the hardware renderers only).
  `kOriginRule_MirrorAround`, for characters and room objects (`IDriverDependantBitmap::SetOriginRule` in `draw.cpp`,
  and `GraphicSpace`, which takes the rule and a `GraphicFlip`), rounds half down and turns a flipped sprite around
  that same texel of its image. Visible change against every earlier version: an unflipped character or object of
  even width with a centred origin stands one pixel right. `ags-test-games` `b10d691`, tag `draconian-auto-test-3`
  (`aa6be98ad` points CI at it). Verified: prepush without `linux`, `ctest` 1196; the auto-test game was not run on the
  final version (2414 of 2414 on four drivers on the first try); the user checked the characters in the game.
- **Not done:** the software renderer still scales with Allegro's stretch (scaled frames still drift there);
  `GraphicSpace` uses the truncated scaled size for hit tests (up to a pixel off the drawn sprite); frame offsets are
  scaled with truncating division (`draw.cpp`); the camera's own nearest-texel ties at x1.5 and x2 (1 pixel). Not run:
  the `linux`, `emscripten` and other prepush stages.

### 5.2 step 1 and 5.3: bindings apart, `Engine/ac/` reorganised

- **Definition:** PLAN.md 5.2 step 1 (each file's `Sc_*` block and `Register*API` to its own file, `[move]`) and
  5.3 (`Engine/ac/` into folders by domain). The layout was redesigned first (the user, 2026-11-13): four rings
  instead of ten flat folders, the domain folders nested under `Engine/game/`.
- **Status:** done 2026-11-14, pushed as `dfdee4868..cd0e7010a` (221 commits, with what was waiting), the tag
  `pre-5.3` on `b0777d74d` (local). `Engine/ac/` and `Engine/gui/` no longer exist; `Common/ac/` holds
  `def_version.h` alone, which CI and the packaging scripts of every platform name (it stays: the user, 2026-11-14).
- **The rings** (`Script/check_layers.py` has the table): Common; base (`Engine/util`, `debug`); host (`platform`,
  `device`, `gfx`, `media`); the script VM (`Engine/script`, with the eight managed-object files it uses in
  `script/dynobj`); the game module (`Engine/game`: `world`, `navigation`, `dialog`, `ui`, `render`, `audio`,
  `video`, `input`, `save`, `lib`, `dynobj`, `script_api`, and `game`, `game_state`, `event`, `system`, `file`,
  `translation`, `script` at its root); composition (`main`, `plugin`). Upstream's draft names and rules where they
  exist ("Script VM: only Utils", renderers with "no game logic", `game` with subdirs). Two changes against the
  first sketch, both from the include graph: no `script/lib` in the VM (String, the containers and utils read game
  state: the script library is `game/lib`), and `script/dynobj` gets only what the VM includes (73 other `dynobj`
  files are game types).
- **Tools, all in `Script/`, all with `--repo`** so that the work can be replayed on an upstream tree:
  `split_bindings.py` (bindings found by name, moved byte for byte, includes derived from the identifiers the text
  names, the declarations the bindings need added to the headers first), `move_functions.py` (folds functions from
  one file into others from a map), `apply_moves.py` (whole-file moves from a map: `git mv`, every `#include`,
  three CMake files, ten `.vcxproj` + `.filters`, the Xcode project), `compare_objects.py` (two new verdicts:
  functions moved between objects, moved or renamed files), `check_layers.py`. The decisions are tables:
  `docs/refactor/moves-5.3.tsv` (278 files, 154 renamed: also the path table for ports) and
  `docs/refactor/fold-5.3.tsv` (137 rows).
- **5.2 step 1** (86 commits): 44 files split, 1,547 functions and 8,618 lines moved to
  `Engine/game/script_api/<type>_api.cpp`, about 885 declarations added first (a declare commit per file: the
  wrappers were the only callers of some 650 API functions, which no header declared; 10 `static` functions became
  external; five files got a header). `compare_objects.py` over the range: 1,573 functions moved, 1 changed
  (`RegisterCharacterAPI`: same source, the same symbols referenced, MSVC builds its table with different inlining
  in the new translation unit). `ScriptExports.*` covers the tables, which the object comparison does not.
  Outside `script_api/`, `script/script_api.h` is still included by `script/runtimescriptvalue.h`, so nearly every
  engine file gets it transitively: cutting that include is a change of its own.
- **5.3, the fold** (13 commits): the thirteen `global_*.cpp` pairs (the flat API by topic) folded into their
  domain files, one new pair (`keyboard`). Range: 215 functions moved, 3 changed, each with unchanged source:
  `SkipUntilCharacterStops` and `StartCutscene` lose a call to the empty `initialize_skippable_cutscene()`, now in
  their file; `remove_popup_interface` calls `IsInterfaceEnabled()` once instead of twice, its one-line body now
  visible. Every fold commit builds and passes `ctest`.
- **5.3, the moves** (20 commits, one per destination folder, each built and tested before it was committed):
  278 files, 1,999 include lines in 397 files. Range: `move: 144 objects moved, code identical (mechanical)`.
- **Verification of the final tree:** `prepush.py`, every stage (MSVC, GCC 13 in WSL with `ctest` 1197 of 1197,
  Emscripten, libc++ headers, MSBuild with the Editor's NUnit tests); the auto-test game 2414 of 2414 on Software
  and D3D9, built by `agsbuild` from this tree. Not built: macOS, iOS, Android (five rewritten includes are in
  their platform files; the Xcode list is checked by `check_build_lists.py` only).
- **Performance** (STYLE.md "Performance"): a wrapper and the function it calls are now in two translation units.
  With LTO nothing changes: MSVC Release (CMake and MSBuild), all 1,620 wrappers identical instruction for
  instruction before and after; GCC 13 `-O3 -flto`, cheap wrappers identical, benchmark within 0.05 ns. Without
  LTO, GCC 13 `-O2`: 455 cheap wrappers call instead of inlining, `Sc_Math_GetPi` 1.41 → 2.16 ns a call,
  `Sc_Math_DegreesToRadians` 1.59 → 2.62 ns (called through a function pointer in a loop, not through the VM). That
  was the build of `debian/rules`, which also makes the Linux bundle: it now passes
  `CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON` (`bcd220a96`; checked in WSL: no call again; **not** tried on the Debian
  10 image with GCC 8, by the user's choice: the next full CI run shows it). Emscripten has no LTO and was not
  measured.
- **Layering debt, measured:** 129 includes break the ring rules and are the baseline of `check_layers.py`
  (prepush stage `layers`, `verify.yml`), which may only shrink: game → composition 70 (`main/game_run.h` 22: the
  blocking script calls that run the game loop; `main/game_setup.h` 18: `usetup`; `plugin/plugin_engine.h` 12:
  plugin hooks), host → composition 16, host → game 15 (`runtime_defines.h`, `asset_helper.h`: both mix host and
  game content), base → above 21 (`debug/debug.cpp`, `memory_inspect.cpp`, `util/timer.cpp`), VM → game 6, VM →
  host 1. `game_setup.h` was tried in `util/` and put back: it includes `main/graphics_mode.h` and game headers,
  so it is composition data that the game reads, which 5.1's contexts are for.
- **Gates changed on the way:** `tidy_diff.py` and `check_helpers.py` do not count a moved or copied line as new
  (an added line whose code is in the base copy of a file the diff touches; 1,216 diagnostics → the 5 real ones on
  the split), and `tidy_diff.py` passes its line filter in a response file (a 114-file diff exceeded the Windows
  command line). Known limits: a new line equal to an existing line of a touched file is not checked, nor a new
  function with the name of one that existed in a touched file.
- **Found on the way, not fixed:** `Walkbehind_SetBaseline` does not set `walk_behind_baselines_changed`, its
  older twin `SetWalkBehindBase` does; six script API methods only forward to a flat function (`Game_Pause`,
  `Game_Resume`, `Game_GetSpeed`, `Game_SetSpeed`, `Character_SayAt`, `Walkarea_SetScaling`); camelCase parameters
  left in `AudioChannel_SetRoomLocation` and `dialog_options_rendering` (4.1 missed them);
  `Solutions/Engine.App/Engine.App.vcxproj` lists `Engine\main\def_version.h`, which does not exist; the comments
  above the `extern` declarations in `game.h` name the API file that first needed them.
- **Lessons:** a tool proved on three cases is not proved (the fold broke the build on the ten that were only
  dry-run: names reached through macros, `using` lines left without their header; since then every `[move]` commit
  is built and tested before it is committed); an MSBuild tree from before the moves fails to link
  `Engine.Full.Test` (it links every object in Engine.App's folder, the old names included): delete
  `.obj/x64/Release/Engine.App`; `prepush.py`'s `linux` and `msbuild` stages do not run from a linked worktree, so
  the supervisor's prepush runs in the main tree after the fast-forward.

### 5.2 step 2: the template binder

*Definition:* replace the `API_*` macros and the hand-written `Sc_*` wrappers with a C++17 binder that generates
each wrapper from the signature of the function it calls, without changing what any wrapper does or the code the
script VM runs.

**Status (2026-11-14): done**, 54 commits.

- **What a binding is now:** one table row, `ScMethod<&Character_AddInventory>("Character::AddInventory^2")` or
  `ScStatic<...>`; `ScMethodFormat` and `ScStaticFormat` for the 18 functions that take a format and its arguments.
  `Engine/script/script_binder.h` (VM ring) reads each parameter and returns the result as the C++ type says;
  `Engine/game/script_api/bindings.h` (game ring) names the manager of each returned type (`ScriptManagerOf<T>`)
  and holds the format rows, which translate the format. A returned type without a manager must be an
  `IScriptObject`, else the row does not compile; a `void *` result must be tagged.
- **Before the design, measured:** 1,603 wrappers; 1,574 a single plain macro, 16 sprintf-style, 13 hand-written.
  In 1,552 of the 1,574 the macro said what the signature says. A probe of six made-up functions compiled to
  identical code with MSVC at `/O1` and `/O2` in five cases, the sixth a probe artefact.
- **Conversion:** `Script/convert_bindings.py` rewrote 1,644 of 1,645 rows in 45 files, one commit per file. The
  first 8 files were built and tested one by one (about 3 minutes each), the other 37 in batches of 7 with one
  build and `ctest` per batch (about 35 s per file; the build is about 165 s whatever the batch size), so those
  commits were not built singly.
- **Tags, 69 rows:** where the old wrapper differed from the signature, the row keeps the old contract with a
  tag: `ScUndefinedResult` 35 (setters whose macro returned an undefined value, not 0), `ScManagedBy<globalDynamicArray>`
  21 (`void *` arrays), `ScVoidResult` 3 (`*_RunFrameEvent` return a `bool` the script never sees), `ScBoolResult` 3
  (an `int` returned as a script bool), `ScBoolArg` 3 (`String.EndsWith`, `StartsWith`, `Replace` take `int`), `ScIntArg`
  2 (`Overlay.set_AutoSize`, `VideoPlayer.Open` take `bool`), `ScResultAs<sc_File>` 1 (`File.Open` returns `void *`),
  `ScManagedBy<ccDynamicGUIControl>` 1 (below). Each is a candidate for a signature fix, which would be a
  behaviour change and is not done.
- **Proof, generated wrapper against the macro wrapper it replaced** (`Script/compare_bindings.py`, pairs by table
  row; the tables' names, order and plugin functions unchanged in every run; `ScriptExports.*` unchanged):

  | Build | Identical | Different |
  |---|---|---|
  | MSVC `/O1 /Ob1` objects | 1,600 of 1,600 | 0 |
  | MSVC Release `/O2 /GL` LTCG, `ags.exe` | 1,599 | 1: `RunAGSGame`, one store with base and index registers swapped |
  | GCC 13.3 `-O3 -flto` | 1,600 | 0 |
  | GCC 13.3 `-O2`, no LTO | 1,584 | 16: the 15 format wrappers and `RunAGSGame` |

  The 16 with GCC `-O2` are the same up to the last `ret`; the exception landing pad sits at the function's end
  where the old wrapper had it in a `.cold` part (2 instructions fewer each). No cost on the normal path.
- **The design change that the proof forced:** the first binder called an inner helper per call; MSVC at
  `/O1 /Ob1` did not inline it for 2 of the first 144 wrappers. Each binding is now one function template that
  reads its parameters in its own body, and no `__forceinline` is needed.
- **After the conversion:** `script_api.h` holds `ScriptSprintf`, `ScriptVSprintf` and the two macros of the
  plugins' C-variadic functions (840 lines to 81); `runtimescriptvalue.h` no longer includes it (the layering
  baseline 129 to 128). `Sc_System_Log` is written by hand: it formats without translating.
- **Cost** (MSVC Release, three runs each, old and new alternated in one source and build directory, idle
  machine): clean engine build 56.8–58.2 s before, 53.8–58.3 s after; LTCG link 35.4–38.4 s before, 34.9–41.5 s
  after: no measurable difference. `ags.exe` 6,962,688 to 6,959,104 bytes. The 46 API objects grow from 2.2 MB
  to 5.3 MB (unreferenced template copies and longer names, which the linker drops).
- **Compilers:** MSVC 19.44, GCC 13.3, Clang 20 with libc++ (47 files clean). Not compiled: GCC 8 (the Debian 10
  builder; the binder uses `template <auto>`, `if constexpr` and fold expressions, no partial specialisation on an
  `auto` parameter), macOS, iOS, Android.
- **Verified on the merged tree:** `prepush.py` with every stage (format, helpers, layers, build lists, MSVC build
  and `ctest`, tidy, headers, libc++, Emscripten, the Linux GCC job with 1,207 tests, MSBuild with the Editor's
  NUnit tests); the first run failed the tidy gate on `<stddef.h>` and `<stdint.h>` in the rewritten
  `script_api.h`, fixed. The auto-test game 2414 of 2414 on Software and D3D9.
- **Found, not fixed:** `GUIControl.OwningGUI` returns a `ScriptGUI *` with the GUI control manager, probably a
  defect, kept by its tag; three macros asserted the wrong parameter count (`LockViewAnchored` 1 of 6,
  `BlendSurface` 10 of 11, `SetConstantF4` 3 of 5), the binder asserts the real one (Debug builds only).
- **Lessons:** build time compared across two source directories is not a fair test (7% difference on unchanged
  files; use one source and one build directory, alternate the trees, idle machine); batches of 7 files per build
  cut the conversion from about 2 hours to 25 minutes at the price of commits not built singly; a long agent run
  is split at a phase boundary with a written handoff note, and the second agent did the proof phase from it.

### 5.1 step 1: every global declared once

*Definition:* take the `extern` declarations out of the engine's source files: each global is declared in one
header, the file that defines it includes that header, and the files that use it include it too. No function's
code changes.

**Status (2026-11-15): done**, 5 commits (`92a98be06`..`3c0a21a65`) and the rename `632e3fec5`; prepush
passes every stage; not pushed.

- **Before, measured:** 552 `extern` lines in 107 source files of `Engine/` (533 without the tests; BASELINE.md
  had 596 at the start of the refactor), naming 190 globals and functions. 39 of the names had a header
  declaration that the files did not include; 151 had none, 47 of them functions.
- **What was done:** the missing declarations written by hand into the header of each global's domain, with a
  comment each (86 `extern` and 49 function declarations in 26 headers; `game/game.h` now has 38, `game/script.h`
  18, `game/world/character.h` 14). Then `Script/collect_externs.py` deleted 549 lines, added 137 includes and
  took the keyword off 3 forward declarations of functions (`prescan_saves`, `dispose_engine_overlay`, and
  `main` in the iOS file); it also gives a defining file the include of its declaration when it lacks it (4
  includes: `game.cpp` for three `scr*` arrays, `minidump.cpp`). The `Register*API` functions are declared in
  `game/script_api/exports.h`, which `bindings.h` includes. One new header, `platform/windows/minidump.h`.
  `mousex`, `mousey` and `ignore_bounds` were declared in `game/ui/mouse.h` and defined in
  `device/mousew32.cpp`: `device/mousew32.h` alone declares them now.
- **Result:** `extern` declarations in engine source files 552 to 0; in the engine as a whole 652 to 203, all
  in headers, one per global. `Script/check_layers.py` fails on a new one (prepush stage `layers`,
  `verify.yml`).
- **No declaration disagreed with its definition** (MSVC, GCC and Clang compile each definition against its
  header now).
- **The 15 dependencies that the `extern` lines hid.** A file of a lower ring that named a global of a higher
  one broke the ring rules without an include to show it. The includes are in the layering baseline now, 128 to
  143, and are the first things for step 2 to remove:

  | File (ring) | Global | Now includes |
  |---|---|---|
  | `util/timer.cpp` (base) | `game_update_suspend`, `want_exit`, `abort_engine` | `game/game.h` |
  | `debug/debug.cpp` (base) | `game`, `displayed_room`, `want_exit`, `abort_engine`; `thisroom`; `check_dynamic_sprites_at_exit` | `game/game.h`, `game/world/room.h`, `game/render/dynamic_sprite.h` |
  | `device/mousew32.cpp` (host) | `switched_away` | `game/game.h` |
  | `gfx/ali3dogl.cpp`, `platform/windows/gfx/ali3dd3d.cpp`, `ali3dd3d11.cpp` (host) | `palette` | `game/render/draw.h` |
  | `platform/emscripten/acpemscripten.cpp` (host) | `gfxDriver` | `game/render/draw.h` |
  | `platform/windows/win_ex_handling.cpp` (host) | `proper_exit`, `spriteset`; `eip_guinum`, `eip_guiobj` | `game/game.h`, `game/ui/gui.h` |
  | `script/scriptexecutor.cpp` (vm) | `scriptExecutor` | `game/script.h` |
  | `game/game_init.cpp`, `game/script.cpp`, `game/world/room.cpp` (game) | `logScriptRTTI`, `logScriptTOC` | `main/main.h` |

- **Found by the object comparison: an include changed code.** The first MSVC comparison showed 4 functions of
  each Direct3D driver compiled differently: `TRUE` had become -1. `<windows.h>` and Allegro each define `TRUE`
  only if it is not defined yet, as 1 and as -1, and `game/render/draw.h` brought Allegro in ahead of the
  Direct3D headers, so `SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE)` passed `0xFFFFFFFF`. The two drivers
  include `platform/windows/windows.h` first now and compile to the code they had. It compiled and linked; only the
  object comparison showed it. The trap is older than this task and is in
  STYLE.md ("Headers and includes").
- **Proof, MSVC** (`compare_objects.py HEAD`, `/O1`, 674 objects): 537 identical, 120 with the same code in
  another order, 8 test objects that differ as test objects do, and 9 where one compiler helper was added and
  no function changed (`vector constructor iterator` in 5, `std::locale::id::id<0>` in 4; `dumpbin /relocations`
  shows that nothing but the helper's own unwind data refers to it). The tool's verdict is "not mechanical"
  because of those 9 helpers. The `ags.exe` of prepush's Release build is 6,959,104 bytes, the size 5.2 recorded;
  its functions were not compared.
- **Proof, GCC 13.3 in WSL** (`.compare/helpers/gcc_cmp_51.sh`: both trees built in one source and build
  directory, objects hashed, the differing ones compared function by function):

  | Build | Result |
  |---|---|
  | `-Os`, no LTO, 708 objects | 697 byte-identical; 8 test objects differ; 3 engine objects differ in bytes: `game.cpp` and `ali3dogl.cpp` in the order of their functions only, `draw.cpp` in one libstdc++ function |
  | `-O3` with LTO (Release), 17 linked programs | 15 byte-identical (the tools, `agscc`, the other tests); `engine_full_test` differs (a test); `ags`: 33 of 14,530 functions differ, text 7,943,205 to 7,943,461 bytes (+256) |

  The one function at `-Os` is `vector<unique_ptr<Bitmap>>::_M_default_append`: GCC inlined the destructor of
  the moved-from elements, which does nothing, where it called it before (93 to 85 instructions). The 33 at
  `-O3` include FreeType (`FT_Stroker_CubicTo`, `cff_get_ros`, `tt_done_blend`), SDL (`DSP_PlayDevice`) and
  `std::regex` functions, which no changed header reaches: the whole-program optimiser decides by the order of
  what it is given. None of the 33 is in the script VM, a blender or a pixel loop (the engine's own:
  `prepare_roomview_frame`, `init_translation`, `video_core_slot_init`, the four `MotionPath_GetStage*` and
  their bindings, `AssetManager::RemoveLibrary`, `RoomStruct::~RoomStruct`, ...).
- **Verified:** `prepush.py --auto` with every stage: format, helpers, layers, build lists, MSVC build and
  `ctest`, headers, libc++ (335 headers), Emscripten (`ags.wasm` linked), the Linux GCC job (1,207 tests),
  MSBuild with the Editor's NUnit tests: pass. `tidy`: see the next point. The auto-test game 2414 of 2414 on
  Software, Direct3D 9, Direct3D 11 and OpenGL.
- **The tidy gate, two things.** It reported `croom`, `objs` and `scsystem` as banned abbreviations "in a new
  name": their `extern` lines were new header lines. `Script/tidy_diff.py` no longer applies that check to an
  `extern` declaration, which repeats the name of its definition. One finding was not from this task:
  `game/world/room_object.h:71`, "invalid case style for public member `audio_speed`", on the line whose
  initializer `0b94d780b` changed from 0 to 1000 (the gate reads a changed declaration line as a new name).
  The member is `AudioSpeed` now, in `RoomObject` and in `CharacterExtras`, which has the same thing for a
  character (the user, 2026-11-15; `632e3fec5`, a first piece of 4.3; 674 objects identical). With it
  `prepush.py --auto` passes every stage.
- **Not compiled by any local build:** `platform/ios/acplios.cpp` (the keyword of one forward declaration
  removed), and the macOS and Android builds, whose changed files are the shared engine sources that the Linux
  and Emscripten stages compile.
- **`#pragma once` was not everywhere, and nothing checks it.** Nine first-party headers lacked it; one of
  them had an include guard instead. `device/mousew32.h` and `platform/base/sys_main.h` got it with the engine
  commit, because their second inclusion failed the build; the other seven in `9078a8c83` (the user,
  2026-11-15: all of our code uses it): `gfx/ogl_headers.h`, `platform/base/mobile_base.h`,
  `Common/data/data_helpers.h`, `Compiler/preproc/preprocessor.h`, and `diagnostics.h` (which had a guard),
  `pipedprocess.h` and `test/spriteimport_env.h` of the tools. `compare_objects.py` on that commit: 657 objects
  identical, 17 with the same code, "mechanical change"; prepush with every stage ran on the tree that has it.
  The two `resource.h` files
  (`Engine/resource`, `Editor/AGS.Native`) are left: Visual Studio's resource editor writes them and `rc.exe`
  reads them.
- **Also found:** two source files have a second block of includes in the middle, left by the folds of 5.3
  (`game/game.cpp`, `game/dialog/speech.cpp`); outside the engine, `extern` in source files:
  `Editor/AGS.Native` 61, `Common` 5 (4 of them Allegro's blender functions), `Compiler` 2, `Plugins` 2.
- **Commits**, in the order that keeps each one passing its checks: `92a98be06` the tool and the tidy gate's
  rule for `extern` declarations; `2805bc661` the engine, its build lists and the 15 baseline lines, with the
  comparison results in its message; `2356f0c20` the `extern` check of `check_layers.py`; `9078a8c83`
  `#pragma once`; then the documents.
- **Lessons:** a change that only adds includes is not mechanical until the objects say so; a helper COMDAT
  that nothing references shows up as an "added function" in `compare_objects.py`, which could tell it apart
  by its relocations; GCC's objects are byte-identical when only declarations move, so a hash comparison in
  WSL is a cheap second proof, but a Release build there is LTO and only its linked programs can be compared,
  where a few dozen functions move with the input order whatever the change.

### 5.1 step 2 and 5.1a: the design

*Definition:* decide what the 203 globals become and how design data is kept apart from runtime state, before
any conversion.

**Status (2026-11-15): designed with the user, no code changed** (`fecc3d7f9`). The design and its evidence are
`RUNTIME-SPLIT.md`; PLAN.md has the tasks (5.1 steps 2 to 4 and 5.1a rewritten; 2.10, 5.8 to 5.12 and L.3 new)
and their order in "Next up".

- **Decided:** globals are placed by lifetime (`GameDef`, `Game`, `Room`, `EngineServices`); the definitions
  are kept as loaded and a live object is built from its definition; three words, `XDef`, `X`, `XState`, with
  each field declared once in a struct; the Editor's class name as the stem, room entities with the `Room`
  prefix; `GUI` with the namespace renamed `GUIUtil`, `EngineConfig`, `GameState` once `GamePlayState` is sorted
  by owner, `RoomDef` for `RoomData` with `RoomStruct` merged into `Room`; the accessors `X::Current()`; a
  component describes itself (game file methods on the records, the new savestate format from property rows)
  and today's save code is gathered into one legacy reader; the savestate format is free to change, with a
  legacy routine for older saves (C2 constrains reading only); the editor's C library stays (6.7c).
- **Changed during the review:** deriving the live class from its record instead of redeclaring fields
  (the user: fields are a struct's job); no file per save component (the user: compile time; measured at 2.4 s
  per file); the game file's methods stay on the records instead of moving to free functions (the user:
  components serialise themselves); the suffix `Def` instead of `Info` (below); no base type for the tools'
  part of the room (the two room types are a definition and its runtime
  form, not two definitions).
- **Found by a last review against the code, and corrected in both files:** `GameSetupStruct` has a member
  named `GameInfo` (the game's descriptive texts), which is also a script type frozen by C3, so the first
  draft's class `GameInfo` would have shared its name: the user chose the suffix `Def` for every definition
  (`GameDef`, `CharacterDef`...), which leaves `Info` to mean facts about a thing; the sprite cache writes `game.SpriteInfos` through a reference; the audio clip
  needs a live object because scripts hold its address; `CharacterInfo` and `CharacterExtras` both have
  `flags`; `RoomObject` repeats `RoomObjectInfo`'s fields under other types; the palette is game state, so the
  drivers are given it instead of it moving down a ring; `debug/debug.cpp` reads game state, so 12 of the 15
  exposed includes go with `EngineServices`, not 15, and 21 carry `usetup`, not 18 (the target of 110 holds);
  an in-place exchange of two `Game` objects conflicts with the pool's address map (left to 5.11); the engine
  structs' and private-member classes' save methods stay where they are until 5.12.
- **Left for the tasks:** how far the property rows go towards data-driven code (opens 5.9), the shared parts'
  names (5.1a step 2), dynamic sprites with two `Game` objects and `RestartGame()` without a restart point
  (5.11), matching lists by script name in a savestate (5.12), a precompiled header or unity builds (2.10b).
- **Found:** three defects in today's save code, read in the code and not fixed (PLAN.md open items).

### The three save defects of the design pass

*Definition:* PLAN.md "Next up" item 2 (a): fix the three defects of RUNTIME-SPLIT.md 11, a test each.

**Status (2026-11-15): done** (`1dc82bb6c`, not pushed). By an Opus 5.5 subagent in a worktree, the patch read and applied
by the supervisor.

- A character's animation volume is restored (`CharacterExtras::ReadFromSavegame` read it into a member nothing
  used and rebuilt the animation with 100; the member `cur_audio_volume` is gone); an animated overlay's wait
  counter is restored (`AnimatedOverlay::ReadFromSavegame` read it into a local); `RoomStatus::ReadFromSavegame`
  reads the walk-area count before the walk-behind count, as the writer writes them. The stream's bytes are the
  same in all three.
- Tests, in `Engine/test/savegame_components_test.cpp`, each seen failing before its fix:
  `SavegameContent.CharacterAnimationVolumeIsRestored`, `SavegameState.AnimatedOverlayWaitIsRestored`,
  `SavegameState.RoomStatusReadsTheCountsInTheWrittenOrder` (a stream with 1 walk area and 2 walk-behinds, which
  no engine writes: both limits are 16).
- **No existing save loads differently through the third fix:** the counts came with `ddc9d21b7` (2024-02-15),
  the writer has had no version branch since, and `MAX_WALK_AREAS` went from 15 to 16 a month before; read in
  the history, not tried with saves. Through the first two, a save now restores what it holds.
- **Checked on the way** (RUNTIME-SPLIT.md 11): `init_object_states`' double assignment is redundant, not a
  defect; `move_object` on a moving object leaks the move list and leaves `Object.MotionPath` stale;
  `GetObjectIDAtRoom` tests against the scaled size; `RoomStatus::ReadFromSavegame` does not check its counts
  against the fixed arrays. Open (PLAN.md open items).

### 5.1 step 2: `EngineServices`

*Definition:* the globals that live as long as the process become members of one struct reached through
`EngineServices::Current()`; the engine's config moves to the host ring with it. *Done when:* the layering
baseline is 110 or less and these globals are reached only through the accessor.

**Status (2026-11-15): done**, 10 commits (`b8ac51d20`..`59b5d7092`, the save fixes among them); prepush
passes every stage; not pushed.
Designed and supervised by Fable 5.2, the batches by four Opus 5.5 subagents and one Sonnet 5.5 (the
measurement), each batch built once.

- **The design, where it differs from PLAN.md's text.**
  - `EngineServices` is a plain struct in `Engine/platform/base/engine_services.h` (host ring, global namespace
    like `EngineConfig` and `AGSPlatformDriver`): `Version`, `Config` (the `EngineConfig`, by value), `ResPaths`,
    `VoiceAvail`, `SeparateMusicLib`, `TextLanguage`, `TextLocaleName`, `Platform`, `GfxDriver`, `Assets`, the
    loop flags (`SwitchingAwayFromGame`, `SwitchedAway`, `GameUpdateSuspend`, `WantExit`, `AbortEngine`,
    `ProperExit`), `LogScriptRTTI`, `LogScriptTOC`. The engine's own instance is a static of
    `engine_services.cpp` and `_current` is initialized with its address, a constant, so `Current()` is valid
    before any dynamic initialization, as the globals were; `SetCurrent()` is for tests.
  - **"Every ring can include it" was wrong for two rings.** The host ring is above `util` and `debug` and beside
    the script VM. The VM does not need it. `util/timer.cpp` (the frame wait loops read the loop flags) and
    `debug/debug.cpp` include it: two new baseline lines, with the reason that both already include the host's
    `agsplatformdriver.h`. A struct of forward-declared pointers in the base ring would have had no such lines,
    and could not have held the config by value.
  - **The sprite cache stays a global until step 3.** `SpriteCache spriteset(game.SpriteInfos, ...)` keeps a
    reference to a vector of the game struct, which it resizes and writes for dynamic sprites, and
    `texturecache(spriteset)` in `draw.cpp` binds to it during static initialization: a pointer in the struct
    would be null at that moment, and an owner in the struct would have to own the sprite table too, which is
    step 3's question (`GameDef` and `Game`) and 5.11's (dynamic sprites per `Game`).
  - **The palette: a pointer given once, not a call per change.** `IGraphicsDriver::SetIndexedPalette(const RGB
    *)`, stored in `GraphicsDriverBase`, called in `main/graphics_mode.cpp` where the driver is created. The
    three hardware drivers convert 8-bit bitmaps with it and no longer include `game/render/draw.h`. About ten
    places change the palette in place; a call at each would be ten chances of a stale copy.
  - **Use sites:** one use in a function is `EngineServices::Current().Member`; more than one,
    `EngineServices &services = EngineServices::Current();` as the first statement and `services.Member`.
    Always the reference to the struct, never a copy of a member pointer (the drivers and the asset manager are
    created and destroyed while the engine runs).
- **The batches**, each kept as a snapshot `refs/wip/51s2/<name>` made with `git stash create` (no branch,
  index or working tree change) until the commits were cut along them:
  1. `prep`: the display setup types (`WindowSetup`, `DisplayModeSetup`...) from `main/graphics_mode.h` to
     `gfx/display_setup.h`; `eScriptSystemOSID`, `RuntimeBehaviorSwitch`, `SkipSpeechStyle`, `SpeechMode` to
     `platform/base/engine_defines.h`; `[rename]` `GameSetup` to `EngineConfig`; `[move]` `main/game_setup.h` to
     `platform/base/engine_config.h`; 11 includes of lower-ring files switched or dropped. Baseline 143 to 111.
  2. `moves`: `scriptExecutor` defined in `script/scriptexecutor.cpp`; `eip_guinum`, `eip_guiobj` in
     `debug/debug.cpp`, declared in `debug/debugger.h`. Baseline 107.
  3. `services`: the struct, `Script/use_context.py`, 656 uses of 14 globals in 41 files (148 direct, 508
     through the local in 113 functions), the definitions and `extern` lines deleted. Baseline 97.
  4. `savefix`: the three save fixes above, applied on top.
  5. `state`: the palette; `voice_avail`, `separate_music_lib`, `enable_antialiasing` and the text language out
     of `GamePlayState`. Baseline 94.
  6. `final`: four lines for the tidy gate and one for `check_helpers.py` (below).
- **`Script/use_context.py`** (864 lines): a tokenizer that skips comments, literals and preprocessor lines, a
  brace tracker that tells function bodies from namespace, class and initializer blocks and follows `#if`
  branches; a use in a `[&]` lambda goes through the local, in any other lambda or in a class defined inside a
  function it is direct; parameters, locals and class members of the same name are left alone; `--dry-run`,
  `--check`, `--self-test`. The context's type, accessor, local name, header and name map are one table entry:
  step 3 adds `Game` and `Room`. **A tool proved on three files was not proved here either:** on the whole tree
  it turned the member `SavegameDescription::EngineVersion` into the global's replacement in a copy
  constructor; found by reading the diff, and the tool knows class members since.
- **What left `GamePlayState`.** `voice_avail` and `separate_music_lib` described the running engine's packs
  and were carried around a restore by `PreservedParams::SpeechVOX` and `MusicVOX`, which are gone. The save
  keeps its slot: the reader reads `separate_music_lib` and drops it, the writer writes the engine's value
  (test `SavegameContent.RestoreKeepsTheRunningEnginesMusicPack`). `enable_antialiasing` was a copy of
  `Config.AntialiasSprites`, which nothing writes after the config is loaded: `ShouldAASprites()` reads the
  config and is out of line (`game_state.h` must not include the config, which brings SDL). The text language
  and its locale name are members; `SetGameTextLanguage` is a function of `game/translation.cpp`.
  Two edges: a plugin whose restore callback asks whether the music pack is there now gets the engine's answer,
  not the save's (no engine code does); after a failed restore the engine's value stays instead of the save's.
  `unload_game` no longer resets the four values; `InitGameState` and `engine_init_game_settings` set them when
  the next game starts.
- **Proof of the mechanical batches, MSVC** (`compare_objects.py`, `/O1`, 674 objects). `prep` against HEAD: 19
  objects differ, exactly the 19 sources that name `usetup`, because MSVC puts a variable's type into its
  symbol (`?usetup@@3UGameSetup@@A`). Against "HEAD with only the word renamed" (a commit object made by
  `.compare/helpers/mk_rename_ref.py`): 652 identical, 22 with the same code, "mechanical change". `moves`
  against `prep`: "move: 0 functions changed" (the global's initializer went from one object to the other).
- **The conversion is not mechanical, by design:** 39 objects, 337 functions changed at `/O1`. Not among them:
  the script VM, `Engine/gfx` and `Common/gfx` (the software renderer and the blenders), `Common/util`, the
  sprite cache, the audio mixing.
- **Measured, MSVC x64 `/O2` without `/GL`** (cl 19.44; the four changed objects on or near a hot path,
  compiled from the `moves` and `services` snapshots with the Release flags, `dumpbin /disasm` compared function
  by function; scripts and tables in `.compare/perf51`):

  | Object | Instructions | Bytes | Functions changed |
  |---|---|---|---|
  | `game/render/draw.cpp` | 37,271 to 37,660 (+1.04%) | 136,149 to 137,763 (+1.19%) | 66 of 2,131 |
  | `game/render/screen.cpp` | 3,552 to 3,599 (+1.32%) | 13,252 to 13,544 (+2.20%) | 23 of 205 |
  | `util/timer.cpp` | 643 to 647 (+0.62%) | 2,440 to 2,454 | `WaitForNextFrame` |
  | `media/audio/sound.cpp` | 5,329 to 5,333 (+0.08%) | 18,373 to 18,395 | 2 of 458 |

  The functions that run once per drawn object (`prepare_objects_for_drawing`, `prepare_characters_for_drawing`,
  `prepare_and_add_object_gfx`, `construct_object_gfx`, `sync_object_texture`, `add_to_sprite_list`,
  `draw_sprite_list`) are byte-identical. A function with the local loads `_current` once at entry and reads
  members through a register; in `render_to_screen`, `put_sprite_list_on_screen` and `draw_gui_controls_batch`
  that replaced a load of the global inside the loop by none. **Two loops reload `_current` per iteration:** the
  per-GUI loop of `draw_gui_and_overlays` (two uses, from single-use helpers inlined into it) and the loop
  condition of `ShakeScreen`. A micro-benchmark of the pattern (100 million virtual calls, the pointee in
  another translation unit): the global 1.42 to 1.45 ns per call, `Current().Member` per iteration 1.64 to 1.71,
  a held reference 1.43 to 1.46. So the cost was about 0.2 ns per reload, twice per GUI per frame. **Moved out
  of the loops** (the user, 2026-11-15): a helper inlined into a loop loads the pointer again, whatever local
  its caller has, so the helpers take the services from the caller: `recycle_render_target`,
  `recreate_drawobj_bitmap` and a second `CreateCompatBitmap` have an `EngineServices &` first parameter, and
  `ShakeScreenState` holds the reference as a member. Checked in the `/O2` object
  (`.compare/perf51/verify_loops.py`): `draw_gui_and_overlays` loads `_current` once, at its entry, and none of
  its three loops does (491 instructions; 488 before the conversion, 494 after it); `ShakeScreen` loads it
  once, before its loop. This is the first use of what step 4 makes the rule: the context as a parameter.
  Most of the growth in instructions is not loads: MSVC now inlines
  `recycle_ddb_bitmap` and `CreateCompatBitmap` at six sites (+23 to +46 instructions each, the executed path
  about the same); why its inliner decided so was not found.
- **Measured, GCC 13.3 `-O3` with LTO** (the linked `ags` of `moves` and of `final`, in WSL): text 7,942,301 to
  7,945,361 bytes (+0.04%); 1,470,249 to 1,470,758 instructions over all functions (+0.035%).
  `ScriptExecutor::Run(int)` has 2,718 instructions in both, and its diff is the name of the neighbour of its
  jump tables. 902 functions differ in instructions by `.compare/helpers/gcc_exe_code_51s2.py`, many with equal
  counts: the new virtual method moves the vtable offset of every later `IGraphicsDriver` call, and LTO moves a
  few dozen functions with any change of input (5.1 step 1). The functions that differ have 487 instructions
  fewer in all.
- **Compile time** (2.10; BASELINE.md "compile time", "After 5.1 step 2"; two clean builds each): MSVC total
  1685.5 and 1687.8 s against 1731.4 and 1676.4 before, median per object 2.62 and 2.57 s against 2.70 and
  2.59: inside the spread. GCC: the first reading was disturbed (`Common`, untouched, 10% slower than in the
  second); the second is 1625.4 s against 1603.9 and 1585.3 (+1.3% to +2.5%), `Engine/game` 460.6 s against
  464.8 and 459.6. `script_api` took 123.2 s against 108.4 and 108.9 with GCC, which the step does not explain
  (none of its files includes the new header): to read again in 2.10b. 47 source files and no header include
  `engine_services.h`.
- **Verified:** `prepush.py --auto` with every stage: format, helpers, layers (94), build lists, the MSVC build
  and `ctest`, tidy, headers, libc++ (338 headers), Emscripten (`ags.wasm` linked), the Linux GCC job (1,211
  tests), MSBuild with the Editor's NUnit tests; `--only sanitizers` (ASan, UBSan and leaks on the unit tests and
  the auto-test game): no finding. The auto-test game 2414 of 2414 on Software, Direct3D 9, Direct3D 11 and
  OpenGL, with the project at tag `draconian-auto-test-3`.
- **The first prepush run failed two stages**, both fixed: `helpers` took `EngineServices::Current()` for a
  copy of `Directory`'s (listed in `check_helpers.py`'s `KNOWN`: the accessors of the contexts); `tidy` reported
  four old lines that the conversion touched (`push_back(make_pair)`, `reset(new)`, a C-style cast, the unused
  parameter `game` of `ApplyBehaviorOptions`, whose name is commented out now).
- **A false alarm worth knowing:** the first auto-test run showed 5 failures in the character placement tests
  on every renderer. The local export of the test project (`.compare/proj/ags4/auto-test`) was older than the
  tag; `.compare/helpers/run_autotest4.sh` now takes the project directory, exported from the tag.
- **Not compiled by any local build:** `platform/ios/acplios.cpp` (1 use, 2 includes), `platform/android/
  acpland.cpp` and `platform/osx/acplmac.cpp` (an include each), the `BUILD_STR` and
  `AGS_SEARCH_FOR_GAME_ON_LAUNCH` branches of `main/engine.cpp` and `main/main.cpp` (4 uses), the mobile branch
  of `main/graphics_mode.cpp` (1 use); the Xcode project's edit (`engine_services.cpp` added,
  `game_setup.h` renamed) was not opened in Xcode. Not run: a game with 8-bit colour on the hardware drivers
  (the path `SetIndexedPalette` serves), the corpus game.
- **Left:** `util/timer.cpp` is frame pacing that needs the host (five baseline lines) and would be at home in
  the host ring; `asset_helper.h`'s host half; `ApplyBehaviorOptions`' unused parameter; `ResourcePaths::
  VoiceAvail` beside `EngineServices::VoiceAvail`; `AGSPlatformDriver::GetDriver()` beside
  `EngineServices::Platform`.
- **Lessons:** a ring table has to be read before a sentence like "every ring can include it" is written; a
  global whose constructor takes a reference to another global is not a service until that reference has an
  owner; a rename of a type shows up in every object that names a variable of that type (MSVC), so a
  `[rename]` is proved against a base with the rename applied; snapshots by `git stash create` under
  `refs/wip/` let several agents build on one working tree in turn and keep the batches apart for the commits;
  the tidy gate reads a reformatted old line as new, so a conversion pays for the lines it touches.

### 5.1 step 3: `GameDef`, `Game`, `Room`

*Definition:* the globals of one play-through become members of `Game`, the room-loaded ones of `Room`, the
rest of the process-wide ones and the sprite cache of `EngineServices`; `GameSetupStruct` becomes `GameDef`.
*Done when:* no `extern` of mutable state in an engine header but the contexts and a list of exceptions in
`Script/check_layers.py`; the median compile time per file not higher than before.

**Status (2026-11-16): the plan's text is done; committed, not pushed** (`26a9122bb`..`0501021fe`, 20
commits: the tool, the rename in three, a commit per batch, two for the tidy gate, the documents). The
commits were cut from one snapshot per batch (`refs/wip/51s3/*`, made with `git stash create`, deleted
since) by `.compare/helpers/51s3/commit_51s3.py`, which keeps the documents out of the code commits. Designed and
supervised by Fable 5.2; the batches by Opus 5.5 subagents (the tool, `core`, `room`, `world`, `ui`, `script`,
`services`, `exceptions` and `sprites`, `hot`, `roomfields` and `viewports`, `gamedef-lists`) and Sonnet 5.5
ones (the rename, the probe, the code comparison, the tidy fixes), each batch built once. The user built the
tree and ran a game on it after `hot`. The compile-time cost is real (median per object +5%, below) and
accepted by the user. The `linux`, `emscripten` and `msbuild` stages, left open for the last six commits (the
tidy fixes and the three batches of 2026-11-16), passed on `ec0098a2a` later that day (`ags.wasm` linked;
1,218 tests under GCC; 5 solutions and the Editor's NUnit tests); pushed, 111 commits
(`ffa8705f4..ec0098a2a`), and CI's `Verify` and `CI` runs passed.

- **The design, where it was decided here** (RUNTIME-SPLIT.md 2 and 9 hold the reasons):
  - `class Game : public GameDef` and `class Room : public RoomStruct`: the definition and the live state stay
    one object until 5.1a gives an entity a live copy, so `game.chars` reads as before through `Game &game`,
    each use was converted once, and `GameDef::Current()` waits for the first kept definition.
  - The contexts have headers of their own (`game/game_context.h`, `game/world/room_context.h`), not
    `game/game.h`. A probe decided how members are held: today's module headers for every member type, forced
    into the 120 sources that would use the contexts, made those 28% slower to compile (+0.95 s each, `Engine`
    +17%, all +8%; the files without it +4%). So: by value from light headers (the `GameDef` base, `Play`,
    state structs of plain values declared in the context header, `std::vector`s of forward-declared types
    with the constructor and destructor out of line), a reference member where only a module header defines
    the type (`Game::Scripts`, `Room::TempState`, `EngineServices::Sprites`). The plan's "about 335 files"
    counted the word `game` in include paths: `game/game.h` has 101 includers, 73 files named `game` or `play`.
  - The loose globals are grouped: `RoomChange`, `Speech`, `LipSync`, `TextLayout`, `Cursor`, `BuiltinDialogs`,
    `Scripts`, `Handles`, `Events` in `Game`; `Input`, `Debug` in `EngineServices`.
  - The sprite cache: `SpriteCache` takes its table and callbacks after construction (`SetSpriteInfos`,
    `SetCallbacks`, a table of its own until then; two unit tests, one seen failing first); the services own it;
    `init_sprite_cache()` attaches the game's table as the second statement of `initialize_engine`; the texture
    cache of `draw.cpp` no longer binds during static initialization.
- **The batches** (uses converted by `Script/use_context.py`, direct form + through the local):
  1. `rename`: `GameSetupStruct` to `GameDef` (equal to a word replacement of HEAD but two re-wrapped lines),
     the files to `game_def.h/.cpp` (`compare_objects.py`: mechanical, 3 objects moved), the base folded in.
     The fold is not byte-mechanical: the member offsets are the same (checked in `GetCharIDAtRoom`), MSVC
     `/O1` generates other code in about ten functions that read both former halves (only that one was
     disassembled); `tools_test` now compiles `main_game_file.cpp` too.
  2. `core`: `game` 1,020 uses, `play` 1,510, 73 files. Two macros hid `play` (`FONT_NORMAL`, `FONT_SPEECH`):
     removed, written out.
  3. `room`: `thisroom`, `croom`, `objs`, `troom`, `roomscript` and six more, 689 uses, 25 files.
  4. `world` with dialog: 47 globals, about 980 uses, 53 files. `loaded_game_file_version` stays a value of its
     own (`Game::LoadedDataVersion`): `--tell data` prints it after the pre-read, when it is still undefined.
  5. `ui`: 29 globals, about 530 uses; `CursorGraphicState` moved to a light header. Corrected in the next
     batch: three "state" values of the built-in dialogs were constants (`constexpr` in `csci_dialog.h` now),
     and the init functions of `game_init.cpp` take `Game &`, not `GameDef &`.
  6. `script`: script modules and callbacks, handle arrays, events, `ScSystem`, audio channel objects, the
     palette, about 450 uses. `ScriptEventCb` is `const`. No `"system"` export by address exists.
  7. `services`: start-up values, input state, debugger state, `GameScaling`, 27 globals, about 310 uses; the
     layering baseline 94 to 92.
  8. `exceptions`: `EXTERN_ALLOWED` in `check_layers.py`, 41 variables (8 constants, 24 stateless script
     managers, 5 of the script ring, 4 crash handler values, `pluginReaders`); the check fails on another
     `extern` variable in an engine header and on a stale entry (both seen failing). `extern` lines in engine
     headers: 190 to 41. `gl_SysConfig` and `pluginsWantingDebugHooks` are services members,
     `roomBackgroundBmp` a static of `draw.cpp`. The design said 26 `ccDynamic*` managers; there are 19.
  9. `sprites`: 164 uses. Four savegame tests crashed until their fixtures attached the table: they had relied
     on the old static being bound to the game's table from process start.
  10. `hot`: about 45 helpers of the draw, update, walkable-area, hit-test, overlay and input paths take the
      context from their caller (`Game &`, `Room &`, `EngineServices &`, in that order); the transition
      classes hold the references.
  11. `roomfields` (2026-11-16): the fields of `GamePlayState` that describe the room on screen are members
      of `Room` (`BgFrame`, `BgFrameLocked`, `AnimBackgroundSpeed`, `WalkableAreasOn`, `EnteredAtX`,
      `EnteredAtY`, `EnteredEdge`, `BgModified`, `MaskModified`, `ScriptFinished`, `LoadedWidth`,
      `LoadedHeight`; about 110 uses), reset by `Room::ResetPlayState()` beside the one place that rebuilds the
      play state (`unload_game`). The Game State save component keeps its byte order and reads into the room;
      where each value is set, reset, saved and restored relative to the room load was traced per field and is
      the same (room loading assigns only the `RoomStruct` base). **`room_width` and `room_height` are not the
      room's `Width` and `Height`:** they are 0 until the first room is loaded and after `unload_game`, while
      the `Room` object starts at 320x200 and keeps the last room's size, so `game.room_width` read in
      `game_start` would have gone from 0 to 320; they moved as they are, under names that say so.
  12. `viewports` (2026-11-16): `GameViewports` (`game/render/viewport.h`), 32 bodies moved verbatim from
      `GamePlayState` with ten data members, held as `Game::Viewports`; 184 call sites in 29 files; reset
      beside the play state's; the save component's sequence and `DoBeforeRestore`/`DoAfterRestore` unchanged.
      `GetGlobalTransform` reads the shake offset from the game.
  13. `gamedef-lists` (2026-11-16): `GameDef` holds `Views`, `Dialogs`, `Guis`, the six control lists and the
      compiled scripts; the loader reads into them (`LoadedGameEntities` keeps `CharEx`, the GUI version, the
      plugin infos and the sprite flags); `Game` lost its own lists. `DialogTopic` and `ViewStruct` are
      forward-declared in `game_def.h` and `GameDef`'s special members are defined in `game_def.cpp`.
      `InitGameState` no longer copies the lists (they were copies: `ents` is const, so `std::move` copied).
      The compiled scripts died at the end of `load_game_file`; they are kept until `unload_game` now: about
      1.6 MB for the auto-test game (669,504 bytes of code), which a second `Game` from the `GameDef` needs.
      The GUI control lists are freed at `unload_game` now; before, the next load replaced them.
- **`Script/use_context.py`** (864 to about 1,300 lines): the contexts `game` and `room`; a global that is the
  context itself (`game`, `thisroom`); a local's name that is taken means the direct form; incremental runs
  (a later batch reuses the declaration or turns direct forms into the local; proved equal to one run on a copy
  of the tree, and idempotent); **a use inside a loop takes the local** (added after `core` left
  `chan_id < Game::Current().numGameChannels`; 23 functions). Two misreadings found by the batches and fixed
  with a self-test each: `x > dialog[i]` read as the end of a template type, and a later declarator of
  `int a = 0, b = 0, win_y = ...;` read as a use. A skipped function cannot hide: the global is deleted and the
  compiler rejects it.
- **Measured, MSVC x64 `/O2` without `/GL`** (instructions, nothing timed; scripts in the session's scratch
  directory, method of step 2). After `room`, 13 objects: `draw.cpp` +0.38%, `object.cpp` +3.1%,
  `walkable_area.cpp` +6.2%, `update.cpp` -4.6%, `game_run.cpp` -3.1%: a function that holds one reference
  instead of addressing `game` and `play` many times is shorter (`doNextCharMoveStep` 197 to 117). The cost was
  step 2's pattern, a helper inlined into a per-entity loop reloading `_current` (seven loops). After `hot`, 19
  objects: 139,914 instructions before the step, 139,516 after the conversions, 139,841 after `hot` (-0.05%);
  no per-frame or per-step loop loads a context pointer in its own body; what remains is on rare branches or
  in functions declared in Common or bound to the script API. GCC was not compared.
- **Compile time, MSVC: the step costs 2.4% of the build and the criterion is not met** (BASELINE.md "After
  5.1 step 3"). On an idle machine, before, after, before, after: total 1,728 and 1,717 s against 1,773 and
  1,754 s; median per object 2.57 and 2.61 against 2.76 and 2.68 s (+5.1%); the 95 engine sources that
  include a context header +7.3% (0.27 s each), the 160 other engine sources +1.6%, the sources outside
  `Engine` +0.9% (mostly `main_game_file.cpp` compiled a second time, into `tools_test`). A first reading,
  taken the evening before with the machine in use, was worthless: three builds of the unchanged tree
  differed by 26%, and by a control group it showed no cost at all. The cost is the context header's
  includes (`game_def.h`, `game_state.h`, `viewport.h` reach every user of `Game`); `Play` and `Viewports`
  held through reference members would take two of them out, at one more load per use. By what each of the 95
  sources included before (`/showIncludes`, `.compare/helpers/51s3/ct_why.py`): the 53 that already saw both
  `game_state.h` and the game struct pay 0.08 s each, the 42 that saw one or neither carry 21.5 of the 26 s
  (the 19 that saw neither named a global from a light header, such as `displayed_room` or `guis`). The user
  accepts the cost (2026-11-16); reference members are decided against, and PLAN.md 2.10b has the order of
  what lowers compile time from here. GCC not measured (the user).
- **Verified:** `prepush.py` with every stage on the tree after `hot`: format, layers, build lists, the MSVC
  build and `ctest`, headers, libc++ (341 headers), Emscripten (`ags.wasm` linked), the Linux GCC job (1,218
  tests), MSBuild with the Editor's NUnit tests pass; `helpers` and `tidy` failed (the two new `Current()`
  accessors, two comments; 32 diagnostics on old lines the conversion touched) and were fixed in 27 files.
  A second run, stopped by the user, passed format, helpers and layers on the fixed tree. The last three
  batches (2026-11-16): the MSVC build, `ctest` (1,218), the header check, the libc++ check of all 341
  headers, format, layers and build lists, by their agents. **The auto-test game on the finished tree: 2414 of
  2414 on Software, Direct3D 9, Direct3D 11 and OpenGL** (the project at tag `draconian-auto-test-3`, built
  with the tree's tools).
  `format`, `helpers`, `layers` and `tidy` on the final tree (2026-11-16): one tidy finding, on `GameDef`'s
  move constructor, which is defined out of line now and is read as a function that must not throw; it is not
  `noexcept`, as before (a `std::map` member's move may allocate), so the line says so and silences the check.
- **Not verified:** the `linux`, `emscripten` and `msbuild` stages on the tidy fixes (ten renames,
  `RBSwitchNames` to `kRBSwitchNames`, six named casts, of which the two `reinterpret_cast`s in
  `agsplugin.cpp` were chosen without reading the types; they compile with MSVC and pass the tidy gate) and on
  the last three batches (`Editor/AGS.Native` constructs a `GameDef`, whose special members are out of line now: read in its
  project, not built); no sanitizer run; no `compare_objects.py` beyond the rename; the GCC comparison of the
  linked engine; the corpus game; macOS, iOS, Android (one edit in an Android-only branch of
  `main/engine.cpp`); a plugin that asks for the debug hook.
- **Found on the way** (PLAN.md open items): `DialogOptions::RunKey` runs the key-press callback where it
  prepared the text-input one (upstream `ags4` too); `SetSpriteNum` resets every animating overlay's image on
  every frame.
- **Lessons:** a count by `grep -w` of a word that is also a directory name is not a count of uses; a probe
  with today's headers settled in half an hour what the design left open; macros hide globals from a
  token-level tool; a tool rule ("a use in a loop takes the local") is cheaper than a list of loops to fix by
  hand, and what it cannot see (a helper inlined into a loop) needs the disassembly; a compile-time reading
  needs a quiet machine and nothing started beside it, and when the user changes what a running agent does,
  the agent is stopped, not messaged.

### 5.1a step 1: `Character`

*Definition:* PLAN.md 5.1a step 1: one live list in `Game` replaces `game.chars` + `charextra`; the
`Character_*` functions take `Character *`; `GameDef`'s characters are not written after load; first an alias
and the signatures, then the class; `CharacterInfo` becomes `CharacterDef`; with 4.3 for the two structs
(5.1a point 6).

**Status (2026-11-16): done, 17 commits (`f47b8ccb4`..`7347023cb`), not pushed**, cut by
`.compare/helpers/51a1/commit_51a1.py` from one snapshot per batch (`refs/wip/51a1/*`, deleted since:
`prep-flags`, `prep-chars`, `prep-alias`, `class`, `fields`, `names-types`, `names-common`, `names-engine`,
`fieldnames`, `trim`, `cleanup`, `runners`; the tool, the tidy fix, the prepush fix and the documents are
commits of their own; 88 files, of them 64 under `Common`, `Engine`, `Tools` and `Editor`). Designed and supervised by Fable 5.2 (`.compare/briefs/brief-51a1-*.md`); the batches by Sonnet 5.5
`medium` (`prep`, `names`, `fieldnames`, `trim`, the code comparison) and Opus 5.5 `high` (`class` and
`fields`, one agent; the rename tool), each batch built once. **Compile time: no measurable change**
(below). **Open:** the GCC comparison of the linked engine.

- **The design, where it was decided here** (RUNTIME-SPLIT.md 2, "What 5.1a step 1 built"):
  - **No layout was frozen**, which the old comments still claimed: `AGSCharacter` is opaque to plugins, and
    scripts reach one field by offset, through `CCCharacter`. So fields could move between the structs.
  - **The game file had stopped holding the play-only fields long ago**: `ReadFromFile` skips twelve slots as
    `[UNUSED]` and writes zeros (`prevroom`, `wait`, `idleleft`, `activeinv`, `blinkinterval`, `blinktimer`,
    `blinkframe`, `z`, `walkwait`, `loop`, `frame`, `walking`; `walkwaitcounter` never had a slot). They moved
    to the state with no reader change.
  - **Three definition fields were outside the record**: blend mode, graphic anchor and graphic offset were
    read into `CharDataEx` and copied into `CharacterExtras` at init. They are fields of `CharacterDef` now,
    read into directly; `CharDataEx` and `LoadedGameEntities::CharEx` are gone. The defaults were equal, and
    the copy ran for every game (the loader always sized the side list), so a game file without the extension
    behaves as before.
  - **`Game::Characters` hides `GameDef::Characters`.** `GameDef::chars` was renamed `Characters` first, as a
    `[rename]`; when `Game` got its own `std::vector<Character> Characters`, every `game.Characters[i]` of
    the engine meant the live list without a text change, and a write to the definitions through a `Game &`
    cannot compile into existence by oversight. The definitions are read through `game.Def()`, in one place
    (`InitGameState`). `GameDef::Current()` was not needed and was not added.
  - **Behaviour on the class, data in the structs**: the methods of `CharacterExtras` are `Character`'s,
    without their `ch` parameter; the three methods of `CharacterInfo` that only the engine defined
    (`get_baseline`, `get_blocking_top`, `get_blocking_bottom`) and its three predicates on play-only fields
    left Common. Seven free functions lost their `CharacterExtras *chex` parameter.
  - **One header, one source fewer**: `character_extras.h/.cpp` merged into `character.h/.cpp`,
    `character_info_engine.cpp` is `character_update.cpp`, `Common/game/character_info.{h,cpp}` are
    `character_def.{h,cpp}`.
- **The batches:**
  1. `prep`, three `[rename]`s: `CharacterExtras::flags` to `RuntimeFlags` (11 places, found by type),
     `GameDef::chars` to `Characters` (186), the engine's `CharacterInfo` to `Character` through an alias
     (546; the alias was needed in six headers, two more than the forward declarations).
     `compare_objects.py`: mechanical, 672 objects identical.
  2. `class`: `class Character : public CharacterInfo, public CharacterExtras`, the live list, `chex` folded
     away; 27 files. Two methods' parameters would have shadowed members (`SetLockedView`, `RunFrameEvent`:
     MSVC does not warn). The agent listed every `GameDef &` in the engine and every `GameDef` method that
     names the characters: all run before the live list is built. One difference in moment: the definition
     part of a live character is cleared at the `Characters.clear()` of `unload_game`, earlier than the
     `GameDef` reset that cleared it before; nothing reads a character in between.
  3. `fields`: the thirteen fields to the state, `CharDataEx` into the definition; 9 files. Inside `Character`
     the name `BlendMode` is the member, so the type is written `AGS::Common::BlendMode` there.
  4. `names`: the two type names (a whole-word replacement of the snapshot before it, shown by a script, but
     two history comments rewritten), then the Common move and the engine merge and move
     (`compare_objects.py`: "3 objects moved, code identical; 15 functions moved between objects, 0 changed").
  5. `fieldnames`, PLAN.md 4.3 for the two structs: 78 fields by `Script/rename_fields.py` with the map
     `.compare/helpers/51a1/fieldnames.tsv`, 37 files, no refusal, nothing left "by hand", a second run
     "nothing to do"; two lines under `#if (AGS_PLATFORM_DEBUG)` in `draw.cpp`, which no Release unit
     compiles, were converted by hand from the tool's list; 20 comment lines follow.
     `compare_objects.py`: mechanical, 671 objects identical. The names are the script properties' where one
     exists; decided with the user: `AnimWait` (was `wait`) and `WalkAnimWait` (`animwait`), `Zoom` (not the
     script's `Scaling`: room objects have the field too), `TintLevel` and `TintLight` (`tint_light` is also
     the light level); `ID`, not `Id` (29 public members against 2).
  6. `trim` (2.10b item 3): `game_state.h` included the character header and named nothing of it, so every
     user of a context got it; `inv_window.h` and `room.h` included the definition for a parameter type;
     `game_def.h` forward-declares `CharacterDef` now. First-party translation units that see
     `game/world/character.h`: 104 to 34 of 451; `game/character_def.h`: 119 to 40. Two sources gained the
     include they had relied on (`cc_character.cpp`, `agsplugin.cpp`), three in Common for `game_def.h`'s.
  7. `cleanup` and `runners`, by the supervisor: comments of the merged header, `tidy_diff.py`'s reserved
     names (`chex` gone); `compare_builds.py` and `ci/corpus/run.py` pass `--windowed --background`.
- **`Script/rename_fields.py`** (new, 1,342 lines; Opus 5.5 in a worktree): renames data members by a map of
  `Struct::old<TAB>New`, finding each reference through libclang's Python bindings (`clang==19.1.7` with
  Visual Studio's `libclang.dll`; the venv is `.compare/renameenv`) over `compile_commands.json`, the units
  chosen by `clang-scan-deps` closure. It refuses a new name that a parameter would capture, that would hide
  a type used in a member function, or that a sibling base has; lists references in macro bodies, dependent
  ones in templates, comments, and code no unit compiles; `--self-test` has 32 checks. `clang-rename` is in
  the same LLVM directory and was not used: no collision checks, silent about macro bodies, no report. Its
  trial on six awkward fields (`x`, `flags`, `view`, `name`, `defview`, `room`): 449 references in 26 files,
  the tree built with no hand edit, and a token-level diff showed nothing else changed. Limits: template
  instantiations are not walked; token pasting is not found.
- **Measured, MSVC x64 `/O2` without `/GL`** (instructions and bytes of 15 objects, `prep-alias` against
  `fields`; nothing timed; scripts in the session's scratch directory, method of 5.1 steps 2 and 3):
  - a character is 752 bytes before and after (392 + 360 in two arrays; 376 + 232 + the 144-byte graphic
    space in one object);
  - the per-character functions have the same or fewer instructions: `UpdateCharacterMoving` 184 to 170,
    `UpdateCharacterAnimating` 185 to 176, `UpdateCharacterTurning` 112 to 106,
    `prepare_characters_for_drawing` 149 to 140, the loop of `update_character_move_and_anim` 37 to 35 (one
    pointer stepped where two were); `Character::UpdateEffectiveValues` 11 to 12 is the one growth;
  - no loop gained a load of a context pointer; about 68 `Character_*` script functions lost theirs (no
    lookup of the extras by index);
  - the state's fields sit at offset 376 and up, so their accesses take a 4-byte displacement where 1 byte
    did: more code bytes in those functions (`UpdateCharacterAnimating` +72, `construct_char_gfx` +46), not
    more instructions. `character.cpp` -3.6% bytes, `character_update.cpp` +2.0%;
  - `game_init.cpp` +3,900 instructions: the copy constructors and vector code that build the live list at
    game load.
  The four `Character_LockView*` script functions grew by about 45 instructions each (a changed inlining
  decision, not traced). GCC was not compared.
- **Compile time, MSVC** (BASELINE.md "After 5.1a step 1"; `ec0098a2a` against `7347023cb` in worktrees,
  before, after, before, after, on an idle machine with nothing beside it): total 1,687.6 and 1,658.5 s
  against 1,703.4 and 1,652.9 s (+0.3% of the averages), median per object 2.59 and 2.58 against 2.63 and
  2.56 s (+0.3%), wall-clock 202.7 and 198.7 against 204.3 and 197.9 s. The readings of one side differ by
  1.7% and 3.0%, so there is no measurable change: the class made `character.h` heavier and the trim took it
  out of 70 translation units. GCC was not measured.
- **Verified:** per batch, the MSVC build, the header check and `ctest` (1,218). The auto-test game, 2414 of
  2414 on Software, Direct3D 9, Direct3D 11 and OpenGL, on the binaries of `class` and again of `fields`
  (the second run windowed and in the background: the same result). `prepush.py` with every stage on the
  finished tree: `format`, `helpers`, `layers`, `buildlists`, `build`, `headers`, `libcxx` (340 headers),
  `emscripten` (`ags.wasm` linked) and `linux` (1,218 tests) passed; `tidy` had one finding, a C-style cast
  on a changed line of `IAGSEngine::GetCharacter` (a `reinterpret_cast` now); `msbuild` failed on stale
  objects, below. After the two fixes `format`, `helpers`, `tidy` and `msbuild` (5 solutions, the Editor's
  NUnit tests) pass; the other stages were not rerun for a cast, comments and scripts. `compare_objects.py`
  from `fieldnames` to the finished tree (the trim, the comments, the cast): no code generation difference,
  so the auto-test run on the binaries of `fields` stands for the finished code.
- **Found on the way:**
  - **A renamed engine source broke the local `msbuild` stage.** `Engine.Full.Test` links every object in
    `Engine.App`'s directory and `prepush.py` keeps its MSBuild output between runs, so `character_extras.obj`
    and `character_info_engine.obj` joined the link with the old symbols (10 unresolved). CI builds from
    nothing and does not see it. `prepush.py` now deletes the objects that no source of `Engine.App.vcxproj`
    gives before it builds.
  - `game_state.h` had carried an include of the character header that nothing in it used, probably since
    upstream's file.
  - `def` cannot be a reserved name for a definition: older code uses it for a default value
    (`CfgReadInt(..., int def)`). STYLE.md recommends it without enforcing it.
- **Not verified:** the GCC Release comparison of the linked engine and GCC's compile time; sanitizers; that a
  real game's save and game file are byte-identical before and after (the save round-trip tests, the tools'
  tests and the auto-test game pass; the code was read); the two debug-only lines of `draw.cpp` (the MSBuild
  Debug jobs of a full CI run compile them); the Xcode project's entries; macOS, iOS, Android; the corpus
  game.
- **Lessons:** renaming the base's member first and letting the derived class hide it converts every use
  site for free and makes the wrong access a different type; a rename of a type is proved as a word
  replacement of its parent, a rename of fields by objects; a tool that asks the compiler what a name refers
  to costs an afternoon once and makes hundreds of `x` and `flags` safe; a test program that links a
  directory's objects by wildcard needs that directory cleaned when a source is renamed.

### 5.1a step 2: the room's entities

*Definition:* PLAN.md 5.1a step 2: `RoomObject` and the room entities as definition, state and live class;
`RoomStatus` to `RoomState`, `RoomData` to `RoomDef`; the object's copies of its definition's fields removed;
the field groups it shares with the character declared once; with 4.3 and 5.9 step 5 for these structs.

**Status (2026-11-16): built, verified and committed, not pushed** (7 code commits and one of documents,
cut by `.compare/helpers/51a2/commit_51a2.py`: the user asked not to split excessively, so adjacent
snapshots are grouped by theme and a rename's own proof is in the message). One snapshot per sub-step,
`refs/wip/51a2/*` (35 of them, in order: `names-types`, `names-common`, `names-engine`, `net-extract`,
`net-tests`, `net-fix-id`, `net-fix-region`, `net-fix-walkbehind`, `fix-assert`, `objnames`, `types-sprite`,
`types-baseline`, `types-blocking`, `object-class`, `object-list`, `entities-wbtype`, `entities-class`,
`entities-comments`, `prepush-fixes`, `fieldnames`, `faceratio`, `parts`, `names2-rename`,
`names2-showplayer`, `names2-idlebool`, `names2-charxyz`, `names2-fix-destination`, `tables-diff`,
`tables-aggregate`, `tables-rows`, `tables-tests`, `trim`, `role`, `movestate-predicate`, `movestate-uses`);
102 files, of them 82 under `Common`, `Engine`, `Tools` and `Editor`. Designed and supervised by Fable 5.2
(`.compare/briefs/brief-51a2-*.md`); the batches by Sonnet 5.5 (`names`, `objnames`, `fieldnames`, `names2`,
`trim`, `movestate`, the code comparison, the pre-push fixes) and Opus 5.5 `high` (`net`, `types`, `object`,
`entities`, `parts`, `tables`, `role`), each built once per batch in `.tidy/dev`. Closed on 2026-11-17
("5.1a step 3" below): the auto-test game (2414 of 2414 on four renderers), the compile-time reading (total
+1.2%, median -0.6%) and the order of `RoomObject`'s members. **Open:** the GCC comparison of the linked
engine.

- **The design, where it was decided here** (RUNTIME-SPLIT.md 2, "What 5.1a step 2 built"):
  - **Two lifetimes in one live entity.** A definition field of a room entity is either kept by the room
    state between visits and saved, or given again by the room file at every load. `BindRoomState` (extracted
    from `load_new_room` and `init_object_states`, so that a test can call it) is the one place that says
    which; each live class has `RebindDefinition`. Nothing of what is kept or saved changed, with the one
    exception the user decided (the face ratio, below).
  - **The live lists hide `RoomDef`'s**, as `Game::Characters` hides `GameDef::Characters`: `Room::Objects`
    and the four others point at the bound state's lists, `Room::Def()` reads the file's values. Proof that
    the engine no longer writes the definitions: a scratch build with `RoomDef`'s lists `const`, 0 errors in
    the engine.
  - **A rectangle stays one value** (the user): the object keeps `Rect BlockingRect` of its definition and its
    four `blocking_` copies go; the character keeps four fields; which shape both take is marked for a later
    review in PLAN.md.
  - **A fourth field role, `FromDefinition`** (the user, on the finding of the batch `tables`): the 24 rows
    of the room's definitions that the file gives again had borrowed `Derived`.
  - **Names** (the user, STYLE.md "Naming"): a field is named for what it does (`AnimationDelay`,
    `BgAnimDelay`, not "speed"); a boolean takes `Is`/`Has`/`Needs`, or starts with its verb
    (`ShowPlayerCharacter`, `ProcessIdleThisTime`), whatever type holds it, and is named for the positive
    sense, the stored value turned round at the file boundary; the script's names do not follow.
- **The batches:**
  1. `names`: the Common types (`RoomData`, `RoomObjectInfo`, `RoomHotspot`, `RoomRegion`, `WalkArea`,
     `WalkBehind` to `RoomDef`, `Room...Def`), `RoomStatus` to `RoomState`, `roomdata.*` to `room_def.*`,
     `room_status.*` to `room_state.*`. Proved as a whole-word replacement of the parent tree by a script, and
     by `compare_objects.py` ("3 moved, code identical").
  2. `net` (ground rule 3): `BindRoomState` extracted, then `Engine/test/room_state_test.cpp`: first visit,
     later visit, a restored state, the object's save round trip, region light and tint, walk-area scaling.
     The extraction first returned `!been_here`, which differs from the old test for a negative `beenhere`;
     made exact by the supervisor. Three defects found by the tests and fixed, each its own commit (below).
  3. `objnames`, `fieldnames`, `names2-rename`: PLAN.md 4.3 by `Script/rename_fields.py` (38 fields of the
     object, 14 of `RoomState`, 6 more after the naming rules), each `compare_objects.py`: mechanical (674,
     674 and 672 objects identical; the test files differ by `__LINE__`).
  4. `types`, one commit per field with a test at the limit: `Sprite` and `Baseline` take the definition's
     32-bit type and narrow explicitly where the 16-bit copy did implicitly; the blocking rectangle.
  5. `object`, `entities`: the classes, the state structs, the copies gone, the hiding pointers, region light
     and tint and walk-area scaling off the definition. `Room::WalkableAreasOn` stays in `Room`.
  6. `faceratio`: a behaviour fix the user decided, below. `parts`: the four shared structs
     (`entity_state.h`), 24 of 26 fields; `AnimWait` takes `int` and the object narrows explicitly, pinned at
     the limit; `MoveListID` defaults to 0 in the part and `RoomObject`'s constructors assign the object's -1.
     `sizeof` unchanged (`Character` 752, `RoomObject` 608).
  7. `names2`: `ShowPlayerCharacter` (was `PlayerCharacterOff`; turned round at the room file read and write,
     a test pins the byte), `ProcessIdleThisTime` as `bool` (the save byte read as `== 1`, as its one reader
     tested), `Character.X`, `Y`, `Z` in `agsdefns.sh` beside the lowercase ones (the engine has registered
     both spellings since before the fork point).
  8. `tables` (5.9 step 5): `DiffFields` descends into a base or a member that has a table; the definitions
     as aggregates (`RoomObjectBase` is `ID` and `ScriptName`, each definition with events declares `Events`,
     `RoomDef` no longer derives from it); 11 tables; one test per entity type from the rows, which replaced
     87 lines of hand-kept mirror structs. `role`: `FromDefinition`, `AlterFields(object, role)`.
  9. `trim` (2.10b item 3): first-party translation units that see `game/roomstruct.h` 118 to 30,
     `room_object.h` 35 to 19, `room_state.h` 23 to 19, `room_def.h` 133 to 127 (the `MAX_ROOM_*` constants
     keep it in `game_context.h`).
  10. `movestate` (the user): `MoveState::kNoMoveList` and one `IsMoving()` in the shared part; the two
      `is_moving()` copies and every comparison of `MoveListID` with a literal are gone.
- **Defects found and fixed, each with a test seen failing** (the first three and the fifth are in upstream
  `ags4` too: candidates for a PR):
  - `SetAreaLightLevel` accepted a region number equal to the count (`>` for `>=`).
  - `get_walkbehind_pixel` read the walkable-area mask.
  - `Object.DestinationX` and `DestinationY` of an object that has not moved since the room's first visit
    dereferenced null: the object starts with `MoveListID` -1 and the two functions tested it for non-zero
    (an access violation in the test before the fix).
  - A restored room object had id 0 until the room was entered (`RoomState::ReadFromSavegame` now sets it).
  - **`WalkableArea.FaceDirectionRatio` set by a script was lost** on leaving the room and on restoring a
    save, although the room state's save has a slot for it (the room's and the character's ratio are kept).
    It is a kept field now. A save of an older engine holds in that slot the first visit's copy of the room
    file's value, so it restores as before unless the room file changed since.
  - A stale `assert(ch->walking > 0)` from step 1 (asserts are compiled away in the builds the rename tool
    sees).
- **Defects found and recorded, not fixed** ("Open items" below): the object's frame offsets after a
  restore, the room file writer's uninitialised stand-in image, `walkbehinds_cropout`'s unchecked mask index,
  `Walkarea_GetScalingMin/Max`, and three in the save code.
- **Measured, MSVC x64 `/O2`** (`objnames` against `prepush-fixes`; nothing timed): the per-frame functions
  that touch a room object gained 0 to 4 instructions (one pointer load); `sizeof(RoomObject)` 336 to 608 and
  `sizeof(RoomState)` 7,136 to 30,608, since the live entities hold their definition part. A reordering of
  `RoomObject`'s members (the hot fields span about four cache lines, two before) waits for a timing run.
- **Verified:** per batch, the MSVC build with the header check and `ctest` (1,218 to 1,266 tests).
  `prepush.py --auto --emsdk` three times: after `entities` (found a GCC link error on an in-class `static
  const`, 61 tidy findings and a stale comment reference: fixed in `prepush-fixes`), again (all 11 stages),
  and on the finished tree: `format`, `helpers`, `layers`, `buildlists`, `build`, `headers`, `libcxx` (342
  headers), `emscripten`, `linux` (GCC, 1,266 tests) and `msbuild` (5 solutions, the Editor's NUnit tests,
  `agsnative.cpp`) passed; `tidy` had one finding, the constant's name (`kNoMoveList` now).
- **Not verified:** the GCC Release comparison; sanitizers; macOS, iOS, Android, the Xcode project's two new entries; a real game's
  saves before and after (the round-trip tests pass); the corpus game.
- **Lessons:** the tests written before the surgery found five defects that had nothing to do with it; a
  hand-kept mirror struct with a `sizeof` assert is a fair stand-in for a table for a few batches, and goes
  when the table comes; MSVC accepts an odr-used in-class `static const` that GCC does not link (`constexpr`);
  a predicate that exists twice is bypassed about thirty times: one definition, and no literal left to
  compare with.

### 5.1a step 3: the plain entities

*Definition:* PLAN.md 5.1a step 3: the inventory item, the mouse cursor, the audio clip and its type, the view,
the dialog, the options and the sprite table as definitions with live copies in `Game`; with 4.3 and 5.9 step
5 for these structs, and step 2's open items (the auto-test game, the order of `RoomObject`'s members).

**Status (2026-11-17): built, verified and committed, not pushed** (`b635a47d7` for the first three batches,
then `449e40175`..`35c6d5a8f`, 7 commits cut from the snapshots `refs/wip/51a3/*` by
`.compare/helpers/commit_snaps.py`). Designed and supervised by Fable 5.2 in a session the user left
unattended (`.compare/briefs/brief-51a3-*.md`); the batches by Sonnet 5.5 (`names`, `lists`, `headers`,
`fieldnames`, `trim`) and Opus 5.5 `high` (`net`, `live`, `sprites`, `tables`, the layout benchmark), one at a
time in the main tree, each built once per batch in `.tidy/dev`.

- **The design** (RUNTIME-SPLIT.md 2, "What 5.1a step 3 built"): every list the game changes has a live copy
  in `Game` that hides the definition's by name, so no use site changed its text; the live types are aliases
  of their definitions until they hold more (the user, 2026-11-16); `Game::SpriteInfos` is the live sprite
  table and the cache is attached to it (the user: "table in Game now").
- **The batches:**
  1. `names`, `net`, `lists` (2026-11-16, in `b635a47d7`): the ten type names, the engine's aliases,
     `DialogOptionsState`; `Engine/test/game_entities_test.cpp` with the fixture `GameEntities`; `GameDef`'s
     lists and values under the names both copies have.
  2. `headers`: a room object starts at `MoveState::kNoMoveList` (the user: one constant, so that changing the
     value is redefining it), and the move list table names the constant; `Engine/game/anim_flow.h` and
     `Common/game/room_limits.h` split out. Translation units that see `room_def.h`: 128 to 46.
     `compare_objects.py` on the two moves: mechanical (642 identical; 2 objects lost an unreferenced
     `std::locale::id` constructor).
  3. `live`: `StartFromDefinitions`, the live lists, options and values; the save code takes `Game &`; two
     readers that run before the game starts read `Def().Options`; the fix-ups in the loader; `GameInfo` read
     without `operator[]`. The scratch build with 14 members `const`: no error in the engine.
  4. `sprites`: the live sprite table. The order of events at start was read first: `init_sprite_cache`
     (attaches the still empty live table), the loader (writes the definition), `StartFromDefinitions` (the
     copy), `EnlargeTo`, then the sprite file's sizes; nothing touches the table between the first and the
     third.
  5. `fieldnames`: 74 fields by `Script/rename_fields.py`, no refusal; `compare_objects.py`: 671 objects
     identical, no code generation difference. `Editor/AGS.Native`: 49 lines by hand.
  6. `tables`: ten tables with the count check; the structs made aggregates (three constructors now generated
     inline where the structs are built, load-time code only); five "every saved field is restored" tests
     from the rows; with six roles flipped on purpose all five fail and name the field.
  7. `trim`: `game_def.h` drops three includes (about 90 translation units lose each).
- **Visible changes:** a save holds 0 where it held -1 as the move list of an object that has not moved (a
  restored -1 still means "not moving"); a cursor whose view is past the game's views has none from the
  loader on, for the tools too; `GameChannelCount` is 32 from the start, where it was 0 until the game was
  initialised (nothing read it then).
- **Found, not changed** ("Open items" below): `GameDef::IsSpriteValid` has no caller;
  `AudioClipDef::IsRepeatingByDefault` is a `char`; `CharacterDef::ScriptName` is a `Saved` row that no save
  holds; two includes of `game_def.h` that a small move would free.
- **The order of `RoomObject`'s members** (step 2's open item): measured, and left as it is. A benchmark
  (a disabled test injected into `engine_full_test`, kept local in `.compare/helpers/layout/` with the
  candidate patch and `run_bench.py`) makes a room of 256 live objects and runs the engine's own per-frame
  update over it (`update_cycling_views`, `update_object_scale`), warm and after the cache was flushed, in
  three mixes; a synthetic loop reads the fields the draw preparation reads (the real one needs a graphics
  driver). The candidate order puts the definition's hot fields first and the bounding box before the
  matrices of `GraphicSpace` (one cache line fewer per object for the scale update, 1.5 fewer for the draw
  read; sizes unchanged, 608 and 752 bytes). MSVC x64 Release on an idle machine, pinned to one core, five
  alternated runs, run-to-run spread 0.7 to 2.5% warm:

  | Per pass over the room | as it is, warm / cold | candidate, warm / cold |
  |---|---:|---:|
  | update, all idle | 49.9 / 66.2 µs | 50.3 / 65.6 µs (+0.8% / -0.9%) |
  | update, all animating | 56.4 / 75.9 µs | 57.1 / 76.4 µs (+1.2% / +0.7%) |
  | update, half the objects off | 25.3 / 40.5 µs | 25.6 / 36.5 µs (+1.4% / -9.9%) |
  | draw read (synthetic), all idle | 1.38 / 9.1 µs | 1.32 / 8.6 µs (-4.6% / -5.5%) |

  A full room's update is 0.3 to 0.45% of a 16.7 ms game loop and is computation (the two transforms of
  `UpdateGraphicSpace`, the walk-mask lookup), about 200 ns per object. Everything memory costs is the gap
  between cold and warm, 15 to 20 µs per pass (0.1% of a loop): that bounds what step 2's growth from 336 to
  608 bytes can cost, and the reorder moves it by at most 4 µs one way (half the objects off) and half a
  microsecond the other (warm, in every run). No case for reordering; GCC was not measured.
- **Verified:** per batch, the MSVC build with the header check and `ctest` (1,283 to 1,305 tests). On the
  finished tree: `prepush.py --auto --emsdk`, every stage (`format`, `helpers`, `layers`, `buildlists`,
  `build`, `headers`, `libcxx` with 344 headers, `emscripten`, `linux` with GCC and 1,305 tests, `msbuild`
  with 5 solutions and the Editor's tests); `tidy` had 5 findings on renamed lines, fixed (`35c6d5a8f`) and
  rerun clean. The auto-test game: 2414 of 2414 on Software, Direct3D 9, Direct3D 11 and OpenGL, at
  `b635a47d7` (which also closes step 2's) and on the finished tree.
- **Compile time** (BASELINE.md "After 5.1a steps 2 to 4"; MSVC, an idle machine, two readings a side that
  agree within 0.1%): against the tree after step 2, total +0.6% (1,674.8 to 1,684.3 s), the median
  unchanged at 2.55 s per object, `Engine/game` -1.7%; the new test source alone compiles in 11.9 s, more
  than the whole difference. **Step 2's own reading**, which was open, comes from the same run: total +1.2%
  (1,655.1 to 1,674.8 s), median 2.57 to 2.55 s, `Engine` +2.5%, wall-clock 198.8 to 200.6 s.
- **Not verified:** the GCC Release comparison of the linked engine; sanitizers; macOS, iOS, Android, the
  Xcode project's new entries; a real game's saves before and after (the round-trip tests pass); the corpus
  game.
- **Lessons:** a live copy that hides its definition by name moves hundreds of uses without touching them,
  and the compiler proves the rest (`const` in a scratch build); what runs before the copy exists must be
  looked for by hand (two readers of the options, found by reading the start order, not by the build); a
  role has to say what a restore does, and for a list of structs that is the elements' business.

### 5.1a step 4: what was left of `GamePlayState`

*Definition:* PLAN.md 5.1a step 4: the per-object custom property lists go to the live objects; the rest is
the game's own state, renamed `GameState`, the blocking-loop base class of that name renamed with it; with
4.3 and 5.9 step 5 for the struct.

**Status (2026-11-17): built, verified and committed, not pushed** (`532891144`..`0eda178d5`, 9 commits, cut from the snapshots
`refs/wip/51a4/*` by `.compare/helpers/commit_snaps.py`). Designed and supervised by Fable 5.2
(`.compare/briefs/brief-51a4-*.md`), partly while the user was away; the batches by Opus 5.5 `high` (`net`,
`props`, `tables`) and Sonnet 5.5 (`names`, `fieldnames`, `trim`, `positive`).

- **The design** (RUNTIME-SPLIT.md 2, "What 5.1a step 4 built"), with what was decided here:
  - The play values of a custom property are a member of the live object, **`PropertyOverrides`** (the
    supervisor's name, told to the user: it is what the lookup does with them, and it leaves `Properties` to
    the definition's values, as the room's definitions have it).
  - **The GUI's two lists stay in the state** until the GUI has live classes (step 5); the definition's six
    lists stay in `GameDef` (renamed); the room's lists stay in `RoomState`.
  - **The three script switches with negative names stay the script's `int`** (the user, 2026-11-17, after
    seeing both versions side by side): `DisableAntialiasing`, `DisableDialogParser`, `NoMultiLoopRepeat`,
    with positive predicates for the engine's logic (`ShouldAASprites()`, `IsDialogParserEnabled()`,
    `ShouldRepeatMultiLoop()`). The ruling of 2026-11-16 had made them positive `bool` fields; that costs two
    inversions per switch at the script and at the save, and a script that wrote 5 would read back 1.
    `NoMultiLoopRepeat` has a `CHECKME`: only a script can set it (no game option, no Editor setting;
    upstream `ags4` has it too; neither private game nor the test game names it).
  - **A fifth field role, `Transient`** (the supervisor's, told to the user): play state that a save does
    not hold and a restore does not give back. 15 rows of `GameState` have it; no other table yet.
- **The batches:**
  1. `net` (ground rule 3): 19 tests; the fixture `GameEntities` gains GUIs with controls and a property
     schema. The property functions of the eleven kinds, their save round trip and bytes, the three
     switches at the script and in the save, the state's bit fields, the script timers.
  2. `props`: `CharacterState::PropertyOverrides`; `InventoryItem`, `Dialog` and `AudioClip` become classes
     of a definition and a state struct; playback takes `const AudioClipDef *` and the playing clip is found
     by id; `FreePropertyOverrides` before a restore. Save bytes unchanged.
  3. `names`: `GamePlayState` to `GameState`, the loop base to `GameLoopState`; proved by tokens (10 and 53
     replacements in 18 files, nothing else).
  4. `fieldnames`: 143 fields of `GameState` and `GameDef`'s six lists by `Script/rename_fields.py`, no
     refusal; 58 declarations name the script's variable or API property; `compare_objects.py`: 673 objects
     identical, no code generation difference.
  5. `trim`: `Engine/game/game_loop_state.h`; `game_state.h` no longer includes `speech.h` (107 to 22
     translation units), `move_list.h` (118 to 53), `graphicsdriver.h` (112 to 36). `compare_objects.py`: no
     code difference.
  6. `positive`: the three switches named and asked through predicates. `compare_objects.py` (MinSizeRel):
     the two predicates emitted as functions, one caller changed (`DialogOptionsState::Begin`), the
     per-frame reader unchanged.
  7. `tables`: `GameState` an aggregate (constructor gone, the eight private members public under plain
     names, used through their methods), 154 counted rows (134 `Saved`, 5 `Derived`, 15 `Transient`);
     `field_table.h` gains `std::array`, `std::unordered_set`, a time point, and an integer alteration that
     steps down at the type's maximum; `SavegameContent.EveryGameStateFieldIsRestored` (four exclusions,
     each checked to be still needed); three methods read `this`.
- **Defects found.** Fixed, with its test seen failing first (also in upstream `ags4`): **a restore kept
  the GUI controls' custom property values set after the save** (`FreeProperties` did not clear their
  lists). Recorded, not fixed ("Open items"): a restore appends the saved parsed words (a disabled test);
  `OverlayCount` after restoring a save that had overlays (not tested); the transient fields that a restore
  leaves from the game played before.
- **Visible changes:** the fix above; `InventoryItemWidth` is 0 instead of indeterminate between a `Game`'s
  construction and `engine_init_game_settings` (which sets 40).
- **Sizes** (MSVC x64): `Character` 752 to 824, `InventoryItem` 216 to 280, `Dialog` and `AudioClip` 72 to
  136 (one map each).
- **Verified:** per batch, the MSVC build with the header check and `ctest` (1,305 to 1,330 tests); a
  syntax check of the counted 154-row table with GCC 8.3 in the builder image. On the finished tree:
  `prepush.py --auto --emsdk`, every stage (`libcxx` with 345 headers, `emscripten`, `linux` with GCC and
  1,330 tests, `msbuild` with 5 solutions and the Editor's tests). Its first run found three `tidy` findings
  on changed lines and, with GCC only, a byte test that compared two saves of property maps with two values
  (the hash map's order, "Open items"); both fixed, the second in the commit that adds the test, and
  `format`, `build`, `tidy` and `linux` rerun clean. The auto-test game: 2414 of 2414 on Software, Direct3D
  9, Direct3D 11 and OpenGL.
- **Compile time** (BASELINE.md "After 5.1a steps 2 to 4"): against the tree after step 3, total +1.2%
  (1,684.3 to 1,705.0 s), median 2.55 to 2.58 s per object, `Engine/game` +0.8%, wall-clock 202.0 to
  203.8 s. Over steps 2 to 4 together: total +3.0%, of which more than half is two new test sources;
  `Engine/game` -0.6% in total and -3.7% in the median, the header trims outweighing what came into the
  headers.
- **Not verified:** the GCC Release comparison of the linked engine; sanitizers; macOS, iOS, Android, the
  Xcode project's new entry; a real game's saves before and after (the round-trip and byte tests pass); the
  corpus game.
- **Lessons:** a question put to the user with a recommendation is not yet a question they can answer: the
  three switches were settled only when both versions were shown as code, and the answer reversed an
  earlier ruling; a table for a 150-field struct pays at once (one test replaces a hand-written check of 16
  fields and names what a restore leaves behind); a role that is missing shows as rows nobody can label.

### 5.8 step 1: Common's save code into the engine

*Definition:* PLAN.md 5.8 step 1: the `...Savegame` methods of six `Common/game` files go to
`Engine/game/save/savegame_components.cpp`, first as they are (a `[move]`), then as functions of that file with
their declarations out of the Common headers. *Done when* (of step 1): no `Savegame` method is left in
`Common/game`.

**Status (2026-11-15): done**, 4 commits (`c61a37714` tests, `413a9ee3e` `[move]`, `c91c001cc` free functions,
`970aa19cf` `[move]` of the enums), not pushed. By Opus 5.5 alone, no subagents: each step was one script over a few known places.

- **Tests first** (ground rule 3): the save tests covered characters and inventory items only. Added to
  `Engine/test/savegame_components_test.cpp`: `DialogOptionsAreRestored` (flags of every option, the text only of
  one with `DFLG_TEXTSET`, the custom properties), `SaveWithFewerDialogOptionsIsRefused`,
  `MouseCursorsAreRestored`, `AudioTypesAreRestored` (the reserved channels are game data and stay),
  `GameSetupIsRestored` (options 0 to 39 and `OPT_LIPSYNCTEXT`, the player, the dialog bullet, the inventory
  cursor's marker colours and sprite, the default lip-sync frame). They pass before and after.
- **The `[move]`** (`.compare/helpers/move_58s1.py`, the 13 definitions verbatim before the file's namespaces):
  `compare_objects.py` says "NOT a purely mechanical change" for a reason it does not model: `tools_test`
  compiles five of the six Common sources itself, so 11 methods (and 6 unwind funclets) had two copies before and
  have one after. All 17 are listed as removed with the same code before and after (the script calls a
  difference of code "changed", and there is none); the two `GameSetupStruct` methods, with one copy before, are
  "moved". The tools stop compiling the save code, as the task meant.
- **Free functions** (`.compare/helpers/free_58s1.py` and `free_58s1_block.cpp`): `ReadCharacterInfo`,
  `WriteCharacterInfo`, `ReadInventoryItemInfo`... named after the record type, since `Tools/data/
  data_file_writer.cpp` has a `WriteInventoryItem` of its own; in an anonymous namespace before the component
  code. `CharacterInfo::LegacyFields` is the file's `CharacterLegacyFields`; the unused version parameters of
  the cursor and dialog option readers are gone. Three includes the moved code alone used leave
  `character_info.cpp` and `inventory_item_info.cpp`.
- **The save version enums leave Common** (`.compare/helpers/enums_58s1.py`, a `[move]`, objects identical):
  `CharacterSvgVersion` to `game/world/character_extras.h`, `GameStateSvgVersion` to `game/game_state.h` (both
  had a TODO saying so), `InvitemSvgVersion`, `DialogTopicSvgVersion` and `CursorSvgVersion` to
  `savegame_components.cpp`, their only user. At global scope, so no symbol changes. The dialog enum's comment
  said "RoomStatus runtime save format", a copy slip.
- **Verified:** `prepush.py --auto`, every stage passes except `tidy`, which found nine names on changed lines
  (CamelCase members for the test's `GameSetupFields` and `CharacterLegacyFields`, an unused `cmp_size`);
  after the fixes `format`, `build` (with `ctest`) and `tidy` pass again, the other stages were not rerun for
  three renames and a commented parameter name. `linux`: 1216 tests. Auto-test game 2414 of 2414 on Software,
  D3D9, D3D11 and OpenGL. The fixes are in the commits they belong to (the `refs/wip/5.8s1/*` snapshots were
  patched with `.compare/helpers/resnap_58s1.sh` before the commits were cut from them).
- **Left in Common:** the eight `Common/gui` classes' save methods and `GuiSvgVersion`, until 5.1a step 5 (private
  members). No file was added; the engine's and Common's file counts are unchanged.
- **Lesson:** the working tree is CRLF and the repository LF; a script that writes files must keep each file's
  line ending (the first run of `move_58s1.py` left one lone CR and git saw the whole file as rewritten).

### 6.7a The old-game import deleted

*Definition:* PLAN.md 6.7a: delete the old compiled-game import (`ImportOldGameFile`, `import_compiled_game_dta`,
`load_dta_file_into_thisgame`, `init_game_after_import`, the SCI font import) and its menu entries, so that 5.1
step 3 and 5.1a step 5 do not have to keep it building.

**Status (2026-11-15): done**, 1 commit (`79f6369fc`), not pushed. By Opus 5.5 alone.

- **It was dead code already.** Upstream removed the menu entries, the wizard page and `OldGameImporter.cs` in
  `a39e6cfc6` ("Editor: remove support for importing 2.* games", 2025); the SCI font import lost its last caller
  in upstream's Fonts/FontFiles split (`3a3b96dfb`). Nothing called what was left, so the task deletes and
  changes no behaviour; no test was needed first (ground rule 3 is about code that runs).
- **C#:** `NativeProxy.ImportOldGame`, `ImportSCIFont`, `LoadSpriteDimensions`; the `ImportedOldGame` event,
  its raiser, handler and `ComponentController.NotifyImportedOldGame`; `ImportExport.ImportOldEditorDatFile`
  (the 2.72 `editor.dat` reader) with `ReadGlobalScriptAndScriptHeader`, `ImportRoomList`,
  `GetRoomNumberFromFileName`, `ImportSpriteFolders`, the fixed-length string reader and
  `EDITOR_DAT_LATEST_FILE_VERSION`. `ReadNullTerminatedStringAsBytes` loses its fixed-length parameter, which
  only the sprite-folder reader used.
- **Native:** `NativeMethods::ImportOldGameFile`, `ImportSCIFont`, `LoadAllSpriteDimensions`; in
  `agsnative.cpp` the functions above, `load_sprite_dimensions`, `free_old_game_data`, the old-game globals
  (`dialog`, `guis`, `newViews`, `thisgamePlugins`), `loaded_game_file_version` and `game_compiled_version`
  (only the import set them; Common does not use them), the unused `GetCharacterScriptName`, and three includes
  (`dialog_topic.h`, `main_game_file.h`, `plugininfo.h`). The GUI control vectors stay: the GUI preview uses
  them. `LoadRoomScript` stays: the rooms component uses it. 920 lines out; `thisgame` is named on 73 lines of
  `agsnative.cpp` instead of 189 (fonts, sprites, palette, GUI preview).
- **Docs:** 6.7c and EDITOR-SPLIT.md no longer list a SCI import or `LoadAllSpriteDimensions` for the C library.
- **Left:** `Common/test/common_stubs.cpp` still defines `loaded_game_file_version` and `game_compiled_version`,
  which nothing in Common uses (harmless; for a Common change).
- **Verified:** `prepush.py --auto --base HEAD` picks `format` and `msbuild` for these files; both pass (5
  solutions with `AGS.Native` and the editor, the editor's NUnit tests). Not run: opening a project in the editor
  (no code path that runs changed), the other stages (no file they verify changed).

### 5.9: the design and step 1, field tables for the character

*Definition:* PLAN.md 5.9: one row per field of a struct, from which comparison, dump and the new save format
are written once. The task opened with the question the design of 2026-11-15 left: how far to take data-driven
code (what a row carries beyond name and member, whether a row drives behaviour, whether a schema file
generates the rows), to be decided against the real `CharacterDef` and `CharacterState`.

**Status (2026-11-16): designed with the user and step 1 built** (`a206fefde`, `0a8a21b54`; not pushed). By Fable 5.2 alone (the
precompiled header of 2.10b ran beside it in a worktree, Sonnet 5.5). The design is RUNTIME-SPLIT.md 6; the
steps left are in PLAN.md 5.9.

- **Decided** (four questions put to the user with the measurements below; all four as recommended): a row is
  a name, a member and a role, with no default, range or script name; no change hook, the live class
  computes its derived fields again in one function; no schema file, the rows are C++ and the compiler counts
  the struct's members against them; the script properties that only read or write a member or a flag bit
  are bound from the member in the binder, the exact forms only.
- **Named "field", not "property":** the game's custom properties own that word (`PropertyDesc`,
  `Character.GetProperty`). The plan's "property rows" are `FieldRow`s.
- **Built:** `Common/util/field_table.h` (header only: `FieldRow`, `FieldTable`, `Field`, `OpaqueField`,
  `BaseFields`, `FieldsCoverStruct`, `FieldsAreDistinct`, `DiffFields`, `PrintFields`, `AlterFields`, and
  `FieldValue<V>` for numbers, enums, `String`, points, vectors, maps, arrays and structs with a table); the
  tables of `CharacterDef` (32 rows, `character_def.cpp`), `CharacterState` (52, `character.cpp`),
  `AnimFlowParams` and `ViewAnimateParams` (`object.cpp`, beside `CycleViewAnim`). `CharacterDef` became an
  aggregate: `_events` is the public member `Events` (the accessors stay until step 2), its five defaulted
  special members are gone.
- **Measured:**
  - *The count works on the real structs* with MSVC 19.4x, GCC 13 (WSL) and Clang 20 with libc++ (emsdk), and
    a dropped row fails the build on all three. GCC 8.3 too, checked after the commits in the Debian 10
    builder image (`ghcr.io/alandrake/ags-draconian-linux-builder-image:ags4-amd64`, run with Docker in the
    WSL distro: `-fsyntax-only` on `character_def.cpp` and on a scratch struct, whole and one row short).
  - *Where the table goes:* a file that includes only `character.h` compiles in 0.85 s, with `field_table.h`
    in 0.89 s, with both tables as `inline constexpr` in 0.98 s (MSVC Release flags, the fastest of 7, on a
    machine that was building beside it: rough). 35 files include `character.h`, so the tables are in the
    source files.
  - *The script properties of the character:* 142 functions behind 152 rows; 35 return a member, 16 test a
    flag, 14 assign a member, 12 set a flag (77 exact forms); 5 return a view number plus one, 6 clamp, 5 call
    an update after the write; 46 do more. One of the 77 is called by engine code by name
    (`Character_GetDiagonalWalking`), seven by plugins through the name lookup. All types have 1,015 property
    rows.
  - *Generated against hand-written* (MSVC `/O2`, a scratch file with both): the same instructions for a
    getter, a setter and a flag getter of a `CharacterDef` member. For a `CharacterState` member (the second
    base of `Character`) `obj->*Member` and `(*obj).*Member` compile to a null test of the object and a
    conditional move before the load, four instructions more; `static_cast<CharacterState &>(*obj).*Member` is
    the hand-written `mov eax,[rcx+190h]`.
- **Found by the first table-driven test:** nothing lost. `SavegameContent.EveryCharacterFieldIsRestored` sets
  every saved field of a character (73 of the 84), saves, restores and compares: today's save code returns all
  of them. The test was then shown to fail: with `SpriteWidth` marked saved it reports `SpriteWidth: 1 != 0`.
  The hand-written `CharactersAndItemsAreRestored` checks 20 fields.
- **Seen and left:** five of the seven derived fields (`SpriteWidth`, `SpriteHeight`, `FrameOffsetX`,
  `FrameOffsetY`, `OffsetZoom`) are set by the next update of the character, not by the restore; whether
  anything reads them in between is for step 2's test. The script event tables have no comparison, so their
  two rows are opaque. The member-pointer trait exists twice (`xml_fields.h`).
- **Verified:** `prepush.py --auto`, all eleven stages (`format`, `helpers`, `layers`, `buildlists`, `build`
  with `ctest`, `tidy`, `headers`, `libcxx`, `emscripten`, `linux` with 1,226 tests on GCC 13, `msbuild` with
  five solutions and the editor's NUnit tests); the auto-test game, 2414 of 2414 on Software, D3D9, D3D11 and
  OpenGL. Not verified: the macOS, Android and iOS compilers on the member count;
  `compare_objects.py` was not run (the commits are not mechanical: `CharacterDef` lost its declared special
  members and gained a static function).
- **Helpers kept** in `.compare/helpers/59`: `classify.py` (the forms of a type's property functions),
  `cc.py` (one source with the build's own command line, timed), `cl.py`, `disasm.py`, `getter.cpp` (the
  generated accessors beside the hand-written ones), `mkprobe.py` (the header-cost probes).

### 2.10b item 2: a precompiled header of stable content, kept local

*Definition:* PLAN.md 2.10b item 2: lower the fixed cost of a file (about 2.4 s of its 2.7 s) with a
precompiled header of what does not change, CMake targets only, with the guards built without it.

**Status (2026-11-16): done.** By Sonnet 5.5 `medium` in a worktree beside 5.9 (CMake files, the scripts and
the workflows; 162 tool calls), then carried onto the tree after 5.9's commits and checked by Fable 5.2. The
user kept it local and took the readings of a busy machine as good enough.

- **Built:** `CMake/PrecompiledHeaders.cmake`, `ags_precompile_headers(<target> [ENGINE] [GTEST])`, called for
  `common`, `engine`, `libtools`, `compiler` and the five test targets; `PRIVATE`, C++ only (the C files of a
  mixed target get none), one header per target (their definitions differ, so none is reused): nine, 27 s to
  build, 540 MB. The tool executables and the vendored libraries have none.
- **The content came from counts and parse times** (`ninja -t deps` over 551 objects, `/Zs` per candidate):
  the standard headers (521 to 538 objects include them, 0.45 s), `platform.h`, `types.h`, `util/string.h` and
  `debug/assert.h` (495 to 518); glm for the engine (121 of its 248, 0.29 s); gtest for the tests (0.86 s).
  Left out: Allegro (0.04 s) and `SDL.h` (33 of the engine's 248), 2% more on a sample of ten files and their
  macros in files that do not include them; `stream.h`, `bbop.h`, `geometry.h` and the like (328 to 427), which
  change more often than they save; every game header.
- **Kept local** means: on for MSVC, and for GCC 11+ or Clang on Linux, when no compiler launcher is set. Off
  for Emscripten, Android, Apple (the Objective-C++ source), FreeBSD, GCC 8, and with ccache: MSVC's `/Yu`
  objects cannot be cached, and GCC's need ccache settings that nobody tested. `compare_objects.py`,
  `prepush.py`, `linux_jobs.sh`, `verify.yml` and `analysis.yml` pass `-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`.
  So no CI job builds with it, by choice: the alternative was to lose the Windows job's cache.
- **Two build directories where there was one.** clang-tidy cannot read the compile database of an MSVC build
  with the precompiled header (`is not a valid precompiled PCH file`, every file), and the batch builds shared
  `.tidy/build` with `tidy_fix.py`. That directory is now configured without it, and the batches build in
  `.tidy/dev` (`.compare/helpers/build_dev.py`), which has it.
- **Found: a header the file did not include changes its code.** `Plugins/ags_snowrain` calls `floor(x)` and
  `sin(x)` unqualified on floats; with `<cmath>` from the precompiled header MSVC takes the float overloads
  where it took the double ones. The built-in plugins' sources are left out of the header
  (`SKIP_PRECOMPILE_HEADERS`). After that the disassembly of all 558 objects was compared with the build
  without: 481 the same, the 77 others differ in generated names only (numbered statics and
  anonymous-namespace names in five product objects, a gtest path string in 72 test objects).
- **Found in CMake:** `$<$<COMPILE_LANGUAGE:CXX>:<vector>>` ends at the first bare `>` when the condition is
  false and gave the C header `#include ">"`; the module writes `$<ANGLE-R>`. A build directory configured
  before this change turns the header on at its next configure and rebuilds everything, unless it is given
  the switch.
- **Measured** (BASELINE.md "With the precompiled header"; all on a busy machine): compile time of the
  first-party objects -17% (MinSizeRel), the median per object 2.69 to 2.08 s, `Engine` -30%, `Common` -7%;
  twelve single compiles -32%; the rebuild after a change to `game_context.h` 44 to 49 s down to 28 to 37 s;
  the wall-clock of a clean build did not move in the one pair there is.
- **Verified:** with it, MSVC Release in `.tidy/dev`, 1,226 of 1,226 tests; GCC 13 as Release with warnings
  as errors, 1,218 of 1,218 (on the tree before 5.9's commits). Without it, on the final tree: every prepush
  stage, whose logs say "Precompiled headers: off"; `actionlint` on the two workflows. Not run: Clang with
  libc++ with it, the Visual Studio generator; the workflow edits run in CI only.
- **Seen and left:** `refactor_metrics.py --ninja-log` refuses to run in a git worktree (it looks for a
  `.git` directory, and a worktree has a file).

### Open items from finished tasks

*Moved from PLAN.md "Next up" on 2026-11-07* (details in the tasks' sections):

- 0.4: the CodeQL setting, the `D3DCOMPILER_47`/Windows 7 question and the macOS 10.9 deployment target.
- 1.4: the `agfreader` crash on a project without `<Settings>` (fixed 2026-11-12: `AGFReader::Open` refuses the
  project, `ReadFields` a missing element; test `AGFReader.OpenReportsMissingSettings` failed before the fix);
  `StrUtil::Unescape` reading past the end (gone with its rewrite in `28badf14b`, U.3).
- 1.2: the uninitialised `DataUtil::GameSettings`: 47 `bool`/`int` fields have no initializer, so a `Game.agf` element
  that is absent (an older Editor's project) leaves garbage in `agsbuild` and `ReadCustomDataDirectories` instead of
  the default from `AGS.Types/Settings.cs` (several are `true`, so `{}` is not the fix); only `PixelPerfect` got its
  default, after CI failed on it. The open upstream PR #3187 (branch `ags4-tools-2-agfreader`, which took #3147's tests)
  still has the wrong `EXPECT_FALSE(s.PixelPerfect)` in `AGFReaderBlocks.GameSettingsDefaults` (checked 2026-11-12).
- 7.1f's "left open" notes: closed on 2026-11-12. The private 8-bit game builds byte-identical with the `pre-6.7d`
  reference editor in all ten variants (`compare_builds.py --editor <reference> --api-header pre-6.7d`, today's
  `agsbuild`), once its 63 Background palette slots hold what the reference editor writes: that editor loads a room
  while it builds and writes that room's colours into the slots, and the project's `Game.agf` (saved 2026-11-11) held
  another room's. Old indexed interactions do not exist in the 4.0 projects the tools read (D1). Found: the editor's
  `CopyGamePalette` shares the `PaletteEntry` objects with the game, so opening a room changes the game palette and
  the next save writes it (fork and upstream `ags4`; PLAN.md open items).
- 3.1: the unchecked `GetDiskFreeSpaceExW` result in `AGSWin32::GetDiskFreeSpaceMB`.
- 1.5a: the 4-bit pixel write in `Common/gfx/bitmapdata.cpp` (wrong mask and shift, untested).
- 3.4-1's leftovers that are constant now: the write-only `GUI::DataVersion`, the always-on runtime behaviour
  switches, three dialog option offsets that are always 0, `FindNearestWalkablePoint`'s old method used by tests only.
- The scripts in a git worktree (2026-11-07): `refactor_metrics.py` refuses to run (`.git` is a file there),
  `prepush.py`'s `linux` stage cannot fetch from one and its `refs/prepush/worktree` ref is shared with the main
  checkout, and the `msbuild` stage needs the git-ignored NuGet packages, irrKlang DLLs and the googletest submodule a
  fresh worktree lacks.
- 5.1a step 2 (2026-11-16), found while writing its tests, none fixed:
  - a room object's `FrameOffsetX/Y` are set only while it animates and a save does not hold them, so a
    restored object that had stopped on a frame with an offset is drawn without it (`update_object_scale`;
    upstream the same);
  - `WriteMainBlock` (`Common/game/room_file.cpp`) writes an uninitialised 4x4 stand-in buffer for a room
    without images, so two writes of such a room differ (real rooms have images; a test gives the room zeroed
    ones);
  - `walkbehinds_cropout` indexes its 16 entries with a mask value up to 255 unchecked;
  - `Walkarea_GetScalingMin/Max` return the stored `ScalingFar`/`ScalingNear` as they are (noted by the batch
    `net`; to look at when the scaling is next touched);
  - `RoomState::ReadFromSavegame` trusts the counts a save gives;
  - upstream candidates from the fixes of this step: the `SetAreaLightLevel` bound, `get_walkbehind_pixel`'s
    mask, `Object.DestinationX/Y` of an object that has not moved, the walkable area's `FaceDirectionRatio`;
    from step 3, `ViewFrame.YOffset` returning the x offset; from step 4, the GUI controls' property values
    kept by a restore;
  - `RoomObjectState::NoView` is not named as a constant (`kNoView`); `check_build_lists.py --build` on a
    tree with the precompiled header reports its `cmake_pch.cxx` entries.
  Closed since: `InventoryItemWidth` has an initializer (step 4), a room object starts at `kNoMoveList`
  and the two header moves are done (step 3).
- 5.1a steps 3 and 4 (2026-11-17), found on the way, none fixed:
  - **a restore appends the saved parsed words** to `GameState::ParsedWords` instead of replacing them
    (`ReadFromSavegame` uses a back inserter, `DoBeforeRestore` does not clear the list): disabled test
    `SavegameContent.DISABLED_ParsedWordsAreReplacedByARestore`; likely harmless (the list is rebuilt at
    every parse), upstream not checked;
  - **`GameState::OverlayCount` after restoring a save that had overlays**: `remove_all_overlays` brings it
    to 0 and `ReadOverlays` puts the saved overlays back without counting them, so it is too low and goes
    negative as they are removed (`reset_drawobj_dynamic_index` then runs while overlays exist). Read from
    the code, not confirmed by a test; upstream not checked;
  - the `Transient` fields that a restore into a running game leaves from the game played before:
    `WaitSkippedBy` and `WaitSkippedByData` (the script's report of how the last wait was skipped),
    `LastNamedLocation`, `LocationNameSaveCursor`, `IsScreenFadedOut`, `DialogRequestResult`; `RoomChanges`
    gains one per restore. 5.11's restore into a fresh `Game` settles them;
  - a save whose voice word says "blocking, no voice" restores as voice and blocking (the read takes any
    non-zero word as "has voice"); the engine never writes that state;
  - a custom property map with more than one value is written to a save in the hash map's iteration order
    (`Properties::WriteValues` over a `std::unordered_map`), which a restored map need not have again: two
    saves of the same state can differ in the order of a map's entries. Found by a byte test on GCC (with
    MSVC's library the order happened to come back); the saves are equivalent. The same holds for the script
    timers. 5.12's format should write such maps sorted;
  - GUI control property values are read from a save by their index in the save, without the control remap
    that `ReadGUIControlArray` applies to the controls themselves (seen while reading, not investigated);
  - `GameState::NoMultiLoopRepeat` (`game.no_multiloop_repeat`): `CHECKME` in the header: only a script can
    set it; retiring it would make the script variable an unused slot, a script API change for the user;
  - about ten other script switches of `GameState` that the engine only tests for zero are `int`
    (`ShowSingleDialogOption`, `GreyOutInventory`, `AutoUseWalkToPoints`...); the three with negative names
    stay `int` by the user's decision, so these stay too;
  - `AudioClipDef::IsRepeatingByDefault` is a `char` used as a boolean; `GameDef::IsSpriteValid` has no
    caller (`Game::IsSpriteValid` hides it); `CharacterDef::ScriptName` is a `Saved` row that no save holds
    (the character's save test copies it back by hand);
  - the definition's custom property values are six lists of `GameDef`, not a `Properties` member of each
    `XDef` as the room's definitions have it (a loader change); the room's play values are lists of
    `RoomState`, not `PropertyOverrides` of the live room entities; the GUI's two lists are in `GameState`
    until step 5;
  - header proposals of the two trims, each a small move of declarations: `CursorRole` out of
    `mouse_cursor_def.h` (80 translation units would lose it through `game_def.h`), the `HGameFileError`
    typedefs out of `main_game_file.h` (77; it also frees `guimain.h` and `cc_script.h` from `game_def.h`),
    `QueuedAudioItem` held by value in `GameState` (it brings `soundclip.h`), `util/keycode.h` in
    `game_state.h` for one inline helper (131), `screen.h`'s include of `game_state.h`;
  - `Script/use_context.py --check` cannot read a function template with default template arguments (it
    reports "use outside a function"); a list row of a field table does not pass its role to its elements
    (`Loops`, `Frames`, `Options` are `FromDefinition` rows for that reason).
- `Tools/test/directory_test.cpp`'s comment on the MSBuild googletest being older is out of date;
  `FetchGoogleTest.cmake` still pins 1.8.1 for GCC older than 5, which no builder uses.
