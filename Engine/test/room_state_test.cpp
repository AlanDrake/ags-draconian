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
//
// The state of a room against its definition: what BindRoomState makes of a
// new state, of a state kept from an earlier visit and of a restored one with
// another number of objects, and what a restored state takes from the room
// file again; a room object's and a room state's save round
// trip; the region and walkable-area values that the script changes, in the
// live entities and not in the definition, and the "Loaded Room State"
// component that saves them; what the hotspots, regions, walk-behinds and
// walkable areas keep between visits and take from the room file again; the
// lists a room reads with no state bound; the 16-bit limits of an
// object's sprite, baseline, blocking width and animation wait and of a
// walk-behind's baseline; the setters of an object's blocking rectangle; a later
// visit's round trip of every field of the five entities, by their field tables.
// The room is made by hand: no room file is read.
//
// Not tested: load_new_room itself (it reads the room file and the room
// script), and RoomObject::RunFrameEvent (observing which handler runs needs a
// room script).
//
//=============================================================================
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "game/customproperties.h"
#include "game/dynobj/script_objects.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/view_def.h"
#include "game/world/hotspot.h"
#include "game/world/object.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_object.h"
#include "game/world/room_state.h"
#include "game/world/view_frame.h"
#include "game/world/walk_behind.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gtest/gtest.h"
#include "util/field_table.h"
#include "util/memorystream.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace
{

// Compares two event tables or interaction lists: the script module and every handler
void ExpectSameHandlers(const ScriptEventsBase &expected, const ScriptEventsBase &actual, const char *what)
{
    SCOPED_TRACE(what);
    EXPECT_STREQ(expected.GetScriptModule().GetCStr(), actual.GetScriptModule().GetCStr());
    const std::vector<ScriptEventHandler> expected_handlers = expected.GetHandlers();
    const std::vector<ScriptEventHandler> actual_handlers = actual.GetHandlers();
    ASSERT_EQ(expected_handlers.size(), actual_handlers.size());
    for (size_t i = 0; i < expected_handlers.size(); ++i)
    {
        EXPECT_STREQ(expected_handlers[i].FunctionName.GetCStr(), actual_handlers[i].FunctionName.GetCStr())
            << "handler " << i;
        EXPECT_EQ(expected_handlers[i].Enabled, actual_handlers[i].Enabled) << "handler " << i;
        EXPECT_EQ(expected_handlers[i].Checked, actual_handlers[i].Checked) << "handler " << i;
    }
}

// Compares two room objects field by field: the definition part, then the play state. The fields that a save does
// not hold are compared only with with_unsaved: of the definition the id, the script name, the room number, the
// events, the interactions and the custom properties; of the play state the frame offsets of the view frame (set by
// the update). The graphic space is derived from the other fields by UpdateGraphicSpace, and not compared at all.
void ExpectSameObject(const RoomObject &expected, const RoomObject &actual, bool with_unsaved)
{
    // the definition
    if (with_unsaved)
    {
        EXPECT_EQ(expected.ID, actual.ID);
        EXPECT_STREQ(expected.ScriptName.GetCStr(), actual.ScriptName.GetCStr());
        EXPECT_EQ(expected.Room, actual.Room);
        ExpectSameHandlers(expected.GetEvents(), actual.GetEvents(), "events");
        ExpectSameHandlers(expected.Interactions, actual.Interactions, "interactions");
        EXPECT_EQ(expected.Properties, actual.Properties);
    }
    EXPECT_EQ(expected.X, actual.X);
    EXPECT_EQ(expected.Y, actual.Y);
    EXPECT_EQ(expected.Sprite, actual.Sprite);
    EXPECT_EQ(expected.BlendMode, actual.BlendMode);
    EXPECT_EQ(expected.Baseline, actual.Baseline);
    EXPECT_EQ(expected.Opacity, actual.Opacity);
    EXPECT_EQ(expected.Flags, actual.Flags);
    EXPECT_STREQ(expected.Name.GetCStr(), actual.Name.GetCStr());
    EXPECT_EQ(expected.BlockingRect.GetWidth(), actual.BlockingRect.GetWidth());
    EXPECT_EQ(expected.BlockingRect.GetHeight(), actual.BlockingRect.GetHeight());
    EXPECT_EQ(expected.BlockingRect.Left, actual.BlockingRect.Left);
    EXPECT_EQ(expected.BlockingRect.Top, actual.BlockingRect.Top);
    EXPECT_EQ(expected.GraphicAnchor, actual.GraphicAnchor);
    EXPECT_EQ(expected.GraphicOffset, actual.GraphicOffset);
    // the play state
    if (with_unsaved)
    {
        EXPECT_EQ(expected.FrameOffsetX, actual.FrameOffsetX);
        EXPECT_EQ(expected.FrameOffsetY, actual.FrameOffsetY);
    }
    EXPECT_EQ(expected.TintRed, actual.TintRed);
    EXPECT_EQ(expected.TintGreen, actual.TintGreen);
    EXPECT_EQ(expected.TintBlue, actual.TintBlue);
    EXPECT_EQ(expected.TintLevel, actual.TintLevel);
    EXPECT_EQ(expected.TintLight, actual.TintLight);
    EXPECT_EQ(expected.Zoom, actual.Zoom);
    EXPECT_EQ(expected.Pivot, actual.Pivot);
    EXPECT_EQ(expected.PivotOffset, actual.PivotOffset);
    EXPECT_EQ(expected.SpriteWidth, actual.SpriteWidth);
    EXPECT_EQ(expected.SpriteHeight, actual.SpriteHeight);
    EXPECT_EQ(expected.Width, actual.Width);
    EXPECT_EQ(expected.Height, actual.Height);
    EXPECT_EQ(expected.View, actual.View);
    EXPECT_EQ(expected.Loop, actual.Loop);
    EXPECT_EQ(expected.Frame, actual.Frame);
    EXPECT_EQ(expected.AnimWait, actual.AnimWait);
    EXPECT_EQ(expected.MoveListID, actual.MoveListID);
    for (const String &line : DiffFields(expected.Anim, actual.Anim))
        ADD_FAILURE() << "anim: " << line.GetCStr();
    EXPECT_EQ(expected.AnimationVolume, actual.AnimationVolume);
    EXPECT_EQ(expected.AudioPanning, actual.AudioPanning);
    EXPECT_EQ(expected.AudioSpeed, actual.AudioSpeed);
    EXPECT_EQ(expected.ShaderID, actual.ShaderID);
    EXPECT_EQ(expected.ShaderHandle, actual.ShaderHandle);
    EXPECT_EQ(expected.Rotation, actual.Rotation);
    EXPECT_EQ(expected.MoveListHandle, actual.MoveListHandle);
}

// Gives every field that RoomObject::WriteToSavegame writes a value of its own, different for each seed; the
// width and height also make the sprite size that the read derives from them
void SetSavedFields(RoomObject &obj, int seed)
{
    obj.X = 100 + seed;
    obj.Y = 200 + seed;
    obj.Opacity = 120 + seed;
    obj.TintRed = static_cast<int16_t>(11 + seed);
    obj.TintGreen = static_cast<int16_t>(12 + seed);
    obj.TintBlue = static_cast<int16_t>(13 + seed);
    obj.TintLevel = static_cast<int16_t>(14 + seed);
    obj.TintLight = static_cast<int16_t>(15 + seed);
    obj.Zoom = static_cast<int16_t>(80 + seed);
    obj.Width = static_cast<int16_t>(33 + seed);
    obj.Height = static_cast<int16_t>(44 + seed);
    obj.SpriteWidth = obj.Width;
    obj.SpriteHeight = obj.Height;
    obj.Sprite = 555 + seed;
    obj.Baseline = static_cast<int16_t>(66 + seed);
    obj.View = static_cast<uint16_t>(2 + seed);
    obj.Loop = static_cast<uint16_t>(1 + seed);
    obj.Frame = static_cast<uint16_t>(3 + seed);
    obj.AnimWait = static_cast<int16_t>(4 + seed);
    obj.MoveListID = static_cast<int16_t>(5 + seed);
    // a delay above 8 bits: the save has an 8-bit and a 16-bit copy, the read takes the second
    obj.Anim = ViewAnimateParams(kAnimFlow_RepeatAlternate, kAnimDirBackward, kAnimDirForward, 300 + seed, 60 + seed);
    obj.Flags = OBJF_ENABLED | OBJF_VISIBLE | OBJF_HASTINT | (seed << 16);
    obj.BlockingRect = RectWH(-23 - seed, 24 + seed, 21 + seed, 22 + seed);
    obj.AnimationVolume = 70 + seed;
    obj.AudioPanning = -40 + seed;
    obj.AudioSpeed = 1500 + seed;
    obj.Name = String::FromFormat("Saved thing %d", seed);
    obj.BlendMode = kBlend_Multiply;
    obj.ShaderID = 3 + seed;
    obj.ShaderHandle = 14 + seed;
    obj.Rotation = 45.5f + seed;
    obj.Pivot = Pointf(0.25f, 0.75f + seed);
    obj.GraphicAnchor = Pointf(0.5f, 0.125f + seed);
    obj.MoveListHandle = 12 + seed;
    obj.GraphicOffset = Point(-7 - seed, 9 + seed);
    obj.PivotOffset = Point(2 + seed, -3 - seed);
}

std::vector<uint8_t> WriteObject(const RoomObject &obj)
{
    std::vector<uint8_t> buf;
    Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
    obj.WriteToSavegame(&out);
    return buf;
}

std::vector<uint8_t> WriteState(const RoomState &state)
{
    std::vector<uint8_t> buf;
    Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
    state.WriteToSavegame(&out);
    return buf;
}

// Reads a room state from a buffer at the current version; returns whether the read took the whole buffer
bool ReadState(const std::vector<uint8_t> &buf, RoomState &state)
{
    Stream in(std::make_unique<VectorStream>(buf));
    state.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    return in.GetPosition() == static_cast<soff_t>(buf.size());
}

// Gives the definition fields that a save does not hold, but for the id and the room number, values made of the
// name and the index: the script name o<name><i>, an event handler, an interaction, a custom property
void SetUnsavedFields(RoomObjectDef &def, const char *name, int i)
{
    def.ScriptName = String::FromFormat("o%s%d", name, i);
    def.GetEvents().SetScriptModule(String::FromFormat("%s.asc", name));
    def.GetEvents().SetHandler(kRoomObjectEvent_OnFrameEvent, String::FromFormat("o%s%d_OnFrame", name, i));
    def.Interactions.SetScriptModule(String::FromFormat("%s.asc", name));
    def.Interactions.SetHandlers(std::vector<ScriptEventHandler>{
        ScriptEventHandler(), ScriptEventHandler(String::FromFormat("o%s%d_Look", name, i))});
    def.Properties["Weight"] = String::FromFormat("%d", i);
}

const int kObjectCount = 3;

// Compares the live hotspots, regions, walk-behinds and walkable areas of a state with their definitions, field by
// field, but for the fields that a room state keeps between visits (a hotspot's name, a walk-behind's baseline, a
// walkable area's face direction ratio) and the play state
void ExpectEntitiesTakeTheDefinition(const RoomState &state, const RoomDef &def)
{
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        SCOPED_TRACE(testing::Message() << "hotspot " << i);
        const RoomHotspotDef &expected = def.Hotspots[i];
        const RoomHotspot &actual = state.Hotspots[i];
        EXPECT_EQ(expected.ID, actual.ID);
        EXPECT_STREQ(expected.ScriptName.GetCStr(), actual.ScriptName.GetCStr());
        ExpectSameHandlers(expected.GetEvents(), actual.GetEvents(), "events");
        ExpectSameHandlers(expected.Interactions, actual.Interactions, "interactions");
        EXPECT_EQ(expected.Properties, actual.Properties);
        EXPECT_EQ(expected.WalkTo, actual.WalkTo);
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        SCOPED_TRACE(testing::Message() << "region " << i);
        const RoomRegionDef &expected = def.Regions[i];
        const RoomRegion &actual = state.Regions[i];
        EXPECT_EQ(expected.ID, actual.ID);
        EXPECT_STREQ(expected.ScriptName.GetCStr(), actual.ScriptName.GetCStr());
        ExpectSameHandlers(expected.GetEvents(), actual.GetEvents(), "events");
        ExpectSameHandlers(expected.Interactions, actual.Interactions, "interactions");
        EXPECT_EQ(expected.Properties, actual.Properties);
        EXPECT_EQ(expected.Light, actual.Light);
        EXPECT_EQ(expected.Tint, actual.Tint);
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
    {
        SCOPED_TRACE(testing::Message() << "walk-behind " << i);
        EXPECT_EQ(def.WalkBehinds[i].ID, state.WalkBehinds[i].ID);
        EXPECT_STREQ(def.WalkBehinds[i].ScriptName.GetCStr(), state.WalkBehinds[i].ScriptName.GetCStr());
    }
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        SCOPED_TRACE(testing::Message() << "walkable area " << i);
        const RoomWalkableAreaDef &expected = def.WalkAreas[i];
        const RoomWalkableArea &actual = state.WalkAreas[i];
        EXPECT_EQ(expected.ID, actual.ID);
        EXPECT_STREQ(expected.ScriptName.GetCStr(), actual.ScriptName.GetCStr());
        EXPECT_EQ(expected.CharacterView, actual.CharacterView);
        EXPECT_EQ(expected.ScalingFar, actual.ScalingFar);
        EXPECT_EQ(expected.ScalingNear, actual.ScalingNear);
        EXPECT_EQ(expected.PlayerView, actual.PlayerView);
        EXPECT_EQ(expected.Top, actual.Top);
        EXPECT_EQ(expected.Bottom, actual.Bottom);
        EXPECT_EQ(expected.Properties, actual.Properties);
    }
}

