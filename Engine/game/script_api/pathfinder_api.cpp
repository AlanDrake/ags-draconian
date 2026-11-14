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
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptuserobject.h"
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_pathfinder.h"
#include "game/navigation/pathfinder_script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Pathfinder_FindPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptPathfinder, void, globalDynamicArray, Pathfinder_FindPath);
}

RuntimeScriptValue Sc_Pathfinder_Trace(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT4(ScriptPathfinder, ScriptUserObject, Pathfinder_Trace);
}

RuntimeScriptValue Sc_Pathfinder_IsWalkableAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT2(ScriptPathfinder, Pathfinder_IsWalkableAt);
}

RuntimeScriptValue Sc_Pathfinder_NearestWalkablePoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2(ScriptPathfinder, ScriptUserObject, Pathfinder_NearestWalkablePoint);
}

RuntimeScriptValue Sc_MaskPathfinder_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptMaskPathfinder, MaskPathfinder_Create);
}

RuntimeScriptValue Sc_MaskPathfinder_SetMask(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptMaskPathfinder, MaskPathfinder_SetMask);
}

RuntimeScriptValue Sc_MotionPath_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PFLOAT2_PINT2(ScriptMotionPath, MotionPath_Create, void);
}

RuntimeScriptValue Sc_MotionPath_Create2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ3_PINT2(ScriptMotionPath, MotionPath_Create2, void, void, void);
}

RuntimeScriptValue Sc_MotionPath_GetPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptMotionPath, void, globalDynamicArray, MotionPath_GetPath);
}

RuntimeScriptValue Sc_MotionPath_StepBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptMotionPath, MotionPath_StepBack);
}

RuntimeScriptValue Sc_MotionPath_StepForward(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptMotionPath, MotionPath_StepForward);
}

RuntimeScriptValue Sc_MotionPath_Reset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_PFLOAT(ScriptMotionPath, MotionPath_Reset);
}

RuntimeScriptValue Sc_MotionPath_GetDirection(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetDirection);
}

RuntimeScriptValue Sc_MotionPath_GetRepeatStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetRepeatStyle);
}

RuntimeScriptValue Sc_MotionPath_GetWalkWhere(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetWalkWhere);
}

RuntimeScriptValue Sc_MotionPath_GetValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptMotionPath, MotionPath_GetValid);
}

RuntimeScriptValue Sc_MotionPath_GetIsCompleted(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptMotionPath, MotionPath_GetIsCompleted);
}

RuntimeScriptValue Sc_MotionPath_GetStageCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetStageCount);
}

RuntimeScriptValue Sc_MotionPath_GetStageX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptMotionPath, MotionPath_GetStageX);
}

RuntimeScriptValue Sc_MotionPath_GetStageY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptMotionPath, MotionPath_GetStageY);
}

RuntimeScriptValue Sc_MotionPath_GetStageSpeedX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT(ScriptMotionPath, MotionPath_GetStageSpeedX);
}

RuntimeScriptValue Sc_MotionPath_GetStageSpeedY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT(ScriptMotionPath, MotionPath_GetStageSpeedY);
}

RuntimeScriptValue Sc_MotionPath_GetStage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetStage);
}

RuntimeScriptValue Sc_MotionPath_GetProgress(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptMotionPath, MotionPath_GetProgress);
}

RuntimeScriptValue Sc_MotionPath_GetPositionX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetPositionX);
}

RuntimeScriptValue Sc_MotionPath_GetPositionY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptMotionPath, MotionPath_GetPositionY);
}

RuntimeScriptValue Sc_MotionPath_GetVelocityX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptMotionPath, MotionPath_GetVelocityX);
}

RuntimeScriptValue Sc_MotionPath_GetVelocityY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptMotionPath, MotionPath_GetVelocityY);
}


void RegisterPathfinderAPI()
{
    ScFnRegister pathfinder_api[] = {
        {"Pathfinder::FindPath", API_FN_PAIR(Pathfinder_FindPath)},
        {"Pathfinder::Trace", API_FN_PAIR(Pathfinder_Trace)},
        {"Pathfinder::IsWalkableAt", API_FN_PAIR(Pathfinder_IsWalkableAt)},
        {"Pathfinder::NearestWalkablePoint", API_FN_PAIR(Pathfinder_NearestWalkablePoint)},

        {"MaskPathfinder::Create", API_FN_PAIR(MaskPathfinder_Create)},
        {"MaskPathfinder::SetMask", API_FN_PAIR(MaskPathfinder_SetMask)},

        {"MotionPath::Create", API_FN_PAIR(MotionPath_Create)},
        {"MotionPath::Create2", API_FN_PAIR(MotionPath_Create2)},
        {"MotionPath::GetPath", API_FN_PAIR(MotionPath_GetPath)},
        {"MotionPath::StepBack", API_FN_PAIR(MotionPath_StepBack)},
        {"MotionPath::StepForward", API_FN_PAIR(MotionPath_StepForward)},
        {"MotionPath::Reset", API_FN_PAIR(MotionPath_Reset)},
        {"MotionPath::get_Valid", API_FN_PAIR(MotionPath_GetValid)},
        {"MotionPath::get_Direction", API_FN_PAIR(MotionPath_GetDirection)},
        {"MotionPath::get_RepeatStyle", API_FN_PAIR(MotionPath_GetRepeatStyle)},
        {"MotionPath::get_WalkWhere", API_FN_PAIR(MotionPath_GetWalkWhere)},
        {"MotionPath::get_IsCompleted", API_FN_PAIR(MotionPath_GetIsCompleted)},
        {"MotionPath::get_StageCount", API_FN_PAIR(MotionPath_GetStageCount)},
        {"MotionPath::geti_StageX", API_FN_PAIR(MotionPath_GetStageX)},
        {"MotionPath::geti_StageY", API_FN_PAIR(MotionPath_GetStageY)},
        {"MotionPath::geti_StageSpeedX", API_FN_PAIR(MotionPath_GetStageSpeedX)},
        {"MotionPath::geti_StageSpeedY", API_FN_PAIR(MotionPath_GetStageSpeedY)},
        {"MotionPath::get_Stage", API_FN_PAIR(MotionPath_GetStage)},
        {"MotionPath::get_Progress", API_FN_PAIR(MotionPath_GetProgress)},
        {"MotionPath::get_PositionX", API_FN_PAIR(MotionPath_GetPositionX)},
        {"MotionPath::get_PositionY", API_FN_PAIR(MotionPath_GetPositionY)},
        {"MotionPath::get_VelocityX", API_FN_PAIR(MotionPath_GetVelocityX)},
        {"MotionPath::get_VelocityY", API_FN_PAIR(MotionPath_GetVelocityY)},
    };

    ccAddExternalFunctions(pathfinder_api);
}
