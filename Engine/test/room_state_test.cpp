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
// another number of objects; a room object's and a room state's save round
// trip; the region and walkable-area values that the script changes and the
// "Loaded Room State" component that saves them; the 16-bit limits of an
// object's baseline and blocking width and of a walk-behind's baseline. The
// room is made by hand: no room file is read.
//
// Not tested: load_new_room itself (it reads the room file and the room
// script), and RoomObject::RunFrameEvent (observing which handler runs needs a
// room script).
//
//=============================================================================
#include <memory>
#include <vector>

#include "game/customproperties.h"
#include "game/dynobj/script_objects.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/world/hotspot.h"
#include "game/world/object.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_object.h"
#include "game/world/room_state.h"
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

// The members of RoomObject in their order, with their types. A field added to RoomObject changes its size, and
// the build stops at the static_assert below: add the field here and to ExpectSameObject (a saved field also to
// SetSavedFields), so that no test passes without looking at it.
struct RoomObjectMembers
{
    int id;
    int x, y;
    int opacity;
    int16_t tint_r, tint_g, tint_b, tint_level, tint_light;
    int16_t zoom;
    Point spr_offset;
    Pointf spr_anchor;
    Pointf pivot;
    Point pivot_offset;
    int spr_width, spr_height;
    int frame_xoff, frame_yoff;
    int16_t width, height;
    uint16_t num;
    int16_t baseline;
    uint16_t view, loop, frame;
    int16_t wait, moving;
    ViewAnimateParams anim;
    int flags;
    int16_t blocking_width, blocking_height, blocking_x, blocking_y;
    int audio_volume;
    int audio_panning;
    int AudioSpeed;
    String name;
    BlendMode blend_mode;
    int shader_id;
    int shader_handle;
    float rotation;
    int movelist_handle;
    GraphicSpace gs;
};
static_assert(sizeof(RoomObjectMembers) == sizeof(RoomObject),
              "RoomObject has a field this test does not know: see RoomObjectMembers");

// Compares two room objects field by field. The fields that a save does not hold are compared only with
// with_unsaved: the id, the frame offsets of the view frame (set by the update) and the graphic space (derived
// from the other fields by UpdateGraphicSpace, not compared at all).
void ExpectSameObject(const RoomObject &expected, const RoomObject &actual, bool with_unsaved)
{
    if (with_unsaved)
    {
        EXPECT_EQ(expected.id, actual.id);
        EXPECT_EQ(expected.frame_xoff, actual.frame_xoff);
        EXPECT_EQ(expected.frame_yoff, actual.frame_yoff);
    }
    EXPECT_EQ(expected.x, actual.x);
    EXPECT_EQ(expected.y, actual.y);
    EXPECT_EQ(expected.opacity, actual.opacity);
    EXPECT_EQ(expected.tint_r, actual.tint_r);
    EXPECT_EQ(expected.tint_g, actual.tint_g);
    EXPECT_EQ(expected.tint_b, actual.tint_b);
    EXPECT_EQ(expected.tint_level, actual.tint_level);
    EXPECT_EQ(expected.tint_light, actual.tint_light);
    EXPECT_EQ(expected.zoom, actual.zoom);
    EXPECT_EQ(expected.spr_offset, actual.spr_offset);
    EXPECT_EQ(expected.spr_anchor, actual.spr_anchor);
    EXPECT_EQ(expected.pivot, actual.pivot);
    EXPECT_EQ(expected.pivot_offset, actual.pivot_offset);
    EXPECT_EQ(expected.spr_width, actual.spr_width);
    EXPECT_EQ(expected.spr_height, actual.spr_height);
    EXPECT_EQ(expected.width, actual.width);
    EXPECT_EQ(expected.height, actual.height);
    EXPECT_EQ(expected.num, actual.num);
    EXPECT_EQ(expected.baseline, actual.baseline);
    EXPECT_EQ(expected.view, actual.view);
    EXPECT_EQ(expected.loop, actual.loop);
    EXPECT_EQ(expected.frame, actual.frame);
    EXPECT_EQ(expected.wait, actual.wait);
    EXPECT_EQ(expected.moving, actual.moving);
    for (const String &line : DiffFields(expected.anim, actual.anim))
        ADD_FAILURE() << "anim: " << line.GetCStr();
    EXPECT_EQ(expected.flags, actual.flags);
    EXPECT_EQ(expected.blocking_width, actual.blocking_width);
    EXPECT_EQ(expected.blocking_height, actual.blocking_height);
    EXPECT_EQ(expected.blocking_x, actual.blocking_x);
    EXPECT_EQ(expected.blocking_y, actual.blocking_y);
    EXPECT_EQ(expected.audio_volume, actual.audio_volume);
    EXPECT_EQ(expected.audio_panning, actual.audio_panning);
    EXPECT_EQ(expected.AudioSpeed, actual.AudioSpeed);
    EXPECT_STREQ(expected.name.GetCStr(), actual.name.GetCStr());
    EXPECT_EQ(expected.blend_mode, actual.blend_mode);
    EXPECT_EQ(expected.shader_id, actual.shader_id);
    EXPECT_EQ(expected.shader_handle, actual.shader_handle);
    EXPECT_EQ(expected.rotation, actual.rotation);
    EXPECT_EQ(expected.movelist_handle, actual.movelist_handle);
}