// The fields in which a live entity of a room differs from the expected one, a line each, by the tables of its two
// parts: every field but the opaque and the derived ones (computed by the update). State is void for an entity without
// a play state.
template <typename Def, typename State = void, typename Entity>
std::string LostFields(const Entity &expected, const Entity &actual)
{
    std::string lines;
    for (const String &line : DiffFields<Def>(expected, actual, false))
        lines += std::string(line.GetCStr()) + "\n";
    if constexpr (!std::is_void_v<State>)
    {
        for (const String &line : DiffFields<State>(expected, actual, false))
            lines += std::string(line.GetCStr()) + "\n";
    }
    return lines;
}

// Changes the fields of a role in both parts of a live entity of a room (AlterFields)
template <typename Def, typename State = void, typename Entity>
void AlterEntity(Entity &entity, FieldRole role)
{
    AlterFields<Def>(entity, role);
    if constexpr (!std::is_void_v<State>)
        AlterFields<State>(entity, role);
}

} // namespace

// Fixture: a room of its own as Room::Current(), with a definition made by hand (MakeDefinition) and a new state
// as room.State. The engine globals it touches, all reset in TearDown:
// - Room::Current(): the fixture's room
// - Game::DisplayedRoom: set by the tests of the "Loaded Room State" component
class LoadedRoom : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _wasRoom = Room::SetCurrent(&_room);
        _wasDisplayedRoom = Game::Current().DisplayedRoom;
        MakeDefinition(kObjectCount);
        _room.SetState(&_state);
    }

    void TearDown() override
    {
        Game::Current().DisplayedRoom = _wasDisplayedRoom;
        Room::SetCurrent(_wasRoom);
    }

    // The room's definition, which the test writes as the room file reader would (the engine reads it through
    // Room::Def(), which is const)
    RoomDef &Definition() { return _room; }

    // The room's definition: object_count objects, and in every hotspot, region, walk-behind and walkable area values
    // of its own; walkable area 2 spans the mask rows 10 to 30
    void MakeDefinition(int object_count)
    {
        Definition().Objects.clear();
        Definition().Objects.resize(object_count);
        for (int i = 0; i < object_count; ++i)
        {
            RoomObjectDef &def = Definition().Objects[i];
            def.ID = i;
            def.Room = 7;
            SetUnsavedFields(def, "Thing", i);
            def.X = 10 + i;
            def.Y = 20 + i;
            def.Sprite = 30 + i;
            def.BlendMode = kBlend_Add;
            def.Baseline = 40 + i;
            def.Opacity = 50 + i;
            def.Flags = OBJF_ENABLED | OBJF_VISIBLE | (i << 16);
            def.Name = String::FromFormat("Thing %d", i);
            def.BlockingRect = RectWH(60 + i, 70 + i, 80 + i, 90 + i);
            def.GraphicAnchor = Pointf(0.5f, 0.25f * i);
            def.GraphicOffset = Point(-3 - i, 7 + i);
        }
        for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        {
            RoomHotspotDef &def = Definition().Hotspots[i];
            def.ID = i;
            def.ScriptName = String::FromFormat("hHotspot%d", i);
            def.GetEvents().SetScriptModule("room.asc");
            def.GetEvents().SetHandler(kHotspotEvent_MouseOver, String::FromFormat("hHotspot%d_MouseOver", i));
            def.Interactions.SetScriptModule("room.asc");
            def.Interactions.SetHandlers(
                std::vector<ScriptEventHandler>{ScriptEventHandler(String::FromFormat("hHotspot%d_Look", i))});
            def.Properties["Smell"] = String::FromFormat("%d", i);
            def.Name = String::FromFormat("Hotspot %d", i);
            def.WalkTo = Point(5 + i, 6 + i);
        }
        for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        {
            RoomRegionDef &def = Definition().Regions[i];
            def.ID = i;
            def.ScriptName = String::FromFormat("region%d", i);
            def.GetEvents().SetScriptModule("room.asc");
            def.GetEvents().SetHandler(kRegionEvent_WalkOn, String::FromFormat("region%d_WalkOn", i));
            def.Properties["Wet"] = String::FromFormat("%d", i);
            // a light level in the even regions, a tint in the odd ones
            def.Light = (i % 2) == 0 ? -20 - i : 30 + i;
            def.Tint = (i % 2) == 0 ? 0 : (10 + i) | (20 << 8) | (30 << 16) | (40 << 24);
        }
        for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        {
            RoomWalkBehindDef &def = Definition().WalkBehinds[i];
            def.ID = i;
            def.ScriptName = String::FromFormat("walkbehind%d", i);
            def.Baseline = 500 + i;
        }
        for (int i = 0; i < MAX_WALK_AREAS; ++i)
        {
            RoomWalkableAreaDef &def = Definition().WalkAreas[i];
            def.ID = i;
            def.ScriptName = String::FromFormat("walkarea%d", i);
            def.CharacterView = 2 + i;
            def.ScalingFar = -10 - i;
            def.ScalingNear = 10 + i;
            def.PlayerView = 3 + i;
            def.FaceDirectionRatio = 0.5f + i;
            def.Properties["Slippery"] = String::FromFormat("%d", i);
        }
        Definition().WalkAreas[2].Top = 10;
        Definition().WalkAreas[2].Bottom = 30;
        _room.Options.FaceDirectionRatio = 1.25f;
    }

    // The object the first visit makes of object i of MakeDefinition
    static RoomObject FirstVisitObject(int i)
    {
        RoomObject obj;
        obj.ID = i;
        obj.Room = 7;
        SetUnsavedFields(obj, "Thing", i);
        obj.X = 10 + i;
        obj.Y = 20 + i;
        obj.Sprite = 30 + i;
        obj.BlendMode = kBlend_Add;
        obj.Baseline = static_cast<int16_t>(40 + i);
        obj.Opacity = 50 + i;
        obj.Flags = OBJF_ENABLED | OBJF_VISIBLE | (i << 16);
        obj.Name = String::FromFormat("Thing %d", i);
        obj.BlockingRect = RectWH(60 + i, 70 + i, 80 + i, 90 + i);
        obj.GraphicAnchor = Pointf(0.5f, 0.25f * i);
        obj.GraphicOffset = Point(-3 - i, 7 + i);
        return obj;
    }

    // A later visit's round trip of the entity that pick finds in a room state: the entity of a new state bound to the
    // definition (the first visit) gets the saved fields and the fields from the definition of its two parts altered
    // (AlterFields: what the game changed during the visit), the state is saved and restored into a second one, which
    // is bound to the definition, unaltered, as a later visit does. Returns the fields of the restored entity that
    // differ from the expected one (LostFields), which has the altered values in its saved fields and the definition's
    // in its fields from the definition: a saved field the save lost, or a field from the definition that the bind did
    // not give again.
    template <typename Entity, typename Def, typename State = void, typename Pick>
    std::string RoundTripLosses(Pick pick)
    {
        BindRoomState(_room);
        Entity &entity = pick(_state);
        Entity expected = entity;
        AlterEntity<Def, State>(expected, FieldRole::Saved);
        EXPECT_NE("", (LostFields<Def, State>(entity, expected))) << "the entity has no saved field to alter";
        AlterEntity<Def, State>(entity, FieldRole::Saved);
        AlterEntity<Def, State>(entity, FieldRole::FromDefinition);
        EXPECT_NE("", (LostFields<Def, State>(expected, entity))) << "the entity has no field from the definition";

        RoomState restored;
        EXPECT_TRUE(ReadState(WriteState(_state), restored)) << "the read did not take the whole stream";
        _room.SetState(&restored);
        EXPECT_FALSE(BindRoomState(_room));
        const std::string lost = LostFields<Def, State>(expected, pick(restored));
        _room.SetState(&_state);
        return lost;
    }

    Room _room;
    RoomState _state;
    Room *_wasRoom = nullptr;
    int _wasDisplayedRoom = 0;
};

