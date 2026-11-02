# AGS Draconian code style

This guide applies to all first-party code. It formalises the style that the newer upstream code
(`Common/util`, `Common/game`, `Engine/game`, `Compiler/script2`) already follows, so that most of
the work is bringing the old code in line rather than inventing something new.

Rules marked **[enforced]** are checked by a tool. The others are checked in review.

## Scope

**Third-party code is never reformatted, renamed or linted.** It keeps its upstream style so it can be
updated by dropping in a new version. Each of these directories has a `.clang-format` with
`DisableFormat: true`, and `.editorconfig` marks it as `generated_code`:

| Directory | Contents |
|---|---|
| `libsrc/` | allegro (trimmed 4.x), freetype, glm, miniz, mojoAL, sdl_ttf, stb, tinyxml2 |
| `Common/libsrc/`, `Common/libinclude/` | aastr, googletest, ogg/theora/vorbis headers |
| `Engine/libsrc/` | apeg, glad, ogg, theora, vorbis |
| `Editor/scintilla/`, `Editor/AGS.Controls/ScintillaNET/`, `Editor/AGS.Controls/AdressBarEx/` | Scintilla and the .NET wrappers |
| `Plugins/agslua/` | Lua plugin with bundled Lua/zlib (removal candidate, see PLAN.md 7.2) |

If you add a vendored library, give its folder the same `.clang-format` stub and add it to the
`.editorconfig` third-party section and to `THIRD_PARTY` in `Script/refactor_metrics.py`.

## Tools

| Tool | Version | Config | Used for |
|---|---|---|---|
| clang-format | 19.1.x (`pip install clang-format==19.1.7`) | `/.clang-format` | C/C++ layout **[enforced]** |
| clang-tidy | 19.1.x (`pip install clang-tidy==19.1.0`) | `/.clang-tidy` | C/C++ naming and bug-prone patterns, changed lines only **[enforced]** |
| dotnet format / IDE analyzers | .NET SDK 8+ | `/.editorconfig` | C# layout and naming (suggestions until PLAN.md 6.2) |
| `Script/refactor_metrics.py` | Python 3.8+ | — | progress tracking, see `BASELINE.md` |

Everyone uses the pinned versions, because different clang-format versions produce different
output. Older system installs (for example a clang-format 7 shipped with an old LLVM) are too old:
they don't understand this config.

```sh
# format only the lines you changed (run before committing)
git clang-format --binary <path-to>/clang-format origin/draconian
# lint the lines you changed (needs build/compile_commands.json; clang-tidy-diff.py is not in the
# pip package, take it from llvm-project/clang-tools-extra/clang-tidy/tool/ at the same version)
git diff -U0 origin/draconian -- '*.cpp' '*.h' | python clang-tidy-diff.py -p1 -path build
```

---

## C++

### Language level

- **C++17** for all first-party targets (PLAN.md 0.4 raises it from C++11).
- Don't add compiler extensions or platform intrinsics outside `Engine/platform/*` and `Common/platform/*`.

### Layout **[enforced by clang-format]**

- 4 spaces, no tabs, 120 columns.
- Allman braces: the opening brace goes on its own line for functions, classes, namespaces, and
  control statements.
- Every `if`/`for`/`while` body goes on its own line. Braces are optional for a single statement but
  required if any branch of the same `if`/`else` chain has them.
- Namespace contents are not indented. Access specifiers sit at the class's indentation level.
- Pointer and reference symbols go next to the name: `Bitmap *bmp`, `const String &name`.
- Short inline member functions may stay on one line: `int GetWidth() const { return _width; }`.

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

### Naming **[enforced by clang-tidy on changed lines]**

| Entity | Style | Example |
|---|---|---|
| Namespaces | `CamelCase` | `AGS::Common`, `AGS::Engine` |
| Classes, structs, enums, type aliases | `CamelCase` | `SpriteCache`, `GameSetupStruct` |
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
| Files | `snake_case.cpp/.h`, named after the main class | `sprite_cache.cpp` |

