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

#include "game/render/viewport_script.h"

#include "debug/debug_log.h"
#include "game/dynobj/script_camera.h"
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_viewport.h"
#include "game/game_context.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"

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
    Game &game = Game::Current();
    auto cam = game.Play.CreateRoomCamera();
    if (!cam)
        return nullptr;
    return game.Play.RegisterRoomCamera(cam->GetID());
}

void Camera_Delete(ScriptCamera *script_cam)
{
    Game::Current().Play.DeleteRoomCamera(script_cam->GetID());
}

int Camera_GetX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.X: trying to use deleted camera");
        return 0;
    }
    return Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetRect().Left;
}

void Camera_SetX(ScriptCamera *script_cam, int x)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.X: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->LockAt(x, cam->GetRect().Top);
}

int Camera_GetY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Y: trying to use deleted camera");
        return 0;
    }
    return Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetRect().Top;
}

void Camera_SetY(ScriptCamera *script_cam, int y)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Y: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->LockAt(cam->GetRect().Left, y);
}

int Camera_GetWidth(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Width: trying to use deleted camera");
        return 0;
    }
    return Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetRect().GetWidth();
}

void Camera_SetWidth(ScriptCamera *script_cam, int width)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Width: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetSize(Size(width, cam->GetRect().GetHeight()));
}

int Camera_GetHeight(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Height: trying to use deleted camera");
        return 0;
    }
    return Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetRect().GetHeight();
}

void Camera_SetHeight(ScriptCamera *script_cam, int height)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Height: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetSize(Size(cam->GetRect().GetWidth(), height));
}

float Camera_GetRotation(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Rotation: trying to use deleted camera");
        return 0;
    }
    return Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetRotation();
}

void Camera_SetRotation(ScriptCamera *script_cam, float rotation)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Rotation: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetRotation(rotation);
}

float Camera_GetPivotX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotX: trying to use deleted camera");
        return 0.f;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivot().X;
}

void Camera_SetPivotX(ScriptCamera *script_cam, float pivotx)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotX: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetPivot(Pointf(pivotx, cam->GetPivot().Y));
}

float Camera_GetPivotY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotY: trying to use deleted camera");
        return 0.f;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivot().Y;
}

void Camera_SetPivotY(ScriptCamera *script_cam, float pivoty)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotY: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetPivot(Pointf(cam->GetPivot().X, pivoty));
}

int Camera_GetPivotOffsetX(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetX: trying to use deleted camera");
        return 0;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivotOffset().X;
}

void Camera_SetPivotOffsetX(ScriptCamera *script_cam, int px)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetX: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetPivotOffset(Point(px, cam->GetPivotOffset().Y));
}

int Camera_GetPivotOffsetY(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetY: trying to use deleted camera");
        return 0;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    return cam->GetPivotOffset().Y;
}

void Camera_SetPivotOffsetY(ScriptCamera *script_cam, int py)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.PivotOffsetY: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetPivotOffset(Point(cam->GetPivotOffset().X, py));
}

bool Camera_GetAutoTracking(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.AutoTracking: trying to use deleted camera");
        return false;
    }
    return !Game::Current().Play.GetRoomCamera(script_cam->GetID())->IsLocked();
}

void Camera_SetAutoTracking(ScriptCamera *script_cam, bool on)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.AutoTracking: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
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
    Game::Current().Play.GetRoomCamera(script_cam->GetID())->LockAt(x, y);
}

void Camera_SetSize(ScriptCamera *script_cam, int width, int height)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.SetSize: trying to use deleted camera");
        return;
    }
    Game::Current().Play.GetRoomCamera(script_cam->GetID())->SetSize(Size(width, height));
}

ScriptShaderInstance *Camera_GetShader(ScriptCamera *script_cam)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Shader: trying to use deleted camera");
        return nullptr;
    }
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(Game::Current().Play.GetRoomCamera(script_cam->GetID())->GetShaderHandle()));
}

void Camera_SetShader(ScriptCamera *script_cam, ScriptShaderInstance *shader_inst)
{
    if (script_cam->GetID() < 0)
    {
        debug_script_warn("Camera.Shader: trying to use deleted camera");
        return;
    }
    auto cam = Game::Current().Play.GetRoomCamera(script_cam->GetID());
    cam->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                   ccReplaceObjectHandle(cam->GetShaderHandle(), shader_inst));
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
    Game &game = Game::Current();
    auto view = game.Play.CreateRoomViewport();
    if (!view)
        return nullptr;
    return game.Play.RegisterRoomViewport(view->GetID());
}

void Viewport_Delete(ScriptViewport *script_view)
{
    Game::Current().Play.DeleteRoomViewport(script_view->GetID());
}

int Viewport_GetX(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.X: trying to use deleted viewport");
        return 0;
    }
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->GetRect().Left;
}

void Viewport_SetX(ScriptViewport *script_view, int x)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.X: trying to use deleted viewport");
        return;
    }
    auto view = Game::Current().Play.GetRoomViewport(script_view->GetID());
    view->SetAt(x, view->GetRect().Top);
}

int Viewport_GetY(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Y: trying to use deleted viewport");
        return 0;
    }
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->GetRect().Top;
}