TEST_F(LoadedRoom, FirstVisitTakesTheDefinition)
{
    EXPECT_TRUE(BindRoomState(_room));

    ASSERT_EQ(static_cast<uint32_t>(kObjectCount), _state.ObjectCount);
    ASSERT_EQ(static_cast<size_t>(kObjectCount), _state.Objects.size());
    EXPECT_EQ(static_cast<size_t>(kObjectCount), _state.ObjectProperties.size());
    EXPECT_EQ(&_state.Objects[0], _room.Objects);
    for (int i = 0; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        const RoomObject &obj = _state.Objects[i];
        ExpectSameObject(FirstVisitObject(i), obj, true);
        // the play state starts as a new object's
        EXPECT_EQ(RoomObject::NoView, obj.View);
        EXPECT_FALSE(obj.IsMoving());
        EXPECT_FALSE(obj.is_animating());
        EXPECT_EQ(100, obj.Zoom);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        EXPECT_TRUE(_state.Hotspots[i].IsEnabled) << "hotspot " << i;
        EXPECT_STREQ(String::FromFormat("Hotspot %d", i).GetCStr(), _state.Hotspots[i].Name.GetCStr());
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_EQ(1, _state.Regions[i].IsEnabled) << "region " << i;
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        EXPECT_EQ(500 + i, _state.WalkBehinds[i].Baseline) << "walk-behind " << i;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        EXPECT_EQ(0.5f + i, _state.WalkAreas[i].FaceDirectionRatio) << "walkable area " << i;
    ExpectEntitiesTakeTheDefinition(_state, _room.Def());
    // the room reads the lists of the bound state
    EXPECT_EQ(&_state.Hotspots[0], _room.Hotspots);
    EXPECT_EQ(&_state.Regions[0], _room.Regions);
    EXPECT_EQ(&_state.WalkAreas[0], _room.WalkAreas);
    EXPECT_EQ(&_state.WalkBehinds[0], _room.WalkBehinds);
    EXPECT_EQ(1.25f, _state.FaceDirectionRatio);
    EXPECT_EQ(1, _state.IsVisited);
    EXPECT_EQ(kRoomStatSvgVersion_Current, _state.ContentFormat);
}

TEST_F(LoadedRoom, SpriteOutsideTheInternalRangeBecomesZero)
{
    Definition().Objects[0].Sprite = 65535;
    Definition().Objects[1].Sprite = 65536;
    Definition().Objects[2].Sprite = -5;
    BindRoomState(_room);
    EXPECT_EQ(65535, _state.Objects[0].Sprite);
    EXPECT_EQ(0, _state.Objects[1].Sprite);
    EXPECT_EQ(0, _state.Objects[2].Sprite);
}

TEST_F(LoadedRoom, LaterVisitKeepsTheState)
{
    BindRoomState(_room);
    // what the game changed during the first visit
    ScriptObject obj1;
    obj1.id = 1;
    Object_SetPosition(&obj1, 111, 222);
    Object_SetName(&obj1, "Moved thing");
    Object_SetVisible(&obj1, false);
    ScriptHotspot hotspot4;
    hotspot4.id = 4;
    Hotspot_SetEnabled(&hotspot4, 0);
    Hotspot_SetName(&hotspot4, "Renamed hotspot");
    ScriptRegion region5;
    region5.id = 5;
    Region_SetEnabled(&region5, 0);
    ScriptWalkbehind walkbehind6;
    walkbehind6.id = 6;
    Walkbehind_SetBaseline(&walkbehind6, 333);
    const RoomObject changed = _state.Objects[1];
    // what the state must save after the next load: the same, the walkable areas' face direction ratio included,
    // although the room file below gives another one (9): a room state keeps it between visits (the user's decision
    // of 2026-11-16)
    const std::vector<uint8_t> before = WriteState(_state);

    // the room is loaded again, from a file that says otherwise for every value the state keeps
    MakeDefinition(kObjectCount);
    for (RoomObjectDef &def : Definition().Objects)
    {
        def.X += 1000;
        def.Y += 1000;
        def.Sprite += 1000;
        def.BlendMode = kBlend_Darken;
        def.Baseline += 1000;
        def.Opacity = 1;
        def.Flags = OBJF_ENABLED | OBJF_VISIBLE | OBJF_SOLID;
        def.Name = "Other";
        def.BlockingRect = RectWH(1, 2, 3, 4);
        def.GraphicAnchor = Pointf(1.f, 1.f);
        def.GraphicOffset = Point(1, 1);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        Definition().Hotspots[i].Name = "Other";
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        Definition().WalkBehinds[i].Baseline = 9;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        Definition().WalkAreas[i].FaceDirectionRatio = 9.f;
    _room.Options.FaceDirectionRatio = 9.f;
    _room.Objects = nullptr;
    EXPECT_FALSE(BindRoomState(_room));

    ASSERT_EQ(static_cast<size_t>(kObjectCount), _state.Objects.size());
    EXPECT_EQ(&_state.Objects[0], _room.Objects);
    EXPECT_TRUE(before == WriteState(_state)) << "the state changed";
    ExpectSameObject(changed, _state.Objects[1], true);
    EXPECT_EQ(111, _state.Objects[1].X);
    EXPECT_FALSE(_state.Objects[1].is_visible());
    EXPECT_FALSE(_state.Hotspots[4].IsEnabled);
    EXPECT_STREQ("Renamed hotspot", _state.Hotspots[4].Name.GetCStr());
    EXPECT_EQ(0, _state.Regions[5].IsEnabled);
    EXPECT_EQ(333, _state.WalkBehinds[6].Baseline);
    EXPECT_EQ(1, _state.IsVisited);
}

// What a room state does not keep between visits is taken from the room file again at the next load: a region's light
// level and tint and a walkable area's scaling, which the script changed during the visit (they live as long as the
// room is loaded); what it keeps stays: a hotspot's name and flag, a region's flag, a walk-behind's baseline, and a
// walkable area's face direction ratio (taken from the room file again before the user's decision of 2026-11-16)
TEST_F(LoadedRoom, LaterVisitTakesTheRegionsAndWalkableAreasFromTheDefinitionAgain)
{
    BindRoomState(_room);
    // region 2 has a light level in the room file, region 3 a tint
    ScriptRegion region2;
    region2.id = 2;
    Region_SetLightLevel(&region2, 100);
    ScriptRegion region3;
    region3.id = 3;
    Region_Tint(&region3, 1, 2, 3, 4, 5);
    ScriptRegion region5;
    region5.id = 5;
    Region_SetEnabled(&region5, 0);
    SetAreaScaling(2, 50, 150);
    ScriptWalkableArea area3;
    area3.id = 3;
    Walkarea_SetFaceDirectionRatio(&area3, 7.5f);
    ScriptHotspot hotspot4;
    hotspot4.id = 4;
    Hotspot_SetEnabled(&hotspot4, 0);
    Hotspot_SetName(&hotspot4, "Renamed hotspot");
    ScriptWalkbehind walkbehind6;
    walkbehind6.id = 6;
    Walkbehind_SetBaseline(&walkbehind6, 333);
    ASSERT_EQ(100, Region_GetLightLevel(&region2));
    ASSERT_EQ(1, Region_GetTintRed(&region3));
    ASSERT_EQ(-50, _room.WalkAreas[2].ScalingFar);
    ASSERT_EQ(7.5f, _room.WalkAreas[3].FaceDirectionRatio);

    // the room is left, and entered again
    _room.SetState(nullptr);
    _room.SetState(&_state);
    EXPECT_FALSE(BindRoomState(_room));

    ExpectEntitiesTakeTheDefinition(_state, _room.Def());
    EXPECT_EQ(-22, Region_GetLightLevel(&region2));
    EXPECT_EQ(0, Region_GetTintEnabled(&region2));
    EXPECT_EQ(1, Region_GetTintEnabled(&region3));
    EXPECT_EQ(13, Region_GetTintRed(&region3));
    EXPECT_EQ(40, Region_GetTintSaturation(&region3));
    EXPECT_EQ(33, Region_GetTintLuminance(&region3));
    EXPECT_EQ(0, Region_GetLightLevel(&region3));
    ScriptWalkableArea area2;
    area2.id = 2;
    EXPECT_EQ(-12, Walkarea_GetScalingMin(&area2));
    EXPECT_EQ(12, Walkarea_GetScalingMax(&area2));
    // kept
    EXPECT_EQ(7.5f, Walkarea_GetFaceDirectionRatio(&area3));
    EXPECT_FALSE(_state.Hotspots[4].IsEnabled);
    EXPECT_STREQ("Renamed hotspot", _state.Hotspots[4].Name.GetCStr());
    EXPECT_EQ(0, Region_GetEnabled(&region5));
    EXPECT_EQ(333, Walkbehind_GetBaseline(&walkbehind6));
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        EXPECT_EQ(i != 4, _state.Hotspots[i].IsEnabled) << "hotspot " << i;
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_EQ(i != 5 ? 1 : 0, _state.Regions[i].IsEnabled) << "region " << i;
}

// A walkable area's face direction ratio is kept by the room state between visits, as a hotspot's name is (the user's
// decision of 2026-11-16): the value a script set, and for the other areas the first visit's, whatever the room file
// says at the next load
TEST_F(LoadedRoom, LaterVisitKeepsTheWalkableAreasFaceDirectionRatio)
{
    BindRoomState(_room);
    ScriptWalkableArea area3;
    area3.id = 3;
    Walkarea_SetFaceDirectionRatio(&area3, 7.5f);

    // the room is left, and entered again, its file giving other ratios
    Definition().WalkAreas[3].FaceDirectionRatio = 9.f;
    Definition().WalkAreas[4].FaceDirectionRatio = 9.f;
    _room.SetState(nullptr);
    _room.SetState(&_state);
    EXPECT_FALSE(BindRoomState(_room));

    EXPECT_EQ(7.5f, Walkarea_GetFaceDirectionRatio(&area3));
    EXPECT_EQ(4.5f, _state.WalkAreas[4].FaceDirectionRatio);
    EXPECT_EQ(2.5f, _state.WalkAreas[2].FaceDirectionRatio);
}

// The ratio a script set comes back from a save made in the room: the state restored and bound to the room keeps it
TEST_F(LoadedRoom, RestoredStateKeepsTheWalkableAreasFaceDirectionRatio)
{
    BindRoomState(_room);
    ScriptWalkableArea area3;
    area3.id = 3;
    Walkarea_SetFaceDirectionRatio(&area3, 7.5f);
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));

    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));

    EXPECT_EQ(7.5f, Walkarea_GetFaceDirectionRatio(&area3));
    EXPECT_EQ(2.5f, restored.WalkAreas[2].FaceDirectionRatio);
}

