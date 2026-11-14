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
// Script bindings registered by RegisterWalkbehindAPI().
//
//=============================================================================
#include "game/dynobj/script_objects.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "game/world/room.h"
#include "game/world/walk_behind.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;


void RegisterWalkbehindAPI()
{
    ScFnRegister walkbehind_api[] = {
        ScStatic<&Walkbehind_GetAtRoomXY>("Walkbehind::GetAtRoomXY^2"),
        ScStatic<&Walkbehind_GetAtScreenXY>("Walkbehind::GetAtScreenXY^2"),
        ScStatic<&Walkbehind_GetByName>("Walkbehind::GetByScriptName^1"),
        ScStatic<&GetDrawingSurfaceForWalkbehind>("Walkbehind::GetDrawingSurface"),

        ScMethod<&Walkbehind_GetBaseline>("Walkbehind::get_Baseline"),
        ScMethod<&Walkbehind_SetBaseline>("Walkbehind::set_Baseline"),
        ScMethod<&Walkbehind_GetID>("Walkbehind::get_ID"),
        ScMethod<&Walkbehind_GetScriptName>("Walkbehind::get_ScriptName"),
    };

    ccAddExternalFunctions(walkbehind_api);
}
