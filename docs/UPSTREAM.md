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
