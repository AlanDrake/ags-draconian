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
// Script API of the ListBox GUI control: the implementations behind the
// properties of the ListBox script type.
//
//=============================================================================
#pragma once

#include "gui/guilistbox.h"

using AGS::Common::GUIListBox;

// Appends an item; returns 1 on success, 0 if it was not added.
int ListBox_AddItem(GUIListBox *listbox, const char *text);
// Inserts an item at the index (0 to the item count); returns 1 on success, 0 if the index is out of range.
int ListBox_InsertItemAt(GUIListBox *listbox, int index, const char *text);
// Removes all items.
void ListBox_Clear(GUIListBox *listbox);
// Replaces the items with the names of the files matching the mask, sorted by name.
void ListBox_FillDirList(GUIListBox *listbox, const char *file_mask);
// Returns the save slot number behind the item filled by FillSaveGameList; -1 if the item has none.
int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index);
// Replaces the items with the descriptions of the saved games, the newest first; returns 1 if every slot of the
// range is occupied.
int ListBox_FillSaveGameList(GUIListBox *listbox);
// As FillSaveGameList, for the slot range min_slot to max_slot and the given sorting; returns 1 if every slot of
// the range is occupied.
int ListBox_FillSaveGameList4(GUIListBox *listbox, int min_slot, int max_slot, int file_sort, int sort_dir);
// Returns the index of the item at the screen position, or -1 if the GUI is not displayed, the position is
// outside the list box or on its scrollbar, or there is no item there.
int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y);
// Copies the item's text into the buffer and returns the buffer; quits on an invalid index.
char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer);
// Returns the item's text as a new script string; quits on an invalid index.
const char *ListBox_GetItems(GUIListBox *listbox, int index);
// Changes the item's text; quits on an invalid index.
void ListBox_SetItemText(GUIListBox *listbox, int index, const char *new_text);
// Removes the item; quits on an invalid index.
void ListBox_RemoveItem(GUIListBox *listbox, int index);
// Returns the number of items.
int ListBox_GetItemCount(GUIListBox *listbox);
// Returns the font number of the items.
int ListBox_GetFont(GUIListBox *listbox);
// Sets the font number of the items (validated).
void ListBox_SetFont(GUIListBox *listbox, int new_font);
// Returns the index of the selected item, or -1 if none.
int ListBox_GetSelectedIndex(GUIListBox *listbox);
// Selects the item (-1 for none; an invalid index also selects none), scrolling it into view.
void ListBox_SetSelectedIndex(GUIListBox *listbox, int new_index);
// Returns the index of the first item shown.
int ListBox_GetTopItem(GUIListBox *listbox);
// Scrolls so that the item is the first shown; an index out of range is clamped to 0 to the item count, with a warning.
void ListBox_SetTopItem(GUIListBox *listbox, int item);
// Returns the number of rows that fit in the list box.
int ListBox_GetRowCount(GUIListBox *listbox);
// Scrolls down by one item, unless the last item is already shown.
void ListBox_ScrollDown(GUIListBox *listbox);
// Scrolls up by one item, unless the first item is already shown.
void ListBox_ScrollUp(GUIListBox *listbox);
