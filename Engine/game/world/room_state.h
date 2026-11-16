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

#include "game/roomstruct.h"
#include "game/save/savegame.h"
#include "game/scripteventtable.h"
#include "game/world/room_object.h"
#include "util/string_types.h"

// The state of a room number (RoomState), kept between visits and saved for every visited room, holds the room's live
// entities: the objects (RoomObject), hotspots (RoomHotspot), regions (RoomRegion), walkable areas (RoomWalkableArea)
// and walk-behinds (RoomWalkBehind). Each is one object: its definition's fields (Common's XDef: what the room file
// says, which the entity starts as a copy of at the first visit) and, where it has one, its play state (XState: what
// exists only while the game runs). When the room is loaded again, BindRoomState rebinds each entity to the room
// file (RebindDefinition): it keeps what a save holds and takes the rest from the definition again. The definitions
// are kept as loaded in the room (Room::Def()) and are not written while the game runs.

// Forward declaration
namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS
using AGS::Common::Stream;

// RoomState runtime save format
// Before 4.0: 0 (3.5.0 pre-alpha, and "Room States" of 3.5.0 by mistake); 1 (3.5.0) new movelist format;
// 2 (3.6.0.16) hotspot and object names; 3 (3.6.0.25) object animation volume; 4 (3.6.0.41) room state's
// ContentFormat; 5 (3.6.1.9) removed movelists, save externally; 3060214 serialize current room's changes to
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

// The play state of a hotspot: what exists only while the game runs. RoomHotspot holds it with its definition.
struct RoomHotspotState
{
    // Whether the hotspot answers the player (Hotspot.Enabled)
    bool Enabled = false;
};

// A hotspot of a room, as the game changes it: its definition's fields, which start as a copy of what the room file
// says, and its play state. A room state keeps its Name and Enabled between visits, and a save holds them.
class RoomHotspot : public AGS::Common::RoomHotspotDef, public RoomHotspotState
{
public:
    RoomHotspot() = default;
    // Starts a hotspot from its definition
    explicit RoomHotspot(const AGS::Common::RoomHotspotDef &def) : RoomHotspotDef(def) {}

    // Takes again from the definition what a room state does not keep between visits: every field of the
    // definition but the name
    void RebindDefinition(const AGS::Common::RoomHotspotDef &def);
};

// The play state of a region: what exists only while the game runs. RoomRegion holds it with its definition.
struct RoomRegionState
{
    // Whether the region answers the player (Region.Enabled): 1 or 0. A char, as the save's byte is read into it whole
    char Enabled = 0;
};

// A region of a room, as the game changes it: its definition's fields, which start as a copy of what the room file
// says, and its play state. A room state keeps Enabled between visits, and a save holds it. The light level and the
// tint that the script sets last while the room is loaded: the next visit takes them from the room file again.
class RoomRegion : public AGS::Common::RoomRegionDef, public RoomRegionState
{
public:
    RoomRegion() = default;
    // Starts a region from its definition
    explicit RoomRegion(const AGS::Common::RoomRegionDef &def) : RoomRegionDef(def) {}

    // Takes again from the definition what a room state does not keep between visits: every field of the definition
    void RebindDefinition(const AGS::Common::RoomRegionDef &def);

    // Gets if the region has a light level set
    bool HasLightLevel() const { return Tint == 0; }
    // Gets if the region has a tint set
    bool HasTint() const { return Tint != 0; }
    // Gets the region's light level in -255 to 255 range value; returns 0 (default level) if the region's tint is set
    int GetLightLevel() const { return HasLightLevel() ? Light : 0; }
    // Gets the region's tint luminance in 0 to 255 range value; returns 0 if the region's light level is set
    int GetTintLuminance() const { return HasTint() ? Light : 0; }
};

// A walkable area of a room, as the game changes it: its definition's fields, which start as a copy of what the room
// file says. A room state keeps none of them between visits: the scaling and the face direction ratio that the script
// sets last while the room is loaded. Whether the area is enabled is Room::WalkableAreasOn.
class RoomWalkableArea : public AGS::Common::RoomWalkableAreaDef
{
public:
    RoomWalkableArea() = default;
    // Starts a walkable area from its definition
    explicit RoomWalkableArea(const AGS::Common::RoomWalkableAreaDef &def) : RoomWalkableAreaDef(def) {}

    // Takes again from the definition what a room state does not keep between visits: every field of the definition
    void RebindDefinition(const AGS::Common::RoomWalkableAreaDef &def);
};

// A walk-behind of a room, as the game changes it: its definition's fields, which start as a copy of what the room
// file says. A room state keeps its Baseline between visits, and a save holds it, in 16 bits (PLAN.md 5.12).
class RoomWalkBehind : public AGS::Common::RoomWalkBehindDef
{
public:
    RoomWalkBehind() = default;
    // Starts a walk-behind from its definition
    explicit RoomWalkBehind(const AGS::Common::RoomWalkBehindDef &def) : RoomWalkBehindDef(def) {}

    // Takes again from the definition what a room state does not keep between visits: every field of the
    // definition but the baseline
    void RebindDefinition(const AGS::Common::RoomWalkBehindDef &def);
};

// RoomState is the state of one room number, kept between visits and saved for every visited room.
class RoomState
{
public:
    // TODO: hide these fields under get/set methods
    int Visited = 0;          // 0 until the room has been entered once, then 1
    uint32_t ObjectCount = 0; // the number of live objects, equal to Objects.size()
    std::vector<RoomObject> Objects;
    uint32_t ScriptDataSize = 0;     // the size of ScriptData in bytes
    std::vector<uint8_t> ScriptData; // the room script's global data, kept between visits

    // The live entities of the room, by number; Room::Hotspots... point at them while the room is loaded
    RoomHotspot Hotspots[MAX_ROOM_HOTSPOTS];
    RoomRegion Regions[MAX_ROOM_REGIONS];
    RoomWalkBehind WalkBehinds[MAX_WALK_BEHINDS];
    float FaceDirectionRatio = 0.f; // the room's ratio for turning a walking character left/right or up/down
    RoomWalkableArea WalkAreas[MAX_WALK_AREAS];

    AGS::Common::StringIMap Properties;
    std::vector<AGS::Common::StringIMap> ObjectProperties;
    AGS::Common::StringIMap HotspotProperties[MAX_ROOM_HOTSPOTS];
    AGS::Common::StringIMap RegionProperties[MAX_ROOM_REGIONS];
    AGS::Common::StringIMap WalkAreaProperties[MAX_WALK_AREAS];

    // A version of a save this RoomState was restored from.
    // This is used as a hint when merging RoomState with the loaded room file (upon room enter).
    // We need this for cases when an old format save is restored within an upgraded game
    // (for example, game was upgraded from 3.4.0 to 3.6.0, but player tries loading 3.4.0 save),
    // because room files are only loaded once entered, so we cannot fixup all RoomStatuses at once.
    RoomStatSvgVersion ContentFormat = kRoomStatSvgVersion_Current;

    RoomState();
    ~RoomState();

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
RoomState *GetRoomState(int room);
// Retrieves a existing room's runtime state; returns null if one was not allocated
RoomState *GetRoomStateIfExists(int room);
// Deletes particular room's runtime state. This will make the game forget about
// what happened in that room, and make it start anew whenever player enters it again.
void ResetRoomState(int room);
// Deletes all room runtime states.
void ResetRoomStates();
