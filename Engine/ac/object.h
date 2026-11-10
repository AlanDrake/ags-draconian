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
// Gets the ID of the object at the given room coordinates, or -1 if there is none. Only objects that are displayed
// (enabled and visible) count, and with kHit_Interactable in hit_options only clickable ones; with the pixel-perfect
// game option a transparent pixel of the sprite is a miss. If several objects are there, the one with the highest
// baseline wins, the last by ID on a tie. The winner's baseline (-1 if none) is also left in the global obj_lowest_yp,
// which GetLocationTypeImpl (game.cpp) compares with the character's and the walk-behind's.
int GetObjectIDAtRoom(int roomx, int roomy, int hit_options);
// Gets the object's current image: the last cached (transformed) one, which exists in software render mode only
// and when always_original is not set, else the source sprite; is_original, if given, tells which one it is.
AGS::Common::Bitmap *GetObjectImage(int obj_id, bool always_original = true, bool *is_original = nullptr);
// Gets current source image (untransformed) for the room object
AGS::Common::Bitmap *GetObjectSourceImage(int obj_id);

// **** OBJECT: FUNCTIONS ****
//
// An Object_<Name> function implements the script function Object.<Name> of the same name, which scripts reach through
// the Sc_Object_<Name> wrapper registered in RegisterObjectAPI (object.cpp); a pair Object_Get<Name> and
// Object_Set<Name> implements the script property Object.<Name> (get_<Name> and set_<Name>). obj is the script object
// the function is called on. A comment tells where the script name differs. Most of them warn about an invalid object
// ID (AssertObject) and then do nothing: they return 0, false or nullptr.

// Object.IsCollidingWithObject: 1 if the rectangles of the two objects overlap, 0 if they do not or either is disabled.
int Object_IsCollidingWithObject(ScriptObject *obj, ScriptObject *other);
// Object.Tint: sets an explicit tint, which replaces a light level; red, green, blue, saturation and luminance
// are all 0-255, otherwise a warning is posted and the tint is not changed.
void Object_Tint(ScriptObject *obj, int red, int green, int blue, int saturation, int luminance);
// Object.RemoveTint: removes the explicit tint or light level; warns if there is none.
void Object_RemoveTint(ScriptObject *obj);
// Object.SetView: the view is 1-based, an invalid one or one without loops is a script error. A loop or frame out of
// range is warned about and replaced by 0; the frame's
// sprite becomes the object's and an animation in progress is cancelled. A view, loop or frame that does not fit
// 16 bits resets the object to sprite 0 without a view.
void Object_SetView(ScriptObject *obj, int view, int loop, int frame);
// Object.Opacity: alpha is 0 (invisible) to 255 (fully solid); any other value is a script error that ends the game.
void Object_SetOpacity(ScriptObject *obj, int alpha);
int Object_GetOpacity(ScriptObject *obj);
// Object.Baseline: 0 or less, which is what the getter returns for it, means no explicit baseline:
// the object's Y is used then (RoomObject::get_baseline).
void Object_SetBaseline(ScriptObject *obj, int baseline);
int Object_GetBaseline(ScriptObject *obj);
// Object.Animate with every argument; Animate5 and Animate6 (not declared here) supply sframe and volume by default.
// Warns and does nothing if the object is disabled or has no view; an invalid loop is a script error and a start
// frame out of range is clamped. Volume (0-100) is the volume of the frame sounds. With blocking set, waits for the
// animation to end.
void Object_Animate(ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction, int sframe = 0,
                    int volume = 100);
// Object.StopAnimating: stops the animation, the object keeps its current frame.
void Object_StopAnimating(ScriptObject *obj);
// Object.StopMoving: stops the object's move.
void Object_StopMoving(ScriptObject *obj);
void Object_SetVisible(ScriptObject *obj, bool on);
// Object.View is 1-based and 0 if the object has no view; Loop and Frame are 0 then too.
int Object_GetView(ScriptObject *obj);
int Object_GetLoop(ScriptObject *obj);
int Object_GetFrame(ScriptObject *obj);
bool Object_GetVisible(ScriptObject *obj);
// Object.Graphic (set): changes the sprite and resets the view, loop, frame and animation.
// A sprite beyond 16 bits is warned about and replaced by 0.
void Object_SetGraphic(ScriptObject *obj, int slot);
int Object_GetGraphic(ScriptObject *obj);
int Object_GetX(ScriptObject *obj);
int Object_GetY(ScriptObject *obj);
// Object.Animating and Moving are 1 or 0.
int Object_GetAnimating(ScriptObject *obj);
int Object_GetMoving(ScriptObject *obj);
// Object.SetPosition, and Object.X and Y (set): refused with a warning while the object is moving.
void Object_SetPosition(ScriptObject *obj, int x, int y);
void Object_SetX(ScriptObject *obj, int x);
void Object_SetY(ScriptObject *obj, int y);
// Object.DisplayName (get), also known to scripts as the obsolete Object.Name; returns a new script string.
const char *Object_GetName_New(ScriptObject *obj);
// Tells if the object has an event handler for the cursor mode (mood) or for "any click", without running it:
// runs RunObjectInteraction in the check-only mode.
bool Object_IsInteractionAvailable(ScriptObject *obj, int mood);
// Object.Move: ignwal 1 (ANYWHERE) goes straight, 0 (WALKABLE_AREAS) follows a route over the walkable areas.
// Warns and does nothing if the object is disabled; with blocking set, waits until the object stops moving.
void Object_Move(ScriptObject *obj, int x, int y, int speed, int blocking, int ignwal);
// Object.Clickable: an object that is not clickable is skipped by GetObjectIDAtRoom with kHit_Interactable.
void Object_SetClickable(ScriptObject *obj, int clickable);
int Object_GetClickable(ScriptObject *obj);
// Object.Solid: an enabled solid object cuts its blocking rectangle (get_object_blocking_rect) out of the
// walkable areas.
void Object_SetSolid(ScriptObject *obj, int solid);
int Object_GetSolid(ScriptObject *obj);
// Object.BlockingWidth and BlockingHeight: the size of the blocking rectangle; 0 or less means the default
// (see get_object_blocking_rect).
void Object_SetBlockingWidth(ScriptObject *obj, int blocking_width);
int Object_GetBlockingWidth(ScriptObject *obj);
void Object_SetBlockingHeight(ScriptObject *obj, int blocking_height);
int Object_GetBlockingHeight(ScriptObject *obj);
int Object_GetID(ScriptObject *obj);
// Object.RunInteraction: runs the object's event for the cursor mode (mood) from the Interactions table,
// then the "any click" event, unless the first one changed the game state.
void Object_RunInteraction(ScriptObject *obj, int mode);