// A state restored from a save holds of the hotspots, regions, walk-behinds and walkable areas only what a room state
// keeps; bound to the room, each takes the rest from the definition: script names, events, interactions, properties
TEST_F(LoadedRoom, RestoredStateTakesTheEntitiesDefinitions)
{
    BindRoomState(_room);
    ScriptHotspot hotspot4;
    hotspot4.id = 4;
    Hotspot_SetName(&hotspot4, "Renamed hotspot");
    ScriptWalkbehind walkbehind6;
    walkbehind6.id = 6;
    Walkbehind_SetBaseline(&walkbehind6, 333);
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        EXPECT_TRUE(restored.Hotspots[i].ScriptName.IsEmpty()) << "hotspot " << i << " as restored";
        EXPECT_FALSE(restored.Hotspots[i].GetEvents().HasHandler(kHotspotEvent_MouseOver));
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_TRUE(restored.Regions[i].ScriptName.IsEmpty()) << "region " << i << " as restored";

    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));

    ExpectEntitiesTakeTheDefinition(restored, _room.Def());
    EXPECT_STREQ("hHotspot4", restored.Hotspots[4].ScriptName.GetCStr());
    EXPECT_STREQ("hHotspot4_MouseOver",
                 restored.Hotspots[4].GetEvents().GetHandler(kHotspotEvent_MouseOver).FunctionName.GetCStr());
    EXPECT_STREQ("region7_WalkOn",
                 restored.Regions[7].GetEvents().GetHandler(kRegionEvent_WalkOn).FunctionName.GetCStr());
    // the saved values are kept
    EXPECT_STREQ("Renamed hotspot", restored.Hotspots[4].Name.GetCStr());
    EXPECT_STREQ("Hotspot 5", restored.Hotspots[5].Name.GetCStr());
    EXPECT_EQ(333, restored.WalkBehinds[6].Baseline);
    EXPECT_EQ(507, restored.WalkBehinds[7].Baseline);
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        EXPECT_TRUE(restored.Hotspots[i].IsEnabled) << "hotspot " << i;
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_EQ(1, restored.Regions[i].IsEnabled) << "region " << i;
}

