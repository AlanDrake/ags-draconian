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
// Script bindings registered by RegisterInventoryWindowAPI().
//
//=============================================================================
#include "ac/characterinfo.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_inventory.h"
#include "ac/game.h"
#include "game/ui/inv_window.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollDown);
}

// void (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIInvWindow, InvWindow_ScrollUp);
}

// CharacterInfo* (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIInvWindow, CharacterInfo, ccDynamicCharacter, InvWindow_GetCharacterToUse);
}

// void (GUIInvWindow *guii, CharacterInfo *chaa)
RuntimeScriptValue Sc_InvWindow_SetCharacterToUse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIInvWindow, InvWindow_SetCharacterToUse, CharacterInfo);
}

// ScriptInvItem* (GUIInvWindow *guii, int index)
RuntimeScriptValue Sc_InvWindow_GetItemAtIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIInvWindow, ScriptInvItem, ccDynamicInv, InvWindow_GetItemAtIndex);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemHeight);
}

// void (GUIInvWindow *guii, int newhit)
RuntimeScriptValue Sc_InvWindow_SetItemHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemHeight);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemWidth);
}

// void (GUIInvWindow *guii, int newwidth)
RuntimeScriptValue Sc_InvWindow_SetItemWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetItemWidth);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetItemsPerRow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetItemsPerRow);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetRowCount);
}

// int (GUIInvWindow *guii)
RuntimeScriptValue Sc_InvWindow_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIInvWindow, InvWindow_GetTopItem);
}

// void (GUIInvWindow *guii, int topitem)
RuntimeScriptValue Sc_InvWindow_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIInvWindow, InvWindow_SetTopItem);
}


void RegisterInventoryWindowAPI()
{
    ScFnRegister invwindow_api[] = {
        {"InvWindow::ScrollDown^0", API_FN_PAIR(InvWindow_ScrollDown)},
        {"InvWindow::ScrollUp^0", API_FN_PAIR(InvWindow_ScrollUp)},
        {"InvWindow::get_CharacterToUse", API_FN_PAIR(InvWindow_GetCharacterToUse)},
        {"InvWindow::set_CharacterToUse", API_FN_PAIR(InvWindow_SetCharacterToUse)},
        {"InvWindow::geti_ItemAtIndex", API_FN_PAIR(InvWindow_GetItemAtIndex)},
        {"InvWindow::get_ItemCount", API_FN_PAIR(InvWindow_GetItemCount)},
        {"InvWindow::get_ItemHeight", API_FN_PAIR(InvWindow_GetItemHeight)},
        {"InvWindow::set_ItemHeight", API_FN_PAIR(InvWindow_SetItemHeight)},
        {"InvWindow::get_ItemWidth", API_FN_PAIR(InvWindow_GetItemWidth)},
        {"InvWindow::set_ItemWidth", API_FN_PAIR(InvWindow_SetItemWidth)},
        {"InvWindow::get_ItemsPerRow", API_FN_PAIR(InvWindow_GetItemsPerRow)},
        {"InvWindow::get_RowCount", API_FN_PAIR(InvWindow_GetRowCount)},
        {"InvWindow::get_TopItem", API_FN_PAIR(InvWindow_GetTopItem)},
        {"InvWindow::set_TopItem", API_FN_PAIR(InvWindow_SetTopItem)},
    };

    ccAddExternalFunctions(invwindow_api);
}
