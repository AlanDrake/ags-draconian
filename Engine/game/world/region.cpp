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
#include "game/world/region.h"

#include "debug/debug_log.h"
#include "game/common_defines.h"
#include "game/dynobj/cc_region.h"
#include "game/game.h"
#include "game/game_setup_struct.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/roomstruct.h"
#include "game/script.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "game/world/room_status.h"
#include "script/runtimescriptvalue.h"
#include "script/script_runtime.h"
#include "util/common.h"

using namespace AGS::Common;


int GetRegionIDAtRoom(int x, int y, int hit_options)
{
    // if the co-ordinates are off the edge of the screen,
    // correct them to be just within
    // this fixes walk-off-screen problems
    x = room_to_mask_coord(x);
    y = room_to_mask_coord(y);

    if (x >= thisroom.RegionMask->GetWidth())
        x = thisroom.RegionMask->GetWidth() - 1;
    if (y >= thisroom.RegionMask->GetHeight())
        y = thisroom.RegionMask->GetHeight() - 1;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;

    int hsthere = thisroom.RegionMask->GetPixel(x, y);
    if (hsthere <= 0 || hsthere >= MAX_ROOM_REGIONS)
        return 0;
    if (((hit_options & kHit_Interactable) != 0) && (croom->region_enabled[hsthere] == 0))
        return 0;
    return hsthere;
}

ScriptRegion *Region_GetAtRoomXY(int x, int y, int hit_options)
{
    return &scrRegion[GetRegionIDAtRoom(x, y, hit_options)];
}

ScriptRegion *Region_GetAtRoomXY2(int x, int y)
{
    return Region_GetAtRoomXY(x, y, kHit_Interactable);
}

ScriptRegion *Region_GetAtScreenXY(int x, int y, int hit_options)
{
    VpPoint vpt = play.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return &scrRegion[0]; // return region[0] for consistency and backwards compatibility
    return Region_GetAtRoomXY(vpt.first.X, vpt.first.Y, hit_options);
}

ScriptRegion *Region_GetAtScreenXY2(int x, int y)
{
    return Region_GetAtScreenXY(x, y, kHit_Interactable);
}

ScriptRegion *Region_GetByName(const char *name)
{
    return static_cast<ScriptRegion *>(ccGetScriptObjectAddress(name, ccDynamicRegion.GetType()));
}

void SetAreaLightLevel(int area, int brightness)
{
    if ((area < 0) || (area > MAX_ROOM_REGIONS))
        quit("!SetAreaLightLevel: invalid region");
    if (brightness < -255)
        brightness = -255;
    if (brightness > 255)
        brightness = 255;
    // Set light level and disable RGB tint for this area
    if (thisroom.Regions[area].Light != brightness || thisroom.Regions[area].Tint != 0)
    {
        thisroom.Regions[area].Light = brightness;
        thisroom.Regions[area].Tint = 0;
        debug_script_log("Region %d light level set to %d", area, brightness);
    }
}

void Region_SetLightLevel(ScriptRegion *ssr, int brightness)
{
    SetAreaLightLevel(ssr->id, brightness);
}

int Region_GetLightLevel(ScriptRegion *ssr)
{
    return thisroom.GetRegionLightLevel(ssr->id);
}

const char *Region_GetScriptName(ScriptRegion *ssr)
{
    return CreateNewScriptString(thisroom.Regions[ssr->id].ScriptName);
}

int Region_GetTintEnabled(ScriptRegion *srr)
{
    if (thisroom.Regions[srr->id].Tint & 0xFF000000)
        return 1;
    return 0;
}

int Region_GetTintRed(ScriptRegion *srr)
{
    return thisroom.Regions[srr->id].Tint & 0x000000ff;
}

int Region_GetTintGreen(ScriptRegion *srr)
{
    return (thisroom.Regions[srr->id].Tint >> 8) & 0x000000ff;
}

int Region_GetTintBlue(ScriptRegion *srr)
{
    return (thisroom.Regions[srr->id].Tint >> 16) & 0x000000ff;
}

int Region_GetTintSaturation(ScriptRegion *srr)
{
    return (thisroom.Regions[srr->id].Tint >> 24) & 0xFF;
}

int Region_GetTintLuminance(ScriptRegion *srr)
{
    return thisroom.GetRegionTintLuminance(srr->id);
}

