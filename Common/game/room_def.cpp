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
#include "game/room_def.h"

#include <type_traits>

#include "game/common_defines.h"
#include "game/room_version.h"
#include "gfx/gfx_def.h"
#include "util/field_table.h"

namespace AGS
{
namespace Common
{

// The structs that get a field table must stay aggregates (util/field_table.h)
static_assert(std::is_aggregate_v<RoomOptions>, "RoomOptions must stay an aggregate");
static_assert(std::is_aggregate_v<RoomEdges>, "RoomEdges must stay an aggregate");
static_assert(std::is_aggregate_v<RoomObjectBase>, "RoomObjectBase must stay an aggregate");
static_assert(std::is_aggregate_v<RoomHotspotDef>, "RoomHotspotDef must stay an aggregate");
static_assert(std::is_aggregate_v<RoomObjectDef>, "RoomObjectDef must stay an aggregate");
static_assert(std::is_aggregate_v<RoomRegionDef>, "RoomRegionDef must stay an aggregate");
static_assert(std::is_aggregate_v<RoomWalkableAreaDef>, "RoomWalkableAreaDef must stay an aggregate");
static_assert(std::is_aggregate_v<RoomWalkBehindDef>, "RoomWalkBehindDef must stay an aggregate");

namespace
{

// The roles are those of the live entity that starts as a copy of the definition (in the engine: RoomObject,
// RoomHotspot..., their RebindDefinition, RoomState's and RoomObject's save). Saved: a room state keeps the field
// between visits and a save holds it. FromDefinition: the room file gives the field again at every load
// (RebindDefinition), whatever the game did to the live entity's. Opaque: the event tables, which have no comparison
// yet (PLAN.md 5.9 step 2).
constexpr FieldRow<RoomObjectBase> kRoomObjectBaseFields[] = {
    // a restored object's id is its index (RoomState::ReadFromSavegame) until the bind gives the definition's
    Field<&RoomObjectBase::ID>("ID", FieldRole::FromDefinition),
    Field<&RoomObjectBase::ScriptName>("ScriptName", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kRoomObjectBaseFields), "RoomObjectBase: a field has no row in kRoomObjectBaseFields");

// The custom properties of a definition are taken again at every load: the values the game sets are RoomState's
// (HotspotProperties...), until PLAN.md 5.1a step 4 moves them
constexpr FieldRow<RoomHotspotDef> kRoomHotspotDefFields[] = {
    BaseFields<RoomHotspotDef, RoomObjectBase>("Base"),
    Field<&RoomHotspotDef::Name>("Name"),
    Field<&RoomHotspotDef::Properties>("Properties", FieldRole::FromDefinition),
    OpaqueField<&RoomHotspotDef::Interactions>("Interactions"),
    Field<&RoomHotspotDef::WalkTo>("WalkTo", FieldRole::FromDefinition),
    OpaqueField<&RoomHotspotDef::Events>("Events"),
};
static_assert(FieldsCoverStruct(kRoomHotspotDefFields), "RoomHotspotDef: a field has no row in kRoomHotspotDefFields");

// The save holds Sprite, Baseline and the four values of BlockingRect in 16 bits (PLAN.md 5.12)
constexpr FieldRow<RoomObjectDef> kRoomObjectDefFields[] = {
    BaseFields<RoomObjectDef, RoomObjectBase>("Base"),
    Field<&RoomObjectDef::Room>("Room", FieldRole::FromDefinition),
    Field<&RoomObjectDef::X>("X"),
    Field<&RoomObjectDef::Y>("Y"),
    Field<&RoomObjectDef::Sprite>("Sprite"),
    Field<&RoomObjectDef::BlendMode>("BlendMode"),
    Field<&RoomObjectDef::Baseline>("Baseline"),
    Field<&RoomObjectDef::Opacity>("Opacity"),
    Field<&RoomObjectDef::Flags>("Flags"),
    Field<&RoomObjectDef::Name>("Name"),
    Field<&RoomObjectDef::BlockingRect>("BlockingRect"),
    Field<&RoomObjectDef::GraphicAnchor>("GraphicAnchor"),
    Field<&RoomObjectDef::GraphicOffset>("GraphicOffset"),
    Field<&RoomObjectDef::Properties>("Properties", FieldRole::FromDefinition),
    OpaqueField<&RoomObjectDef::Interactions>("Interactions"),
    OpaqueField<&RoomObjectDef::Events>("Events"),
};
static_assert(FieldsCoverStruct(kRoomObjectDefFields), "RoomObjectDef: a field has no row in kRoomObjectDefFields");

constexpr FieldRow<RoomRegionDef> kRoomRegionDefFields[] = {
    BaseFields<RoomRegionDef, RoomObjectBase>("Base"),
    // a script's change lasts while the room is loaded: a save also holds them, for the room that is loaded when it is
    // made only, in the "Loaded Room State" component, not in the room state
    Field<&RoomRegionDef::Light>("Light", FieldRole::FromDefinition),
    Field<&RoomRegionDef::Tint>("Tint", FieldRole::FromDefinition),
    Field<&RoomRegionDef::Properties>("Properties", FieldRole::FromDefinition),
    OpaqueField<&RoomRegionDef::Interactions>("Interactions"),
    OpaqueField<&RoomRegionDef::Events>("Events"),
};
static_assert(FieldsCoverStruct(kRoomRegionDefFields), "RoomRegionDef: a field has no row in kRoomRegionDefFields");

constexpr FieldRow<RoomWalkableAreaDef> kRoomWalkableAreaDefFields[] = {
    BaseFields<RoomWalkableAreaDef, RoomObjectBase>("Base"),
    Field<&RoomWalkableAreaDef::CharacterView>("CharacterView", FieldRole::FromDefinition),
    // the scaling, as a region's light: a save also holds a script's change, for the loaded room only ("Loaded Room
    // State")
    Field<&RoomWalkableAreaDef::ScalingFar>("ScalingFar", FieldRole::FromDefinition),
    Field<&RoomWalkableAreaDef::ScalingNear>("ScalingNear", FieldRole::FromDefinition),
    Field<&RoomWalkableAreaDef::PlayerView>("PlayerView", FieldRole::FromDefinition),
    Field<&RoomWalkableAreaDef::FaceDirectionRatio>("FaceDirectionRatio"),
    Field<&RoomWalkableAreaDef::Top>("Top", FieldRole::FromDefinition),
    Field<&RoomWalkableAreaDef::Bottom>("Bottom", FieldRole::FromDefinition),
    Field<&RoomWalkableAreaDef::Properties>("Properties", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kRoomWalkableAreaDefFields),
              "RoomWalkableAreaDef: a field has no row in kRoomWalkableAreaDefFields");

constexpr FieldRow<RoomWalkBehindDef> kRoomWalkBehindDefFields[] = {
    BaseFields<RoomWalkBehindDef, RoomObjectBase>("Base"),
    // in 16 bits in the save (PLAN.md 5.12)
    Field<&RoomWalkBehindDef::Baseline>("Baseline"),
};
static_assert(FieldsCoverStruct(kRoomWalkBehindDefFields),
              "RoomWalkBehindDef: a field has no row in kRoomWalkBehindDefFields");

// The room's options and edges have no live copy: the engine reads them from the loaded room, which the room file
// gives at every load
constexpr FieldRow<RoomOptions> kRoomOptionsFields[] = {
    Field<&RoomOptions::ShowPlayerCharacter>("ShowPlayerCharacter", FieldRole::FromDefinition),
    Field<&RoomOptions::PlayerView>("PlayerView", FieldRole::FromDefinition),
    Field<&RoomOptions::FaceDirectionRatio>("FaceDirectionRatio", FieldRole::FromDefinition),
    Field<&RoomOptions::Flags>("Flags", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kRoomOptionsFields), "RoomOptions: a field has no row in kRoomOptionsFields");

constexpr FieldRow<RoomEdges> kRoomEdgesFields[] = {
    Field<&RoomEdges::Left>("Left", FieldRole::FromDefinition),
    Field<&RoomEdges::Right>("Right", FieldRole::FromDefinition),
    Field<&RoomEdges::Top>("Top", FieldRole::FromDefinition),
    Field<&RoomEdges::Bottom>("Bottom", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kRoomEdgesFields), "RoomEdges: a field has no row in kRoomEdgesFields");

} // namespace

FieldTable<RoomOptions> RoomOptions::Fields()
{
    return kRoomOptionsFields;
}

FieldTable<RoomEdges> RoomEdges::Fields()
{
    return kRoomEdgesFields;
}

FieldTable<RoomObjectBase> RoomObjectBase::Fields()
{
    return kRoomObjectBaseFields;
}

FieldTable<RoomHotspotDef> RoomHotspotDef::Fields()
{
    return kRoomHotspotDefFields;
}

FieldTable<RoomObjectDef> RoomObjectDef::Fields()
{
    return kRoomObjectDefFields;
}

FieldTable<RoomRegionDef> RoomRegionDef::Fields()
{
    return kRoomRegionDefFields;
}

FieldTable<RoomWalkableAreaDef> RoomWalkableAreaDef::Fields()
{
    return kRoomWalkableAreaDefFields;
}

FieldTable<RoomWalkBehindDef> RoomWalkBehindDef::Fields()
{
    return kRoomWalkBehindDefFields;
}

RoomBgFrame::RoomBgFrame()
{
    std::memset(Palette, 0, sizeof(Palette));
}

RoomBgFrame::RoomBgFrame(const RoomBgFrame &src)
{
    *this = src;
}

RoomBgFrame &RoomBgFrame::operator=(const RoomBgFrame &src)
{
    std::copy(src.Palette, src.Palette + 256, Palette);
    IsPaletteShared = src.IsPaletteShared;
    GraphicBuf = src.GraphicBuf;
    return *this;
}

/* static */ ScriptEventSchema RoomHotspotDef::_eventSchema = {{{"OnAnyClick", kHotspotEvent_AnyClick},
                                                                {"OnMouseMove", kHotspotEvent_MouseOver},
                                                                {"OnWalkOn", kHotspotEvent_StandOn}}};

void RoomHotspotDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[1];
    new_interactions[STD_MODE_HAND] = old_interactions[2];
    new_interactions[STD_MODE_TALK] = old_interactions[4];
    new_interactions[STD_MODE_USE] = old_interactions[3];
    new_interactions[STD_MODE_PICKUP] = old_interactions[7];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[8];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[9];
    Interactions.SetHandlers(new_interactions);
    Interactions.SetScriptModule(Events.GetScriptModule());

