# AGS Draconian code style

For all first-party code. It formalises the style the newer upstream code (`Common/util`,
`Common/game`, `Engine/game`, `Compiler/script`) already follows. **[enforced]** rules are checked by
CI; the rest in review.

## Scope

**Third-party code is never reformatted, renamed or linted**, so it can be updated by dropping in
a new version. Each directory below has a `.clang-format` with `DisableFormat: true` and is
`generated_code` in `.editorconfig`:

| Directory | Contents |
|---|---|
| `libsrc/` | allegro (trimmed 4.x), freetype, glm, miniz, mojoAL, sdl_ttf, stb, tinyxml2 |
| `Common/libsrc/`, `Common/libinclude/` | aastr, googletest, ogg/theora/vorbis headers |
| `Engine/libsrc/` | apeg, glad, ogg, theora, vorbis |
| `Editor/scintilla/`, `Editor/AGS.Controls/ScintillaNET/`, `Editor/AGS.Controls/AdressBarEx/` | Scintilla and the .NET wrappers |
| `Plugins/agslua/` | Lua plugin with bundled Lua/zlib (removal candidate, PLAN.md 7.2) |

A new vendored library gets the same stub, an entry in the `.editorconfig` third-party section and
in `THIRD_PARTY` in `Script/refactor_metrics.py`.

## Tools

| Tool | Version | Config | Used for |
|---|---|---|---|
| clang-format | 19.1.7 (`pip install clang-format==19.1.7`) | `/.clang-format` | C/C++ layout, whole files **[enforced]** (`verify.yml`) |
| clang-tidy | 19.1.0 (`pip install clang-tidy==19.1.0`) | `/.clang-tidy` | C/C++ naming and bug-prone patterns, changed lines (CI gate: PLAN.md 2.4) |
| dotnet format / IDE analyzers | .NET SDK 8+ | `/.editorconfig` | C# layout and naming (suggestions until PLAN.md 6.2) |
| `Script/refactor_metrics.py` | Python 3.8+ | — | progress tracking, `BASELINE.md` |

Use the pinned versions: other clang-format versions format differently, and old system installs
(clang-format 7) don't understand the config.

```sh
# CI's format check (whole files); clang-format -i to fix
git ls-files '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hpp' '*.inl' | xargs clang-format --dry-run -Werror
# clang-tidy on changed lines, as the CI gate does (needs build/compile_commands.json)
python Script/tidy_diff.py --build build
```

## C++

**C++17** for all first-party targets. No compiler extensions or platform intrinsics outside
`Engine/platform/*` and `Common/platform/*`.

### Layout **[enforced by clang-format]**

- 4 spaces, no tabs, 120 columns; Allman braces for functions, classes, namespaces and control
  statements.
- Every `if`/`for`/`while` body on its own line. Braces optional for one statement, required if any
  branch of the same `if`/`else` chain has them.
- Namespace contents not indented; access specifiers at the class's level.
- `Bitmap *bmp`, `const String &name`; short inline members may stay on one line.

```cpp
namespace AGS
{
namespace Engine
{

class Viewport
{
public:
    int GetWidth() const { return _width; }
    void SetSize(int width, int height);

private:
    int _width = 0;
    int _height = 0;
};

} // namespace Engine
} // namespace AGS
```

### Naming (clang-tidy)

| Entity | Style | Example |
|---|---|---|
| Namespaces, classes, structs, enums, type aliases | `CamelCase` | `AGS::Common`, `SpriteCache` |
| Interfaces (pure abstract) | `I` + `CamelCase` | `IStreamBase`, `IGraphicsDriver` |
| Functions and methods | `CamelCase` | `LoadRoom()`, `GetCStr()` |
| Script API implementations | `Type_Member` | `Character_AddInventory()` |
| Parameters, locals | `snake_case` | `max_chars`, `room_index` |
| Private/protected data members | `_camelCase` | `_bufHead`, `_spriteCount` |
| Public data members (structs, PODs) | `CamelCase` | `Capacity`, `Version` |
| Unscoped enum values | `k` + `Type_Value` or `kValue` | `kStream_Read`, `kSeekBegin` |
| Scoped (`enum class`) values | `CamelCase` | `RenderMode::Hardware` |
| `constexpr`/global constants | `k` + `CamelCase` | `kMaxViewports` |
| Macros | `UPPER_CASE`, `AGS_` prefix for new ones | `AGS_PLATFORM_OS_WINDOWS` |
| Files | `snake_case.cpp/.h`, after the main class | `sprite_cache.cpp` |

