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
#include "game/render/viewport.h"

#include <algorithm>

#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "game/dynobj/script_camera.h"
#include "game/dynobj/script_system.h"
#include "game/dynobj/script_viewport.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/world/character.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "gfx/graphicsdriver.h"
#include "script/dynobj/dynobj_manager.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// FIXME: don't use global variables here (Game::DisplayedRoom, Room::Current()), have access to a GameState or some kind of a
// camera manager etc

void Camera::SetID(int id)
{
    _id = id;
}

// Returns Room camera position and size inside the room (in room coordinates)
const Rect &Camera::GetRect() const
{
    return _position;
}

// Sets explicit room camera's orthographic size
void Camera::SetSize(const Size cam_size)
{
    Room &room = Room::Current();
    // TODO: currently we don't support having camera larger than room background
    // (or rather - looking outside of the room background); look into this later
    const Size real_room_sz = (Game::Current().DisplayedRoom >= 0 && (room.Width > 0 && room.Height > 0))
                                  ? Size(room.Width, room.Height)
                                  : Size(INT32_MAX, INT32_MAX);

    const Size real_size = Size::Clamp(cam_size, Size(1, 1), real_room_sz);
    if (_position.GetWidth() == real_size.Width && _position.GetHeight() == real_size.Height)
        return;

    _position.SetWidth(real_size.Width);
    _position.SetHeight(real_size.Height);
    // readjust in case went off-room after size changed
    int x = Math::Clamp(_position.Left, 0, room.Width - _position.GetWidth());
    int y = Math::Clamp(_position.Top, 0, room.Height - _position.GetHeight());
    if (_position.Left != x || _position.Top != y)
    {
        _position.MoveTo(Point(x, y));
        _hasChangedPosition = true;
    }
    AdjustTransformations();
    _hasChangedSize = true;
}

// Puts room camera to the new location in the room
void Camera::SetAt(int x, int y)
{
    Room &room = Room::Current();
    x = Math::Clamp(x, 0, room.Width - _position.GetWidth());
    y = Math::Clamp(y, 0, room.Height - _position.GetHeight());
    if (_position.Left == x && _position.Top == y)
        return;

    _position.MoveTo(Point(x, y));
    AdjustTransformations();
    _hasChangedPosition = true;
}

float Camera::GetRotation() const
{
    return _rotation;
}

void Camera::SetRotation(float degrees)
{
    _rotation = Math::ClampAngle360(degrees);
    AdjustTransformations();
    _hasChangedSize = true;
}

Pointf Camera::GetPivot() const
{
    return _pivot;
}

void Camera::SetPivot(const Pointf &pivot)
{
    _pivot = pivot;
    AdjustTransformations();
    _hasChangedPosition = true; // changing pivot shifts camera but does not change the rotation angle
}

Point Camera::GetPivotOffset() const
{
    return _pivotOff;
}

void Camera::SetPivotOffset(const Point &pivot_off)
{
    _pivotOff = pivot_off;
    AdjustTransformations();
    _hasChangedPosition = true; // changing pivot shifts camera but does not change the rotation angle
}

Pointf Camera::GetEffectivePivot() const
{
    return _pivot + Pointf((static_cast<float>(_pivotOff.X) / _position.GetWidth()),
                           (static_cast<float>(_pivotOff.Y) / _position.GetHeight()));
}

void Camera::AdjustTransformations()
{
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto locked_vp = vp->lock();
        if (locked_vp)
            locked_vp->AdjustTransformation();
    }
}

// Tells if camera is currently locked at custom position
bool Camera::IsLocked() const
{
    return _locked;
}

// Locks room camera at its current position
void Camera::Lock()
{
    if (!_locked)
    {
        debug_script_log("Room camera pos locked");
        _locked = true;
    }
}

// Similar to SetAt, but also locks camera preventing it from following player character
void Camera::LockAt(int x, int y)
{
    if (!_locked || _position.Left != x || _position.Top != y)
    {
        const Point old_pos = _position.GetLT();
        SetAt(x, y);
        _locked = true;
        if (old_pos != _position.GetLT())
            debug_script_log("Room camera pos locked to %d,%d (request %d,%d)", _position.Left, _position.Top, x, y);
    }
}

// Releases camera lock, letting it follow player character
void Camera::Release()
{
    if (_locked)
    {
        _locked = false;
        debug_script_log("Room camera pos released");
    }
}

