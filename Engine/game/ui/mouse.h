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
#pragma once

#include "device/mousew32.h"
#include "game/mouse_cursor_def.h"
#include "game/ui/cursor_graphic_state.h"

// The engine's mouse cursor; an alias of its definition until it holds more (PLAN.md 5.1a step 3)
using MouseCursor = MouseCursorDef;

void Mouse_SetVisible(int isOn);
int Mouse_GetVisible();
int Mouse_GetModeGraphic(int curs);
void Mouse_ChangeModeView(int curs, int newview, int delay);
// The Mouse:: functions are static so the script doesn't pass
// in an object parameter
void Mouse_SetPosition(int newx, int newy);
int Mouse_GetCursorMode();
void Mouse_SetNextCursor();
// permanently change cursor graphic
void Mouse_ChangeCursorGraphic(int curs, int newslot);
void Mouse_ChangeCursorHotspot(int curs, int x, int y);
int Mouse_IsButtonDown(int which);
void Mouse_SetBounds(int x1, int y1, int x2, int y2);
void Mouse_Refresh();
// mouse cursor functions:
// Mouse_SetCursor: changes visual appearance to specified cursor
void Mouse_SetCursor(int newcurs);
int Mouse_GetCursor();
// set_default_cursor: resets visual appearance to current mode (walk, look, etc);
void Mouse_SetDefaultCursor();
// set_cursor_mode: changes mode and appearance
void Mouse_SetCursorMode(int newmode);
void Mouse_EnableCursorMode(int modd);
void Mouse_DisableCursorMode(int modd);

void Mouse_SaveCursorForLocationChange();

// Try to enable or disable mouse speed control by the engine
void Mouse_EnableControl(bool on);
void Mouse_SimulateClick(int button_id);

//=============================================================================

void enable_cursor_mode(int mode);
void disable_cursor_mode(int mode);
// Set new current cursor mode
void set_cursor_mode(int newmode);
// Set new current cursor mode to the first found mode with the given Role;
// if none found, then sets to the fallback_mode
void set_cursor_mode_with_role(CursorRole role, int fallback_mode = 0);

// Tells if current mode has this standard role
bool is_current_cursor_mode(CursorRole role);
// Tells if current cursor *look* belongs to a cursor with this standard role
bool is_current_cursor_look(CursorRole role);

// Changes visual appearance of the current cursor to another cursor's looks
void set_cursor_look(int newcurs, bool force_update = false);
// Resets visual appearance to current mode (after it has been changed to something else)
void set_default_cursor_look();

void update_script_mouse_coords();
void update_inv_cursor(int invnum);
void set_new_cursor_graphic(int spriteslot);
int find_next_enabled_cursor(int startwith);
int find_previous_enabled_cursor(int startwith);

class ScriptShaderInstance;
struct ScriptUserObject;

// Script API: Mouse::GetModeHotspot^1
ScriptUserObject *Mouse_GetModeHotspot(int curs);

// Script API: Mouse::ChangeModeView^2
void Mouse_ChangeModeView2(int curs, int newview);

// Script API: Mouse::SelectPreviousMode^0
void Mouse_SetPreviousCursor();

// Script API: Mouse::IsAnyButtonDown^0
bool IsAnyButtonDown();

// Script API: Mouse::IsModeEnabled^1
int Mouse_IsModeEnabled(int which);

// Script API: Mouse::get_AutoLock
bool Mouse_GetAutoLock();

// Script API: Mouse::set_AutoLock
void Mouse_SetAutoLock(bool on);

// Script API: Mouse::get_Enabled
bool Mouse_GetEnabled();

// Script API: Mouse::set_Enabled
void Mouse_SetEnabled(bool on);

// Script API: Mouse::get_CursorShader
ScriptShaderInstance *Mouse_GetCursorShader();

// Script API: Mouse::set_CursorShader
void Mouse_SetCursorShader(ScriptShaderInstance *shader_inst);
