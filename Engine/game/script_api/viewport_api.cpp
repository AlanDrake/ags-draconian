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
#include "script/dynobj/script_user_object.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_Camera_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptCamera, Camera_Create);
}

RuntimeScriptValue Sc_Camera_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptCamera, Camera_Delete);
}

RuntimeScriptValue Sc_Camera_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetX);
}

RuntimeScriptValue Sc_Camera_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetX);
}

RuntimeScriptValue Sc_Camera_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetY);
}

RuntimeScriptValue Sc_Camera_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetY);
}

RuntimeScriptValue Sc_Camera_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetWidth);
}

RuntimeScriptValue Sc_Camera_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetWidth);
}

RuntimeScriptValue Sc_Camera_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetHeight);
}

RuntimeScriptValue Sc_Camera_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetHeight);
}

RuntimeScriptValue Sc_Camera_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetRotation);
}

RuntimeScriptValue Sc_Camera_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetRotation);
}

RuntimeScriptValue Sc_Camera_GetPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetPivotX);
}

RuntimeScriptValue Sc_Camera_SetPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetPivotX);
}

RuntimeScriptValue Sc_Camera_GetPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptCamera, Camera_GetPivotY);
}

RuntimeScriptValue Sc_Camera_SetPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptCamera, Camera_SetPivotY);
}

RuntimeScriptValue Sc_Camera_GetPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetPivotOffsetX);
}

RuntimeScriptValue Sc_Camera_SetPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetPivotOffsetX);
}

RuntimeScriptValue Sc_Camera_GetPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptCamera, Camera_GetPivotOffsetY);
}

RuntimeScriptValue Sc_Camera_SetPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptCamera, Camera_SetPivotOffsetY);
}

RuntimeScriptValue Sc_Camera_GetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptCamera, Camera_GetAutoTracking);
}

RuntimeScriptValue Sc_Camera_SetAutoTracking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptCamera, Camera_SetAutoTracking);
}

RuntimeScriptValue Sc_Camera_SetAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptCamera, Camera_SetAt);
}

RuntimeScriptValue Sc_Camera_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptCamera, Camera_SetSize);
}

RuntimeScriptValue Sc_Camera_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptCamera, ScriptShaderInstance, Camera_GetShader);
}

RuntimeScriptValue Sc_Camera_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptCamera, Camera_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_Viewport_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptViewport, Viewport_Create);
}

RuntimeScriptValue Sc_Viewport_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptViewport, Viewport_Delete);
}

RuntimeScriptValue Sc_Viewport_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetX);
}

RuntimeScriptValue Sc_Viewport_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetX);
}

RuntimeScriptValue Sc_Viewport_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetY);
}

RuntimeScriptValue Sc_Viewport_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetY);
}

RuntimeScriptValue Sc_Viewport_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetWidth);
}

RuntimeScriptValue Sc_Viewport_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetWidth);
}

RuntimeScriptValue Sc_Viewport_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetHeight);
}

RuntimeScriptValue Sc_Viewport_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetHeight);
}

RuntimeScriptValue Sc_Viewport_GetCamera(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptViewport, ScriptCamera, Viewport_GetCamera);
}

RuntimeScriptValue Sc_Viewport_SetCamera(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewport, Viewport_SetCamera, ScriptCamera);
}

RuntimeScriptValue Sc_Viewport_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptViewport, Viewport_GetVisible);
}

RuntimeScriptValue Sc_Viewport_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptViewport, Viewport_SetVisible);
}

RuntimeScriptValue Sc_Viewport_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewport, Viewport_GetZOrder);
}

RuntimeScriptValue Sc_Viewport_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewport, Viewport_SetZOrder);
}

RuntimeScriptValue Sc_Viewport_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptViewport, ScriptShaderInstance, Viewport_GetShader);
}

RuntimeScriptValue Sc_Viewport_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewport, Viewport_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_Viewport_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptViewport, Viewport_GetAtScreenXY);
}

RuntimeScriptValue Sc_Viewport_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptViewport, Viewport_SetPosition);
}

RuntimeScriptValue Sc_Viewport_ScreenToRoomPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Viewport_ScreenToRoomPoint);
}

RuntimeScriptValue Sc_Viewport_RoomToScreenPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptViewport, ScriptUserObject, Viewport_RoomToScreenPoint);
}