- **No abbreviations except well-known ones** (`id`, `idx`, `num`, `pos`, `src`, `dst`, `bmp`, `gfx`,
  `cfg`, `str`, `x`/`y`/`w`/`h`). `chaa`, `objj`, `butt`, `guio`, `sds`, `tqs`, `eres`, `scam`,
  `texx`, `widd`, `iit`, `vii`, `charid`, `objid`, `invi`, `newval`, `newwid`, `newhit` are on the `refactor_metrics.py` watchlist and banned in new code
  **[enforced]**: `tidy_diff.py` flags a new variable or parameter so named.
- **One concept, one name:** a `CharacterInfo*` is `ch` (or `character`) everywhere, not `chaa`,
  `chap`, `chin`, `chinfo`, `char1` or `chi`. Name the concept, not the struct: `ch` stays right when
  `CharacterInfo` becomes design-time data and the engine's character is a `Character` (PLAN.md 5.1a).
- **Reserved names** mean exactly one thing in first-party code, so a reader never has to check
  **[enforced]** (`tidy_diff.py` flags a variable or parameter so named with another explicit type;
  `auto` is not checked): `ch` = a character (`CharacterInfo`, the plugin API's `AGSCharacter`, later
  `Character`), `chex` = `CharacterExtras`, `btn` = `GUIButton`. A text character or byte is `c`, a
  playing sound (`SoundClip`, not the `AudioClip` asset) `playback`: `voice` is speech voice-over
  and `channel` a `ScriptAudioChannel`. An index is `<thing>_id` (`char_id`, `obj_id`, `btn_id`,
  `gui_id`, `item_id`), never `charid`, `objn` or the bare name of the thing. Where a file has the
  script handle and the engine's record of one thing, the handle takes the short name
  (`ScriptObject *obj`, `RoomObject &room_obj`). Words are spelled out and separated: `new_value`,
  `new_width`, not `newval`, `newwid`. A name is added here once the code uses it for nothing else.
- Booleans are predicates (`is_visible`, `HasAlpha()`, `_needsRedraw`); functions are verbs, types
  nouns; ambiguous units go in the name (`delay_ms`, `volume_percent`, `speed_fps`).
- **No new mutable globals.** The existing ones (`game`, `play`, `thisroom`, `croom`, `objs`, ...)
  move into context objects in PLAN.md phase 5; don't mass-rename them before that.

### Headers and includes

- `#pragma once`, not include guards (`__AGS_CN_UTIL__STRING_H` is a reserved identifier).
- **Every header compiles on its own** (include what you use, forward-declare where enough)
  **[enforced]**: `-DAGS_HEADER_CHECK=1` and the `header_check` target (`CMake/HeaderCheck.cmake`).
- Include order **[enforced]** (`SortIncludes: CaseSensitive`, `IncludeBlocks: Regroup`): own
  header; `platform/windows/windows.h` if needed (before any other Windows header); C/C++ standard
  headers; other system and third-party headers (`<SDL.h>`, `<shlwapi.h>`); AGS headers (`"..."`).
  A blank line between groups, each sorted; a `#if` splits a group, so conditional includes stay.
- **No `using namespace` in headers**; in `.cpp` files only after all includes.
- No `extern` declarations in `.cpp` files: a global belongs in its owning module's header.

### Memory, strings and types

- Ownership is `std::unique_ptr` / `std::shared_ptr`; no new raw `new`/`delete`, `new[]`/`delete[]`,
  `malloc`/`free`. Raw pointers and references are non-owning.
- `AGS::Common::String` (`std::string` where there are no engine dependencies), formatted with
  `String::FromFormat`; no new `char buf[N]` + `sprintf`/`strcpy`/`strcat`.
- `nullptr`, never `NULL` or `0`; `override` on every override, `final` where not meant to be
  extended; `enum class` unless the values are `|`-combined flags.
