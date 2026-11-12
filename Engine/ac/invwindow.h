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
// Script API of the InvWindow GUI control: the implementations behind the
// properties of the InvWindow script type.
//
//=============================================================================
#pragma once

#include "ac/characterinfo.h"
#include "ac/dynobj/scriptobjects.h"
#include "gui/guiinv.h"

using AGS::Common::GUIInvWindow;

// Sets the character whose inventory is shown (null for none) and scrolls to the top.
void InvWindow_SetCharacterToUse(GUIInvWindow *guii, CharacterInfo *ch);
// Returns the character whose inventory is shown, or null if none is set.
CharacterInfo *InvWindow_GetCharacterToUse(GUIInvWindow *guii);
// Sets the width of one item cell in pixels and recalculates the layout.
void InvWindow_SetItemWidth(GUIInvWindow *guii, int new_width);
// Returns the width of one item cell in pixels.
int InvWindow_GetItemWidth(GUIInvWindow *guii);
// Sets the height of one item cell in pixels and recalculates the layout.
void InvWindow_SetItemHeight(GUIInvWindow *guii, int new_height);
// Returns the height of one item cell in pixels.
int InvWindow_GetItemHeight(GUIInvWindow *guii);
// Sets the index of the first inventory item shown.
void InvWindow_SetTopItem(GUIInvWindow *guii, int topitem);
// Returns the index of the first inventory item shown.
int InvWindow_GetTopItem(GUIInvWindow *guii);
// Returns the number of item columns.
int InvWindow_GetItemsPerRow(GUIInvWindow *guii);
// Returns the number of items in the shown character's inventory.
int InvWindow_GetItemCount(GUIInvWindow *guii);
// Returns the number of item rows.
int InvWindow_GetRowCount(GUIInvWindow *guii);
// Scrolls down by one row, if there are items beyond the last visible row.
void InvWindow_ScrollDown(GUIInvWindow *guii);
// Scrolls up by one row, if not at the top.
void InvWindow_ScrollUp(GUIInvWindow *guii);
// Returns the inventory item at the index of the shown character's inventory, or null if out of range.
ScriptInvItem *InvWindow_GetItemAtIndex(GUIInvWindow *guii, int index);
// Returns the id of the inventory item at the position (relative to the control), or -1 if there is none.
int InvWindow_GetItemAtXY(GUIInvWindow *inv, int at_x, int at_y);

//=============================================================================