// With no state bound the four lists are those of TempState, never nullptr; the placeholder room, used while no room
// is loaded, makes them copies of its definition without marking the state visited
TEST_F(LoadedRoom, UnboundRoomReadsTheListsOfTheTemporaryState)
{
    BindRoomState(_room);
    _room.SetState(nullptr);
    EXPECT_EQ(nullptr, _room.State);
    EXPECT_EQ(nullptr, _room.Objects);
    EXPECT_EQ(&_room.TempState.Hotspots[0], _room.Hotspots);
    EXPECT_EQ(&_room.TempState.Regions[0], _room.Regions);
    EXPECT_EQ(&_room.TempState.WalkAreas[0], _room.WalkAreas);
    EXPECT_EQ(&_room.TempState.WalkBehinds[0], _room.WalkBehinds);
    ScriptRegion region3;
    region3.id = 3;
    EXPECT_EQ(0, Region_GetLightLevel(&region3));
    EXPECT_EQ(0, Region_GetEnabled(&region3));

    set_room_placeholder();
    EXPECT_EQ(&_room.TempState, _room.State);
    EXPECT_EQ(nullptr, _room.Objects);
    EXPECT_EQ(&_room.TempState.Hotspots[0], _room.Hotspots);
    EXPECT_EQ(0, _room.TempState.IsVisited);
    ExpectEntitiesTakeTheDefinition(_room.TempState, _room.Def());
    // the placeholder's definition has the defaults
    EXPECT_TRUE(_room.Def().Hotspots[1].ScriptName.IsEmpty());
    EXPECT_EQ(NOT_VECTOR_SCALED, _room.WalkAreas[1].ScalingNear);
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
        EXPECT_FALSE(_room.Hotspots[i].IsEnabled) << "hotspot " << i;
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_EQ(0, _room.Regions[i].IsEnabled) << "region " << i;
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        EXPECT_EQ(0, _room.WalkBehinds[i].Baseline) << "walk-behind " << i;
}

// The script functions that change a region's light level and tint and a walkable area's scaling and face direction
// ratio write the live entity: the room's definition stays as loaded
TEST_F(LoadedRoom, ScriptFunctionsLeaveTheDefinitionUntouched)
{
    BindRoomState(_room);
    // region 2 has a light level in the room file, region 3 a tint
    ScriptRegion region2;
    region2.id = 2;
    Region_SetLightLevel(&region2, 100); // SetAreaLightLevel
    ScriptRegion region3;
    region3.id = 3;
    Region_Tint(&region3, 1, 2, 3, 4, 5); // SetRegionTint
    SetAreaScaling(2, 50, 150);
    ScriptWalkableArea area3;
    area3.id = 3;
    Walkarea_SetFaceDirectionRatio(&area3, 7.5f);
    ScriptHotspot hotspot4;
    hotspot4.id = 4;
    Hotspot_SetName(&hotspot4, "Renamed hotspot");
    SetWalkBehindBase(6, 333);

    const RoomDef &def = _room.Def();
    EXPECT_EQ(100, _room.Regions[2].Light);
    EXPECT_EQ(-22, def.Regions[2].Light);
    EXPECT_EQ(0, def.Regions[2].Tint);
    EXPECT_EQ(5, _room.Regions[3].Light);
    EXPECT_EQ(1 | (2 << 8) | (3 << 16) | (4 << 24), _room.Regions[3].Tint);
    EXPECT_EQ(33, def.Regions[3].Light);
    EXPECT_EQ(13 | (20 << 8) | (30 << 16) | (40 << 24), def.Regions[3].Tint);
    EXPECT_EQ(-50, _room.WalkAreas[2].ScalingFar);
    EXPECT_EQ(-12, def.WalkAreas[2].ScalingFar);
    EXPECT_EQ(12, def.WalkAreas[2].ScalingNear);
    EXPECT_EQ(7.5f, _room.WalkAreas[3].FaceDirectionRatio);
    EXPECT_EQ(3.5f, def.WalkAreas[3].FaceDirectionRatio);
    EXPECT_STREQ("Hotspot 4", def.Hotspots[4].Name.GetCStr());
    EXPECT_EQ(333, _room.WalkBehinds[6].Baseline);
    EXPECT_EQ(506, def.WalkBehinds[6].Baseline);
}

TEST_F(LoadedRoom, RestoredStateWithFewerObjectsIsGrown)
{
    // a state saved when the room had one object, restored into a game whose room has three
    MakeDefinition(1);
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    Object_SetPosition(&obj0, 111, 222);
    const RoomObject kept = _state.Objects[0];
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));

    MakeDefinition(kObjectCount);
    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));

    ASSERT_EQ(static_cast<uint32_t>(kObjectCount), restored.ObjectCount);
    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.Objects.size());
    EXPECT_EQ(static_cast<size_t>(kObjectCount), restored.ObjectProperties.size());
    EXPECT_EQ(&restored.Objects[0], _room.Objects);
    ExpectSameObject(kept, restored.Objects[0], false);
    for (int i = 1; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        ExpectSameObject(FirstVisitObject(i), restored.Objects[i], true);
    }
}

TEST_F(LoadedRoom, RestoredStateWithMoreObjectsIsCut)
{
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    Object_SetPosition(&obj0, 111, 222);
    const RoomObject kept = _state.Objects[0];
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));

    MakeDefinition(1);
    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));

    EXPECT_EQ(1u, restored.ObjectCount);
    ASSERT_EQ(1u, restored.Objects.size());
    EXPECT_EQ(1u, restored.ObjectProperties.size());
    EXPECT_EQ(&restored.Objects[0], _room.Objects);
    ExpectSameObject(kept, restored.Objects[0], false);
}

TEST(RoomObjectSave, EverySavedFieldIsRestored)
{
    RoomObject obj;
    SetSavedFields(obj, 0);
    const std::vector<uint8_t> saved = WriteObject(obj);

    RoomObject restored;
    Stream in(std::make_unique<VectorStream>(saved));
    restored.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    EXPECT_EQ(static_cast<soff_t>(saved.size()), in.GetPosition());
    ExpectSameObject(obj, restored, false);
    // the sprite size is derived from the scaled size
    EXPECT_EQ(33, restored.SpriteWidth);
    EXPECT_EQ(44, restored.SpriteHeight);
    EXPECT_TRUE(saved == WriteObject(restored));
}

// An object starts not moving, with MoveListID kNoMoveList from either constructor, as a character does
TEST(RoomObjectStart, StartsWithNoMoveList)
{
    EXPECT_EQ(MoveState::kNoMoveList, RoomObject().MoveListID);
    EXPECT_EQ(MoveState::kNoMoveList, RoomObject(RoomObjectDef()).MoveListID);
    EXPECT_FALSE(RoomObject().IsMoving());
    EXPECT_FALSE(RoomObject(RoomObjectDef()).IsMoving());
    EXPECT_EQ(MoveState::kNoMoveList, MoveState().MoveListID);
}

// The save holds the sprite in 16 bits, which the read takes as unsigned: a number above INT16_MAX comes back as it was
TEST(RoomObjectSave, SpriteAboveInt16RoundTrips)
{
    RoomObject obj;
    SetSavedFields(obj, 0);
    obj.Sprite = 40000;
    const std::vector<uint8_t> saved = WriteObject(obj);

    RoomObject restored;
    Stream in(std::make_unique<VectorStream>(saved));
    restored.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    EXPECT_EQ(40000, restored.Sprite);
    EXPECT_TRUE(saved == WriteObject(restored));
}