- `auto` when the type is obvious from the right-hand side or an iterator; no C-style casts.
- Fixed-width integers (`int32_t`, `uint8_t`) for anything serialized (game data, saves, the
  script ABI); plain `int` / `size_t` elsewhere.

### Functions and control flow

- New and rewritten functions stay under **120 lines, 4 levels of nesting, 7 parameters**
  (`readability-function-size`); early returns over nested `if` pyramids; no new `goto`.
- **No new `quit()` in library-level code** (`Common/`, engine subsystems): return `HError` /
  `HGameFileError`, or log and recover. Only the top-level engine loop decides to abort.
- `Sc_*` wrappers only marshal arguments; logic goes in the `Type_Member` function, or better a
  member of the engine object.

### Portability

MSVC is the local compiler; each rule is a CI break it let through (LOG.md task in brackets).

- Include what you use, C library names too (`size_t`, `time_t`, `assert`): libstdc++ and libc++ have
  fewer transitive includes [2.6, 2.8].
- No `std::filesystem` (macOS 10.9 target); use `Directory`, `File`, `Path` [1.3].
- A target using `std::thread` links `Threads::Threads` (glibc before 2.34) [3.2].
- No reference to an in-class `static const` without a definition (`EXPECT_EQ(String::NoIndex, x)`):
  GCC and Clang fail to link; write `EXPECT_TRUE(x == String::NoIndex)` [7.1o].
- No fixed-function GL (`glColor4f`) in code built as GLES2 (`ali3dogl.cpp`) outside a desktop `#if` [0.4].
- Repo-wide edits reach code Windows does not compile (`platform/` other than `windows`, `*_posix.*`,
  `#if` branches) [2.5]: run `prepush.py`'s `emscripten` stage; check macOS, iOS, Android and FreeBSD
  code by hand.

### Performance

A change that is not meant to make something faster must not make it slower in the builds given to
users: MSVC Release (`/O2`), GCC and Clang Release (`-O3`, LTO). Debug builds are used internally only
and do not count (2026-11-06). A fix for a sanitizer report, a warning or a lint finding is such a
change. What the first case taught is marked [1.5b-3] (LOG.md).

- **Hot paths:** the script VM (`Engine/script/`) and what it calls per instruction; the loops over
  pixels (`Common/gfx/`, `Engine/gfx/`, `Engine/ac/draw*`); `Common/util/` (memory, streams, `String`,
  the decompressors); the sprite cache; audio decoding and mixing (`Engine/media/audio/`). The commit
  message of a change there says what it costs in the Release builds and how that was measured.
- **Measure the objects that use the code, not the code on its own.** `compare_objects.py` names the
  objects whose code changed (it builds MinSizeRel, which nobody runs: it shows where to look, not the
  cost); compare their disassembly built as Release, or benchmark before and after. The
  `Memory::Read/Write` helpers compiled to the same code on their own with GCC `-O3` and to a different
  `ScriptExecutor::Run` once inlined [1.5b-3].
- **A cost is the user's decision:** report the numbers (which build, ns or percent, which path) and
  stop there.
- **Optimizers differ** [1.5b-3]: MSVC does not merge a value composed from bytes into one load
  (`(b[3] << 24) | ...` is ten instructions at `/O2`, one with GCC and Clang at `-O2`); a `memcpy` of
  constant size is one load or store at `/O2` and `-O1`, but a call with MSVC at `/O1`, and a checked
  call with GCC's `_FORTIFY_SOURCE=3` (Ubuntu's default) when the size of the destination is known.
- **An integer at an address that may be misaligned** is read and written with `Memory::ReadInt32` and
  its siblings (`util/memory.h`): they go through the unaligned types of `platform/types.h`, which are
  defined behaviour and the same load or store as the cast pointer they replace [1.5b-3].
- A new allocation, `String` or container copy, virtual or `std::function` call, lock or log line
  inside a loop over pixels, script instructions, samples or bytes is a behaviour change to measure,
  not a detail of a refactoring.

### Comments

- Say **why**, or what isn't obvious; don't repeat the code. Every public function and class gets a
  plain `//` block above its declaration in the header (no Doxygen needed).
