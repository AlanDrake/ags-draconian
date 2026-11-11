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

#include "ac/draw.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/dynobj/scriptviewport.h"
#include "ac/gamestate.h"
#include "debug/debug_log.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

//=============================================================================
//
// Camera script API.
//
// A ScriptCamera is a handle to a room camera in the game state (GamePlayState),
// found by its ID. Deleting a camera invalidates the handle (ID becomes -1):
// every function below but Delete (which ignores it) then warns and does nothing,
// or returns a null or zero value.
//
//=============================================================================

// Creates a new room camera and returns its script handle, or null on failure.
ScriptCamera *Camera_Create()
{
    auto cam = play.CreateRoomCamera();
    if (!cam)
        return nullptr;
    return play.RegisterRoomCamera(cam->GetID());
}

void Camera_Delete(ScriptCamera *script_cam)
{
    play.DeleteRoomCamera(script_cam->GetID());
}

int Camera_GetX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.X: trying to use deleted camera");
        return 0;
    }
    return play.GetRoomCamera(script_cam->GetID())->GetRect().Left;
}

void Camera_SetX(ScriptCamera *script_cam, int x)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.X: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->LockAt(x, cam->GetRect().Top);
}

int Camera_GetY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Y: trying to use deleted camera");
        return 0;
    }
    return play.GetRoomCamera(script_cam->GetID())->GetRect().Top;
}

void Camera_SetY(ScriptCamera *script_cam, int y)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Y: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->LockAt(cam->GetRect().Left, y);
}

int Camera_GetWidth(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Width: trying to use deleted camera");
        return 0;
    }
    return play.GetRoomCamera(script_cam->GetID())->GetRect().GetWidth();
}

void Camera_SetWidth(ScriptCamera *script_cam, int width)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Width: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetSize(Size(width, cam->GetRect().GetHeight()));
}

int Camera_GetHeight(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Height: trying to use deleted camera");
        return 0;
    }
    return play.GetRoomCamera(script_cam->GetID())->GetRect().GetHeight();
}

void Camera_SetHeight(ScriptCamera *script_cam, int height)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Height: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetSize(Size(cam->GetRect().GetWidth(), height));
}

float Camera_GetRotation(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Rotation: trying to use deleted camera");
        return 0;
    }
    return play.GetRoomCamera(script_cam->GetID())->GetRotation();
}

void Camera_SetRotation(ScriptCamera *script_cam, float rotation)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Rotation: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetRotation(rotation);
}

float Camera_GetPivotX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotX: trying to use deleted camera");
        return 0.f;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivot().X;
}

void Camera_SetPivotX(ScriptCamera *script_cam, float pivotx)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotX: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetPivot(Pointf(pivotx, cam->GetPivot().Y));
}

float Camera_GetPivotY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotY: trying to use deleted camera");
        return 0.f;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivot().Y;
}

void Camera_SetPivotY(ScriptCamera *script_cam, float pivoty)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotY: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetPivot(Pointf(cam->GetPivot().X, pivoty));
}

int Camera_GetPivotOffsetX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetX: trying to use deleted camera");
        return 0;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivotOffset().X;
}

void Camera_SetPivotOffsetX(ScriptCamera *script_cam, int px)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetX: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetPivotOffset(Point(px, cam->GetPivotOffset().Y));
}

int Camera_GetPivotOffsetY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetY: trying to use deleted camera");
        return 0;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivotOffset().Y;
}

void Camera_SetPivotOffsetY(ScriptCamera *script_cam, int py)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetY: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetPivotOffset(Point(cam->GetPivotOffset().X, py));
}

bool Camera_GetAutoTracking(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.AutoTracking: trying to use deleted camera");
        return false;
    }
    return !play.GetRoomCamera(script_cam->GetID())->IsLocked();
}

void Camera_SetAutoTracking(ScriptCamera *script_cam, bool on)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.AutoTracking: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    if (on)
        cam->Release();
    else
        cam->Lock();
}

void Camera_SetAt(ScriptCamera *script_cam, int x, int y)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.SetAt: trying to use deleted camera");
        return;
    }
    play.GetRoomCamera(script_cam->GetID())->LockAt(x, y);
}

void Camera_SetSize(ScriptCamera *script_cam, int width, int height)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.SetSize: trying to use deleted camera");
        return;
    }
    play.GetRoomCamera(script_cam->GetID())->SetSize(Size(width, height));
}

ScriptShaderInstance *Camera_GetShader(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Shader: trying to use deleted camera");
        return nullptr;
    }
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(play.GetRoomCamera(script_cam->GetID())->GetShaderHandle()));
}

void Camera_SetShader(ScriptCamera *script_cam, ScriptShaderInstance *shader_inst)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Shader: trying to use deleted camera");
        return;
    }
    auto cam = play.GetRoomCamera(script_cam->GetID());
    cam->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                   ccReplaceObjectHandle(cam->GetShaderHandle(), shader_inst));
}

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


//=============================================================================
//
// Viewport script API.
//
// A ScriptViewport is a handle to a room viewport in the game state, with the
// same rules as ScriptCamera above: a deleted viewport has the ID -1.
//
//=============================================================================

