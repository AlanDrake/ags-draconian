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

// Tells if item_id is a valid inventory item index; if not, writes a script warning naming apiname.
bool AssertInvItem(const char *apiname, int item_id);

// Sets the sprite of the item's cursor; refreshes the cursor if this item is the one in use.
void InventoryItem_SetCursorGraphic(ScriptInvItem *item, int sprite);
// Returns the sprite of the item's cursor.
int InventoryItem_GetCursorGraphic(ScriptInvItem *item);
// Sets the item's sprite and marks the inventory GUIs for update.
void InventoryItem_SetGraphic(ScriptInvItem *item, int sprite);
// Sets the item's display name; quits if the item is invalid.
void InventoryItem_SetName(ScriptInvItem *item, const char *new_name);
// Returns the item's index.
int InventoryItem_GetID(ScriptInvItem *item);
// Returns the item's display name as a new script string.
const char *InventoryItem_GetName_New(ScriptInvItem *item);
// Returns the item's sprite.
int InventoryItem_GetGraphic(ScriptInvItem *item);
// Runs the item's event for the cursor mode, then its any-click event.
void InventoryItem_RunInteraction(ScriptInvItem *item, int mood);
// Same by item index; quits if the index is invalid.
void RunInventoryInteraction(int item_id, int mood);
// Returns 1 if the item has a handler for the cursor mode, 0 if not; no handler is run.
int InventoryItem_CheckInteractionAvailable(ScriptInvItem *item, int mood);
// Returns the item's integer custom property; the property's default value if the item has none set,
// 0 if there is no such integer property.
int InventoryItem_GetProperty(ScriptInvItem *item, const char *property);
// Copies the item's text custom property into the buffer (MAX_MAXSTRLEN bytes at most);
// leaves the buffer as it is if there is no such text property.
void InventoryItem_GetPropertyText(ScriptInvItem *item, const char *property, char *buffer);
// Returns the item's text custom property as a new script string, null if there is no such text property.
const char *InventoryItem_GetTextProperty(ScriptInvItem *item, const char *property);
// Returns the index of the item shown at the screen position by an inventory window control, -1 if none.
// The hit options say which GUIs and which controls count.
int GetInvAt(int x, int y, int gui_hitoptions, int inv_hitoptions);

// Sets the sprite of the item's cursor; refreshes the cursor if this item is the one in use.
void set_inv_item_cursorpic(int item_id, int sprite);

// Subscribes to changes of the dynamic sprites that items use, to update the inventory GUIs.
void InvItems_RegisterDynamicSpriteCallbacks();
