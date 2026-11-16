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
// Screen effects and script API
//
//=============================================================================
#pragma once

#include <memory>

#include "game/game_state.h"

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS
namespace AGS
{
namespace Engine
{
class IDriverDependantBitmap;
}
} // namespace AGS

// Runs fade-in effect using explicit settings
void run_fade_in_effect(ScreenTransitionStyle style, int speed);
// Runs fade-out effect using explicit settings
void run_fade_out_effect(ScreenTransitionStyle style, int speed);
// Runs fade-in transition using default settings
void current_fade_in_effect();
// Runs fade-out transition using default settings
void current_fade_out_effect();

class ScriptShaderInstance;
struct ScriptUserObject;
struct ScriptViewport;

// Script API: Screen::get_Width
int Screen_GetScreenWidth();

// Script API: Screen::get_Height
int Screen_GetScreenHeight();

// Script API: Screen::get_AutoSizeViewportOnRoomLoad
bool Screen_GetAutoSizeViewport();

// Script API: Screen::set_AutoSizeViewportOnRoomLoad
void Screen_SetAutoSizeViewport(bool on);

// Script API: Screen::get_Viewport
ScriptViewport *Screen_GetViewport();

// Script API: Screen::get_ViewportCount
int Screen_GetViewportCount();

// Script API: Screen::geti_Viewports
ScriptViewport *Screen_GetAnyViewport(int index);

// Script API: Screen::get_Shader
ScriptShaderInstance *Screen_GetShader();

// Script API: Screen::set_Shader
void Screen_SetShader(ScriptShaderInstance *shader_inst);

// Script API: Screen::ScreenToRoomPoint^3
ScriptUserObject *Screen_ScreenToRoomPoint(int scrx, int scry, bool restrict);

// Script API: Screen::ScreenToRoomPoint^2
ScriptUserObject *Screen_ScreenToRoomPoint2(int scrx, int scry);

// Script API: Screen::RoomToScreenPoint
ScriptUserObject *Screen_RoomToScreenPoint(int roomx, int roomy);

void FlipScreen(int direction);
void ShakeScreen(int severe);
void ShakeScreenBackground(int delay, int amount, int length);
void TintScreen(int red, int grn, int blu);
void SetScreenTransition(int newtrans);
void SetNextScreenTransition(int newtrans);
void SetFadeColor(int red, int green, int blue);
void FadeIn(int sppd);
void FadeOut(int sppd);
