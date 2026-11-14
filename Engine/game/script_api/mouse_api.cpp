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
// Script bindings registered by RegisterMouseAPI().
//
//=============================================================================
#include "device/mousew32.h"
#include "game/dynobj/script_shader.h"
#include "game/ui/mouse.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

// void  (int curs, int newslot)
RuntimeScriptValue Sc_Mouse_ChangeCursorGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_ChangeCursorGraphic);
}

// void  (int curs, int x, int y)
RuntimeScriptValue Sc_Mouse_ChangeCursorHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Mouse_ChangeCursorHotspot);
}

RuntimeScriptValue Sc_Mouse_GetModeHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptUserObject, Mouse_GetModeHotspot);
}

// void (int curs, int newview)
RuntimeScriptValue Sc_Mouse_ChangeModeView2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_ChangeModeView2);
}

RuntimeScriptValue Sc_Mouse_ChangeModeView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Mouse_ChangeModeView);
}

// void (int modd)
RuntimeScriptValue Sc_Mouse_DisableCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_DisableCursorMode);
}

// void (int modd)
RuntimeScriptValue Sc_Mouse_EnableCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_EnableCursorMode);
}

// int (int curs)
RuntimeScriptValue Sc_Mouse_GetModeGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_GetModeGraphic);
}

// int (int which)
RuntimeScriptValue Sc_Mouse_IsButtonDown(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_IsButtonDown);
}

RuntimeScriptValue Sc_IsAnyButtonDown(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(IsAnyButtonDown);
}

// int (int which)
RuntimeScriptValue Sc_Mouse_IsModeEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Mouse_IsModeEnabled);
}

// void ();
RuntimeScriptValue Sc_Mouse_SaveCursorForLocationChange(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SaveCursorForLocationChange);
}

// void  ()
RuntimeScriptValue Sc_Mouse_SetNextCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetNextCursor);
}

// void  ()
RuntimeScriptValue Sc_Mouse_SetPreviousCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetPreviousCursor);
}

// void  (int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_Mouse_SetBounds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(Mouse_SetBounds);
}

// void  (int newx, int newy)
RuntimeScriptValue Sc_Mouse_SetPosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Mouse_SetPosition);
}

// void ()
RuntimeScriptValue Sc_Mouse_Refresh(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_Refresh);
}

// void ()
RuntimeScriptValue Sc_Mouse_SetDefaultCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Mouse_SetDefaultCursor);
}

// void (int newcurs)
RuntimeScriptValue Sc_Mouse_SetCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetCursor);
}

// int ()
RuntimeScriptValue Sc_Mouse_GetCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Mouse_GetCursorMode);
}

// void (int newmode)
RuntimeScriptValue Sc_Mouse_SetCursorMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetCursorMode);
}

// int ()
RuntimeScriptValue Sc_Mouse_GetVisible(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Mouse_GetVisible);
}

// void (int isOn)
RuntimeScriptValue Sc_Mouse_SetVisible(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SetVisible);
}

RuntimeScriptValue Sc_Mouse_SimulateClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Mouse_SimulateClick);
}

RuntimeScriptValue Sc_Mouse_GetControlEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse::IsControlEnabled);
}

RuntimeScriptValue Sc_Mouse_SetControlEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Mouse_EnableControl);
}

RuntimeScriptValue Sc_Mouse_GetAutoLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse_GetAutoLock);
}

RuntimeScriptValue Sc_Mouse_SetAutoLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Mouse_SetAutoLock);
}

RuntimeScriptValue Sc_Mouse_GetEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Mouse_GetEnabled);
}

RuntimeScriptValue Sc_Mouse_SetEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Mouse_SetEnabled);
}

RuntimeScriptValue Sc_Mouse_GetCursorShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptShaderInstance, Mouse_GetCursorShader);
}

RuntimeScriptValue Sc_Mouse_SetCursorShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Mouse_SetCursorShader, ScriptShaderInstance);
}


RuntimeScriptValue Sc_Mouse_GetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Mouse::GetSpeed);
}

