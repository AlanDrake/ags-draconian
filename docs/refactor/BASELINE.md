# Refactor baseline

Snapshot taken 2026-10-31 on `ags4draconianr21` (fork point from upstream `ags4`: `4cd88beba`).
Regenerate with `python Script/refactor_metrics.py` from the repo root and compare; each phase in
[PLAN.md](PLAN.md) names the metrics it is expected to move. Numbers are heuristic (regex-based) and
meant for tracking trends, not as exact counts.

Not captured by the script, measured by hand for this snapshot:

| metric | value |
|---|---:|
| clang-format 19 churn with the new `.clang-format` (Common/util, Engine/ac, Engine/main, Compiler/script2) | ~15% of lines |
| C++ standard in CMake | C++11 |
| CMake minimum | 3.13 |
| Editor target framework / C# version | .NET Framework 4.7 / C# 6 |
| Build systems maintained in parallel | CMake, MSBuild (Solutions/), hand Makefiles (Engine, Compiler, Plugins, PSP), Xcode, Gradle |
| Script compilers shipped | 2 (`Compiler/script` legacy, `Compiler/script2` extended) |
| Engine unit-test files vs Engine source files | 3 vs ~430 |



## loc

| metric | value |
|---|---:|
| Common C/C++ | 49993 |
| Compiler C/C++ | 43698 |
| Editor C# | 107472 |
| Editor C/C++ | 7067 |
| Engine C/C++ | 119802 |
| Plugins C/C++ | 9561 |
| Tools C/C++ | 16581 |

## globals

| metric | value |
|---|---:|
| extern decls in Engine (all) | 704 |
| extern decls in Engine .cpp files | 596 |

## legacy_c

| metric | value |
|---|---:|
| NULL | 276 |
| sprintf/strcpy/strcat/strncpy | 63 |
| malloc/calloc/realloc/free | 51 |
| fixed char buffers | 123 |
| new[] / delete[] | 76 |
| goto | 1 |
| using namespace in headers | 54 |
| tab-indented lines | 5511 |
| old-style include guards | 372 |
| #pragma once headers | 18 |

## markers

| metric | value |
|---|---:|
| TODO | 735 |
| FIXME | 332 |
| CHECKME | 80 |
| deprecated/obsolete/unused mentions | 272 |
| commented-out code lines | 83 |
| C# TODO/FIXME/HACK | 230 |
| C# catch-all handlers | 122 |

## platforms

| metric | value |
|---|---:|
| AGS_PLATFORM_OS_PSP refs | 8 |
| allegro.h includes | 38 |
| wgt2allg includes | 6 |

## data_versions

| metric | value |
|---|---:|
| legacy version checks (pre-3.99) | 62 |
| files with legacy version checks | 22 |

## functions

| metric | value |
|---|---:|
| functions scanned | 8938 |
| > 100 lines | 91 |
| > 200 lines | 19 |
| nesting depth >= 6 | 10 |
| script API wrappers (Sc_*) | 1598 |
| dead-code candidates (unverified) | 154 (2026-11-08, after 3.5: 0) |
| dead-code exceptions (documented) | 8 (2026-11-08, 3.5; metric added then) |

## cryptic_names

| metric | value |
|---|---:|
| chaa | 652 |
| chap | 88 |
| chin | 126 |
| chinfo | 51 |
| char1 | 98 |
| chi | 368 |
| objj | 335 |
| guio | 220 |
| butt | 187 |
| sds | 226 |
| eres | 445 |
| tqs | 224 |
| scam | 82 |
| texx | 68 |
| widd | 19 |
| aschar | 22 |
| iit | 25 |
| vii | 41 |
| xxx | 7 |
| yyy | 1 |
| xx | 212 |
| yy | 207 |
| fil | 72 |
| scv | 59 |
| guin | 41 |
| butid | 3 |
| guisl | 58 |
| abuf | 4 |
| thisroom | 369 |
| croom | 127 |
| objs | 223 |
| scsystem | 37 |
| usetup | 158 |
| (total) | 4855 |

