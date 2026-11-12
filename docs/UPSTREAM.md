# Upstream ledger

This repository is a **hard fork** of [adventuregamestudio/ags](https://github.com/adventuregamestudio/ags)
(branch `ags4`). The fork point is tagged `fork-point` (`4cd88beba`). Upstream is no longer merged
wholesale: refactors in `docs/refactor/PLAN.md` rename, move and reformat code, so merges would
conflict everywhere.

The `upstream` remote stays configured for reading only.

## Taking an upstream change

1. `git fetch upstream` and look at the change: `git log fork-point..upstream/ags4`.
2. Apply it with `git cherry-pick -x <hash>`. Adapt by hand if the touched code has been
   renamed, moved or reformatted.
3. Add a row below. Skipped changes get a row too, with the reason, so nobody reviews them twice.

The readers of `Game.agf` and room `data.xml` (`Tools/data/agfreader.cpp`, `room_xml.cpp`) do not
cherry-pick in either direction since 2026-11-08: upstream reads one `ValueParser` line per property
(ericoporto's review of #3187), the fork reads through tables (`xml_fields.h`) with the drift guard, a
deliberate difference (AGENTS.md "Facts that differ from upstream"). A property upstream adds there is
ported as a table row: `.compare/helpers/port.py rows Tools/data/agfreader.cpp <upstream commit or range>`
prints the rows to add and the defaults to move into the structs' initializers. The other way,
`port.py plain <file>` prints a fork file with its rows as plain reads, and `port.py apply` converts
them while porting a commit (`.compare/helpers/xmlrows.py`).

Status values: `picked` (applied unchanged), `adapted` (applied with changes), `present` (the
change was already in the fork: own work sent upstream, a PR branch taken before upstream merged
it, or a `master` commit that came in with a local branch), `skipped`. Merge commits get no row.

Reviewed through `d171879df` on `ags4` and `7b9f8f906` on `master` (2026-11-11). Start the next
review at `git log d171879df..upstream/ags4` and `git log upstream/ags4..upstream/master`. The
`master` commits not yet merged into `ags4` are listed in their own rows (status as of this review);
upstream's next master merge brings them into `ags4`, where they are reviewed again only if the
merge resolved them differently. The picked lines go through the fork's clang-tidy gate: the
2026-11-07 review's findings (braces, local names, a using-directive) were fixed in five mechanical
commits after the picks (`9fb5ad0c3` to `33e85fb26`, objects identical by `compare_objects.py`).

A change that touches files the fork has reformatted is taken as a 3-way merge rather than a
cherry-pick (2026-11-11, #3184): clang-format upstream's file at the last reviewed commit and at the
tip with the fork's `.clang-format`, then `git merge-file` the fork's file against the two; the
formatting drops out and only the fork's own edits can conflict (two hunks out of twelve files for
#3184). `.compare/helpers/merge3.py --base <reviewed> --theirs <commit> <files>` (local, git-ignored)
does the formatting and the merge; the merged files are copied into the tree and resolved by hand.

| Upstream commit | Status | Local commit | Notes |
|---|---|---|---|
| `0aa0c8e8d`, `ff38d03ac`, `efbeb3a54`, `b0fac321e`, `bf32091c4` | adapted | `ae9dc935d` | #3184, 4-bit WFN fonts (Gal Shemesh): the extended `WGT Font File 2` format with a glyph table and a sparse character table, shaded rendering in 32-bit (RGB steps of 17 per shade) and 8-bit (hue-matched palette entries, `set_wfn_palette_eligibility` from `game.paluses`), the stencil mode shared by the renderers for auto-outlines (`wouttextxy_stencil` replaces the `AGS_FONTOUTLINE_MOREOPAQUE` block), `WFNFont` behind `unique_ptr`, the Editor's font preview in 32-bit for WFN. Taken as one 3-way merge of the five commits; the two conflicts were the fork's 3.5 deletion of `woutprintf` (`ac1b2e4bd`) below `wouttextxy`, where upstream appends `wouttextxy_stencil`. Adaptations: `RenderChar` is a template on the glyph depth, dispatched once per glyph, so the 1-bit loop stays the old one where upstream tests `BitsPerPixel` for every pixel (STYLE.md "Performance"); the two readers are split along their documented steps (`ReadExtCharacterMap`, `CollectExtGlyphOffsets`, `ReadExtGlyphs`; `CollectClassicOffsets`, `ReadClassicGlyphs`) for the 120-line limit, the classic one because its rename made it a new function to the tidy gate; fork names (`kWfnExtCharEntrySize`, `glyph` for a `WFNChar`, `text`, `font_number` in the new methods). Added `Common/test/wfnfont_test.cpp` (7 tests: extended and classic reads, shared glyphs, flags, duplicate codes, bad offsets, truncated pixels), upstream has none. Left as upstream has it: `RenderChar`'s `BlendPixel` branch draws `text_color`, not the shaded `pixel_color`, so a 4-bit font loses its shades under a blend mode |
| `0f979b256`, `156d6adaf` | picked | `ae9dc935d` | TTF font preview text and outline colours in `agsnative.cpp`; part of the same merge |
| `d171879df`, `1c00f79ff` | present | `bb363c1b5`, `2c1749b13` | the fork's Allegro fixes sent upstream: `fade_interpolate` weights add up to 64, `create_rgb_table` on 8-bit components; the fork's copies keep a comment on the table's cells and a whitespace fix |
| `1592a73ef` | skipped | — | upstream README on the ags3/ags4 plans; the fork's README is its own |
| `7b9f8f906` (`master`) | adapted | `969fabe62` | a dialog whose script changed the room is stopped (`schedule_dialog_stop`), `_doRunDialog` starts at `RUN_DIALOG_STAY`; the fork's braces |
| `e936449ac` (`master`) | skipped | — | `FixupCharacterLoopAndFrame` kept for <= 3.6.2.12 games; the fork loads 3.99+ only (3.4-1) |
| `7e829ab3f`, `a2541e42e` | present | `ea1e94ae3` | `ScriptUserObject` leaked on restore; the fork's 1.5b-5 fix (`globalDynamicStruct.Unserialize`) is the same change; the second hash is the `master` twin |
| `4d4b2c9cd` | present | `42a6f5c00` | `FT2_BUILD_LIBRARY` in Common.Lib's Debug x64 configuration came with the Windows 7 commit |
| `1fbb3d84e`, `33da6c2d0` | adapted | `650f1dbc8` | `TransformStream::EOS()` waits for the transform to report its end; the DeflateStream tests move from `stream_test.cpp` to the new `deflatestream_test.cpp` with 13 tests (CMake + `Common.Lib.Test.vcxproj`); the fork's formatting and standard includes. Upstream's `TestDeflateStream_SemiUniqueSeq` loops to `length % 4` (probably meant `/ 4`), left as upstream has it |
| `4cc80b597`, `fc993ca48` | adapted | `26507d01a` | debug log in the save components behind `DEBUG_SAVECOMPONENTS (0)`; fork formatting. The compiled-out printf formats do not match their argument types, left as upstream has it |
| `a84f4c6ce` | adapted | `396187a2d` | `OPT_GAMEFPS` defaults to 40 for 4.0 data before 4.0.0.14; the fork loads 3.99+ only, so only the 4.x check is added (3.4-1 removed the 3.x branch upstream patched) and `kGameVersion_351` is not added |
| `1c53c848a`, `d6b1fea08` | adapted | `5a94d2833` | no "compiled successfully" popup on a test run: the fork's `BuildGame(forceRebuild, forRun)` passes `!forRun` where upstream has `!createMiniExeForDebug` |
| `2ed795ebf` | skipped | — | version tag Alpha37; the fork's `version.json` is 4.00.00.34 "drac" |
| `e448e19ba`, `cc060f46d` | adapted | `d6e20972b` | Ivan's follow-up to #3193: the move-list operations leave `route_finder` (`Pathfinding`, Allegro `Bitmap` mask) for `movelist.cpp` as `MoveListOp`, so `movelist_test.cpp` links in `Engine.App.Test.vcxproj`; one `[move]` commit with the 19 call sites, `BitmapData::GetSize()` and the TODO comments; `FindRoute` calls `MoveListOp::CalculateMoveList` by name instead of a `using namespace`; the fork's `.vcxproj` already listed the test |
| `7695cc67f` | picked | `6f1dbd09f` | `GetPixelsCopy`/`SetPixels` declared under 3.6.3 in `agsdefns.sh` |
| `c4fc2dda8` | adapted | `569eda92c` | dialog folder created, new scripts saved at once, IDs resaved after a delete; the fork has no `CachedConvertedScript` resets (it reconverts dialog scripts on every build), the rest equals upstream's file |
| `de5508362`, `f0501a5d7` | adapted | `da2a4364b`, `aa1ff3c42` | 4.0.0.33 notes in `Changes.txt` (the fork keeps upstream's changelog); `f0501a5d7`'s month bumps in `version.json` and `AssemblyInfo.cs` left out, the fork has its own version stamp |
| `916fd2514` | present | `45d8aee73` | room backgrounds of 3.6.3.15+ rooms: the fork's 1.5a fix of the same never-true check; upstream's FIXME on the `kRoomVersion_399` ordering not taken |
| `a05a8b590` | picked | `d6d6decba` | "Linear" filter for the Software driver |
| `33bc8039a` | adapted | `be62a5caa` | `GraphicsDriver.D3D11` in the Editor's default setup, filters and Linux/Web fix-ups; the config IDs keep the fork's casing (`Software`, `D3D9`, `D3D11`, `OGL`, upstream lowercased them) because `Tools/data/game_utils.cpp` writes the same `acsetup.cfg` for `agsbuild` and gets `D3D11` in the same commit, with a `tools_test` case |
| `ef01354b3` | picked | `af604bbda` | `RegisterMissingManagedObjects` moved to `game_init.cpp`, registers GUI controls too |
| `69b38a467` | present | `8044b8ad5` | #3193: the 19 MoveList tests |
| `c0f8660ea`, `1da13db36` | present | `dfdca2ace` | #3193 and its `master` backport (#3194); upstream's version carries `Common/test/resourcecache_test.cpp` |
| `0be5a5b51` | present | `29c575b5d` | #3193; upstream's version carries `Common/test/characterinfo_test.cpp` |
| `4f82cbf69`, `6a2454410` | present | `ab44aeb70` | #3193 and its `master` backport (#3194) |
| `4e6e012d9` | present | `d7b6a4275` | #3193 |
| `50a582c18` | present | `c095e77cc` | #3193 |
| `28230f5b9` | present | `7ecac9047`, `db8be439f` | #3193, the two fork commits squashed at the review's request |
| `5ca3e45ce` | present | `d403d7236` | PR #3191 |
| `c5b97977b` | present | `c5b97977b` | PR #3176, same commit (the local branch `ags4-voidreturnvoid` was merged before the fork point) |
| `c13410d01` | adapted | `9daf5c317` | `--runfromide` paths quoted with escaped double quotes; all five of the fork's entries (debug rooms and fonts directories too) |
| `4bdabc986` | adapted | `ce558e377` | `StrUtil::SplitPreservingDoublequoted` and `parse_asset_dirs`; upstream's early return for an empty string lacks `return` (same result by fall-through), written with it |
| `6c03a29f4` | adapted | `76f1f4214` | Theora: warns about YUV 422/444 that APEG cannot decode; the apeg part unchanged, `theora_player.cpp` in fork formatting |
| `bb4b8e56e` | skipped | — | `master` only: save format of the 2.72 interaction lists, which `ags4` and the fork do not have |
| `51e032ae3`, `e1d952092` | skipped | — | `master` only: `miniz_ex` split so a system miniz builds (`AGS_USE_LOCAL_MINIZ`); the fork's local-miniz build has the same breakage, take it when it reaches `ags4` (Debian builds use the bundled miniz) |
| `497450361`, `b9911c7ac` | present | `e54f923d9`, `e217707d3` | `master` only: PR #3196, fork work |
| `6ba515a06` | picked | `07bf2fa82` | dialogs as separate files, broken since `6674e9b39` and `2e99ab58c`: files created, deleted and renamed with the dialog, XML saved from the Dialog editor, copy and paste, dialog files in game templates; dialog scripts are converted on every build for now (upstream TODO: file timestamps). `DialogEditor.cs`, `DialogScriptConverter.cs` and `Utilities.cs` conflicted as whole files, hunks re-applied with `git apply`, diff identical to upstream |
| `b9e8aeb0c` | picked | `06be6a1f0` | Dialog editor marked as modified; `DialogEditor.cs` as above |
| `d84e0aa54`, `2efc93ede` | adapted | `0e2937af5` | PR #3183, GUI previews composited with alpha before GDI draws them; replaces the fork's `ags-fixantialiaseditor` (reverted in `72e1af39b`); ags4's blender API as the merge `e709f84a0` resolved it |
| `35ad81db0` | present | `bef3b8dfc` | fork work sent upstream (`GetKeyValue`) |
| `ad38522e2` | adapted | `e0f775fed` | tests only: the writer change is the fork's `161d22ed9`; the expectations are the merge's (25-character item name, no old-style `\[`) |
| `14ea16de4` | present | `6f357014e` | fork work sent upstream |
| `10994b4d6`, `0cc58a798` | present | `17c44ffc0` | PR #3181 (fork) and its `master` backport |
| `fc59f7a92`, `20fb8d6f0` | present | `e348e5962` | PR #3181 and its backport |
| `95ec2c954`, `27878e351` | present | `3f239ff31` | PR #3181 and its backport; the fork had it from `agfexport: game-cfg command` |
| `576c5b411`, `557f2d41b` | present | `bc67e33ec` | PR #3181 and its backport |
| `fd22bff6a`, `1e01615a7` | present | `81ebb7067` | PR #3181 and its backport |
| `8356f7e1c`, `2b38ef183` | present | `12aae49ed` | PR #3181 and its backport |
| `a9cf72e09` | picked | `9eca84509` | PR #3181: the fork's Makefiles listed a file that does not exist and missed three |
| `f674bf3c2` | picked | `cdbcb1204` | PR #3181; needs `6148cb840` below, which the fork lacked |
| `0b42ce87f` | present | `cf00b31a9` | PR #3181 |
| `e62310bb9` | present | `78c7cd17b` | PR #3181 |
| `bd9a3c085` | present | `53cfd4335` | PR #3181 |
| `2822182da` | present | `f2db53b68` | the fork fixed the same test on its own |
| `a0add5761` | adapted | `18850bd82`, `f85750631` | PR #3166: the fork's copy predates the font file watcher; the rest (fonts of a replaced file updated through `UpdateAllFontsForFile`) applied by hand |
| `60f40b5fc` | picked | `45797dc15` | |
| `2f71f5b34`, `1eb7e103c` | adapted | `b76674658` | font file watcher (`ags4` commit and its `master` twin); `EditorEvents.cs` and `GUIController.cs` hunks re-applied by hand, usings as upstream |
| `e76b37906` | picked | `609e11f11` | |
| `eb7e8518b` | present | `4cd88beba` | the fork point commit itself, rebased copy in PR #3118 |
| `3eccc7c8d` | present | `9c54c9cae` | PR #3118, merged as `812cb84bb` |
| `ebbb2fd01` | present | `c49e031bd` | PR #3118 |
| `88227426d` | present | `d9599561e` | PR #3118 |
| `e745e25cc` | adapted | `fcb7e1eda`, `fe748a6a8` | PR #3118; upstream amended the commit after the fork took it: `DrawTriangleOutline` ends with `FinishedDrawingWithBrush()` |
| `e63d1b536` | present | `17fa211bb` | PR #3118 |
| `e5679342b` | picked | `68e4f361f` | |
| `6148cb840` | picked | `6b4669c67` | |
| `6a0591d7c` | present | — | `C_EXTENSIONS YES` was in `ags4` before the fork point |
| `00a0021bb` | present | `5485afaa5` | `master`, came in with `ags-fixantialiaseditor` (`b65ecf389`) |
| `3520ac6b6` | present | `488ccf636` | `master`, via `b65ecf389` |
| `63ebe34fd` | present | `947effd81` | `master`, via `b65ecf389` |
| `17c1b2490` | present | `5820ffe7b` | `master`, via `b65ecf389` |
| `c47930a1b` | present | `6c2b5bfff` | `master`, via `b65ecf389` |
| `cbba9a613` | present | `e85f7e0ee` | `master`, via `b65ecf389` |
| `6ef0f9e6f` | present | `885ada5ba` | `master`, via `b65ecf389` |
| `3d31f2e9a`, `381dce491`, `b0b8318e0`, `79df0de50` | skipped | — | upstream's `AGENTS.md`, AI policy and pull request template: the fork has its own `AGENTS.md` and process |
| `0accc4250` | present | `66d181ccb` | `ags4` had it before the fork point under another hash |
| `81f0a2cfd` | present | `fdbb09652` | `master`, via `b65ecf389` |
| `ec5ae9a8f` | present | `de38f0d6b` | `master`, via `b65ecf389` |
| `89a48f169` | present | `ce86c9fd6` | `master`, via `b65ecf389` |
| `6df90bfa5` | skipped | — | `freetype-2.1.3` does not exist in `ags4` |
| `137f9915e` | present | `959af3453` | `master`, via `b65ecf389` |
| `0ddc44b1a` | present | `693da40a8` | `master`, via `b65ecf389` |
| `ec86636a5` | present | `bb776e273` | `master`, via `b65ecf389` |
| `e5f9eab3a` | present | `83ae28a17` | `master`, via `b65ecf389` |
| `084932f02` | adapted | `2f3b574e5` | the comment points at `AGENTS.md` only |

## Changes merged before the fork point

These came in as merges up to `fork-point`, and are not ledger entries:

- Upstream PR #3166 (Editor: replace font file command)
- Upstream PR #3118 (DrawingSurface primitive drawing functions)
- Local branches `ags4-poenhancements`, `ags4-fullranges`, `ags-fixantialiaseditor` (reverted in
  `72e1af39b`, upstream PR #3183 replaces it), `ags4-voidreturnvoid`; `ags-fixantialiaseditor` was
  based on `master`, which is how the `master` commits marked `via b65ecf389` above got here

## Changes sent upstream

Fixes and features from this fork offered to `adventuregamestudio/ags`, on branches based on
`upstream/ags4` and written in upstream's style (see "Upstream contributions" in
`docs/refactor/PLAN.md`). Status values: `branch` (prepared, not yet a PR) and `open`; a row goes
once its PR is merged or closed (git history keeps it).

| Branch | Status | PR | Contents |
|---|---|---|---|
| `ags4-tools-2-agfreader` | open | #3187 | 2 commits on `upstream/ags4` `c64e3ed03`: the AGS 4 project format as plain `ValueParser` reads, `ReadInteractions` without the old-format fallback, then the 47 AGFReader tests (#3147). Review round 3 (ivan-mogilko, 2026-11-11) answered 2026-11-12 and force-pushed with branches 3 and 4: `Common::BlendMode`, `Common::GraphicFlip` and `CursorRole` for the fields that were `int`s, the writer converts the flip with `GfxDef::GetFlagsFromFlip`, `AGFReader::DialogsDir`, `ValueParser::ReadInt` through `StringToInt64` only, `Math::Clamp`; replies to post in `.compare/u3-drafts/review-replies-20261112.md`. Left: a trim option for `String::Split`, the dialogs folder as an argument, the C++/CLI drift test ericoporto proposed (for discussion). `AGFReaderBlocks.GameSettingsDefaults` expects `PixelPerfect` false; the Editor's default is true |
| `ags4-tools-3-gamedata` | open | #3188 (on 2) | 4 commits: `acsetup.cfg` with the driver id as upstream's `GetGfxDriverConfigID` writes it (`d3d9`), game data as the editor writes it (scripts stay separate files), interaction tables, `agspak` ender |
| `ags4-tools-4-crmbuild` | open | #3189 (on 3) | 3 commits: `crmbuild` with `room_xml.cpp` reading `data.xml` as plain reads per AGS.Types class, its MSBuild/make files, `Script/agsbuild_autotest.sh`. Round 3 (2026-11-12): the room object's blend mode is `Common::BlendMode`, limits from `game/roomdata.h`, `Utf8::GetChar` instead of a private `DecodeUtf8` (`Utf8ToCp1252` stays: `ConvertUtf8ToAscii` needs a CP-1252 locale). Whole stack 2026-11-12: `tools_test` 152, `common_test` 116, `compiler_test` 594, C++11 check, auto-test built by the tools 2380 of 2380, game output byte-identical to the previous version's; no Editor parity (upstream's Editor now packs scripts into `game28.dta`). Known difference against the Editor: the package's asset order |
| `ags4-d3d9-halfpixel` | branch | — | one commit `05e6cccd0` on `c64e3ed03` (fork `4e94f0c5f`), 2026-11-12, in the worktree `../ags4-d3d9fix`: Direct3D 9's half-pixel offset per render target, after the batch transform (it depended on the window size and grew with the camera's zoom; found from a maintainer's viewport test). Verified: MSVC Ninja build, upstream's auto-test 2380 of 2380 on D3D9/D3D11/OpenGL at window x1 and x3 with a zoom probe (D3D9 now equal to D3D11 at both sizes); not pushed |
| `ags4-save-roundtrip` | branch | — (#2814) | one commit: `savegame_test` target, 6 savegame round-trip tests (`play` state, 14 components, compressed, truncated); the script component is skipped (needs a loaded game); not pushed yet |
| `ags4-types-propertygrid` | open | #3192 (for #1092) | U.6, 3 commits on `de5508362`: the 10 `UITypeEditor` classes, `BuildTargetUIEditor`, `SpriteFileNameEditor`, `StringListUIEditorControl`, `PropertyTabEvents` + `event.bmp` and `PaintColorBlock` (on `ColorUIEditor`) move to `AGS.Controls/PropertyGridExtras`; 112 `[Editor]` and 11 `[PropertyTab]` attributes use the string constants `AGS.Types.EditorTypeNames`; upstream's static GUI delegates stay; `AGS.Types` loses its `System.Design` reference; 6 NUnit tests resolve every attribute through `TypeDescriptor` (382 of 382). Left for step 2: `EditorContentPanel`, `ContentDocument`, `MenuCommand`, `IDockingContainer`, `IScriptEditorControl`. Not yet checked in the running editor: the current version, the Debug build |
