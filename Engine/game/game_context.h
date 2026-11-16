//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#pragma once

#include <vector>

#include "game/dynobj/script_objects.h"
#include "game/game_def.h"
#include "game/game_state.h"

struct DialogTopic;
struct SpeechLipSyncLine;
struct ViewStruct;

// An internal state id, telling how the new room was entered.
// This defines which events to run after room was loaded.
enum EnterNewRoomState
{
    kEnterRoom_None = 0,
    kEnterRoom_Normal = 1,       // changed to room and finished fade-in
    kEnterRoom_FirstTime = 2,    // same as normal, but this is first time this room is visited
    kEnterRoom_RestoredSave = 3, // loaded a room after restoring a save
};

// The pending change of room and how the room now loaded was entered (game/world/room.cpp, main/game_run.cpp)
struct RoomChangeState
{
    EnterNewRoomState InNewRoom = kEnterRoom_None;
    // The value InNewRoom had when the room was entered, kept for the events that run after the fade-in.
    EnterNewRoomState NewRoomWas = kEnterRoom_None;
    // Where to put the player character in the room being entered; 0 means no instruction. A value of 1000 or more
    // is an entrance at an edge of the room, set by Character.ChangeRoomAutoPosition: the thousands give the edge
    // (1000 left, 2000 right, 3000 bottom, 4000 top), the rest the coordinate along it (y for the sides, x for
    // bottom and top; 0 is the middle). Reset once applied.
    int Pos = 0;
    // Position and loop (facing) to give the player character in the room being entered, by Character.ChangeRoom;
    // SCR_NO_VALUE when not set. Reset once applied.
    int X = SCR_NO_VALUE;
    int Y = SCR_NO_VALUE;
    int Loop = SCR_NO_VALUE;
    // Whether to move the player character onto the walkable area after placing it at X, Y.
    bool PlaceOnWalkable = false;
};

// The speech in progress (game/world/character.cpp, game/dialog/)
struct SpeechState
{
    // Indexes of the currently speaking character and of the one playing a speech animation; -1 when none.
    int CharSpeaking = -1;
    int CharSpeakingAnim = -1;
    // Used while in dialog to track whether screen needs updating: set when a character said a line of speech
    int SaidSpeechLine = 0;
    // Set when a character said a line of speech; the text window options use it for the QFG4-style layout
    int SaidText = 0;
    // Sierra-style speech: the speaking character (index in FaceTalking, -1 when none, and FaceTalkChar) and the
    // state of its portrait's animation.
    int FaceTalking = -1;
    int FaceTalkView = 0;
    int FaceTalkWait = 0;
    int FaceTalkFrame = 0;
    int FaceTalkLoop = 0;
    int FaceTalkRepeat = 0;
    int FaceTalkAllowBlink = 1;
    int FaceTalkBlinkLoop = 0;
    CharacterInfo *FaceTalkChar = nullptr;
    // Do override default portrait position during QFG4-style speech overlay update
    bool Qfg4OverridePlacementX = false;
    bool Qfg4OverridePlacementY = false;
};

// Lip-sync speech settings (game/dialog/speech.cpp): game loops per character of text, the game's lip-sync lines
// (NumLines of them), and the line and phoneme being shown, CurLine -1 when none.
struct LipSyncState
{
    int LoopsPerCharacter = 0;
    std::vector<SpeechLipSyncLine> Lines;
    int NumLines = 0;
    int CurLine = -1;
    int CurLinePhoneme = 0;
};

// The results of the last text layout (game/lib/string.cpp, game/dialog/display.cpp)
struct TextLayoutState
{
    // The width of the widest line found by the last break_up_text_into_lines() call (or display code)
    int LongestLine = 0;
    // The efficient length of the last source text prepared for display.
    // TODO: refactor this out; currently it is set at the every get_translation call.
    // Be careful: a number of Say/Display functions expect it to be set beforehand.
    int SourceTextLength = -1;
};

// One play-through: what the game file says (the base, GameDef) and everything that changes while the game
// runs, which is what a save holds. The definitions and the live state are one object until each entity
// gets a live copy (PLAN.md 5.1a). The game logic reaches it through Game::Current(); a function that uses
// it more than once takes a reference at its top:
//     Game &game = Game::Current();
class Game : public GameDef
{
public:
    Game();
    ~Game(); // out of line: later members are vectors of types that are incomplete here
    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    // The instance in use
    static Game &Current() { return *_current; }
    // Makes another instance the current one, for a test, and returns the one that was;
    // nullptr brings the engine's own instance back
    static Game *SetCurrent(Game *game);

    // The game's own state: settings the script changes, play progress, blocking and transient state
    GamePlayState Play;

    // The runtime state of the characters that CharacterInfo does not hold, by character index
    std::vector<CharacterExtras> CharExtras;
    // The game's views.
    std::vector<ViewStruct> Views;
    // The dialog topics of the game, by dialog number
    std::vector<DialogTopic> Dialogs;
    // The player character, and its handle for scripts (the script object "player").
    CharacterInfo *PlayerChar = nullptr;
    int32_t ScPlayerCharPtr = 0;
    // The baseline of the character that the last search for a character at a room point found, compared with
    // ObjLowestYp and the walk-behind's to tell which is in front.
    int CharLowestYp = 0;
    // The baseline of the object that GetObjectIDAtRoom found last, -1 if none; GetLocationTypeImpl (game.cpp)
    // compares it with CharLowestYp and the walk-behind's to tell which is in front.
    // TODO: return it together with the object ID instead of through a member.
    int ObjLowestYp = 0;
    // The number of the room that is loaded now.
    int DisplayedRoom = -10;
    // The pending change of room
    RoomChangeState RoomChange;
    // Fixed game fps, set by script
    int FramesPerSecond = 40;
    // Pause nesting level, above 0 while the game is paused.
    int Paused = 0;
    // Data format version of the loaded game; kGameVersion_Undefined until load_game_file has read the whole
    // game data (gamedataver is set earlier, by the preload of the game's description)
    // TODO: get rid of this member, use gamedataver
    GameDataVersion LoadedDataVersion = kGameVersion_Undefined;

    // The script objects of the game entities are registered in the managed pool by address once per game: they
    // must not move (the room entities' arrays are fixed, the vectors are sized once at game init).
    // Script objects of the dialogs, by dialog number
    std::vector<ScriptDialog> ScDialogs;
    // The script objects of the inventory items, by item index.
    std::vector<ScriptInvItem> ScInvItems;
    // The script objects of the room's hotspots, by hotspot index.
    ScriptHotspot ScHotspots[MAX_ROOM_HOTSPOTS];
    // The script objects of the room objects, by object index.
    ScriptObject ScObjects[MAX_ROOM_OBJECTS];
    // The script objects of the room's regions, by region index.
    ScriptRegion ScRegions[MAX_ROOM_REGIONS];
    // The script objects of the room's walkable areas, by area number.
    ScriptWalkableArea ScWalkableAreas[MAX_WALK_AREAS];
    // The script objects of the room's walk-behinds, by walk-behind number.
    ScriptWalkbehind ScWalkBehinds[MAX_WALK_BEHINDS];

    // The speech in progress
    SpeechState Speech;
    // Lip-sync speech settings
    LipSyncState LipSync;
    // The results of the last text layout
    TextLayoutState TextLayout;

private:
    static Game *_current;
};
