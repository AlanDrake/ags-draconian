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
// Script bindings registered by RegisterViewportAPI().
//
//=============================================================================
#include "game/dynobj/script_camera.h"
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_viewport.h"
#include "game/render/viewport_script.h"
#include "game/script_api/bindings.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterViewportAPI()
{
    ScFnRegister camera_api[] = {
        ScStatic<&Camera_Create>("Camera::Create"),
        ScMethod<&Camera_Delete>("Camera::Delete"),
        ScMethod<&Camera_GetX>("Camera::get_X"),
        ScMethod<&Camera_SetX>("Camera::set_X"),
        ScMethod<&Camera_GetY>("Camera::get_Y"),
        ScMethod<&Camera_SetY>("Camera::set_Y"),
        ScMethod<&Camera_GetWidth>("Camera::get_Width"),
        ScMethod<&Camera_SetWidth>("Camera::set_Width"),
        ScMethod<&Camera_GetHeight>("Camera::get_Height"),
        ScMethod<&Camera_SetHeight>("Camera::set_Height"),
        ScMethod<&Camera_GetRotation>("Camera::get_Rotation"),
        ScMethod<&Camera_SetRotation, ScUndefinedResult>("Camera::set_Rotation"),
        ScMethod<&Camera_GetPivotX>("Camera::get_PivotX"),
        ScMethod<&Camera_SetPivotX, ScUndefinedResult>("Camera::set_PivotX"),
        ScMethod<&Camera_GetPivotY>("Camera::get_PivotY"),
        ScMethod<&Camera_SetPivotY, ScUndefinedResult>("Camera::set_PivotY"),
        ScMethod<&Camera_GetPivotOffsetX>("Camera::get_PivotOffsetX"),
        ScMethod<&Camera_SetPivotOffsetX>("Camera::set_PivotOffsetX"),
        ScMethod<&Camera_GetPivotOffsetY>("Camera::get_PivotOffsetY"),
        ScMethod<&Camera_SetPivotOffsetY>("Camera::set_PivotOffsetY"),
        ScMethod<&Camera_GetAutoTracking>("Camera::get_AutoTracking"),
        ScMethod<&Camera_SetAutoTracking>("Camera::set_AutoTracking"),
        ScMethod<&Camera_SetAt>("Camera::SetAt"),
        ScMethod<&Camera_SetSize>("Camera::SetSize"),
        ScMethod<&Camera_GetShader>("Camera::get_Shader"),
        ScMethod<&Camera_SetShader>("Camera::set_Shader"),
    };

    ccAddExternalFunctions(camera_api);

    ScFnRegister viewport_api[] = {
        ScStatic<&Viewport_Create>("Viewport::Create"),
        ScMethod<&Viewport_Delete>("Viewport::Delete"),
        ScMethod<&Viewport_GetX>("Viewport::get_X"),
        ScMethod<&Viewport_SetX>("Viewport::set_X"),
        ScMethod<&Viewport_GetY>("Viewport::get_Y"),
        ScMethod<&Viewport_SetY>("Viewport::set_Y"),
        ScMethod<&Viewport_GetWidth>("Viewport::get_Width"),
        ScMethod<&Viewport_SetWidth>("Viewport::set_Width"),
        ScMethod<&Viewport_GetHeight>("Viewport::get_Height"),
        ScMethod<&Viewport_SetHeight>("Viewport::set_Height"),
        ScMethod<&Viewport_GetCamera>("Viewport::get_Camera"),
        ScMethod<&Viewport_SetCamera>("Viewport::set_Camera"),
        ScMethod<&Viewport_GetVisible>("Viewport::get_Visible"),
        ScMethod<&Viewport_SetVisible>("Viewport::set_Visible"),
        ScMethod<&Viewport_GetZOrder>("Viewport::get_ZOrder"),
        ScMethod<&Viewport_SetZOrder>("Viewport::set_ZOrder"),
        ScMethod<&Viewport_GetShader>("Viewport::get_Shader"),
        ScMethod<&Viewport_SetShader>("Viewport::set_Shader"),
        ScStatic<&Viewport_GetAtScreenXY>("Viewport::GetAtScreenXY"),
        ScMethod<&Viewport_SetPosition>("Viewport::SetPosition"),
        ScMethod<&Viewport_ScreenToRoomPoint>("Viewport::ScreenToRoomPoint"),
        ScMethod<&Viewport_RoomToScreenPoint>("Viewport::RoomToScreenPoint"),
    };

    ccAddExternalFunctions(viewport_api);
}