    Events.SetHandler(kHotspotEvent_StandOn, old_interactions[0].FunctionName);
    Events.SetHandler(kHotspotEvent_AnyClick, old_interactions[5].FunctionName);
    Events.SetHandler(kHotspotEvent_MouseOver, old_interactions[6].FunctionName);
}

/* static */ ScriptEventSchema RoomObjectDef::_eventSchema = {{
    {"OnAnyClick", kRoomObjectEvent_AnyClick},
    {"OnFrameEvent", kRoomObjectEvent_OnFrameEvent},
}};

void RoomObjectDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[0];
    new_interactions[STD_MODE_HAND] = old_interactions[1];
    new_interactions[STD_MODE_TALK] = old_interactions[2];
    new_interactions[STD_MODE_USE] = old_interactions[3];
    new_interactions[STD_MODE_PICKUP] = old_interactions[5];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[6];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[7];
    Interactions.SetHandlers(new_interactions);
    Interactions.SetScriptModule(Events.GetScriptModule());

    Events.SetHandler(kRoomObjectEvent_AnyClick, old_interactions[4].FunctionName);
}

/* static */ ScriptEventSchema RoomRegionDef::_eventSchema = {{
    {"OnStanding", kRegionEvent_Standing},
    {"OnWalksOnto", kRegionEvent_WalkOn},
    {"OnWalksOff", kRegionEvent_WalkOff},
}};

void RoomRegionDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(3);

    Events.SetHandler(kRegionEvent_Standing, old_interactions[0].FunctionName);
    Events.SetHandler(kRegionEvent_WalkOn, old_interactions[1].FunctionName);
    Events.SetHandler(kRegionEvent_WalkOff, old_interactions[2].FunctionName);
    Interactions = {};
}

RoomDef::RoomDef()
{
    InitDefaults();
}

RoomDef::RoomDef(const RoomDef &src)
{
    *this = src;
}

// Not noexcept, as before the rename: it assigns member by member, and that may allocate
// NOLINTNEXTLINE(bugprone-exception-escape)
RoomDef::RoomDef(RoomDef &&src)
{
    *this = std::move(src);
}

// Not noexcept, as before the rename: Free() clears containers and resets members whose destructors are not
// declared noexcept (the script, the event handlers)
// NOLINTNEXTLINE(bugprone-exception-escape)
RoomDef::~RoomDef()
{
    Free();
}

RoomDef &RoomDef::operator=(const RoomDef &src)
{
    if (this == &src)
        return *this;

    GameID = src.GameID;
    Name = src.Name;
    ScriptName = src.ScriptName;
    DataVersion = src.DataVersion;
    MaskResolution = src.MaskResolution;
    Width = src.Width;
    Height = src.Height;
    std::copy(src.Palette, src.Palette + 256, Palette);
    Options = src.Options;

    BackgroundBPP = src.BackgroundBPP;
    BgFrameCount = src.BgFrameCount;
    BgFrames = src.BgFrames;
    BgAnimDelay = src.BgAnimDelay;
    Edges = src.Edges;
    HotspotMaskBuf = src.HotspotMaskBuf;
    RegionMaskBuf = src.RegionMaskBuf;
    WalkAreaMaskBuf = src.WalkAreaMaskBuf;
    WalkBehindMaskBuf = src.WalkBehindMaskBuf;
    HotspotCount = src.HotspotCount;
    std::copy(src.Hotspots, src.Hotspots + MAX_ROOM_HOTSPOTS, Hotspots);
    Objects = src.Objects;
    RegionCount = src.RegionCount;
    std::copy(src.Regions, src.Regions + MAX_ROOM_REGIONS, Regions);
    WalkAreaCount = src.WalkAreaCount;
    std::copy(src.WalkAreas, src.WalkAreas + MAX_WALK_AREAS, WalkAreas);
    WalkBehindCount = src.WalkBehindCount;
    std::copy(src.WalkBehinds, src.WalkBehinds + MAX_WALK_BEHINDS, WalkBehinds);

    Properties = src.Properties;
    Interactions = src.Interactions;
    Events = src.Events;
    CompiledScript.reset(src.CompiledScript ? new ccScript(*src.CompiledScript) : nullptr);
    StringOptions = src.StringOptions;
    return *this;
}

