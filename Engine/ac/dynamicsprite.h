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

#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/dynobj/scriptdynamicsprite.h"

enum ScriptColorFormat
{
    kScColorFmt_Default = 0,
    kScColorFmt_8bit = 8
};

// Validates color format requested by user script, and returns a compatible color depth value
int ValidateColorFormat(const char *api_name, ScriptColorFormat color_format);

// Script API functions of DynamicSprite. Resize, Flip, CopyTransparencyMask, ChangeCanvasSize, Crop, Rotate,
// Tint, SaveToFile and GetGraphic quit the script if the sprite was deleted.
// Frees the sprite, invalidates its drawing surfaces, and resets the object's slot to 0.
void DynamicSprite_Delete(ScriptDynamicSprite *dynsprite);
// Returns the sprite's DrawingSurface; one that is already acquired is returned again, else a new one is made.
ScriptDrawingSurface *DynamicSprite_GetDrawingSurface(ScriptDynamicSprite *dynsprite);
int DynamicSprite_GetGraphic(ScriptDynamicSprite *dynsprite);
int DynamicSprite_GetWidth(ScriptDynamicSprite *dynsprite);
int DynamicSprite_GetHeight(ScriptDynamicSprite *dynsprite);
int DynamicSprite_GetColorDepth(ScriptDynamicSprite *dynsprite);
// Stretches the sprite to the new size.
void DynamicSprite_Resize(ScriptDynamicSprite *dynsprite, int width, int height);
// "direction" is a script FlipDirection value.
void DynamicSprite_Flip(ScriptDynamicSprite *dynsprite, int direction);
// Merges the transparency of another sprite of the same size and colour depth into this one.
void DynamicSprite_CopyTransparencyMask(ScriptDynamicSprite *dynsprite, int source_sprite);
// Makes the canvas "width" x "height", with the old image placed at (x, y); the new area is transparent.
void DynamicSprite_ChangeCanvasSize(ScriptDynamicSprite *dynsprite, int width, int height, int x, int y);
// Crops the sprite to the given area; the part outside of the sprite is cut away with a warning.
void DynamicSprite_Crop(ScriptDynamicSprite *dynsprite, int x, int y, int width, int height);
// Rotates the sprite by "angle" degrees (1-359) about its centre; if "width" or "height" is SCR_NO_VALUE,
// the new size is taken from RotateSize().
void DynamicSprite_Rotate(ScriptDynamicSprite *dynsprite, int angle, int width, int height);
void DynamicSprite_Tint(ScriptDynamicSprite *dynsprite, int red, int green, int blue, int saturation, int luminance);
// Saves the sprite as an image in the script's write location, adding ".bmp" to a name without a dot.
// Returns 1 on success, 0 on failure.
int DynamicSprite_SaveToFile(ScriptDynamicSprite *dynsprite, const char *name);
// The Create* functions return nullptr if the image could not be made or no sprite slot was available.
// A "width" or "height" below 1 is adjusted to 1 with a warning where the function says so (Create,
// CreateFromDrawingSurface, CreateFromBackground); CreateFromSaveGame scales the screenshot to the given size.
ScriptDynamicSprite *DynamicSprite_CreateFromSaveGame(int save_slot, int width, int height);
ScriptDynamicSprite *DynamicSprite_CreateFromFile(const char *filename, int color_fmt);
// "layers" is a RENDER_BATCH_* value.
ScriptDynamicSprite *DynamicSprite_CreateFromScreenShot(int width, int height, int layers);
// Copies a sprite; quits if there is no sprite with this number.
ScriptDynamicSprite *DynamicSprite_CreateFromExistingSprite(int slot, int color_fmt);
// Copies an area of the surface; returns nullptr if the surface is not valid (released or its source gone).
// An area outside of the surface is clamped to it with a warning.
ScriptDynamicSprite *DynamicSprite_CreateFromDrawingSurface(ScriptDrawingSurface *surface, int x, int y, int width,
                                                            int height, int color_fmt);
ScriptDynamicSprite *DynamicSprite_Create(int width, int height, int color_fmt);
// Copies an area of a room background frame. SCR_NO_VALUE for "frame" is the current frame, for x1 and y1 is 0,
// for "width" and "height" is the room size; an invalid frame or area outside of the room quits the script.
ScriptDynamicSprite *DynamicSprite_CreateFromBackground(int frame, int x1, int y1, int width, int height);

// Registers a new dynamic sprite, and returns a slot number;
// returns a negative value (SpriteCache::NO_SPRITE_INDEX) if no free slot could be found or allocated,
// or the sprite could not be set.
// Updates game.SpriteInfos[].
int add_dynamic_sprite(std::unique_ptr<AGS::Common::Bitmap> image, uint32_t extra_flags = 0u);
// Registers a new dynamic sprite in the given slot number,
// previous bitmap on this slot (if any) will be deleted.
// Returns same slot number on success, or 0 if there was an error.
// Updates game.SpriteInfos[].
int add_dynamic_sprite(int slot, std::unique_ptr<AGS::Common::Bitmap> image, uint32_t extra_flags = 0u);
// Disposes a dynamic sprite, and frees the slot. Tells the draw system that the sprite was deleted;
// "notify_all" selects game_sprite_updated() or notify_sprite_changed(), which currently do the same.
void free_dynamic_sprite(int slot, bool notify_all = true);
// Retrieves a DrawingSurface object, connected to the given dynamic sprite; nullptr if none is acquired.
ScriptDrawingSurface *get_dynsprite_surface(int slot);
// Attaches drawing surface handle to the dynamic sprite.
// This method is used when unserializing drawing surfaces from the game save.
void attach_dynsprite_surface(int slot, int surface_handle);
// Detaches all drawing surfaces related to the given dynamic sprite.
// This invalidates DrawingSurface script object(s), and erases a sprite-surface reference.
void detach_dynsprite_surface(int slot);
// Called when a drawing surface of the sprite is released: if "modified", notifies the game objects that use
// the sprite, then detaches the surfaces and clears the sprite's "surface acquired" flag.
void on_dynsprite_surface_release(int slot, bool modified);
