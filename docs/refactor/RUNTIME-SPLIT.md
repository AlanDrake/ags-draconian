# Definitions, live objects and the engine's contexts: the design for 5.1 and 5.1a

The design study of 2026-11-15 (PLAN.md "Next up" item 2), reviewed with the user the same day. Sources: the
fork's tree at `698243cf2`, read by three inventory passes (the structs per entity, the save and restore flow,
the field overlap between entity types), upstream issue #2058 and the wiki draft "AGS 4 Draft: major engine
refactor", and the ninja log of the last full MSVC build in `.compare/build`. The tasks are in PLAN.md (2.10,
5.1, 5.1a, 5.8 to 5.12, L.3); this file keeps the reasons and the evidence.

Each name below is marked **confirmed** (the user, 2026-11-15) or **proposed** (not yet confirmed). What the
tasks still have to design is listed in LOG.md "5.1 step 2 and 5.1a: the design".

## 1. What the code does today

- **203 globals**, one `extern` each, all in headers since 5.1 step 1. Uses in `Engine/` by a word count: `game`
  about 1,860, `play` 1,540, `thisroom` 320, `gfxDriver` 270, `guis` 265, `objs` 190, `dialog` 180, `charextra`
  170, `views` 170, `spriteset` 160, `usetup` 140.
- **Nearly every "design" field is changed during play and saved.** 37 of `CharacterInfo`'s 40 data members
  (all but `scrname` and the two event tables); the options and six more fields of `GameSetupStruct`; cursors,
  inventory items, audio clip types; every frame of every view; dialog option flags and text; the whole of each
  GUI and control but the id, name, parent and events; in `thisroom` the region light and tint, walk-area
  scaling, masks and backgrounds. Only audio clips and the event tables never change. One write is hidden: the
  sprite cache is constructed on a reference to `game.SpriteInfos` and updates it for dynamic sprites.
- **A restore destroys the running game before it reads the save.** `DoBeforeRestore` unloads the room, frees
  the dynamic sprites, resets the managed pool and the room states; most save components then write straight
  into the globals; any failure after that is `quit` ("AGS cannot properly revert to stable state"). The one
  recovery, `RetryWithClearGame`, reloads the game file and restores again.
- **Nothing rebuilds the state without a save** except `RunAGSGame`, by unloading and reloading the game file.
  `RestartGame()` restores slot 999, which exists only after `SetRestartPoint()`.
- **Addresses tie the state to the process.** The managed pool registers characters, GUI controls and the script
  handles by the address of the globals; scripts import `&play`, `&scmouse`, `&palette[0]`, the `Static*Array`
  data and `&_sc_PlayerCharPtr`; plugins hold `&game.chars[i]` and `&scrObj[i]`; script global data is restored
  into the live instances.
- **`GamePlayState` is five things in one struct** (section 5).
- **Formats are described in more than one place.** The game file is read in Common and written in
  `Tools/data/data_file_writer.cpp`: characters, inventory items and cursors through the Common struct's
  `WriteToFile`, views and GUIs by hand with a comment naming the reader they must match. Save code is methods
  on 14 Common files (which the tools compile without using), on engine structs, and the 2,331 lines of
  `savegame_components.cpp`.
- **`AGS.Native` compiles `GUIMain`, `RoomStruct` and a `GameSetupStruct`** (`thisgame`, 189 uses), part of it
  the old-game import that 6.7a deletes. After 6.7 the editor reaches Common only through the C library
  (`ags_editor_native`, 6.7c), which keeps the GUI and font drawing native (confirmed: the library stays).
- **A file costs about 2.4 s to compile, a line 1.3 ms** (MSVC MinSizeRel, fit over the 155 files of
  `Engine/game`; no precompiled header, no unity build). The 46 files of `script_api/` take 122 s of the 446 s
  of `Engine/game`. `game/game.h` is included by about 335 files.

## 2. The model

**Globals are placed by when they die.**

| Lifetime | Class | Holds | Replaces |
|---|---|---|---|
| Game file loaded | `GameDef` (Common, never written after load) | the settings, the lists of `XDef` records, the compiled scripts | `GameSetupStruct`, `GameSetupStructBase`, `LoadedGameEntities`, and `views`, `dialog`, `guis` as loaded |
| One play-through, until the next restore or restart | `Game` (Engine) | everything a save holds: the game's own state, live options and palette, the lists of live objects, a `RoomState` per room, overlays, move lists, viewports and cameras, the script heap (managed pool, script global data, the handle arrays) | `play`, `game.chars`, `charextra`, `guis`, `views`, `dialog`, `palette`, `scrObj`..., `Static*Array`, `pool` |
| Room loaded | `Room` (Engine, owned by `Game`) | the room's `RoomDef`, the working copies of masks and backgrounds, the room script instance, its `RoomState` | `thisroom`, `croom`, `objs`, `walkareabackup`, `roomscript` |
| Process | `EngineServices` | graphics driver, platform, assets, sprite cache, audio device, `EngineConfig`, script executor, plugins, the loop flags | `gfxDriver`, `platform`, `AssetMgr`, `spriteset`, `usetup`, `want_exit`... |

Three placements that are easy to get wrong:

- **`EngineServices` lives in the host ring** (`platform/base/engine_services.h`, done 2026-11-15) and names
  the graphics driver, the platform driver and the asset manager by forward declaration; it holds the
  configuration and `ResPaths` by value, so `EngineConfig` and what it needs came down to the host ring with
  it. The game and composition rings include it; the script VM does not need it (`scriptExecutor` is a global
  of the script ring); the two base-ring files that use it (`util/timer.cpp`, `debug/debug.cpp`) are in the
  layering baseline, as they already were for the platform driver. Services that describe the loaded game (the
  mounted packs, the voice and music flags, the text language) are set again by `RunAGSGame`.
