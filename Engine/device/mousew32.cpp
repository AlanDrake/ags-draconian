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
// MOUSELIBW32.CPP
//
// Library of mouse functions for graphics and text mode
//
// (c) 1994 Chris Jones
// Win32 (allegro) update (c) 1999 Chris Jones
//
//=============================================================================
#include "device/mousew32.h"

#include <SDL.h>

#include "debug/out.h"
#include "device/sys_events.h"
#include "game/game_context.h"
#include "gfx/bitmap.h"
#include "platform/base/engine_services.h"
#include "platform/base/sys_main.h"

using namespace AGS::Common;
using namespace AGS::Engine;


// real mouse coordinates and bounds (in window coords)
static int real_mouse_x = 0, real_mouse_y = 0;
static Rect mouse_bounds;

namespace Mouse
{
// Tells whether mouse was locked to the game window
bool LockedToWindow = false;

// Screen rectangle, in which the mouse movement is controlled by engine
Rect ControlRect;
// Mouse control enabled flag
bool ControlEnabled = false;
// Mouse speed value provided by user
float SpeedVal = 1.f;
// Mouse speed unit
float SpeedUnit = 1.f;
// Actual speed factor (cached)
float Speed = 1.f;

// Converts real window coordinates to native game coords
void WindowToGame(int &x, int &y);
// Sets mouse position in system coordinates, syncs with the real mouse cursor
// NOTE: real cursor will be updated only if the game window has focus
void SetSysPosition(int x, int y);
} // namespace Mouse

Point Mouse::SysToGamePos(int sys_mx, int sys_my)
{
    // Clamp to control rect, and optionally script bounds
    int mx = Math::Clamp(sys_mx, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
    int my = Math::Clamp(sys_my, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);
    if (EngineServices::Current().Input.IgnoreBounds == 0)
    {
        mx = Math::Clamp(mx, mouse_bounds.Left, mouse_bounds.Right);
        my = Math::Clamp(my, mouse_bounds.Top, mouse_bounds.Bottom);
    }
    // Convert to virtual coordinates
    Mouse::WindowToGame(mx, my);
    return Point(mx, my);
}

void Mouse::Poll()
{
    EngineServices &services = EngineServices::Current();
    // TODO: [sonneveld] find out where Poll is needed, are events polled before that?
    sys_evt_process_pending();

    // CHECKME: disabled, to let Poll work with emulated mouse movement,
    // which might also run in background while the window does not has a focus.
    //if (EngineServices::Current().SwitchedAway)
    //return;

    // Save absolute cursor coordinates provided by system
    // NOTE: relative motion and the speed factor should already be applied by SDL2 or our custom devices.
    real_mouse_x = Math::Clamp((int)services.Input.SysMouseX, Mouse::ControlRect.Left, Mouse::ControlRect.Right);
    real_mouse_y = Math::Clamp((int)services.Input.SysMouseY, Mouse::ControlRect.Top, Mouse::ControlRect.Bottom);

    // Set new in-game cursor position, convert to the in-game logic coordinates
    services.Input.MouseX = real_mouse_x;
    services.Input.MouseY = real_mouse_y;
    // Optionally apply script bounds
    if ((services.Input.IgnoreBounds == 0) && (!mouse_bounds.IsInside(services.Input.MouseX, services.Input.MouseY)))
    {
        services.Input.MouseX = Math::Clamp(services.Input.MouseX, mouse_bounds.Left, mouse_bounds.Right);
        services.Input.MouseY = Math::Clamp(services.Input.MouseY, mouse_bounds.Top, mouse_bounds.Bottom);
        Mouse::SetSysPosition(services.Input.MouseX, services.Input.MouseY);
    }
    // Convert to virtual coordinates
    Mouse::WindowToGame(services.Input.MouseX, services.Input.MouseY);
}

void Mouse::SetSysPosition(int x, int y)
{
    EngineServices &services = EngineServices::Current();
    services.Input.SysMouseX = x;
    services.Input.SysMouseY = y;
    real_mouse_x = x;
    real_mouse_y = y;

    // move the real system cursor, but only if the game has a focus
    if (!services.SwitchedAway)
        sys_window_set_mouse(real_mouse_x, real_mouse_y);
}

int Mouse::GetButtonCount()
{
    // TODO: can SDL tell number of available/supported buttons at all, or whether mouse is present?
    // this is not that critical, but maybe some game devs would like to detect if player has or not a mouse.
    return 3; // SDL *theoretically* support 3 mouse buttons, but that does not mean they are physically present...
}

void Mouse::WindowToGame(int &x, int &y)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    x = services.GameScaling.X.UnScalePt(x) - game.Viewports.GetMainViewport().Left;
    y = services.GameScaling.Y.UnScalePt(y) - game.Viewports.GetMainViewport().Top;
}

