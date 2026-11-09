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

Reviewed through `7e829ab3f` on `ags4` and `3f39d095f` on `master` (2026-11-07). Start the next
review at `git log 7e829ab3f..upstream/ags4` and `git log upstream/ags4..upstream/master`. The
`master` commits not yet merged into `ags4` are listed in their own rows (status as of this review);
upstream's next master merge brings them into `ags4`, where they are reviewed again only if the
merge resolved them differently. The picked lines then go through the fork's clang-tidy gate: this
review's findings (braces, local names, a using-directive) were fixed in five mechanical commits
after the picks (`9fb5ad0c3` to `33e85fb26`, objects identical by `compare_objects.py`). Verified
on the whole: `prepush.py` every stage (GCC 1163 tests, Emscripten, libc++, MSBuild with the
Editor's NUnit), `compare_builds.py` 59 of 59 identical on the auto-test project.

| Upstream commit | Status | Local commit | Notes |
|---|---|---|---|
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
`docs/refactor/PLAN.md`). Status values: `branch` (prepared, not yet a PR), `open`, `merged`,
`closed`.

| Branch | Status | PR | Contents |
|---|---|---|---|
| `ags4-bugfixes` | merged | #3181 | Dialog(id, scriptName) crash; invariant culture in project files; `ikpFlac.dll` missing from the AnyCPU editor (fails upstream's Windows Packaging); stale character after `ParserState.Reset()` (false "unterminated literal" in translations); agsbuild Windows pipe hang; `agf2dlgasc` dropping the last line; inverted `show_fps`; `ImageFile::LoadPNG` partial copy; `crmpak -e ... script` crash; character and item interactions losing their script module (upstream `6148cb840`); agf2dta, crmpak and spriteimport Makefiles (since `63f2b8996`) |
| `ags4-buildtools` | closed | #3182 | Task 7.1 in upstream's formats and C++11 (24 commits, the four `master` tool commits first); stacked on `ags4-bugfixes`; parity 17 of 17 with the upstream editor on `ags4/auto-test`. Superseded 2026-11-03 by the five branches below (kept as `backup/ags4-buildtools-3182`): maintainers asked for smaller PRs and objected to `agf2dta --script-dir` and `agsbuild --json` |
| `ags4-tools-1-fixes` | open | #3186 | 10 commits (reworked 2026-11-08 after ericoporto's review of #3187, v1 kept as `backup/ags4-tools-1-fixes-v1-20261108`): the fixes that let the tools read an AGS 4 project and compile its scripts (API level table without its test, project version, `#module` as a mirror of the C# `ScriptAnnotations` plus the NUnit test `PreprocessorTests.ModuleDirective`, headers and macros for `agscc` with a comment on `ccGetExtensions2`, `<ScriptName>`, `scriptgen` as a minimal change to upstream's functions with `scriptgen_test.cpp`, GUI/room order, audio folders, FLAC) and `Script/compare_builds.py`. Force-pushed 2026-11-08, replies to the review pending |
| `ags4-tools-2-agfreader` | open | #3187 (on 1) | 2 commits (rebuilt 2026-11-08 after the review: no `xml_fields`, no `agf_elements.txt`, no drift guard, no `agsbuild --strict`; v1 kept as `backup/ags4-tools-2-agfreader-v1-20261108`): the AGS 4 project format as plain `ValueParser` reads (+173 lines in `agfreader.cpp` instead of +1467), `ReadInteractions` without the old-format fallback, then the 47 AGFReader tests (`ags4-agfreader-tests` folded in, #3147). The C++/CLI drift test ericoporto proposed is left for discussion. Force-pushed 2026-11-08, replies to the review pending |
| `ags4-tools-3-gamedata` | open | #3188 (on 2) | 4 commits (rebased 2026-11-08): `acsetup.cfg` with the driver id as upstream's `GetGfxDriverConfigID` writes it since `33bc8039a` (`d3d9`, not `D3D9`), game data as the editor writes it (no `--script-dir`: scripts stay separate files), interaction tables, `agspak` ender; the unescape commit was dropped, upstream's `ad38522e2` covers it. Force-pushed 2026-11-08, replies to the review pending |
| `ags4-tools-4-crmbuild` | open | #3189 (on 3) | 3 commits (reworked 2026-11-08): `crmbuild` with `room_xml.cpp` reading `data.xml` as plain reads per AGS.Types class (no `xml_fields`), its MSBuild/make files, `Script/agsbuild_autotest.sh` (moved here from branch 2, `crmbuild` in its tool list, no `--strict`); whole stack verified against the upstream editor: 58 of 59 files identical, the package lists its assets in another order (fork-only editor fix `e69119490`), auto-test 2380 of 2380, room files byte-identical to v1's. Force-pushed 2026-11-08, replies to the review pending |
| `ags4-tools-5-po2tra` | open | #3190 (independent) | 2 commits: `po2tra` next to `trac` (PO → TRA, `-u` TRA → PO), `agsbuild` compiling `.po` |
| `ags4-fixcharinvsave` | open | #3191 | one commit `5ca3e45ce` (fork `d403d7236`): 4.0 saves before 4.0.0.28 read the legacy character inventory; the fork's 4.0.0.27 fixture fails on `upstream/ags4` with "Component data size mismatch" and restores with the fix, 4.0.0.30 restores on both |
| `ags4-agfreader-tests` | branch | — (#3147) | one commit: 47 AGFReader tests, one per `Game.agf` block, XML string → parser; passes on `upstream/ags4` and on `ags4-tools-2-agfreader`; not pushed yet |
| `ags4-save-roundtrip` | branch | — (#2814) | one commit: `savegame_test` target, 6 savegame round-trip tests (`play` state, 14 components, compressed, truncated); the script component is skipped (needs a loaded game); not pushed yet |
| `ags4-types-propertygrid` | open | #3192 (for #1092) | U.6, 3 commits on `de5508362` (2026-11-04, reworked after the first review the same day): the 10 `UITypeEditor` classes, `BuildTargetUIEditor`, `SpriteFileNameEditor`, `StringListUIEditorControl`, `PropertyTabEvents` + `event.bmp` and `PaintColorBlock` (now on `ColorUIEditor`) move to **`AGS.Controls/PropertyGridExtras`** (16 renames); 112 `[Editor]` and 11 `[PropertyTab]` attributes use the string constants `AGS.Types.EditorTypeNames` (assembly `AGS.Controls`, fully qualified `UITypeEditor` base name); upstream's static GUI delegates and their setup in `GUIController`/`FontsComponent` stay untouched; unused WinForms/Design usings and `AGS.Types`' `System.Design` reference gone (`AGS.Controls` gains it for `FileNameEditor`); 6 NUnit tests resolve every attribute through `TypeDescriptor` (382 of 382). Review round 1 (ivan-mogilko): classes belong in `AGS.Controls`, and direct calls from controls into `GUIController` are bad design; both applied, the v1 branch kept as `backup/ags4-types-propertygrid-v1`. Left for step 2: `EditorContentPanel`, `ContentDocument`, `MenuCommand`, `IDockingContainer`, `IScriptEditorControl`. v1 property grid checked by the user in the running editor; v2 and the Debug build not yet |
| `ags4-fixes-1.3` | merged | #3193 | U.7, 7 commits on `6ba515a06` (2026-11-05): the engine bugs found by the fork's 1.3 tests, titles reworded to upstream's tone. Review (ivan-mogilko): the two script-link fixes squashed, `movelist_test.cpp` added; merged as `2c15ef187` the same day with `28230f5b9` link index type and link slot 0, `50a582c18` backward move-list stage progress, `4e6e012d9` `GetGUIAt` any-hit in GUI space, `4f82cbf69` null handler of an unselected save component, `0be5a5b51` inventory saved in item order, `c0f8660ea` `DisposeFreeItems` frees the free items, `69b38a467` the 19 MoveList tests. Two tests ported into `common_test` (`Common/test/characterinfo_test.cpp`, `resourcecache_test.cpp`, each fails without its fix); the engine fixes keep their other tests in the fork (`script_bytecode_test`, `gui_layout_test`, `savegame_components_test`) because upstream has no full-engine test target. Follow-up by upstream: `e448e19ba` moved the move-list operations out of `route_finder` into `MoveListOp` so `movelist_test.cpp` links in `Engine.App.Test.vcxproj` without the Allegro-backed walkable mask (taken back into the fork, see the ledger). Verified before opening: MSVC Ninja build, `common_test` 104, `compiler_test` 563, `tools_test` 77, `engine_test` 3, `savegame_test` 6, C++11 syntax check on every changed file. The user ran a game in the fork's editor and engine built from the same fixes (`1.3-engine-tests`): nothing evident |
| `master-fixes-1.3` | merged | #3194 | U.7 backports to `master`, asked for in the #3193 review: `6a2454410` null handler of an unselected save component, `1da13db36` `DisposeFreeItems`. Merged 2026-11-05; reach `ags4` with upstream's next master merge |
| `master-fixspritealpha` | merged | #3196 | U.9, 2 commits on `upstream/master` (the code is the same in `master` and `ags4`; the two commits apply onto `upstream/ags4` without conflicts), 2026-11-07: `497450361` `Convert8BitARGBTo32` steps its rows in bytes (fork `e54f923d9`), `b9911c7ac` the test `SpriteImport.DirectImportConvert8WithAlphaTo32` (fork `e217707d3`). Verified: MSVC Ninja Release build of `tools_test` from `master`, 81 of 81; without the fix the new test fails 3 runs of 3; C++11 syntax check on both files. Not verified: GCC/macOS, Debug, `ctest`. Merged into `master` as `3f39d095f` (2026-11-07); reaches `ags4` with upstream's next master merge. Branch deleted |