// Object.GetProperty, GetPropertyText and GetTextProperty: read a custom property of the object;
// GetPropertyText copies it into bufer, GetTextProperty returns a new script string.
int Object_GetProperty(ScriptObject *obj, const char *property);
void Object_GetPropertyText(ScriptObject *obj, const char *property, char *buffer);
const char *Object_GetTextProperty(ScriptObject *obj, const char *property);

// **** OBJECT: ENGINE FUNCTIONS ****

// AreThingsOverlapping is also a script function. A thing is a character ID, or a room object ID plus 1000
// (OVERLAPPING_OBJECT in object.cpp). Returns 0 if the things do not overlap or one is not there (a character in
// another room, a disabled object), else the smaller of the horizontal and vertical overlap in pixels;
// an invalid thing is a script error.
int AreThingsOverlapping(int thing1, int thing2);

// Deduces room object's scale, accounting for both manual scaling and the room region effects;
// calculates resulting sprite size. Does nothing to a disabled object.
void update_object_scale(int obj_id);
// Deduces arbitrary object's scale, accounting for both manual scaling and the room region effects
void update_object_scale(int &res_zoom, int &res_width, int &res_height, int objx, int objy, int sprnum, int own_zoom,
                         bool use_region_scaling);
// Gets the rectangle in room coordinates that the object blocks for walking when it is solid:
// blocking_width wide (the object's width minus 4 if unset) and blocking_height tall (5 pixels around its Y
// if unset), centred on the object's X, then shifted by blocking_x and blocking_y.
Rect get_object_blocking_rect(int obj_id);
// Tells if the point x,y hits the sprite sprit: it is inside the area arx,ary,spww,sphh and, if the pixel-perfect
// game option is on, the sprite's pixel there is not transparent (sprite_flags mirror the pixel lookup).
// x,y and arx,ary must be in the same space; the callers pass a point already converted to the object's local
// space, with arx = ary = 0. spww and sphh of 0 mean the bitmap's size.
// Returns TRUE or FALSE.
int is_pos_in_sprite(int x, int y, int arx, int ary, AGS::Common::Bitmap *sprit, int spww, int sphh,
                     AGS::Common::SpriteTransformFlags sprite_flags);
// Runs the interaction of the clickable object at the room coordinates roomx, roomy for the cursor mode (mood);
// returns 1 if there is such an object, whether or not it has a handler, 0 if there is none.
int check_click_on_object(int roomx, int roomy, int mood);

// Tests if the blend mode is in valid range, post a warning and fixes to default if not
AGS::Common::BlendMode ValidateBlendMode(const char *apiname, const char *objname, int blend_mode);
// Shared functions that prepare or advance the view animation;
// used by characters, room objects, buttons and overlays (ValidateAnimParams by motion paths too).
// TODO: pick out some kind of "animation" struct
// Tests if the standard animate parameters are in valid range, if not then clamps them and
// reports a script warning. FORWARDS and BACKWARDS are converted to the animation direction values.
void ValidateAnimParams(const char *apiname, const char *objname, int &repeat, int &direction);
// Like ValidateAnimParams, and also tests blocking: BLOCKING and IN_BACKGROUND are converted to 1 and 0.
void ValidateViewAnimParams(const char *apiname, const char *objname, int &blocking, int &repeat, int &direction);
// Tests if the view, loop, frame animate params are in valid range,
// errors in case of out-of-range view or loop, but clamps a frame to a range.
// NOTE: assumes view is already in an internal 0-based range.
void ValidateViewAnimVLF(const char *apiname, const char *objname, int view, int loop, int &sframe);
// Calculates the first shown frame for a view animation, depending on parameters:
// a loop of 0 or 1 frames always starts at frame 0, a backward animation starts one frame before sframe
// (wrapping to the last one).
int SetFirstAnimFrame(int view, int loop, int sframe, AnimFlowDirection dir);
// General view animation algorithm: find next loop and frame, depending on anim settings;
// o_loop and o_frame are the current values, passed by reference and updated;
// AnimationFlowParams may be modified if direction changes, according to flow style;
// returns whether the animation should continue.
bool CycleViewAnim(int view, uint16_t &o_loop, uint16_t &o_frame, AnimFlowParams &anim_params);

// Tests if the standard move/walk parameters are in valid range, if not then clamps them and
// reports a script warning. BLOCKING, IN_BACKGROUND, ANYWHERE and WALKABLE_AREAS are converted to 1 or 0.
void ValidateMoveParams(const char *apiname, const char *objname, int &blocking, int &ignwal);