// Link this camera to a new viewport; this does not unlink any linked ones
void Camera::LinkToViewport(ViewportRef viewport)
{
    auto new_locked = viewport.lock();
    if (!new_locked)
        return;
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto old_locked = vp->lock();
        if (old_locked->GetID() == new_locked->GetID())
            return;
    }
    _viewportRefs.push_back(viewport);
}

// Unlinks this camera from a given viewport; does nothing if link did not exist
void Camera::UnlinkFromViewport(int id)
{
    for (auto vp = _viewportRefs.begin(); vp != _viewportRefs.end(); ++vp)
    {
        auto locked = vp->lock();
        if (locked && locked->GetID() == id)
        {
            _viewportRefs.erase(vp);
            return;
        }
    }
}

const std::vector<ViewportRef> &Camera::GetLinkedViewports() const
{
    return _viewportRefs;
}

void Viewport::SetID(int id)
{
    _id = id;
}

void Viewport::SetRect(const Rect &rc)
{
    // TODO: consider allowing size 0,0, in which case viewport is considered not visible
    Size fix_size = rc.GetSize().IsNull() ? Size(1, 1) : rc.GetSize();
    Rect new_pos = RectWH(rc.Left, rc.Top, fix_size.Width, fix_size.Height);
    if (new_pos == _position)
        return;
    _position = new_pos;
    AdjustTransformation();
    _hasChangedPosition = true;
    _hasChangedSize = true;
}

void Viewport::SetSize(const Size sz)
{
    // TODO: consider allowing size 0,0, in which case viewport is considered not visible
    Size fix_size = sz.IsNull() ? Size(1, 1) : sz;
    if (_position.GetWidth() == fix_size.Width && _position.GetHeight() == fix_size.Height)
        return;
    _position = RectWH(_position.Left, _position.Top, fix_size.Width, fix_size.Height);
    AdjustTransformation();
    _hasChangedSize = true;
}

void Viewport::SetAt(int x, int y)
{
    if (_position.Left == x && _position.Top == y)
        return;
    _position.MoveTo(Point(x, y));
    AdjustTransformation();
    _hasChangedPosition = true;
}

void Viewport::SetVisible(bool on)
{
    _visible = on;
    _hasChangedVisible = true;
}

void Viewport::SetZOrder(int zorder)
{
    _zorder = zorder;
    _hasChangedVisible = true;
}

void Viewport::AdjustTransformation()
{
    auto locked_cam = _camera.lock();
    if (locked_cam)
    {
        auto cam_rc = locked_cam->GetRect();
        float scale_x = (float)_position.GetWidth() / (float)cam_rc.GetWidth();
        float scale_y = (float)_position.GetHeight() / (float)cam_rc.GetHeight();
        float rotate = locked_cam->GetRotation();
        Pointf pivot = locked_cam->GetEffectivePivot();

        glm::mat4 mat_v2c = glmex::make_transform2d(cam_rc.Left, cam_rc.Top, 1.f / scale_x, 1.f / scale_y,
                                                    Math::DegreesToRadians(rotate), -cam_rc.GetWidth() * pivot.X,
                                                    -cam_rc.GetHeight() * pivot.Y);
        _v2cTransform = glmex::translate(mat_v2c, -_position.Left, -_position.Top);

        glm::mat4 mat_c2v = glmex::translate(_position.Left, _position.Top);
        _c2vTransform = glmex::inv_transform2d(mat_c2v, -cam_rc.Left, -cam_rc.Top, scale_x, scale_y,
                                               -Math::DegreesToRadians(rotate), cam_rc.GetWidth() * pivot.X,
                                               cam_rc.GetHeight() * pivot.Y);
    }
}

PCamera Viewport::GetCamera() const
{
    return _camera.lock();
}

void Viewport::LinkCamera(PCamera cam)
{
    _camera = cam;
    AdjustTransformation();
}

VpPoint Viewport::RoomToScreen(int roomx, int roomy, bool clip) const
{
    auto cam = _camera.lock();
    if (!cam)
        return std::make_pair(Point(), -1);
    glm::vec4 v = _c2vTransform * glmex::vec4(roomx, roomy);
    Point screen_pt(v.x, v.y);
    if (clip && !_position.IsInside(screen_pt))
        return std::make_pair(Point(), -1);
    return std::make_pair(screen_pt, _id);
}

