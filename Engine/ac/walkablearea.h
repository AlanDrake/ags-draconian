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
// Walkable areas: the room's walkable area mask with its scaling, and what blocks a path across it.
// Coordinates are in room pixels unless a function says "mask"; a mask pixel covers
// thisroom.MaskResolution room pixels (room_to_mask_coord).
//=============================================================================
#pragma once

#include "util/geometry.h"

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// Rebuilds the room's walkable area mask from the backup taken when the room was loaded, erasing the areas
// that play.walkable_areas_on has switched off. Call after changing which areas are enabled.
void redo_walkable_areas();
// Returns the walkable area number (the mask value) at the room point, 0 for no area, or a negative value
// outside the mask. Erased (disabled) areas read as 0.
int get_walkable_area_pixel(int x, int y);
// Returns the scaling of the walkable area area_id at the room row y, as a percentage of normal size (100).
// A vector-scaled area interpolates between its far (top) and near (bottom) scaling over its rows,
// any other valid area has one scaling; an invalid area_id gives 100. Never returns 0 (that gives 100).
int get_area_scaling(int area_id, int y);
// Scales the sprite's size by zoom_level (a percentage) into new_width and new_height, each at least 1.
void scale_sprite_size(int sprite_id, int zoom_level, int *new_width, int *new_height);
// Erases the room-coordinate rectangle area from the temporary walkable bitmap (see prepare_walkable_areas).
void remove_walkable_areas_from_temp(const Rect &area);
// Returns the walkable mask with the blocking rectangles of the other characters and the solid objects in the
// room erased from it, for the pathfinder. Pass the moving character's ID, or -1 for none (then every
// enabled blocking character and solid object counts); a rectangle the moving character stands in is left
// walkable, so that it can get out. For a source character that does not block (CHF_NOBLOCKING) the result
// is the plain mask copy.
// IMPORTANT: this function returns *global pointer*, do not delete the returned bitmap! -- subject to future refactor
AGS::Common::Bitmap *prepare_walkable_areas(int source_char_id);
// Returns the walkable area (0 to MAX_WALK_AREAS - 1) at the room point, 0 for none. Off the room's edge it
// takes the area at the nearest edge; on a non-walkable pixel it tries the points 2 pixels to the right,
// left, below and above in turn.
int get_walkable_area_at_location(int x, int y);
// Same for the position of the character char_id.
int get_walkable_area_at_character(int char_id);