// Creates a new room viewport and returns its script handle, or null on failure.
ScriptViewport *Viewport_Create()
{
    auto view = play.CreateRoomViewport();
    if (!view)
        return nullptr;
    return play.RegisterRoomViewport(view->GetID());
}

void Viewport_Delete(ScriptViewport *script_view)
{
    play.DeleteRoomViewport(script_view->GetID());
}

int Viewport_GetX(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.X: trying to use deleted viewport");
        return 0;
    }
    return play.GetRoomViewport(script_view->GetID())->GetRect().Left;
}

void Viewport_SetX(ScriptViewport *script_view, int x)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.X: trying to use deleted viewport");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    view->SetAt(x, view->GetRect().Top);
}

int Viewport_GetY(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Y: trying to use deleted viewport");
        return 0;
    }
    return play.GetRoomViewport(script_view->GetID())->GetRect().Top;
}

void Viewport_SetY(ScriptViewport *script_view, int y)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Y: trying to use deleted viewport");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    view->SetAt(view->GetRect().Left, y);
}

int Viewport_GetWidth(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Width: trying to use deleted viewport");
        return 0;
    }
    return play.GetRoomViewport(script_view->GetID())->GetRect().GetWidth();
}

void Viewport_SetWidth(ScriptViewport *script_view, int width)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Width: trying to use deleted viewport");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    view->SetSize(Size(width, view->GetRect().GetHeight()));
}

int Viewport_GetHeight(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Height: trying to use deleted viewport");
        return 0;
    }
    return play.GetRoomViewport(script_view->GetID())->GetRect().GetHeight();
}

void Viewport_SetHeight(ScriptViewport *script_view, int height)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Height: trying to use deleted viewport");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    view->SetSize(Size(view->GetRect().GetWidth(), height));
}

ScriptCamera *Viewport_GetCamera(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Camera: trying to use deleted viewport");
        return nullptr;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    auto cam = view->GetCamera();
    if (!cam)
        return nullptr;
    return play.GetScriptCamera(cam->GetID());
}

void Viewport_SetCamera(ScriptViewport *script_view, ScriptCamera *script_cam)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Camera: trying to use deleted viewport");
        return;
    }
    if (script_cam != nullptr && script_cam->GetID() < 0)
    {
        debug_script_warn("Viewport.Camera: trying to link deleted camera");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    // unlink the previous camera
    auto cam = view->GetCamera();
    if (cam)
        cam->UnlinkFromViewport(view->GetID());
    // link the new camera; null leaves the viewport without a camera
    if (script_cam != nullptr)
    {
        cam = play.GetRoomCamera(script_cam->GetID());
        view->LinkCamera(cam);
        cam->LinkToViewport(view);
    }
    else
    {
        view->LinkCamera(nullptr);
    }
}

bool Viewport_GetVisible(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Visible: trying to use deleted viewport");
        return false;
    }
    return play.GetRoomViewport(script_view->GetID())->IsVisible();
}

void Viewport_SetVisible(ScriptViewport *script_view, bool on)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Visible: trying to use deleted viewport");
        return;
    }
    play.GetRoomViewport(script_view->GetID())->SetVisible(on);
}

int Viewport_GetZOrder(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ZOrder: trying to use deleted viewport");
        return 0;
    }
    return play.GetRoomViewport(script_view->GetID())->GetZOrder();
}

void Viewport_SetZOrder(ScriptViewport *script_view, int zorder)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ZOrder: trying to use deleted viewport");
        return;
    }
    play.GetRoomViewport(script_view->GetID())->SetZOrder(zorder);
    play.InvalidateViewportZOrder();
}

ScriptShaderInstance *Viewport_GetShader(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Shader: trying to use deleted viewport");
        return nullptr;
    }
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(play.GetRoomViewport(script_view->GetID())->GetShaderHandle()));
}

void Viewport_SetShader(ScriptViewport *script_view, ScriptShaderInstance *shader_inst)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Shader: trying to use deleted viewport");
        return;
    }
    auto view = play.GetRoomViewport(script_view->GetID());
    view->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                    ccReplaceObjectHandle(view->GetShaderHandle(), shader_inst));
}

ScriptViewport *Viewport_GetAtScreenXY(int x, int y)
{
    PViewport view = play.GetRoomViewportAt(x, y);
    if (!view)
        return nullptr;
    return play.GetScriptViewport(view->GetID());
}

void Viewport_SetPosition(ScriptViewport *script_view, int x, int y, int width, int height)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.SetPosition: trying to use deleted viewport");
        return;
    }
    play.GetRoomViewport(script_view->GetID())->SetRect(RectWH(x, y, width, height));
}

ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *script_view, int scrx, int scry, bool clip_viewport)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ScreenToRoomPoint: trying to use deleted viewport");
        return nullptr;
    }
    VpPoint vpt = play.GetRoomViewport(script_view->GetID())->ScreenToRoom(scrx, scry, clip_viewport);
    if (vpt.second < 0)
        return nullptr;
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject *Viewport_RoomToScreenPoint(ScriptViewport *script_view, int roomx, int roomy, bool clip_viewport)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.RoomToScreenPoint: trying to use deleted viewport");
        return nullptr;
    }
    Point pt = play.RoomToScreen(roomx, roomy);
    if (clip_viewport && !play.GetRoomViewport(script_view->GetID())->GetRect().IsInside(pt.X, pt.Y))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
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