VpPoint Viewport::ScreenToRoom(int scrx, int scry, bool clip) const
{
    Point screen_pt(scrx, scry);
    if (clip && !_position.IsInside(screen_pt))
        return std::make_pair(Point(), -1);
    auto cam = _camera.lock();
    if (!cam)
        return std::make_pair(Point(), -1);

    glm::vec4 v = _v2cTransform * glmex::vec4(scrx, scry);
    Point p(v.x, v.y);
    return std::make_pair(p, _id);
}


bool GameViewports::IsAutoRoomViewport() const
{
    return _isAutoRoomViewport;
}

void GameViewports::SetAutoRoomViewport(bool on)
{
    _isAutoRoomViewport = on;
}

void GameViewports::SetMainViewport(const Rect &viewport)
{
    Game &game = Game::Current();
    _mainViewport = viewport;
    Mouse::UpdateGraphicArea();
    game.ScSystem.viewport_width = _mainViewport.GetWidth();
    game.ScSystem.viewport_height = _mainViewport.GetHeight();
    _mainViewportHasChanged = true;
}

const Rect &GameViewports::GetMainViewport() const
{
    return _mainViewport;
}

const Rect &GameViewports::GetUIViewport() const
{
    return _uiViewport;
}

SpriteTransform GameViewports::GetGlobalTransform(bool full_frame_rend) const
{
    // NOTE: the screen shake is a part of the sprite batches' transform only when the whole frame is redrawn
    // each time (full_frame_rend); otherwise it is applied as a final render offset (optimization)
    // TODO: also add global flip to the same transform, instead of passing separately?
    return SpriteTransform(_mainViewport.Left, _mainViewport.Top + Game::Current().Play.shake_screen_yoff *
                                                                       static_cast<int>(full_frame_rend));
}

PViewport GameViewports::GetRoomViewport(int index) const
{
    return _roomViewports[index];
}

const std::vector<PViewport> &GameViewports::GetRoomViewportsZOrdered() const
{
    return _roomViewportsSorted;
}

PViewport GameViewports::GetRoomViewportAt(int x, int y) const
{
    // We iterate backwards, because in AGS low z-order means bottom
    for (auto it = _roomViewportsSorted.rbegin(); it != _roomViewportsSorted.rend(); ++it)
        if ((*it)->IsVisible() && (*it)->GetRect().IsInside(x, y))
            return *it;
    return nullptr;
}

Rect GameViewports::GetRoomViewportAbs(int index) const
{
    return Rect::MoveBy(_roomViewports[index]->GetRect(), _mainViewport.Left, _mainViewport.Top);
}

void GameViewports::SetUIViewport(const Rect &viewport)
{
    _uiViewport = viewport;
}

static bool ViewportZOrder(const PViewport first, const PViewport second)
{
    return first->GetZOrder() < second->GetZOrder();
}

void GameViewports::UpdateViewports()
{
    if (_mainViewportHasChanged)
    {
        on_mainviewport_changed();
        _mainViewportHasChanged = false;
    }
    if (_roomViewportZOrderChanged)
    {
        auto old_sort = _roomViewportsSorted;
        _roomViewportsSorted = _roomViewports;
        std::sort(_roomViewportsSorted.begin(), _roomViewportsSorted.end(), ViewportZOrder);
        for (size_t i = 0; i < _roomViewportsSorted.size(); ++i)
        {
            if (i >= old_sort.size() || _roomViewportsSorted[i] != old_sort[i])
                _roomViewportsSorted[i]->SetChangedVisible();
        }
        _roomViewportZOrderChanged = false;
    }
    size_t vp_changed = SIZE_MAX;
    for (size_t i = _roomViewportsSorted.size(); i-- > 0;)
    {
        auto vp = _roomViewportsSorted[i];
        if (vp->HasChangedSize() || vp->HasChangedPosition() || vp->HasChangedVisible())
        {
            vp_changed = i;
            on_roomviewport_changed(vp.get());
            vp->ClearChangedFlags();
        }
    }
    if (vp_changed != SIZE_MAX)
        detect_roomviewport_overlaps(vp_changed);
    for (auto cam : _roomCameras)
    {
        if (cam->HasChangedSize() || cam->HasChangedPosition())
        {
            on_roomcamera_changed(cam.get());
            cam->ClearChangedFlags();
        }
    }
}

void GameViewports::InvalidateViewportZOrder()
{
    _roomViewportZOrderChanged = true;
}