## tests

| metric | value |
|---|---:|
| C++ test files | 35 |
| Engine C++ test files | 3 |
| C# test files | 22 |

## compile time (2026-11-15, PLAN.md 2.10a)

Compile time of the first-party objects (the tests included; SDL, allegro, googletest and the other fetched or
`libsrc` code left out), read from the ninja log of a clean, full build. Each step's time is the wall-clock time
of one compiler run while about ten ran in parallel, so the numbers compare builds made the same way; they are not
CPU seconds of a single run. Every build was made twice from scratch in a fresh directory (the two readings below),
on `ags4draconianr21` at `698243cf2` with the documentation changes of the working tree, one build at a time and
nothing else heavy running. Same configuration as `Script/compare_objects.py`: `MinSizeRel` (optimised, `NDEBUG`,
no debug info, no LTO), `-DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1`, no compiler launcher (no ccache); MSVC with `/Brepro`,
GCC with `-g0`. Machine: 8 logical cores, Ninja's default of 10 jobs. MSVC: `cl` 19.44.35230 (Visual Studio 2022)
on Windows 10; GCC: g++ 13.3.0, CMake 3.28.3, Ninja 1.11.1 in the WSL 2 distro `Ubuntu-24.04`, building on the
distro's own disk. Objects: 558 per build; a source compiled into several targets counts once per object.

| | MSVC reading 1 | MSVC reading 2 | GCC reading 1 | GCC reading 2 |
|---|---:|---:|---:|---:|
| all: total s | 1731.4 | 1676.4 | 1603.9 | 1585.3 |
| all: median s per object | 2.70 | 2.59 | 2.37 | 2.36 |
| `Engine` (253 objects): total s | 757.9 | 727.8 | 751.8 | 724.8 |
| `Engine` median s | 2.79 | 2.66 | 2.68 | 2.52 |
| `Engine/game` (155 objects): total s | 443.9 | 434.0 | 464.8 | 459.6 |
| `Engine/game` median s | 2.79 | 2.72 | 2.71 | 2.56 |
| `Engine/game/script_api` (46 objects): total s | 124.6 | 122.0 | 108.4 | 108.9 |
| `Engine/game/script_api` median s | 2.57 | 2.61 | 2.28 | 2.26 |
| wall-clock of the whole build, s | 209.8 | 202.5 | 198 | 196 |

The spread between the two readings of the same code: total 3.2% (MSVC) and 1.2% (GCC); `Engine/game` 2.2% and
1.1%; the median per object 4% and 0.4%. A change in a batch of phase 5 is real when it is clearly larger than
that. The other directories (Common, Compiler, Plugins, Tools) are in the script's output. A line fit over the
155 files of `Engine/game`: about 2.4 s fixed plus 1.06 ms per line with MSVC, about 2.3 s plus 2.0 ms per line
with GCC (the first reading of 2026-11-15 gave 2.4 s plus 1.3 ms with MSVC).

**After 5.1 step 2** (2026-11-15, the working tree with `EngineServices`; same machine, configuration and
compilers; 559 objects, one more: `platform/base/engine_services.cpp`):

| | MSVC reading 1 | MSVC reading 2 | GCC reading 1 | GCC reading 2 |
|---|---:|---:|---:|---:|
| all: total s | 1685.5 | 1687.8 | 1706.5 | 1625.4 |
| all: median s per object | 2.62 | 2.57 | 2.45 | 2.40 |
| `Engine` (254 objects): total s | 739.2 | 743.0 | 769.1 | 740.2 |
| `Engine` median s | 2.67 | 2.68 | 2.60 | 2.63 |
| `Engine/game` (155 objects): total s | 439.2 | 449.8 | 485.1 | 460.6 |
| `Engine/game` median s | 2.77 | 2.79 | 2.59 | 2.63 |
| `Engine/game/script_api` (46 objects): total s | 120.2 | 126.4 | 127.0 | 123.2 |
| `Engine/game/script_api` median s | 2.51 | 2.70 | 2.54 | 2.34 |
| wall-clock of the whole build, s | 203.0 | 204.6 | 215 | 200 |

