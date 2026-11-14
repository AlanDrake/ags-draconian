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
#include "game/input/joystick.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

void RegisterJoystickAPI()
{
    ScFnRegister joystick_api[] = {ScStatic<&Joystick_GetJoystickCount>("Joystick::get_JoystickCount"),
                                   ScStatic<&Joystick_GetiJoysticks>("Joystick::geti_Joysticks"),
                                   ScMethod<&Joystick_GetName>("Joystick::get_Name"),
                                   ScMethod<&Joystick_IsConnected>("Joystick::get_IsConnected"),
                                   ScMethod<&Joystick_IsGamepad>("Joystick::get_IsGamepad"),
                                   ScMethod<&Joystick_IsAnyButtonDown>("Joystick::get_IsAnyButtonDown"),
                                   ScMethod<&Joystick_IsGamepadButtonDown>("Joystick::IsGamepadButtonDown^1"),
                                   ScMethod<&Joystick_GetGamepadAxis>("Joystick::GetGamepadAxis^2"),
                                   ScMethod<&Joystick_GetAxis>("Joystick::GetAxis^2"),
                                   ScMethod<&Joystick_IsButtonDown>("Joystick::IsButtonDown^1"),
                                   ScMethod<&Joystick_GetHat>("Joystick::GetHat^1"),
                                   ScMethod<&Joystick_GetAxisCount>("Joystick::get_AxisCount"),
                                   ScMethod<&Joystick_GetButtonCount>("Joystick::get_ButtonCount"),
                                   ScMethod<&Joystick_GetHatCount>("Joystick::get_HatCount")};

    ccAddExternalFunctions(joystick_api);
}