TEST(RoomStateSave, EverySavedFieldIsRestored)
{
    RoomState state;
    state.IsVisited = 1;
    state.ObjectCount = 2;
    state.Objects.resize(2);
    state.ObjectProperties.resize(2);
    SetSavedFields(state.Objects[0], 0);
    SetSavedFields(state.Objects[1], 1);
    state.ObjectProperties[1]["Weight"] = "12";
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        state.Hotspots[i].IsEnabled = (i % 2) == 0;
        state.Hotspots[i].Name = String::FromFormat("Hotspot %d", i);
    }
    state.HotspotProperties[2]["Smell"] = "sweet";
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        state.Regions[i].IsEnabled = static_cast<char>(i % 3 == 0);
    state.RegionProperties[3]["Wet"] = "1";
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        state.WalkBehinds[i].Baseline = static_cast<short>(700 + i);
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        state.WalkAreas[i].FaceDirectionRatio = 0.75f + i;
    state.WalkAreaProperties[4]["Slippery"] = "yes";
    state.Properties["Mood"] = "gloomy";
    state.FaceDirectionRatio = 2.5f;
    state.SetBgShader(5, 6);
    state.ScriptData = {1, 2, 3};
    state.ScriptDataSize = 3;

    const std::vector<uint8_t> saved = WriteState(state);
    RoomState restored;
    EXPECT_TRUE(ReadState(saved, restored)) << "the read did not take the whole stream";

    EXPECT_EQ(1, restored.IsVisited);
    ASSERT_EQ(2u, restored.ObjectCount);
    ASSERT_EQ(2u, restored.Objects.size());
    ASSERT_EQ(2u, restored.ObjectProperties.size());
    for (int i = 0; i < 2; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        ExpectSameObject(state.Objects[i], restored.Objects[i], false);
        EXPECT_EQ(state.ObjectProperties[i], restored.ObjectProperties[i]);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        EXPECT_EQ(state.Hotspots[i].IsEnabled, restored.Hotspots[i].IsEnabled) << "hotspot " << i;
        EXPECT_STREQ(state.Hotspots[i].Name.GetCStr(), restored.Hotspots[i].Name.GetCStr());
        EXPECT_EQ(state.HotspotProperties[i], restored.HotspotProperties[i]) << "hotspot " << i;
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        EXPECT_EQ(state.Regions[i].IsEnabled, restored.Regions[i].IsEnabled) << "region " << i;
        EXPECT_EQ(state.RegionProperties[i], restored.RegionProperties[i]) << "region " << i;
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        EXPECT_EQ(state.WalkBehinds[i].Baseline, restored.WalkBehinds[i].Baseline) << "walk-behind " << i;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        EXPECT_EQ(state.WalkAreas[i].FaceDirectionRatio, restored.WalkAreas[i].FaceDirectionRatio) << "area " << i;
        EXPECT_EQ(state.WalkAreaProperties[i], restored.WalkAreaProperties[i]) << "walkable area " << i;
    }
    EXPECT_EQ(state.Properties, restored.Properties);
    EXPECT_EQ(2.5f, restored.FaceDirectionRatio);
    EXPECT_EQ(5, restored.GetBgShaderID());
    EXPECT_EQ(6, restored.GetBgShaderHandle());
    EXPECT_EQ(3u, restored.ScriptDataSize);
    EXPECT_EQ(state.ScriptData, restored.ScriptData);
    EXPECT_EQ(kRoomStatSvgVersion_Current, restored.ContentFormat);
    EXPECT_TRUE(saved == WriteState(restored)) << "saving the restored state gives other bytes";
}

TEST_F(LoadedRoom, ScriptChangesRegionsAndWalkableAreas)
{
    // the live regions and walkable areas take the definition's values (the walkable area's rows) when bound
    BindRoomState(_room);
    ScriptRegion region3;
    region3.id = 3;
    Region_SetLightLevel(&region3, 100);
    EXPECT_EQ(100, Region_GetLightLevel(&region3));
    EXPECT_EQ(0, Region_GetTintEnabled(&region3));
    Region_SetLightLevel(&region3, 300);
    EXPECT_EQ(255, Region_GetLightLevel(&region3));
    Region_SetLightLevel(&region3, -300);
    EXPECT_EQ(-255, Region_GetLightLevel(&region3));

    ScriptRegion region4;
    region4.id = 4;
    Region_Tint(&region4, 10, 20, 30, 40, 50);
    EXPECT_EQ(1, Region_GetTintEnabled(&region4));
    EXPECT_EQ(10, Region_GetTintRed(&region4));
    EXPECT_EQ(20, Region_GetTintGreen(&region4));
    EXPECT_EQ(30, Region_GetTintBlue(&region4));
    EXPECT_EQ(40, Region_GetTintSaturation(&region4));
    EXPECT_EQ(50, Region_GetTintLuminance(&region4));
    EXPECT_EQ(0, Region_GetLightLevel(&region4));
    // a light level replaces the tint
    Region_SetLightLevel(&region4, 7);
    EXPECT_EQ(0, Region_GetTintEnabled(&region4));
    EXPECT_EQ(7, Region_GetLightLevel(&region4));

    ScriptWalkableArea area2;
    area2.id = 2;
    SetAreaScaling(2, 50, 150);
    // the getters return the stored values, which are the percentages minus 100
    EXPECT_EQ(-50, Walkarea_GetScalingMin(&area2));
    EXPECT_EQ(50, Walkarea_GetScalingMax(&area2));
    EXPECT_EQ(50, get_area_scaling(_room, 2, 10));
    EXPECT_EQ(100, get_area_scaling(_room, 2, 20));
    EXPECT_EQ(150, get_area_scaling(_room, 2, 30));
    EXPECT_EQ(150, get_area_scaling(_room, 2, 99));
    SetAreaScaling(2, 80, 80);
    EXPECT_EQ(-20, Walkarea_GetScalingMin(&area2));
    EXPECT_EQ(NOT_VECTOR_SCALED, Walkarea_GetScalingMax(&area2));
    EXPECT_EQ(80, get_area_scaling(_room, 2, 20));

    Walkarea_SetFaceDirectionRatio(&area2, 1.5f);
    EXPECT_EQ(1.5f, Walkarea_GetFaceDirectionRatio(&area2));
}

TEST_F(LoadedRoom, LoadedRoomStateHoldsRegionsAndWalkableAreas)
{
    Game::Current().DisplayedRoom = 5;
    BindRoomState(_room);
    ScriptRegion region3;
    region3.id = 3;
    Region_SetLightLevel(&region3, -100);
    ScriptRegion region4;
    region4.id = 4;
    Region_Tint(&region4, 10, 20, 30, 40, 50);
    SetAreaScaling(2, 50, 150);
    SetAreaScaling(5, 120, 120);

    std::vector<uint8_t> saved;
    {
        Stream out(std::make_unique<VectorStream>(saved, kStream_Write));
        HSaveError err = SavegameComponents::WriteAllCommon(&out, kSaveCmp_ThisRoom, false);
        ASSERT_TRUE(static_cast<bool>(err)) << err->FullMessage().GetCStr();
    }
    RestoredData r_data;
    {
        Stream in(std::make_unique<VectorStream>(saved));
        PreservedParams pp;
        HSaveError err = SavegameComponents::ReadAll(&in, kSvgVersion_Current, kSaveCmp_ThisRoom, pp, r_data);
        ASSERT_TRUE(static_cast<bool>(err)) << err->FullMessage().GetCStr();
    }
    EXPECT_EQ(5, r_data.Room);
    EXPECT_EQ(-100, r_data.RoomLightLevels[3]);
    EXPECT_EQ(0, r_data.RoomTintLevels[3]);
    EXPECT_EQ(50, r_data.RoomLightLevels[4]);
    EXPECT_EQ(10 | (20 << 8) | (30 << 16) | (40 << 24), r_data.RoomTintLevels[4]);
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        EXPECT_EQ(_room.Regions[i].Light, r_data.RoomLightLevels[i]) << "region " << i;
        EXPECT_EQ(_room.Regions[i].Tint, r_data.RoomTintLevels[i]) << "region " << i;
    }
    EXPECT_EQ(-50, r_data.RoomZoomLevels1[2]);
    EXPECT_EQ(50, r_data.RoomZoomLevels2[2]);
    EXPECT_EQ(20, r_data.RoomZoomLevels1[5]);
    EXPECT_EQ(NOT_VECTOR_SCALED, r_data.RoomZoomLevels2[5]);
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        EXPECT_EQ(_room.WalkAreas[i].ScalingFar, r_data.RoomZoomLevels1[i]) << "walkable area " << i;
        EXPECT_EQ(_room.WalkAreas[i].ScalingNear, r_data.RoomZoomLevels2[i]) << "walkable area " << i;
    }
}

