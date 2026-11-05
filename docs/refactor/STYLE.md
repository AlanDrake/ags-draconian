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
# lint changed lines (CI gate, PLAN.md 2.4; needs build/compile_commands.json; the diff is against
# the merge base with the upstream branch unless --base is given, and includes uncommitted edits)
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
  `texx`, `widd`, `iit`, `vii` are on the `refactor_metrics.py` watchlist and banned in new code
  **[enforced on changed lines]**: `tidy_diff.py` flags a new variable or parameter with such a name
  (`ags-cryptic-name`); a line that only uses a legacy one passes.
- **One concept, one name:** a `CharacterInfo*` is `ch` (or `character`) everywhere, not `chaa`,
  `chap`, `chin`, `chinfo`, `char1` or `chi`.
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

The local build is MSVC; CI also builds with GCC (8 in the Linux image), Clang with libc++,
Emscripten, the Android NDK and Apple Clang (macOS 10.9 target). Each rule is a CI break that
MSVC let through (LOG.md task in brackets).

- Include the header of every name used, C library names too (`size_t`, `time_t`, `atoi`,
  `assert`, `SIZE_MAX`, `uint32_t`): MSVC's library includes them transitively, libstdc++ and
  libc++ do not [2.6, 2.8]. The header checks compile headers, not `.cpp` files.
- No `std::filesystem`: Apple's libc++ has none below macOS 10.15. Use `Directory`, `File` and
  `Path` [1.3].
- A CMake target whose code uses `std::thread` links `Threads::Threads`: before glibc 2.34 threads
  are in libpthread [3.2].
- Don't bind an in-class `static const` member (`String::NoIndex`) to a reference: without an
  out-of-class definition MSVC folds it, GCC and Clang fail to link [7.1o]. See "Tests".
- OpenGL code that Android and Emscripten build as GLES2 (`Engine/gfx/ali3dogl.cpp`) calls no
  fixed-function API (`glColor4f`) outside a desktop-only `#if` [0.4].
- A repo-wide edit (a removed `using`, a rename) reaches files Windows does not compile
  (`platform/` other than `windows`, `*_posix.*`, non-Windows `#if` branches): search them by
  hand, the local build won't [2.5].

### Comments

- Say **why**, or what isn't obvious; don't repeat the code. Every public function and class gets a
  plain `//` block above its declaration in the header (no Doxygen needed).
- **No commented-out code**; no changelogs, author initials or dates ("CJ: fixed for 2.61").
- `// FIXME:` = wrong or unreliable in some case (say which); `// TODO:` = works, could be better
  (say what and why). `HACK` becomes `FIXME`; a `CHECKME` is answered in PLAN.md 3.6 or becomes a
  `TODO`.
- **A stale comment is a bug:** fix or delete it in the commit that makes it stale.
- Keep the license header block at the top of every file.

### Tests

- GoogleTest, in the module's `test/` folder, as `<unit>_test.cpp`; names
  `TEST(Unit, BehaviourUnderCondition)` (`TEST(SpriteFile, ReadsCompressedLzwSprite)`).
- A bug fix comes with a test that fails without it, or the commit message says why it can't.
- `EXPECT_EQ`/`EXPECT_NE` take references: compare an in-class constant with `==`/`!=` inside
  `EXPECT_TRUE`, as `string_test.cpp` does ("Portability").
- A case that passes invalid input on purpose runs only without asserts: `#ifdef NDEBUG` around
  the body, `GTEST_SKIP()` otherwise (`sprite_cache_test.cpp`): debug builds assert on that input [1.3].

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
   commit as a code change.
5. Every commit builds and passes the tests.
