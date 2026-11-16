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
#include "game/world/object.h"

#include "debug/debug_log.h"
#include "game/dynobj/cc_object.h"
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_shader.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/navigation/route_finder.h"
#include "game/render/draw.h"
#include "game/runtime_defines.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/gui.h"
#include "game/view.h"
#include "game/world/character.h"
#include "game/world/move_list.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_status.h"
#include "game/world/view_frame.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;


// AreThingsOverlapping takes a character ID, or a room object ID plus this offset, for a "thing"
// (the script API, C3).
#define OVERLAPPING_OBJECT 1000

bool IsValidObject(int obj_id)
{
    return ((obj_id >= 0) && (static_cast<uint32_t>(obj_id) < Room::Current().State->numobj));
}

bool AssertObject(const char *apiname, int obj_id)
{
    Room &room = Room::Current();
    if ((obj_id >= 0) && (static_cast<uint32_t>(obj_id) < room.State->numobj))
        return true;
    debug_script_error("%s: invalid object id %d (range is 0..%d)", apiname, obj_id,
                       room.State->numobj > 0 ? room.State->numobj - 1 : 0);
    return false;
}

static int GetThingRect(int thing, Rect *rect)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (IsValidCharacter(thing))
    {
        // FIXME: a disabled character in this room still counts as overlapping, a disabled object does not
        // (below), and neither case looks at visibility
        if (game.Characters[thing].Room != game.DisplayedRoom)
            return 0;

        const int charw = GetCharacterWidth(game, thing);
        const int charh = GetCharacterHeight(game, thing);
        *rect = RectWH(game.Characters[thing].X - (charw / 2), game.Characters[thing].GetEffectiveY() - charh + 1,
                       charw, charh);
    }
    else if (IsValidObject(thing - OVERLAPPING_OBJECT))
    {
        int obj_id = thing - OVERLAPPING_OBJECT;
        if (!room.Objs[obj_id].is_enabled())
        {
            return 0;
        }

        const int objw = room.Objs[obj_id].get_width();
        const int objh = room.Objs[obj_id].get_height();
        *rect = RectWH(room.Objs[obj_id].x, room.Objs[obj_id].y - objh + 1, objw, objh);
    }
    else
    {
        quit("!AreThingsOverlapping: invalid parameter");
    }

    return 1;
}

int AreThingsOverlapping(int thing1, int thing2)
{
    Rect r1, r2;
    // get the bounding rectangles, and return 0 if the object/char
    // is not there (a character in another room, a disabled object)
    if (GetThingRect(thing1, &r1) == 0)
        return 0;
    if (GetThingRect(thing2, &r2) == 0)
        return 0;

    if (AreRectsIntersecting(r1, r2))
    {
        // determine how far apart they are
        // take the smaller of the X distances as the overlapping amount
        int xdist = abs(r1.Right - r2.Left) + 1;
        if (abs(r1.Left - r2.Right) < xdist)
            xdist = abs(r1.Left - r2.Right) + 1;
        // take the smaller of the Y distances
        int ydist = abs(r1.Bottom - r2.Top) + 1;
        if (abs(r1.Top - r2.Bottom) < ydist)
            ydist = abs(r1.Top - r2.Bottom) + 1;
        // the overlapping amount is the smaller of the X and Y overlap
        if (xdist < ydist)
            return xdist;
        else
            return ydist;
    }
    return 0;
}

int AreObjectsColliding(int obj1, int obj2)
{
    if (!AssertObject("AreObjectsColliding", obj1) || !AssertObject("AreObjectsColliding", obj2))
        return 0;

    return (AreThingsOverlapping(obj1 + OVERLAPPING_OBJECT, obj2 + OVERLAPPING_OBJECT)) ? 1 : 0;
}

int Object_IsCollidingWithObject(ScriptObject *obj, ScriptObject *other)
{
    return AreObjectsColliding(obj->id, other->id);
}

Bitmap *GetObjectImage(int obj_id, bool always_original, bool *is_original)
{
    Bitmap *pic = nullptr;
    if (!always_original)
    {
        // NOTE: the cached image will only be present in software render mode
        pic = get_cached_object_image(obj_id);
    }

    if (is_original)
        *is_original = pic == nullptr; // if not used cached image, then use original
    if (!pic)
        pic = EngineServices::Current().Sprites[Room::Current().Objs[obj_id].num];
    return pic;
}

Bitmap *GetObjectSourceImage(Room &room, EngineServices &services, int obj_id)
{
    return services.Sprites[room.Objs[obj_id].num];
}

int GetObjectIDAtRoom(int roomx, int roomy, int hit_options)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Room &room = Room::Current();
    const bool only_clickable = (hit_options & kHit_Interactable) != 0;
    int bestshotyp = -1, bestshotwas = -1;
    // Iterate through all objects in the room
    for (uint32_t obj_id = 0; obj_id < room.State->numobj; obj_id++)
    {
        if (!room.Objs[obj_id].is_displayed())
        {
            continue; // disabled or invisible
        }
        if (only_clickable && (room.Objs[obj_id].flags & OBJF_NOINTERACT))
        {
            continue;
        }
        SpriteTransformFlags sprite_flags = kSprTf_None;
        int sprite_width = room.Objs[obj_id].get_width();
        int sprite_height = room.Objs[obj_id].get_height();
        // TODO: support mirrored transformation in GraphicSpace
        if (room.Objs[obj_id].view != RoomObject::NoView)
        {
            sprite_flags =
                game.Views[room.Objs[obj_id].view].loops[room.Objs[obj_id].loop].frames[room.Objs[obj_id].frame].flags;
        }

        Bitmap *image = GetObjectSourceImage(room, services, obj_id);
        // Convert to local object coordinates
        Point local = room.Objs[obj_id].GetGraphicSpace().WorldToLocal(roomx, roomy);
        if (is_pos_in_sprite(game, local.X, local.Y, 0, 0, image, sprite_width, sprite_height, sprite_flags) == FALSE)
        {
            continue;
        }

        int use_baseline = room.Objs[obj_id].get_baseline();
        if (use_baseline < bestshotyp)
        {
            continue;
        }

        bestshotwas = obj_id;
        bestshotyp = use_baseline;
    }
    game.ObjLowestYp = bestshotyp;
    return bestshotwas;
}

ScriptObject *Object_GetAtScreenXY(int x, int y, int hit_options)
{
    Game &game = Game::Current();
    // translate screen co-ordinates to room co-ordinates
    VpPoint vpt = game.Viewports.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return nullptr;
    int obj_id = GetObjectIDAtRoom(vpt.first.X, vpt.first.Y, hit_options);
    if (obj_id < 0)
    {
        return nullptr;
    }
    return &game.ScObjects[obj_id];
}

ScriptObject *Object_GetAtScreenXY2(int x, int y)
{
    return Object_GetAtScreenXY(x, y, true);
}

