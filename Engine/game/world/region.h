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

#include "game/dynobj/script_objects.h"
#include "game/roomdata.h"

int GetRegionIDAtRoom(int x, int y, int hit_options);
void Region_SetLightLevel(ScriptRegion *ssr, int brightness);
int Region_GetLightLevel(ScriptRegion *ssr);
int Region_GetTintEnabled(ScriptRegion *srr);
int Region_GetTintRed(ScriptRegion *srr);
int Region_GetTintGreen(ScriptRegion *srr);
int Region_GetTintBlue(ScriptRegion *srr);
int Region_GetTintSaturation(ScriptRegion *srr);
int Region_GetTintLuminance(ScriptRegion *srr);
void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount, int luminance);
void Region_SetEnabled(ScriptRegion *ssr, int enable);
int Region_GetEnabled(ScriptRegion *ssr);
int Region_GetID(ScriptRegion *ssr);
void Region_RunInteraction(ScriptRegion *ssr, int mood);
void RunRegionInteraction(int regnum, int mood);

void DisableGroundLevelAreas(int alsoEffects);
void EnableGroundLevelAreas();

void generate_light_table();

// Script API: Region::GetAtRoomXY^3
ScriptRegion *Region_GetAtRoomXY(int x, int y, int hit_options);

// Script API: Region::GetAtRoomXY^2
ScriptRegion *Region_GetAtRoomXY2(int x, int y);

// Script API: Region::GetAtScreenXY^3
ScriptRegion *Region_GetAtScreenXY(int x, int y, int hit_options);

// Script API: Region::GetAtScreenXY^2
ScriptRegion *Region_GetAtScreenXY2(int x, int y);

// Script API: Region::GetByScriptName^1
ScriptRegion *Region_GetByName(const char *name);

// Script API: Region::get_ScriptName
const char *Region_GetScriptName(ScriptRegion *ssr);

// Script API: Region::Tint^4
void Region_TintNoLum(ScriptRegion *srr, int red, int green, int blue, int amount);

// Script API: Region::GetProperty^1
int Region_GetProperty(ScriptRegion *ssr, const char *property);

// Script API: Region::GetTextProperty^1
const char *Region_GetTextProperty(ScriptRegion *ssr, const char *property);

// Script API: Region::SetProperty^2
bool Region_SetProperty(ScriptRegion *ssr, const char *property, int value);

// Script API: Region::SetTextProperty^2
bool Region_SetTextProperty(ScriptRegion *ssr, const char *property, const char *value);