PCamera GameViewports::GetRoomCamera(int index) const
{
    return _roomCameras[index];
}

void GameViewports::UpdateRoomCameras()
{
    for (size_t i = 0; i < _roomCameras.size(); ++i)
        UpdateRoomCamera(i);
}

void GameViewports::UpdateRoomCamera(int index)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    auto cam = _roomCameras[index];
    const Rect &cam_rect = cam->GetRect();
    const Size real_room_size = Size(room.Width, room.Height);
    if ((real_room_size.Width > cam_rect.GetWidth()) || (real_room_size.Height > cam_rect.GetHeight()))
    {
        // TODO: split out into Camera Behavior
        if (!cam->IsLocked() && game.PlayerChar->is_enabled())
        {
            int x = game.PlayerChar->X - cam_rect.GetWidth() / 2;
            int y = game.PlayerChar->Y - cam_rect.GetHeight() / 2;
            cam->SetAt(x, y);
        }
    }
}

Point GameViewports::RoomToScreen(int room_x, int room_y)
{
    return _roomViewports[0]->RoomToScreen(room_x, room_y, false).first;
}

int GameViewports::RoomToScreenX(int room_x)
{
    return _roomViewports[0]->RoomToScreen(room_x, 0, false).first.X;
}

VpPoint GameViewports::ScreenToRoomImpl(int screen_x, int screen_y, int viewport_id, bool clip_viewport)
{
    PViewport view;
    if (viewport_id < 0)
    {
        view = GetRoomViewportAt(screen_x, screen_y);
        if (!view)
        {
            if (clip_viewport)
                return std::make_pair(Point(), -1);
            view = _roomViewports[0]; // use primary viewport
        }
    }
    else
    {
        view = _roomViewports[viewport_id];
    }
    return view->ScreenToRoom(screen_x, screen_y, clip_viewport);
}

VpPoint GameViewports::ScreenToRoom(int screen_x, int screen_y, bool clip_viewport)
{
    if (Game::Current().options[OPT_BASESCRIPTAPI] >= kScriptAPI_v3507)
        return ScreenToRoomImpl(screen_x, screen_y, -1, clip_viewport);
    return ScreenToRoomImpl(screen_x, screen_y, 0, false);
}

void GameViewports::CreatePrimaryViewportAndCamera()
{
    Game &game = Game::Current();
    if (_roomViewports.size() == 0)
    {
        game.Viewports.CreateRoomViewport();
        game.Viewports.RegisterRoomViewport(0);
    }
    if (_roomCameras.size() == 0)
    {
        game.Viewports.CreateRoomCamera();
        game.Viewports.RegisterRoomCamera(0);
    }
    _roomViewports[0]->LinkCamera(_roomCameras[0]);
    _roomCameras[0]->LinkToViewport(_roomViewports[0]);
}

PViewport GameViewports::CreateRoomViewport()
{
    int index = (int)_roomViewports.size();
    PViewport viewport(new Viewport());
    viewport->SetID(index);
    viewport->SetRect(_mainViewport);
    _roomViewports.push_back(viewport);
    _scViewportHandles.push_back(0);
    _roomViewportsSorted.push_back(viewport);
    _roomViewportZOrderChanged = true;
    on_roomviewport_created(index);
    return viewport;
}

ScriptViewport *GameViewports::RegisterRoomViewport(int index, int32_t handle)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return nullptr;
    auto script_view = new ScriptViewport(index);
    if (handle == 0)
    {
        handle = ccRegisterManagedObjectAndRef(script_view, script_view); // one ref for GameViewports
    }
    else
    {
        ccRegisterUnserializedObject(handle, script_view, script_view);
    }
    _scViewportHandles[index] = handle; // save handle for us
    return script_view;
}