MSVC: inside the spread of the first two readings everywhere. GCC: reading 1 was disturbed (`Common`, which the
step did not touch, took 522.7 s against 475.1 s in reading 2), so reading 2 is the one to compare: the total is
1.3% to 2.5% above the baseline's, `Engine` and `Engine/game` are inside the baseline's two readings, and
`script_api` is 13% above (123.2 s against 108.4 and 108.9) with GCC only (MSVC: 120.2 and 126.4 against 124.6
and 122.0). That is not explained by the step: no file of `script_api` and no header includes
`platform/base/engine_services.h` (47 source files do), and the rest of `Engine/game` is as much faster as
`script_api` is slower, which points at the order in which the parallel steps ran. To read again in 2.10b.

**After 5.1 step 3** (2026-11-16, MSVC only; the tree before the step, `f8bd17c05`, and the working tree after it
built in turn, before, after, before, after, on an idle machine; 560 objects on both sides: the step folded
`game_setup_struct_base.cpp` into `game_def.cpp` and `tools_test` compiles `main_game_file.cpp` too). The
objects are matched through `compile_commands.json` as above, by `.compare/helpers/51s3/ct_cmp.py`; the result
sets are in `.compare/ct/q-*`.

| | before 1 | before 2 | after 1 | after 2 | difference of the averages |
|---|---:|---:|---:|---:|---:|
| all: total s | 1728.4 | 1716.9 | 1772.8 | 1753.8 | +2.4% |
| all: median s per object | 2.57 | 2.61 | 2.76 | 2.68 | +5.1% |
| `Engine` (255 objects): total s | 751.6 | 741.4 | 789.9 | 766.7 | +4.3% |
| `Engine` median s | 2.63 | 2.71 | 2.94 | 2.78 | +7.2% |
| `Engine/game` (155 objects): total s | 449.1 | 441.0 | 483.3 | 458.9 | +5.9% |
| `Engine/game` median s | 2.69 | 2.87 | 3.11 | 2.87 | +7.7% |
| `Engine/game/script_api` (46 objects): total s | 125.5 | 123.6 | 127.9 | 125.6 | +1.8% |
| `Common` (199 objects): total s | 515.9 | 515.0 | 524.0 | 526.4 | +1.9% |
| `Tools` (59 objects): total s | 279.2 | 277.1 | 279.1 | 278.7 | +0.3% |
| wall-clock of the whole build, s | 206.5 | 205.2 | 211.3 | 209.3 | +2.1% |

The two readings of a side differ by 0.7% and 1.1% in total, so the step's cost is real: 40 s of compile time
over the build, of which 26 s on the 95 engine sources that include a context header (`game/game_context.h`
or `game/world/room_context.h`: 350.1 to 375.8 s, +7.3%, about 0.27 s a file; median 3.38 to 3.68 s), 6 s on
the 160 other engine sources (+1.6%) and 8 s outside `Engine` (+0.9%, of which 6 s are the second copy of
`main_game_file.cpp` in `tools_test`). PLAN.md 5.1 step 3 asked for a median not higher than before: it is
higher, by 0.13 s per object (5%). A reading taken the evening before, with the machine in use, had shown
nothing (three builds of the unchanged tree differed by 26%). GCC was not measured. What the context header
costs is its includes: `game/game_def.h`, `game/game_state.h` and `game/render/viewport.h` reach every user of
`Game`, where a file used to include one of them.

Where the 26 s went (2026-11-16, `.compare/helpers/51s3/ct_why.py`: what each of the 95 sources included
before and after, by `/showIncludes`, against its time in the readings above):