// Gives every field that RoomObject::WriteToSavegame writes a value of its own, different for each seed; the
// width and height also make the sprite size that the read derives from them
void SetSavedFields(RoomObject &obj, int seed)
{
    obj.x = 100 + seed;
    obj.y = 200 + seed;
    obj.opacity = 120 + seed;
    obj.tint_r = static_cast<int16_t>(11 + seed);
    obj.tint_g = static_cast<int16_t>(12 + seed);
    obj.tint_b = static_cast<int16_t>(13 + seed);
    obj.tint_level = static_cast<int16_t>(14 + seed);
    obj.tint_light = static_cast<int16_t>(15 + seed);
    obj.zoom = static_cast<int16_t>(80 + seed);
    obj.width = static_cast<int16_t>(33 + seed);
    obj.height = static_cast<int16_t>(44 + seed);
    obj.spr_width = obj.width;
    obj.spr_height = obj.height;
    obj.num = static_cast<uint16_t>(555 + seed);
    obj.baseline = static_cast<int16_t>(66 + seed);
    obj.view = static_cast<uint16_t>(2 + seed);
    obj.loop = static_cast<uint16_t>(1 + seed);
    obj.frame = static_cast<uint16_t>(3 + seed);
    obj.wait = static_cast<int16_t>(4 + seed);
    obj.moving = static_cast<int16_t>(5 + seed);
    // a delay above 8 bits: the save has an 8-bit and a 16-bit copy, the read takes the second
    obj.anim = ViewAnimateParams(kAnimFlow_RepeatAlternate, kAnimDirBackward, kAnimDirForward, 300 + seed, 60 + seed);
    obj.flags = OBJF_ENABLED | OBJF_VISIBLE | OBJF_HASTINT | (seed << 16);
    obj.blocking_width = static_cast<int16_t>(21 + seed);
    obj.blocking_height = static_cast<int16_t>(22 + seed);
    obj.blocking_x = static_cast<int16_t>(-23 - seed);
    obj.blocking_y = static_cast<int16_t>(24 + seed);
    obj.audio_volume = 70 + seed;
    obj.audio_panning = -40 + seed;
    obj.AudioSpeed = 1500 + seed;
    obj.name = String::FromFormat("Saved thing %d", seed);
    obj.blend_mode = kBlend_Multiply;
    obj.shader_id = 3 + seed;
    obj.shader_handle = 14 + seed;
    obj.rotation = 45.5f + seed;
    obj.pivot = Pointf(0.25f, 0.75f + seed);
    obj.spr_anchor = Pointf(0.5f, 0.125f + seed);
    obj.movelist_handle = 12 + seed;
    obj.spr_offset = Point(-7 - seed, 9 + seed);
    obj.pivot_offset = Point(2 + seed, -3 - seed);
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

const int kObjectCount = 3;

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
        _room.State = &_state;
    }

    void TearDown() override
    {
        Game::Current().DisplayedRoom = _wasDisplayedRoom;
        Room::SetCurrent(_wasRoom);
    }

    // The room's definition: object_count objects, and in every hotspot, walk-behind and walkable area values of
    // its own; walkable area 2 spans the mask rows 10 to 30
    void MakeDefinition(int object_count)
    {
        _room.Objects.clear();
        _room.Objects.resize(object_count);
        for (int i = 0; i < object_count; ++i)
        {
            RoomObjectDef &def = _room.Objects[i];
            def.ID = i;
            def.ScriptName = String::FromFormat("oThing%d", i);
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
            _room.Hotspots[i].Name = String::FromFormat("Hotspot %d", i);
        for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
            _room.WalkBehinds[i].Baseline = 500 + i;
        for (int i = 0; i < MAX_WALK_AREAS; ++i)
            _room.WalkAreas[i].FaceDirectionRatio = 0.5f + i;
        _room.WalkAreas[2].Top = 10;
        _room.WalkAreas[2].Bottom = 30;
        _room.Options.FaceDirectionRatio = 1.25f;
    }

    // The object the first visit makes of object i of MakeDefinition
    static RoomObject FirstVisitObject(int i)
    {
        RoomObject obj;
        obj.id = i;
        obj.x = 10 + i;
        obj.y = 20 + i;
        obj.num = static_cast<uint16_t>(30 + i);
        obj.blend_mode = kBlend_Add;
        obj.baseline = static_cast<int16_t>(40 + i);
        obj.opacity = 50 + i;
        obj.flags = OBJF_ENABLED | OBJF_VISIBLE | (i << 16);
        obj.name = String::FromFormat("Thing %d", i);
        obj.blocking_x = static_cast<int16_t>(60 + i);
        obj.blocking_y = static_cast<int16_t>(70 + i);
        obj.blocking_width = static_cast<int16_t>(80 + i);
        obj.blocking_height = static_cast<int16_t>(90 + i);
        obj.spr_anchor = Pointf(0.5f, 0.25f * i);
        obj.spr_offset = Point(-3 - i, 7 + i);
        return obj;
    }

    Room _room;
    RoomState _state;
    Room *_wasRoom = nullptr;
    int _wasDisplayedRoom = 0;
};