- **Worth a comment:** an order or invariant the code depends on, a tie to code elsewhere, something
  kept for the compatibility contract (name it: "C3, script ABI"), a hot path that was measured. The
  best one says what breaks if the line changes.
- **No commented-out code**; no changelogs, author initials or dates ("CJ: fixed for 2.61").
- **Describe the code, not the change:** no "now uses X", "moved from Y", "fixed the crash", "no longer
  needed". What a commit did goes in its message; the comment must still read true in a year.
- **Comments move with their code:** a refactoring keeps the why-comments of what it moves or
  rewrites, reworded where needed. One is deleted only when its reason is gone, not because the code
  around it changed.
- **A comment and its code disagree:** trust neither. `git log -S`, blame or a test decides which is
  right, then the other is fixed.
- `// FIXME:` = wrong or unreliable in some case (say which); `// TODO:` = works, could be better
  (say what and why). `HACK` becomes `FIXME`; a `CHECKME` is answered in PLAN.md 3.6 or becomes a
  `TODO`.
- **A stale comment is a bug:** fix or delete it in the commit that makes it stale.
- Keep the license header block at the top of every file.

### Tests

- GoogleTest, in the module's `test/` folder, as `<unit>_test.cpp`; names
  `TEST(Unit, BehaviourUnderCondition)` (`TEST(SpriteFile, ReadsCompressedLzwSprite)`).
- A bug fix comes with a test that fails without it, or the commit message says why it can't.
- A case with deliberately invalid input runs only under `NDEBUG` (`GTEST_SKIP()` otherwise): debug
  builds assert on it [1.3].

## C# (Editor)

Layout and naming are in `/.editorconfig`, mirroring the existing code:

| Entity | Style | Example |
|---|---|---|
| Types, methods, properties, events, enum members | `PascalCase` | `RoomsComponent`, `LoadProject()` |
| Interfaces | `I` + `PascalCase` | `IEditorComponent` |
| Private/protected fields | `_camelCase` | `_game`, `_editorExePath` |
| Constants | `UPPER_SNAKE_CASE` | `LOCK_FILE_NAME` |
| Parameters, locals | `camelCase` | `roomNumber` |

- Allman braces, also for one-line bodies; `var` only when the type is visible on the right.
- **No empty catch:** catch specific types and log or show the error (122 catch-alls today,
  BASELINE.md).
- No new uses of the static `Factory` locator: constructor parameters (PLAN.md 6.4).
- Anything that doesn't need WinForms goes in a class that doesn't reference
  `System.Windows.Forms`, so it can be tested and shared with another UI.

## Commits

Extends `CONTRIBUTING.md` (one program per commit, prefixes like `Engine:`, 72-character title):

1. **Mechanical and behavioural changes never share a commit**: a commit is format-only,
   rename-only, move-only, delete-only, or a behaviour change.
2. Titles name the kind: `Engine: [rename] chaa -> ch in character.cpp`, `Common: [format] util/`,
   `Engine: [move] ac/route_finder -> navigation/`.
3. Every format-only commit's hash goes into `.git-blame-ignore-revs`.
4. Format-only and rename-only commits are checked by **comparing object code** with
   `Script/compare_objects.py`: it builds both revisions (optimized, no debug info, no LTO) in one
   worktree and build directory under `.compare/` and compares every object file.

   ```sh
   python Script/compare_objects.py HEAD          # uncommitted changes against HEAD
   python Script/compare_objects.py HEAD~1 HEAD   # the last commit
   ```

   The result must be "mechanical change". The first run is a full build; later runs rebuild only
   what changed. Formatting, comments, renamed locals or parameters and meaning-preserving include
   reorders give **identical** objects; `*_test.cpp` objects are reported separately and tolerated
   (the test macros record `__LINE__`; asserts don't matter under `NDEBUG`); renaming a function,
   type or global, or changing a string literal, changes objects **by design**, so review such a
   commit as a code change. A `[delete]` commit of unreferenced functions must get "deletion: N
   functions removed, 0 changed, 0 added (mechanical)": the changed objects lost those functions and
   nothing else (MSVC only; data such as string literals is not compared).
5. Every commit builds and passes the tests.
