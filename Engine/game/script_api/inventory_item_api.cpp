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
#include "ac/dynobj/cc_inventory.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "game/world/inventory_item.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_InventoryItem_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptInvItem, ccDynamicInv, InventoryItem_GetByName, const char);
}

RuntimeScriptValue Sc_InventoryItem_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT4(ScriptInvItem, ccDynamicInv, InventoryItem_GetAtScreenXY);
}

RuntimeScriptValue Sc_InventoryItem_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptInvItem, ccDynamicInv, InventoryItem_GetAtScreenXY2);
}

// int (ScriptInvItem *iitem, int mood)
RuntimeScriptValue Sc_InventoryItem_CheckInteractionAvailable(void *self, const RuntimeScriptValue *params,
                                                              int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptInvItem, InventoryItem_CheckInteractionAvailable);
}

// int (ScriptInvItem *scii, const char *property)
RuntimeScriptValue Sc_InventoryItem_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptInvItem, InventoryItem_GetProperty, const char);
}

// void (ScriptInvItem *scii, const char *property, char *bufer)
RuntimeScriptValue Sc_InventoryItem_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(ScriptInvItem, InventoryItem_GetPropertyText, const char, char);
}

// const char* (ScriptInvItem *scii, const char *property)
RuntimeScriptValue Sc_InventoryItem_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetTextProperty, const char);
}

RuntimeScriptValue Sc_InventoryItem_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptInvItem, InventoryItem_SetProperty, const char);
}

RuntimeScriptValue Sc_InventoryItem_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptInvItem, InventoryItem_SetTextProperty, const char, const char);
}

// void (ScriptInvItem *iitem, int mood)
RuntimeScriptValue Sc_InventoryItem_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_RunInteraction);
}

// void (ScriptInvItem *scii, const char *newname)
RuntimeScriptValue Sc_InventoryItem_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptInvItem, InventoryItem_SetName, const char);
}

// int (ScriptInvItem *iitem)
RuntimeScriptValue Sc_InventoryItem_GetCursorGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetCursorGraphic);
}

// void (ScriptInvItem *iitem, int newSprite)
RuntimeScriptValue Sc_InventoryItem_SetCursorGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetCursorGraphic);
}

RuntimeScriptValue Sc_InventoryItem_GetCursorHotspotAlignment(void *self, const RuntimeScriptValue *params,
                                                              int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetCursorHotspotAlignment);
}

RuntimeScriptValue Sc_InventoryItem_SetCursorHotspotAlignment(void *self, const RuntimeScriptValue *params,
                                                              int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetCursorHotspotAlignment);
}

RuntimeScriptValue Sc_InventoryItem_GetCursorHotspotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetCursorHotspotX);
}

RuntimeScriptValue Sc_InventoryItem_SetCursorHotspotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetCursorHotspotX);
}

RuntimeScriptValue Sc_InventoryItem_GetCursorHotspotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetCursorHotspotY);
}

RuntimeScriptValue Sc_InventoryItem_SetCursorHotspotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetCursorHotspotY);
}

// int (ScriptInvItem *iitem)
RuntimeScriptValue Sc_InventoryItem_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetGraphic);
}

// void (ScriptInvItem *iitem, int piccy)
RuntimeScriptValue Sc_InventoryItem_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptInvItem, InventoryItem_SetGraphic);
}

// int (ScriptInvItem *scii)
RuntimeScriptValue Sc_InventoryItem_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptInvItem, InventoryItem_GetID);
}

RuntimeScriptValue Sc_InventoryItem_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetScriptName);
}

// const char* (ScriptInvItem *invitem)
RuntimeScriptValue Sc_InventoryItem_GetName_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptInvItem, const char, myScriptStringImpl, InventoryItem_GetName_New);
}


void RegisterInventoryItemAPI()
{
    ScFnRegister invitem_api[] = {
        {"InventoryItem::GetAtScreenXY^2", API_FN_PAIR(InventoryItem_GetAtScreenXY2)},
        {"InventoryItem::GetAtScreenXY^4", API_FN_PAIR(InventoryItem_GetAtScreenXY)},
        // [OBSOLETE] GetByName => GetByScriptName
        {"InventoryItem::GetByName", API_FN_PAIR(InventoryItem_GetByName)},
        {"InventoryItem::GetByScriptName", API_FN_PAIR(InventoryItem_GetByName)},

        {"InventoryItem::IsInteractionAvailable^1", API_FN_PAIR(InventoryItem_CheckInteractionAvailable)},
        {"InventoryItem::GetProperty^1", API_FN_PAIR(InventoryItem_GetProperty)},
        {"InventoryItem::GetPropertyText^2", API_FN_PAIR(InventoryItem_GetPropertyText)},
        {"InventoryItem::GetTextProperty^1", API_FN_PAIR(InventoryItem_GetTextProperty)},
        {"InventoryItem::SetProperty^2", API_FN_PAIR(InventoryItem_SetProperty)},
        {"InventoryItem::SetTextProperty^2", API_FN_PAIR(InventoryItem_SetTextProperty)},
        {"InventoryItem::RunInteraction^1", API_FN_PAIR(InventoryItem_RunInteraction)},
        {"InventoryItem::SetName^1", API_FN_PAIR(InventoryItem_SetName)},
        {"InventoryItem::get_CursorGraphic", API_FN_PAIR(InventoryItem_GetCursorGraphic)},
        {"InventoryItem::set_CursorGraphic", API_FN_PAIR(InventoryItem_SetCursorGraphic)},
        {"InventoryItem::get_CursorHotspotAlignment", API_FN_PAIR(InventoryItem_GetCursorHotspotAlignment)},
        {"InventoryItem::set_CursorHotspotAlignment", API_FN_PAIR(InventoryItem_SetCursorHotspotAlignment)},
        {"InventoryItem::get_CursorHotspotX", API_FN_PAIR(InventoryItem_GetCursorHotspotX)},
        {"InventoryItem::set_CursorHotspotX", API_FN_PAIR(InventoryItem_SetCursorHotspotX)},
        {"InventoryItem::get_CursorHotspotY", API_FN_PAIR(InventoryItem_GetCursorHotspotY)},
        {"InventoryItem::set_CursorHotspotY", API_FN_PAIR(InventoryItem_SetCursorHotspotY)},
        {"InventoryItem::get_DisplayName", API_FN_PAIR(InventoryItem_GetName_New)},
        {"InventoryItem::set_DisplayName", API_FN_PAIR(InventoryItem_SetName)},
        {"InventoryItem::get_Graphic", API_FN_PAIR(InventoryItem_GetGraphic)},
        {"InventoryItem::set_Graphic", API_FN_PAIR(InventoryItem_SetGraphic)},
        {"InventoryItem::get_ID", API_FN_PAIR(InventoryItem_GetID)},
        // [OBSOLETE] Name => DisplayName
        {"InventoryItem::get_Name", API_FN_PAIR(InventoryItem_GetName_New)},
        {"InventoryItem::set_Name", API_FN_PAIR(InventoryItem_SetName)},
        {"InventoryItem::get_ScriptName", API_FN_PAIR(InventoryItem_GetScriptName)},
    };

    ccAddExternalFunctions(invitem_api);
}
