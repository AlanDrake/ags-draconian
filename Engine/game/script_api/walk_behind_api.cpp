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
#include "ac/dynobj/cc_walkbehind.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "ac/room.h"
#include "ac/walkbehind.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern RuntimeScriptValue Sc_GetDrawingSurfaceForWalkbehind(const RuntimeScriptValue *params, int32_t param_count);

RuntimeScriptValue Sc_Walkbehind_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkbehind, ccDynamicWalkbehind, Walkbehind_GetAtRoomXY);
}

RuntimeScriptValue Sc_Walkbehind_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptWalkbehind, ccDynamicWalkbehind, Walkbehind_GetAtScreenXY);
}

RuntimeScriptValue Sc_Walkbehind_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptWalkbehind, ccDynamicWalkbehind, Walkbehind_GetByName, const char);
}

RuntimeScriptValue Sc_Walkbehind_GetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkbehind, Walkbehind_GetBaseline);
}

RuntimeScriptValue Sc_Walkbehind_SetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptWalkbehind, Walkbehind_SetBaseline);
}

RuntimeScriptValue Sc_Walkbehind_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptWalkbehind, Walkbehind_GetID);
}

RuntimeScriptValue Sc_Walkbehind_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptWalkbehind, const char, myScriptStringImpl, Walkbehind_GetScriptName);
}


void RegisterWalkbehindAPI()
{
    ScFnRegister walkbehind_api[] = {
        {"Walkbehind::GetAtRoomXY^2", API_FN_PAIR(Walkbehind_GetAtRoomXY)},
        {"Walkbehind::GetAtScreenXY^2", API_FN_PAIR(Walkbehind_GetAtScreenXY)},
        {"Walkbehind::GetByScriptName^1", API_FN_PAIR(Walkbehind_GetByName)},
        {"Walkbehind::GetDrawingSurface", API_FN_PAIR(GetDrawingSurfaceForWalkbehind)},

        {"Walkbehind::get_Baseline", API_FN_PAIR(Walkbehind_GetBaseline)},
        {"Walkbehind::set_Baseline", API_FN_PAIR(Walkbehind_SetBaseline)},
        {"Walkbehind::get_ID", API_FN_PAIR(Walkbehind_GetID)},
        {"Walkbehind::get_ScriptName", API_FN_PAIR(Walkbehind_GetScriptName)},
    };

    ccAddExternalFunctions(walkbehind_api);
}
