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

bool AssertInvItem(const char *apiname, int item_id);

void InventoryItem_SetCursorGraphic(ScriptInvItem *item, int sprite);
int InventoryItem_GetCursorGraphic(ScriptInvItem *item);
void InventoryItem_SetGraphic(ScriptInvItem *item, int sprite);
void InventoryItem_SetName(ScriptInvItem *item, const char *new_name);
int InventoryItem_GetID(ScriptInvItem *item);
const char *InventoryItem_GetName_New(ScriptInvItem *item);
int InventoryItem_GetGraphic(ScriptInvItem *item);
void InventoryItem_RunInteraction(ScriptInvItem *item, int mood);
void RunInventoryInteraction(int item_id, int mood);
int InventoryItem_CheckInteractionAvailable(ScriptInvItem *item, int mood);
int InventoryItem_GetProperty(ScriptInvItem *item, const char *property);
void InventoryItem_GetPropertyText(ScriptInvItem *item, const char *property, char *buffer);
const char *InventoryItem_GetTextProperty(ScriptInvItem *item, const char *property);
int GetInvAt(int x, int y, int gui_hitoptions, int inv_hitoptions);

void set_inv_item_cursorpic(int item_id, int sprite);

void InvItems_RegisterDynamicSpriteCallbacks();
