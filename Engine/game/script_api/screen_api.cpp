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
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_viewport.h"
#include "game/render/screen.h"
#include "game/script_api/bindings.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterScreenAPI()
{
    ScFnRegister screen_api[] = {
        ScStatic<&Screen_GetScreenHeight>("Screen::get_Height"),
        ScStatic<&Screen_GetScreenWidth>("Screen::get_Width"),
        ScStatic<&Screen_GetAutoSizeViewport>("Screen::get_AutoSizeViewportOnRoomLoad"),
        ScStatic<&Screen_SetAutoSizeViewport>("Screen::set_AutoSizeViewportOnRoomLoad"),
        ScStatic<&Screen_GetViewport>("Screen::get_Viewport"),
        ScStatic<&Screen_GetViewportCount>("Screen::get_ViewportCount"),
        ScStatic<&Screen_GetAnyViewport>("Screen::geti_Viewports"),
        ScStatic<&Screen_GetShader>("Screen::get_Shader"),
        ScStatic<&Screen_SetShader>("Screen::set_Shader"),
        ScStatic<&Screen_ScreenToRoomPoint2>("Screen::ScreenToRoomPoint^2"),
        ScStatic<&Screen_ScreenToRoomPoint>("Screen::ScreenToRoomPoint^3"),
        ScStatic<&Screen_RoomToScreenPoint>("Screen::RoomToScreenPoint"),
    };

    ccAddExternalFunctions(screen_api);
}
