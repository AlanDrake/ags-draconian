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
// Definition for the game viewports and cameras.
//
//=============================================================================
#pragma once

#include <memory>
#include <vector>

#include "util/geometry.h"
#include "util/matrix.h"
#include "util/scaling.h"

namespace AGS
{
namespace Engine
{
struct SpriteTransform;
} // namespace Engine
} // namespace AGS
class Camera;
class Viewport;
struct ScriptCamera;
struct ScriptViewport;

typedef std::shared_ptr<Camera> PCamera;
typedef std::shared_ptr<Viewport> PViewport;
typedef std::weak_ptr<Camera> CameraRef;
typedef std::weak_ptr<Viewport> ViewportRef;

// TODO: move to utility header
// From https://stackoverflow.com/questions/45507041/how-to-check-if-weak-ptr-is-empty-non-assigned
// Tests that weak_ptr is empty (was not initialized with valid reference)
template <typename T>
bool is_uninitialized(std::weak_ptr<T> const &weak)
{
    using wt = std::weak_ptr<T>;
    return !weak.owner_before(wt{}) && !wt{}.owner_before(weak);
}


// Camera defines a "looking eye" inside the room, its position and size.
// It does not render anywhere on its own, instead it is linked to a viewport
// and latter draws what that camera sees.
// One camera may be linked to multiple viewports.
// Camera does not move on its own, this is performed by separate behavior
// algorithm. But it provides "lock" property that tells if its position is
// currently owned by user script.
class Camera
{
public:
    // Gets camera ID (serves as an index)
    inline int GetID() const { return _id; }
    // Sets new camera ID
    void SetID(int id);
    // Returns Room camera position and size inside the room (in room coordinates)
    const Rect &GetRect() const;
    // Sets explicit room camera's orthographic size
    void SetSize(const Size sz);
    // Puts room camera to the new location in the room
    void SetAt(int x, int y);
    // Returns camera's rotation, in degrees
    float GetRotation() const;
    // Sets camera's rotation, in degrees
    void SetRotation(float degrees);
    Pointf GetPivot() const;
    void SetPivot(const Pointf &pivot);
    Point GetPivotOffset() const;
    void SetPivotOffset(const Point &pivot_off);
    Pointf GetEffectivePivot() const;
    int GetShaderID() const { return _shaderID; }
    int GetShaderHandle() const { return _shaderHandle; }
    void SetShader(int shader_id, int shader_handle)
    {
        _shaderID = shader_id;
        _shaderHandle = shader_handle;
    }

    // Tells if camera is currently locked at custom position
    bool IsLocked() const;
    // Locks room camera at its current position
    void Lock();
    // Similar to SetAt, but also locks camera preventing it from following player character
    void LockAt(int x, int y);
    // Releases camera lock, letting it follow player character
    void Release();

    // Link this camera to a new viewport; this does not unlink any linked ones
    void LinkToViewport(ViewportRef viewport);
    // Unlinks this camera from a given viewport; does nothing if link did not exist
    void UnlinkFromViewport(int id);
    // Get the array of linked viewport references
    const std::vector<ViewportRef> &GetLinkedViewports() const;

    // Tell if this camera has changed recently
    inline bool HasChangedPosition() const { return _hasChangedPosition; }
    inline bool HasChangedSize() const { return _hasChangedSize; }
    // Clears the changed flags
    void ClearChangedFlags()
    {
        _hasChangedPosition = false;
        _hasChangedSize = false;
    }

private:
    void AdjustTransformations();

    int _id = -1;
    // Actual position and orthographic size
    Rect _position;
    // Rotation in degrees
    float _rotation = 0.0;
    Pointf _pivot = Pointf(.5f, .5f); // default: center
    Point _pivotOff;
    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _shaderID = 0;
    int _shaderHandle = 0;
    // Locked or following player automatically
    bool _locked = false;
    // Linked viewport refs, used to notify viewports of camera changes
    std::vector<ViewportRef> _viewportRefs;
    // Flags that tell whether this camera's position on screen has changed recently
    bool _hasChangedPosition = false;
    bool _hasChangedSize = false;
};


// A result of coordinate conversion between screen and the room,
// tells which viewport was used to pass the "touch" through.
typedef std::pair<Point, int> VpPoint;