void SetRegionTint(int area, int red, int green, int blue, int amount, int luminance = 255)
{
    if ((area < 0) || (area >= MAX_ROOM_REGIONS))
        quit("!SetRegionTint: invalid region");

    if ((red < 0) || (red > 255) || (green < 0) || (green > 255) || (blue < 0) || (blue > 255))
    {
        quit("!SetRegionTint: RGB values must be 0-255");
    }

    if ((amount < 0) || (amount > 255))
        quit("!SetRegionTint: amount must be 0-255");
    if ((luminance < 0) || (luminance > 255))
        quit("!SetRegionTint: luminance must be 0-255");

    const int tint =
        (amount > 0) ? (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0XFF) << 16) | ((amount & 0xFF) << 24) : 0;
    const int light = (amount > 0) ? luminance : 0;

    if ((thisroom.Regions[area].Tint != tint) || (thisroom.Regions[area].Light != light))
    {
        thisroom.Regions[area].Tint = tint;
        thisroom.Regions[area].Light = light;
        debug_script_log("Region %d tint set to RGB %d,%d,%d, A %d, L %d", area, red, green, blue, amount, luminance);
    }
}

void Region_Tint(ScriptRegion *srr, int red, int green, int blue, int amount, int luminance)
{
    SetRegionTint(srr->id, red, green, blue, amount, luminance);
}

void Region_TintNoLum(ScriptRegion *srr, int red, int green, int blue, int amount)
{
    SetRegionTint(srr->id, red, green, blue, amount);
}

void DisableRegion(int hsnum)
{
    if ((hsnum < 0) || (hsnum >= MAX_ROOM_REGIONS))
        quit("!DisableRegion: invalid region specified");

    croom->region_enabled[hsnum] = 0;
    debug_script_log("Region %d disabled", hsnum);
}

void EnableRegion(int hsnum)
{
    if ((hsnum < 0) || (hsnum >= MAX_ROOM_REGIONS))
        quit("!EnableRegion: invalid region specified");

    croom->region_enabled[hsnum] = 1;
    debug_script_log("Region %d enabled", hsnum);
}

void Region_SetEnabled(ScriptRegion *ssr, int enable)
{
    if (enable)
        EnableRegion(ssr->id);
    else
        DisableRegion(ssr->id);
}

int Region_GetEnabled(ScriptRegion *ssr)
{
    return croom->region_enabled[ssr->id];
}

int Region_GetID(ScriptRegion *ssr)
{
    return ssr->id;
}

void RunRegionInteraction(int regnum, int mood)
{
    if ((regnum < 0) || (regnum >= MAX_ROOM_REGIONS))
        quit("!RunRegionInteraction: invalid region specified");
    if ((mood < kRegionEvent_Standing) || (mood > kRegionEvent_WalkOff))
        quit("!RunRegionInteraction: invalid event specified");

    // Regions do not react to cursor modes, but this function
    // was historically executing special region events (see RegionEventID)
    const auto obj_evt =
        ObjectEvent(kScTypeRoom, RuntimeScriptValue().SetScriptObject(&scrRegion[regnum], &ccDynamicRegion));
    run_event_script(obj_evt, &thisroom.Regions[regnum].GetEvents(), mood);
}

void Region_RunInteraction(ScriptRegion *ssr, int mood)
{
    RunRegionInteraction(ssr->id, mood);
}

int Region_GetProperty(ScriptRegion *ssr, const char *property)
{
    return get_int_property(thisroom.Regions[ssr->id].Properties, croom->regProps[ssr->id], property);
}

const char *Region_GetTextProperty(ScriptRegion *ssr, const char *property)
{
    return get_text_property_dynamic_string(thisroom.Regions[ssr->id].Properties, croom->regProps[ssr->id], property);
}

bool Region_SetProperty(ScriptRegion *ssr, const char *property, int value)
{
    return set_int_property(croom->regProps[ssr->id], property, value);
}

bool Region_SetTextProperty(ScriptRegion *ssr, const char *property, const char *value)
{
    return set_text_property(croom->regProps[ssr->id], property, value);
}

void DisableGroundLevelAreas(int alsoEffects)
{
    if ((alsoEffects < 0) || (alsoEffects > 1))
        quit("!DisableGroundLevelAreas: invalid parameter: must be 0 or 1");

    play.ground_level_areas_disabled = GLED_INTERACTION;

    if (alsoEffects)
        play.ground_level_areas_disabled |= GLED_EFFECTS;

    debug_script_log("Ground-level areas disabled");
}

void EnableGroundLevelAreas()
{
    play.ground_level_areas_disabled = 0;

    debug_script_log("Ground-level areas re-enabled");
}

//=============================================================================

void generate_light_table()
{
    if (game.color_depth == 1 && color_map == nullptr)
    {
        create_light_table(&maincoltable, palette, 0, 0, 0, nullptr);
        color_map = &maincoltable;
    }
}

#include "script/script_runtime.h"