void RoomDef::Free()
{
    BgFrames = {};
    HotspotMaskBuf = {};
    RegionMaskBuf = {};
    WalkAreaMaskBuf = {};
    WalkBehindMaskBuf = {};

    Properties.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
    {
        Hotspots[i].Properties.clear();
    }
    Objects.clear();
    for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
    {
        Regions[i].Properties.clear();
    }

    CompiledScript.reset();

    Interactions = {};
    Events.ClearHandlers();
    for (uint32_t i = 0; i < HotspotCount; ++i)
    {
        Hotspots[i].Interactions = {};
        Hotspots[i].ClearEventHandlers();
    }
    for (auto &obj : Objects)
    {
        obj.Interactions = {};
        obj.ClearEventHandlers();
    }
    for (uint32_t i = 0; i < RegionCount; ++i)
    {
        Regions[i].Interactions = {};
        Regions[i].ClearEventHandlers();
    }
}

void RoomDef::InitDefaults()
{
    DataVersion = kRoomVersion_Current;
    GameID = NO_GAME_ID_IN_ROOM_FILE;

    MaskResolution = 1;
    Width = 320;
    Height = 200;

    Options = RoomOptions();
    Edges = RoomEdges{0, 317, 40, 199};

    BgFrameCount = 1;
    HotspotCount = 0;
    RegionCount = 0;
    WalkAreaCount = 0;
    WalkBehindCount = 0;

    BgFrames.resize(BgFrameCount);

    for (size_t i = 0; i < (size_t)MAX_ROOM_HOTSPOTS; ++i)
        Hotspots[i] = RoomHotspotDef();
    for (size_t i = 0; i < (size_t)MAX_ROOM_REGIONS; ++i)
        Regions[i] = RoomRegionDef();
    for (size_t i = 0; i < (size_t)MAX_WALK_AREAS; ++i)
        WalkAreas[i] = RoomWalkableAreaDef();
    for (size_t i = 0; i < (size_t)MAX_WALK_BEHINDS; ++i)
        WalkBehinds[i] = RoomWalkBehindDef();

    BackgroundBPP = 1;
    BgAnimDelay = 5;

    memset(Palette, 0, sizeof(Palette));
}

