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
// Script bindings registered by RegisterWalkareaAPI().
//
//=============================================================================
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "game/world/room.h"
#include "game/world/walkable_area.h"
#include "script/script_runtime.h"

using namespace AGS::Common;


void RegisterWalkareaAPI()
{
    ScFnRegister walkarea_api[] = {
        ScStatic<&Walkarea_GetAtRoomXY>("WalkableArea::GetAtRoomXY^2"),
        ScStatic<&Walkarea_GetAtScreenXY>("WalkableArea::GetAtScreenXY^2"),
        ScStatic<&Walkarea_GetByName>("WalkableArea::GetByScriptName^1"),
        ScStatic<&GetDrawingSurfaceForWalkableArea>("WalkableArea::GetDrawingSurface"),

        ScMethod<&Walkarea_SetScaling>("WalkableArea::SetScaling^2"),
        ScMethod<&Walkarea_GetProperty>("WalkableArea::GetProperty^1"),
        ScMethod<&Walkarea_GetTextProperty>("WalkableArea::GetTextProperty^1"),
        ScMethod<&Walkarea_SetProperty>("WalkableArea::SetProperty^2"),
        ScMethod<&Walkarea_SetTextProperty>("WalkableArea::SetTextProperty^2"),
        ScMethod<&Walkarea_GetEnabled>("WalkableArea::get_Enabled"),
        ScMethod<&Walkarea_SetEnabled>("WalkableArea::set_Enabled"),
        ScMethod<&Walkarea_GetID>("WalkableArea::get_ID"),
        ScMethod<&Walkarea_GetScalingMin>("WalkableArea::get_ScalingMin"),
        ScMethod<&Walkarea_GetScalingMax>("WalkableArea::get_ScalingMax"),
        ScMethod<&Walkarea_GetScriptName>("WalkableArea::get_ScriptName"),
        ScMethod<&Walkarea_GetFaceDirectionRatio>("WalkableArea::get_FaceDirectionRatio"),
        ScMethod<&Walkarea_SetFaceDirectionRatio, ScUndefinedResult>("WalkableArea::set_FaceDirectionRatio"),
    };

    ccAddExternalFunctions(walkarea_api);
}
