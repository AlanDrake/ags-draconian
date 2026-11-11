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

#include "ac/dynobj/scriptobjects.h"
#include "gui/guidefines.h"
#include "gui/guimain.h"

ScriptGUI *GUI_AsTextWindow(ScriptGUI *sgui);
int GUI_GetPopupStyle(ScriptGUI *sgui);
void GUI_SetVisible(ScriptGUI *sgui, int is_visible);
int GUI_GetVisible(ScriptGUI *sgui);
int GUI_GetX(ScriptGUI *sgui);
void GUI_SetX(ScriptGUI *sgui, int x);
int GUI_GetY(ScriptGUI *sgui);
void GUI_SetY(ScriptGUI *sgui, int y);
void GUI_SetPosition(ScriptGUI *sgui, int x, int y);
void GUI_SetSize(ScriptGUI *sgui, int width, int height);
int GUI_GetWidth(ScriptGUI *sgui);
int GUI_GetHeight(ScriptGUI *sgui);
void GUI_SetWidth(ScriptGUI *sgui, int new_width);
void GUI_SetHeight(ScriptGUI *sgui, int new_height);
void GUI_SetZOrder(ScriptGUI *sgui, int z);
int GUI_GetZOrder(ScriptGUI *sgui);
void GUI_SetClickable(ScriptGUI *sgui, int clickable);
int GUI_GetClickable(ScriptGUI *sgui);
int GUI_GetID(ScriptGUI *sgui);
AGS::Common::GUIControl *GUI_GetiControls(ScriptGUI *sgui, int idx);
int GUI_GetControlCount(ScriptGUI *sgui);
void GUI_SetPopupYPos(ScriptGUI *sgui, int popup_y);
int GUI_GetPopupYPos(ScriptGUI *sgui);
void GUI_SetOpacity(ScriptGUI *sgui, int alpha);
int GUI_GetOpacity(ScriptGUI *sgui);
void GUI_Centre(ScriptGUI *sgui);
void GUI_SetBackgroundGraphic(ScriptGUI *sgui, int sprite);
int GUI_GetBackgroundGraphic(ScriptGUI *sgui);
void GUI_SetBackgroundColor(ScriptGUI *sgui, int color);
int GUI_GetBackgroundColor(ScriptGUI *sgui);
void GUI_SetBorderColor(ScriptGUI *sgui, int color);
int GUI_GetBorderColor(ScriptGUI *sgui);
void GUI_SetTextColor(ScriptGUI *sgui, int color);
int GUI_GetTextColor(ScriptGUI *sgui);
void GUI_SetTextPadding(ScriptGUI *sgui, int padding);
int GUI_GetTextPadding(ScriptGUI *sgui);
ScriptGUI *GUI_GetAtScreenXY(int x, int y, int hit_options);
int GetGUIAt(int x, int y, int hit_options);


void remove_popup_interface(int gui_id);
void process_interface_click(int gui_id, int btn_id, int mbut);
void update_gui_zorder();
// Initializes all GUI and controls for runtime state;
// this must be done after creating or loading GUIs.
void prepare_gui_runtime(bool startup);
void update_gui_disabled_status();
int adjust_x_for_guis(int x, int y, bool assume_blocking = false);
int adjust_y_for_guis(int y, bool assume_blocking = false);
// Create or resize GUI surface, accomodating for any GUI transformations

int gui_get_interactable(int x, int y);
int gui_on_mouse_move(const int mx, const int my);
void gui_on_mouse_hold(const int wasongui, const int wasbutdown);
void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my);
void gui_on_mouse_down(const int gui_id, const int mbut, const int mx, const int my);

AGS::Common::GUITextBox *gui_get_active_textbox();

// currently displayed pop-up GUI (-1 if none)
extern int ifacepopped;
// index of GUI the cursor was on
extern int mouse_on_iface;
extern std::vector<AGS::Common::GUIMain> guis;
extern std::vector<AGS::Common::GUIButton> guibuts;
extern std::vector<AGS::Common::GUIInvWindow> guiinv;
extern std::vector<AGS::Common::GUILabel> guilabels;
extern std::vector<AGS::Common::GUIListBox> guilist;
extern std::vector<AGS::Common::GUISlider> guislider;
extern std::vector<AGS::Common::GUITextBox> guitext;

namespace AGS
{
namespace Engine
{
namespace GUIE
{
// Mark all existing GUI for redraw
void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl);
// Mark all translatable GUI controls for redraw
void MarkForTranslationUpdate();
// Mark all GUI which use the given font for recalculate/redraw;
// pass -1 to update all the textual controls together
void MarkForFontUpdate(int font);
// Mark labels that acts as special text placeholders for redraw
void MarkSpecialLabelsForUpdate(AGS::Common::GUILabelMacro macro);
// Mark inventory windows for redraw, optionally only ones linked to given character;
// also marks buttons with inventory icon mode;
// pass char_id = -1 to update all inventory windows at once.
void MarkInventoryForUpdate(int char_id, bool is_player);
// Mark all *disabled* GUI controls for redraw;
// this function is used when the "disabled style" changes at runtime
void MarkDisabledGUIForUpdate();
} // namespace GUIE
} // namespace Engine
} // namespace AGS
