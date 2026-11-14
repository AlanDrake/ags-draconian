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
// Overlays: images and texts drawn over the room or the screen, such as speech and message boxes and
// the script's Overlay objects; the animations of overlays; the Overlay and AnimatedOverlay script API.
//
//=============================================================================
#pragma once
#include <vector>

#include "ac/dynobj/scriptoverlay.h"
#include "game/dialog/display.h"
#include "game/render/screen_overlay.h"
#include "util/geometry.h"
#include "util/indexedobjectpool.h"

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// Gets an actual ScreenOverlay object given its index;
// validate object, send an error to the logging system on failure
ScreenOverlay *GetOverlayValidate(const char *apiname, int over_id);
// Gets an actual ScreenOverlay object from its ScriptOverlay reference;
// validate object, send an error to the logging system on failure
inline ScreenOverlay *GetOverlayValidate(const char *apiname, ScriptOverlay *script_over)
{
    return GetOverlayValidate(apiname, script_over->GetOverlayID());
}

// Script API: the Overlay_* functions below are the Overlay struct's members and properties
// (the rest of them are registered in overlay.cpp only).
// Removes the overlay and invalidates the script object
void Overlay_Remove(ScriptOverlay *script_over);
// Redraws the overlay's text with the given width, font and color, keeping the overlay's position;
// a negative width means automatic width
void Overlay_SetText(ScriptOverlay *script_over, int width, int font, int text_color, const char *text);
// Redraws the overlay's text at a new fixed position (x, y);
// a negative width means automatic width, a width below 8 is replaced with half of the UI viewport's
void Overlay_SetText(ScreenOverlay &over, int x, int y, int width, int font, int text_color, const char *text);
// Overlay.X and Overlay.Y; a getter updates the position of an overlay that follows a character first,
// a setter turns the overlay into a fixed-position one
int Overlay_GetX(ScriptOverlay *script_over);
void Overlay_SetX(ScriptOverlay *script_over, int new_x);
int Overlay_GetY(ScriptOverlay *script_over);
void Overlay_SetY(ScriptOverlay *script_over, int new_y);
// Overlay.Valid: tells if the overlay still exists
int Overlay_GetValid(ScriptOverlay *script_over);
// Overlay.CreateGraphical: creates an overlay in the screen layer showing the sprite;
// returns nullptr if the sprite does not exist
ScriptOverlay *Overlay_CreateGraphical(int x, int y, int slot, bool clone);
// Overlay.CreateTextual: creates an overlay in the screen layer showing the text;
// a negative width means automatic width, a negative x or y centres the text on that axis
ScriptOverlay *Overlay_CreateTextual(int x, int y, int width, int font, int text_color, const char *text);
// Creates a graphical overlay with a custom ID, in the room or screen layer, and returns it,
// or nullptr if the sprite does not exist. When "clone" is set, a dynamic sprite is copied
// into the overlay's own image; a normal sprite is never cloned
ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool clone = false);
// Creates a text overlay of the given type (an ID, or OVER_CUSTOM for a free one), in the room
// or screen layer, and returns it; the text is displayed as Display() does, and a negative
// x or y centres the text on that axis. "speech_for_char" is the character whom the overlay
// is linked to (autopositioned above), -1 for none
ScreenOverlay *Overlay_CreateTextCore(bool room_layer, int x, int y, int width, int font, int text_color,
                                      const char *text, int over_type, DisplayTextStyle style,
                                      DisplayTextShrink allow_shrink, int speech_for_char = -1);

// Returns the overlay with the given ID, or nullptr if there is no such overlay in use
ScreenOverlay *get_overlay(int type);
// Autopositions overlay, that is linked to a character: above the character's head, kept inside
// the UI viewport; if the character is in another room, centres the overlay on the screen
void autoposition_overlay(ScreenOverlay &over);
// Creates a new overlay and returns its ID; "type" is either a fixed ID (OVER_*, the slot with that
// ID is used), or OVER_CUSTOM, then a free ID is taken. The overlay shows either a sprite or
// an image, which is moved into it, shifted by (offset_x, offset_y) from the overlay's position
size_t add_screen_overlay(bool room_layer, int x, int y, int type, int sprite_num);
size_t add_screen_overlay(bool room_layer, int x, int y, int type, std::unique_ptr<AGS::Common::Bitmap> image);
size_t add_screen_overlay(bool room_layer, int x, int y, int type, std::unique_ptr<AGS::Common::Bitmap> image,
                          int offset_x, int offset_y);