void RegisterViewportAPI()
{
    ScFnRegister camera_api[] = {
        {"Camera::Create", API_FN_PAIR(Camera_Create)},
        {"Camera::Delete", API_FN_PAIR(Camera_Delete)},
        {"Camera::get_X", API_FN_PAIR(Camera_GetX)},
        {"Camera::set_X", API_FN_PAIR(Camera_SetX)},
        {"Camera::get_Y", API_FN_PAIR(Camera_GetY)},
        {"Camera::set_Y", API_FN_PAIR(Camera_SetY)},
        {"Camera::get_Width", API_FN_PAIR(Camera_GetWidth)},
        {"Camera::set_Width", API_FN_PAIR(Camera_SetWidth)},
        {"Camera::get_Height", API_FN_PAIR(Camera_GetHeight)},
        {"Camera::set_Height", API_FN_PAIR(Camera_SetHeight)},
        {"Camera::get_Rotation", API_FN_PAIR(Camera_GetRotation)},
        {"Camera::set_Rotation", API_FN_PAIR(Camera_SetRotation)},
        {"Camera::get_PivotX", API_FN_PAIR(Camera_GetPivotX)},
        {"Camera::set_PivotX", API_FN_PAIR(Camera_SetPivotX)},
        {"Camera::get_PivotY", API_FN_PAIR(Camera_GetPivotY)},
        {"Camera::set_PivotY", API_FN_PAIR(Camera_SetPivotY)},
        {"Camera::get_PivotOffsetX", API_FN_PAIR(Camera_GetPivotOffsetX)},
        {"Camera::set_PivotOffsetX", API_FN_PAIR(Camera_SetPivotOffsetX)},
        {"Camera::get_PivotOffsetY", API_FN_PAIR(Camera_GetPivotOffsetY)},
        {"Camera::set_PivotOffsetY", API_FN_PAIR(Camera_SetPivotOffsetY)},
        {"Camera::get_AutoTracking", API_FN_PAIR(Camera_GetAutoTracking)},
        {"Camera::set_AutoTracking", API_FN_PAIR(Camera_SetAutoTracking)},
        {"Camera::SetAt", API_FN_PAIR(Camera_SetAt)},
        {"Camera::SetSize", API_FN_PAIR(Camera_SetSize)},
        {"Camera::get_Shader", API_FN_PAIR(Camera_GetShader)},
        {"Camera::set_Shader", API_FN_PAIR(Camera_SetShader)},
    };

    ccAddExternalFunctions(camera_api);

    ScFnRegister viewport_api[] = {
        {"Viewport::Create", API_FN_PAIR(Viewport_Create)},
        {"Viewport::Delete", API_FN_PAIR(Viewport_Delete)},
        {"Viewport::get_X", API_FN_PAIR(Viewport_GetX)},
        {"Viewport::set_X", API_FN_PAIR(Viewport_SetX)},
        {"Viewport::get_Y", API_FN_PAIR(Viewport_GetY)},
        {"Viewport::set_Y", API_FN_PAIR(Viewport_SetY)},
        {"Viewport::get_Width", API_FN_PAIR(Viewport_GetWidth)},
        {"Viewport::set_Width", API_FN_PAIR(Viewport_SetWidth)},
        {"Viewport::get_Height", API_FN_PAIR(Viewport_GetHeight)},
        {"Viewport::set_Height", API_FN_PAIR(Viewport_SetHeight)},
        {"Viewport::get_Camera", API_FN_PAIR(Viewport_GetCamera)},
        {"Viewport::set_Camera", API_FN_PAIR(Viewport_SetCamera)},
        {"Viewport::get_Visible", API_FN_PAIR(Viewport_GetVisible)},
        {"Viewport::set_Visible", API_FN_PAIR(Viewport_SetVisible)},
        {"Viewport::get_ZOrder", API_FN_PAIR(Viewport_GetZOrder)},
        {"Viewport::set_ZOrder", API_FN_PAIR(Viewport_SetZOrder)},
        {"Viewport::get_Shader", API_FN_PAIR(Viewport_GetShader)},
        {"Viewport::set_Shader", API_FN_PAIR(Viewport_SetShader)},
        {"Viewport::GetAtScreenXY", API_FN_PAIR(Viewport_GetAtScreenXY)},
        {"Viewport::SetPosition", API_FN_PAIR(Viewport_SetPosition)},
        {"Viewport::ScreenToRoomPoint", API_FN_PAIR(Viewport_ScreenToRoomPoint)},
        {"Viewport::RoomToScreenPoint", API_FN_PAIR(Viewport_RoomToScreenPoint)},
    };

    ccAddExternalFunctions(viewport_api);
}