TEST_F(LoadedRoom, FirstVisitTakesTheDefinition)
{
    EXPECT_TRUE(BindRoomState(_room));

    ASSERT_EQ(static_cast<uint32_t>(kObjectCount), _state.numobj);
    ASSERT_EQ(static_cast<size_t>(kObjectCount), _state.obj.size());
    EXPECT_EQ(static_cast<size_t>(kObjectCount), _state.objProps.size());
    EXPECT_EQ(&_state.obj[0], _room.Objs);
    for (int i = 0; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        const RoomObject &obj = _state.obj[i];
        ExpectSameObject(FirstVisitObject(i), obj, true);
        // the play state starts as a new object's
        EXPECT_EQ(RoomObject::NoView, obj.view);
        EXPECT_FALSE(obj.is_moving());
        EXPECT_FALSE(obj.is_animating());
        EXPECT_EQ(100, obj.zoom);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        EXPECT_TRUE(_state.hotspot[i].Enabled) << "hotspot " << i;
        EXPECT_STREQ(String::FromFormat("Hotspot %d", i).GetCStr(), _state.hotspot[i].Name.GetCStr());
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        EXPECT_EQ(1, _state.region_enabled[i]) << "region " << i;
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        EXPECT_EQ(500 + i, _state.walkbehind_base[i]) << "walk-behind " << i;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        EXPECT_EQ(0.5f + i, _state.walkareas[i].FaceDirectionRatio) << "walkable area " << i;
    EXPECT_EQ(1.25f, _state.face_dir_ratio);
    EXPECT_EQ(1, _state.beenhere);
    EXPECT_EQ(kRoomStatSvgVersion_Current, _state.contentFormat);
}

TEST_F(LoadedRoom, SpriteOutsideTheInternalRangeBecomesZero)
{
    _room.Objects[0].Sprite = 65535;
    _room.Objects[1].Sprite = 65536;
    _room.Objects[2].Sprite = -5;
    BindRoomState(_room);
    EXPECT_EQ(65535, _state.obj[0].num);
    EXPECT_EQ(0, _state.obj[1].num);
    EXPECT_EQ(0, _state.obj[2].num);
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
    const RoomObject changed = _state.obj[1];
    const std::vector<uint8_t> before = WriteState(_state);

    // the room is loaded again, from a file that says otherwise for every value the state keeps
    MakeDefinition(kObjectCount);
    for (RoomObjectDef &def : _room.Objects)
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
        _room.Hotspots[i].Name = "Other";
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        _room.WalkBehinds[i].Baseline = 9;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        _room.WalkAreas[i].FaceDirectionRatio = 9.f;
    _room.Options.FaceDirectionRatio = 9.f;
    _room.Objs = nullptr;
    EXPECT_FALSE(BindRoomState(_room));

    ASSERT_EQ(static_cast<size_t>(kObjectCount), _state.obj.size());
    EXPECT_EQ(&_state.obj[0], _room.Objs);
    EXPECT_TRUE(before == WriteState(_state)) << "the state changed";
    ExpectSameObject(changed, _state.obj[1], true);
    EXPECT_EQ(111, _state.obj[1].x);
    EXPECT_FALSE(_state.obj[1].is_visible());
    EXPECT_FALSE(_state.hotspot[4].Enabled);
    EXPECT_STREQ("Renamed hotspot", _state.hotspot[4].Name.GetCStr());
    EXPECT_EQ(0, _state.region_enabled[5]);
    EXPECT_EQ(333, _state.walkbehind_base[6]);
    EXPECT_EQ(1, _state.beenhere);
}

TEST_F(LoadedRoom, RestoredStateWithFewerObjectsIsGrown)
{
    // a state saved when the room had one object, restored into a game whose room has three
    MakeDefinition(1);
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    Object_SetPosition(&obj0, 111, 222);
    const RoomObject kept = _state.obj[0];
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));

    MakeDefinition(kObjectCount);
    _room.State = &restored;
    EXPECT_FALSE(BindRoomState(_room));

    ASSERT_EQ(static_cast<uint32_t>(kObjectCount), restored.numobj);
    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.obj.size());
    EXPECT_EQ(static_cast<size_t>(kObjectCount), restored.objProps.size());
    EXPECT_EQ(&restored.obj[0], _room.Objs);
    ExpectSameObject(kept, restored.obj[0], false);
    for (int i = 1; i < kObjectCount; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        ExpectSameObject(FirstVisitObject(i), restored.obj[i], true);
    }
}