// Removes the overlay with the given ID, with its animation and script object;
// does nothing if there is no such overlay
void remove_screen_overlay(int type);
// Removes all overlays and all overlay animations
void remove_all_overlays();
// Restores overlays, e.g. after restoring a game save
void restore_overlays();
// Returns a ref to overlays list, useful for iterating over them
// TODO: this should be a CONST ref (if any at all), strictly for reading, but external code writes
// through it: game.cpp clears it, savegame_components.cpp restores it, update.cpp updates the
// timeouts, game_run.cpp and character.cpp call non-const members
IndexedObjectPool<ScreenOverlay, int32_t> &get_overlays();

// Starts tracking an animation for the overlay with the given ID; does not start animating
void CreateAnimatedOverlay(int over_id, bool pause_with_game);
// Stores the animation state, replacing the one for the same overlay if there is one;
// returns its index in GetAnimatedOverlays(), or SIZE_MAX if the overlay ID is invalid
size_t AddAnimatedOverlay(AnimatedOverlay &&aover);
// Starts animating the overlay with the view, loop and frame (0-based view ID), and shows the first frame;
// does nothing if the overlay has no animation record
void BeginAnimateOverlay(int over_id, int view, int loop, int frame, const ViewAnimateParams &params);
// Returns the animations list; records of the removed animations stay in it as empty ones
const std::vector<AnimatedOverlay> &GetAnimatedOverlays();
// Tells if the overlay with the given ID is animating now
bool IsOverlayAnimating(int over_id);
// Returns the overlay's animation, or nullptr if it has none
AnimatedOverlay *GetOverlayAnimation(int over_id);
// Advances the running animations by one step (those that pause with the game are skipped while the game
// is paused), and stops the finished ones
void UpdateOverlayAnimations();
// Removes the animation of the overlay with the given ID; does nothing if it has none
void RemoveAnimatedOverlay(int over_id);
void RemoveAllAnimatedOverlays();

class ScriptShaderInstance;

// Script API: Overlay::get_AutoSize
bool Overlay_GetAutoSize(ScriptOverlay *script_over);

// Script API: Overlay::set_AutoSize
void Overlay_SetAutoSize(ScriptOverlay *script_over, bool auto_size);

// Script API: Overlay::get_Graphic
int Overlay_GetGraphic(ScriptOverlay *script_over);

// Script API: Overlay::set_Graphic
void Overlay_SetGraphic(ScriptOverlay *script_over, int slot);

// Script API: Overlay::get_InRoom
bool Overlay_InRoom(ScriptOverlay *script_over);

// Script API: Overlay::get_Width
int Overlay_GetWidth(ScriptOverlay *script_over);

// Script API: Overlay::get_Height
int Overlay_GetHeight(ScriptOverlay *script_over);

// Script API: Overlay::get_GraphicWidth
int Overlay_GetGraphicWidth(ScriptOverlay *script_over);

// Script API: Overlay::get_GraphicHeight
int Overlay_GetGraphicHeight(ScriptOverlay *script_over);

// Script API: Overlay::set_Width
void Overlay_SetWidth(ScriptOverlay *script_over, int width);

// Script API: Overlay::set_Height
void Overlay_SetHeight(ScriptOverlay *script_over, int height);

// Script API: Overlay::get_Flip
int Overlay_GetFlip(ScriptOverlay *script_over);

// Script API: Overlay::set_Flip
void Overlay_SetFlip(ScriptOverlay *script_over, int flip);