| The source, before the step | Sources | Seconds more | Per file |
|---|---:|---:|---:|
| saw `game_state.h` and the game struct | 53 | 4.2 | 0.08 |
| saw `game_state.h` only | 13 | 4.0 | 0.31 |
| saw the game struct only | 10 | 6.2 | 0.62 |
| saw neither (it named a global declared in a light header: `displayed_room`, `guis`, `palette`...) | 19 | 11.3 | 0.59 |

So the cost is the 42 sources that did not see both big headers and now do; a source that already saw both
pays almost nothing for the context. The front end alone, on an empty source (MSVC `/Zs`, best of five): the
standard headers the engine uses everywhere 0.38 s, `game_def.h` 0.98 s, `game_state.h` 0.97 s, both 1.10 s
(they share most of what they include), `game_context.h` 1.13 s, the three contexts 1.18 s, `roomstruct.h`
0.72 s, `viewport.h` 0.43 s, `engine_services.h` 0.41 s (0.40 s before the step). These are the numbers a
precompiled header would be judged by (2.10b): about 1.2 s of a context user's 3.7 s median is headers that
every one of them parses again.

**After 5.1a step 1** (2026-11-16, MSVC only; `ec0098a2a` before and `7347023cb` after, each in a worktree,
built in turn, before, after, before, after, on an idle machine with nothing beside it; 560 objects before and
559 after: `character_extras.cpp` merged into `character.cpp`; `.compare/helpers/51a1/ct_run.sh`, the result
sets in `.compare/ct/51a1-*`).

| | before 1 | before 2 | after 1 | after 2 | difference of the averages |
|---|---:|---:|---:|---:|---:|
| all: total s | 1687.6 | 1658.5 | 1703.4 | 1652.9 | +0.3% |
| all: median s per object | 2.59 | 2.58 | 2.63 | 2.56 | +0.3% |
| `Engine` (254 objects): total s | 732.7 | 731.5 | 754.2 | 724.0 | +1.0% |
| `Engine` median s | 2.64 | 2.75 | 2.78 | 2.60 | -0.1% |
| `Engine/game` (154 objects): total s | 442.0 | 443.4 | 451.2 | 432.1 | -0.2% |
| `Engine/game/script_api` (46 objects): total s | 122.4 | 119.8 | 122.1 | 119.5 | -0.2% |
| `Common` (199 objects): total s | 511.9 | 499.3 | 507.1 | 498.9 | -0.5% |
| `Tools` (59 objects): total s | 264.3 | 257.8 | 268.0 | 259.7 | +1.1% |
| wall-clock of the whole build, s | 202.7 | 198.7 | 204.3 | 197.9 | +0.2% |

**No measurable change:** the two readings of a side differ by 1.7% (before) and 3.0% (after) in total, ten
times the difference between the sides. The 94 engine sources that include a context header: 350.7 to
353.8 s (+0.9%, median 3.40 to 3.45 s); the 159 other engine sources +1.7%; the sources outside `Engine`
-0.2%. The step made `game/world/character.h` heavier (the class and its state came into it) and took it out
of `game_state.h`, so that 34 of the 451 first-party translation units see it where 104 did: the two about
cancel. This run's "before" is the tree that the morning's reading had as "after" (1,754 and 1,773 s then,
1,659 and 1,688 s now, about 5% less): readings of different hours are not compared, only the pairs of one
run. GCC was not measured.

**After 5.1a steps 2 to 4** (2026-11-17, MSVC only; four revisions, each in a worktree, built clean in turn,
twice round, on an idle machine with nothing beside it: `9101535c8` before step 2, `512a7bdce` after it,
`35c6d5a8f` after step 3, `0eda178d5` after step 4; `.compare/helpers/51a4/ct_run4.sh`, the result sets in
`.compare/ct/51a-s<N>r<round>`). The two readings of a revision differ by 0.1% or less in total, so the
differences between the revisions are real.

