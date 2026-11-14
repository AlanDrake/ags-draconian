# Engine/game

The game logic: what a running AGS game does, on top of the host (`platform/`, `device/`, `gfx/`, `media/`)
and the script VM (`script/`). It may include those, Common and itself; never `main/` or `plugin/`
(`Engine/README.md` has the rings, `Script/check_layers.py` checks them).

New code goes in the folder of its domain. A new file is also added to `Engine/CMakeLists.txt`,
`Solutions/Engine.App/Engine.App.vcxproj` (+ `.filters`) and, if macOS builds it, `OSX/xcode/AGSKit`.

## Root files

- `game.cpp`: the `Game` script API and the game-wide functions (save slots, audio types, unloading the
  game). `game_state.cpp`: `GamePlayState`, the state that scripts read as `game.*`.
- `game_init.cpp`: initialises the game after its data file is loaded.
- `event.cpp`: game events and the room and script events they run.
- `script.cpp`: runs the game's scripts and their events (the glue between the game and the VM).
- `system.cpp`: the script `System` API.
- `file.cpp`: the script `File` API and file handles.
- `translation.cpp`: the loaded translation.
- `runtime_defines.h`, `asset_helper.h`: constants and asset lookup that the host also includes (they mix
  both sides; listed in the layering baseline until they are split).

## Folders

- `world/`: the things in a room and the player's party: `character`, `object`, `room_object`, `room`,
  `room_status`, `hotspot`, `region`, `walkable_area`, `walk_behind`, `inventory_item`, `view_frame`,
  `move_list`, `properties`. A new room or character feature goes here.
- `navigation/`: path finding (`route_finder`, `route_finder_jps`, `pathfinder_script`).
- `dialog/`: dialogs and text: `dialog`, `dialog_options_rendering`, `display` (message boxes and
  overlays), `speech`, `lipsync`, `parser`.
- `ui/`: GUIs and the mouse: `gui`, `gui_control`, `button`, `label`, `list_box`, `slider`, `text_box`,
  `inv_window`, `mouse`, and the built-in dialogs and legacy controls (`gui_dialog`, `csci_dialog`,
  `my_*`).
- `render/`: drawing a frame and what is drawn: `draw`, `draw_software`, `screen`, `overlay`,
  `screen_overlay`, `viewport`, `dynamic_sprite`, `drawing_surface`.
- `audio/`, `video/`: the game's audio and video logic (`audio`, `audio_channel`, `audio_clip`, `video`) on
  top of `media/`.
- `input/`: `keyboard`, `joystick`, `touch`.
- `save/`: the save game reader and writer (`savegame`, `savegame_components`): contract C2.
- `lib/`: the script library that is not a game object: `date_time`, `math`, `string`, `script_containers`,
  `utils_script`.
- `dynobj/`: the managed objects scripts hold handles of (`cc_character`, `script_*`, the serialiser
  `cc_serializer`). `Engine/script/dynobj/` has the ones the VM itself uses.
- `script_api/`: the script bindings, see below.

## Script bindings

A script API function is two pieces:

1. The logic, `Type_Member` or a plain function, in the domain file of its type (`world/character.cpp`),
   declared in that file's header.
2. The row that binds it under its script name, in `Register<Type>API()`:
   `ScMethod<&Character_AddInventory>("Character::AddInventory^2")` for a function whose first parameter is
   the object the script calls it on, `ScStatic<&Character_GetAtScreenXY>("Character::GetAtScreenXY^2")` for
   one without.

The wrapper the VM calls is generated from the function's signature by `script/script_binder.h`. Where the
script expects something the signature does not say, the row says it with a tag (`ScVoidResult`,
`ScBoolArg<1>`; the list is in `script_binder.h`). `script_api/bindings.h` names the managers of the objects
the API returns and has the rows of the functions whose last parameter is a formatted text (`ScMethodFormat`,
`ScStaticFormat`, with a C variadic `ScPl_*` function for plugins). An `Sc_*` wrapper is written by hand only
for what no row can say (`Sc_System_Log`, whose format is not translated).

The rows go in `script_api/<type>_api.cpp` (`character_api.cpp`, `button_api.cpp`). The flat functions of the
old script API (`Display()`, `SetGlobalInt()`) are in `global_api.cpp`; `exports.cpp` and `exports.h`
register all the tables. The files of `script_api/` are the only ones in the game ring that include
`script/script_api.h` (the text formatting of the `ScPl_*` functions) directly (`check_layers.py` enforces
it).
