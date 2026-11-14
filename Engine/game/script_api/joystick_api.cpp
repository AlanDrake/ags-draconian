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
// Script bindings registered by RegisterJoystickAPI().
//
//=============================================================================
#include "ac/dynobj/scriptstring.h"
#include "game/input/joystick.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int ScriptJoystick::()
RuntimeScriptValue Sc_Joystick_GetJoystickCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Joystick_GetJoystickCount);
}

// ScriptJoystick* ScriptJoystick::(int index)
RuntimeScriptValue Sc_Joystick_GetiJoysticks(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptJoystick, Joystick_GetiJoysticks);
}

// String* (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptJoystick, const char, myScriptStringImpl, Joystick_GetName);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_IsConnected(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_IsConnected);
}

// bool (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_IsAnyButtonDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptJoystick, Joystick_IsAnyButtonDown);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_IsGamepad(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_IsGamepad);
}

// int (ScriptJoystick *joy, int button)
RuntimeScriptValue Sc_Joystick_IsGamepadButtonDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_IsGamepadButtonDown);
}

// float (ScriptJoystick *joy, int axis)
RuntimeScriptValue Sc_Joystick_GetGamepadAxis(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT_PFLOAT(ScriptJoystick, Joystick_GetGamepadAxis);
}

RuntimeScriptValue Sc_Joystick_GetAxis(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT_PINT_PFLOAT(ScriptJoystick, Joystick_GetAxis);
}

RuntimeScriptValue Sc_Joystick_IsButtonDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_IsButtonDown);
}

//int (ScriptJoystick *joy, int hat)
RuntimeScriptValue Sc_Joystick_GetHat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptJoystick, Joystick_GetHat);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetAxisCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetAxisCount);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetButtonCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetButtonCount);
}

// int (ScriptJoystick *joy)
RuntimeScriptValue Sc_Joystick_GetHatCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptJoystick, Joystick_GetHatCount);
}

void RegisterJoystickAPI()
{
    ScFnRegister joystick_api[] = {{"Joystick::get_JoystickCount", API_FN_PAIR(Joystick_GetJoystickCount)},
                                   {"Joystick::geti_Joysticks", API_FN_PAIR(Joystick_GetiJoysticks)},
                                   {"Joystick::get_Name", API_FN_PAIR(Joystick_GetName)},
                                   {"Joystick::get_IsConnected", API_FN_PAIR(Joystick_IsConnected)},
                                   {"Joystick::get_IsGamepad", API_FN_PAIR(Joystick_IsGamepad)},
                                   {"Joystick::get_IsAnyButtonDown", API_FN_PAIR(Joystick_IsAnyButtonDown)},
                                   {"Joystick::IsGamepadButtonDown^1", API_FN_PAIR(Joystick_IsGamepadButtonDown)},
                                   {"Joystick::GetGamepadAxis^2", API_FN_PAIR(Joystick_GetGamepadAxis)},
                                   {"Joystick::GetAxis^2", API_FN_PAIR(Joystick_GetAxis)},
                                   {"Joystick::IsButtonDown^1", API_FN_PAIR(Joystick_IsButtonDown)},
                                   {"Joystick::GetHat^1", API_FN_PAIR(Joystick_GetHat)},
                                   {"Joystick::get_AxisCount", API_FN_PAIR(Joystick_GetAxisCount)},
                                   {"Joystick::get_ButtonCount", API_FN_PAIR(Joystick_GetButtonCount)},
                                   {"Joystick::get_HatCount", API_FN_PAIR(Joystick_GetHatCount)}};

    ccAddExternalFunctions(joystick_api);
}
