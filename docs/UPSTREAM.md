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

Status values: `picked` (applied unchanged), `adapted` (applied with changes), `skipped`.

| Upstream commit | Status | Local commit | Notes |
|---|---|---|---|
| | | | _no upstream changes taken since the fork point_ |

## Changes merged before the fork point

These came in as merges up to `fork-point`, and are not ledger entries:

- Upstream PR #3166 (Editor: replace font file command)
- Upstream PR #3118 (DrawingSurface primitive drawing functions)
- Local branches `ags4-poenhancements`, `ags4-fullranges`, `ags-fixantialiaseditor`, `ags4-voidreturnvoid`

## Changes sent upstream

Fixes and features from this fork offered to `adventuregamestudio/ags`, on branches based on
`upstream/ags4` and written in upstream's style (see "Upstream contributions" in
`docs/refactor/PLAN.md`). Status values: `branch` (prepared, not yet a PR), `open`, `merged`,
`closed`.

| Branch | Status | PR | Contents |
|---|---|---|---|
| `ags4-bugfixes` | merged | #3181 | Dialog(id, scriptName) crash; invariant culture in project files; `ikpFlac.dll` missing from the AnyCPU editor (fails upstream's Windows Packaging); stale character after `ParserState.Reset()` (false "unterminated literal" in translations); agsbuild Windows pipe hang; `agf2dlgasc` dropping the last line; inverted `show_fps`; `ImageFile::LoadPNG` partial copy; `crmpak -e ... script` crash; character and item interactions losing their script module (upstream `6148cb840`); agf2dta, crmpak and spriteimport Makefiles (since `63f2b8996`) |
| `ags4-buildtools` | closed | #3182 | Task 7.1 in upstream's formats and C++11 (24 commits, the four `master` tool commits first); stacked on `ags4-bugfixes`; parity 17 of 17 with the upstream editor on `ags4/auto-test`. Superseded 2026-11-03 by the five branches below (kept as `backup/ags4-buildtools-3182`): maintainers asked for smaller PRs and objected to `agf2dta --script-dir` and `agsbuild --json` |
| `ags4-tools-1-fixes` | branch | (#3182 closed) | 9 commits: the fixes that let the tools read an AGS 4 project and compile its scripts (API level table, project version, `#module`, headers and macros for `agscc`, `<ScriptName>`, `scriptgen` order, GUI/room order, audio folders, FLAC) and `Script/compare_builds.py` |
| `ags4-tools-2-agfreader` | branch | — (on 1) | 8 commits: `xml_fields` table-driven reading, the AGS 4 project format, the `agfreader` migration, the drift guard (`agf_elements.txt`, C# and C++ tests), `agsbuild --strict` and `agsbuild_autotest.sh` |
| `ags4-tools-3-gamedata` | branch | — (on 2) | 4 commits: `acsetup.cfg`, game data as the editor writes it (no `--script-dir`: scripts stay separate files), interaction tables, `agspak` ender; the unescape commit was dropped, upstream's `ad38522e2` covers it |
| `ags4-tools-4-crmbuild` | branch | — (on 3), ticket A | 2 commits: `crmbuild` and its MSBuild/make files; parity 58 of 58 files with the upstream editor, auto-test 2380 of 2380; the package lists its assets in another order (fork-only editor fix `e69119490`) |
| `ags4-tools-5-po2tra` | branch | — (independent), ticket B | 2 commits: `po2tra` next to `trac` (PO → TRA, `-u` TRA → PO), `agsbuild` compiling `.po` |
| `ags4-agfreader-tests` | branch | — (#3147) | one commit: 47 AGFReader tests, one per `Game.agf` block, XML string → parser; passes on `upstream/ags4` and on `ags4-tools-2-agfreader`; not pushed yet |
| `ags4-save-roundtrip` | branch | — (#2814) | one commit: `savegame_test` target, 6 savegame round-trip tests (`play` state, 14 components, compressed, truncated); the script component is skipped (needs a loaded game); not pushed yet |