// Viewport class defines a rectangular area on game screen where the contents
// of a Camera are rendered.
// Viewport may have one linked camera at a time.
class Viewport
{
public:
    // Gets viewport ID (serves as an index)
    inline int GetID() const { return _id; }
    // Sets new viewport ID
    void SetID(int id);
    // Returns viewport's position on screen
    inline const Rect &GetRect() const { return _position; }
    // Returns viewport's room-to-screen (camera -> viewport) transformation
    inline const glm::mat4 &GetR2STransform() const { return _c2vTransform; }
    // Returns viewport's screen-to-room (viewport -> camera) transformation
    inline const glm::mat4 &GetS2RTransform() const { return _v2cTransform; }
    // Set viewport's rectangle on screen
    void SetRect(const Rect &rc);
    // Sets viewport size
    void SetSize(const Size sz);
    // Sets viewport's position on screen
    void SetAt(int x, int y);

    // Tells whether viewport content is rendered on screen
    bool IsVisible() const { return _visible; }
    // Changes viewport visibility
    void SetVisible(bool on);
    // Gets the order viewport is displayed on screen
    int GetZOrder() const { return _zorder; }
    // Sets the viewport's z-order on screen
    void SetZOrder(int zorder);

    int GetShaderID() const { return _shaderID; }
    int GetShaderHandle() const { return _shaderHandle; }
    void SetShader(int shader_id, int shader_handle)
    {
        _shaderID = shader_id;
        _shaderHandle = shader_handle;
    }

    // Calculates room-to-viewport coordinate conversion.
    void AdjustTransformation();
    // Returns linked camera
    PCamera GetCamera() const;
    // Links new camera to this viewport, overriding existing link;
    // pass nullptr to leave viewport without a camera link
    void LinkCamera(PCamera cam);

    // Converts room coordinates to the game screen coordinates through this viewport;
    // if clipping is on, the function will fail for room coordinates outside of camera
    VpPoint RoomToScreen(int roomx, int roomy, bool clip = false) const;
    // Converts game screen coordinates to the room coordinates through this viewport;
    // if clipping is on, the function will fail for screen coordinates outside of viewport
    VpPoint ScreenToRoom(int scrx, int scry, bool clip = false) const;

    // Following functions tell if this viewport has changed recently
    inline bool HasChangedPosition() const { return _hasChangedPosition; }
    inline bool HasChangedSize() const { return _hasChangedSize; }
    inline bool HasChangedVisible() const { return _hasChangedVisible; }
    inline void SetChangedVisible() { _hasChangedVisible = true; }
    // Clears the changed flags
    inline void ClearChangedFlags()
    {
        _hasChangedPosition = false;
        _hasChangedSize = false;
        _hasChangedVisible = false;
    }

private:
    int _id = -1;
    // Position of the viewport on screen
    Rect _position;
    // Linked camera reference
    CameraRef _camera;
    // Coordinate tranform camera -> viewport
    glm::mat4 _c2vTransform;
    // Coordinate tranform viewport -> camera
    glm::mat4 _v2cTransform;
    bool _visible = true;
    int _zorder = 0;
    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _shaderID = 0;
    int _shaderHandle = 0;
    // Flags that tell whether this viewport's position on screen has changed recently
    bool _hasChangedPosition = false;
    bool _hasChangedOffscreen = false;
    bool _hasChangedSize = false;
    bool _hasChangedVisible = false;
};

// The game's viewports and cameras: the main and UI viewports on the game screen, the room viewports and the
// room cameras they show, and the managed handles of their script objects (ScriptViewport, ScriptCamera).
// Held by the game context as Game::Viewports (game/game_context.h).
class GameViewports
{
public:
    // Viewports are positioned in game screen coordinates, related to the "game size",
    // while cameras are positioned in room coordinates.

