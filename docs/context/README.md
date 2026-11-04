# docs/context

Project knowledge for coding agents. Entry point: `/AGENTS.md`. Policy: `/CONTRIBUTING.md`.

AGS Draconian copy: kept on par with upstream's (PR #3185, worktree branch `ags4-agent-context`),
but about `ags4` (AGS 4) only; AGS 3 (`master`, 3.x releases) is left out except where it reaches
`ags4`. Fork-only knowledge goes into `/AGENTS.md` and `docs/refactor/`, not here.

| File | Load when |
|---|---|
| `repository.md` | locating code; branch, build and test facts; what must not break; where a change goes |
| `workflows.md` | building, testing, documenting, submitting |
| `decisions.md` | designing; checking whether an idea is settled, positioned or rejected |
| `roadmap.md` | state of 4.0; Beta checklist; open PRs; proposals; parked ideas |
| `sources.md` | who said what; where to search; fetch commands; reading list |

Editing rules:

- every claim: source (issue/PR number, wiki page, forum board + title, or `path@commit`) and date
- status words only: **decided**, **in code**, **proposed**, **open**, **rejected**; a maintainer's design is "proposed" until merged
- shipped → one line "in code" (details are in `Changes.txt`); rejected → one line under "Parked", with the maintainer's reason; no entry without a reason
- verify paths with `git grep`/`ls` on `upstream/ags4`
- ≤3k tokens per file; snapshot date at the top of each file
- refresh: `roadmap.md` at each release or milestone change; `decisions.md` when a maintainer settles something; `repository.md` when layout or build systems move
- these files describe; `CONTRIBUTING.md` decides
