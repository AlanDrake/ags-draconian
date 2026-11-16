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
// Walk-behinds calculation logic.
//
//=============================================================================
#pragma once

#include "game/dynobj/script_objects.h"
#include "util/geometry.h"

// A method of rendering walkbehinds on screen:
// DrawAsSeparateSprite - draws whole walkbehind as a sprite;
//     this method is most simple and is optimal for 3D renderers.
// DrawOverCharSprite - turns parts of the character and object sprites
//     transparent when they are covered by walkbehind (walkbehind itself
//     is not drawn separately in this case);
//     this method is optimized for software render.
enum WalkBehindMethodEnum
{
    DrawAsSeparateSprite,
    DrawOverCharSprite,
};

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

class Room;

// Recalculates walk-behind positions
void walkbehinds_recalc();
// Generates walk-behinds as separate sprites
void walkbehinds_generate_sprites();
// Edits the given game object's sprite, cutting out pixels covered by walk-behinds;
// returns whether any pixels were updated
bool walkbehinds_cropout(Room &room, AGS::Common::Bitmap *sprit, int sprx, int spry, int basel);

struct ScriptWalkbehind;

// Script API: Walkbehind::GetAtRoomXY^2
ScriptWalkbehind *Walkbehind_GetAtRoomXY(int x, int y);

// Script API: Walkbehind::GetAtScreenXY^2
ScriptWalkbehind *Walkbehind_GetAtScreenXY(int x, int y);

// Script API: Walkbehind::GetByScriptName^1
ScriptWalkbehind *Walkbehind_GetByName(const char *name);

// Script API: Walkbehind::get_ID
int Walkbehind_GetID(ScriptWalkbehind *wb);

// Script API: Walkbehind::get_Baseline
int Walkbehind_GetBaseline(ScriptWalkbehind *wb);

// Script API: Walkbehind::set_Baseline
void Walkbehind_SetBaseline(ScriptWalkbehind *wb, int baseline);

// Script API: Walkbehind::get_ScriptName
const char *Walkbehind_GetScriptName(ScriptWalkbehind *wb);

void SetWalkBehindBase(int wa, int bl);
int GetWalkBehindBase(int wa);
