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
#include "game/world/room_object.h"

#include "debug/debug_log.h"
#include "game/common_defines.h"
#include "game/dynobj/cc_object.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/runtime_defines.h"
#include "game/script.h"
#include "game/world/object.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_state.h"
#include "game/world/view_frame.h"
#include "gfx/gfx_def.h"
#include "main/update.h"
#include "util/common.h"
#include "util/field_table.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace
{

// The roles of RoomObject's play state (the definition's part has its rows in room_def.cpp). RoomObject::_graphicSpace
// is data the live class declares, not a row here: it moves into the state with the character's (PLAN.md 5.9 step 2).
constexpr FieldRow<RoomObjectState> kRoomObjectStateFields[] = {
    // the parts a character has too (entity_state.h; their tables are in object.cpp)
    BaseFields<RoomObjectState, GraphicState>("Graphic"),
    BaseFields<RoomObjectState, ViewAnimState>("ViewAnim"),
    BaseFields<RoomObjectState, MoveState>("Move"),
    BaseFields<RoomObjectState, FrameSoundState>("FrameSound"),
    // computed by the update from the view frame, only while the object animates (update_object_scale); a save does
    // not hold them, so a restored object that no longer animates is drawn without them (a known defect)
    Field<&RoomObjectState::FrameOffsetX>("FrameOffsetX", FieldRole::Derived),
    Field<&RoomObjectState::FrameOffsetY>("FrameOffsetY", FieldRole::Derived),
    Field<&RoomObjectState::View>("View"),
};
static_assert(FieldsCoverStruct(kRoomObjectStateFields),
              "RoomObjectState: a field has no row in kRoomObjectStateFields");

} // namespace

FieldTable<RoomObjectState> RoomObjectState::Fields()
{
    return kRoomObjectStateFields;
}

void RoomObject::RebindDefinition(const RoomObjectDef &def)
{
    ID = def.ID;
    ScriptName = def.ScriptName;
    Room = def.Room;
    GetEvents() = def.GetEvents();
    Interactions = def.Interactions;
    Properties = def.Properties;
}

int RoomObject::get_width() const
{
    // FIXME: don't use the game context here, instead make sure last_width is always valid
    if (Width == 0)
        return Game::Current().SpriteInfos[Sprite].Width;
    return Width;
}

int RoomObject::get_height() const
{
    // FIXME: don't use the game context here, instead make sure last_height is always valid
    if (Height == 0)
        return Game::Current().SpriteInfos[Sprite].Height;
    return Height;
}

int RoomObject::get_baseline() const
{
    if (Baseline < 1)
        return Y;
    return Baseline;
}

void RoomObject::UpdateCyclingView(Game &game, int ref_id)
{
    if (!is_enabled())
        return;
    if (IsMoving())
    {
        MoveList &mlist = *get_movelist(MoveListID);
        // If we support smooth walk, then keep moving even across multiple stages,
        // until all the "current move" is not depleted OR until we have to turn.
        for (MoveResult mresult = kMoveResult_Continue;
             ((mresult = do_movelist_move(MoveListID, X, Y, mresult == kMoveResult_Continue)) ==
              kMoveResult_NextStage) &&
             !Math::IsFloatZero(mlist.onpart);)
        {
        }

        if (!IsMoving())
            OnStopMoving();
    }
    if (!is_animating())
        return;
    if (View == RoomObject::NoView)
        return;
    if (AnimWait > 0)
    {
        AnimWait--;
        return;
    }

    if (!CycleViewAnim(game, View, Loop, Frame, Anim))
    {
        Anim = ViewAnimateParams();
    }

    ViewFrame *vfptr = &game.Views[View].loops[Loop].frames[Frame];
    if (vfptr->pic > UINT16_MAX)
        debug_script_warn("Object's (id %d) sprite %d is outside of internal range (%d), reset to 0", ref_id,
                          vfptr->pic, UINT16_MAX);
    Sprite = Math::InRangeOrDef<uint16_t>(vfptr->pic, 0);

    if (!is_animating())
        return;

    AnimWait = static_cast<int16_t>(Anim.Delay + vfptr->speed); // 16 bits: the legacy save format
    CheckViewFrame();
}