    // Returns main (game's) viewport position on screen, this is the overall game view
    const Rect &GetMainViewport() const;
    // Returns UI viewport position on screen, within the main viewport
    const Rect &GetUIViewport() const;
    // Returns SpriteTransform corresponding to the global screen offsets: the main viewport's position, and
    // the shaking screen's y offset if full_frame_rend is set
    AGS::Engine::SpriteTransform GetGlobalTransform(bool full_frame_rend) const;
    // Tells if the room viewport should be adjusted automatically each time a new room is loaded
    bool IsAutoRoomViewport() const;
    // Returns Room viewport object by its main index
    PViewport GetRoomViewport(int index) const;
    // Returns the Room viewports in z-order, the bottom one first
    const std::vector<PViewport> &GetRoomViewportsZOrdered() const;
    // Finds the topmost visible room viewport at the given screen coordinates; returns nullptr if none found
    PViewport GetRoomViewportAt(int x, int y) const;
    // Returns Room viewport position in absolute coordinates (with main viewport offset);
    // this is a helper function, meant for peculiar cases.
    Rect GetRoomViewportAbs(int index) const;
    // Sets if the room viewport should be adjusted automatically each time a new room is loaded
    void SetAutoRoomViewport(bool on);
    // Main viewport defines the location of all things drawn and interactable on the game screen.
    // Other viewports are defined relative to the main viewports.
    // Also updates the mouse's graphic area and the viewport size in the script's system struct.
    void SetMainViewport(const Rect &viewport);
    // UI viewport is a formal dummy viewport for GUI and Overlays (like speech).
    void SetUIViewport(const Rect &viewport);
    // Applies all the pending changes to viewports and cameras;
    // NOTE: this function may be slow, thus recommended to be called only once
    // and during the main game update.
    void UpdateViewports();
    // Tells that the viewports need z-order resorting upon next update.
    void InvalidateViewportZOrder();
    // Returns room camera object chosen by index
    PCamera GetRoomCamera(int index) const;
    // Runs cameras behaviors: a camera that does not show the whole room and is not locked follows
    // the player character, if it is enabled
    void UpdateRoomCameras();
    // Converts room coordinates to the game screen coordinates through the room viewport
    // This group of functions always tries to pass a point through the **primary** room viewport
    // TODO: also support using arbitrary viewport (for multiple viewports).
    Point RoomToScreen(int room_x, int room_y);
    int RoomToScreenX(int room_x);
    // Converts game screen coordinates to the room coordinates through the room viewport.
    // Tries to find if there is any viewport at the given coords.
    // If "clip_viewport" parameter is true, then not finding a viewport results in failure,
    // if it is false, proceeds converting through the primary viewport.
    // A game made for a script API older than kScriptAPI_v3507 ignores clip_viewport: it always converts through
    // the primary viewport, without failing.
    VpPoint ScreenToRoom(int screen_x, int screen_y, bool clip_viewport = true);

    // Makes sure primary viewport and camera are created and linked together
    void CreatePrimaryViewportAndCamera();
    // Creates new room viewport, with the size of the main viewport; its ID is its index
    PViewport CreateRoomViewport();
    // Register viewport in the managed system; optionally links to existing handle
    ScriptViewport *RegisterRoomViewport(int index, int32_t handle = 0);
    // Deletes existing room viewport: invalidates its script object, unlinks its camera and numbers
    // the viewports after it anew
    void DeleteRoomViewport(int index);
    // Get number of room viewports
    int GetRoomViewportCount() const;
    // Creates new room camera, with the size of the main viewport; its ID is its index
    PCamera CreateRoomCamera();
    // Register camera in the managed system; optionally links to existing handle
    ScriptCamera *RegisterRoomCamera(int index, int32_t handle = 0);
    // Deletes existing room camera: invalidates its script object, unlinks it from the viewports and numbers
    // the cameras after it anew
    void DeleteRoomCamera(int index);
    // Get number of room cameras
    int GetRoomCameraCount() const;
    // Gets script viewport reference; does NOT increment refcount
    // because script interpreter does this when acquiring managed pointer.
    ScriptViewport *GetScriptViewport(int index);
    // Gets script camera reference; does NOT increment refcount
    // because script interpreter does this when acquiring managed pointer.
    ScriptCamera *GetScriptCamera(int index);

    // Removes all the room viewports and cameras with their script objects; required for freeing only particular
    // parts when restoring the game (see DoBeforeRestore in savegame.cpp)
    void FreeViewportsAndCameras();

private:
    VpPoint ScreenToRoomImpl(int screen_x, int screen_y, int viewport_id, bool clip_viewport);
    void UpdateRoomCamera(int index);

    // Defines if the room viewport should be adjusted to the room size automatically.
    bool _isAutoRoomViewport = true;
    // Main viewport defines the rectangle of the drawn and interactable area;
    // in the most basic case it will be equal to the game size.
    Rect _mainViewport;
    // UI viewport defines the render and interaction rectangle of game's UI,
    // within the main game viewport.
    Rect _uiViewport;
    // Room viewports define place on screen where the room camera's
    // contents are drawn.
    std::vector<PViewport> _roomViewports;
    // Vector of viewports sorted in z-order.
    std::vector<PViewport> _roomViewportsSorted;
    // Cameras defines the position of a "looking eye" inside the room.
    std::vector<PCamera> _roomCameras;
    // We keep handles to the script refs to viewports and cameras, so that we
    // could address them and invalidate as the actual object gets destroyed.
    std::vector<int32_t> _scViewportHandles;
    std::vector<int32_t> _scCameraHandles;

    // Tells that the main viewport's position has changed since last game update
    bool _mainViewportHasChanged = false;
    // Tells that room viewports need z-order resort
    bool _roomViewportZOrderChanged = false;
};
