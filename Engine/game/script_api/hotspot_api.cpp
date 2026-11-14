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
// Script bindings registered by RegisterHotspotAPI().
//
//=============================================================================
#include "game/game.h"
#include "game/lib/string.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/hotspot.h"
#include "game/world/room.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterHotspotAPI()
{
    ScFnRegister hotspot_api[] = {
        ScStatic<&Hotspot_GetAtRoomXY2>("Hotspot::GetAtRoomXY^2"),
        ScStatic<&Hotspot_GetAtScreenXY2>("Hotspot::GetAtScreenXY^2"),
        ScStatic<&Hotspot_GetAtRoomXY>("Hotspot::GetAtRoomXY^3"),
        ScStatic<&Hotspot_GetAtScreenXY>("Hotspot::GetAtScreenXY^3"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&Hotspot_GetByName>("Hotspot::GetByName"),
        ScStatic<&Hotspot_GetByName>("Hotspot::GetByScriptName"),
        ScStatic<&Hotspot_GetDrawingSurface>("Hotspot::GetDrawingSurface"),

        ScMethod<&Hotspot_GetProperty>("Hotspot::GetProperty^1"),
        ScMethod<&Hotspot_GetPropertyText>("Hotspot::GetPropertyText^2"),
        ScMethod<&Hotspot_GetTextProperty>("Hotspot::GetTextProperty^1"),
        ScMethod<&Hotspot_SetProperty>("Hotspot::SetProperty^2"),
        ScMethod<&Hotspot_SetTextProperty>("Hotspot::SetTextProperty^2"),
        ScMethod<&Hotspot_IsInteractionAvailable>("Hotspot::IsInteractionAvailable^1"),
        ScMethod<&Hotspot_RunInteraction>("Hotspot::RunInteraction^1"),
        ScMethod<&Hotspot_GetName_New>("Hotspot::get_DisplayName"),
        ScMethod<&Hotspot_SetName>("Hotspot::set_DisplayName"),
        ScMethod<&Hotspot_GetEnabled>("Hotspot::get_Enabled"),
        ScMethod<&Hotspot_SetEnabled>("Hotspot::set_Enabled"),
        ScMethod<&Hotspot_GetID>("Hotspot::get_ID"),
        // [OBSOLETE] Name => DisplayName
        ScMethod<&Hotspot_GetName_New>("Hotspot::get_Name"),
        ScMethod<&Hotspot_SetName>("Hotspot::set_Name"),
        ScMethod<&Hotspot_GetScriptName>("Hotspot::get_ScriptName"),
        ScMethod<&Hotspot_GetWalkToX>("Hotspot::get_WalkToX"),
        ScMethod<&Hotspot_GetWalkToY>("Hotspot::get_WalkToY"),
    };

    ccAddExternalFunctions(hotspot_api);
}
