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

#include "ac/display.h"
#include "ac/dynobj/scriptoverlay.h"
#include "ac/screenoverlay.h"
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