| | before step 2 | after step 2 | after step 3 | after step 4 | steps 2 to 4 |
|---|---:|---:|---:|---:|---:|
| all (561, 562, 562, 562 objects): total s, both readings | 1655.3, 1655.0 | 1674.5, 1675.0 | 1683.4, 1685.2 | 1704.5, 1705.5 | +3.0% |
| all: median s per object | 2.57 | 2.55 | 2.55 | 2.58 | +0.5% |
| `Engine` (255, 256, 256, 256): total s | 721.9 | 739.6 | 747.6 | 763.8 | +5.8% |
| `Engine` median s | 2.65 | 2.65 | 2.60 | 2.60 | -2.0% |
| `Engine/game` (154): total s | 434.5 | 435.8 | 428.2 | 431.8 | -0.6% |
| `Engine/game` median s | 2.77 | 2.71 | 2.69 | 2.66 | -3.7% |
| `Engine/game/script_api` (46): total s | 119.3 | 120.1 | 116.9 | 117.2 | -1.7% |
| `Common` (200): total s | 502.3 | 506.4 | 508.8 | 513.8 | +2.3% |
| `Tools` (59): total s | 263.5 | 261.1 | 259.7 | 260.4 | -1.2% |
| wall-clock of the whole build, s, both readings | 198.8, 198.7 | 200.6, 200.6 | 201.8, 202.1 | 203.8, 203.7 | +2.5% |

**The game's sources compile a little faster, the total grows by the tests.** `Engine/game`, where the three
steps did their work, is at -0.6% in total and -3.7% in the median: the header trims (`room_def.h` out of 82
translation units, `speech.h`, `move_list.h` and `graphicsdriver.h` out of `game_state.h`, three entity
headers out of `game_def.h`) outweigh the classes, the state structs and the tables that came in. The total
grows 49.9 s, of which the two new test sources are 27.6 s (`room_state_test.cpp` 12.8 s,
`game_entities_test.cpp` 14.7 s) and the tests these steps extended a few seconds more
(`savegame_components_test.cpp`, `field_table_test.cpp`); a single object's time varies by about a second
between readings, so only the totals and the groups are firm. Per step: +1.2% (step 2, one new test source and
`room_state.cpp`), +0.6% (step 3, one new test source of 11.9 s, more than the whole difference), +1.2%
(step 4: `game_entities_test.cpp` +2.9 s, `game_state.cpp` with its 154-row counted table 1.45 to 1.77 s,
and the live classes of three entities in headers that were forward declarations). GCC was not measured.

**With the precompiled header** (2.10b item 2, 2026-11-16, MSVC only; `95c9e7e09` with the change in a
worktree). **Indicative: taken while another session built on the same machine**, and accepted so by the user;
no reading on an idle machine was taken. The readings of this file above and below stay without the
precompiled header (`-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`, which `compare_objects.py` passes), so a later
step is compared with them, not with this table.

| MinSizeRel, a clean build each | without | with | difference |
|---|---:|---:|---:|
| all (558 objects): total s | 1708.6 | 1410.5, and 27.0 for the nine headers | -17% |
| all: median s per object | 2.69 | 2.08 | -23% |
| `Engine` (253 objects): total s | 751.5 | 523.2 | -30% |
| `Engine` median s | 2.80 | 1.86 | -34% |
| `Engine/game` (154 objects): total s | 454.0 | 312.7 | -31% |
| `Engine/game/script_api` (46 objects): total s | 123.7 | 84.3 | -32% |
| `Common` (199 objects): total s | 507.4 | 471.6 | -7% |
| `Tools` (59 objects): total s | 277.3 | 255.6 | -8% |
| `Compiler` (31 objects): total s | 153.6 | 146.5 | -5% |
| wall-clock of the whole build, s | 205 | 215 | none seen |
| rebuild after a change to `game_context.h` (98 steps), s | 48.8, 43.8 | 27.5, 29.3, 37.0 | about -35% |