/* static */ ScriptEventSchema RoomDef::_eventSchema = {{
    {"OnLeaveLeft", kRoomEvent_EdgeLeft},
    {"OnLeaveRight", kRoomEvent_EdgeRight},
    {"OnLeaveBottom", kRoomEvent_EdgeBottom},
    {"OnLeaveTop", kRoomEvent_EdgeTop},
    {"OnFirstTimeEnter", kRoomEvent_FirstEnter},
    {"OnLoad", kRoomEvent_BeforeFadein},
    {"OnRepExec", kRoomEvent_Repexec},
    {"OnAfterFadeIn", kRoomEvent_AfterFadein},
    {"OnLeave", kRoomEvent_BeforeFadeout},
    {"OnUnload", kRoomEvent_AfterFadeout},
}};

void RoomDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(10);

    Events.SetHandler(kRoomEvent_EdgeLeft, old_interactions[0].FunctionName);
    Events.SetHandler(kRoomEvent_EdgeRight, old_interactions[1].FunctionName);
    Events.SetHandler(kRoomEvent_EdgeBottom, old_interactions[2].FunctionName);
    Events.SetHandler(kRoomEvent_EdgeTop, old_interactions[3].FunctionName);
    Events.SetHandler(kRoomEvent_FirstEnter, old_interactions[4].FunctionName);
    Events.SetHandler(kRoomEvent_BeforeFadein, old_interactions[5].FunctionName);
    Events.SetHandler(kRoomEvent_Repexec, old_interactions[6].FunctionName);
    Events.SetHandler(kRoomEvent_AfterFadein, old_interactions[7].FunctionName);
    Events.SetHandler(kRoomEvent_BeforeFadeout, old_interactions[8].FunctionName);
    Events.SetHandler(kRoomEvent_AfterFadeout, old_interactions[9].FunctionName);
    Interactions = {};
}

bool RoomDef::HasRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint == 0;
    return false;
}

bool RoomDef::HasRegionTint(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return Regions[id].Tint != 0;
    return false;
}

int RoomDef::GetRegionLightLevel(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionLightLevel(id) ? Regions[id].Light : 0;
    return 0;
}

int RoomDef::GetRegionTintLuminance(int id) const
{
    if (id >= 0 && id < MAX_ROOM_REGIONS)
        return HasRegionTint(id) ? Regions[id].Light : 0;
    return 0;
}

void RoomRegionDef::UpgradeLegacyLight(int &light, int &tint)
{
    if (tint != 0)
    {
        // Tint saturation is stored in the highest byte
        const int saturation = GfxDef::Value100ToValue255((tint >> 24) & 0xFF);
        tint = (tint & 0x00FFFFFF) | (saturation << 24);
        light = GfxDef::Value250ToValue255(light);
    }
    else
    {
        light = GfxDef::Value100ToValue255(light);
    }
}

} // namespace Common
} // namespace AGS