// Script API: Overlay::get_GraphicAnchorX
float Overlay_GetGraphicAnchorX(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicAnchorX
void Overlay_SetGraphicAnchorX(ScriptOverlay *script_over, float x);

// Script API: Overlay::get_GraphicAnchorY
float Overlay_GetGraphicAnchorY(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicAnchorY
void Overlay_SetGraphicAnchorY(ScriptOverlay *script_over, float y);

// Script API: Overlay::get_GraphicOffsetX
int Overlay_GetGraphicOffsetX(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicOffsetX
void Overlay_SetGraphicOffsetX(ScriptOverlay *script_over, int x);

// Script API: Overlay::get_GraphicOffsetY
int Overlay_GetGraphicOffsetY(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicOffsetY
void Overlay_SetGraphicOffsetY(ScriptOverlay *script_over, int y);

// Script API: Overlay::GetGraphicPosition^0
void *Overlay_GetGraphicPosition(ScriptOverlay *script_over);

// Script API: Overlay::GetGraphicBoundBox^0
void *Overlay_GetGraphicBoundBox(ScriptOverlay *script_over);

// Script API: Overlay::CreateGraphical^5
ScriptOverlay *Overlay_CreateGraphical5(int x, int y, int slot, bool /* dummy */, bool clone);

// Script API: Overlay::CreateRoomGraphical^4
ScriptOverlay *Overlay_CreateRoomGraphical(int x, int y, int slot, bool clone);

// Script API: Overlay::CreateRoomGraphical^5
ScriptOverlay *Overlay_CreateRoomGraphical5(int x, int y, int slot, bool /*dummy*/, bool clone);

// Script API: Overlay::CreateRoomTextual^106
ScriptOverlay *Overlay_CreateRoomTextual(int x, int y, int width, int font, int text_color, const char *text);

// Script API: Overlay::get_Text
const char *Overlay_GetTextProperty(ScriptOverlay *script_over);

// Script API: Overlay::set_Text
void Overlay_SetTextProperty(ScriptOverlay *script_over, const char *text);

// Script API: Overlay::get_BlendMode
int Overlay_GetBlendMode(ScriptOverlay *script_over);

// Script API: Overlay::set_BlendMode
void Overlay_SetBlendMode(ScriptOverlay *script_over, int blend_mode);

// Script API: Overlay::get_Shader
ScriptShaderInstance *Overlay_GetShader(ScriptOverlay *script_over);

// Script API: Overlay::set_Shader
void Overlay_SetShader(ScriptOverlay *script_over, ScriptShaderInstance *shader_inst);

// Script API: Overlay::get_Opacity
int Overlay_GetOpacity(ScriptOverlay *script_over);

// Script API: Overlay::set_Opacity
void Overlay_SetOpacity(ScriptOverlay *script_over, int alpha);

// Script API: Overlay::get_Transparency
int Overlay_GetTransparency(ScriptOverlay *script_over);

// Script API: Overlay::set_Transparency
void Overlay_SetTransparency(ScriptOverlay *script_over, int trans);

// Script API: Overlay::get_Visible
bool Overlay_GetVisible(ScriptOverlay *script_over);

// Script API: Overlay::set_Visible
void Overlay_SetVisible(ScriptOverlay *script_over, bool visible);

// Script API: Overlay::get_Rotation
float Overlay_GetRotation(ScriptOverlay *script_over);

// Script API: Overlay::set_Rotation
void Overlay_SetRotation(ScriptOverlay *script_over, float degrees);

// Script API: Overlay::get_GraphicPivotX
float Overlay_GetGraphicPivotX(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicPivotX
void Overlay_SetGraphicPivotX(ScriptOverlay *script_over, float pivot_x);

// Script API: Overlay::get_GraphicPivotY
float Overlay_GetGraphicPivotY(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicPivotY
void Overlay_SetGraphicPivotY(ScriptOverlay *script_over, float pivot_y);

// Script API: Overlay::get_GraphicPivotOffsetX
int Overlay_GetGraphicPivotOffsetX(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicPivotOffsetX
void Overlay_SetGraphicPivotOffsetX(ScriptOverlay *script_over, int pivot_offset_x);

// Script API: Overlay::get_GraphicPivotOffsetY
int Overlay_GetGraphicPivotOffsetY(ScriptOverlay *script_over);

// Script API: Overlay::set_GraphicPivotOffsetY
void Overlay_SetGraphicPivotOffsetY(ScriptOverlay *script_over, int pivot_offset_y);

// Script API: Overlay::get_ZOrder
int Overlay_GetZOrder(ScriptOverlay *script_over);

// Script API: Overlay::set_ZOrder
void Overlay_SetZOrder(ScriptOverlay *script_over, int zorder);

// Script API: Overlay::SetPosition^4
void Overlay_SetPosition(ScriptOverlay *script_over, int x, int y, int width, int height);

// Script API: Overlay::SetSize^2
void Overlay_SetSize(ScriptOverlay *script_over, int width, int height);

// Script API: Overlay::SetScale^2
void Overlay_SetScale(ScriptOverlay *script_over, float scale_x, float scale_y);

// Script API: Overlay::get_ScaleX
float Overlay_GetScaleX(ScriptOverlay *script_over);

// Script API: Overlay::set_ScaleX
void Overlay_SetScaleX(ScriptOverlay *script_over, float scale_x);

// Script API: Overlay::get_ScaleY
float Overlay_GetScaleY(ScriptOverlay *script_over);

// Script API: Overlay::set_ScaleY
void Overlay_SetScaleY(ScriptOverlay *script_over, float scale_y);

// Script API: Overlay::Tint^5
void Overlay_Tint(ScriptOverlay *script_over, int red, int green, int blue, int opacity, int luminance);

// Script API: Overlay::SetLightLevel^1
void Overlay_SetLightLevel(ScriptOverlay *script_over, int light_level);

// Script API: Overlay::RemoveTint^0
void Overlay_RemoveTint(ScriptOverlay *script_over);

// Script API: Overlay::get_HasLightLevel
bool Overlay_GetHasLight(ScriptOverlay *script_over);

// Script API: Overlay::get_HasTint
bool Overlay_GetHasTint(ScriptOverlay *script_over);

// Script API: Overlay::get_LightLevel
int Overlay_GetLightLevel(ScriptOverlay *script_over);

// Script API: Overlay::get_TintRed
int Overlay_GetTintRed(ScriptOverlay *script_over);

// Script API: Overlay::get_TintGreen
int Overlay_GetTintGreen(ScriptOverlay *script_over);

// Script API: Overlay::get_TintBlue
int Overlay_GetTintBlue(ScriptOverlay *script_over);

// Script API: Overlay::get_TintSaturation
int Overlay_GetTintSaturation(ScriptOverlay *script_over);

// Script API: Overlay::get_TintLuminance
int Overlay_GetTintLuminance(ScriptOverlay *script_over);

// Script API: AnimatedOverlay::CreateAnimated
ScriptAnimatedOverlay *AnimatedOverlay_CreateAnimated(int x, int y, int slot, bool pause_with_game);

// Script API: AnimatedOverlay::CreateRoomAnimated
ScriptAnimatedOverlay *AnimatedOverlay_CreateRoomAnimated(int x, int y, int slot, bool pause_with_game);

// Script API: AnimatedOverlay::Animate
void AnimatedOverlay_Animate(ScriptAnimatedOverlay *script_over, int view, int loop, int speed, int repeat,
                             int blocking, int direction, int start_frame, int volume);

// Script API: AnimatedOverlay::get_Animating
bool AnimatedOverlay_GetAnimating(ScriptAnimatedOverlay *script_over);

// Script API: AnimatedOverlay::get_Frame
int AnimatedOverlay_GetFrame(ScriptAnimatedOverlay *script_over);

// Script API: AnimatedOverlay::get_Loop
int AnimatedOverlay_GetLoop(ScriptAnimatedOverlay *script_over);

// Script API: AnimatedOverlay::get_PauseWithGame
bool AnimatedOverlay_GetPauseWithGame(ScriptAnimatedOverlay *script_over);

// Script API: AnimatedOverlay::get_View
int AnimatedOverlay_GetView(ScriptAnimatedOverlay *script_over);
