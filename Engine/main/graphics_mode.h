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
//
//
//=============================================================================
#pragma once

#include "gfx/display_setup.h"
#include "gfx/gfxdefines.h"
#include "util/scaling.h"
#include "util/string.h"

using AGS::Common::String;
using AGS::Common::GraphicResolution;
using AGS::Engine::DisplayMode;
using AGS::Engine::WindowMode;

// Gets desktop size for the given display
Size get_desktop_size(int display_index);
// Gets desktop size for the displays that hosts game window
Size get_desktop_size();

namespace AGS
{
namespace Engine
{
class IGfxModeList;
}
} // namespace AGS
bool find_nearest_supported_mode(const AGS::Engine::IGfxModeList &modes, const Size &wanted_size, const int color_depth,
                                 const Size *ratio_reference, const Size *upper_bound, AGS::Engine::DisplayMode &dm,
                                 int *mode_index = nullptr);


// Display mode color depth variants suggested for the use
struct ColorDepthOption
{
    int Bits;    // color depth value in bits
    bool Forced; // whether the depth should be forced, or driver's recommendation used

    ColorDepthOption() : Bits(0), Forced(false) {}
    ColorDepthOption(int bits, bool forced = false) : Bits(bits), Forced(forced) {}
};

// ActiveDisplaySetting struct merges DisplayMode and FrameScaleDef,
// which is useful if you need to save active settings and reapply them later.
struct ActiveDisplaySetting
{
    DisplayMode Dm;
    FrameScaleDef Frame = kFrame_Undefined;
};

// Initializes any possible gfx mode, using user config as a recommendation;
// may try all available renderers and modes before succeeding (or failing).
// Returns result, and sets original desktop size on the used display.
bool graphics_mode_init_any(const GraphicResolution &game_res, const DisplayModeSetup &setup,
                            const ColorDepthOption &color_depth, Size *init_desktop_size);
// Return last saved display mode of the given kind
ActiveDisplaySetting graphics_mode_get_last_setting(bool windowed);
// Creates graphics driver of given id
bool graphics_mode_create_renderer(const String &driver_id);
// Try to find and initialize compatible display mode as close to given setup as possible
bool graphics_mode_set_dm_any(const Size &game_size, const WindowSetup &ws, const ColorDepthOption &color_depth,
                              const FrameScaleDef frame, const DisplayParamsEx &params);
// Set the display mode with given parameters
bool graphics_mode_set_dm(const AGS::Engine::DisplayMode &dm);
// Set the native image size
bool graphics_mode_set_native_res(const GraphicResolution &native_res);
// Get current render frame setup
FrameScaleDef graphics_mode_get_render_frame();
// Set the render frame position inside the window
bool graphics_mode_set_render_frame(const FrameScaleDef frame);
// Set requested graphics filter, or default filter if the requested one failed
bool graphics_mode_set_filter_any(const GfxFilterSetup &setup);
// Set the scaling filter with given ID
bool graphics_mode_set_filter(const String &filter_id);
// Update graphic renderer and render frame when window size changes
void graphics_mode_on_window_changed(const Size &sz);
// Releases current graphic mode and shuts down renderer
void graphics_mode_shutdown();