ScriptObject *Object_GetAtRoomXY(int x, int y, int hit_options)
{
    int obj_id = GetObjectIDAtRoom(x, y, hit_options);
    if (obj_id < 0)
    {
        return nullptr;
    }
    return &Game::Current().ScObjects[obj_id];
}
ScriptObject *Object_GetAtRoomXY2(int x, int y)
{
    return Object_GetAtRoomXY(x, y, true);
}

void SetObjectTint(int obj_id, int red, int green, int blue, int opacity, int luminance)
{
    Room &room = Room::Current();
    if (!AssertObject("SetObjectTint", obj_id))
    {
        return;
    }

    if ((red < 0) || (green < 0) || (blue < 0) || (red > 255) || (green > 255) || (blue > 255) || (opacity < 0) ||
        (opacity > 255) || (luminance < 0) || (luminance > 255))
    {
        debug_script_warn("Object.Tint: invalid parameter(s). R,G,B must be 0-255 (passed: %d,%d,%d), opacity & "
                          "luminance 0-255 (passed: %d,%d)",
                          red, green, blue, opacity, luminance);
        return;
    }

    debug_script_log("Set object %d tint RGB(%d,%d,%d) %d%%", obj_id, red, green, blue, opacity);

    room.Objs[obj_id].tint_r = red;
    room.Objs[obj_id].tint_g = green;
    room.Objs[obj_id].tint_b = blue;
    room.Objs[obj_id].tint_level = opacity;
    room.Objs[obj_id].tint_light = luminance;
    room.Objs[obj_id].flags &= ~OBJF_HASLIGHT;
    room.Objs[obj_id].flags |= OBJF_HASTINT;
}

void RemoveObjectTint(int obj_id)
{
    Room &room = Room::Current();
    if (!AssertObject("RemoveObjectTint", obj_id))
    {
        return;
    }

    if (room.Objs[obj_id].flags & (OBJF_HASTINT | OBJF_HASLIGHT))
    {
        debug_script_log("Un-tint object %d", obj_id);
        room.Objs[obj_id].flags &= ~(OBJF_HASTINT | OBJF_HASLIGHT);
    }
    else
    {
        debug_script_warn("RemoveObjectTint called but object was not tinted");
    }
}

void Object_Tint(ScriptObject *obj, int red, int green, int blue, int saturation, int luminance)
{
    SetObjectTint(obj->id, red, green, blue, saturation, luminance);
}

void Object_RemoveTint(ScriptObject *obj)
{
    RemoveObjectTint(obj->id);
}

void SetObjectGraphic(int obj_id, int slot)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Graphic", obj_id))
    {
        return;
    }

    if (room.Objs[obj_id].num != slot)
    {
        room.Objs[obj_id].num = Math::InRangeOrDef<uint16_t>(slot, 0);
        if (slot > UINT16_MAX)
        {
            debug_script_warn("Object's (id %d) sprite %d is outside of internal range (%d), reset to 0", obj_id, slot,
                              UINT16_MAX);
        }
        debug_script_log("Object %d graphic changed to slot %d", obj_id, slot);
    }
    room.Objs[obj_id].frame = 0;
    room.Objs[obj_id].loop = 0;
    room.Objs[obj_id].view = RoomObject::NoView;
    room.Objs[obj_id].anim = ViewAnimateParams(); // reset anim
}

bool SetObjectFrameSimple(int obj_id, int view, int loop, int frame)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (!AssertObject("SetObjectFrame", obj_id))
    {
        return false;
    }
    view--;
    AssertViewHasLoops("SetObjectFrame", room.Objects[obj_id].ScriptName.GetCStr(), view);

    auto &room_obj = room.Objs[obj_id];
    // Fixup invalid loop & frame numbers by using default 0 value
    if (loop < 0 || loop >= game.Views[view].numLoops)
    {
        debug_script_warn("SetObjectFrame: invalid loop number used for view %d (%d, range is 0 - %d)", view, loop,
                          game.Views[view].GetLastLoop());
        loop = 0;
    }
    if (frame < 0 || frame >= game.Views[view].loops[loop].numFrames)
    {
        debug_script_warn("SetObjectFrame: frame index out of range (%d, must be 0 - %d)", frame,
                          game.Views[view].loops[loop].GetLastFrame());
        frame = 0; // NOTE: we have 1 dummy frame allocated for empty loops
    }

    // Current engine's object data limitation by uint16_t
    if (view > UINT16_MAX || loop > UINT16_MAX || frame > UINT16_MAX)
    {
        debug_script_warn(
            "Object's (id %d) view/loop/frame (%d/%d/%d) is outside of internal range (%d/%d/%d), reset to no view",
            obj_id, view + 1, loop, frame, UINT16_MAX + 1, UINT16_MAX, UINT16_MAX);
        SetObjectGraphic(obj_id, 0);
        return false;
    }

    room_obj.view = static_cast<uint16_t>(view);
    room_obj.loop = static_cast<uint16_t>(loop);
    room_obj.frame = static_cast<uint16_t>(frame);
    room_obj.anim = ViewAnimateParams(); // reset anim
    int pic = game.Views[view].loops[loop].frames[frame].pic;
    room_obj.num = Math::InRangeOrDef<uint16_t>(pic, 0);
    if (pic > UINT16_MAX)
        debug_script_warn("Object's (id %d) sprite %d is outside of internal range (%d), reset to 0", obj_id, pic,
                          UINT16_MAX);
    return true;
}

void Object_SetView(ScriptObject *obj, int view, int loop, int frame)
{
    SetObjectFrameSimple(obj->id, view, loop, frame);
}

// pass alpha=255 for fully solid, alpha=0 for fully transparent
void SetObjectAlpha(int obj_id, int alpha)
{
    if (!AssertObject("Object.Opacity", obj_id))
    {
        return;
    }
    if ((alpha < 0) || (alpha > 255))
        quit("!Object.Opacity: alpha value must be between 0 and 255");

    Room::Current().Objs[obj_id].opacity = alpha;
}

void Object_SetOpacity(ScriptObject *obj, int alpha)
{
    SetObjectAlpha(obj->id, alpha);
}

int Object_GetOpacity(ScriptObject *obj)
{
    if (!AssertObject("Object.Opacity", obj->id))
    {
        return 0;
    }

    return Room::Current().Objs[obj->id].opacity;
}

// [OBSOLETE] the AGS 4.0 Transparency, for compiled 4.0 games
void Object_SetTransparency(ScriptObject *obj, int trans)
{
    if ((trans < 0) || (trans > 100))
        quit("!SetObjectTransparency: transparency value must be between 0 and 100");
    SetObjectAlpha(obj->id, GfxDef::Trans100ToOpacity(trans));
}

int Object_GetTransparency(ScriptObject *obj)
{
    if (!AssertObject("Object.Transparency", obj->id))
    {
        return 0;
    }

    return GfxDef::OpacityToTrans100(Room::Current().Objs[obj->id].opacity);
}