RuntimeScriptValue Sc_Mouse_SetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT("Mouse::Speed", 1);
    Mouse::SetSpeed(params[0].FValue);
    return RuntimeScriptValue();
}

void RegisterMouseAPI()
{
    ScFnRegister mouse_api[] = {
        {"Mouse::ChangeModeGraphic^2", API_FN_PAIR(Mouse_ChangeCursorGraphic)},
        {"Mouse::ChangeModeHotspot^3", API_FN_PAIR(Mouse_ChangeCursorHotspot)},
        {"Mouse::ChangeModeView^2", API_FN_PAIR(Mouse_ChangeModeView2)},
        {"Mouse::ChangeModeView^3", API_FN_PAIR(Mouse_ChangeModeView)},
        {"Mouse::Click^1", API_FN_PAIR(Mouse_SimulateClick)},
        {"Mouse::DisableMode^1", API_FN_PAIR(Mouse_DisableCursorMode)},
        {"Mouse::EnableMode^1", API_FN_PAIR(Mouse_EnableCursorMode)},
        {"Mouse::GetModeGraphic^1", API_FN_PAIR(Mouse_GetModeGraphic)},
        {"Mouse::GetModeHotspot^1", API_FN_PAIR(Mouse_GetModeHotspot)},
        {"Mouse::IsButtonDown^1", API_FN_PAIR(Mouse_IsButtonDown)},
        {"Mouse::IsAnyButtonDown^0", API_FN_PAIR(IsAnyButtonDown)},
        {"Mouse::IsModeEnabled^1", API_FN_PAIR(Mouse_IsModeEnabled)},
        {"Mouse::SaveCursorUntilItLeaves^0", API_FN_PAIR(Mouse_SaveCursorForLocationChange)},
        {"Mouse::SelectNextMode^0", API_FN_PAIR(Mouse_SetNextCursor)},
        {"Mouse::SelectPreviousMode^0", API_FN_PAIR(Mouse_SetPreviousCursor)},
        {"Mouse::SetBounds^4", API_FN_PAIR(Mouse_SetBounds)},
        {"Mouse::SetPosition^2", API_FN_PAIR(Mouse_SetPosition)},
        {"Mouse::Update^0", API_FN_PAIR(Mouse_Refresh)},
        {"Mouse::UseDefaultGraphic^0", API_FN_PAIR(Mouse_SetDefaultCursor)},
        {"Mouse::UseModeGraphic^1", API_FN_PAIR(Mouse_SetCursor)},
        {"Mouse::get_AutoLock", API_FN_PAIR(Mouse_GetAutoLock)},
        {"Mouse::set_AutoLock", API_FN_PAIR(Mouse_SetAutoLock)},
        {"Mouse::get_ControlEnabled", Sc_Mouse_GetControlEnabled, Mouse::IsControlEnabled},
        {"Mouse::set_ControlEnabled", Sc_Mouse_SetControlEnabled, Mouse_EnableControl},
        {"Mouse::get_Enabled", API_FN_PAIR(Mouse_GetEnabled)},
        {"Mouse::set_Enabled", API_FN_PAIR(Mouse_SetEnabled)},
        {"Mouse::get_CursorShader", API_FN_PAIR(Mouse_GetCursorShader)},
        {"Mouse::set_CursorShader", API_FN_PAIR(Mouse_SetCursorShader)},
        {"Mouse::get_Mode", API_FN_PAIR(Mouse_GetCursorMode)},
        {"Mouse::set_Mode", API_FN_PAIR(Mouse_SetCursorMode)},
        {"Mouse::get_Speed", Sc_Mouse_GetSpeed, Mouse::GetSpeed},
        {"Mouse::set_Speed", Sc_Mouse_SetSpeed, Mouse::SetSpeed},
        {"Mouse::get_Visible", API_FN_PAIR(Mouse_GetVisible)},
        {"Mouse::set_Visible", API_FN_PAIR(Mouse_SetVisible)},
    };

    ccAddExternalFunctions(mouse_api);
}
