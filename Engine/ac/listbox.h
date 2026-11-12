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
//
//
//=============================================================================
#pragma once

#include "gui/guilistbox.h"

using AGS::Common::GUIListBox;

int ListBox_AddItem(GUIListBox *listbox, const char *text);
int ListBox_InsertItemAt(GUIListBox *listbox, int index, const char *text);
void ListBox_Clear(GUIListBox *listbox);
void ListBox_FillDirList(GUIListBox *listbox, const char *file_mask);
int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index);
int ListBox_FillSaveGameList(GUIListBox *listbox);
int ListBox_FillSaveGameList4(GUIListBox *listbox, int min_slot, int max_slot, int file_sort, int sort_dir);
int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y);
char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer);
const char *ListBox_GetItems(GUIListBox *listbox, int index);
void ListBox_SetItemText(GUIListBox *listbox, int index, const char *new_text);
void ListBox_RemoveItem(GUIListBox *listbox, int index);
int ListBox_GetItemCount(GUIListBox *listbox);
int ListBox_GetFont(GUIListBox *listbox);
void ListBox_SetFont(GUIListBox *listbox, int new_font);
int ListBox_GetSelectedIndex(GUIListBox *listbox);
void ListBox_SetSelectedIndex(GUIListBox *listbox, int new_index);
int ListBox_GetTopItem(GUIListBox *listbox);
void ListBox_SetTopItem(GUIListBox *listbox, int item);
int ListBox_GetRowCount(GUIListBox *listbox);
void ListBox_ScrollDown(GUIListBox *listbox);
void ListBox_ScrollUp(GUIListBox *listbox);