int Object_GetAudioPanning(ScriptObject *obj)
{
    if (!AssertObject("Object.AudioPanning", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].audio_panning;
}

void Object_SetAudioPanning(ScriptObject *obj, int new_value)
{
    if (!AssertObject("Object.AudioPanning", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].audio_panning = Math::Clamp(new_value, -100, 100);
}

int Object_GetAudioSpeed(ScriptObject *obj)
{
    if (!AssertObject("Object.AudioSpeed", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].AudioSpeed;
}

void Object_SetAudioSpeed(ScriptObject *obj, int new_value)
{
    if (!AssertObject("Object.AudioSpeed", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].AudioSpeed = new_value;
}

int Object_GetAudioVolume(ScriptObject *obj)
{
    if (!AssertObject("Object.AudioVolume", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].audio_volume;
}

void Object_SetAudioVolume(ScriptObject *obj, int new_value)
{
    if (!AssertObject("Object.AudioVolume", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].audio_volume = Math::Clamp(new_value, 0, 100);
}

void SetObjectBaseline(int obj_id, int baseline)
{
    Room &room = Room::Current();
    if (!AssertObject("SetObjectBaseline", obj_id))
    {
        return;
    }
    // baseline has changed, invalidate the cache
    if (room.Objs[obj_id].baseline != baseline)
    {
        room.Objs[obj_id].baseline = baseline;
        mark_object_changed(obj_id);
    }
}

int GetObjectBaseline(int obj_id)
{
    Room &room = Room::Current();
    if (!AssertObject("GetObjectBaseline", obj_id))
    {
        return 0;
    }

    if (room.Objs[obj_id].baseline < 1)
    {
        return 0;
    }

    return room.Objs[obj_id].baseline;
}

void Object_SetBaseline(ScriptObject *obj, int baseline)
{
    SetObjectBaseline(obj->id, baseline);
}

int Object_GetBaseline(ScriptObject *obj)
{
    return GetObjectBaseline(obj->id);
}

void AnimateObjectImpl(int obj_id, int loop, int spdd, int rept, int direction, int blocking, int sframe, int volume)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (!AssertObject("AnimateObject", obj_id))
    {
        return;
    }

    RoomObject &room_obj = room.Objs[obj_id];
    const RoomObjectInfo &obji = room.Objects[obj_id];

    if (!room_obj.is_enabled())
    {
        debug_script_warn("%s: object %d is disabled, cannot animate", "Object.Animate", obj_id);
        return;
    }
    if (room_obj.view == RoomObject::NoView)
    {
        debug_script_warn("%s: object %d has not been assigned a view", "AnimateObject", obj_id);
        return;
    }

    ValidateViewAnimVLF("Object.Animate", obji.ScriptName.GetCStr(), room_obj.view, loop, sframe);
    ValidateViewAnimParams("Object.Animate", obji.ScriptName.GetCStr(), blocking, rept, direction);

    if (loop > UINT16_MAX || sframe > UINT16_MAX)
    {
        debug_script_warn("Object's (id %d) loop/frame (%d/%d) is outside of internal range (%d/%d), cancel animation",
                          obj_id, loop, sframe, UINT16_MAX, UINT16_MAX);
        return;
    }

    debug_script_log("Obj %d start anim view %d loop %d, speed %d, repeat %d, frame %d", obj_id, room_obj.view + 1,
                     loop, spdd, rept, sframe);

    room_obj.set_animating(static_cast<AnimFlowStyle>(rept), static_cast<AnimFlowDirection>(direction), spdd,
                           Math::Clamp(volume, 0, 100));
    room_obj.loop = (uint16_t)loop;
    room_obj.frame =
        (uint16_t)SetFirstAnimFrame(room_obj.view, loop, sframe, static_cast<AnimFlowDirection>(direction));
    room_obj.wait = spdd + game.Views[room_obj.view].loops[loop].frames[room_obj.frame].speed;
    int pic = game.Views[room_obj.view].loops[loop].frames[room_obj.frame].pic;
    room_obj.num = Math::InRangeOrDef<uint16_t>(pic, 0);
    if (pic > UINT16_MAX)
        debug_script_warn("Object's (id %d) sprite %d is outside of internal range (%d), reset to 0", obj_id, pic,
                          UINT16_MAX);

    room.Objs[obj_id].CheckViewFrame();

    if (blocking)
        GameLoopUntilViewAnimEnd(&room_obj.anim);
}

void Object_Animate(ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction, int sframe,
                    int volume)
{
    AnimateObjectImpl(obj->id, loop, delay, repeat, direction, blocking, sframe, volume);
}

void Object_Animate5(ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction)
{
    Object_Animate(obj, loop, delay, repeat, blocking, direction, 0 /* frame */, 100 /* full volume */);
}

void Object_Animate6(ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction, int sframe)
{
    Object_Animate(obj, loop, delay, repeat, blocking, direction, sframe, 100 /* full volume */);
}

void Object_StopAnimating(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.StopAnimating", obj->id))
    {
        return;
    }

    if (room.Objs[obj->id].is_animating())
    {
        room.Objs[obj->id].anim = ViewAnimateParams();
        room.Objs[obj->id].wait = 0;
    }
}

void StopObjectMoving(int obj_id)
{
    if (!AssertObject("StopObjectMoving", obj_id))
    {
        return;
    }

    Room::Current().Objs[obj_id].OnStopMoving();

    debug_script_log("Object %d stop moving", obj_id);
}

void Object_StopMoving(ScriptObject *obj)
{
    StopObjectMoving(obj->id);
}

int Object_GetView(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.View", obj->id))
    {
        return 0;
    }
    if (room.Objs[obj->id].view == RoomObject::NoView)
    {
        return 0;
    }
    return room.Objs[obj->id].view + 1;
}

int Object_GetLoop(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Loop", obj->id))
    {
        return 0;
    }
    if (room.Objs[obj->id].view == RoomObject::NoView)
    {
        return 0;
    }
    return room.Objs[obj->id].loop;
}

int Object_GetFrame(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Frame", obj->id))
    {
        return 0;
    }
    if (room.Objs[obj->id].view == RoomObject::NoView)
    {
        return 0;
    }
    return room.Objs[obj->id].frame;
}

bool Object_GetEnabled(ScriptObject *obj)
{
    if (!AssertObject("Object.Enabled", obj->id))
    {
        return false;
    }
    return Room::Current().Objs[obj->id].is_enabled();
}

void Object_SetEnabled(ScriptObject *obj, bool on)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Enabled", obj->id))
    {
        return;
    }
    if (room.Objs[obj->id].is_enabled() != on)
    {
        room.Objs[obj->id].set_enabled(on);
        debug_script_log("Object %d enabled = %d", obj->id, on);
    }
}

bool Object_GetVisible(ScriptObject *obj)
{
    if (!AssertObject("Object.Visible", obj->id))
    {
        return false;
    }
    return Room::Current().Objs[obj->id].is_visible();
}

void Object_SetVisible(ScriptObject *obj, bool on)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Visible", obj->id))
    {
        return;
    }
    if (room.Objs[obj->id].is_visible() != on)
    {
        room.Objs[obj->id].set_visible(on);
        debug_script_log("Object %d visible = %d", obj->id, on);
    }
}

void Object_SetGraphic(ScriptObject *obj, int slot)
{
    SetObjectGraphic(obj->id, slot);
}

int GetObjectGraphic(int obj_id)
{
    if (!AssertObject("GetObjectGraphic", obj_id))
    {
        return 0;
    }
    return Room::Current().Objs[obj_id].num;
}

int Object_GetGraphic(ScriptObject *obj)
{
    return GetObjectGraphic(obj->id);
}

int GetObjectX(int obj_id)
{
    if (!AssertObject("GetObjectX", obj_id))
    {
        return 0;
    }
    return Room::Current().Objs[obj_id].x;
}

int Object_GetX(ScriptObject *obj)
{
    return GetObjectX(obj->id);
}

int GetObjectY(int obj_id)
{
    if (!AssertObject("GetObjectY", obj_id))
    {
        return 0;
    }
    return Room::Current().Objs[obj_id].y;
}

int Object_GetY(ScriptObject *obj)
{
    return GetObjectY(obj->id);
}

int IsObjectAnimating(int obj_id)
{
    if (!AssertObject("IsObjectAnimating", obj_id))
    {
        return 0;
    }
    return Room::Current().Objs[obj_id].is_animating() ? 1 : 0;
}

int Object_GetAnimating(ScriptObject *obj)
{
    return IsObjectAnimating(obj->id);
}

int IsObjectMoving(int obj_id)
{
    if (!AssertObject("IsObjectMoving", obj_id))
    {
        return 0;
    }
    return (Room::Current().Objs[obj_id].moving > 0) ? 1 : 0;
}

int Object_GetMoving(ScriptObject *obj)
{
    return IsObjectMoving(obj->id);
}

bool Object_HasExplicitLight(ScriptObject *obj)
{
    if (!AssertObject("Object.HasExplicitLight", obj->id))
        return false;
    return Room::Current().Objs[obj->id].has_explicit_light();
}

bool Object_HasExplicitTint(ScriptObject *obj)
{
    if (!AssertObject("Object.HasExplicitTint", obj->id))
        return false;
    return Room::Current().Objs[obj->id].has_explicit_tint();
}

int Object_GetLightLevel(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.LightLevel", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_light() ? room.Objs[obj->id].tint_light : 0;
}

void Object_SetLightLevel(ScriptObject *obj, int light_level)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.LightLevel", obj->id))
    {
        return;
    }

    int obj_id = obj->id;
    room.Objs[obj_id].tint_light = light_level;
    room.Objs[obj_id].flags &= ~OBJF_HASTINT;
    room.Objs[obj_id].flags |= OBJF_HASLIGHT;
}

int Object_GetTintRed(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.TintRed", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_tint() ? room.Objs[obj->id].tint_r : 0;
}

int Object_GetTintGreen(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.TintGreen", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_tint() ? room.Objs[obj->id].tint_g : 0;
}

int Object_GetTintBlue(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.TintBlue", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_tint() ? room.Objs[obj->id].tint_b : 0;
}

int Object_GetTintSaturation(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.TintSaturation", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_tint() ? room.Objs[obj->id].tint_level : 0;
}

int Object_GetTintLuminance(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.TintLuminance", obj->id))
        return 0;
    return room.Objs[obj->id].has_explicit_tint() ? room.Objs[obj->id].tint_light : 0;
}

void SetObjectPosition(int obj_id, int tox, int toy)
{
    Room &room = Room::Current();
    if (!AssertObject("SetObjectPosition", obj_id))
    {
        return;
    }

    if (room.Objs[obj_id].moving > 0)
    {
        debug_script_warn("Object.SetPosition: cannot set position while object is moving");
        return;
    }

    room.Objs[obj_id].x = tox;
    room.Objs[obj_id].y = toy;
    room.Objs[obj_id].UpdateGraphicSpace();
}

void Object_SetPosition(ScriptObject *obj, int x, int y)
{
    SetObjectPosition(obj->id, x, y);
}

void Object_SetX(ScriptObject *obj, int x)
{
    SetObjectPosition(obj->id, x, Room::Current().Objs[obj->id].y);
}

void Object_SetY(ScriptObject *obj, int y)
{
    SetObjectPosition(obj->id, Room::Current().Objs[obj->id].x, y);
}

const char *Object_GetName_New(ScriptObject *obj)
{
    if (!AssertObject("Object.Name", obj->id))
    {
        return nullptr;
    }

    return CreateNewScriptString(Room::Current().State->obj[obj->id].name);
}

void Object_SetName(ScriptObject *obj, const char *new_name)
{
    if (!AssertObject("Object.Name", obj->id))
    {
        return;
    }
    Room::Current().State->obj[obj->id].name = new_name;
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

void RunObjectInteraction(int obj_id, int mood)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (!AssertObject("RunObjectInteraction", obj_id))
    {
        return;
    }

    if (!AssertCursorValidForEvent("Object.RunInteraction", mood))
        return;

    // Cursor mode must match the event index in "Interactions" table
    const int evnt = (game.mcurs[mood].flags & MCF_EVENT) != 0 ? mood : -1;
    const int anyclick_evt = kRoomObjectEvent_AnyClick;

    // For USE verb: remember active inventory
    if (game.HasCursorRole(mood, kCursorRole_UseInv))
    {
        game.Play.usedinv = game.PlayerChar->ActiveInventory;
    }

    const auto obj_evt =
        ObjectEvent(kScTypeRoom, LOCTYPE_OBJ, obj_id,
                    RuntimeScriptValue().SetScriptObject(&game.ScObjects[obj_id], &ccDynamicObject), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &room.Objects[obj_id].Interactions, evnt, &room.Objects[obj_id].GetEvents(),
                         anyclick_evt, true /* do unhandled event */) < 0)
        return; // game state changed, don't do "any click"
    // any click on obj
    run_event_script(obj_evt, &room.Objects[obj_id].GetEvents(), anyclick_evt);
}

bool Object_IsInteractionAvailable(ScriptObject *obj, int mood)
{
    Game &game = Game::Current();
    game.Play.check_interaction_only = 1;
    RunObjectInteraction(obj->id, mood);
    // In the check-only mode run_event_script runs no handler, but sets check_interaction_only to 2
    // if it found one (for the event, or for "any click")
    // TODO: return the check result from run_event_script, instead of passing it through game.Play.check_interaction_only
    int ciwas = game.Play.check_interaction_only;
    game.Play.check_interaction_only = 0;
    return (ciwas == 2);
}

void move_object(int obj_id, const std::vector<Point> *path, int tox, int toy, int speed, bool ignwal,
                 const RunPathParams &run_params);

void Object_DoMove(ScriptObject *obj, const char *api_name, void *path_arr, int x, int y, bool use_path, int speed,
                   int blocking, int ignwal, int repeat = kAnimFlow_Once, int direction = FORWARDS)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Move", obj->id))
    {
        return;
    }
    const RoomObjectInfo &obji = room.Objects[obj->id];
    ValidateMoveParams(api_name, obji.ScriptName.GetCStr(), blocking, ignwal);
    ValidateAnimParams(api_name, obji.ScriptName.GetCStr(), repeat, direction);

    if (!room.Objs[obj->id].is_enabled())
    {
        debug_script_warn("%s: object %d is disabled, cannot start move", "Object.Move", obj->id);
        return;
    }

    if (use_path)
    {
        std::vector<Point> path;
        if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
        {
            debug_script_warn("%s: path is null, or failed to resolve array of points", api_name);
            return;
        }
        move_object(obj->id, &path, 0, 0, speed, ignwal != 0,
                    RunPathParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction)));
    }
    else
    {
        move_object(obj->id, nullptr, x, y, speed, ignwal != 0, RunPathParams::DefaultOnce());
    }

    if (blocking)
        GameLoopUntilNotMoving(&room.Objs[obj->id].moving);
}

