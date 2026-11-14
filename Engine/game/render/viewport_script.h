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
// Viewport and Camera script API.
//
//=============================================================================
#pragma once

class ScriptShaderInstance;
struct ScriptCamera;
struct ScriptUserObject;
struct ScriptViewport;

// Script API: Camera::Create
ScriptCamera *Camera_Create();

// Script API: Camera::Delete
void Camera_Delete(ScriptCamera *script_cam);

// Script API: Camera::get_X
int Camera_GetX(ScriptCamera *script_cam);

// Script API: Camera::set_X
void Camera_SetX(ScriptCamera *script_cam, int x);

// Script API: Camera::get_Y
int Camera_GetY(ScriptCamera *script_cam);

// Script API: Camera::set_Y
void Camera_SetY(ScriptCamera *script_cam, int y);

// Script API: Camera::get_Width
int Camera_GetWidth(ScriptCamera *script_cam);

// Script API: Camera::set_Width
void Camera_SetWidth(ScriptCamera *script_cam, int width);

// Script API: Camera::get_Height
int Camera_GetHeight(ScriptCamera *script_cam);

// Script API: Camera::set_Height
void Camera_SetHeight(ScriptCamera *script_cam, int height);

// Script API: Camera::get_Rotation
float Camera_GetRotation(ScriptCamera *script_cam);

// Script API: Camera::set_Rotation
void Camera_SetRotation(ScriptCamera *script_cam, float rotation);

// Script API: Camera::get_PivotX
float Camera_GetPivotX(ScriptCamera *script_cam);

// Script API: Camera::set_PivotX
void Camera_SetPivotX(ScriptCamera *script_cam, float pivotx);

// Script API: Camera::get_PivotY
float Camera_GetPivotY(ScriptCamera *script_cam);

// Script API: Camera::set_PivotY
void Camera_SetPivotY(ScriptCamera *script_cam, float pivoty);

// Script API: Camera::get_PivotOffsetX
int Camera_GetPivotOffsetX(ScriptCamera *script_cam);

// Script API: Camera::set_PivotOffsetX
void Camera_SetPivotOffsetX(ScriptCamera *script_cam, int px);

// Script API: Camera::get_PivotOffsetY
int Camera_GetPivotOffsetY(ScriptCamera *script_cam);

// Script API: Camera::set_PivotOffsetY
void Camera_SetPivotOffsetY(ScriptCamera *script_cam, int py);

// Script API: Camera::get_AutoTracking
bool Camera_GetAutoTracking(ScriptCamera *script_cam);

// Script API: Camera::set_AutoTracking
void Camera_SetAutoTracking(ScriptCamera *script_cam, bool on);

// Script API: Camera::SetAt
void Camera_SetAt(ScriptCamera *script_cam, int x, int y);

// Script API: Camera::SetSize
void Camera_SetSize(ScriptCamera *script_cam, int width, int height);

// Script API: Camera::get_Shader
ScriptShaderInstance *Camera_GetShader(ScriptCamera *script_cam);

// Script API: Camera::set_Shader
void Camera_SetShader(ScriptCamera *script_cam, ScriptShaderInstance *shader_inst);

// Script API: Viewport::Create
ScriptViewport *Viewport_Create();

// Script API: Viewport::Delete
void Viewport_Delete(ScriptViewport *script_view);

// Script API: Viewport::get_X
int Viewport_GetX(ScriptViewport *script_view);

// Script API: Viewport::set_X
void Viewport_SetX(ScriptViewport *script_view, int x);

// Script API: Viewport::get_Y
int Viewport_GetY(ScriptViewport *script_view);

// Script API: Viewport::set_Y
void Viewport_SetY(ScriptViewport *script_view, int y);

// Script API: Viewport::get_Width
int Viewport_GetWidth(ScriptViewport *script_view);

// Script API: Viewport::set_Width
void Viewport_SetWidth(ScriptViewport *script_view, int width);

// Script API: Viewport::get_Height
int Viewport_GetHeight(ScriptViewport *script_view);

// Script API: Viewport::set_Height
void Viewport_SetHeight(ScriptViewport *script_view, int height);

// Script API: Viewport::get_Camera
ScriptCamera *Viewport_GetCamera(ScriptViewport *script_view);

// Script API: Viewport::set_Camera
void Viewport_SetCamera(ScriptViewport *script_view, ScriptCamera *script_cam);

// Script API: Viewport::get_Visible
bool Viewport_GetVisible(ScriptViewport *script_view);

// Script API: Viewport::set_Visible
void Viewport_SetVisible(ScriptViewport *script_view, bool on);

// Script API: Viewport::get_ZOrder
int Viewport_GetZOrder(ScriptViewport *script_view);

// Script API: Viewport::set_ZOrder
void Viewport_SetZOrder(ScriptViewport *script_view, int zorder);

// Script API: Viewport::get_Shader
ScriptShaderInstance *Viewport_GetShader(ScriptViewport *script_view);

// Script API: Viewport::set_Shader
void Viewport_SetShader(ScriptViewport *script_view, ScriptShaderInstance *shader_inst);

// Script API: Viewport::GetAtScreenXY
ScriptViewport *Viewport_GetAtScreenXY(int x, int y);

// Script API: Viewport::SetPosition
void Viewport_SetPosition(ScriptViewport *script_view, int x, int y, int width, int height);

// Script API: Viewport::ScreenToRoomPoint
ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *script_view, int scrx, int scry, bool clip_viewport);

// Script API: Viewport::RoomToScreenPoint
ScriptUserObject *Viewport_RoomToScreenPoint(ScriptViewport *script_view, int roomx, int roomy, bool clip_viewport);
