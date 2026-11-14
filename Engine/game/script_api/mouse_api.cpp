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
#include "game/script_api/bindings.h"
#include "game/ui/mouse.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterMouseAPI()
{
    ScFnRegister mouse_api[] = {
        ScStatic<&Mouse_ChangeCursorGraphic>("Mouse::ChangeModeGraphic^2"),
        ScStatic<&Mouse_ChangeCursorHotspot>("Mouse::ChangeModeHotspot^3"),
        ScStatic<&Mouse_ChangeModeView2>("Mouse::ChangeModeView^2"),
        ScStatic<&Mouse_ChangeModeView>("Mouse::ChangeModeView^3"),
        ScStatic<&Mouse_SimulateClick>("Mouse::Click^1"),
        ScStatic<&Mouse_DisableCursorMode>("Mouse::DisableMode^1"),
        ScStatic<&Mouse_EnableCursorMode>("Mouse::EnableMode^1"),
        ScStatic<&Mouse_GetModeGraphic>("Mouse::GetModeGraphic^1"),
        ScStatic<&Mouse_GetModeHotspot>("Mouse::GetModeHotspot^1"),
        ScStatic<&Mouse_IsButtonDown>("Mouse::IsButtonDown^1"),
        ScStatic<&IsAnyButtonDown>("Mouse::IsAnyButtonDown^0"),
        ScStatic<&Mouse_IsModeEnabled>("Mouse::IsModeEnabled^1"),
        ScStatic<&Mouse_SaveCursorForLocationChange>("Mouse::SaveCursorUntilItLeaves^0"),
        ScStatic<&Mouse_SetNextCursor>("Mouse::SelectNextMode^0"),
        ScStatic<&Mouse_SetPreviousCursor>("Mouse::SelectPreviousMode^0"),
        ScStatic<&Mouse_SetBounds>("Mouse::SetBounds^4"),
        ScStatic<&Mouse_SetPosition>("Mouse::SetPosition^2"),
        ScStatic<&Mouse_Refresh>("Mouse::Update^0"),
        ScStatic<&Mouse_SetDefaultCursor>("Mouse::UseDefaultGraphic^0"),
        ScStatic<&Mouse_SetCursor>("Mouse::UseModeGraphic^1"),
        ScStatic<&Mouse_GetAutoLock>("Mouse::get_AutoLock"),
        ScStatic<&Mouse_SetAutoLock>("Mouse::set_AutoLock"),
        ScStatic<&Mouse::IsControlEnabled>("Mouse::get_ControlEnabled"),
        ScStatic<&Mouse_EnableControl>("Mouse::set_ControlEnabled"),
        ScStatic<&Mouse_GetEnabled>("Mouse::get_Enabled"),
        ScStatic<&Mouse_SetEnabled>("Mouse::set_Enabled"),
        ScStatic<&Mouse_GetCursorShader>("Mouse::get_CursorShader"),
        ScStatic<&Mouse_SetCursorShader>("Mouse::set_CursorShader"),
        ScStatic<&Mouse_GetCursorMode>("Mouse::get_Mode"),
        ScStatic<&Mouse_SetCursorMode>("Mouse::set_Mode"),
        ScStatic<&Mouse::GetSpeed>("Mouse::get_Speed"),
        ScStatic<&Mouse::SetSpeed, ScUndefinedResult>("Mouse::set_Speed"),
        ScStatic<&Mouse_GetVisible>("Mouse::get_Visible"),
        ScStatic<&Mouse_SetVisible>("Mouse::set_Visible"),
    };

    ccAddExternalFunctions(mouse_api);
}
