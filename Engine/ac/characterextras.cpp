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
#include "ac/characterextras.h"

#include "ac/characterinfo.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/gamesetupstruct.h"
#include "ac/viewframe.h"
#include "script/script.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

extern CCCharacter ccDynamicCharacter;
extern std::vector<ViewStruct> views;
extern GameSetupStruct game;

void CharacterExtras::UpdateGraphicSpace(const CharacterInfo *ch)
{
    // TODO: zoom_offs is not applied to eff_offset here, while draw.cpp scales the offsets by it on its own.
    // To apply it here, either:
    // * AABB should *divide* graphical offset by zoom, which may only work if AABB is made
    // of floats (otherwise value precision will be lost in integer division).
    // * OR we might require ADDITIONAL parameter into GS, which is an offset applied UNSCALED.
    _gs = GraphicSpace(ch->x, ch->y, eff_anchor,    // origin
                       Size(spr_width, spr_height), // source sprite size
                       Size(width, height),         // destination size (scaled)
                       // real graphical aabb (maybe with extra offsets)
                       RectWH((eff_offset.X + frame_xoff), (-ch->z + eff_offset.Y + frame_yoff), spr_width, spr_height),
                       rotation, pivot, pivot_offset);
}

void CharacterExtras::SetTurning(bool on, bool ccw, int turn_steps)
{
    if (on && (turn_steps > 0))
        flags = (flags & ~kCharf_TurningMask) | kCharf_Turning | (ccw ? kCharf_TurningCCW : 0);
    else
        flags = (flags & ~kCharf_TurningMask);
    turns = turn_steps;
}

void CharacterExtras::DecrementTurning()
{
    turns--;
    if (turns <= 0)
    {
        SetTurning(false, false, 0);
    }
}

void CharacterExtras::SetAnimating(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int anim_volume)
{
    anim = ViewAnimateParams(flow, dir, delay, anim_volume);
}

void CharacterExtras::ResetAnimating()
{
    anim = ViewAnimateParams();
}

int CharacterExtras::GetEffectiveY(const CharacterInfo *ch) const
{
    return ch->y - (ch->z * zoom_offs) / 100;
}

int CharacterExtras::GetFrameSoundVolume(const CharacterInfo *ch) const
{
    return ::CalcFrameSoundVolume(audio_volume, anim.AudioVolume, (ch->flags & CHF_SCALEVOLUME) ? zoom : 100);
}

void CharacterExtras::SetLockedView(CharacterInfo *ch, int view, int loop, int frame, const Pointf &anchor,
                                    const Point &off)
{
    ch->view = view;
    ch->loop = loop;
    ch->frame = frame;
    ch->wait = 0;
    ch->flags |= CHF_FIXVIEW;
    view_anchor = anchor;
    view_offset = off;
    UpdateEffectiveValues(ch);
}

void CharacterExtras::SetUnlockedView(CharacterInfo *ch)
{
    ch->flags &= ~CHF_FIXVIEW;
    ch->view = ch->defview;
    ch->frame = 0;
    view_anchor = CharacterInfo::GetDefaultSpriteAnchor();
    view_offset = Point();
    UpdateEffectiveValues(ch);
}

void CharacterExtras::CheckViewFrame(CharacterInfo *ch)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(ch, &ccDynamicCharacter));
    ::CheckViewFrame(ch->view, ch->loop, ch->frame, GetFrameSoundVolume(ch), audio_panning, audio_speed, objevt,
                     &ch->GetEvents(), kCharacterEvent_OnFrameEvent);
}

bool CharacterExtras::RunFrameEvent(CharacterInfo *ch, int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(ch, &ccDynamicCharacter));
    return ::RunViewFrameEvent(view, loop, frame, objevt, &ch->GetEvents(), kCharacterEvent_OnFrameEvent);
}

void CharacterExtras::SetFollowing(CharacterInfo *ch, int follow_who, int distance, int eagerness, bool sort_behind)
{
    if (follow_who < 0)
    {
        following = -1;
        follow_dist = 0;
        follow_eagerness = 0;
        ch->set_following_sortbehind(false);
    }
    else
    {
        following = follow_who;
        follow_dist = distance;
        follow_eagerness = (distance == FOLLOW_ALWAYSONTOP) ? 0 : eagerness;
        ch->set_following_sortbehind(sort_behind && (distance == FOLLOW_ALWAYSONTOP));
    }
}