void Mouse::UpdateGraphicArea()
{
    Mouse::ControlRect = EngineServices::Current().GameScaling.ScaleRange(Game::Current().Viewports.GetMainViewport());
    Debug::Printf("Mouse cursor graphic area: (%d,%d)-(%d,%d) (%dx%d)", Mouse::ControlRect.Left, Mouse::ControlRect.Top,
                  Mouse::ControlRect.Right, Mouse::ControlRect.Bottom, Mouse::ControlRect.GetWidth(),
                  Mouse::ControlRect.GetHeight());
}

void Mouse::SetMoveLimit(const Rect &r)
{
    Rect src_r = OffsetRect(r, Game::Current().Viewports.GetMainViewport().GetLT());
    mouse_bounds = EngineServices::Current().GameScaling.ScaleRange(src_r);
}

void Mouse::SetPosition(const Point &p)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Mouse::SetSysPosition(services.GameScaling.X.ScalePt(p.X + game.Viewports.GetMainViewport().Left),
                          services.GameScaling.Y.ScalePt(p.Y + game.Viewports.GetMainViewport().Top));
}

bool Mouse::IsLockedToWindow()
{
    return LockedToWindow;
}

bool Mouse::TryLockToWindow()
{
    if (!LockedToWindow)
        LockedToWindow = sys_window_lock_mouse(true, Mouse::ControlRect);
    return LockedToWindow;
}

void Mouse::UnlockFromWindow()
{
    sys_window_lock_mouse(false);
    LockedToWindow = false;
}

void Mouse::SetMovementControl(bool on)
{
#if defined(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE)
    ControlEnabled = on;
    SDL_SetRelativeMouseMode(static_cast<SDL_bool>(on));
    if (on)
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, String::FromFormat("%.2f", Mouse::Speed).GetCStr());
    else
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE, "");
#else
    ControlEnabled = false;
    if (on)
        Debug::Printf(kDbgMsg_Warn,
                      "WARNING: SDL_HINT_MOUSE_RELATIVE_SPEED_SCALE not supported, mouse control can't be enabled");
#endif
    ags_clear_mouse_movement();
}

bool Mouse::IsControlEnabled()
{
    return ControlEnabled;
}

void Mouse::SetTouch2MouseMode(TouchMouseEmulation mode, bool relative, float speed)
{
    ags_touch_set_mouse_emulation(mode, relative, speed);
}

void Mouse::SetMouse2Touch(bool enable)
{
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, enable ? "1" : "0");
}

void Mouse::SetSpeedUnit(float f)
{
    SpeedUnit = f;
    Speed = SpeedVal / SpeedUnit;
}

float Mouse::GetSpeedUnit()
{
    return SpeedUnit;
}

void Mouse::SetSpeed(float speed)
{
    SpeedVal = std::max(0.f, speed);
    Speed = SpeedUnit * SpeedVal;
}

float Mouse::GetSpeed()
{
    return SpeedVal;
}
