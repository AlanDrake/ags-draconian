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
// Legacy built-in GUI dialogs and controls.
//
//=============================================================================
#pragma once

#include "gui/guidialogdefines.h"
#include "platform/types.h"

// Opens a window on the GUI screen and returns its id; the area drawn is 2 pixels larger than
// the given one on each side. Quits if 5 windows are already open. Pair with CSCIEraseWindow.
int CSCIDrawWindow(int x, int y, int width, int height);
// Closes the window of the given id, returns to the window below it, and clears the GUI screen.
void CSCIEraseWindow(int win_id);
// Runs the game loop until a control or key message arrives, and puts it into msg;
// returns 0 always. Return and Escape give CM_COMMAND with the id of the default or cancel
// control (-1 if there is none), other keys go to the listbox or textbox of the top window.
int CSCIWaitMessage(CSCIMessage *msg);
// Creates a control in the top window, draws it and returns its id (1 or more). The position
// is relative to the window; for a push button, width -1 means the title's width plus 20.
// Quits if 19 controls exist already or the type (low byte of typeandflags) is unknown.
int CSCICreateControl(int typeandflags, int x, int y, int width, int height, const char *title);
// Deletes the control of the given id; an id that has no control is ignored.
void CSCIDeleteControl(int ctrl_id);
// Sends a message to a control and returns its answer, or -1 if no control has this id.
int CSCISendControlMessage(int ctrl_id, int msg, int w_param, intptr_t ip_param);
// Returns the "pressed on" result of the first control (by id) under the mouse cursor and
// records its id in controlid; 0 if mouse input is disabled or no control is hit.
int checkcontrols();
// Returns the id of the first control of the top window that has any of the flags in flag_mask
// in its type and flags, or -1.
int finddefaultcontrol(int flag_mask);
