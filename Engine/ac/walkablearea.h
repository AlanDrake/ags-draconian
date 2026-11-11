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
//
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

void redo_walkable_areas();
int get_walkable_area_pixel(int x, int y);
int get_area_scaling(int area_id, int x, int y);
void scale_sprite_size(int sprite_id, int zoom_level, int *new_width, int *new_height);
void remove_walkable_areas_from_temp(const Rect &area);
// IMPORTANT: this function returns *global pointer*, do not delete the returned bitmap! -- subject to future refactor
AGS::Common::Bitmap *prepare_walkable_areas(int source_char_id);
int get_walkable_area_at_location(int x, int y);
int get_walkable_area_at_character(int char_id);