void RoomObject::OnStopMoving()
{
    // Release movelist
    if (IsMoving())
    {
        remove_movelist(MoveListID);
        MoveListID = kNoMoveList;
    }
    // If we have a movelist reference attached, then invalidate and dec refcount
    if (MoveListHandle > 0)
    {
        release_script_movelist(MoveListHandle);
        MoveListHandle = 0;
    }
}

// Calculate wanted frame sound volume based on multiple factors
int RoomObject::GetFrameSoundVolume() const
{
    // NOTE: room objects don't have "scale volume" flag at the moment
    return ::CalcFrameSoundVolume(AnimationVolume, Anim.AudioVolume);
}

void RoomObject::CheckViewFrame()
{
    ObjectEvent objevt(kScTypeRoom,
                       RuntimeScriptValue().SetScriptObject(&Game::Current().ScObjects[ID], &ccDynamicObject));
    ::CheckViewFrame(View, Loop, Frame, GetFrameSoundVolume(), AudioPanning, AudioSpeed, objevt, &GetEvents(),
                     kRoomObjectEvent_OnFrameEvent);
}

bool RoomObject::RunFrameEvent(int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeRoom,
                       RuntimeScriptValue().SetScriptObject(&Game::Current().ScObjects[ID], &ccDynamicObject));
    return ::RunViewFrameEvent(view, loop, frame, objevt, &GetEvents(), kRoomObjectEvent_OnFrameEvent);
}

