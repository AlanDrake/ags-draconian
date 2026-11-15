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
- **The sprite cache is not in it yet.** It is constructed on a reference to `game.SpriteInfos`, which it
  resizes and writes for dynamic sprites, and the texture cache of `draw.cpp` binds to it during static
  initialization. It moves when the sprite table gets its owner (5.1 step 3; the dynamic sprites of each
  `Game` are 5.11's).
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
- Two bases must not share a member name. `CharacterInfo` and `CharacterExtras` both have `flags` (different
  bits) and the two save methods; the first is renamed, the second leaves with 5.8 step 1.
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
| Character | `CharacterInfo` | `CharacterDef` | `CharacterInfo` + `CharacterExtras` | `Character` (+ `CharacterState`) |
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
| New savestate | the engine | the engine, into a second `Game` built from the `GameDef` | one generic reader and writer in `Engine/game/save/` | each component: its property rows beside the struct, and a hook on the live class for what rows cannot say (handles, caches to rebuild) | new (5.9, 5.12; section 6a) |
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
| The current room | `room_width`, `room_height`, `bg_frame`, `walkable_areas_on`, `room_bg_modified`, `entered_at_x` | `Room`, `RoomState` |
| Per-object custom properties | `charProps`, `invProps`, `guiProps`, `dialogProps`, `guicontrolProps` | each live object |
| What outlives a restore | `voice_avail`, `separate_music_lib`, `enable_antialiasing`, the text language | done 2026-11-15: `EngineServices::VoiceAvail`, `SeparateMusicLib`, `TextLanguage`, `TextLocaleName`; `ShouldAASprites()` reads `Config.AntialiasSprites`. The save keeps the `separate_music_lib` slot: read and dropped, written from the engine's value |
| Viewports and cameras | `_roomViewports`, `_roomCameras` | `Game`, as objects of their own |

The legacy reader of the "Game State" component keeps its byte order and assigns to the fields where they are.
The script's `game.` variable is read through `CCScriptGame`'s offset switch, not the C++ layout (C3).

## 6. Field tables

The fork already describes `Game.agf` (`xml_fields.h`) and the script bindings (`script_binder.h`) by rows.
The same for records, in two kinds:

```cpp
// What the property is: beside the struct.
inline constexpr PropertyRow<CharacterDef> kCharacterDefProps[] = {
    Prop<&CharacterDef::Baseline>("Baseline"),
    Flag<&CharacterDef::Flags, CHF_NOINTERACT>("Clickable", Inverted),
    Prop<&CharacterDef::ScriptName>("ScriptName", ReadOnly),
};

// How a frozen format lays it out: beside the format. Order, width, padding.
inline constexpr LayoutRow<CharacterDef> kCharacterFileLayout[] = {
    Int32<&CharacterDef::NormalView>(),
    Int16<&CharacterDef::Baseline>(),
    Skip(2), // alignment padding in the file
};
```

- **Property rows** (5.9) give the new save format (6a), comparison and dump of two objects (round-trip tests,
  the check of a restore, a debug inspector), the plain getters and setters of the script API, and a drift
  check of `agsdefns.sh`.
- **Layout rows** are for the game file only, where C1 freezes order, width and padding: one loop for the
  reader and one for the writer, so the two cannot disagree; a hand-written row covers what a plain row cannot.
  They are optional: the game file code can also stay as moved (5.8 step 2). Saves need none (6a).
- One property, the character's `Baseline`, is written by hand in twelve places today (the C# property, the
  project struct, its XML row, the conversion, the field, four stream lines, getter, setter and two binding
  rows, the `agsdefns.sh` line). Rows bring that to about eight, with the pairs unable to drift. One schema for
  C++ and C# would bring it to one; that is not planned (L.3).

## 6a. The save format is free to change

The user, 2026-11-15: the current save format need not be kept; old saves are read by a legacy routine. C2
stays as written (saves made by 3.99+ engines load), and constrains reading only.

- **The legacy reader** is today's code, moved into one place (5.8 step 1) and then left alone: it never gains
  a field. Its writers are deleted when the new format is written. It still fills a `Game`, so its assignments
  follow the fields as they move. Guard: the `compat_restore` fixtures, plus one written by the last engine
  with the old format.
- **The new format is written from the property rows** (5.12). Proposed shape: per type, the list of saved
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

A decision for the user in 5.11: `RestartGame()` without a restart point fails today (the automatic first
restart point went with the pre-3.6.1 paths); it could start a new game from the `GameDef` instead, which is a
behaviour change.

## 9. Build time

The cost of a file is its headers. Consequences:

- The moves of section 4 add no file. The file count of the engine does not rise in 5.1, 5.1a or 5.8.
- `game/game.h` must not become the header that includes every entity: a `Game` holding every list by value
  would make about 335 files pay for all of them. The mechanism is chosen in 5.1 step 3 (candidate: `Game`
  holds its sections through pointers to types defined in the domain headers) and judged by the numbers.
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
