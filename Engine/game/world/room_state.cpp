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
#include "game/world/room_state.h"

#include <cstdlib> // free
#include <memory>

#include "game/customproperties.h"
#include "game/game_version.h"
#include "game/save/savegame_components.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/common.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;


void RoomHotspot::RebindDefinition(const RoomHotspotDef &def)
{
    String name = std::move(Name); // kept
    RoomHotspotDef::operator=(def);
    Name = std::move(name);
}

void RoomRegion::RebindDefinition(const RoomRegionDef &def)
{
    RoomRegionDef::operator=(def);
}

void RoomWalkableArea::RebindDefinition(const RoomWalkableAreaDef &def)
{
    const float face_direction_ratio = FaceDirectionRatio; // kept
    RoomWalkableAreaDef::operator=(def);
    FaceDirectionRatio = face_direction_ratio;
}

void RoomWalkBehind::RebindDefinition(const RoomWalkBehindDef &def)
{
    const int32_t baseline = Baseline; // kept
    RoomWalkBehindDef::operator=(def);
    Baseline = baseline;
}


RoomState::RoomState()
{
    ContentFormat = kRoomStatSvgVersion_Current; // set current to avoid fixups
    IsVisited = 0;
    ObjectCount = 0;
    ScriptDataSize = 0;
}

RoomState::~RoomState() {}

void RoomState::FreeScriptData()
{
    ScriptData.clear();
    ScriptDataSize = 0;
}

void RoomState::FreeProperties()
{
    Properties.clear();
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        HotspotProperties[i].clear();
    }
    ObjectProperties.clear();
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        RegionProperties[i].clear();
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        WalkAreaProperties[i].clear();
    }
}

void RoomState::SetBgShader(int shader_id, int shader_handle)
{
    _bgShaderID = shader_id;
    _bgShaderHandle = shader_handle;
}

void RoomState::ReadFromSavegame(Stream *in, RoomStatSvgVersion cmp_ver)
{
    FreeScriptData();
    FreeProperties();

    IsVisited = in->ReadInt8();
    ObjectCount = static_cast<uint32_t>(in->ReadInt32());

    const int num_hotspots = in->ReadInt32();
    const int num_regions = in->ReadInt32();
    const int num_walkareas = in->ReadInt32();
    const int num_walkbehinds = in->ReadInt32();

    Objects.resize(ObjectCount);
    ObjectProperties.resize(ObjectCount);
    for (uint32_t i = 0; i < ObjectCount; ++i)
    {
        Objects[i].ReadFromSavegame(in, cmp_ver);
        Objects[i].ID = static_cast<int>(i); // not saved: the object's index
        Properties::ReadValues(ObjectProperties[i], in);
    }
    // Of the hotspots, regions, walk-behinds and walkable areas only the fields a room state keeps between visits;
    // the room's definition gives the others when the room is loaded (BindRoomState)
    for (int i = 0; i < num_hotspots; ++i)
    {
        Hotspots[i].IsEnabled = in->ReadInt8() != 0;
        Hotspots[i].Name = StrUtil::ReadString(in);
        Properties::ReadValues(HotspotProperties[i], in);
    }
    for (int i = 0; i < num_regions; ++i)
    {
        Regions[i].IsEnabled = in->ReadInt8();
    }
    for (int i = 0; i < num_walkbehinds; ++i)
    {
        WalkBehinds[i].Baseline = static_cast<int16_t>(in->ReadInt32()); // 16 bits, as before
    }
    for (int i = 0; i < num_walkareas; ++i)
    {
        // reserve 3 int32 for existing older WA properties:
        // enabled (or rather flags?), scaling min & max, anything else?
        in->ReadInt32();
        in->ReadInt32();
        in->ReadInt32();
        WalkAreas[i].FaceDirectionRatio = in->ReadFloat32();
    }

    Properties::ReadValues(Properties, in);

    ScriptDataSize = static_cast<uint32_t>(in->ReadInt32());
    if (ScriptDataSize)
    {
        ScriptData.resize(ScriptDataSize);
        in->Read(ScriptData.data(), ScriptDataSize);
    }

    ContentFormat = (RoomStatSvgVersion)in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();

    FaceDirectionRatio = in->ReadFloat32();
    _bgShaderID = in->ReadInt32(); // used since kRoomStatSvgVersion_40018
    _bgShaderHandle = in->ReadInt32();
    in->ReadInt32(); // reserved

    if (cmp_ver >= kRoomStatSvgVersion_40008)
    {
        for (int i = 0; i < num_regions; ++i)
        {
            Properties::ReadValues(RegionProperties[i], in);
        }
        for (int i = 0; i < num_walkareas; ++i)
        {
            Properties::ReadValues(WalkAreaProperties[i], in);
        }
    }
}