void RoomObject::ReadFromSavegame(Stream *in, int cmp_ver)
{
    X = in->ReadInt32();
    Y = in->ReadInt32();
    Opacity = in->ReadInt32();
    TintRed = in->ReadInt16();
    TintGreen = in->ReadInt16();
    TintBlue = in->ReadInt16();
    TintLevel = in->ReadInt16();
    TintLight = in->ReadInt16();
    Zoom = in->ReadInt16();
    Width = in->ReadInt16();
    Height = in->ReadInt16();
    Sprite = static_cast<uint16_t>(in->ReadInt16()); // 16 bits, unsigned: the legacy save format
    Baseline = static_cast<int16_t>(in->ReadInt16());
    View = in->ReadInt16();
    Loop = in->ReadInt16();
    Frame = in->ReadInt16();
    AnimWait = in->ReadInt16();
    MoveListID = in->ReadInt16();
    int legacy_animating = in->ReadInt8(); // legacy animating
    int anim_delay = in->ReadInt8();       // legacy anim delay (8-bit)
    Flags = in->ReadInt32();

    // the blocking rectangle's size; its offset comes later
    const int16_t blocking_width = in->ReadInt16();
    const int16_t blocking_height = in->ReadInt16();
    Name = StrUtil::ReadString(in);

    // anim vols order inverted compared to character, by mistake :(
    int cur_audio_volume = static_cast<uint8_t>(in->ReadInt8());
    AnimationVolume = static_cast<uint8_t>(in->ReadInt8());
    AudioPanning = in->ReadInt8();
    in->ReadInt8(); // reserved

    int16_t blocking_x = 0;
    int16_t blocking_y = 0;
    if (cmp_ver >= kRoomStatSvgVersion_40026)
    {
        blocking_x = in->ReadInt16();
        blocking_y = in->ReadInt16();
    }
    BlockingRect = RectWH(blocking_x, blocking_y, blocking_width, blocking_height);

    if (cmp_ver >= kRoomStatSvgVersion_40028)
    {
        AudioSpeed = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
    else
    {
        AudioSpeed = 1000; // default: normal speed
    }

    BlendMode = (AGS::Common::BlendMode)in->ReadInt32();
    // Reserved for colour options
    in->ReadInt32(); // colour flags
    // Reserved for transform options
    in->ReadInt32();              // sprite transform flags1
    in->ReadInt32();              // sprite transform flags2
    in->ReadInt32();              // transform scale x
    in->ReadInt32();              // transform scale y
    in->ReadInt32();              // transform skew x
    in->ReadInt32();              // transform skew y
    Rotation = in->ReadFloat32(); // transform rotate
    float px = in->ReadFloat32(); // sprite pivot x
    float py = in->ReadFloat32(); // sprite pivot y
    float ax = in->ReadFloat32(); // sprite anchor x
    float ay = in->ReadFloat32(); // sprite anchor y
    GraphicAnchor = Pointf(ax, ay);
    Pivot = Pointf(px, py);

    if (cmp_ver >= kRoomStatSvgVersion_40016)
    {
        MoveListHandle = in->ReadInt32();
        in->ReadInt32(); // reserve up to 4 ints
        in->ReadInt32();
        in->ReadInt32();
    }
    else
    {
        MoveListHandle = 0;
    }

    AnimFlowStyle anim_flow = kAnimFlow_None;
    AnimFlowDirection anim_dir_initial = kAnimDirForward;
    AnimFlowDirection anim_dir_current = kAnimDirForward;
    if (cmp_ver >= kRoomStatSvgVersion_40018)
    {
        ShaderID = in->ReadInt32();
        ShaderHandle = in->ReadInt32();
        // new anim fields are valid since kRoomStatSvgVersion_40020
        anim_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
        anim_dir_initial = static_cast<AnimFlowDirection>(in->ReadInt8());
        anim_dir_current = static_cast<AnimFlowDirection>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        anim_delay = in->ReadInt16();
        in->ReadInt16(); // reserved to fill int32
    }
    else
    {
        ShaderID = 0;
        ShaderHandle = 0;
    }

    if (cmp_ver >= kRoomStatSvgVersion_40026)
    {
        int offx = in->ReadInt32();
        int offy = in->ReadInt32();
        GraphicOffset = Point(offx, offy);
    }
    else
    {
        GraphicAnchor = Pointf(0.f, 1.f); // default: left-bottom
        GraphicOffset = Point();
    }

    if (cmp_ver >= kRoomStatSvgVersion_40029)
    {
        int pivot_offx = in->ReadInt32();
        int pivot_offy = in->ReadInt32();
        PivotOffset = Point(pivot_offx, pivot_offy);
    }
    else
    {
        Pivot = Pointf(0.5f, 0.5f); // default: center
        PivotOffset = Point();
    }

    // Apply animation params either from old or new save
    if (cmp_ver < kRoomStatSvgVersion_40020)
    {
        switch (legacy_animating % 10)
        {
        case LEGACY_OBJANIM_ONCE: anim_flow = kAnimFlow_Once; break;
        case LEGACY_OBJANIM_REPEAT: anim_flow = kAnimFlow_Repeat; break;
        case LEGACY_OBJANIM_ONCERESET: anim_flow = kAnimFlow_OnceAndReset; break;
        default: anim_flow = kAnimFlow_None; break;
        }
        anim_dir_initial = legacy_animating < 10 ? kAnimDirForward : kAnimDirBackward;
        anim_dir_current = anim_dir_initial;
    }

    // Upgrade legacy transparency, tint and light ranges
    if (cmp_ver < kRoomStatSvgVersion_40034)
    {
        Opacity = GfxDef::LegacyTrans255ToAlpha255(Opacity);
        if (has_explicit_tint())
        {
            TintLevel = static_cast<int16_t>(GfxDef::Value100ToValue255(TintLevel));
            TintLight = static_cast<int16_t>(GfxDef::Value250ToValue255(TintLight));
        }
        else if (has_explicit_light())
        {
            TintLight = static_cast<int16_t>(GfxDef::Value100ToValue255(TintLight));
        }
    }

    Anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, cur_audio_volume);
    SpriteWidth = Width;
    SpriteHeight = Height;
    UpdateGraphicSpace();
}