Rules for picking names:

- **No abbreviations except well-known ones** (`id`, `idx`, `num`, `pos`, `src`, `dst`, `bmp`, `gfx`,
  `cfg`, `str`, `x`/`y`/`w`/`h`). Names like `chaa`, `objj`, `butt`, `guio`, `sds`, `tqs`, `eres`, `scam`,
  `texx`, `widd`, `iit` and `vii` are on the rename watchlist in `refactor_metrics.py` and must not
  appear in new code.
- **The same concept gets the same name.** A `CharacterInfo*` parameter is `ch` (or `character`)
  everywhere. It is not `chaa`, `chap`, `chin`, `chinfo`, `char1` or `chi` depending on the file.
- Booleans read as predicates: `is_visible`, `HasAlpha()`, `_needsRedraw`.
- Functions are verbs (`UpdateCharacterAnimation`). Types are nouns.
- Units go in the name when they're ambiguous: `delay_ms`, `volume_percent`, `speed_fps`.
- **New mutable globals are not allowed.** The existing globals (`game`, `play`, `thisroom`, `croom`,
  `objs`, ...) are grandfathered and are being moved into context objects (PLAN.md phase 5). Don't
  mass-rename them before that. Renaming them only to remove them later wastes a commit.

### Headers and includes

- `#pragma once` instead of include guards (PLAN.md 2.5). Identifiers like `__AGS_CN_UTIL__STRING_H`
  are reserved by the C++ standard.