// The values that do not fit the 16 bits of the live fields, as they are today: they wrap (int16 today),
// also through a save.
TEST_F(LoadedRoom, NarrowFieldsWrapAt16Bits)
{
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    ScriptObject obj1;
    obj1.id = 1;
    // Object.Baseline: 40000 is stored as -25536, which the getter reports as 0 (no baseline);
    // -40000 is stored as 25536
    Object_SetBaseline(&obj0, 40000);
    Object_SetBaseline(&obj1, -40000);
    EXPECT_EQ(0, Object_GetBaseline(&obj0));
    EXPECT_EQ(25536, Object_GetBaseline(&obj1));
    // Object.BlockingWidth
    Object_SetBlockingWidth(&obj0, 40000);
    Object_SetBlockingWidth(&obj1, -40000);
    EXPECT_EQ(-25536, Object_GetBlockingWidth(&obj0));
    EXPECT_EQ(25536, Object_GetBlockingWidth(&obj1));
    // the walk-behind's baseline
    SetWalkBehindBase(1, 40000);
    SetWalkBehindBase(2, -40000);
    EXPECT_EQ(-25536, GetWalkBehindBase(1));
    EXPECT_EQ(25536, GetWalkBehindBase(2));

    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));
    _room.SetState(&restored);
    BindRoomState(_room);
    EXPECT_EQ(0, Object_GetBaseline(&obj0));
    EXPECT_EQ(25536, Object_GetBaseline(&obj1));
    EXPECT_EQ(-25536, Object_GetBlockingWidth(&obj0));
    EXPECT_EQ(25536, Object_GetBlockingWidth(&obj1));
    EXPECT_EQ(-25536, GetWalkBehindBase(1));
    EXPECT_EQ(25536, GetWalkBehindBase(2));
}

// Each of the four values of the blocking rectangle is set alone: the three others read back as they were, also
// when the size is zero or negative (a size below 1 means "from the sprite")
TEST_F(LoadedRoom, BlockingRectSettersKeepTheOtherValues)
{
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    // object 0's rectangle is RectWH(60, 70, 80, 90)
    int left = 60, top = 70, width = 80, height = 90;
    auto expect_rect = [&](const char *what)
    {
        SCOPED_TRACE(what);
        EXPECT_EQ(left, Object_GetBlockingRectX(&obj0));
        EXPECT_EQ(top, Object_GetBlockingRectY(&obj0));
        EXPECT_EQ(width, Object_GetBlockingWidth(&obj0));
        EXPECT_EQ(height, Object_GetBlockingHeight(&obj0));
    };
    expect_rect("as bound");
    for (int value : {37, 0, -5})
    {
        SCOPED_TRACE(testing::Message() << "value " << value);
        Object_SetBlockingWidth(&obj0, value);
        width = value;
        expect_rect("width");
        Object_SetBlockingHeight(&obj0, value);
        height = value;
        expect_rect("height");
        Object_SetBlockingRectX(&obj0, value - 1);
        left = value - 1;
        expect_rect("x");
        Object_SetBlockingRectY(&obj0, value - 2);
        top = value - 2;
        expect_rect("y");
    }
    // a size set before the position, and a position set before the size
    Object_SetBlockingWidth(&obj0, 12);
    Object_SetBlockingRectX(&obj0, 300);
    Object_SetBlockingHeight(&obj0, -7);
    Object_SetBlockingRectY(&obj0, -300);
    Object_SetBlockingWidth(&obj0, -9);
    left = 300;
    top = -300;
    width = -9;
    height = -7;
    expect_rect("mixed");
}

// The room file's baseline is 32 bits; the object keeps 16 of them, as the save does
TEST_F(LoadedRoom, DefinitionBaselineWrapsAt16Bits)
{
    Definition().Objects[0].Baseline = 40000;
    Definition().Objects[1].Baseline = -40000;
    BindRoomState(_room);
    EXPECT_EQ(-25536, _state.Objects[0].Baseline);
    EXPECT_EQ(25536, _state.Objects[1].Baseline);
}

// A walk-behind's baseline is 32 bits in the room file; the room state keeps 16 of them, as the save does, both from
// the definition and from the script's Walkbehind.Baseline
TEST_F(LoadedRoom, WalkBehindBaselineWrapsAt16Bits)
{
    Definition().WalkBehinds[1].Baseline = 40000;
    Definition().WalkBehinds[2].Baseline = -40000;
    BindRoomState(_room);
    EXPECT_EQ(-25536, GetWalkBehindBase(1));
    EXPECT_EQ(25536, GetWalkBehindBase(2));
    ScriptWalkbehind walkbehind3;
    walkbehind3.id = 3;
    Walkbehind_SetBaseline(&walkbehind3, 40000);
    EXPECT_EQ(-25536, Walkbehind_GetBaseline(&walkbehind3));
    EXPECT_EQ(-25536, GetWalkBehindBase(3));
}

// Fixture: LoadedRoom with one view for the objects to animate in: one loop of two frames, of speeds 3 and 5. The engine
// globals it touches besides LoadedRoom's, reset in TearDown: Game::Views and game.ViewCount.
class LoadedRoomWithView : public LoadedRoom
{
protected:
    void SetUp() override
    {
        LoadedRoom::SetUp();
        Game &game = Game::Current();
        GameDef &game_def = game.DefForLoader();
        _wasNumViews = game_def.ViewCount;
        _wasViews.swap(game.Views);
        game.Views.resize(1);
        game.Views[0].Initialize(1);
        game.Views[0].Loops[0].Initialize(2);
        game.Views[0].Loops[0].Frames[0].Delay = 3;
        game.Views[0].Loops[0].Frames[1].Delay = 5;
        game_def.ViewCount = 1;
    }

    void TearDown() override
    {
        Game &game = Game::Current();
        game.Views.swap(_wasViews);
        game.DefForLoader().ViewCount = _wasNumViews;
        LoadedRoom::TearDown();
    }

    std::vector<View> _wasViews;
    int _wasNumViews = 0;
};

// An object's animation wait stays within 16 bits, as the legacy save format holds it: a delay above INT16_MAX wraps
// when the animation starts and at every frame, and the save keeps the wrapped value
TEST_F(LoadedRoomWithView, ObjectAnimWaitWrapsAt16Bits)
{
    BindRoomState(_room);
    RoomObject &obj = _state.Objects[0];
    obj.View = 0;
    ScriptObject obj0;
    obj0.id = 0;
    Object_Animate(&obj0, 0, 40000, kAnimFlow_Repeat, 0 /* not blocking */, kAnimDirForward, 0, 100);
    EXPECT_EQ(0, obj.Frame);
    EXPECT_EQ(40000 + 3 - 65536, obj.AnimWait);
    // a wait below 1 shows the next frame at the next update, which takes the next wait in 16 bits too
    obj.UpdateCyclingView(Game::Current(), 0);
    EXPECT_EQ(1, obj.Frame);
    EXPECT_EQ(40000 + 5 - 65536, obj.AnimWait);

    const std::vector<uint8_t> saved = WriteObject(obj);
    RoomObject restored;
    Stream in(std::make_unique<VectorStream>(saved));
    restored.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    EXPECT_EQ(40000 + 5 - 65536, restored.AnimWait);
}

// An object that has not moved since the room was entered has no move list; its destination is where it stands.
// A save of an earlier engine holds -1 for such an object, and it must keep working
TEST_F(LoadedRoom, ObjectDestinationOfAnObjectThatHasNotMoved)
{
    BindRoomState(_room);
    RoomObject &obj = _state.Objects[1];
    ASSERT_EQ(MoveState::kNoMoveList, obj.MoveListID);
    ScriptObject obj1;
    obj1.id = 1;
    EXPECT_EQ(obj.X, Object_GetDestinationX(&obj1));
    EXPECT_EQ(obj.Y, Object_GetDestinationY(&obj1));

    obj.MoveListID = -1; // what a save of an earlier engine restores
    EXPECT_FALSE(obj.IsMoving());
    EXPECT_EQ(obj.X, Object_GetDestinationX(&obj1));
    EXPECT_EQ(obj.Y, Object_GetDestinationY(&obj1));

    obj.MoveListID = MoveState::kNoMoveList;
    EXPECT_EQ(obj.X, Object_GetDestinationX(&obj1));
    EXPECT_EQ(obj.Y, Object_GetDestinationY(&obj1));
}

