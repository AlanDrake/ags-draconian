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

#include <vector>

#include "game/dialog_topic.h"
#include "game/dynobj/script_objects.h"

// Script function: the dialog's number
int Dialog_GetID(ScriptDialog *script_dialog);
// Script function: the number of options in the dialog
int Dialog_GetOptionCount(ScriptDialog *script_dialog);
// Script function: whether the dialog shows the text parser (1 or 0)
int Dialog_GetShowTextParser(ScriptDialog *script_dialog);
// Script function: the text of an option (1-based), as a new script string
const char *Dialog_GetOptionText(ScriptDialog *script_dialog, int option);
// Script function: shows the dialog's options and returns the chosen one (1-based, or CHOSE_TEXTPARSER);
// say_chosen_option is 1 (say it unless the option is no-repeat), 2 (always say it) or 3 (never)
int Dialog_DisplayOptions(ScriptDialog *script_dialog, int say_chosen_option);
// Script function: the state of an option (1-based): 1 is on, 0 is off, 2 is permanently off
int Dialog_GetOptionState(ScriptDialog *script_dialog, int option);
// Script function: tells if an option (1-based) has been chosen before (1 or 0)
int Dialog_HasOptionBeenChosen(ScriptDialog *script_dialog, int option);
// Script function: sets the state of an option (1-based): 1 is on, 0 is off, 2 is permanently off
void Dialog_SetOptionState(ScriptDialog *script_dialog, int option, int new_state);
// Script function: starts the dialog
void Dialog_Start(ScriptDialog *script_dialog);
// Script function: stops the running dialog, or closes the displayed dialog options
void StopDialog();

// Starts a dialog, at its entry start_opt (0 is the starting entry)
void do_conversation(int dialog_id, int start_opt = 0);
// Tells if the game is currently running a dialog
bool is_in_dialog();
// Tells if the game is currently displaying dialog options;
// this may be inside a Dialog, or if Dialog.DisplayOptions was called
bool is_in_dialogoptions();
// Tells if the game is currently running a dialog, and a dialog script is being executed
bool is_dialog_executing_script();
// Commands dialog executor to goto a different dialog topic after current option's script has finished executing
void set_dialog_result_goto(int dialog_id, int start_opt = 0);
// Commands dialog executor to stop a dialog after current option's script has finished executing
void set_dialog_result_stop();
// Displays dialog options, and returns the chosen number (0-based), or CHOSE_TEXTPARSER if parser input was
// activated, or -1 if nothing was chosen (e.g. all options are off); optionally runs the game loops in the
// background meanwhile
int show_dialog_options(int dialog_id, bool run_game_loops_in_background);
// Handles a dialog option (0-based dialog_choice, as returned by show_dialog_options), optionally says its
// text (say_chosen_option as in Dialog_DisplayOptions), optionally runs the corresponding dialog script's
// entry; returns a result of the dialog script, or 0 if it was not run
int run_dialog_option(int dialog_id, int dialog_choice, int say_chosen_option, bool run_script);
// Handles a game-state changing command (such as StartDialog) inside "dialog_request" callback.
// Returns whether the change was handled in "dialog's way", and further processing is not necessary.
// Otherwise should process the command as normal.
bool handle_state_change_in_dialog_request(const char *api_name, int request_result, int start_opt = 0);
// Schedule dialog state to stop next time it receives a control
void schedule_dialog_stop();
// Disposes dialog state object(s)
void shutdown_dialog_state();

struct ScriptOverlay;

// Script API: Dialog::StartOption^1
void Dialog_StartOption(ScriptDialog *script_dialog, int option);

// Script API: Dialog::SetHasOptionBeenChosen^2
void Dialog_SetHasOptionBeenChosen(ScriptDialog *script_dialog, int option, bool chosen);

// Script API: Dialog::SetOptionText^2
void Dialog_SetOptionText(ScriptDialog *script_dialog, int option, const char *text);

// Script API: Dialog::get_ScriptName
const char *Dialog_GetScriptName(ScriptDialog *script_dialog);

// Script API: DialogOptions::get_AreDisplayed
int DialogOptions_GetAreDisplayed();

// Script API: DialogOptions::get_BulletGraphic
int DialogOptions_GetBulletGraphic();

// Script API: DialogOptions::set_BulletGraphic
void DialogOptions_SetBulletGraphic(int sprite);

// Script API: DialogOptions::get_DrawMode
int DialogOptions_GetDrawMode();

// Script API: DialogOptions::set_DrawMode
void DialogOptions_SetDrawMode(int draw_mode);

// Script API: DialogOptions::get_Font
int DialogOptions_GetFont();

