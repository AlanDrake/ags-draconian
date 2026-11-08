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

#include "ac/roomobject.h"
#include "game/roomstruct.h"
#include "game/savegame.h"
#include "game/scripteventtable.h"
#include "util/string_types.h"

// Forward declaration
namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS
using AGS::Common::Stream;

// RoomStatus runtime save format
// Before 4.0: 0 (3.5.0 pre-alpha, and "Room States" of 3.5.0 by mistake); 1 (3.5.0) new movelist format;
// 2 (3.6.0.16) hotspot and object names; 3 (3.6.0.25) object animation volume; 4 (3.6.0.41) room state's
// contentFormat; 5 (3.6.1.9) removed movelists, save externally; 3060214 serialize current room's changes to
// masks; 3060304 room object's blocking rect x,y; 3060310 audio panning and speed; 3060314 arbitrary number of
// room bgs. 4000000: room object blendmodes etc.
enum RoomStatSvgVersion
{
    kRoomStatSvgVersion_40003 = 4000003, // room object flags as 32-bit, facedirratio
    kRoomStatSvgVersion_40008 = 4000008, // custom properties for regions and walk-areas
    kRoomStatSvgVersion_40016 = 4000016, // room objects expose script motion path
    kRoomStatSvgVersion_40018 = 4000018, // shaders
    kRoomStatSvgVersion_40020 = 4000020, // expanded and bit more consistent anim params serialization
    kRoomStatSvgVersion_40024 = 4000024, // sync with 3.6.2.14 (current room's changes to masks)
    kRoomStatSvgVersion_40026 = 4000026, // sync with 3.6.3.4 (room object's blocking rect x,y)
    kRoomStatSvgVersion_40028 = 4000028, // sync with 3.6.3.10 (audio panning and speed)
    kRoomStatSvgVersion_40029 = 4000029, // objects rotation pivot
    kRoomStatSvgVersion_40033 = 4000033, // sync with 3.6.3.14 (arbitrary number of room bgs)
    kRoomStatSvgVersion_40034 = 4000034, // opacity instead of transparency, full tint and light ranges
    kRoomStatSvgVersion_Current = kRoomStatSvgVersion_40034
};

struct HotspotState
{
    bool Enabled = false;
    AGS::Common::String Name;

    void ReadFromSavegame(AGS::Common::Stream *in, RoomStatSvgVersion save_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

struct WalkareaState
{
    float FaceDirectionRatio = 0.f;

    void ReadFromSavegame(AGS::Common::Stream *in, RoomStatSvgVersion save_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};

// RoomStatus contains everything about a room that could change at runtime.
class RoomStatus
{
public:
    // TODO: hide these fields under get/set methods
    int beenhere = 0;
    uint32_t numobj = 0;
    std::vector<RoomObject> obj;
    uint32_t tsdatasize = 0;
    std::vector<uint8_t> tsdata;

    HotspotState hotspot[MAX_ROOM_HOTSPOTS];
    char region_enabled[MAX_ROOM_REGIONS];
    short walkbehind_base[MAX_WALK_BEHINDS];
    float face_dir_ratio = 0.f;
    WalkareaState walkareas[MAX_WALK_AREAS];

    AGS::Common::StringIMap roomProps;
    std::vector<AGS::Common::StringIMap> objProps;
    AGS::Common::StringIMap hsProps[MAX_ROOM_HOTSPOTS];
    AGS::Common::StringIMap regProps[MAX_ROOM_REGIONS];
    AGS::Common::StringIMap waProps[MAX_WALK_AREAS];

    // A version of a save this RoomStatus was restored from.
    // This is used as a hint when merging RoomStatus with the loaded room file (upon room enter).
    // We need this for cases when an old format save is restored within an upgraded game
    // (for example, game was upgraded from 3.4.0 to 3.6.0, but player tries loading 3.4.0 save),
    // because room files are only loaded once entered, so we cannot fixup all RoomStatuses at once.
    RoomStatSvgVersion contentFormat = kRoomStatSvgVersion_Current;

    RoomStatus();
    ~RoomStatus();

    int GetBgShaderID() const { return _bgShaderID; }
    int GetBgShaderHandle() const { return _bgShaderHandle; }
    void SetBgShader(int shader_id, int shader_handle);

    void ReadFromSavegame(AGS::Common::Stream *in, RoomStatSvgVersion cmp_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;

private:
    void FreeScriptData();
    void FreeProperties();

    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _bgShaderID = 0;
    int _bgShaderHandle = 0;
};

// Retrieves a given room's runtime state. *Allocates* one if it does not exist
RoomStatus *GetRoomState(int room);
// Retrieves a existing room's runtime state; returns null if one was not allocated
RoomStatus *GetRoomStateIfExists(int room);
// Checks whether the room state exists.
bool IsRoomStateValid(int room);
// Deletes particular room's runtime state. This will make the game forget about
// what happened in that room, and make it start anew whenever player enters it again.
void ResetRoomState(int room);
// Deletes all room runtime states.
void ResetRoomStates();