Twelve single compiles, the fastest of three each, which a busy machine disturbs less: 21.15 s without, 14.47 s
with (-32%; `Common/util/string.cpp` 0.53 to 0.18 s, `Engine/game/render/draw.cpp` 5.21 to 4.13 s). As Release,
the configuration of the batch builds, the first clean build with it (`.tidy/dev`) took 635 s of compile time
over 568 steps, median 0.99 s, 278 s of wall-clock with the links; there is no clean Release build without it
to set beside it (the prepush directory's log, objects compiled at different hours of a busy day, adds up to
1,348 s, median 2.27 s). The wall-clock of a clean build did not move in the one pair there is.

To reproduce: build as `compare_objects.py` does (`cmake -S . -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=MinSizeRel
-DAGS_BUILD_TOOLS=1 -DAGS_TESTS=1 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON`, from a developer prompt for MSVC, then
`cmake --build <dir>`), then `python Script/refactor_metrics.py --ninja-log <dir>/.ninja_log` (`--json` for the
numbers as data). The script maps each object to its source through the `compile_commands.json` beside the log.

## longest functions (top 25)

| lines | nesting | location |
|---:|---:|---|
| 1477 | 8 | `Compiler/script/cs_parser.cpp:3440` __cc_compile_file |
| 1161 | 5 | `Engine/script/scriptexecutor.cpp:690` ScriptExecutor::Run |
| 745 | 8 | `Plugins/agspalrender/agspalrender/raycast.cpp:794` Raycast_Render |
| 535 | 5 | `Tools/agsbuild/main.cpp:158` BuildWithTaskManager |
| 489 | 8 | `Engine/ac/character.cpp:3056` display_speech |
| 482 | 4 | `Compiler/script/cs_parser.cpp:2490` parse_sub_expr |
| 408 | 5 | `Engine/ac/room.cpp:580` load_new_room |
| 354 | 5 | `Compiler/script/cs_parser.cpp:2122` do_variable_ax |
| 303 | 2 | `Engine/ac/gamestate.cpp:572` GamePlayState::ReadFromSavegame |
| 282 | 4 | `Plugins/agspalrender/agspalrender/ags_palrender.cpp:2177` AGS_EngineOnEvent |
| 282 | 4 | `Plugins/agsblend/agsblend/AGSBlend.cpp:427` DrawSprite |
| 265 | 2 | `Engine/main/engine.cpp:616` engine_init_game_settings |
| 228 | 2 | `Engine/ac/character.cpp:4864` RegisterCharacterAPI |
| 226 | 2 | `Compiler/compiler.cpp:117` Compile |
| 223 | 2 | `Engine/plugin/plugin_stubs.cpp:50` RegisterPluginStubs |
| 215 | 1 | `Engine/ac/gamestate.cpp:877` GamePlayState::WriteForSavegame |
| 208 | 1 | `Plugins/agspalrender/agspalrender/ags_palrender.cpp:1959` AGS_EngineStartup |
| 206 | 3 | `Engine/ac/dialog.cpp:888` DialogOptions::Begin |
| 204 | 3 | `Engine/game/savegame.cpp:718` DoAfterRestore |
| 194 | 4 | `Plugins/agspalrender/agspalrender/ags_palrender.cpp:1627` DrawReflections |
| 193 | 2 | `Engine/ac/roomobject.cpp:159` RoomObject::ReadFromSavegame |
| 192 | 8 | `Plugins/agspalrender/agspalrender/ags_palrender.cpp:1320` DrawStars |
| 189 | 2 | `Tools/data/agfreader.cpp:1432` ReadGameData |
| 182 | 3 | `Engine/ac/dynobj/cc_serializer.cpp:62` AGSDeSerializer::Unserialize |
| 178 | 4 | `Compiler/main.cpp:81` parser_to_compiler_opts |