void RoomObject::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(Opacity);
    out->WriteInt16(TintRed);
    out->WriteInt16(TintGreen);
    out->WriteInt16(TintBlue);
    out->WriteInt16(TintLevel);
    out->WriteInt16(TintLight);
    out->WriteInt16(Zoom);
    out->WriteInt16(Width);
    out->WriteInt16(Height);
    out->WriteInt16(static_cast<int16_t>(Sprite));
    out->WriteInt16(static_cast<int16_t>(Baseline));
    out->WriteInt16(View);
    out->WriteInt16(Loop);
    out->WriteInt16(Frame);
    out->WriteInt16(static_cast<int16_t>(AnimWait));
    out->WriteInt16(MoveListID);
    out->WriteInt8(0); // legacy animating (8-bit field)
    out->WriteInt8(Anim.Delay);
    out->WriteInt32(Flags);
    out->WriteInt16(static_cast<int16_t>(BlockingRect.GetWidth()));
    out->WriteInt16(static_cast<int16_t>(BlockingRect.GetHeight()));
    // since 3.6.0.16
    StrUtil::WriteString(Name, out);
    // since 3.6.0.25
    out->WriteInt8(static_cast<uint8_t>(Anim.AudioVolume));
    out->WriteInt8(static_cast<uint8_t>(AnimationVolume));
    out->WriteInt8(AudioPanning);
    out->WriteInt8(0); // reserved
    // kRoomStatSvgVersion_40026
    out->WriteInt16(static_cast<int16_t>(BlockingRect.Left));
    out->WriteInt16(static_cast<int16_t>(BlockingRect.Top));
    // kRoomStatSvgVersion_40028
    out->WriteInt32(AudioSpeed);
    out->WriteInt32(0); // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
    // since 4.0.0.0
    out->WriteInt32(BlendMode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    // Reserved for transform options
    out->WriteInt32(0);                 // sprite transform flags1
    out->WriteInt32(0);                 // sprite transform flags2
    out->WriteInt32(0);                 // transform scale x
    out->WriteInt32(0);                 // transform scale y
    out->WriteInt32(0);                 // transform skew x
    out->WriteInt32(0);                 // transform skew y
    out->WriteFloat32(Rotation);        // transform rotate
    out->WriteFloat32(Pivot.X);         // sprite pivot x
    out->WriteFloat32(Pivot.Y);         // sprite pivot y
    out->WriteFloat32(GraphicAnchor.X); // sprite anchor x
    out->WriteFloat32(GraphicAnchor.Y); // sprite anchor y
    // kRoomStatSvgVersion_40016
    out->WriteInt32(MoveListHandle);
    out->WriteInt32(0); // reserve up to 4 ints
    out->WriteInt32(0);
    out->WriteInt32(0);
    // kRoomStatSvgVersion_40018
    out->WriteInt32(ShaderID);
    out->WriteInt32(ShaderHandle);
    // new anim fields are valid since kRoomStatSvgVersion_40020
    out->WriteInt8(Anim.Flow);
    out->WriteInt8(Anim.InitialDirection);
    out->WriteInt8(Anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(Anim.Delay);
    out->WriteInt16(0); // reserved to fill int32
    // kRoomStatSvgVersion_40026
    out->WriteInt32(GraphicOffset.X);
    out->WriteInt32(GraphicOffset.Y);
    // kRoomStatSvgVersion_40029
    out->WriteInt32(PivotOffset.X);
    out->WriteInt32(PivotOffset.Y);
}

void RoomObject::UpdateGraphicSpace()
{
    const GraphicFlip flip = (View != RoomObject::NoView) ? GetViewFrameFlip(View, Loop, Frame, Sprite) : kFlip_None;
    _graphicSpace =
        GraphicSpace(X, Y, GraphicAnchor,             // origin
                     Size(SpriteWidth, SpriteHeight), // source sprite size
                     Size(Width, Height),             // destination size (scaled)
                     // real graphical aabb (maybe with extra offsets)
                     RectWH(FrameOffsetX + GraphicOffset.X, FrameOffsetY + GraphicOffset.Y, SpriteWidth, SpriteHeight),
                     Rotation, Pivot, PivotOffset, kOriginRule_MirrorAround, flip);
}