void GameViewports::DeleteRoomViewport(int index)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return;

    // Invalidate script object
    auto handle = _scViewportHandles[index];
    auto script_obj = static_cast<ScriptViewport *>(ccGetObjectAddressFromHandle(handle));
    if (script_obj)
    {
        script_obj->Invalidate();
        ccReleaseObjectReference(handle);
    }
    // Unlink camera
    auto cam = _roomViewports[index]->GetCamera();
    if (cam)
        cam->UnlinkFromViewport(index);

    // Release shader handle (if present)
    ccRemoveObjectHandle(_roomViewports[index]->GetShaderHandle());

    // Erase viewport object and adjust the cameras lists
    _roomViewports.erase(_roomViewports.begin() + index);
    _scViewportHandles.erase(_scViewportHandles.begin() + index);
    for (size_t i = index; i < _roomViewports.size(); ++i)
    {
        _roomViewports[i]->SetID(i);
        handle = _scViewportHandles[index];
        script_obj = static_cast<ScriptViewport *>(ccGetObjectAddressFromHandle(handle));
        if (script_obj)
            script_obj->SetID(i);
    }
    for (size_t i = 0; i < _roomViewportsSorted.size(); ++i)
    {
        if (_roomViewportsSorted[i]->GetID() == index)
        {
            _roomViewportsSorted.erase(_roomViewportsSorted.begin() + i);
            break;
        }
    }
    on_roomviewport_deleted(index);
}

int GameViewports::GetRoomViewportCount() const
{
    return (int)_roomViewports.size();
}

PCamera GameViewports::CreateRoomCamera()
{
    int index = (int)_roomCameras.size();
    PCamera camera(new Camera());
    camera->SetID(index);
    camera->SetAt(0, 0);
    camera->SetSize(_mainViewport.GetSize());
    _scCameraHandles.push_back(0);
    _roomCameras.push_back(camera);
    return camera;
}

ScriptCamera *GameViewports::RegisterRoomCamera(int index, int32_t handle)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return nullptr;
    auto script_cam = new ScriptCamera(index);
    if (handle == 0)
    {
        handle = ccRegisterManagedObjectAndRef(script_cam, script_cam); // one ref for GameViewports
    }
    else
    {
        ccRegisterUnserializedObject(handle, script_cam, script_cam);
    }
    _scCameraHandles[index] = handle;
    return script_cam;
}

void GameViewports::DeleteRoomCamera(int index)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return;

    // Invalidate script object
    auto handle = _scCameraHandles[index];
    auto script_obj = static_cast<ScriptCamera *>(ccGetObjectAddressFromHandle(handle));
    if (script_obj)
    {
        script_obj->Invalidate();
        ccReleaseObjectReference(handle);
    }
    // Unlink viewport
    for (auto &view_ref : _roomCameras[index]->GetLinkedViewports())
    {
        auto view = view_ref.lock();
        if (view)
            view->LinkCamera(nullptr);
    }

    // Release shader handle (if present)
    ccRemoveObjectHandle(_roomCameras[index]->GetShaderHandle());

    // Erase camera object and adjust the cameras lists
    _roomCameras.erase(_roomCameras.begin() + index);
    _scCameraHandles.erase(_scCameraHandles.begin() + index);
    for (size_t i = index; i < _roomCameras.size(); ++i)
    {
        _roomCameras[i]->SetID(i);
        handle = _scCameraHandles[index];
        script_obj = static_cast<ScriptCamera *>(ccGetObjectAddressFromHandle(handle));
        if (script_obj)
            script_obj->SetID(i);
    }
}

int GameViewports::GetRoomCameraCount() const
{
    return (int)_roomCameras.size();
}

ScriptViewport *GameViewports::GetScriptViewport(int index)
{
    if (index < 0 || (size_t)index >= _roomViewports.size())
        return nullptr;
    return static_cast<ScriptViewport *>(ccGetObjectAddressFromHandle(_scViewportHandles[index]));
}

ScriptCamera *GameViewports::GetScriptCamera(int index)
{
    if (index < 0 || (size_t)index >= _roomCameras.size())
        return nullptr;
    return static_cast<ScriptCamera *>(ccGetObjectAddressFromHandle(_scCameraHandles[index]));
}

void GameViewports::FreeViewportsAndCameras()
{
    _roomViewports.clear();
    _roomViewportsSorted.clear();
    for (auto handle : _scViewportHandles)
    {
        auto script_view = static_cast<ScriptViewport *>(ccGetObjectAddressFromHandle(handle));
        if (script_view)
        {
            script_view->Invalidate();
            ccReleaseObjectReference(handle);
        }
    }
    _scViewportHandles.clear();
    _roomCameras.clear();
    for (auto handle : _scCameraHandles)
    {
        auto script_cam = static_cast<ScriptCamera *>(ccGetObjectAddressFromHandle(handle));
        if (script_cam)
        {
            script_cam->Invalidate();
            ccReleaseObjectReference(handle);
        }
    }
    _scCameraHandles.clear();
}