// The id is not saved; CheckViewFrame and RunFrameEvent find the object's script object and events by it
TEST_F(LoadedRoom, RestoredObjectsKeepTheirIds)
{
    BindRoomState(_room);
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));
    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));
    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.Objects.size());
    for (int i = 0; i < kObjectCount; ++i)
        EXPECT_EQ(i, restored.Objects[i].ID) << "object " << i;
}

// A state restored from a save holds no script name, event, interaction, custom property defaults nor id: a later
// visit takes them from the room file again, and each object keeps what the save gave it
TEST_F(LoadedRoom, LaterVisitTakesTheUnsavedDefinitionFieldsAgain)
{
    BindRoomState(_room);
    ScriptObject obj1;
    obj1.id = 1;
    Object_SetPosition(&obj1, 111, 222);
    Object_SetName(&obj1, "Moved thing");
    const std::vector<RoomObject> saved_objects = _state.Objects;
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));
    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.Objects.size());
    for (int i = 0; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i << " as restored");
        EXPECT_TRUE(restored.Objects[i].ScriptName.IsEmpty());
        EXPECT_FALSE(restored.Objects[i].GetEvents().HasHandler(kRoomObjectEvent_OnFrameEvent));
        EXPECT_TRUE(restored.Objects[i].Interactions.GetHandlers().empty());
        EXPECT_TRUE(restored.Objects[i].Properties.empty());
        EXPECT_EQ(-1, restored.Objects[i].Room);
        // the id is the index (RoomState::ReadFromSavegame): wiped here, so that the test sees where the bind takes
        // it from
        restored.Objects[i].ID = -1;
    }

    // the room file says other things now for what a save does not hold
    for (int i = 0; i < kObjectCount; ++i)
    {
        SetUnsavedFields(Definition().Objects[i], "Other", i);
        Definition().Objects[i].Room = 8;
    }
    _room.SetState(&restored);
    EXPECT_FALSE(BindRoomState(_room));

    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.Objects.size());
    for (int i = 0; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        const RoomObject &obj = restored.Objects[i];
        EXPECT_EQ(i, obj.ID);
        EXPECT_STREQ(String::FromFormat("oOther%d", i).GetCStr(), obj.ScriptName.GetCStr());
        EXPECT_EQ(8, obj.Room);
        ExpectSameHandlers(Definition().Objects[i].GetEvents(), obj.GetEvents(), "events");
        EXPECT_STREQ(String::FromFormat("oOther%d_OnFrame", i).GetCStr(),
                     obj.GetEvents().GetHandler(kRoomObjectEvent_OnFrameEvent).FunctionName.GetCStr());
        ExpectSameHandlers(Definition().Objects[i].Interactions, obj.Interactions, "interactions");
        EXPECT_EQ(Definition().Objects[i].Properties, obj.Properties);
        // the saved values are kept
        ExpectSameObject(saved_objects[i], obj, false);
    }
    EXPECT_EQ(111, restored.Objects[1].X);
    EXPECT_STREQ("Moved thing", restored.Objects[1].Name.GetCStr());
}

// SetAreaLightLevel rejects the region number MAX_ROOM_REGIONS with a quit, as SetRegionTint does; a quit ends
// the process and no test here observes one, so this checks the other side of the bound: the last region is
// taken by both
TEST_F(LoadedRoom, RegionFunctionsTakeTheLastRegion)
{
    ScriptRegion last;
    last.id = MAX_ROOM_REGIONS - 1;
    Region_SetLightLevel(&last, 20);
    EXPECT_EQ(20, Region_GetLightLevel(&last));
    Region_Tint(&last, 1, 2, 3, 4, 5);
    EXPECT_EQ(1, Region_GetTintRed(&last));
    EXPECT_EQ(5, Region_GetTintLuminance(&last));
}

// The walk-behind mask has the room's size, whatever the mask resolution of the other masks
TEST_F(LoadedRoom, WalkbehindAtRoomXYReadsTheWalkBehindMask)
{
    Game &game = Game::Current();
    _room.Width = 40;
    _room.Height = 20;
    _room.MaskResolution = 2;
    _room.WalkBehindMask = std::make_shared<Bitmap>(40, 20, 8);
    _room.WalkAreaMask = std::make_shared<Bitmap>(20, 10, 8);
    _room.WalkBehindMask->Clear(0);
    _room.WalkAreaMask->Clear(0);
    _room.WalkBehindMask->GetScanLineForWriting(6)[12] = 3;
    _room.WalkAreaMask->GetScanLineForWriting(3)[6] = 5;
    _room.WalkBehindMask->GetScanLineForWriting(16)[30] = 4;
    EXPECT_EQ(&game.ScWalkBehinds[3], Walkbehind_GetAtRoomXY(12, 6));
    EXPECT_EQ(&game.ScWalkBehinds[0], Walkbehind_GetAtRoomXY(13, 6));
    EXPECT_EQ(&game.ScWalkBehinds[4], Walkbehind_GetAtRoomXY(30, 16));
}

// Every field of a live room entity comes back from a later visit: a saved field with the value the game gave it, a
// field from the definition, which the game changed too, with the room file's. The roles in the field tables of the
// entity's definition and play state say which fields those are, so a field added to either struct is checked here
// without a line changing. Alter
// adds one to a number: from the fixture's values, every saved field stays within what the legacy save holds (the
// sprite, the baselines and the blocking rectangle in 16 bits, the volumes in 8).
TEST_F(LoadedRoom, EveryObjectFieldIsRestoredOrTakenAgain)
{
    auto object1 = [](RoomState &state) -> RoomObject & { return state.Objects[1]; };
    EXPECT_EQ("", (RoundTripLosses<RoomObject, RoomObjectDef, RoomObjectState>(object1)));
}

TEST_F(LoadedRoom, EveryHotspotFieldIsRestoredOrTakenAgain)
{
    auto hotspot4 = [](RoomState &state) -> RoomHotspot & { return state.Hotspots[4]; };
    EXPECT_EQ("", (RoundTripLosses<RoomHotspot, RoomHotspotDef, RoomHotspotState>(hotspot4)));
}

TEST_F(LoadedRoom, EveryRegionFieldIsRestoredOrTakenAgain)
{
    auto region3 = [](RoomState &state) -> RoomRegion & { return state.Regions[3]; };
    EXPECT_EQ("", (RoundTripLosses<RoomRegion, RoomRegionDef, RoomRegionState>(region3)));
}

TEST_F(LoadedRoom, EveryWalkableAreaFieldIsRestoredOrTakenAgain)
{
    auto area2 = [](RoomState &state) -> RoomWalkableArea & { return state.WalkAreas[2]; };
    EXPECT_EQ("", (RoundTripLosses<RoomWalkableArea, RoomWalkableAreaDef>(area2)));
}

TEST_F(LoadedRoom, EveryWalkBehindFieldIsRestoredOrTakenAgain)
{
    auto walkbehind6 = [](RoomState &state) -> RoomWalkBehind & { return state.WalkBehinds[6]; };
    EXPECT_EQ("", (RoundTripLosses<RoomWalkBehind, RoomWalkBehindDef>(walkbehind6)));
}

// The definitions' tables name each member once
TEST(RoomFieldTables, EveryRowNamesItsOwnMember)
{
    EXPECT_TRUE(FieldsAreDistinct(RoomObjectBase()));
    EXPECT_TRUE(FieldsAreDistinct(RoomHotspotDef()));
    EXPECT_TRUE(FieldsAreDistinct(RoomObjectDef()));
    EXPECT_TRUE(FieldsAreDistinct(RoomRegionDef()));
    EXPECT_TRUE(FieldsAreDistinct(RoomWalkableAreaDef()));
    EXPECT_TRUE(FieldsAreDistinct(RoomWalkBehindDef()));
    EXPECT_TRUE(FieldsAreDistinct(RoomOptions()));
    EXPECT_TRUE(FieldsAreDistinct(RoomEdges()));
    EXPECT_TRUE(FieldsAreDistinct<RoomObjectState>(RoomObject()));
    EXPECT_TRUE(FieldsAreDistinct(RoomHotspotState()));
    EXPECT_TRUE(FieldsAreDistinct(RoomRegionState()));
}
