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

#include "ac/dialogtopic.h"
#include "ac/dynobj/scriptobjects.h"

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

// Script objects of the dialogs, by dialog number
extern std::vector<ScriptDialog> scrDialog;
// Dialog topics of the game, by dialog number
extern std::vector<DialogTopic> dialog;
