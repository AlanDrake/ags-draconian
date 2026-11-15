# Engine

The AGS runtime: it loads a game made by the editor and runs it. Built by `Engine/CMakeLists.txt` and
`Solutions/Engine.App/Engine.App.vcxproj` (+ `.filters`), which list the same files.

## Rings

Each top-level directory is in one ring. A file may include headers of its own ring and of the rings
below it in the table, and nothing else. `Script/check_layers.py` (prepush stage `layers`, `verify.yml`)
checks this; the ring table is in its docstring.

| Ring | Directories | May include |
|---|---|---|
| common | `Common/` | common |
| base | `util/`, `debug/` | common, base |
| host | `platform/`, `device/`, `gfx/`, `media/`, `resource/` | common, base, host |
| vm | `script/` | common, base, vm |
| game | `game/` | common, base, host, vm, game |
| composition | `main/`, `plugin/` | everything |
| test | `test/` | everything |

What must not depend on what:

- The host (window, input, graphics, audio, files) and the script VM know nothing about the game logic: they
  do not include `game/` or `main/`, and not each other.
- `game/` does not include `main/` or `plugin/`. `main/` and `plugin/` join the other rings.
- That is the rule, not yet the tree: 94 includes break it today (the baseline, below).
- What lives as long as the process (the configuration, the platform and graphics drivers, the game's files,
  the loop flags) is in `EngineServices` (`platform/base/engine_services.h`, host ring), reached through
  `EngineServices::Current()`; a function that uses it more than once takes
  `EngineServices &services = EngineServices::Current();` at its top.
- `script/script_api.h` (the text formatting of the script bindings that take a format) is included only by
  `script/` and `game/script_api/`.
- A new include that breaks a rule fails `check_layers.py`. The includes that break the rules today are
  listed in `Script/check_layers_baseline.txt`: the list can only shrink, so whoever removes such an include
  removes its line in the same commit (`python Script/check_layers.py --list` groups them).

## Entry points

- `main/main_sdl2.cpp`: `main()`, which calls `ags_entry_point()` in `main/main.cpp` (command line, start-up
  modes such as help and setup).
- `main/engine.cpp`: the engine's start-up steps (backend, window, setup dialog, fonts, asset paths).
- `main/game_run.cpp`: the game loop (input, updates, drawing, the blocking script calls).
- `main/config.cpp`, `platform/base/engine_config.h`: `acsetup.cfg` and the `EngineConfig` settings,
  `EngineServices::Config` (contract C6).

## Directories

- `util/`: small engine-wide helpers: dynamic library loading, thread, time and path helpers, `timer`.
- `debug/`: logging, the link to the editor's debugger, memory inspection.
- `platform/`: one `AGSPlatformDriver` per OS (`windows`, `linux`, `osx`, `android`, `ios`, `bsd`,
  `emscripten`), the shared `base/` and `util/`, and each OS's setup dialog and graphics driver.
- `device/`: mouse and SDL event handling.
- `gfx/`: the graphics driver interface, the software and OpenGL renderers, filters, drawable bitmaps.
- `media/`: `audio/` (OpenAL mixing, decoders, sound clips) and `video/` (players, FLIC, Theora); the engine
  side of both, with no game logic.
- `resource/`: Windows resources (icon, version), shaders.
- `script/`: the script VM (`scriptexecutor`, `runtimescript`, `systemimports`, `script_api.h`), the binder
  that generates the script bindings from the signatures of the functions they call (`script_binder.h`) and
  `script/dynobj/`, the managed-object files the VM itself uses (pool, manager, base object, arrays, user and
  plugin objects).
- `game/`: the game logic, see `game/README.md`.
- `main/`: start-up, configuration, the game loop, graphics mode, game file loading, quitting.
- `plugin/`: the engine side of the plugin API (`agsplugin.h`, contract C4) and the built-in plugin stubs.
- `test/`: engine unit tests (googletest) and their data.
- `libsrc/`: third-party code (video and audio decoders, glad), never reformatted.