TEST_F(LoadedRoom, RestoredStateWithMoreObjectsIsCut)
{
    BindRoomState(_room);
    ScriptObject obj0;
    obj0.id = 0;
    Object_SetPosition(&obj0, 111, 222);
    const RoomObject kept = _state.obj[0];
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));

    MakeDefinition(1);
    _room.State = &restored;
    EXPECT_FALSE(BindRoomState(_room));

    EXPECT_EQ(1u, restored.numobj);
    ASSERT_EQ(1u, restored.obj.size());
    EXPECT_EQ(1u, restored.objProps.size());
    EXPECT_EQ(&restored.obj[0], _room.Objs);
    ExpectSameObject(kept, restored.obj[0], false);
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
    EXPECT_EQ(33, restored.spr_width);
    EXPECT_EQ(44, restored.spr_height);
    EXPECT_TRUE(saved == WriteObject(restored));
}

TEST(RoomStateSave, EverySavedFieldIsRestored)
{
    RoomState state;
    state.beenhere = 1;
    state.numobj = 2;
    state.obj.resize(2);
    state.objProps.resize(2);
    SetSavedFields(state.obj[0], 0);
    SetSavedFields(state.obj[1], 1);
    state.objProps[1]["Weight"] = "12";
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        state.hotspot[i].Enabled = (i % 2) == 0;
        state.hotspot[i].Name = String::FromFormat("Hotspot %d", i);
    }
    state.hsProps[2]["Smell"] = "sweet";
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
        state.region_enabled[i] = static_cast<char>(i % 3 == 0);
    state.regProps[3]["Wet"] = "1";
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        state.walkbehind_base[i] = static_cast<short>(700 + i);
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
        state.walkareas[i].FaceDirectionRatio = 0.75f + i;
    state.waProps[4]["Slippery"] = "yes";
    state.roomProps["Mood"] = "gloomy";
    state.face_dir_ratio = 2.5f;
    state.SetBgShader(5, 6);
    state.tsdata = {1, 2, 3};
    state.tsdatasize = 3;

    const std::vector<uint8_t> saved = WriteState(state);
    RoomState restored;
    EXPECT_TRUE(ReadState(saved, restored)) << "the read did not take the whole stream";

    EXPECT_EQ(1, restored.beenhere);
    ASSERT_EQ(2u, restored.numobj);
    ASSERT_EQ(2u, restored.obj.size());
    ASSERT_EQ(2u, restored.objProps.size());
    for (int i = 0; i < 2; ++i)
    {
        SCOPED_TRACE(testing::Message() << "object " << i);
        ExpectSameObject(state.obj[i], restored.obj[i], false);
        EXPECT_EQ(state.objProps[i], restored.objProps[i]);
    }
    for (int i = 0; i < MAX_ROOM_HOTSPOTS; ++i)
    {
        EXPECT_EQ(state.hotspot[i].Enabled, restored.hotspot[i].Enabled) << "hotspot " << i;
        EXPECT_STREQ(state.hotspot[i].Name.GetCStr(), restored.hotspot[i].Name.GetCStr());
        EXPECT_EQ(state.hsProps[i], restored.hsProps[i]) << "hotspot " << i;
    }
    for (int i = 0; i < MAX_ROOM_REGIONS; ++i)
    {
        EXPECT_EQ(state.region_enabled[i], restored.region_enabled[i]) << "region " << i;
        EXPECT_EQ(state.regProps[i], restored.regProps[i]) << "region " << i;
    }
    for (int i = 0; i < MAX_WALK_BEHINDS; ++i)
        EXPECT_EQ(state.walkbehind_base[i], restored.walkbehind_base[i]) << "walk-behind " << i;
    for (int i = 0; i < MAX_WALK_AREAS; ++i)
    {
        EXPECT_EQ(state.walkareas[i].FaceDirectionRatio, restored.walkareas[i].FaceDirectionRatio) << "area " << i;
        EXPECT_EQ(state.waProps[i], restored.waProps[i]) << "walkable area " << i;
    }
    EXPECT_EQ(state.roomProps, restored.roomProps);
    EXPECT_EQ(2.5f, restored.face_dir_ratio);
    EXPECT_EQ(5, restored.GetBgShaderID());
    EXPECT_EQ(6, restored.GetBgShaderHandle());
    EXPECT_EQ(3u, restored.tsdatasize);
    EXPECT_EQ(state.tsdata, restored.tsdata);
    EXPECT_EQ(kRoomStatSvgVersion_Current, restored.contentFormat);
    EXPECT_TRUE(saved == WriteState(restored)) << "saving the restored state gives other bytes";
}

TEST_F(LoadedRoom, ScriptChangesRegionsAndWalkableAreas)
{
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
    _room.State = &restored;
    BindRoomState(_room);
    EXPECT_EQ(0, Object_GetBaseline(&obj0));
    EXPECT_EQ(25536, Object_GetBaseline(&obj1));
    EXPECT_EQ(-25536, Object_GetBlockingWidth(&obj0));
    EXPECT_EQ(25536, Object_GetBlockingWidth(&obj1));
    EXPECT_EQ(-25536, GetWalkBehindBase(1));
    EXPECT_EQ(25536, GetWalkBehindBase(2));
}

// The id is not saved; CheckViewFrame and RunFrameEvent find the object's script object and events by it
TEST_F(LoadedRoom, RestoredObjectsKeepTheirIds)
{
    BindRoomState(_room);
    RoomState restored;
    ASSERT_TRUE(ReadState(WriteState(_state), restored));
    _room.State = &restored;
    EXPECT_FALSE(BindRoomState(_room));
    ASSERT_EQ(static_cast<size_t>(kObjectCount), restored.obj.size());
    for (int i = 0; i < kObjectCount; ++i)
        EXPECT_EQ(i, restored.obj[i].id) << "object " << i;
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