void Object_Move(ScriptObject *obj, int x, int y, int speed, int blocking, int ignwal)
{
    Object_DoMove(obj, "Object.Move", nullptr, x, y, false /* no path */, speed, blocking, ignwal);
}

void Object_MovePath(ScriptObject *obj, void *path_arr, int speed, int blocking, int repeat, int direction)
{
    Object_DoMove(obj, "Object.MovePath", path_arr, 0, 0, true /* use path */, speed, blocking, ANYWHERE, repeat,
                  direction);
}

void SetObjectClickable(int obj_id, int clickable)
{
    Room &room = Room::Current();
    if (!AssertObject("SetObjectClickable", obj_id))
    {
        return;
    }
    room.Objs[obj_id].flags &= ~OBJF_NOINTERACT;
    if (clickable == 0)
    {
        room.Objs[obj_id].flags |= OBJF_NOINTERACT;
    }
}

void Object_SetClickable(ScriptObject *obj, int clickable)
{
    SetObjectClickable(obj->id, clickable);
}

int Object_GetClickable(ScriptObject *obj)
{
    if (!AssertObject("Object.Clickable", obj->id))
    {
        return 0;
    }

    if (Room::Current().Objs[obj->id].flags & OBJF_NOINTERACT)
    {
        return 0;
    }
    return 1;
}

