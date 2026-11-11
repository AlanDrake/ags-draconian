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

// Script API functions of the GUI struct. The Get/Set pairs read and write the GUI property of the same name
// (the Set ones of the colour and padding pair apply to the matching kind of GUI only, see below).
// A ScriptGUI wraps a GUI index; the GUI itself is guis[sgui->id].
// Returns the same GUI if it is a text window, otherwise nullptr.
ScriptGUI *GUI_AsTextWindow(ScriptGUI *sgui);
int GUI_GetPopupStyle(ScriptGUI *sgui);
void GUI_SetVisible(ScriptGUI *sgui, int is_visible);
int GUI_GetVisible(ScriptGUI *sgui);
int GUI_GetX(ScriptGUI *sgui);
void GUI_SetX(ScriptGUI *sgui, int x);
int GUI_GetY(ScriptGUI *sgui);
void GUI_SetY(ScriptGUI *sgui, int y);
void GUI_SetPosition(ScriptGUI *sgui, int x, int y);
// Sets the size; a side below 1 is raised to 1 with a script warning.
void GUI_SetSize(ScriptGUI *sgui, int width, int height);
int GUI_GetWidth(ScriptGUI *sgui);
int GUI_GetHeight(ScriptGUI *sgui);
// Like SetSize, one side at a time.
void GUI_SetWidth(ScriptGUI *sgui, int new_width);
void GUI_SetHeight(ScriptGUI *sgui, int new_height);
void GUI_SetZOrder(ScriptGUI *sgui, int z);
int GUI_GetZOrder(ScriptGUI *sgui);
void GUI_SetClickable(ScriptGUI *sgui, int clickable);
int GUI_GetClickable(ScriptGUI *sgui);
int GUI_GetID(ScriptGUI *sgui);
// Returns the control with the given index in the GUI, or nullptr if out of range.
AGS::Common::GUIControl *GUI_GetiControls(ScriptGUI *sgui, int idx);
int GUI_GetControlCount(ScriptGUI *sgui);
// Popup Y position: ignored for text windows.
void GUI_SetPopupYPos(ScriptGUI *sgui, int popup_y);
int GUI_GetPopupYPos(ScriptGUI *sgui);
// Opacity is 0-255; other values quit with an error.
void GUI_SetOpacity(ScriptGUI *sgui, int alpha);
int GUI_GetOpacity(ScriptGUI *sgui);
// Centres the GUI in the UI viewport.
void GUI_Centre(ScriptGUI *sgui);
void GUI_SetBackgroundGraphic(ScriptGUI *sgui, int sprite);
int GUI_GetBackgroundGraphic(ScriptGUI *sgui);
void GUI_SetBackgroundColor(ScriptGUI *sgui, int color);
int GUI_GetBackgroundColor(ScriptGUI *sgui);
// Border color: ignored for text windows (the colour is then the text color).
void GUI_SetBorderColor(ScriptGUI *sgui, int color);
int GUI_GetBorderColor(ScriptGUI *sgui);
// Text color: text windows only, ignored for normal GUIs (which read back 0).
void GUI_SetTextColor(ScriptGUI *sgui, int color);
int GUI_GetTextColor(ScriptGUI *sgui);
// Text padding: text windows only, ignored for normal GUIs.
void GUI_SetTextPadding(ScriptGUI *sgui, int padding);
int GUI_GetTextPadding(ScriptGUI *sgui);
// Returns the topmost displayed GUI at the screen position, or nullptr; kHit_Interactable in hit_options
// restricts the test to the clickable GUIs.
ScriptGUI *GUI_GetAtScreenXY(int x, int y, int hit_options);
// Same as GUI_GetAtScreenXY, returns the GUI index or -1.
int GetGUIAt(int x, int y, int hit_options);

// Hides the pop-up GUI if it is the one currently popped up (ifacepopped), and restores the game state
// (unpauses, mouse cursor look) that popping it up changed.
void remove_popup_interface(int gui_id);
// Runs the script handler of a click on a GUI (btn_id < 0: the GUI background) or on one of its controls;
// falls back to the old-style interface_click function when the control has no handler of its own.
void process_interface_click(int gui_id, int btn_id, int mbut);
// Re-sorts play.gui_draw_order by z-order (ties by GUI index).
void update_gui_zorder();
// Initializes all GUI and controls for runtime state;
// this must be done after creating or loading GUIs.
void prepare_gui_runtime(bool startup);
// Applies the "interface disabled" state to the GUIs: marks them for redraw if the way they look changed.
void update_gui_disabled_status();
// Moves a message position right, if a displayed GUI covers it (not when GUIs are all off).
int adjust_x_for_guis(int x, int y, bool assume_blocking = false);
// Same for the vertical position: moves it down.
int adjust_y_for_guis(int y, bool assume_blocking = false);

// Returns the index of the GUI that is interactable at the position (-1 if none, or all GUIs are off).
int gui_get_interactable(int x, int y);
// Pops up the PopupAtY GUIs the mouse reached; returns the index of the nearest interactable GUI under
// the mouse, or -1.
int gui_on_mouse_move(const int mx, const int my);
// Mouse button is held over a GUI: repeats the activation of a dragged slider.
void gui_on_mouse_hold(const int wasongui, const int wasbutdown);
// Mouse button released over a GUI: runs the click of the activated control and the GUI mouse-up event.
void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my);
// Mouse button pressed over a GUI: lets the control under the mouse react, or runs the GUI's own click handler.
void gui_on_mouse_down(const int gui_id, const int mbut, const int mx, const int my);

// Returns the first enabled, visible text box of a displayed GUI, or nullptr (also when GUIs are disabled).
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