void RoomState::WriteToSavegame(Stream *out) const
{
    out->WriteInt8(IsVisited);
    out->WriteInt32(ObjectCount);
    // -- kRoomStatSvgVersion_40003
    out->WriteInt32(MAX_ROOM_HOTSPOTS);
    out->WriteInt32(MAX_ROOM_REGIONS);
    out->WriteInt32(MAX_WALK_AREAS);
    out->WriteInt32(MAX_WALK_BEHINDS);
    // --
    for (uint32_t i = 0; i < ObjectCount; ++i)
    {
        Objects[i].WriteToSavegame(out);
        Properties::WriteValues(ObjectProperties[i], out);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        out->WriteInt8(Hotspots[i].IsEnabled);
        StrUtil::WriteString(Hotspots[i].Name, out);
        Properties::WriteValues(HotspotProperties[i], out);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        out->WriteInt8(Regions[i].IsEnabled);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        out->WriteInt32(WalkBehinds[i].Baseline);
    }
    // -- kRoomStatSvgVersion_40003
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        // reserve 3 int32 for existing older WA properties:
        // enabled (or rather flags?), scaling min & max, anything else?
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        // The live ratio, which a room state keeps between visits; earlier engines wrote here the first visit's copy of
        // the definition's, which nothing read
        out->WriteFloat32(WalkAreas[i].FaceDirectionRatio);
    }
    // --

    Properties::WriteValues(Properties, out);

    out->WriteInt32(static_cast<int32_t>(ScriptDataSize));
    if (ScriptDataSize)
        out->Write(ScriptData.data(), ScriptDataSize);

    // since 3.6.0.41
    out->WriteInt32(ContentFormat);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);

    // -- kRoomStatSvgVersion_40003
    out->WriteFloat32(FaceDirectionRatio);
    out->WriteInt32(_bgShaderID); // used since kRoomStatSvgVersion_40018
    out->WriteInt32(_bgShaderHandle);
    out->WriteInt32(0); // reserved

    // -- kRoomStatSvgVersion_40008
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        Properties::WriteValues(RegionProperties[i], out);
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        Properties::WriteValues(WalkAreaProperties[i], out);
    }
}

std::unique_ptr<RoomState> room_statuses[MAX_ROOMS];

RoomState *GetRoomState(int room)
{
    if (room < 0 || room >= MAX_ROOMS)
        return nullptr;

    if (!room_statuses[room])
    {
        // First access, allocate and initialise the status
        room_statuses[room] = std::make_unique<RoomState>();
    }
    return room_statuses[room].get();
}

RoomState *GetRoomStateIfExists(int room)
{
    if (room < 0 || room >= MAX_ROOMS)
        return nullptr;
    return room_statuses[room].get();
}

void ResetRoomState(int room)
{
    if (room < 0 || room >= MAX_ROOMS)
        return;

    RoomState *roomstat = room_statuses[room].get();
    if (roomstat)
    {
        ccRemoveObjectHandle(roomstat->GetBgShaderHandle());
        for (auto &obj : roomstat->Objects)
            ccRemoveObjectHandle(obj.ShaderHandle);
        room_statuses[room].reset();
    }
}

void ResetRoomStates()
{
    for (int i = 0; i < MAX_ROOMS; ++i)
    {
        ResetRoomState(i);
    }
}
