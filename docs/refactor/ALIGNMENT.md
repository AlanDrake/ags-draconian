# The fork's plan against upstream's direction

Written 2026-11-03 after reading upstream's open issues, milestones, wiki and the engine/editor
forum boards (sources and dates in `docs/context/`). Purpose: find where PLAN.md repeats, contradicts
or can feed upstream's own plans, and what to change. "Upstream" means `adventuregamestudio/ags`
on 2026-11-02; references are issue/PR numbers there.

## Decisions D1-D7

| Fork decision | Upstream position | Assessment |
|---|---|---|
| D1 load 3.99+ game data only | `ags4` reads game data from 3.6.0.21 (`kGameVersion_LowSupported`), rooms from 3508; the Editor imports projects saved by 3.6.0+; running pre-4.0 compiled games is not guaranteed (decisions P2) | Fork-only choice, legitimate for a hard fork. Cost: every `Common/game` deletion in 3.4 moves the fork away from upstream's readers, so later `U.x` ports touching `main_game_file.cpp` / `room_file.cpp` get harder. Keep, but do 3.4 **after** the remaining upstream offers in that area (#3182 review, AGFReader tests). |
| D2 drop PSP, keep iOS/FreeBSD | PSP unsupported upstream too (README); iOS has no CI upstream either | Aligned. 3.1 is safe. |
| D3 remove the legacy compiler | #3172 (ivan-mogilko, 2026-10): remove it from `ags4` for 4.0 Beta; #3178 first add a non-parallel compile option for debugging | Aligned, and the fork did it first (3.3 done). The removal commits are not portable as-is (fork layout: `script2` → `script`), but the checklist of what had to go (editor "AGS SCOM 3 EXT", `LEGACY_COMPILER`, CI steps, `Compiler/Makefile`) is useful for upstream's #3172. Offer it as a comment on #3172, not a PR, unless asked. |
| D4 Avalonia editor; WinForms in maintenance | No cross-platform editor is planned; UI framework undecided; the team will not adopt a whole new editor. Welcome in small PRs: `AGS.Types` without WinForms (#1092), `AGS.Native` platform-neutral or gone, building through the tools (P6). a-riddle-fan's `ags-for-mac` (3.6.x, .NET 10 + Avalonia, `AGS.Native` replaced by a CMake C library, ~60 `AGS.Types` files adapted) already exists, AI-assisted, not offered upstream as is. | Tasks 6.0, 6.3, 6.7 are exactly what upstream welcomes; 6.8 (Avalonia front end) is fork-only. **Before 6.7c, read `ags-for-mac`'s C library and its `AGS.Types` diff**: it is a second implementation of the same split, on 3.6.x, and shows which 60 files need touching. Coordinate on #1092/#3177 rather than inventing the split twice. |
| D5 CMake only for native code | Upstream keeps CMake, MSBuild and the hand Makefiles; CI depends on all three; `debian/` packaging uses `Engine/Makefile` | Fork-only. Fine, but it is the single biggest source of port friction: every `U.x` port must re-add Makefile and `.vcxproj` entries (already learned with `mk_linktest.py`). 3.2 stays low priority until the tool work is upstreamed. |
| D6 Windows 7+ | Editor must support Windows 8 at least (edmundito, #3177) | Compatible. |
| D7 build through the C++ tools, delete the C# build code | #2316 is the master ticket; ivan-mogilko 2026-10-30: "compilation and translation should use the command-line tools"; the Editor switch is "a separate task" | Aligned; the fork is ahead. Two of #2316's open items ("Room file compiler", "PO translation compiler", both "ticket TBD") are done in the fork and sit in PR #3182. **File the two upstream tickets** (referencing #2316 and #3182) so the work has a home; split #3182 along them if review stalls. Upstream review will ask for C++11, Makefile/MSBuild entries, and logging through the Common log system (#2530, ivan-mogilko) rather than ad-hoc output: `agsbuild --json` (7.1h) may need to become a log sink. |

## Phases 1-7 against upstream tickets

- **1.2 contract tests.** C5 (`Game.agf` round trip) and the AGFReader drift guard are what #3147
  asks for; C2 (save round trip) is #2814 (ivan-mogilko's 5-step plan: generate data, save, restore
  into a second instance, compare). Write these in upstream style (C++11, GoogleTest in the existing
  test targets) and they are portable. The C3 script-API snapshot has no upstream equivalent; keep.
- **1.3 engine unit tests.** Upstream has 3 engine test files and the same blocker the fork has:
  globals (`#1834` fixed Common, not Engine). Any fixture that constructs engine state without the
  globals is portable in spirit, not in code (fork is C++17).
- **2.x formatting.** Upstream discussed `.clang-format` (#2211, 2023) and did not adopt it. The
  fork's whole-tree reformat is the reason ports need `port.py`; accepted cost.
- **3.4 legacy data paths.** See D1. Also note upstream's own Beta rule (P3): nothing in 4.0 may
  require manual project upgrades any more, so upstream will not follow.
- **4.x names.** Upstream's naming convention (wiki) matches STYLE.md except file names (upstream
  has no rule). Renames are fork-only; `port.py` ignores them when matching hunks.
- **5.1 context structs** (`GameData`, `GameState`, `RoomContext`, `EngineServices`). Upstream's
  #2058 wants the same split with different words: data structs in Common (`CharacterInfo` data
  only, `GameSetupStruct` data only, a `GameData` holding lists) and runtime classes in Engine
  (`Character` = data + `CharacterExtras`, `Game` = `GameSetupStruct` + `GameState` + runtime
  lists). **Use upstream's names** where they exist so that the two trees stay recognisable.
