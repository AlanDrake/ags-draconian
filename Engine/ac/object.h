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
//
// [IKM] 2012-06-25: This bugs me that type is called 'Object'; in modern
// world of programming 'object' is usually a base class; should not we
// rename this to RoomObject one day?
//=============================================================================
#pragma once

#include "ac/common_defines.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/roomobject.h"
#include "gfx/gfx_def.h"

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// Tells if the object ID is valid (in the current room)
bool IsValidObject(int obj_id);
// Asserts the object ID is valid in the current room,
// if not then prints a warning to the log; returns assertion result
bool AssertObject(const char *apiname, int obj_id);
// Get object at the given room coordinates
int GetObjectIDAtRoom(int roomx, int roomy, int hit_options);
// Gets last cached object's image; this may be a source image if there was no transformation
AGS::Common::Bitmap *GetObjectImage(int obj, bool always_original = true, bool *is_original = nullptr);
// Gets current source image (untransformed) for the room object
AGS::Common::Bitmap *GetObjectSourceImage(int obj);
int Object_IsCollidingWithObject(ScriptObject *obj, ScriptObject *other);
void Object_Tint(ScriptObject *obj, int red, int green, int blue, int saturation, int luminance);
void Object_RemoveTint(ScriptObject *obj);
void Object_SetView(ScriptObject *obj, int view, int loop, int frame);
void Object_SetOpacity(ScriptObject *obj, int alpha);
int Object_GetOpacity(ScriptObject *obj);
void Object_SetBaseline(ScriptObject *obj, int basel);
int Object_GetBaseline(ScriptObject *obj);
void Object_Animate(ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction, int sframe = 0,
                    int volume = 100);
void Object_StopAnimating(ScriptObject *obj);
void Object_StopMoving(ScriptObject *obj);
void Object_SetVisible(ScriptObject *obj, bool on);
int Object_GetView(ScriptObject *obj);
int Object_GetLoop(ScriptObject *obj);
int Object_GetFrame(ScriptObject *obj);
bool Object_GetVisible(ScriptObject *obj);
void Object_SetGraphic(ScriptObject *obj, int slott);
int Object_GetGraphic(ScriptObject *obj);
int Object_GetX(ScriptObject *obj);
int Object_GetY(ScriptObject *obj);
int Object_GetAnimating(ScriptObject *obj);
int Object_GetMoving(ScriptObject *obj);
void Object_SetPosition(ScriptObject *obj, int x, int y);
void Object_SetX(ScriptObject *obj, int x);
void Object_SetY(ScriptObject *obj, int y);
const char *Object_GetName_New(ScriptObject *obj);
bool Object_IsInteractionAvailable(ScriptObject *obj, int mood);
void Object_Move(ScriptObject *obj, int x, int y, int speed, int blocking, int ignwal);
void Object_SetClickable(ScriptObject *obj, int clik);
int Object_GetClickable(ScriptObject *obj);
void Object_SetSolid(ScriptObject *obj, int solid);
int Object_GetSolid(ScriptObject *obj);
void Object_SetBlockingWidth(ScriptObject *obj, int bwid);
int Object_GetBlockingWidth(ScriptObject *obj);
void Object_SetBlockingHeight(ScriptObject *obj, int bhit);
int Object_GetBlockingHeight(ScriptObject *obj);
int Object_GetID(ScriptObject *obj);
void Object_RunInteraction(ScriptObject *obj, int mode);

int Object_GetProperty(ScriptObject *obj, const char *property);
void Object_GetPropertyText(ScriptObject *obj, const char *property, char *bufer);
const char *Object_GetTextProperty(ScriptObject *obj, const char *property);

int AreThingsOverlapping(int thing1, int thing2);

// Deduces room object's scale, accounting for both manual scaling and the room region effects;
// calculates resulting sprite size.
void update_object_scale(int objid);
// Deduces arbitrary object's scale, accounting for both manual scaling and the room region effects
void update_object_scale(int &res_zoom, int &res_width, int &res_height, int objx, int objy, int sprnum, int own_zoom,
                         bool use_region_scaling);
Rect get_object_blocking_rect(int objid);
// x,y is the position in room co-ordinates that we are checking
// arx,ary,spww,sphh are the sprite's bounding box (including sprite scaling);
// bitmap_original tells whether bitmap is an original sprite, or transformed version
int is_pos_in_sprite(int x, int y, int arx, int ary, AGS::Common::Bitmap *sprit, int spww, int sphh,
                     AGS::Common::SpriteTransformFlags sprite_flags);
// X and Y co-ordinates must be in native format
// X and Y are ROOM coordinates
int check_click_on_object(int roomx, int roomy, int mood);

// Tests if the blend mode is in valid range, post a warning and fixes to default if not
AGS::Common::BlendMode ValidateBlendMode(const char *apiname, const char *objname, int blend_mode);
// Shared functions that prepare or advance the view animation;
// used by characters, room objects and buttons.
// TODO: pick out some kind of "animation" struct
// Tests if the standard animate parameters are in valid range, if not then clamps them and
// reports a script warning.
void ValidateAnimParams(const char *apiname, const char *objname, int &repeat, int &direction);
void ValidateViewAnimParams(const char *apiname, const char *objname, int &blocking, int &repeat, int &direction);
// Tests if the view, loop, frame animate params are in valid range,
// errors in case of out-of-range view or loop, but clamps a frame to a range.
// NOTE: assumes view is already in an internal 0-based range.
void ValidateViewAnimVLF(const char *apiname, const char *objname, int view, int loop, int &sframe);
// Calculates the first shown frame for a view animation, depending on parameters.
int SetFirstAnimFrame(int view, int loop, int sframe, AnimFlowDirection dir);
// General view animation algorithm: find next loop and frame, depending on anim settings;
// loop and frame values are passed by reference and will be updated;
// AnimationFlowParams may be modified if direction changes, according to flow style;
// returns whether the animation should continue.
bool CycleViewAnim(int view, uint16_t &o_loop, uint16_t &o_frame, AnimFlowParams &anim_params);

// Tests if the standard move/walk parameters are in valid range, if not then clamps them and
// reports a script warning.
void ValidateMoveParams(const char *apiname, const char *objname, int &blocking, int &ignwal);
