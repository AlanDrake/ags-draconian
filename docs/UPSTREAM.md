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

Status values: `picked` (applied unchanged), `adapted` (applied with changes), `present` (the
change was already in the fork: own work sent upstream, a PR branch taken before upstream merged
it, or a `master` commit that came in with a local branch), `skipped`. Merge commits get no row.

Reviewed through `e709f84a0` (`master` merged into `ags4`, 2026-11-04). Start the next review
at `git log e709f84a0..upstream/ags4`. The two dialog fixes in the first rows were taken ahead of
that review (2026-11-05); the other commits up to `2c15ef187` are not reviewed yet.

| Upstream commit | Status | Local commit | Notes |
|---|---|---|---|
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
| `ags4-tools-1-fixes` | open | #3186 | 9 commits: the fixes that let the tools read an AGS 4 project and compile its scripts (API level table, project version, `#module`, headers and macros for `agscc`, `<ScriptName>`, `scriptgen` order, GUI/room order, audio folders, FLAC) and `Script/compare_builds.py` |
| `ags4-tools-2-agfreader` | open | #3187 (on 1) | 8 commits: `xml_fields` table-driven reading, the AGS 4 project format, the `agfreader` migration, the drift guard (`agf_elements.txt`, C# and C++ tests), `agsbuild --strict` and `agsbuild_autotest.sh` |
| `ags4-tools-3-gamedata` | open | #3188 (on 2) | 4 commits: `acsetup.cfg`, game data as the editor writes it (no `--script-dir`: scripts stay separate files), interaction tables, `agspak` ender; the unescape commit was dropped, upstream's `ad38522e2` covers it |
| `ags4-tools-4-crmbuild` | open | #3189 (on 3) | 2 commits: `crmbuild` and its MSBuild/make files; parity 58 of 58 files with the upstream editor, auto-test 2380 of 2380; the package lists its assets in another order (fork-only editor fix `e69119490`) |
| `ags4-tools-5-po2tra` | open | #3190 (independent) | 2 commits: `po2tra` next to `trac` (PO → TRA, `-u` TRA → PO), `agsbuild` compiling `.po` |
| `ags4-fixcharinvsave` | open | #3191 | one commit `5ca3e45ce` (fork `d403d7236`): 4.0 saves before 4.0.0.28 read the legacy character inventory; the fork's 4.0.0.27 fixture fails on `upstream/ags4` with "Component data size mismatch" and restores with the fix, 4.0.0.30 restores on both |
| `ags4-agfreader-tests` | branch | — (#3147) | one commit: 47 AGFReader tests, one per `Game.agf` block, XML string → parser; passes on `upstream/ags4` and on `ags4-tools-2-agfreader`; not pushed yet |
| `ags4-save-roundtrip` | branch | — (#2814) | one commit: `savegame_test` target, 6 savegame round-trip tests (`play` state, 14 components, compressed, truncated); the script component is skipped (needs a loaded game); not pushed yet |
| `ags4-types-propertygrid` | open | PR (#1092) | U.6, 3 commits on `de5508362` (2026-11-04, reworked after the first review the same day): the 10 `UITypeEditor` classes, `BuildTargetUIEditor`, `SpriteFileNameEditor`, `StringListUIEditorControl`, `PropertyTabEvents` + `event.bmp` and `PaintColorBlock` (now on `ColorUIEditor`) move to **`AGS.Controls/PropertyGridExtras`** (16 renames); 112 `[Editor]` and 11 `[PropertyTab]` attributes use the string constants `AGS.Types.EditorTypeNames` (assembly `AGS.Controls`, fully qualified `UITypeEditor` base name); upstream's static GUI delegates and their setup in `GUIController`/`FontsComponent` stay untouched; unused WinForms/Design usings and `AGS.Types`' `System.Design` reference gone (`AGS.Controls` gains it for `FileNameEditor`); 6 NUnit tests resolve every attribute through `TypeDescriptor` (382 of 382). Review round 1 (ivan-mogilko): classes belong in `AGS.Controls`, and direct calls from controls into `GUIController` are bad design; both applied, the v1 branch kept as `backup/ags4-types-propertygrid-v1`. Left for step 2: `EditorContentPanel`, `ContentDocument`, `MenuCommand`, `IDockingContainer`, `IScriptEditorControl`. v1 property grid checked by the user in the running editor; v2 and the Debug build not yet |
| `ags4-fixes-1.3` | open | #3193 | U.7, 7 commits on `6ba515a06` (2026-11-05): the engine bugs found by the fork's 1.3 tests, one fix per commit, titles reworded to upstream's tone: `c4813c467` link slot 0 freed, `112402203` link index not `uint8_t`, `5a26083ee` backward move-list stage progress, `2253b6653` `GetGUIAt` any-hit in GUI space, `82f64474e` null handler of an unselected save component, `bae0f7b31` inventory saved in item order, `84d754a4e` `DisposeFreeItems` frees the free items. Two tests ported into `common_test` (`Common/test/characterinfo_test.cpp`, `resourcecache_test.cpp`, each fails without its fix); the five engine fixes keep their tests in the fork (`script_bytecode_test`, `movelist_test`, `gui_layout_test`, `savegame_components_test`) because upstream has no full-engine test target. Verified: MSVC Ninja build, `common_test` 104, `compiler_test` 563, `tools_test` 77, `engine_test` 3, `savegame_test` 6, C++11 syntax check on every changed file. Not verified: GCC/macOS, `ctest`. The user ran a game in the fork's editor and engine built from the same fixes (`1.3-engine-tests`): nothing evident. Opened 2026-11-05 |