void Viewport_SetY(ScriptViewport *script_view, int y)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Y: trying to use deleted viewport");
        return;
    }
    auto view = Game::Current().Play.GetRoomViewport(script_view->GetID());
    view->SetAt(view->GetRect().Left, y);
}

int Viewport_GetWidth(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Width: trying to use deleted viewport");
        return 0;
    }
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->GetRect().GetWidth();
}

void Viewport_SetWidth(ScriptViewport *script_view, int width)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Width: trying to use deleted viewport");
        return;
    }
    auto view = Game::Current().Play.GetRoomViewport(script_view->GetID());
    view->SetSize(Size(width, view->GetRect().GetHeight()));
}

int Viewport_GetHeight(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Height: trying to use deleted viewport");
        return 0;
    }
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->GetRect().GetHeight();
}

void Viewport_SetHeight(ScriptViewport *script_view, int height)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Height: trying to use deleted viewport");
        return;
    }
    auto view = Game::Current().Play.GetRoomViewport(script_view->GetID());
    view->SetSize(Size(view->GetRect().GetWidth(), height));
}

ScriptCamera *Viewport_GetCamera(ScriptViewport *script_view)
{
    Game &game = Game::Current();
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Camera: trying to use deleted viewport");
        return nullptr;
    }
    auto view = game.Play.GetRoomViewport(script_view->GetID());
    auto cam = view->GetCamera();
    if (!cam)
        return nullptr;
    return game.Play.GetScriptCamera(cam->GetID());
}

void Viewport_SetCamera(ScriptViewport *script_view, ScriptCamera *script_cam)
{
    Game &game = Game::Current();
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
    auto view = game.Play.GetRoomViewport(script_view->GetID());
    // unlink the previous camera
    auto cam = view->GetCamera();
    if (cam)
        cam->UnlinkFromViewport(view->GetID());
    // link the new camera; null leaves the viewport without a camera
    if (script_cam != nullptr)
    {
        cam = game.Play.GetRoomCamera(script_cam->GetID());
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
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->IsVisible();
}

void Viewport_SetVisible(ScriptViewport *script_view, bool on)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Visible: trying to use deleted viewport");
        return;
    }
    Game::Current().Play.GetRoomViewport(script_view->GetID())->SetVisible(on);
}

int Viewport_GetZOrder(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ZOrder: trying to use deleted viewport");
        return 0;
    }
    return Game::Current().Play.GetRoomViewport(script_view->GetID())->GetZOrder();
}

void Viewport_SetZOrder(ScriptViewport *script_view, int zorder)
{
    Game &game = Game::Current();
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ZOrder: trying to use deleted viewport");
        return;
    }
    game.Play.GetRoomViewport(script_view->GetID())->SetZOrder(zorder);
    game.Play.InvalidateViewportZOrder();
}

ScriptShaderInstance *Viewport_GetShader(ScriptViewport *script_view)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Shader: trying to use deleted viewport");
        return nullptr;
    }
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(Game::Current().Play.GetRoomViewport(script_view->GetID())->GetShaderHandle()));
}

void Viewport_SetShader(ScriptViewport *script_view, ScriptShaderInstance *shader_inst)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.Shader: trying to use deleted viewport");
        return;
    }
    auto view = Game::Current().Play.GetRoomViewport(script_view->GetID());
    view->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                    ccReplaceObjectHandle(view->GetShaderHandle(), shader_inst));
}

ScriptViewport *Viewport_GetAtScreenXY(int x, int y)
{
    Game &game = Game::Current();
    PViewport view = game.Play.GetRoomViewportAt(x, y);
    if (!view)
        return nullptr;
    return game.Play.GetScriptViewport(view->GetID());
}

void Viewport_SetPosition(ScriptViewport *script_view, int x, int y, int width, int height)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.SetPosition: trying to use deleted viewport");
        return;
    }
    Game::Current().Play.GetRoomViewport(script_view->GetID())->SetRect(RectWH(x, y, width, height));
}

ScriptUserObject *Viewport_ScreenToRoomPoint(ScriptViewport *script_view, int scrx, int scry, bool clip_viewport)
{
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.ScreenToRoomPoint: trying to use deleted viewport");
        return nullptr;
    }
    VpPoint vpt = Game::Current().Play.GetRoomViewport(script_view->GetID())->ScreenToRoom(scrx, scry, clip_viewport);
    if (vpt.second < 0)
        return nullptr;
    return ScriptStructHelpers::CreatePoint(vpt.first.X, vpt.first.Y);
}

ScriptUserObject *Viewport_RoomToScreenPoint(ScriptViewport *script_view, int roomx, int roomy, bool clip_viewport)
{
    Game &game = Game::Current();
    if (script_view->GetID() < 0)
    {
        debug_script_warn("Viewport.RoomToScreenPoint: trying to use deleted viewport");
        return nullptr;
    }
    Point pt = game.Play.RoomToScreen(roomx, roomy);
    if (clip_viewport && !game.Play.GetRoomViewport(script_view->GetID())->GetRect().IsInside(pt.X, pt.Y))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}