int Object_GetDestinationX(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.DestinationX", obj->id))
    {
        return 0;
    }

    if (room.Objs[obj->id].moving)
    {
        MoveList *cmls = get_movelist(room.Objs[obj->id].moving);
        return cmls->GetLastPos().X;
    }
    return room.Objs[obj->id].x;
}

int Object_GetDestinationY(ScriptObject *obj)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.DestinationY", obj->id))
    {
        return 0;
    }

    if (room.Objs[obj->id].moving)
    {
        MoveList *cmls = get_movelist(room.Objs[obj->id].moving);
        return cmls->GetLastPos().Y;
    }
    return room.Objs[obj->id].y;
}

bool Object_GetManualScaling(ScriptObject *obj)
{
    if (!AssertObject("Object.ManualScaling", obj->id))
    {
        return false;
    }

    return !(Room::Current().Objs[obj->id].flags & OBJF_USEROOMSCALING);
}

void Object_SetManualScaling(ScriptObject *obj, bool on)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.ManualScaling", obj->id))
    {
        return;
    }
    if (on)
        room.Objs[obj->id].flags &= ~OBJF_USEROOMSCALING;
    else
        room.Objs[obj->id].flags |= OBJF_USEROOMSCALING;
    // clear the cache
    mark_object_changed(obj->id);
}

int Object_GetScaling(ScriptObject *obj)
{
    if (!AssertObject("Object.Scaling", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].zoom;
}

void Object_SetScaling(ScriptObject *obj, int zoomlevel)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Scaling", obj->id))
    {
        return;
    }
    if ((room.Objs[obj->id].flags & OBJF_USEROOMSCALING) != 0)
    {
        debug_script_warn("Object.Scaling: cannot set property unless ManualScaling is enabled");
        return;
    }
    int zoom_fixed = Math::Clamp(zoomlevel, 1, (int)(INT16_MAX)); // RoomObject.zoom is int16
    if (zoomlevel != zoom_fixed)
        debug_script_warn("Object.Scaling: scaling level must be between 1 and %d%%, asked for: %d", (int)(INT16_MAX),
                          zoomlevel);
    room.Objs[obj->id].zoom = zoom_fixed;
}

void Object_SetSolid(ScriptObject *obj, int solid)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.Solid", obj->id))
    {
        return;
    }
    room.Objs[obj->id].flags &= ~OBJF_SOLID;
    if (solid)
        room.Objs[obj->id].flags |= OBJF_SOLID;
}

int Object_GetSolid(ScriptObject *obj)
{
    if (!AssertObject("Object.Solid", obj->id))
    {
        return 0;
    }
    if (Room::Current().Objs[obj->id].flags & OBJF_SOLID)
    {
        return 1;
    }
    return 0;
}

void Object_SetBlockingWidth(ScriptObject *obj, int blocking_width)
{
    if (!AssertObject("Object.BlockingWidth", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].blocking_width = static_cast<int16_t>(blocking_width);
}

int Object_GetBlockingWidth(ScriptObject *obj)
{
    if (!AssertObject("Object.BlockingWidth", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].blocking_width;
}

void Object_SetBlockingHeight(ScriptObject *obj, int blocking_height)
{
    if (!AssertObject("Object.BlockingHeight", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].blocking_height = static_cast<int16_t>(blocking_height);
}

int Object_GetBlockingHeight(ScriptObject *obj)
{
    if (!AssertObject("Object.BlockingHeight", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].blocking_height;
}

int Object_GetBlockingRectX(ScriptObject *obj)
{
    if (!AssertObject("Object.BlockingRectX", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].blocking_x;
}

void Object_SetBlockingRectX(ScriptObject *obj, int x)
{
    if (!AssertObject("Object.BlockingRectX", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].blocking_x = static_cast<int16_t>(x);
}

int Object_GetBlockingRectY(ScriptObject *obj)
{
    if (!AssertObject("Object.BlockingRectY", obj->id))
    {
        return 0;
    }
    return Room::Current().Objs[obj->id].blocking_y;
}

void Object_SetBlockingRectY(ScriptObject *obj, int y)
{
    if (!AssertObject("Object.BlockingRectY", obj->id))
    {
        return;
    }
    Room::Current().Objs[obj->id].blocking_y = static_cast<int16_t>(y);
}

int Object_GetID(ScriptObject *obj)
{
    return obj->id;
}

const char *Object_GetScriptName(ScriptObject *obj)
{
    return CreateNewScriptString(Room::Current().Objects[obj->id].ScriptName);
}

int Object_GetBlendMode(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].blend_mode;
}

void Object_SetBlendMode(ScriptObject *obj, int blend_mode)
{
    Room &room = Room::Current();
    room.Objs[obj->id].blend_mode =
        ValidateBlendMode("Object.BlendMode", room.Objects[obj->id].ScriptName.GetCStr(), blend_mode);
}

float Object_GetGraphicAnchorX(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].spr_anchor.X;
}

void Object_SetGraphicAnchorX(ScriptObject *obj, float x)
{
    Room::Current().Objs[obj->id].spr_anchor.X = Math::Clamp(x, 0.f, 1.f);
}

float Object_GetGraphicAnchorY(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].spr_anchor.Y;
}

void Object_SetGraphicAnchorY(ScriptObject *obj, float y)
{
    Room::Current().Objs[obj->id].spr_anchor.Y = Math::Clamp(y, 0.f, 1.f);
}

int Object_GetGraphicOffsetX(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].spr_offset.X;
}

void Object_SetGraphicOffsetX(ScriptObject *obj, int x)
{
    Room::Current().Objs[obj->id].spr_offset.X = x;
}

int Object_GetGraphicOffsetY(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].spr_offset.Y;
}

void Object_SetGraphicOffsetY(ScriptObject *obj, int y)
{
    Room::Current().Objs[obj->id].spr_offset.Y = y;
}

ScriptShaderInstance *Object_GetShader(ScriptObject *obj)
{
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(Room::Current().Objs[obj->id].shader_handle));
}

