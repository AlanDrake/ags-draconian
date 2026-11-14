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
// Script bindings registered by RegisterScreenAPI().
//
//=============================================================================
#include "ac/dynobj/scriptuserobject.h"
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_viewport.h"
#include "game/render/screen.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Screen_GetScreenHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetScreenHeight);
}

RuntimeScriptValue Sc_Screen_GetScreenWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetScreenWidth);
}

RuntimeScriptValue Sc_Screen_GetAutoSizeViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Screen_GetAutoSizeViewport);
}

RuntimeScriptValue Sc_Screen_SetAutoSizeViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Screen_SetAutoSizeViewport);
}

RuntimeScriptValue Sc_Screen_GetViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptViewport, Screen_GetViewport);
}

RuntimeScriptValue Sc_Screen_GetViewportCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Screen_GetViewportCount);
}

RuntimeScriptValue Sc_Screen_GetAnyViewport(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptViewport, Screen_GetAnyViewport);
}

RuntimeScriptValue Sc_Screen_GetShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptShaderInstance, Screen_GetShader);
}

RuntimeScriptValue Sc_Screen_SetShader(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Screen_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_Screen_ScreenToRoomPoint2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptUserObject, Screen_ScreenToRoomPoint2);
}

RuntimeScriptValue Sc_Screen_ScreenToRoomPoint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2_PBOOL(ScriptUserObject, Screen_ScreenToRoomPoint);
}

RuntimeScriptValue Sc_Screen_RoomToScreenPoint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptUserObject, Screen_RoomToScreenPoint);
}

void RegisterScreenAPI()
{
    ScFnRegister screen_api[] = {
        {"Screen::get_Height", API_FN_PAIR(Screen_GetScreenHeight)},
        {"Screen::get_Width", API_FN_PAIR(Screen_GetScreenWidth)},
        {"Screen::get_AutoSizeViewportOnRoomLoad", API_FN_PAIR(Screen_GetAutoSizeViewport)},
        {"Screen::set_AutoSizeViewportOnRoomLoad", API_FN_PAIR(Screen_SetAutoSizeViewport)},
        {"Screen::get_Viewport", API_FN_PAIR(Screen_GetViewport)},
        {"Screen::get_ViewportCount", API_FN_PAIR(Screen_GetViewportCount)},
        {"Screen::geti_Viewports", API_FN_PAIR(Screen_GetAnyViewport)},
        {"Screen::get_Shader", API_FN_PAIR(Screen_GetShader)},
        {"Screen::set_Shader", API_FN_PAIR(Screen_SetShader)},
        {"Screen::ScreenToRoomPoint^2", API_FN_PAIR(Screen_ScreenToRoomPoint2)},
        {"Screen::ScreenToRoomPoint^3", API_FN_PAIR(Screen_ScreenToRoomPoint)},
        {"Screen::RoomToScreenPoint", API_FN_PAIR(Screen_RoomToScreenPoint)},
    };

    ccAddExternalFunctions(screen_api);
}
