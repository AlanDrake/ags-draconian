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
// Script bindings registered by RegisterPathfinderAPI().
//
//=============================================================================
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_pathfinder.h"
#include "game/navigation/pathfinder_script.h"
#include "game/script_api/bindings.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterPathfinderAPI()
{
    ScFnRegister pathfinder_api[] = {
        ScMethod<&Pathfinder_FindPath, ScManagedBy<globalDynamicArray>>("Pathfinder::FindPath"),
        ScMethod<&Pathfinder_Trace>("Pathfinder::Trace"),
        ScMethod<&Pathfinder_IsWalkableAt>("Pathfinder::IsWalkableAt"),
        ScMethod<&Pathfinder_NearestWalkablePoint>("Pathfinder::NearestWalkablePoint"),

        ScStatic<&MaskPathfinder_Create>("MaskPathfinder::Create"),
        ScMethod<&MaskPathfinder_SetMask>("MaskPathfinder::SetMask"),

        ScStatic<&MotionPath_Create>("MotionPath::Create"),
        ScStatic<&MotionPath_Create2>("MotionPath::Create2"),
        ScMethod<&MotionPath_GetPath, ScManagedBy<globalDynamicArray>>("MotionPath::GetPath"),
        ScMethod<&MotionPath_StepBack>("MotionPath::StepBack"),
        ScMethod<&MotionPath_StepForward>("MotionPath::StepForward"),
        ScMethod<&MotionPath_Reset>("MotionPath::Reset"),
        ScMethod<&MotionPath_GetValid>("MotionPath::get_Valid"),
        ScMethod<&MotionPath_GetDirection>("MotionPath::get_Direction"),
        ScMethod<&MotionPath_GetRepeatStyle>("MotionPath::get_RepeatStyle"),
        ScMethod<&MotionPath_GetWalkWhere>("MotionPath::get_WalkWhere"),
        ScMethod<&MotionPath_GetIsCompleted>("MotionPath::get_IsCompleted"),
        ScMethod<&MotionPath_GetStageCount>("MotionPath::get_StageCount"),
        ScMethod<&MotionPath_GetStageX>("MotionPath::geti_StageX"),
        ScMethod<&MotionPath_GetStageY>("MotionPath::geti_StageY"),
        ScMethod<&MotionPath_GetStageSpeedX>("MotionPath::geti_StageSpeedX"),
        ScMethod<&MotionPath_GetStageSpeedY>("MotionPath::geti_StageSpeedY"),
        ScMethod<&MotionPath_GetStage>("MotionPath::get_Stage"),
        ScMethod<&MotionPath_GetProgress>("MotionPath::get_Progress"),
        ScMethod<&MotionPath_GetPositionX>("MotionPath::get_PositionX"),
        ScMethod<&MotionPath_GetPositionY>("MotionPath::get_PositionY"),
        ScMethod<&MotionPath_GetVelocityX>("MotionPath::get_VelocityX"),
        ScMethod<&MotionPath_GetVelocityY>("MotionPath::get_VelocityY"),
    };

    ccAddExternalFunctions(pathfinder_api);
}
