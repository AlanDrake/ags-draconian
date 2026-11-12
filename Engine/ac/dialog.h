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

int Dialog_GetID(ScriptDialog *script_dialog);
int Dialog_GetOptionCount(ScriptDialog *script_dialog);
int Dialog_GetShowTextParser(ScriptDialog *script_dialog);
const char *Dialog_GetOptionText(ScriptDialog *script_dialog, int option);
int Dialog_DisplayOptions(ScriptDialog *script_dialog, int say_chosen_option);
int Dialog_GetOptionState(ScriptDialog *script_dialog, int option);
int Dialog_HasOptionBeenChosen(ScriptDialog *script_dialog, int option);
void Dialog_SetOptionState(ScriptDialog *script_dialog, int option, int new_state);
void Dialog_Start(ScriptDialog *script_dialog);
void StopDialog();

// Starts a dialog
void do_conversation(int dialog_id, int start_opt = 0);
// Tells if the game is currently running a dialog
bool is_in_dialog();
// Tells if the game is currently displaying dialog options;
// this may be inside a Dialog, or if Dialog.DisplayOptions was called
bool is_in_dialogoptions();
// Tells if the game is currently running a dialog, and a dialog script is being executed
bool is_dialog_executing_script();
// Commands dialog executor to goto a different dialog topic after current option's script have finished executing
void set_dialog_result_goto(int dialog_id, int start_opt = 0);
// Commands dialog executor to stop a dialog after current option's script have finished executing
void set_dialog_result_stop();
// Displays dialog options, and returns the chosen number, or CHOSE_TEXTPARSER if parser input was activated
int show_dialog_options(int dialog_id, bool run_game_loops_in_background);
// Handles a dialog option, optionally "sais" its text, optionally run corresponding dialog script's entry
int run_dialog_option(int dialog_id, int dialog_choice, int say_chosen_option, bool run_script);
// Handles a game-state changing command (such as StartDialog) inside "dialog_request" callback.
// Returns whether the change was handled in "dialog's way", and further processing is not necessary.
// Otherwise should process the command as normal.
bool handle_state_change_in_dialog_request(const char *api_name, int request_result, int start_opt = 0);
// Shedule dialog state to stop next time it receives a control
void schedule_dialog_stop();
// Disposes dialog state object(s)
void shutdown_dialog_state();

extern std::vector<ScriptDialog> scrDialog;
extern std::vector<DialogTopic> dialog;
