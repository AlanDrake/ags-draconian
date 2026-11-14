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
// Script bindings registered by RegisterInventoryItemAPI().
//
//=============================================================================
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/inventory_item.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterInventoryItemAPI()
{
    ScFnRegister invitem_api[] = {
        ScStatic<&InventoryItem_GetAtScreenXY2>("InventoryItem::GetAtScreenXY^2"),
        ScStatic<&InventoryItem_GetAtScreenXY>("InventoryItem::GetAtScreenXY^4"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&InventoryItem_GetByName>("InventoryItem::GetByName"),
        ScStatic<&InventoryItem_GetByName>("InventoryItem::GetByScriptName"),

        ScMethod<&InventoryItem_CheckInteractionAvailable>("InventoryItem::IsInteractionAvailable^1"),
        ScMethod<&InventoryItem_GetProperty>("InventoryItem::GetProperty^1"),
        ScMethod<&InventoryItem_GetPropertyText>("InventoryItem::GetPropertyText^2"),
        ScMethod<&InventoryItem_GetTextProperty>("InventoryItem::GetTextProperty^1"),
        ScMethod<&InventoryItem_SetProperty>("InventoryItem::SetProperty^2"),
        ScMethod<&InventoryItem_SetTextProperty>("InventoryItem::SetTextProperty^2"),
        ScMethod<&InventoryItem_RunInteraction>("InventoryItem::RunInteraction^1"),
        ScMethod<&InventoryItem_SetName>("InventoryItem::SetName^1"),
        ScMethod<&InventoryItem_GetCursorGraphic>("InventoryItem::get_CursorGraphic"),
        ScMethod<&InventoryItem_SetCursorGraphic>("InventoryItem::set_CursorGraphic"),
        ScMethod<&InventoryItem_GetCursorHotspotAlignment>("InventoryItem::get_CursorHotspotAlignment"),
        ScMethod<&InventoryItem_SetCursorHotspotAlignment>("InventoryItem::set_CursorHotspotAlignment"),
        ScMethod<&InventoryItem_GetCursorHotspotX>("InventoryItem::get_CursorHotspotX"),
        ScMethod<&InventoryItem_SetCursorHotspotX>("InventoryItem::set_CursorHotspotX"),
        ScMethod<&InventoryItem_GetCursorHotspotY>("InventoryItem::get_CursorHotspotY"),
        ScMethod<&InventoryItem_SetCursorHotspotY>("InventoryItem::set_CursorHotspotY"),
        ScMethod<&InventoryItem_GetName_New>("InventoryItem::get_DisplayName"),
        ScMethod<&InventoryItem_SetName>("InventoryItem::set_DisplayName"),
        ScMethod<&InventoryItem_GetGraphic>("InventoryItem::get_Graphic"),
        ScMethod<&InventoryItem_SetGraphic>("InventoryItem::set_Graphic"),
        ScMethod<&InventoryItem_GetID>("InventoryItem::get_ID"),
        // [OBSOLETE] Name => DisplayName
        ScMethod<&InventoryItem_GetName_New>("InventoryItem::get_Name"),
        ScMethod<&InventoryItem_SetName>("InventoryItem::set_Name"),
        ScMethod<&InventoryItem_GetScriptName>("InventoryItem::get_ScriptName"),
    };

    ccAddExternalFunctions(invitem_api);
}