void CharacterExtras::UpdateEffectiveValues(const CharacterInfo *ch)
{
    eff_offset = ch->is_view_locked() ? view_offset : spr_offset;
    eff_anchor = ch->is_view_locked() ? view_anchor : spr_anchor;
}

void CharacterExtras::ReadFromSavegame(CharacterInfo *ch, Stream *in, CharacterSvgVersion save_ver)
{
    // saves before kCharSvgVersion_400_28 have the legacy inventory (as with audio_speed below)
    if (save_ver < kCharSvgVersion_400_28)
    {
        int16_t invorder[LEGACY_MAX_CHAR_INVENTORY];
        in->ReadArrayOfInt16(invorder, LEGACY_MAX_CHAR_INVENTORY);
        int invorder_count = in->ReadInt16();
        inventory.resize(invorder_count);
        std::copy(invorder, invorder + invorder_count, inventory.begin());
    }
    else
    {
        uint32_t inv_count = in->ReadInt32();
        inventory.resize(inv_count);
        in->ReadArrayOfInt32(inventory.data(), inv_count);
        if (inv_count > MAX_CHAR_INVENTORY)
            inventory.resize(MAX_CHAR_INVENTORY);
    }

    width = in->ReadInt16();
    height = in->ReadInt16();
    zoom = in->ReadInt16();
    xwas = in->ReadInt16();
    ywas = in->ReadInt16();
    tint_r = in->ReadInt16();
    tint_g = in->ReadInt16();
    tint_b = in->ReadInt16();
    tint_level = in->ReadInt16();
    tint_light = in->ReadInt16();
    process_idle_this_time = in->ReadInt8();
    slow_move_counter = in->ReadInt8();
    animwait = in->ReadInt16();
    int cur_anim_volume = 100;
    audio_volume = static_cast<uint8_t>(in->ReadInt8());
    cur_audio_volume = static_cast<uint8_t>(in->ReadInt8());
    audio_panning = in->ReadInt8();
    in->ReadInt8(); // reserved

    if (save_ver >= kCharSvgVersion_400_13)
    {
        following = in->ReadInt32();
        follow_dist = in->ReadInt32();
        follow_eagerness = in->ReadInt32();
    }
    else
    {
        following = -1;
        follow_dist = 0;
        follow_eagerness = 0;
    }

    if (save_ver >= kCharSvgVersion_400_28)
    {
        audio_speed = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }

    blend_mode = (BlendMode)in->ReadInt32();
    // Reserved for colour options
    in->ReadInt32(); // colour flags
    // Reserved for transform options
    in->ReadInt32();              // sprite transform flags1
    in->ReadInt32();              // sprite transform flags2
    in->ReadInt32();              // transform scale x
    in->ReadInt32();              // transform scale y
    in->ReadInt32();              // transform skew x
    in->ReadInt32();              // transform skew y
    rotation = in->ReadFloat32(); // transform rotate
    float px = in->ReadFloat32(); // sprite pivot x
    float py = in->ReadFloat32(); // sprite pivot y
    float ax = in->ReadFloat32(); // sprite anchor x
    float ay = in->ReadFloat32(); // sprite anchor y
    spr_anchor = Pointf(ax, ay);
    pivot = Pointf(px, py);

    if (save_ver >= kCharSvgVersion_400_03)
    {
        face_dir_ratio = in->ReadFloat32();
        movelist_handle = in->ReadInt32();
        flags = in->ReadInt32();
        turns = in->ReadInt32();
    }
    else
    {
        face_dir_ratio = 0.f;
        movelist_handle = 0;
        flags = 0;
        turns = 0;
    }

    AnimFlowStyle anim_flow = kAnimFlow_None;
    AnimFlowDirection anim_dir_initial = kAnimDirForward;
    AnimFlowDirection anim_dir_current = kAnimDirForward;
    int anim_delay = 0;
    if (save_ver >= kCharSvgVersion_400_18)
    {
        shader_id = in->ReadInt32();
        shader_handle = in->ReadInt32();
        // new anim fields are valid since kCharSvgVersion_400_20
        anim_flow = static_cast<AnimFlowStyle>(in->ReadInt8());
        anim_dir_initial = static_cast<AnimFlowDirection>(in->ReadInt8());
        anim_dir_current = static_cast<AnimFlowDirection>(in->ReadInt8());
        in->ReadInt8(); // reserved to fill int32
        anim_delay = in->ReadInt16();
        in->ReadInt16(); // reserved to fill int32
    }
    else
    {
        shader_id = 0;
        shader_handle = 0;
    }

    if (save_ver >= kCharSvgVersion_400_26)
    {
        int offx = in->ReadInt32();
        int offy = in->ReadInt32();
        spr_offset = Point(offx, offy);
        float view_ax = in->ReadFloat32();
        float view_ay = in->ReadFloat32();
        view_anchor = Pointf(view_ax, view_ay);
        offx = in->ReadInt32();
        offy = in->ReadInt32();
        view_offset = Point(offx, offy);
    }
    else
    {
        spr_anchor = Pointf(0.5f, 1.f); // default: middle-bottom
        spr_offset = Point();
    }

    if (save_ver >= kCharSvgVersion_400_29)
    {
        int pivot_offx = in->ReadInt32();
        int pivot_offy = in->ReadInt32();
        pivot_offset = Point(pivot_offx, pivot_offy);
    }
    else
    {
        pivot = Point(0.5f, 0.5f); // default: center
        pivot_offset = Point();
    }

    // NOTE: the legacy animating fields from old save fmt are applied externally,
    // because they are loaded into deprecated CharacterInfo fields
    anim = ViewAnimateParams(anim_flow, anim_dir_initial, anim_dir_current, anim_delay, cur_anim_volume);

    UpdateEffectiveValues(ch);
}

