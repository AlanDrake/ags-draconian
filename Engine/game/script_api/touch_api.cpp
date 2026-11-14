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
// Script bindings registered by RegisterTouchAPI().
//
//=============================================================================
#include "ac/dynobj/scripttouchpointer.h"
#include "game/input/touch.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_Touch_GetTouchPointerCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Touch_GetTouchPointerCount);
}

RuntimeScriptValue Sc_Touch_GetTouchPointers(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptTouchPointer, Touch_GetTouchPointers);
}

RuntimeScriptValue Sc_TouchPointer_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetID);
}

RuntimeScriptValue Sc_TouchPointer_GetIsDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptTouchPointer, TouchPointer_GetIsDown);
}

RuntimeScriptValue Sc_TouchPointer_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetX);
}

RuntimeScriptValue Sc_TouchPointer_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptTouchPointer, TouchPointer_GetY);
}

void RegisterTouchAPI()
{
    ScFnRegister touch_api[] = {
        {"Touch::get_TouchPointerCount", API_FN_PAIR(Touch_GetTouchPointerCount)},
        {"Touch::geti_TouchPointers", API_FN_PAIR(Touch_GetTouchPointers)},

        {"TouchPointer::get_ID", API_FN_PAIR(TouchPointer_GetID)},
        {"TouchPointer::get_IsDown", API_FN_PAIR(TouchPointer_GetIsDown)},
        {"TouchPointer::get_X", API_FN_PAIR(TouchPointer_GetX)},
        {"TouchPointer::get_Y", API_FN_PAIR(TouchPointer_GetY)},
    };

    ccAddExternalFunctions(touch_api);
}