- **Every header compiles on its own** (include what you use, forward-declare where it's enough).
- Include order: the file's own header, a blank line, C/C++ standard headers, a blank line,
  third-party headers, a blank line, AGS headers. Sorting is manual until PLAN.md 2.6 turns on
  `SortIncludes`.
- **Never put `using namespace` in a header.** In `.cpp` files it's allowed after all includes.
- Don't write `extern` declarations in `.cpp` files. A global belongs in the header of the module
  that owns it.

### Memory, strings and types

- Ownership is expressed with `std::unique_ptr` / `std::shared_ptr`. Don't add new raw
  `new`/`delete`, `new[]`/`delete[]`, `malloc`/`free`. Raw pointers and references are non-owning.
- Use `AGS::Common::String` (or `std::string` in code without engine dependencies). Don't add
  fixed `char buf[N]` + `sprintf`/`strcpy`/`strcat`. Format with `String::FromFormat`.
- Use `nullptr`, never `NULL` or `0`, for pointers.
- Use `override` on every overriding virtual. Use `final` where a class isn't meant to be extended.
- Use `enum class` for new enumerations unless the values are bit flags combined with `|`.
- `auto` is fine when the type is obvious from the right-hand side or is an iterator. Otherwise spell
  the type out.
- No C-style casts in new code: `static_cast`, `reinterpret_cast`, `const_cast`.
- Use fixed-width integers (`int32_t`, `uint8_t`) for anything serialized (game data, saves, the script
  ABI). Use plain `int` / `size_t` elsewhere.

### Functions and control flow

- New and rewritten functions stay under **120 lines, 4 levels of nesting, 7 parameters**
  (`readability-function-size`). If a function is longer than that, it's doing several jobs.
- Prefer early returns over nested `if` pyramids.
- Don't add new `goto`. The one existing `goto` is a removal target.
- **No new `quit()` calls in library-level code** (`Common/`, engine subsystems). Return `HError` /
  `HGameFileError` or log and recover. Only the top-level engine loop decides whether to abort.
- Script API wrappers (`Sc_*`) contain only argument marshalling. Logic goes in the `Type_Member`
  implementation function, or better, a member function of the engine object.

### Comments

- A comment says **why**, or what isn't obvious. It doesn't repeat the code.
- Document every public function and class in the header with a plain `//` block above the
  declaration (this is the existing style; Doxygen is not required).
- **No commented-out code.** Delete it. Git keeps the history.
- No changelogs, author initials or dates in comments ("CJ: fixed for 2.61"). That's what
  `git blame` is for.
- Two markers, by severity:
  - `// FIXME: ...` means the code is wrong or unreliable in some case. Say which case.
  - `// TODO: ...` means the code works but could be improved. Say what and why.

  `HACK` becomes `FIXME`. A `CHECKME` is answered during PLAN.md 3.6 and replaced with a comment
  explaining why, or becomes a `TODO` if it can't be answered yet.
- **A comment that no longer matches the code is a bug.** Fix or delete it in the same commit that
  makes it stale.
- Keep the standard license header block at the top of every file.

### Tests

- Unit tests use GoogleTest and live in the `test/` folder of their module, as `<unit>_test.cpp`.
- Test names follow `TEST(Unit, BehaviourUnderCondition)`, for example
  `TEST(SpriteFile, ReadsCompressedLzwSprite)`.
- A bug fix comes with a test that fails without the fix. If a test genuinely can't be written,
  say why in the commit message.

---

## C# (Editor)

Layout and naming are defined in `/.editorconfig` and mirror the existing editor code:

| Entity | Style | Example |
|---|---|---|
| Types, methods, properties, events, enum members | `PascalCase` | `RoomsComponent`, `LoadProject()` |
| Interfaces | `I` + `PascalCase` | `IEditorComponent` |
| Private/protected fields | `_camelCase` | `_game`, `_editorExePath` |
| Constants | `UPPER_SNAKE_CASE` (the existing editor convention) | `LOCK_FILE_NAME` |
| Parameters, locals | `camelCase` | `roomNumber` |

- Allman braces. Use braces even for one-line bodies.
- `var` only when the type is visible on the right-hand side.
- **Don't swallow exceptions with an empty catch.** Catch specific exception types, and log or show
  the error. There are 122 catch-all handlers in the editor today, tracked in BASELINE.md.
- Don't add new uses of the static `Factory` service locator in new classes. Pass dependencies through
  the constructor (PLAN.md 6.4).
- Separate UI from logic. Anything that doesn't need WinForms belongs in a class that doesn't reference
  `System.Windows.Forms`, so it can be tested and later shared with another UI front end.

---

## Commits

These rules extend the existing ones in `CONTRIBUTING.md` (one program per commit, prefixes like
`Engine:`, a 72-character title):

1. **Mechanical and behavioural changes never share a commit.** A commit is either *format-only*,
   *rename-only*, *move-only*, *delete-only*, or *behaviour change*.
2. Commit titles state the kind of change: `Engine: [rename] chaa -> ch in character.cpp`,
   `Common: [format] util/`, `Engine: [move] ac/route_finder -> navigation/`.
3. Add the hash of every format-only commit to `.git-blame-ignore-revs`.
4. Check format-only and rename-only commits by **comparing object code** with
   `Script/compare_objects.py`. It builds both revisions (optimized, no debug info, no LTO) in one
   worktree and build directory under `.compare/`, then compares every object file:

   ```sh
   python Script/compare_objects.py HEAD          # uncommitted changes against HEAD
   python Script/compare_objects.py HEAD~1 HEAD   # the last commit
   ```

   The result must be "mechanical change". The first run builds everything (about the time of a
   full build); later runs only rebuild what the change touches. What to expect:
   - formatting, comments, renaming locals or parameters, reordering includes that don't change
     meaning: objects are **identical**;
   - GoogleTest files (`*_test.cpp`) are reported separately and tolerated, because the test macros
     record `__LINE__`; asserts don't matter, since the build defines `NDEBUG`;
   - renaming a function, type or global, or changing a string literal, changes the objects **by
     design**. Such a commit is not purely mechanical: review it as a code change.
5. Every commit builds and passes the tests.