void CharacterExtras::WriteToSavegame(Stream *out) const
{
    out->WriteInt32(static_cast<uint32_t>(inventory.size()));
    out->WriteArrayOfInt32(inventory.data(), inventory.size());
    out->WriteInt16(width);
    out->WriteInt16(height);
    out->WriteInt16(zoom);
    out->WriteInt16(xwas);
    out->WriteInt16(ywas);
    out->WriteInt16(tint_r);
    out->WriteInt16(tint_g);
    out->WriteInt16(tint_b);
    out->WriteInt16(tint_level);
    out->WriteInt16(tint_light);
    out->WriteInt8(process_idle_this_time);
    out->WriteInt8(slow_move_counter);
    out->WriteInt16(animwait);
    // since 3.6.0.25
    out->WriteInt8(static_cast<uint8_t>(audio_volume));
    out->WriteInt8(static_cast<uint8_t>(anim.AudioVolume));
    out->WriteInt8(audio_panning);
    out->WriteInt8(0); // reserved
    // kCharSvgVersion_400_13
    out->WriteInt32(following);
    out->WriteInt32(follow_dist);
    out->WriteInt32(follow_eagerness);
    // kCharSvgVersion_400_28
    out->WriteInt32(audio_speed);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    // kCharSvgVersion_400
    out->WriteInt32(blend_mode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    // Reserved for transform options
    out->WriteInt32(0);              // sprite transform flags1
    out->WriteInt32(0);              // sprite transform flags2
    out->WriteInt32(0);              // transform scale x
    out->WriteInt32(0);              // transform scale y
    out->WriteInt32(0);              // transform skew x
    out->WriteInt32(0);              // transform skew y
    out->WriteFloat32(rotation);     // transform rotate
    out->WriteFloat32(pivot.X);      // sprite pivot x
    out->WriteFloat32(pivot.Y);      // sprite pivot y
    out->WriteFloat32(spr_anchor.X); // sprite anchor x
    out->WriteFloat32(spr_anchor.Y); // sprite anchor y
    // -- kCharSvgVersion_400_03
    out->WriteFloat32(face_dir_ratio);
    out->WriteInt32(movelist_handle);
    out->WriteInt32(flags);
    out->WriteInt32(turns);
    // -- kCharSvgVersion_400_18
    out->WriteInt32(shader_id);
    out->WriteInt32(shader_handle);
    // new anim fields are valid since kCharSvgVersion_400_20
    out->WriteInt8(anim.Flow);
    out->WriteInt8(anim.InitialDirection);
    out->WriteInt8(anim.Direction);
    out->WriteInt8(0); // reserved to fill int32
    out->WriteInt16(anim.Delay);
    out->WriteInt16(0); // reserved to fill int32
    // kCharSvgVersion_400_26
    out->WriteInt32(spr_offset.X);
    out->WriteInt32(spr_offset.Y);
    out->WriteFloat32(view_anchor.X);
    out->WriteFloat32(view_anchor.Y);
    out->WriteInt32(view_offset.X);
    out->WriteInt32(view_offset.Y);
    // kCharSvgVersion_400_29
    out->WriteInt32(pivot_offset.X);
    out->WriteInt32(pivot_offset.Y);
}