// Script API: DialogOptions::set_Font
void DialogOptions_SetFont(int font_id);

// Script API: DialogOptions::get_ItemNumbering
int DialogOptions_GetItemNumbering();

// Script API: DialogOptions::set_ItemNumbering
void DialogOptions_SetItemNumbering(int style);

// Script API: DialogOptions::get_HighlightColor
int DialogOptions_GetHighlightColor();

// Script API: DialogOptions::set_HighlightColor
void DialogOptions_SetHighlightColor(int color);

// Script API: DialogOptions::get_ReadColor
int DialogOptions_GetReadColor();

// Script API: DialogOptions::set_ReadColor
void DialogOptions_SetReadColor(int color);

// Script API: DialogOptions::get_TextAlignment
int DialogOptions_GetTextAlignment();

// Script API: DialogOptions::set_TextAlignment
void DialogOptions_SetTextAlignment(int align);

// Script API: DialogOptions::get_ZOrder
int DialogOptions_GetZOrder();

// Script API: DialogOptions::set_ZOrder
void DialogOptions_SetZOrder(int zorder);

// Script API: DialogOptions::get_ItemGap
int DialogOptions_GetItemGap();

// Script API: DialogOptions::set_ItemGap
void DialogOptions_SetItemGap(int gap);

// Script API: DialogOptions::get_TemplateGUI
ScriptGUI *DialogOptions_GetTemplateGUI();

// Script API: DialogOptions::set_TemplateGUI
void DialogOptions_SetTemplateGUI(ScriptGUI *sgui);

// Script API: DialogOptions::get_GUIX
int DialogOptions_GetGUIX();

// Script API: DialogOptions::set_GUIX
void DialogOptions_SetGUIX(int x);

// Script API: DialogOptions::get_GUIY
int DialogOptions_GetGUIY();

// Script API: DialogOptions::set_GUIY
void DialogOptions_SetGUIY(int y);

// Script API: DialogOptions::get_PaddingX
int DialogOptions_GetPaddingX();

// Script API: DialogOptions::set_PaddingX
void DialogOptions_SetPaddingX(int x);

// Script API: DialogOptions::get_PaddingY
int DialogOptions_GetPaddingY();

// Script API: DialogOptions::set_PaddingY
void DialogOptions_SetPaddingY(int y);

// Script API: DialogOptions::get_MaxGUIWidth
int DialogOptions_GetMaxGUIWidth();

// Script API: DialogOptions::set_MaxGUIWidth
void DialogOptions_SetMaxGUIWidth(int width);

// Script API: DialogOptions::get_MinGUIWidth
int DialogOptions_GetMinGUIWidth();

// Script API: DialogOptions::set_MinGUIWidth
void DialogOptions_SetMinGUIWidth(int width);

// Script API: Dialog::GetProperty^1
int Dialog_GetProperty(ScriptDialog *script_dialog, const char *property);

// Script API: Dialog::GetTextProperty^1
const char *Dialog_GetTextProperty(ScriptDialog *script_dialog, const char *property);

// Script API: Dialog::SetProperty^2
bool Dialog_SetProperty(ScriptDialog *script_dialog, const char *property, int value);

// Script API: Dialog::SetTextProperty^2
bool Dialog_SetTextProperty(ScriptDialog *script_dialog, const char *property, const char *value);

// Script API: DialogOptions::get_HasCustomRender
bool DialogOptions_GetHasCustomRender();

// Script API: DialogOptions::get_ItemCount
int DialogOptions_GetItemCount();

// Script API: DialogOptions::geti_ItemOptionID
int DialogOptions_GetItemOptionID(int index);

// Script API: DialogOptions::geti_ItemX
int DialogOptions_GetItemX(int index);

// Script API: DialogOptions::geti_ItemY
int DialogOptions_GetItemY(int index);

// Script API: DialogOptions::geti_ItemWidth
int DialogOptions_GetItemWidth(int index);

// Script API: DialogOptions::geti_ItemHeight
int DialogOptions_GetItemHeight(int index);

// Script API: Dialog::GetByName, Dialog::GetByScriptName
ScriptDialog *Dialog_GetByName(const char *name);

// Script API: Dialog::Stop
void Dialog_Stop();

// Script API: Dialog::get_CurrentDialog
ScriptDialog *Dialog_GetCurrentDialog();

// Script API: Dialog::get_ExecutedOption
int Dialog_GetExecutedOption();

// Script API: DialogOptions::get_Overlay
ScriptOverlay *DialogOptions_GetOverlay();

// Script API: Dialog::get_AreOptionsDisplayed
bool Dialog_GetAreOptionsDisplayed();