void Object_SetShader(ScriptObject *obj, ScriptShaderInstance *shader_inst)
{
    auto &room_obj = Room::Current().Objs[obj->id];
    room_obj.shader_id = shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID;
    room_obj.shader_handle = ccReplaceObjectHandle(room_obj.shader_handle, shader_inst);
}

float Object_GetRotation(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].rotation;
}

void Object_SetRotation(ScriptObject *obj, float degrees)
{
    Room &room = Room::Current();
    room.Objs[obj->id].rotation = Math::ClampAngle360(degrees);
    room.Objs[obj->id].UpdateGraphicSpace();
}

float Object_GetGraphicPivotX(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].pivot.X;
}

void Object_SetGraphicPivotX(ScriptObject *obj, float pivotx)
{
    Room::Current().Objs[obj->id].pivot.X = pivotx;
}

float Object_GetGraphicPivotY(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].pivot.Y;
}

void Object_SetGraphicPivotY(ScriptObject *obj, float pivoty)
{
    Room::Current().Objs[obj->id].pivot.Y = pivoty;
}

int Object_GetGraphicPivotOffsetX(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].pivot_offset.X;
}

void Object_SetGraphicPivotOffsetX(ScriptObject *obj, int px)
{
    Room::Current().Objs[obj->id].pivot_offset.X = px;
}

int Object_GetGraphicPivotOffsetY(ScriptObject *obj)
{
    return Room::Current().Objs[obj->id].pivot_offset.Y;
}

void Object_SetGraphicPivotOffsetY(ScriptObject *obj, int py)
{
    Room::Current().Objs[obj->id].pivot_offset.Y = py;
}

void move_object(int obj_id, const std::vector<Point> *path, int tox, int toy, int speed, bool ignwal,
                 const RunPathParams &run_params)
{
    Room &room = Room::Current();
    RoomObject &room_obj = room.Objs[obj_id];
    if (path)
    {
        if (path->empty())
        {
            debug_script_log("MoveObject: path is empty for object %d, not moving", obj_id);
            return;
        }

        room_obj.x = path->front().X;
        room_obj.y = path->front().Y;
        tox = path->back().X;
        toy = path->back().Y;
    }
    else if ((tox == room_obj.x) && (toy == room_obj.y))
    {
        debug_script_log("MoveObject: object %d already at destination, not moving", obj_id);
        return;
    }

    debug_script_log("Object %d start move to %d,%d", obj_id, tox, toy);

    MoveList new_mlist;
    bool path_result = false;
    if (path)
    {
        path_result =
            MoveListOp::CalculateMoveList(new_mlist, *path, speed, speed, ignwal ? kMoveStage_Direct : 0, run_params);
    }
    else
    {
        MaskRouteFinder *pathfind = get_room_pathfinder();
        pathfind->SetWalkableArea(prepare_walkable_areas(-1), room.MaskResolution);
        path_result = Pathfinding::FindRoute(new_mlist, pathfind, room_obj.x, room_obj.y, tox, toy, speed, speed, false,
                                             ignwal, run_params);
    }

    // If successful, then start moving
    if (path_result)
    {
        room.Objs[obj_id].moving = add_movelist(std::move(new_mlist));
    }
}

bool Object_RunFrameEvent(ScriptObject *obj, int view, int loop, int frame)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.RunFrameEvent", obj->id))
    {
        return false;
    }
    view--; // convert to internal 0-based view ID
    AssertFrame("Object.RunFrameEvent", room.Objects[obj->id].ScriptName.GetCStr(), view, loop, frame);
    return room.Objs[obj->id].RunFrameEvent(view, loop, frame);
}

void Object_RunInteraction(ScriptObject *obj, int mode)
{
    RunObjectInteraction(obj->id, mode);
}

