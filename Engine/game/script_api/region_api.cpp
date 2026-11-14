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
// Script bindings registered by RegisterRegionAPI().
//
//=============================================================================
#include "game/dynobj/script_drawing_surface.h"
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterRegionAPI()
{
    ScFnRegister region_api[] = {
        ScStatic<&Region_GetAtRoomXY2>("Region::GetAtRoomXY^2"),
        ScStatic<&Region_GetAtRoomXY>("Region::GetAtRoomXY^3"),
        ScStatic<&Region_GetAtScreenXY2>("Region::GetAtScreenXY^2"),
        ScStatic<&Region_GetAtScreenXY>("Region::GetAtScreenXY^3"),
        ScStatic<&Region_GetByName>("Region::GetByScriptName^1"),
        ScStatic<&Region_GetDrawingSurface>("Region::GetDrawingSurface"),

        ScMethod<&Region_TintNoLum>("Region::Tint^4"),
        ScMethod<&Region_Tint>("Region::Tint^5"),
        ScMethod<&Region_RunInteraction>("Region::RunInteraction^1"),
        ScMethod<&Region_GetProperty>("Region::GetProperty^1"),
        ScMethod<&Region_GetTextProperty>("Region::GetTextProperty^1"),
        ScMethod<&Region_SetProperty>("Region::SetProperty^2"),
        ScMethod<&Region_SetTextProperty>("Region::SetTextProperty^2"),
        ScMethod<&Region_GetEnabled>("Region::get_Enabled"),
        ScMethod<&Region_SetEnabled>("Region::set_Enabled"),
        ScMethod<&Region_GetID>("Region::get_ID"),
        ScMethod<&Region_GetLightLevel>("Region::get_LightLevel"),
        ScMethod<&Region_SetLightLevel>("Region::set_LightLevel"),
        ScMethod<&Region_GetScriptName>("Region::get_ScriptName"),
        ScMethod<&Region_GetTintEnabled>("Region::get_TintEnabled"),
        ScMethod<&Region_GetTintBlue>("Region::get_TintBlue"),
        ScMethod<&Region_GetTintGreen>("Region::get_TintGreen"),
        ScMethod<&Region_GetTintRed>("Region::get_TintRed"),
        ScMethod<&Region_GetTintSaturation>("Region::get_TintSaturation"),
        ScMethod<&Region_GetTintLuminance>("Region::get_TintLuminance"),
    };

    ccAddExternalFunctions(region_api);
}
