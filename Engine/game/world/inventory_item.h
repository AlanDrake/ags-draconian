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

#include "game/dynobj/script_objects.h"
#include "game/inventory_item_def.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

// The play state of an inventory item: what exists only while the game runs. InventoryItem holds it with its
// definition.
struct InventoryItemState
{
    // The values of the item's custom properties that the script set during play, by property name: they hide the
    // definition's values (GameDef) and the schema's defaults (game/world/properties.h)
    AGS::Common::StringIMap PropertyOverrides;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<InventoryItemState> Fields();
};

// An inventory item of the running game: its definition's fields, which start as a copy of what the game file says,
// and its play state. The game's items are Game::InventoryItems, by item index (item 0 is not used); the script
// objects (ScriptInvItem) hold the index.
class InventoryItem : public InventoryItemDef, public InventoryItemState
{
public:
    InventoryItem() = default;
    // Starts an item from its definition
    explicit InventoryItem(const InventoryItemDef &def) : InventoryItemDef(def) {}
};

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

// Script API: InventoryItem::get_CursorHotspotAlignment
int InventoryItem_GetCursorHotspotAlignment(ScriptInvItem *item);

// Script API: InventoryItem::set_CursorHotspotAlignment
void InventoryItem_SetCursorHotspotAlignment(ScriptInvItem *item, int align);

// Script API: InventoryItem::get_CursorHotspotX
int InventoryItem_GetCursorHotspotX(ScriptInvItem *item);

// Script API: InventoryItem::set_CursorHotspotX
void InventoryItem_SetCursorHotspotX(ScriptInvItem *item, int hotspot_x);

// Script API: InventoryItem::get_CursorHotspotY
int InventoryItem_GetCursorHotspotY(ScriptInvItem *item);

// Script API: InventoryItem::set_CursorHotspotY
void InventoryItem_SetCursorHotspotY(ScriptInvItem *item, int hotspot_y);

// Script API: InventoryItem::get_ScriptName
const char *InventoryItem_GetScriptName(ScriptInvItem *item);

// Script API: InventoryItem::GetAtScreenXY^4
ScriptInvItem *InventoryItem_GetAtScreenXY(int x, int y, int gui_hitoptions, int inv_hitoptions);

// Script API: InventoryItem::GetAtScreenXY^2
ScriptInvItem *InventoryItem_GetAtScreenXY2(int x, int y);

// Script API: InventoryItem::SetProperty^2
bool InventoryItem_SetProperty(ScriptInvItem *item, const char *property, int value);

// Script API: InventoryItem::SetTextProperty^2
bool InventoryItem_SetTextProperty(ScriptInvItem *item, const char *property, const char *value);

// Script API: InventoryItem::GetByName, InventoryItem::GetByScriptName
ScriptInvItem *InventoryItem_GetByName(const char *name);
