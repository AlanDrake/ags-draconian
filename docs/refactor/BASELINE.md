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
| dead-code candidates (unverified) | 154 |

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