- **5.2 template binder** replacing `Sc_*`/`API_*`. Upstream #1223 lists the options; ericoporto
  prefers templates and rejects a runtime switch. The fork's C++17 `auto` binder cannot be ported
  (C++11), so this is fork-only; keep the registered names identical (they are the ABI).
- **5.3 `Engine/ac` reorganisation.** Upstream's draft layout (wiki "AGS 4 Draft: major engine
  refactor") is `system/main/script/data/game/gfx/audio/video/util`; the fork's is
  `world/navigation/dialog/presentation/ui/input/audio/script_api/runtime`. Neither is decided
  upstream. Moving files is the change that hurts ports most (hunk matching by path). Either adopt
  upstream's draft names or postpone 5.3 until the `U.x` offers are closed.
- **5.6 `quit()` → errors.** Upstream #1833 wants the same for Common ("replace with returning an
  error"); portable if done in Common first, in upstream style.
- **6.7d/7.1l scripts as separate assets.** Upstream suggested exactly this for the room compiler
  (F5). Aligned.
- **7.1k `_obj/` intermediate directory.** No upstream decision; the "fixed folder names" rule (F3)
  fits. Propose it in an issue before the Editor side (6.7d) depends on it.
- **7.1n MO files.** Upstream's PO plan keeps `.tra` as the compiled format and left contexts open
  (#1780). Low value, likely contested; keep it as "future" or drop.
- **Full ranges (`ags4-fullranges`, PR #3119) and PO enhancements (`ags4-poenhancements`).**
  Both are 4.0 Beta checklist items (#2549, #2856; #1780 items 3-4), so the timing is right. Known
  friction on #2549: ivan-mogilko prefers a script-API switch or a compat plugin for the old 0-100
  functions; the fork prefers documentation shims. Expect that discussion on #3119.
- **Fake gamma / hardware gamma removal.** Only AlanDrake's wish in #3115; edmundito suggested a
  separate issue. File it if it should be upstream.
- **Frame scripts (`ags4-framescript`).** Upstream 4.0 already has `ViewFrame.EventName` and
  `OnFrameEvent` (Changes.txt). Check whether the fork branch is superseded before merging it.

## Process

- Upstream now expects **issue first** for anything beyond a fix (P5) and dislikes large PRs
  (#3182 is 24 commits; ivan-mogilko already asked for `--script-dir` to go). Offer one #2316 item
  per PR, each behind its ticket.
- The AI policy (PR #3025, merged 2026-11-03) forbids generated **documentation prose** and user-facing text
  in upstream. The agent-facing files (`AGENTS.md`, `docs/context/`) are contributor tooling, but a
  maintainer may read them as documentation: when offering them, say they were AI-assisted and
  human-reviewed, as #3025 itself was (edmundito disclosed Claude use). The script-API reference
  for agents in `ags-manual-source` (branch `agents-reference`) is generated from `agsdefns.sh`
  and not part of the built manual; keep it in the fork or a separate repository unless the
  maintainers ask for it (ivan-mogilko suggested a separate repo for proposal material in #2786).
- PLAN.md is ~20k tokens and is read at the start of every session. Most of it is status logs.
  Move the "*Status*" paragraphs of finished tasks into `docs/refactor/LOG.md` (dated, per task)
  and keep PLAN.md to plan + a status table; `AGENTS.md` already tells sessions to read only the
  current task.

## Recommended changes to "Next up"

Applied to PLAN.md on 2026-11-03 (ground rules 7-8, tasks 0.6, 0.7, U.3, U.4, notes on 1.2, 5.1,
5.3, 6.7c, 7.1n, D4, and the reordered "Next up"). Kept here as the reasoning.

1. Keep 1.2 first, written in upstream style; add "offer C5/AGFReader tests as #3147" to it.
2. Add a task **U.3**: file the "room compiler" and "PO compiler" tickets under #2316, split
   #3182 along them if asked, and adapt `agsbuild` diagnostics to the Common log system (#2530).
3. Before 6.7c: a short study task on `ags-for-mac` (C library surface, `AGS.Types` diff) and
   #1092; decide whether 6.0 steps 1-3 can be offered upstream as the #1092 fix.
4. Postpone 3.2 and 5.3 until U.3 is closed; when 5.3 starts, use upstream's draft directory
   names.
5. Rename the 5.1 context structs to upstream's #2058 terms (`GameData`, runtime `Game`,
   `Character` + `CharacterExtras`).
6. Check `ags4-framescript` against upstream's `OnFrameEvent`; drop or rebase.
7. Split PLAN.md status logs into `LOG.md`.
