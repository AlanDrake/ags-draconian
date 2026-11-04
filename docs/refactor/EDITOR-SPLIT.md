# The editor split: what ags-for-mac, #1092 and #3177 settle for 6.0 and 6.7c

The 6.0 study of 2026-11-04 (PLAN.md "Next up" item 2). Sources: the `ags-for-mac` repository at its
2026-10-30 state (`4c3f45c`, read in a scratch clone), upstream issue #1092 (2020), the #3177 thread
(2026-10-30 to 11-03), #2432 (2024-25), and the fork's `Editor/` as measured on this date. Decisions
are applied to PLAN.md (6.0, 6.1, 6.7c, U.6); this file keeps the evidence.

## 1. What ags-for-mac is

a-riddle-fan's port of the **3.6.x** editor (upstream `master` @ `edf5bb991`, 3.6.4 dev) to .NET 10 and
Avalonia on macOS arm64, AI-written (Claude Code), Artistic License 2.0 like AGS. Not a fork: upstream's
sources are **linked in place** from a sibling `ags/` checkout; what has to change is forked file by file,
excluded, or rewritten by a regex at build time, every case listed in its `docs/FORKED_FILES.md`.

| Part | Size | Content |
|---|---:|---|
| `src/AGS.Types.Core` | 18 forked files, 1 excluded, 6 shims, 1 rewrite | upstream `AGS.Types` compiled in place (see §3) |
| `src/AGS.Editor.Core` | 6,521 lines | UI-free editor logic: project load/save/upgrade, sprites, rooms, audio, fonts, translations, speech tools, templates, debugger, build through `agsbuild`/`agscc` |
| `src/AGS.Editor.Avalonia` | 10,259 lines (C# + axaml) | the UI |
| `src/AGS.Imaging`, `AGS.Native.Interop` | 371 + 1,709 lines | `IImage`/`IDrawingContext`/`PixelImage`; P/Invoke wrappers (`NativeRoom.cs` 617, `NativeGuiRenderer.cs` 213) |
| `native/shim` (`libagsnative`) | 3,072 lines, 72 exported C functions | CMake `SHARED` library over `AGS::Common`, C++14 (see §4) |
| `native/patches` | 4 unified diffs | upstream bugs found while porting; three are merged in upstream `ags4` and in the fork (#3144 mask colour, #3163 `agsbuild` string race, #3164 `CopyPixelsRegion`); the fourth is the file-based debugger (#3162, open) |
| tests | 81 `[Fact]` + 8 `[Theory]` methods in 4 projects | against a real game (Bernard-SP); rooms re-saved byte-stable; the GUI test only checks size and that opaque pixels exist, no comparison with the Windows editor's pixels |

The author's position (#3177, 2026-10-30): not offered upstream as is; waits for a stable 4.0 before
moving to `ags4`; worried about being seen as competing. Upstream's AI policy (#3025) rules out taking
fork code 1:1.

## 2. What upstream said (#3177, #1092, #2432)

- ivan-mogilko (2026-10-30): development focus is `ags4`; **(1)** decoupling `AGS.Types` from
  WinForms/`System.Drawing` is a valid objective (#1092); **(2)** `AGS.Native` should ideally go,
  `ags4` is closer (room editing is C# there); **(3)** the editor should build through the command-line
  tools. (2026-11-01): any improvement of the existing code's portability is welcome and "safe"
  whatever the framework decision.
- edmundito: interest exists, nothing planned, Windows 8 must stay supported; a cross-platform editor
  would be a separate team's effort; a-riddle-fan may continue solo.
- AlanDrake: Avalonia was one of the options considered earlier.
- #1092 (ericoporto, 2020): the file list, the idea of `IUserControl`, the observation that the UI
  editors already delegate to the editor through static delegates; his branch is deleted; no PR ever
  landed. #2432 (2024-25) closed without action.

So upstream welcomes 6.0 and the `AGS.Native` replacement in small PRs on `ags4`; the UI framework stays
undecided and 6.8 is fork-only (ALIGNMENT.md D4).

## 3. `AGS.Types`: measured, and how ags-for-mac changed it

The fork's `AGS.Types` and upstream `ags4`'s have the **same 17 files** using `System.Windows.Forms`
(`master` has 19: `PropertyTabInteractions.cs` and `RoomMessagesUIEditor.cs` no longer exist in `ags4`).
Of the 17, five only carry an unused `using` (`BaseFolderCollection.cs`, `Interfaces/IEditorComponent.cs`,
`CustomPropertiesUIEditor.cs`, `SpriteSelectUIEditor.cs`, `ViewUIEditor.cs`). The real dependencies:

| Where | WinForms / GDI use | ags-for-mac's replacement |
|---|---|---|
| `EditorFeatures/EditorContentPanel.cs` | `: UserControl`, `Keys`, `ActiveControl` | plain class with `object View`, `Func<bool> HasFocus`, `NotifyShownAtFullSize()` |
| `EditorFeatures/ContentDocument.cs` | holds `EditorContentPanel` | fork of the file (unused using only) |
| `EditorFeatures/MenuCommand.cs` | `Keys` | `AGS.Types.Keys`, WinForms values |
| `Interfaces/Docking/IDockingContainer.cs` | `Icon`, `ContextMenuStrip` | `IImage`, `object` |
| `Interfaces/IScriptEditorControl.cs` | `Control Control` | `object` |
| `Interfaces/IGUIController.cs` | `RegisterIcon(Icon)`, `DrawSprite(Graphics ...)` | `IImage`, `IDrawingContext` |
| `Interfaces/IRoomController.cs` | `Bitmap` ×4, `Graphics` ×3 | `IDrawingContext` (3.6 version; `ags4`'s file differs by 80 lines) |
| `Plugins/IAGSEditor.cs` | `CreateNewSprite(..., Bitmap, ...)` | `IImage` |
| `EditorFeatures/BaseTemplate.cs`, `GameTemplate.cs`, `RoomTemplate.cs` | `Icon` | `IImage` |
| `PropertyGridExtras/PropertyTabEvents.cs` | `: PropertyTab`, `Bitmap` (`event.bmp`) | `AgsPropertyTab` base, no bitmap |
| `PropertyGridExtras/BuildTargetUIEditor.cs` | `ItemCheck`, `MessageBox` | `CanUncheck` override |
| `PropertyGridExtras/ScriptFunctionUIEditor.cs` | `MessageBox` | `EditorMessages.ShowWarning` hook |
| `PropertyGridExtras/SpriteFileNameEditor.cs` | `: FileNameEditor` | `SpriteFileNameGUI` hook |
| `PropertyGridExtras/StringListUIEditor.cs` + `StringListUIEditorControl(.Designer).cs` | `IWindowsFormsEditorService`, a `UserControl` | `StringListGUI` hook; control excluded |
| `Utilities.PaintColorBlock`, `ColorUIEditor.PaintValue` | `Graphics` | `GetSwatchColor(value)` |
| 112 `[Editor(typeof(X), typeof(System.Drawing.Design.UITypeEditor))]` on 10 `UITypeEditor` classes; 11 `[PropertyTab(typeof(PropertyTabEvents), ...)]` | `UITypeEditor` lives in `System.Windows.Forms.dll` on .NET 6+ | a shim **redefining** `System.Drawing.Design.UITypeEditor` and empty `System.Windows.Forms[.Design]` namespaces, so the attributes compile unchanged |
| `Utilities.cs`, `Translation.cs`: `Encoding.Default` | UTF-8 on .NET Core, Windows-1252 on .NET Framework | regex rewrite to `AnsiEncoding.Default` (code-pages provider) |

What the plan had wrong (PLAN.md 6.0 before 2026-11-04): "the model stores `System.Drawing` `Font` (55)
and `Bitmap` (15)". The `Font` hits are AGS's own `AGS.Types.Font` class; no `System.Drawing.Font` exists
outside the WinForms designer file, and the model classes hold no `Bitmap` at all. The 91
`[TypeConverter]` attributes are `System.ComponentModel`, portable, and need no work. The `System.Drawing`
objects that remain are in the **plugin-facing interfaces and templates** (`Icon` 5 places, `Graphics` 6,
`Bitmap` 6), and the 9 static GUI delegates of the UI editors are set in `GUIController.cs` (8) and
`FontsComponent.cs` (1).

Consequence: a fully UI-free `AGS.Types` **changes the editor plugin API** (C5): plugins derive panes
from `EditorContentPanel`, build `MenuCommand`s with `Keys`, receive `IGUIController`/`IRoomController`/
`IAGSEditor` with `Icon`/`Graphics`/`Bitmap`. ags-for-mac states this ("plugins out of scope"). Only the
property-grid part (step 1 below) leaves plugins untouched, except a plugin that puts AGS's own UI editor
types on its properties.

Compatibility rules ags-for-mac had to establish for .NET Core and non-Windows, all relevant to 6.1's
`net8.0` build and to tests on Linux (6.0/6.3): `Encoding.Default`; the XML encoding name written as
`Windows-1252` (Framework casing) not `windows-1252`; `Path.GetInvalidFileNameChars()` rejects only `/`
and NUL off Windows; `Environment.NewLine`; stored paths keep `\`; the current directory is the game folder
while a project is open.

## 4. `libagsnative`: surface and conventions

Conventions: UTF-8 strings; `int` results, `0` ok, negative `AGSN_E_*` with a thread-local
`agsn_last_error()`; opaque handles with a matching `*_free`/`*_close`; pixel and palette data copied into
caller-owned buffers with an explicit stride; `agsn_api_version()`; symbols hidden except `agsn_*`;
**one process-wide recursive mutex around every call**, because `Common` is not thread-safe (the LZW codec
uses global buffers). Common's script code needs two host functions (`cc_format_error`,
`cc_get_callstack`), as the fork's `agsnative.cpp` provides them today.

| Area | Functions | Fork (`ags4`) need | Verdict for 6.7c |
|---|---:|---|---|
| init, version, last error, constants | 5 | `GetNativeConstant`: 115 call sites | conventions yes; constants **no**: generated by CMake from `NativeMethods.cpp` (a C++/CLI file 6.7f deletes); 6.7b ports them to C# with a drift guard |
| images (BMP/PCX/PNG via `Common/gfx/image_file`) | 6 | import/export with palettes kept | **yes**, as is |
| sprite file read | 8 | `LoadAllSpriteDimensions`, `GetSpriteBitmap*`, `GetSpriteInfo` | **yes**: slot info, slot as image, store flags |
| sprite file write | 6 | `SaveNativeSprites`, `ReplaceSpriteFile`, `ChangeSpriteNumbers` | **yes**: slot-by-slot writer with raw copy from an open file; the sprite set is owned by C#, not by a native cache |
| rooms (`.crm` load, getters, setters, save, background import, masks) | 38 | `ags4` rooms are XML + PNG; `.crm` is read only when a 3.99 project is upgraded (`ConvertAllRoomsFromCrmToOpenFormat`), written only by the room compile step (`SaveCrm`), which 6.7d gives to `crmbuild` | **shape only**: a read-only loader (~15 getters) with fork-defined structs; `ags4`'s `RoomStruct` differs (no messages, local variables, legacy interactions) |
| GUI preview | 3 | `DrawGUI(HDC ...)` | **approach yes**: host-flattened GUI and control descriptions, temporary `Common` GUI objects drawn with the game's fonts and sprites, 32-bit RGBA with alpha out; the structs are redefined from the fork's model (Opacity, `ags4` fields) |
| fonts (set, metrics, draw text, TTF size for height, SCI import) | 5 | the same five operations | **yes**, as is |
| templates (extract game/room) | 2 | `ExtractTemplateFiles`, `ExtractRoomTemplateFiles` | no: 6.7b ports the multi-file library reader to C# |
| not in the library | | sprite import conversion (`CreateBlockFromBitmap`, `sort_out_transparency`, `sort_out_palette`), ini, exe icon/version, script compile, view loop drawing | ags-for-mac runs the `spriteimport` **tool as a subprocess with a temporary project** per import; the fork links `Tools/data/sprite_utils` and exposes one `agsn_sprite_convert`; the rest as PLAN 6.7a/b/d and "to the UI" |

One library serving both editors is not realistic now: the branches differ (3.6 `RoomStruct`, GUI fields,
no Opacity), ags-for-mac waits for 4.0, and the room half is the larger part of its surface. What is
realistic and cheap: the same conventions, the `agsn_` prefix and the same function names and signatures
where the semantics match (images, sprite file, fonts, GUI entry points), so a later merge on `ags4` is
mechanical. Say so on #3177 when 6.7c is cut, not before.

Not to copy: the `System.Drawing.Design` shim namespace (redefining BCL types is fine for a port that
cannot edit upstream's files, wrong for a codebase that can); the regex source rewriter; the CMake parser
of a C++/CLI file.

## 5. Decisions

1. **6.0 is three steps, re-scoped** (PLAN.md 6.0): (1) property-grid editors and tabs to `AGS.Editor`
   with string-form attributes, the static delegates become direct calls, unused usings go; a guard test
   resolves every attribute string. (2) The plugin-facing surface (`EditorContentPanel`, `MenuCommand`,
   the four interfaces, the templates) loses WinForms and GDI types using ags-for-mac's replacements as
   the template; this breaks the editor plugin API (binary break, recompilation against the new
   `AGS.Types` with small source changes). *Accepted 2026-11-04:* the editor plugin API leaves C5; a new
   editor is a big step and better design wins over backward compatibility, so the interfaces are
   designed for the new editor, not shaped around the old plugins. (3) `Encoding.Default`
   and the other .NET Core differences, with the C5 round-trip test covering them (shared with 6.1).
   The old step 2 (`Font`/`Bitmap` fields to plain data) is dropped as a mismeasurement.
2. **Upstream gets step 1 as U.6** on `upstream/ags4`, the #1092 fix ericoporto described in 2020 ("the
   view part of `PropertyGridExtras` to the editor, the model stays"): no behaviour change in the WinForms
   editor, no plugin break. Step 2 is a design question on #1092 (the `IUserControl` idea is there already),
   not a PR, until the maintainers answer; step 3 goes with whichever of 6.1's pieces upstream takes (#1462).
3. **6.7c reuses `libagsnative`'s conventions and the image, sprite-file, font and GUI entry points**, not
   its room half, its constants or its structs (PLAN.md 6.7c). The fork adds the sprite conversion as a
   library function instead of a subprocess. Pixel-identical comparison against the C++/CLI `drawGUIAt`
   output is part of 6.7c's tests, which ags-for-mac does not have.
4. **After U.6's review (2026-11-04):** upstream put the editors in `AGS.Controls` and kept the static
   delegate hooks; "controls must not know `GUIController` or editor components" is now an enforced
   rule of 6.3. Open design note, parked until 6.0 step 2 is done and in any case before 6.8: the
   string attributes still let the model name its WinForms editors (`EditorTypeNames`, 112 `[Editor]`,
   11 `[PropertyTab]`), and the base-type string pins `System.Drawing 4.0.0.0`; semantic attributes in
   the model with a host-side `TypeDescriptionProvider` mapping them to editors would remove both.
   Not evaluated now: the refactor comes first, Avalonia last.
5. Nothing from ags-for-mac's patches is left to pick: #3144, #3163 and #3164 are in `ags4` and in the
   fork; the `LoadPNG` truecolor copy it noted uses `GetDataSize()` in the fork, so the 3.6 note does not
   apply.
