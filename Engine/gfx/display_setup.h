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
// Display configuration: what the engine config asks of the display.
//
//=============================================================================
#pragma once

#include "gfx/gfxdefines.h"
#include "util/geometry.h"
#include "util/string.h"

// WindowSizeHint is an auxiliary value telling which kind of size was requested
// by configuration. This is primarily meant for precise parsing and writing
// of a config file, and for the setup application.
enum WindowSizeHint
{
    kWndSizeHint_Desktop,    // use current desktop resolution
    kWndSizeHint_GameNative, // use native game resolution
    kWndSizeHint_Explicit,   // explicit size (WxH)
    kWndSizeHint_GameScale   // as a scale factor of native game res
};

// Filter configuration
struct GfxFilterSetup
{
    AGS::Common::String ID; // filter ID
};

// Defines how game frame is scaled inside a larger window
enum FrameScaleDef
{
    kFrame_Undefined = -1,
    kFrame_Round,        // max round (integer) scaling factor
    kFrame_Stretch,      // resize to maximal possible inside the display box
    kFrame_Proportional, // same as stretch, but keep game's aspect ratio
    kNumFrameScaleDef
};

// Configuration that is used to determine the size and style of the window
struct WindowSetup
{
    WindowSizeHint SizeHint = kWndSizeHint_Explicit;
    ::Size Size;                                               // explicit screen metrics
    int Scale = 0;                                             // explicit game scale factor
    AGS::Engine::WindowMode Mode = AGS::Engine::kWnd_Windowed; // window mode

    inline bool IsDefaultSize() const { return Size.IsNull() && Scale == 0; }

    WindowSetup() = default;
    WindowSetup(WindowSizeHint hint, const ::Size &sz, AGS::Engine::WindowMode mode = AGS::Engine::kWnd_Windowed)
        : SizeHint(hint),
          Size(sz),
          Mode(mode)
    {
    }
    WindowSetup(const ::Size &sz, AGS::Engine::WindowMode mode = AGS::Engine::kWnd_Windowed)
        : SizeHint(kWndSizeHint_Explicit),
          Size(sz),
          Mode(mode)
    {
    }
    WindowSetup(int scale, AGS::Engine::WindowMode mode = AGS::Engine::kWnd_Windowed)
        : SizeHint(kWndSizeHint_GameScale),
          Scale(scale),
          Mode(mode)
    {
    }
    WindowSetup(AGS::Engine::WindowMode mode) // Init with mode type only assumes using "desktop resolution"
        : SizeHint(kWndSizeHint_Desktop),
          Mode(mode)
    {
    }
};

// Additional parameters for the display mode setup
struct DisplayParamsEx
{
    int DisplayIndex = 0; // 0-based display index
    int RefreshRate = 0;  // gfx mode refresh rate
    bool VSync = false;   // vertical sync

    DisplayParamsEx(int display_index, int rate, bool vsync)
        : DisplayIndex(display_index),
          RefreshRate(rate),
          VSync(vsync)
    {
    }
};

// Full graphics configuration, contains graphics driver selection,
// alternate settings for windowed and fullscreen modes and gfx filter setup.
struct DisplayModeSetup
{
    AGS::Common::String DriverID; // graphics driver ID

    // Definitions for the fullscreen and windowed modes and scaling methods.
    // When the initial display mode is set, corresponding scaling method from this pair is used.
    // The second method is meant to be saved and used if display mode is switched at runtime.
    WindowSetup FsSetup;        // definition of the fullscreen mode
    WindowSetup WinSetup;       // definition of the windowed mode
    FrameScaleDef FsGameFrame = // how the game frame should be scaled/positioned in fullscreen mode
        kFrame_Undefined;
    FrameScaleDef WinGameFrame = // how the game frame should be scaled/positioned in windowed mode
        kFrame_Undefined;

    bool UseDefaultDisplay = true; // used when writing config back
    int DisplayIndex = 0;          // 0-based display index
    bool Windowed = false;         // initial mode
    int RefreshRate = 0;           // gfx mode refresh rate
    bool VSync = false;            // vertical sync

    GfxFilterSetup Filter; // graphics filter definition
};