void *Object_GetGraphicPosition(ScriptObject *obj)
{
    Room &room = Room::Current();
    room.Objs[obj->id].UpdateGraphicSpace();
    std::vector<Point> points;
    room.Objs[obj->id].GetGraphicSpace().GetTransformedCorners(
        points, Size(room.Objs[obj->id].spr_width, room.Objs[obj->id].spr_height));
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void *Object_GetGraphicBoundBox(ScriptObject *obj)
{
    Room &room = Room::Current();
    room.Objs[obj->id].UpdateGraphicSpace();
    std::vector<Point> points;
    room.Objs[obj->id].GetGraphicSpace().GetAABBPoints(points);
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

int GetObjectProperty(int obj_id, const char *property)
{
    Room &room = Room::Current();
    if (!AssertObject("GetObjectProperty", obj_id))
    {
        return 0;
    }
    return get_int_property(room.Objects[obj_id].Properties, room.State->objProps[obj_id], property);
}

void GetObjectPropertyText(int obj_id, const char *property, char *buffer)
{
    Room &room = Room::Current();
    if (!AssertObject("GetObjectPropertyText", obj_id))
    {
        return;
    }
    get_text_property(room.Objects[obj_id].Properties, room.State->objProps[obj_id], property, buffer);
}

int Object_GetProperty(ScriptObject *obj, const char *property)
{
    return GetObjectProperty(obj->id, property);
}

void Object_GetPropertyText(ScriptObject *obj, const char *property, char *buffer)
{
    GetObjectPropertyText(obj->id, property, buffer);
}

const char *Object_GetTextProperty(ScriptObject *obj, const char *property)
{
    Room &room = Room::Current();
    if (!AssertObject("Object.GetTextProperty", obj->id))
    {
        return nullptr;
    }
    return get_text_property_dynamic_string(room.Objects[obj->id].Properties, room.State->objProps[obj->id], property);
}

bool Object_SetProperty(ScriptObject *obj, const char *property, int value)
{
    if (!AssertObject("Object.SetProperty", obj->id))
    {
        return false;
    }
    return set_int_property(Room::Current().State->objProps[obj->id], property, value);
}

bool Object_SetTextProperty(ScriptObject *obj, const char *property, const char *value)
{
    if (!AssertObject("Object.SetTextProperty", obj->id))
    {
        return false;
    }
    return set_text_property(Room::Current().State->objProps[obj->id], property, value);
}

bool Object_GetUseRegionTint(ScriptObject *obj)
{
    return (Room::Current().State->obj[obj->id].flags & OBJF_USEREGIONTINTS) != 0;
}

void Object_SetUseRegionTint(ScriptObject *obj, int yesorno)
{
    Room &room = Room::Current();
    room.Objs[obj->id].flags &= ~OBJF_USEREGIONTINTS;
    if (yesorno)
        room.Objs[obj->id].flags |= OBJF_USEREGIONTINTS;
}

ScriptMotionPath *Object_GetMotionPath(ScriptObject *obj)
{
    RoomObject &room_obj = Room::Current().Objs[obj->id];
    const int mslot = room_obj.moving;
    if (mslot <= 0)
        return nullptr;

    if (room_obj.movelist_handle > 0)
    {
        return static_cast<ScriptMotionPath *>(ccGetObjectAddressFromHandle(room_obj.movelist_handle));
    }

    auto sc_path = ScriptMotionPath::Create(mslot);
    ccAddObjectReference(sc_path.Handle());
    room_obj.movelist_handle = sc_path.Handle();
    return static_cast<ScriptMotionPath *>(sc_path.Obj());
}

void update_object_scale(Game &game, Room &room, int &res_zoom, int &res_width, int &res_height, int objx, int objy,
                         int sprnum, int own_zoom, bool use_region_scaling)
{
    int zoom = own_zoom;
    if (use_region_scaling)
    {
        // Only apply area zoom if we're on a valid area:
        // * either area is > 0, or
        // * area 0 has valid scaling property (<> 0)
        int onarea = get_walkable_area_at_location(room, objx, objy);
        if ((onarea > 0) || (room.WalkAreas[0].ScalingFar != 0))
        {
            zoom = get_area_scaling(room, onarea, objy);
        }
    }

    // safety fixes
    if ((sprnum < 0) || (static_cast<uint32_t>(sprnum) >= game.SpriteInfos.size()) ||
        !game.SpriteInfos[sprnum].IsValid())
        sprnum = 0;
    if (zoom == 0)
        zoom = 100;

    int sprwidth = game.SpriteInfos[sprnum].Width;
    int sprheight = game.SpriteInfos[sprnum].Height;
    if (zoom != 100)
    {
        scale_sprite_size(game, sprnum, zoom, &sprwidth, &sprheight);
    }

    res_zoom = zoom;
    res_width = sprwidth;
    res_height = sprheight;
}

void update_object_scale(Game &game, Room &room, int obj_id)
{
    RoomObject &room_obj = room.Objs[obj_id];
    if (!room_obj.is_enabled())
    {
        return; // not enabled
    }

    int zoom, scale_width, scale_height;
    update_object_scale(game, room, zoom, scale_width, scale_height, room_obj.x, room_obj.y, room_obj.num,
                        room_obj.zoom, (room_obj.flags & OBJF_USEROOMSCALING) != 0);

    // Save calculated properties and recalc GS
    room_obj.zoom = zoom;
    room_obj.spr_width = game.SpriteInfos[room_obj.num].Width;
    room_obj.spr_height = game.SpriteInfos[room_obj.num].Height;
    if (room_obj.is_animating())
    {
        room_obj.frame_xoff = game.Views[room_obj.view].loops[room_obj.loop].frames[room_obj.frame].xoffs;
        room_obj.frame_yoff = game.Views[room_obj.view].loops[room_obj.loop].frames[room_obj.frame].yoffs;
    }
    room_obj.width = scale_width;
    room_obj.height = scale_height;
    room_obj.UpdateGraphicSpace();
}

Rect get_object_blocking_rect(Room &room, int obj_id)
{
    RoomObject *room_obj = &room.Objs[obj_id];

    int width;
    if (room_obj->blocking_width < 1)
    {
        width = room_obj->width - 4;
    }
    else
        width = room_obj->blocking_width;

    int x = room_obj->x + (room_obj->width / 2) - width / 2 + room_obj->blocking_x;

    int y1, y2;
    if (room_obj->blocking_height > 0)
    {
        y1 = room_obj->y - room_obj->blocking_height / 2;
    }
    else
        y1 = room_obj->y - 2;
    if (room_obj->blocking_height > 0)
    {
        y2 = room_obj->y + room_obj->blocking_height / 2;
    }
    else
        y2 = room_obj->y + 3;
    y1 += room_obj->blocking_y;
    y2 += room_obj->blocking_y;

    return RectWH(x, y1, width, y2 - y1);
}

int is_pos_in_sprite(Game &game, int x, int y, int arx, int ary, Bitmap *sprit, int spww, int sphh,
                     Common::SpriteTransformFlags sprite_flags)
{
    if (spww == 0)
        spww = sprit->GetWidth();
    if (sphh == 0)
        sphh = sprit->GetHeight();

    if (!RectWH(arx, ary, spww, sphh).IsInside(x, y))
    {
        return false;
    }

    if (game.options[OPT_PIXPERFECT])
    {
        // if it's transparent, or off the edge of the sprite, ignore
        int xpos = x - arx;
        int ypos = y - ary;

        if (sprite_flags & kSprTf_FlipX)
            xpos = (sprit->GetWidth() - 1) - xpos;
        if (sprite_flags & kSprTf_FlipY)
            ypos = (sprit->GetHeight() - 1) - ypos;

        int gpcol = sprit->GetPixel(xpos, ypos);
        if (gpcol == sprit->GetMaskColor())
            return FALSE;
    }
    return TRUE;
}

int check_click_on_object(int roomx, int roomy, int mood)
{
    int obj_id = GetObjectIDAtRoom(roomx, roomy, kHit_Interactable);
    if (obj_id < 0)
    {
        return 0;
    }
    RunObjectInteraction(obj_id, mood);
    return 1;
}

BlendMode ValidateBlendMode(const char *apiname, const char *objname, int blend_mode)
{
    if ((blend_mode < 0) || (blend_mode >= kNumBlendModes))
    {
        debug_script_warn("!%s (%s): invalid blend mode %d, supported modes are %d - %d", apiname, objname, blend_mode,
                          0, kNumBlendModes - 1);
        return kBlend_Normal;
    }
    return static_cast<BlendMode>(blend_mode);
}

void ValidateAnimParams(const char *apiname, const char *objname, int &repeat, int &direction)
{
    if (direction == FORWARDS)
        direction = kAnimDirForward;
    else if (direction == BACKWARDS)
        direction = kAnimDirBackward;

    if ((repeat < kAnimFlow_First) || (repeat > kAnimFlow_Last))
    {
        debug_script_warn("%s (%s): invalid 'repeat' value %d, will treat as ONCE (0).", apiname, objname, repeat);
        repeat = kAnimFlow_Once;
    }
    if ((direction < kAnimDirForward) || (direction > kAnimDirBackward))
    {
        debug_script_warn("%s (%s): invalid 'direction' value %d, will treat as FORWARDS (0)", apiname, objname,
                          direction);
        direction = kAnimDirForward;
    }
}

void ValidateViewAnimParams(const char *apiname, const char *objname, int &blocking, int &repeat, int &direction)
{
    if (blocking == BLOCKING)
        blocking = 1;
    else if (blocking == IN_BACKGROUND)
        blocking = 0;

    if ((blocking < 0) || (blocking > 1))
    {
        debug_script_warn("%s (%s): invalid 'blocking' value %d, will treat as BLOCKING (1)", apiname, objname,
                          blocking);
        blocking = 1;
    }

    ValidateAnimParams(apiname, objname, repeat, direction);
}

void ValidateViewAnimVLF(const char *apiname, const char *objname, int view, int loop, int &sframe)
{
    Game &game = Game::Current();
    // NOTE: we assume that the view is already in an internal 0-based range.
    // but when printing an error we will use (view + 1) for compliance with the script API.
    AssertLoop(apiname, objname, view, loop);

    if (game.Views[view].loops[loop].numFrames < 1)
        debug_script_warn("%s (%s): view %d loop %d does not have any frames, will use a frame placeholder.", apiname,
                          objname, view + 1, loop);
    else if (sframe < 0 || sframe >= game.Views[view].loops[loop].numFrames)
        debug_script_warn("%s (%s): invalid starting frame number %d for view %d loop %d (range is 0..%d)", apiname,
                          objname, sframe, view + 1, loop, game.Views[view].loops[loop].GetLastFrame());
    // NOTE: there's always frame 0 allocated for safety
    sframe = std::max(0, std::min(sframe, game.Views[view].loops[loop].GetLastFrame()));
}

int SetFirstAnimFrame(int view, int loop, int sframe, AnimFlowDirection dir)
{
    Game &game = Game::Current();
    if (game.Views[view].loops[loop].numFrames <= 1)
        return 0;
    // reverse animation starts at the *previous frame*
    if (dir == kAnimDirBackward)
    {
        sframe--;
        if (sframe < 0)
            sframe = game.Views[view].loops[loop].numFrames - (-sframe);
    }
    return sframe;
}

static void CycleResetToBegin(const ViewStruct *view, uint16_t &loop, uint16_t &frame, bool forward,
                              bool multi_loop_reset)
{
    if (forward)
    {
        if (multi_loop_reset)
        {
            while ((loop > 0) && view->loops[loop - 1].RunNextLoop())
                loop--;
        }
        frame = 0;
    }
    else
    {
        if (multi_loop_reset)
        {
            while ((loop + 1 < view->numLoops) && view->loops[loop].RunNextLoop())
                loop++;
        }
        frame = view->loops[loop].GetLastFrame();
    }
}

static bool CycleNextFrame(const ViewStruct *view, uint16_t &loop, uint16_t &frame, bool forward)
{
    if (forward)
    {
        // If there are still frames available after on current loop, then move 1 frame forward
        if (frame + 1 < view->loops[loop].numFrames)
        {
            frame++;
            return true;
        }

        // If no more frames here, try if this is a multi-loop sequence,
        // and find next valid loop (with frames)
        for (int try_loop = loop; (try_loop + 1 < view->numLoops) && view->loops[try_loop].RunNextLoop();)
        {
            if (view->loops[++try_loop].numFrames > 0)
            {
                loop = try_loop;
                frame = 0;
                return true;
            }
        }
        // End of the animation sequence
        return false;
    }
    else
    {
        // If there are still frames available before on current loop, then move 1 frame back
        if (frame > 0)
        {
            frame--;
            return true;
        }

        // If no more frames here, try if this is a multi-loop sequence,
        // and find *previous* valid loop (with frames)
        for (int try_loop = loop; (try_loop > 0) && view->loops[try_loop - 1].RunNextLoop();)
        {
            if (view->loops[--try_loop].numFrames > 0)
            {
                loop = try_loop;
                frame = view->loops[try_loop].GetLastFrame();
                return true;
            }
        }
        // End of the animation sequence
        return false;
    }
}

// General view animation algorithm: find next loop and frame, depending on anim settings
bool CycleViewAnim(Game &game, int view, uint16_t &o_loop, uint16_t &o_frame, AnimFlowParams &anim_params)
{
    const ViewStruct *aview = &game.Views[view];
    uint16_t loop = o_loop;
    uint16_t frame = o_frame;
    const bool forward = anim_params.IsForward();
    const AnimFlowStyle flow = anim_params.Flow;
    // Allow multi-loop repeat depending on a game option
    const bool multi_loop_repeat = (game.Play.no_multiloop_repeat == 0);

    bool is_done = false;
    // Test next frame (fw or bw), if we see that it is past the frame range,
    // then decide to either stop anim or repeat, depending on flow style
    if (!CycleNextFrame(aview, loop, frame, forward))
    {
        // Reached the end of the animation sequence;
        // Decide whether to continue (and how) or stop, depending on the animation flow
        switch (flow)
        {
        case kAnimFlow_OnceAndReset:
            // Reset to begin and stop
            CycleResetToBegin(aview, loop, frame, forward, multi_loop_repeat);
            is_done = true;
            break;
        case kAnimFlow_OnceAndBack:
            // Test if we've already back, in which case stop
            if (anim_params.Direction != anim_params.InitialDirection)
            {
                is_done = true;
                break;
            }
            // Backtrack one frame back (if possible)
            CycleNextFrame(aview, loop, frame, !forward);
            // Change direction and *continue*
            anim_params.Direction = forward ? kAnimDirBackward : kAnimDirForward;
            break;
        case kAnimFlow_Repeat:
            // Reset to begin and *continue*
            CycleResetToBegin(aview, loop, frame, forward, multi_loop_repeat);
            break;
        case kAnimFlow_RepeatAlternate:
            // Backtrack one frame back (if possible)
            CycleNextFrame(aview, loop, frame, !forward);
            // Change direction and *continue*
            anim_params.Direction = forward ? kAnimDirBackward : kAnimDirForward;
            break;
        default:
            // Just stop at the current frame
            is_done = true;
            break;
        }
    }

    // Update object values
    o_loop = loop;
    o_frame = frame;
    return !is_done; // have we finished animating?
}

void ValidateMoveParams(const char *apiname, const char *objname, int &blocking, int &ignwal)
{
    if (blocking == BLOCKING)
        blocking = 1;
    else if (blocking == IN_BACKGROUND)
        blocking = 0;

    if (ignwal == ANYWHERE)
        ignwal = 1;
    else if (ignwal == WALKABLE_AREAS)
        ignwal = 0;

    if ((blocking < 0) || (blocking > 1))
    {
        debug_script_warn("%s (%s): invalid 'blocking' value %d, will treat as BLOCKING (1)", apiname, objname,
                          blocking);
        blocking = 1;
    }
    if ((ignwal < 0) || (ignwal > 1))
    {
        debug_script_warn("%s (%s): invalid 'walk where' value %d, will treat as ANYWHERE (1)", apiname, objname,
                          ignwal);
        ignwal = 1;
    }
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "game/dynobj/script_string.h"
#include "script/script_runtime.h"


ScriptObject *Object_GetByName(const char *name)
{
    return static_cast<ScriptObject *>(ccGetScriptObjectAddress(name, ccDynamicObject.GetType()));
}