- **The sprite cache is `EngineServices::Sprites`** (5.1 step 3, 2026-11-15). `SpriteCache` used to be
  constructed on a reference to `game.SpriteInfos`, which it resizes and writes for dynamic sprites, and the
  texture cache of `draw.cpp` bound to it during static initialization. Now the cache can be told its table
  and callbacks after construction (`SetSpriteInfos`, `SetCallbacks`; a table of its own until then), the
  services own it, the game attaches its table once at engine start (`init_sprite_cache`), and the texture
  cache reaches it at use. A second `Game` attaches its own table when it becomes current (the dynamic
  sprites of each `Game` are 5.11's).
- **The palette is game state**: a savestate holds it, so it is `Game`'s. The three graphics drivers read the
  global to convert 8-bit bitmaps (`select_palette(palette)`); since 2026-11-15 they are given a pointer to
  it once, when the driver is created (`IGraphicsDriver::SetIndexedPalette`), and the game changes the colours
  in place as before: no call at each of the ten places that change it, no copy that could go stale. A second
  `Game` (5.11) gives its own palette when it becomes current.
- **The draw caches** (textures per character, object, GUI, overlay) are derived from a `Game` and belong to
  neither context: they stay with the render code and are rebuilt when a `Game` becomes current.

**A live object is built from its definition.** It starts as a copy of the definition's fields and adds what
exists only during play. Fields are declared once, in plain structs, and a class is assembled from them:

```cpp
struct CharacterDef   { ... };  // Common: what the game file says, the starting values
struct CharacterState { ... };  // Engine: what exists only during play
class  Character : public CharacterDef, public CharacterState { /* behaviour */ };
```

- `XDef`: the record the game or room file holds. A plain struct in Common, with no save code and no engine
  include; the tools link it.
- `XState`: the play-only fields. A plain struct in the engine. `CharacterState` is today's `CharacterExtras`
  plus the play-only fields now inside `CharacterInfo` (`walking`, `frame`, `wait`...). A type with no
  play-only field has no `State` struct.
- `X`: the live object. The engine's functions and the script API take this one.
- A class hierarchy (the GUI controls) gets one struct per level, so that no record is inherited twice:
  `GUIButtonDef` holds the button's own fields and does not derive from `GUIControlDef`.
- Two bases must not share a member name. `CharacterInfo` and `CharacterExtras` both had `flags` (different
  bits) and the two save methods: the state's is `RuntimeFlags` (5.1a step 1), the record's save methods left
  with 5.8 step 1.
- Where the live struct repeats its record today, deriving removes the copy and the types have to be
  reconciled. `RoomObject` repeats `RoomObjectInfo`: `x`/`X` (`int`/`int32_t`), `num`/`Sprite`
  (`uint16_t`/`int32_t`), `baseline`/`Baseline` (`int16_t`/`int32_t`), four `blocking_` fields against a
  `Rect`. Each difference is a behaviour commit with a test. The character has no such copies.
- A definition the scripts hold a handle to needs a live object even if it never changes (the audio clip: the
  pool registers its address and the engine writes its id).

This differs from upstream's #2058 sketch in one point: there the runtime `Game` inherits the loaded data and
the loaded instances are disposed. Here the definitions are kept and never written, which is what makes the
rest possible.

**What it gives.**

- New game: construct a `Game` from the `GameDef`.
- Restore: build a second `Game`, read the save into it, validate, make it the live one. A failed restore
  leaves the running game untouched (section 8, where two points are still to design).
- Retired: the "is the game clear" checks, `RetryWithClearGame`, `PreservedParams::GameOptions`,
  `DialogOption::OriginalText`, `walkareabackup` as a special case.
- Tests construct a `Game` from a small `GameDef` instead of poking globals.

**Access.** Each context has one accessor that returns the current instance through a pointer:
`Game::Current()`, `GameDef::Current()`, `Room::Current()`, `EngineServices::Current()` (confirmed), so a test
or a restore can make another instance current while old code runs. A function that uses a context more than
once takes a local reference at its top (`Game &game = Game::Current();`) and its body reads as today; that
local is what becomes the parameter in 5.1 step 4. The extra pointer load is measured on the script VM and the
draw loops before the first batch lands (STYLE.md "Performance").

**What 5.1 step 3 built (2026-11-15), and where it is a transitional shape.**

- `GameSetupStruct` and its base are one struct, `GameDef` (`Common/game/game_def.h`).
- **`class Game : public GameDef`** (`Engine/game/game_context.h`). The engine's one game is at once what the
  game file says and what the game changes, as `game` was: until an entity has a live copy, there is one
  object per datum. So `game.chars[i]` and `game.options[...]` read as before through `Game &game`, each use
  was converted once, and 5.1a moves or doubles a member between `GameDef` and `Game` without touching the use
  sites. `GameDef::Current()` does not exist yet: it gets a meaning when a definition is kept apart from the
  live game (5.1a step 1, 5.11). `Game` also holds `Play` (`GamePlayState`), `Viewports`, `CharExtras`, the
  script objects and handle arrays, the palette, and the loose globals grouped into state structs (`RoomChange`, `Speech`, `LipSync`, `TextLayout`,
  `Cursor`, `BuiltinDialogs`, `Scripts`, `Handles`, `Events`).
- **`class Room : public RoomStruct`** (`Engine/game/world/room_context.h`): `State` (was `croom`),
  `TempState`, `Objs`, `Script`, the walkable-area and walk-behind working state. One instance; a `Game`
  owns its `Room` once a second `Game` can exist (5.11). The number of the room on screen and the pending
  room change are the game's (`Game::DisplayedRoom`, `Game::RoomChange`): they exist while no room is loaded.
- **A loop does not reload the accessor.** A function that uses a context once inside a loop takes the local
  too, and a helper that is inlined into a loop over drawn or updated entities takes the context from its
  caller as its first parameter (`update_object_scale(Game &, Room &, ...)`): the first use of 5.1 step 4's
  rule, on about 45 functions of the draw and update paths.
- **Globals that stay** are listed with a reason in `Script/check_layers.py` (`EXTERN_ALLOWED`): constants,
  the stateless script managers, the script ring's own state (`pool`, `simp`, `scriptExecutor`: the ring
  cannot reach a context; one pool per `Game` is 5.11's), the crash handler's values, `pluginReaders`.
- **`GamePlayState` lost two of its five things** (section 5): the fields that describe the room on screen are
  members of `Room` (`BgFrame`, `WalkableAreasOn`, `EnteredAtX`..., reset by `Room::ResetPlayState()` where
  the play state is reset), and the viewports and cameras are a class of their own, `GameViewports`
  (`game/render/viewport.h`), held as `Game::Viewports`. The save bytes and the order in which these values
  are set, reset, saved and restored did not change.
- **`GameDef` holds what the loader reads**: `Views`, `Dialogs`, `Guis` and the six control lists, and the
  compiled scripts (`GlobalScript`, `DialogScript`, `ScriptModules`, `ScriptModuleNames`). The loader reads
  into them directly and `Game` inherits them; `LoadedGameEntities` keeps only what is needed while loading.
  The compiled scripts used to die at the end of `load_game_file`; they are kept until `unload_game` now
  (about 1.6 MB for the auto-test game), which is what a second `Game` built from the `GameDef` needs (5.11).

**What 5.1a step 1 built (2026-11-16): the character, the first entity with a live copy.**

- `struct CharacterDef` (`Common/game/character_def.h`, upstream's `CharacterInfo`): what the game file gives.
  It lost thirteen play-only fields, which `ReadFromFile` had long skipped as unused slots of the record
  (`prevroom`, `wait`, `idleleft`, `activeinv`, the three blink counters, `z`, `walkwait`, `walkwaitcounter`,
  `loop`, `frame`, `walking`), and the three methods the engine defined for it; it gained `BlendMode`,
  `GraphicAnchor` and `GraphicOffset`, which the loader used to read into a side struct (`CharDataEx`, gone)
  for the engine to copy.
- `struct CharacterState` (`Engine/game/world/character.h`, upstream's `CharacterExtras`): plain data, with
  those thirteen fields and without the three it had copies of.
- `class Character : public CharacterDef, public CharacterState`: all the behaviour (the methods of
  `CharacterExtras` without their `ch` parameter, the engine's methods of `CharacterInfo`), built by
  `explicit Character(const CharacterDef &def)`. Every engine function, the managed pool, `CCCharacter`, the
  plugin API's `GetCharacter` and the script objects take and hold a `Character *`; no layout was frozen
  (`AGSCharacter` is opaque, scripts reach one field by offset, through `CCCharacter`).
- **One live list, `Game::Characters`**, built in `InitGameState` from the definitions and cleared in
  `unload_game`; `Game::CharExtras` and every `chex` are gone. The definitions stay in
  `GameDef::Characters` as loaded.
- **The live list hides the definitions' list by name.** `Game` still derives from `GameDef`, so
  `Game::Characters` hides `GameDef::Characters`: through a `Game &` the name is the live list, which is what
  every engine use means, no use site had to change when the class arrived (`GameDef::chars` was renamed
  first), and nothing can write a definition by oversight. The definitions are read through
  `const GameDef &Game::Def() const`, in one place so far. The same works for each entity that gets a live
  copy; when the last has one, `Game` stops deriving from `GameDef` and holds the reference that `Def()`
  returns. **`GameDef::Current()` was not needed**: every reader of a definition has its `Game` at hand; it
  comes with the first that has not (5.11 at the latest), as `Game::Current().Def()`.
- **The starting values are still assigned in three places** (`InitAndRegisterCharacters`,
  `engine_init_game_settings`, the default member initializers): one constructor for the starting state is
  5.11 step 1. The three `[INTERNAL]` bits of `CHF_` stay in the definition's `Flags`: a save holds them
  there (5.12).
- The fields have their STYLE.md names (PLAN.md 4.3: `NormalView`, `SpeechColor`, `MoveListID`, `AnimWait`
  and `WalkAnimWait`, `Zoom`, `TintLevel`, `TintLight`...), renamed by `Script/rename_fields.py`, which
  resolves each reference through libclang.
- Measured (MSVC x64 `/O2`, LOG.md "5.1a step 1"): a character is 752 bytes as before (392 + 360 in two
  arrays, now one); every per-character update and draw function has the same or fewer instructions but
  `UpdateEffectiveValues` (11 to 12); no loop gained an accessor load; the state's fields sit at offset 376
  and up, so their accesses take a 4-byte displacement (more code bytes, not more instructions).

**What 5.1a step 2 built (2026-11-16): the room's entities, and the parts they share with the character.**

- **`RoomDef`** (`Common/game/room_def.h`, upstream's `RoomData`) with `RoomObjectDef`, `RoomHotspotDef`,
  `RoomRegionDef`, `RoomWalkableAreaDef`, `RoomWalkBehindDef`: what the room file gives. The five entity
  definitions, `RoomOptions` and `RoomEdges` are aggregates with field tables; each definition that has events
  declares its own `Events` table, and a walkable area and a walk-behind, which never had events, no longer
  carry an empty one (`RoomDef` went from 26,440 to 23,592 bytes). `RoomDef` itself is still a class: it owns
  the images and a hand-written copy (6.7e).
- **`RoomState`** (`Engine/game/world/room_state.h`, upstream's `RoomStatus`) holds the live entities of one
  room number: `Objects` (`class RoomObject : RoomObjectDef, RoomObjectState`), `Hotspots` (`RoomHotspot :
  RoomHotspotDef, RoomHotspotState`), `Regions`, `WalkAreas`, `WalkBehinds` (the last two have no state struct:
  nothing of them exists only during play). The object's second copy of its definition's fields is gone
  (`x`/`X`, `num`/`Sprite`, `baseline`/`Baseline`, the four `blocking_` values against `Rect BlockingRect`);
  where the copy was narrower, the one field has the definition's type and the places that narrowed implicitly
  narrow explicitly, each pinned by a test at the limit (the legacy save holds 16 bits, 5.12).
- **The live lists hide the definition's, as `Game::Characters` does.** `Room::Objects`, `Hotspots`,
  `Regions`, `WalkAreas`, `WalkBehinds` point at the lists of the bound `RoomState` (never null: at the
  temporary state's when no room is bound), so every `room.Objects[i]` of the engine is the live entity, and
  the room file's values are read through `const RoomDef &Room::Def() const`. With the lists of `RoomDef`
  made `const` in a scratch build the engine compiles: it does not write them.
- **Two lifetimes in one live entity, which is what the room adds to the character's model.** A field of the
  definition part is either *kept*: a room state holds it between visits and a save holds it (an object's
  position, sprite, baseline, flags, name, blocking rectangle; a hotspot's `Name`; a walk-behind's `Baseline`;
  a walkable area's `FaceDirectionRatio`); or *given again*: the room file's value at every load of the room,
  whatever a script did during the visit (a region's light and tint, a walkable area's scaling, the script
  names, events, interactions, properties, ids). `BindRoomState` is the one place that says which: a first
  visit builds the entities from the definitions (`StartObjectsFromDefinitions`,
  `StartEntitiesFromDefinitions`), a later visit or a restored state calls each entity's `RebindDefinition`,
  which takes the definition again but the kept fields. In the field tables the second kind has the role
  `FromDefinition` (section 6), and one test per entity type alters every row and checks both halves after
  save, restore and bind. Region light and tint and the scaling of the room that is loaded when a save is
  made are also saved, by the "Loaded Room State" component; that is unchanged.
- **Shared parts** (section 7), `Engine/game/world/entity_state.h`: `GraphicState` (tint, zoom, rotation,
  pivot, shader, sprite size, drawn size), `ViewAnimState` (`Loop`, `Frame`, `AnimWait`, `Anim`), `MoveState`
  (`MoveListID`, `MoveListHandle`, the constant `kNoMoveList` and the one predicate `IsMoving()`),
  `FrameSoundState` (volume, panning, speed): plain aggregates with tables, bases of `CharacterState` and
  `RoomObjectState`, no use site changed. 24 of the 26 candidate fields went in; `FrameOffsetX/Y` stayed in
  each struct (the character computes them at every update, the object only while it animates). The blocking
  rectangle is not shared yet: the object has a `Rect`, the character four fields (PLAN.md 5.1a step 2, "for
  a later review"). The view, the opacity, the graphic anchor's default and the flags differ between the two
  types and are not shared.
- Measured (MSVC x64 `/O2`, LOG.md "5.1a step 2"): the per-frame functions that touch a room object gained 0
  to 4 instructions (one pointer load, the live list being reached through `Room`); `sizeof(RoomObject)` went
  from 336 to 608 bytes, because the object now holds its definition part (strings, the property map, the
  event tables) where the definition was a separate array, and `sizeof(RoomState)` from 7,136 to 30,608, the
  four entity arrays having moved into it from the room. No timing run yet (PLAN.md "Next up").

## 3. Names

The stem is the Editor's class name (`AGS.Types`). `ScriptX` handles, `CCX` managers, the Tools' `AGF::X` and
`DataUtil::XData` and the Editor's classes keep their names. `Data` is not used in Common because the Tools
have it (`DataUtil::GameData`, `CharacterData`...).

| | Status |
|---|---|
| The three words `XDef`, `X`, `XState` | confirmed |
| Fields declared once, in a struct; no field redeclared | confirmed |
| Room entities carry the `Room` prefix | confirmed |
| `GameDef`, `Game`, `Room`, `RoomState` | follow from the rule; `GameDef` replaces PLAN.md's earlier `GameData` |
| `Def`, not `Info`, as the suffix of a definition (below) | confirmed |
| `GUI` for the live class, the helper namespace `AGS::Common::GUI` (196 uses) renamed `GUIUtil` | confirmed |
| `EngineConfig` for `GameSetup` (`usetup`) | confirmed |
| `GameState` for what is left of `GamePlayState` (section 5), the blocking-loop base class renamed in the same commit; at the end of the sorting, not before | confirmed |
| `RoomDef` for `RoomData`; `RoomStruct` is not a definition and merges into the live `Room` (below) | confirmed |
| Where serialisation goes (section 4) | confirmed |
| The accessors `Game::Current()`, `GameDef::Current()`, `Room::Current()`, `EngineServices::Current()` | confirmed |

**The room has two types today, and only one is a definition.** `RoomData` is the whole record, with the
backgrounds and masks as pixel buffers; the tools and the room file code use it, so it is `RoomDef`.
`RoomStruct : RoomData` adds the same images as Allegro bitmaps and is, by its own header, "the Room data class
prepared for use at runtime", with a TODO to move it to the engine: it is the bitmap half of the live `Room`.
It stays in Common under its name only while `AGS.Native` compiles it (19 mentions; 6.7e moves the editor's
room code to the C library), then merges into `Room`. An earlier draft of this table had a base type for the
tools' part; no such type is needed.

**Why `Def` and not `Info`.** The first draft used the suffix `Info`, after the five records that have the suffix today.
The review found that `GameSetupStruct` has a member `GameInfo`: the map of the game's descriptive texts
(title, developer; 36 uses), which is also a script type, `builtin struct GameInfo` with
`GameInfo_GetTitle()` and its siblings, frozen by C3, and the `v363_gameinfo` block of the game file.
A class of that name for all the game data would sit beside script functions that belong to another thing. In
this code `Info` already means facts about a thing (`SpriteInfo`, `PluginInfo`, the script's game texts), and
this file calls the records "definitions" throughout; Box2D (`b2BodyDef` → `b2Body`) and RimWorld (`ThingDef`
→ `Thing`) name the same pattern so. Renamed: `CharacterInfo`, `InventoryItemInfo`, `RoomObjectInfo`,
`FontInfo`. Staying `Info`: the game texts' member and script type, `SpriteInfo`, `PluginInfo`. The engine's
557 uses of `CharacterInfo` become `Character` in 5.1a step 1 whatever the suffix; what is left to rename is
about 25 uses in Common and 5 in the Tools. The cost is distance from upstream, whose #2058 says
"`CharacterInfo` or `CharacterData`".

| Entity | Definition today | Definition | Live today | Live |
|---|---|---|---|---|
| Character (done, 5.1a step 1) | `CharacterInfo` | `CharacterDef` | `CharacterInfo` + `CharacterExtras` | `Character` (+ `CharacterState`) |
| Inventory item | `InventoryItemInfo` | `InventoryItemDef` | in place | `InventoryItem` |
| Cursor | `MouseCursor` | `MouseCursorDef` | in place | `MouseCursor` |
| View | `ViewStruct`, `ViewLoopNew`, `ViewFrame` | `ViewDef`, `ViewLoopDef`, `ViewFrameDef` | in place | `View`, `ViewLoop`, `ViewFrame` |
| Audio clip | `ScriptAudioClip` | `AudioClipDef` | the definition itself, registered as the script handle | `AudioClip` |
| Audio type | `AudioClipType` | `AudioClipTypeDef` | in place | `AudioClipType` |
| Font | `FontInfo` | `FontDef` | `Font` (inside `fonts.cpp`) | same |
| Dialog | `DialogTopic`, `::DialogOption` | `DialogDef`, `DialogOptionDef` | in place | `Dialog`, `DialogOption` (the options-screen class `DialogOptions` becomes `DialogOptionsState`) |
| GUI | `GUIMain` | `GUIDef` | in place | `GUI` |
| GUI controls | `GUIButton`, `GUILabel`... | `GUIButtonDef`... | in place | names as today |
| Room | `RoomData` | `RoomDef` | `thisroom` (a `RoomStruct`) | `Room` |
| Per-room state | none | none | `RoomStatus` | `RoomState` |
| Room object | `RoomObjectInfo` | `RoomObjectDef` | `RoomObject` | same |
| Hotspot | `RoomHotspot` | `RoomHotspotDef` | `HotspotState` | `RoomHotspot` |
| Region | `RoomRegion` | `RoomRegionDef` | `region_enabled[]`, values in `thisroom` | `RoomRegion` |
| Walkable area | `WalkArea` | `RoomWalkableAreaDef` | `WalkareaState`, values in `thisroom` | `RoomWalkableArea` |
| Walk-behind | `WalkBehind` | `RoomWalkBehindDef` | `walkbehind_base[]` | `RoomWalkBehind` |

`SpriteInfo`, `PluginInfo`, `ScreenOverlay`, `Viewport`, `Camera` and `MoveList` keep their names.

## 4. Where serialisation goes

**A component describes itself; a dead format is kept in one place.** (Revised 2026-11-15 after the user's
question; an earlier draft moved every format into a module of free functions.) A component that serialises
itself is the right design for a format that lives and changes with it. What is wrong today is something else:
save code sits on Common records, in the wrong layer; each format is two hand-written mirror functions that
can disagree; a component carries up to three formats, two of them history; and the readers fill globals.

Two families, which never mix: **game data** is the definitions (`GameDef`, `RoomDef`, the `XDef` records),
written when the game is built and only read by the engine; a **savestate** (a saved game) is the live objects
(`Game`, `Room`, each `X` with its `XState`), written and read by the engine during play.

```
project files --tools--> game data --engine, at load--> GameDef --construct--> Game <--> savestate
```

Game data:

| Format | Written by | Read by | Code lives | Described by | Change |
|---|---|---|---|---|---|
| Project files (`Game.agf`, `Rooms/<n>/data.xml`; C5) | the Editor | the Editor, the tools | `Editor/AGS.Types`, `Tools/data` | C# properties; the XML rows of `xml_fields.h` | none |
| Game file (C1) | the tools: project structs → `GameDef` → file | the engine at startup, into `GameDef` | `Common/game` | each `XDef` record's own `ReadFromFile` and `WriteToFile`; the block structure in `main_game_file.cpp` | the writers the tools write by hand (views, GUIs, the header) are added beside their readers; `data_file_writer.cpp` becomes the conversion from the project structs to a `GameDef` (5.8 step 2) |
| Room file (C1) | the tools (`crmbuild`) | the engine when a room loads, into `RoomDef` | `Common/game/room_file*.cpp` | that module | none |

Savestates:

| Format | Written by | Read by | Code lives | Described by | Change |
|---|---|---|---|---|---|
| New savestate | the engine | the engine, into a second `Game` built from the `GameDef` | one generic reader and writer in `Engine/game/save/` | each component: its field rows (section 6) in the struct's source file, and a function on the live class for what rows cannot say (handles, the derived fields to compute again) | new (5.9, 5.12; section 6a) |
| Today's savestate (C2) | the engine until 5.12, then nothing | the engine, for saves of older engines | all of it in `Engine/game/save/savegame_components.cpp` | hand-written code that no longer changes | the `...Savegame` methods of the six `Common/game` files move in first (5.8 step 1), the eight `Common/gui` files' with 5.1a step 5, the engine structs' own with 5.12; after 5.12 the file only reads |

The rows of an `XDef` struct sit beside it in Common although the savestate code is in the engine: a row
says what a field is, not how a save stores it, and the tools can use the same rows to dump or compare
records. A savestate never contains game data: it holds what differs from it, and the engine never writes the
game or room file.

So one thing moves into a single file, the old save code, because it is about to become a fossil that no
component should carry. No step adds a file per component: a file costs 2.4 s to compile (section 1). The GUI
classes keep their save methods until their data is plain records (5.1a step 5): their members are private.

## 5. `GamePlayState` sorted by owner

| What it holds | Examples | Goes to |
|---|---|---|
| Settings the script changes, play progress | `text_speed`, `speech_mode`, the dialog-options and top-bar fields, `do_once_tokens`, `randseed`, the music queue | stays: the game's own state |
| Blocking and transient state | `wait_counter`, `messagetime`, `in_cutscene`, `fast_forward`, `shakesc_*` | stays for now; some later to the blocking-loop classes |
| The current room | `room_width`, `room_height`, `bg_frame`, `walkable_areas_on`, `room_bg_modified`, `entered_at_x` | done 2026-11-16: all to `Room` (`LoadedWidth`, `LoadedHeight`, `BgFrame`, `BgFrameLocked`, `AnimBackgroundDelay`, `WalkableAreasOn`, `EnteredAtX`, `EnteredAtY`, `EnteredEdge`, `BgModified`, `MaskModified`, `ScriptFinished`); none to `RoomState`: none is kept per room between visits, and moving one there would make it persist. `LoadedWidth` and `LoadedHeight` are not the room's `Width` and `Height`: they are 0 until the first room is loaded and after `unload_game`, which the script's `game.room_width` shows in `game_start` |
| Per-object custom properties | `charProps`, `invProps`, `guiProps`, `dialogProps`, `guicontrolProps` | each live object |
| What outlives a restore | `voice_avail`, `separate_music_lib`, `enable_antialiasing`, the text language | done 2026-11-15: `EngineServices::VoiceAvail`, `SeparateMusicLib`, `TextLanguage`, `TextLocaleName`; `ShouldAASprites()` reads `Config.AntialiasSprites`. The save keeps the `separate_music_lib` slot: read and dropped, written from the engine's value |
| Viewports and cameras | `_roomViewports`, `_roomCameras` | done 2026-11-16: `GameViewports`, held as `Game::Viewports` |

The legacy reader of the "Game State" component keeps its byte order and assigns to the fields where they are.
The script's `game.` variable is read through `CCScriptGame`'s offset switch, not the C++ layout (C3).

## 6. Field tables

The fork already describes `Game.agf` (`xml_fields.h`) and the script bindings (`script_binder.h`) by rows.
The same for records, in two kinds: **field rows** say what each data member of a struct is (5.9), **layout
rows** how a frozen format stores it.

### Field rows (5.9; designed with the user 2026-11-16 and built for the character)

How far to take data-driven code was left open by the design of 2026-11-15. It was decided against the real
`CharacterDef` (32 fields) and `CharacterState` (52), with the tables written and measured first. The names in
the code say *field*, not *property*: `PropertyDesc` and `Character.GetProperty` are the game's custom
properties.

```cpp
// Common/game/character_def.cpp. The struct declares: static AGS::Common::FieldTable<CharacterDef> Fields();
constexpr FieldRow<CharacterDef> kCharacterDefFields[] = {
    Field<&CharacterDef::Baseline>("Baseline"),
    Field<&CharacterDef::Name>("Name"),
    OpaqueField<&CharacterDef::Events>("Events"),
};
static_assert(FieldsCoverStruct(kCharacterDefFields), "CharacterDef: a field has no row in kCharacterDefFields");
```

- **A row is a name, a member and a role** (`Common/util/field_table.h`, header only). The member gives the
  address and, through its type, the operations (`FieldValue<V>`: equal, print, alter), which every row of that
  type shares: numbers, enums, `String`, points, vectors, maps and arrays of those have them, and so has a
  struct with a table of its own (`CharacterState::Anim`); a base class is one row (`BaseFields`). The role is
  `Saved` (a part of the state: a restored game has the value the saved one had), `FromDefinition` (the
  definition gives it again whenever the object is loaded, play may change it meanwhile and a save does not
  hold it: a region's light and tint, a walkable area's scaling, the script names of a room's entities, which
  the room file gives again at every load of the room), `Derived` (computed from
  other fields; a restore computes it again: the character's `EffectiveOffset`, `EffectiveAnchor`,
  `SpriteWidth`, `SpriteHeight`, `FrameOffsetX`, `FrameOffsetY`, `OffsetZoom`) or `Opaque` (not a value that can
  be compared or stored; listed so that the table is whole: the two event tables for now, which have no
  comparison). `FromDefinition` was added with the room's tables (the user, 2026-11-16): until then such a row
  borrowed `Derived`, and a test had to tell "given again by the file" from "computed" by the kind of struct.
  The comparison, the print and the alteration descend into a base or a member that has a table, so a
  difference is reported by field (`TintRed: 0 != 1`, `Anim.Delay: 0 != 1`) and a role applies at every depth;
  `AlterFields(object, role)` alters the rows of one role, which is how a test of a load shows both halves: a
  `Saved` field keeps the altered value, a `FromDefinition` field comes back as the definition's.
- **One row per data member, never more.** The bits of `Flags` are not rows: for a comparison and for a save
  `Flags` is one integer.
- **What a row does not hold** (the user, 2026-11-16, on these findings):
  - *the default:* it is the member initializer, as in `xml_fields.h`;
  - *a range:* nothing reads one yet, and the character's range-checked setters follow four policies
    (`AudioPanning`, `AudioSpeed`, the volume, the anchors and the rotation clamp silently; `Opacity`,
    `Transparency`, `BlinkInterval` and two flags quit; `Scaling` warns and clamps; `Frame`, `Baseline` and the
    blocking size check nothing), so a range in a row could not replace them without changing behaviour. A
    range is added to the row type when something reads it (a check of a restored save, an inspector);
  - *script exposure:* a binding is a function per property with an address of its own, which plugins call
    (C4), and the script VM is an engine ring that Common cannot name. Script properties are bound from the
    member in the engine's binding table (below), not from the row;
  - *behaviour:* no change hook. The engine writes these fields directly in hundreds of places, so a hook
    would fire only for writes through the table and code would come to rely on what the engine's own writes
    never trigger. After a write through the table (a restore, an inspector) the live class recomputes its
    `Derived` fields in one function; a test compares them, so a forgotten one fails.
- **A field without a row does not build.** `FieldsCoverStruct` counts the members of the struct at compile
  time (the largest number of initializers its aggregate initialization takes) against the rows; checked with
  MSVC, GCC 13, Clang with libc++ and GCC 8.3 (the Debian 10 builder image). So an `XDef` and an `XState` is an
  **aggregate**: no private data, no constructor written out, no virtual function. `CharacterDef` needed its
  event table public (`Events`) and its five defaulted special members deleted. A struct that keeps
  constructors (`AnimFlowParams`, `ViewAnimateParams`) has a table without the count. The count does not tell
  two rows of one member from two members (the table's test does, `FieldsAreDistinct`) and does not see the
  live class's own members (`Character::_graphicSpace`: it moves into the state, 5.9 step 2).
- **The table is in the struct's source file**, the struct declares `static FieldTable<X> Fields();` with a
  forward declaration, no include. In the header the two character tables cost about 0.13 s on each of the 35
  files that include `character.h` (0.85 s without, 0.98 s with, the fastest of 7 compiles of a file that
  includes only it, MSVC); in the source file once.
- **No schema file** (the user, 2026-11-16). A generator would have to run in every build system (CMake, the
  hand-written solutions, Xcode, Android) or its output be checked in with a drift check, and a generated
  struct is harder to read, to rename (`rename_fields.py`) and to port from upstream. With the count the only
  thing written twice is the field's name, and the compiler guards it. One schema for C++ and C# stays
  unplanned (L.3).
- **What reads the rows.** Today `DiffFields`, `PrintFields` and `AlterFields`: the test
  `SavegameContent.EveryCharacterFieldIsRestored` sets every saved field of a character, saves, restores and
  compares, and names a lost field (today's save code loses none of the 73; with a derived field marked saved
  it fails with `SpriteWidth: 1 != 0`). The hand-written test checked 20 of the 84 fields. Next: the comparison
  of two `Game` objects (5.11) and the new save format (5.12), which adds two operations per value type (read,
  write), not per row. A row's name becomes a part of the save format the day a save holds it: from then on a
  renamed member keeps its row's name.
- **Script properties are bound from the member, in the binder** (5.9 step 3). Of the character's 142
  property functions (152 binding rows; all types have 1,015), 77 are exact forms: 35 return a member, 14
  assign one, 28 test or set a flag bit; one is called by engine code by name. A function template makes each
  (`ScMethod<&FieldGetter<Character, &Character::Z>>("Character::get_Z")`), a plain function returning `int`
  or `float` whatever the member's width, since plugins call it as a C function. The script object's type is a
  template argument: a member of the second base is at another offset in `Character`. The generated code is
  the hand-written function's (`mov eax,[rcx+190h]`, MSVC `/O2`) when the template reads through a reference
  cast to the member's class; through `obj->*Member` MSVC adds a null test of the object, four instructions.
  So each conversion is proven by comparing the objects, as 5.2 step 2 was. A function with a check, a clamp,
  a view number plus one or a call after the write stays a named function (65 of the 142).
- **The drift check of `agsdefns.sh` needs no rows:** it compares the imports the compiler reads from
  `agsdefns.sh` with the registered names (`Engine/test/data/script_exports.txt`); no test does that today
  (5.9 step 4).

### Layout rows

```cpp
// How a frozen format lays a record out: beside the format. Order, width, padding.
inline constexpr LayoutRow<CharacterDef> kCharacterFileLayout[] = {
    Int32<&CharacterDef::NormalView>(),
    Int16<&CharacterDef::Baseline>(),
    Skip(2), // alignment padding in the file
};
```

- **Layout rows** are for the game file only, where C1 freezes order, width and padding: one loop for the
  reader and one for the writer, so the two cannot disagree; a hand-written row covers what a plain row cannot.
  They are optional: the game file code can also stay as moved (5.8 step 2). Saves need none (6a).
- One plain property of a character is written by hand in twelve places today (the C# property, the project
  struct, its XML row, the conversion, the field, four stream lines, getter, setter and two binding rows, the
  `agsdefns.sh` line). With field rows, members bound to the script and the new save format it is nine (the
  field's row replaces the four stream lines and the getter and setter go), and the compiler or a test guards
  each pair.

## 6a. The save format is free to change

The user, 2026-11-15: the current save format need not be kept; old saves are read by a legacy routine. C2
stays as written (saves made by 3.99+ engines load), and constrains reading only.

- **The legacy reader** is today's code, moved into one place (5.8 step 1) and then left alone: it never gains
  a field. Its writers are deleted when the new format is written. It still fills a `Game`, so its assignments
  follow the fields as they move. Guard: the `compat_restore` fixtures, plus one written by the last engine
  with the old format.
- **The new format is written from the field rows** (5.12; the rows with the role `Saved`, section 6). Proposed shape: per type, the list of saved
  field names and types once, then the objects' values in that order. The reader matches the saved names
  against the current rows: a field the save lacks keeps the value the `Game` got from its definition, a field
  the engine no longer has is skipped. So adding a property needs no version number and no reader change.
- **Only what differs from the definition needs writing.** Today every frame of every view and the whole of
  every GUI go into each save. Because a `Game` starts as a copy of its definitions, an object equal to its
  definition can be left out.
- **A shared part is one block** in the new format, written by the part's rows wherever the part is used. The
  interleaving of section 7 concerns the legacy reader only.
- **Rows do not cover everything.** Managed objects (their managers serialise them already), script global
  data, plugin data, bitmaps, and the lists that grow during play (overlays, move lists, room states) keep
  writers of their own inside the new container.
- **Open, for 5.12:** whether lists are matched by script name where one exists, so that a save survives a
  patch that adds a character or a GUI (today the counts must match); the size and time against today's
  format, measured on the corpus game.

## 7. Shared parts

The overlap, by meaning, from the field-by-field comparison:

| Part | Fields | Who has it | State of the logic |
|---|---|---|---|
| Identity and interaction | id, script name, name, events, custom properties | room entities share `RoomObjectBase`; characters, inventory items and GUI objects repeat it | properties share helpers; the four `Run...Interaction` functions are near-identical copies |
| Graphic properties | tint (5 values and flags), opacity, blend mode, rotation, pivot, shader | characters, room objects, overlays; GUI objects have four of them | setters are copies with the same range checks; only blend-mode validation is shared |
| View animation | view, loop, frame, wait, `ViewAnimateParams` | characters, room objects, animated buttons, animated overlays | all call `CycleViewAnim`; the wrappers are copies that differ at the end of an animation; the cursor has its own loop |
| Movement | move list slot and handle | characters, room objects | both call `do_movelist_move`; the stage loop is copied |
| Blocking rectangle | x, y, width, height | characters, room objects | centred on the character, left-anchored on the object |
| Frame sound | volume, panning, speed | characters, room objects | shared (`CheckViewFrame`) |

What differs under the same name, each a behaviour question: scale (percent as `int16` on characters and
objects, a float factor on overlays and GUIs); flag polarity (`CHF_NOINTERACT`, `NOBLOCKING`, `MANUALSCALING`,
`NOLIGHTING` against the positive `OBJF_` forms); "none" values (no view is -1, `UINT16_MAX` or a quit; not
moving is 0 on characters, -1 or 0 on objects); default anchor ((0.5, 1), (0, 1), (0, 0)); setter side effects
(`SetObjectBaseline` marks the object changed and `Character_SetBaseline` does not; the object's light level is
not clamped).

In today's save streams the shared groups are interleaved with other fields; only the tail from `AudioSpeed`
on has the same layout in characters and objects. The legacy reader keeps reading them field by field; the new
format writes a part as one block (6a).

Two steps: the **data** in 5.1a (a group that a second type already has with the same meaning is declared once,
when that type is split; nothing visible changes), the **logic** in 5.10 (tests that pin today's behaviour per
type first). Part names are chosen when the second type is split.

## 8. Restore and reset with two instances (5.11)

| Obstacle | Way through |
|---|---|
| The managed pool registers objects by the address of the globals | one pool per `Game`; registration goes through the current `Game` |
| Scripts import `&play`, the static arrays, `&_sc_PlayerCharPtr` by address | a restore already unlinks and relinks the scripts; the exports are registered again after the exchange |
| Plugins hold `&game.chars[i]`, `&scrObj[i]` | not settled. Exchanging the contents element by element (not the vectors' buffers) keeps the addresses (C4), but the pool filled during the read maps handles to the second instance's addresses and must be rebound. Switching the current pointer keeps the pool right and moves the addresses, so plugins would have to fetch their pointers again after a restore |
| Script global data is copied into the live instances | it is already read into `RestoredData` first; it becomes part of the `Game` |
| `validate_restored_save` runs in script with the restored state live | exchange, validate, exchange back on a refusal: with two instances a rollback is the same operation |
| Dynamic sprites live in the sprite cache and are freed first today | each `Game` owns its list of dynamic sprite slots; the hardest part, to design in 5.11 |
| Audio channels, draw caches | rebuilt after the exchange, as `DoAfterRestore` does today |
| The loaded room | the second `Game` loads its own `Room` |

**Sprites** (the target, the user 2026-11-16; to design in detail in 5.11). The sprite cache holds two things
with different lifetimes today. The static sprites are read from the game's sprite file on demand, kept up to a
memory limit and evicted: an asset cache, which lasts as long as the game file and holds nothing a save needs.
The dynamic sprites are bitmaps a script creates; they sit in the same cache, locked against eviction, in the
same number space (a dynamic sprite's number is valid wherever a sprite number is, C3), and they are game
state: a save writes each bitmap, and a restore frees all of the running game's before it reads the save
(`DoBeforeRestore`), which is one reason a failed restore cannot go back. The target:

- a **sprite asset service** in `EngineServices` for the static sprites alone: number to bitmap, load and
  evict; its sizes and flags belong to the definition and are not written during play;
- the **dynamic sprites belong to `Game`**: a table of slot to bitmap and info, saved and restored with the
  game, destroyed with it;
- **one lookup in front of both**: a number in the game's dynamic table resolves there, any other goes to the
  asset service;
- a **restore** builds the second `Game`'s dynamic sprites inside it; on success the games are exchanged and
  the textures drawn from dynamic slots are dropped, on failure the second `Game` is discarded.

5.1a step 3 takes the first step only: `Game::SpriteInfos` is the live info table (a copy of the loaded one,
which hides it), and the cache is pointed at it instead of at the definition's.

A decision for the user in 5.11: `RestartGame()` without a restart point fails today (the automatic first
restart point went with the pre-3.6.1 paths); it could start a new game from the `GameDef` instead, which is a
behaviour change.

## 9. Build time

The cost of a file is its headers. Consequences:

- The moves of section 4 add no file. The file count of the engine does not rise in 5.1, 5.1a or 5.8.
- `game/game.h` must not become the header that includes every entity. Chosen in 5.1 step 3 (2026-11-15) from
  a probe: forcing today's module headers for every member type into the 120 sources that use the contexts
  made those files 28% slower to compile (+0.95 s each; `Engine` +17%, the whole build +8%; the files without
  the forced include moved +4%, the noise). The count behind "about 335 files" was the word `game`, include
  paths counted in; `game/game.h` has 101 includers and 73 files named `game` or `play`. So:
  - the contexts have headers of their own (`game/game_context.h`, `game/world/room_context.h`), included only
    by the files that use a context; `game/game.h` stays the module's function declarations;
  - a member is held by value when its type is complete from a light header: the `GameDef` base and `Play`
    (what their users included already), a state struct of plain values declared in the context header, a
    `std::vector` of a forward-declared type (C++17, with the context's constructor and destructor out of
    line), an array of a type from a header without includes of its own (`game/dynobj/script_objects.h`);
  - a member whose type only a module header defines is held through a reference member bound to an object
    the context owns (`Game::Scripts`, `Room::TempState`, `EngineServices::Sprites`): one more load, the same
    syntax at the use;
  - a context header never includes a module header.
  What it cost in the end (BASELINE.md "After 5.1 step 3", MSVC on an idle machine): the build +2.4%, the
  median per object +5.1%, 0.27 s more on each of the 95 engine sources that include a context header. The
  by-value `GameDef` base, `Play` and `Viewports` are that cost: their three headers reach every user of
  `Game`, where a file used to include one: 42 of the 95 did not see both `game_def.h` and `game_state.h`
  before and carry 21 of the 26 s; a file that already saw both pays 0.08 s. Accepted by the user
  (2026-11-16). `Play` and `Viewports` through reference members would give back at most those 26 s for one
  more load per use: decided against. What lowers compile time from here is PLAN.md 2.10b, in its order: a
  precompiled header of stable content (the standard headers alone are 0.38 s of every file), and the big
  headers trimmed as 5.1a splits their structs. 5.1a step 1 (2026-11-16) took `game/world/character.h` out of
  `game_state.h`, which named nothing of it and passed it to every user of a context, and the definition's
  header out of `game_def.h` (a forward declaration, as for the views and dialogs): of the 451 first-party
  translation units, 34 see `character.h` where 104 did, and 40 see `character_def.h` where 119 did.
- Two numbers go into BASELINE.md and are taken before and after each batch from the ninja log: the total
  compile time and the median per file (2.10). Lowering the fixed cost (a precompiled header; unity builds
  only where `compare_objects.py` does not need per-file objects) is the second half of 2.10.

## 10. Wanted but out of reach under C1 to C6 (L.3)

What a new engine would do and the contract forbids here: identity by a stable id instead of the array index
in the script ABI (C3; inside saves it is open, 6a); handles instead of addresses for plugins (C4); blocking
calls as script coroutines instead of nested game loops (script semantics); one schema generating the C++ rows
and the C# properties. Saves as differences from the definitions were on this list until 2026-11-15 (6a).

## 11. Defects found on the way

Each read in the code on 2026-11-15. The first three are fixed since (the same day, a test each in
`Engine/test/savegame_components_test.cpp`, seen failing first; no stream byte changes):

- Fixed: `CharacterExtras::ReadFromSavegame` read the animation volume into a member nothing else used and
  rebuilt the animation with a local fixed at 100: the volume was lost on load. `RoomObject` did it correctly.
  The dead member `cur_audio_volume` is gone.
- Fixed: `AnimatedOverlay::ReadFromSavegame` read the wait counter into a local and never assigned `_wait`.
- Fixed: `RoomStatus` wrote the counts as hotspots, regions, walk areas, walk-behinds and read them as hotspots,
  regions, walk-behinds, walk areas. No existing save loads differently: both limits are 16 and have been since
  the counts came (`ddc9d21b7`, 2024-02; `MAX_WALK_AREAS` went from 15 to 16 a month before), read in the history,
  not tried with saves.
- Checked, open:
  - `init_object_states` assigns `flags` and `baseline` twice (`room.cpp`): the same values, redundant code,
    not a defect.
  - `move_object` (`object.cpp`) assigns `objs[id].moving = add_movelist(...)` on an object that is already
    moving without `OnStopMoving()`: the old move list slot is never freed (it stays in `movelists` and goes
    into saves) and `Object.MotionPath` keeps returning the earlier path. The character code stops first.
  - `GetObjectIDAtRoom` (`object.cpp`) tests a point in unscaled sprite pixels against the scaled size: above
    100% the hit area reaches past the sprite to the right and below, under 100% it shrinks to the top-left.
    `GetCharIDAtRoom` passes the unscaled size.
  - `RoomStatus::ReadFromSavegame` does not check the four counts against its fixed arrays: a damaged save
    with a count over the limit writes past them.
  Upstream `master` and `ags4` not checked for these.
