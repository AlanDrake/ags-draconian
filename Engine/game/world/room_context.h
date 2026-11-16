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

#include <memory>
#include <vector>

#include "game/roomstruct.h"

class RoomHotspot;
class RoomObject;
class RoomRegion;
class RoomState;
class RoomWalkableArea;
class RoomWalkBehind;
namespace AGS
{
namespace Engine
{
class RuntimeScript;
using PRuntimeScript = std::shared_ptr<RuntimeScript>;
} // namespace Engine
} // namespace AGS

// The loaded room: what the room file says (the base, RoomStruct, which also holds the masks and backgrounds
// as bitmaps) and what exists while the room is loaded. The definitions of the room's entities are read through
// Def() and are not written while the game runs; through a Room, the names Objects, Hotspots, Regions, WalkAreas
// and WalkBehinds are the live entities of the bound room state (State), which hide the definitions' lists. One
// instance for now; a Game owns its Room once a second Game can exist (PLAN.md 5.11). The game logic reaches it
// through Room::Current(); a function that uses it more than once takes a reference at its top:
//     Room &room = Room::Current();
class Room : public AGS::Common::RoomStruct
{
    // Owns TempState; declared first, so that it exists when TempState is bound to it
    std::unique_ptr<RoomState> _tempState;

public:
    Room();
    ~Room(); // out of line: RoomState is incomplete here
    Room(const Room &) = delete;
    Room &operator=(const Room &) = delete;
    // Reloading the room from its data goes through the base's assignments
    using AGS::Common::RoomStruct::operator=;

    // The instance in use
    static Room &Current() { return *_current; }
    // Makes another instance the current one, for a test, and returns the one that was;
    // nullptr brings the engine's own instance back
    static Room *SetCurrent(Room *room);

    // What the room file says, as loaded
    const AGS::Common::RoomDef &Def() const { return *this; }

    // Binds a room state: sets State and the live lists below (Objects, Hotspots, Regions, WalkAreas, WalkBehinds)
    // to it; nullptr unbinds (State and Objects nullptr, the four other lists those of TempState)
    void SetState(RoomState *state);

    // The state of the current room: the saved state of its number, or TempState; nullptr when no room is loaded
    RoomState *State = nullptr;
    // The state of the current room if it is a temporary one (number >= MAX_ROOMS), e.g. an intro room
    RoomState &TempState;
    // The live objects of the current room (the first of State->Objects), nullptr when the room has none or no room is
    // loaded; their number is State->ObjectCount. It hides RoomDef::Objects on purpose: through a Room the name is the
    // live list, and the definitions are Def().Objects.
    RoomObject *Objects = nullptr;
    // The live hotspots, regions, walkable areas and walk-behinds of the current room (the lists of State, by number),
    // never nullptr: with no state bound, the lists of TempState. Each hides the list of RoomDef of the same name on
    // purpose: through a Room the name is the live list, and the definitions are Def().Hotspots...
    RoomHotspot *Hotspots = nullptr;       // hides RoomDef::Hotspots
    RoomRegion *Regions = nullptr;         // hides RoomDef::Regions
    RoomWalkableArea *WalkAreas = nullptr; // hides RoomDef::WalkAreas
    RoomWalkBehind *WalkBehinds = nullptr; // hides RoomDef::WalkBehinds
    // The runtime script of the current room
    AGS::Engine::PRuntimeScript Script;
    // Set to 1 when the room's background frame changed, until the next draw updates the background's texture
    int BgJustChanged = 0;
    // The walkable area mask as loaded with the room, before disabled areas are erased from it
    // (WalkAreaMask); redo_walkable_areas copies it back. Created by the room loader (room.cpp).
    AGS::Common::Bitmap *WalkAreaBackup = nullptr;
    // Scratch copy of the mask that prepare_walkable_areas fills and hands out to the pathfinder, same size
    // as the mask. Created by the room loader (room.cpp).
    AGS::Common::Bitmap *WalkableAreasTemp = nullptr;
    // Quick report that no walk-behinds are in this room
    bool NoWalkBehindsAtAll = false;
    // The walk-behind textures are for this background
    int WalkBehindsCachedForBgNum = -1;
    // Set when a walk-behind baseline changed, until the next room draw; while set, the cached object images
    // are not reused
    bool WalkBehindBaselinesChanged = false;

    // The room on screen as the game changes it. These were fields of the game state (GamePlayState); the save
    // keeps them at their places in the game state's layout (GamePlayState::ReadFromSavegame, WriteForSavegame,
    // and the room component for BgModified and MaskModified). None is kept per room between visits.
    //
    // The size of the loaded room as the script's game.room_width/room_height report it, 0 until a room has
    // been loaded in this game
    int LoadedWidth = 0;
    int LoadedHeight = 0;
    // 0 while the room's on_call runs (CallRoomScript), 1 after; the script's game.roomscript_finished
    int ScriptFinished = 0;
    // The game loops between background frames, the room file's BgAnimSpeed when the room is loaded; the script's
    // game.anim_background_speed
    int AnimBackgroundSpeed = 0;
    // The background frame shown now, and whether the frame is locked (no animation)
    int BgFrame = 0;
    int BgFrameLocked = 0;
    // Whether each walkable area is enabled, by area number
    char WalkableAreasOn[MAX_WALK_AREAS]{};
    // Where the player character entered the room, and from which edge (0 left, 1 right, 2 bottom, 3 top;
    // -1 not from an edge, -10 once the edge has been left)
    int EnteredAtX = 0;
    int EnteredAtY = 0;
    int EnteredEdge = 0;
    // Tells which room background frames and which masks (by RoomAreaMask) were modified by script drawing
    // operations
    std::vector<bool> BgModified;
    bool MaskModified[kNumRoomAreaTypes]{};

    // Resets the members above (LoadedWidth ... MaskModified) to their initial values; called where the game
    // state is reset as a whole (unload_game), as these were reset with it.
    void ResetPlayState();

private:
    static Room *_current;
};
