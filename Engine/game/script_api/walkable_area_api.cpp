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
#include "ac/character.h"
#include "ac/dynobj/cc_walkablearea.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "ac/room.h"
#include "ac/walkablearea.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

extern RuntimeScriptValue Sc_GetDrawingSurfaceForWalkableArea(const RuntimeScriptValue *params, int32_t param_count);

RuntimeScriptValue Sc_Walkarea_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkableArea, ccDynamicWalkarea, Walkarea_GetAtRoomXY);
}

RuntimeScriptValue Sc_Walkarea_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkableArea, ccDynamicWalkarea, Walkarea_GetAtScreenXY);
}

RuntimeScriptValue Sc_Walkarea_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptWalkableArea, ccDynamicWalkarea, Walkarea_GetByName, const char);
}

RuntimeScriptValue Sc_Walkarea_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptWalkableArea, Walkarea_SetScaling);
}

RuntimeScriptValue Sc_Walkarea_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetEnabled);
}

RuntimeScriptValue Sc_Walkarea_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptWalkableArea, Walkarea_SetEnabled);
}

RuntimeScriptValue Sc_Walkarea_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetID);
}

RuntimeScriptValue Sc_Walkarea_GetScalingMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetScalingMin);
}

RuntimeScriptValue Sc_Walkarea_GetScalingMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkableArea, Walkarea_GetScalingMax);
}

RuntimeScriptValue Sc_Walkarea_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptWalkableArea, const char, myScriptStringImpl, Walkarea_GetScriptName);
}

RuntimeScriptValue Sc_Walkarea_GetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptWalkableArea, Walkarea_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Walkarea_SetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptWalkableArea, Walkarea_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Walkarea_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptWalkableArea, Walkarea_GetProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptWalkableArea, const char, myScriptStringImpl, Walkarea_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptWalkableArea, Walkarea_SetProperty, const char);
}

RuntimeScriptValue Sc_Walkarea_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptWalkableArea, Walkarea_SetTextProperty, const char, const char);
}


void RegisterWalkareaAPI()
{
    ScFnRegister walkarea_api[] = {
        {"WalkableArea::GetAtRoomXY^2", API_FN_PAIR(Walkarea_GetAtRoomXY)},
        {"WalkableArea::GetAtScreenXY^2", API_FN_PAIR(Walkarea_GetAtScreenXY)},
        {"WalkableArea::GetByScriptName^1", API_FN_PAIR(Walkarea_GetByName)},
        {"WalkableArea::GetDrawingSurface", API_FN_PAIR(GetDrawingSurfaceForWalkableArea)},

        {"WalkableArea::SetScaling^2", API_FN_PAIR(Walkarea_SetScaling)},
        {"WalkableArea::GetProperty^1", API_FN_PAIR(Walkarea_GetProperty)},
        {"WalkableArea::GetTextProperty^1", API_FN_PAIR(Walkarea_GetTextProperty)},
        {"WalkableArea::SetProperty^2", API_FN_PAIR(Walkarea_SetProperty)},
        {"WalkableArea::SetTextProperty^2", API_FN_PAIR(Walkarea_SetTextProperty)},
        {"WalkableArea::get_Enabled", API_FN_PAIR(Walkarea_GetEnabled)},
        {"WalkableArea::set_Enabled", API_FN_PAIR(Walkarea_SetEnabled)},
        {"WalkableArea::get_ID", API_FN_PAIR(Walkarea_GetID)},
        {"WalkableArea::get_ScalingMin", API_FN_PAIR(Walkarea_GetScalingMin)},
        {"WalkableArea::get_ScalingMax", API_FN_PAIR(Walkarea_GetScalingMax)},
        {"WalkableArea::get_ScriptName", API_FN_PAIR(Walkarea_GetScriptName)},
        {"WalkableArea::get_FaceDirectionRatio", API_FN_PAIR(Walkarea_GetFaceDirectionRatio)},
        {"WalkableArea::set_FaceDirectionRatio", API_FN_PAIR(Walkarea_SetFaceDirectionRatio)},
    };

    ccAddExternalFunctions(walkarea_api);
}
