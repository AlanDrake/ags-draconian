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
#include "game/character_info.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "game/ui/inv_window.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterInventoryWindowAPI()
{
    ScFnRegister invwindow_api[] = {
        ScMethod<&InvWindow_ScrollDown>("InvWindow::ScrollDown^0"),
        ScMethod<&InvWindow_ScrollUp>("InvWindow::ScrollUp^0"),
        ScMethod<&InvWindow_GetCharacterToUse>("InvWindow::get_CharacterToUse"),
        ScMethod<&InvWindow_SetCharacterToUse>("InvWindow::set_CharacterToUse"),
        ScMethod<&InvWindow_GetItemAtIndex>("InvWindow::geti_ItemAtIndex"),
        ScMethod<&InvWindow_GetItemCount>("InvWindow::get_ItemCount"),
        ScMethod<&InvWindow_GetItemHeight>("InvWindow::get_ItemHeight"),
        ScMethod<&InvWindow_SetItemHeight>("InvWindow::set_ItemHeight"),
        ScMethod<&InvWindow_GetItemWidth>("InvWindow::get_ItemWidth"),
        ScMethod<&InvWindow_SetItemWidth>("InvWindow::set_ItemWidth"),
        ScMethod<&InvWindow_GetItemsPerRow>("InvWindow::get_ItemsPerRow"),
        ScMethod<&InvWindow_GetRowCount>("InvWindow::get_RowCount"),
        ScMethod<&InvWindow_GetTopItem>("InvWindow::get_TopItem"),
        ScMethod<&InvWindow_SetTopItem>("InvWindow::set_TopItem"),
    };

    ccAddExternalFunctions(invwindow_api);
}
