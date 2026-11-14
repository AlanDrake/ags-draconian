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
// Script bindings registered by RegisterListBoxAPI().
//
//=============================================================================
#include <allegro.h>

#include "game/script_api/bindings.h"
#include "game/ui/list_box.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterListBoxAPI()
{
    ScFnRegister listbox_api[] = {
        ScMethod<&ListBox_AddItem>("ListBox::AddItem^1"),
        ScMethod<&ListBox_Clear>("ListBox::Clear^0"),
        ScMethod<&ListBox_FillDirList>("ListBox::FillDirList^1"),
        ScMethod<&ListBox_FillDirList3>("ListBox::FillDirList^3"),
        ScMethod<&ListBox_FillSaveGameList>("ListBox::FillSaveGameList^0"),
        ScMethod<&ListBox_FillSaveGameList2>("ListBox::FillSaveGameList^2"),
        ScMethod<&ListBox_FillSaveGameList4>("ListBox::FillSaveGameList^4"),
        ScMethod<&ListBox_FillSaveGameSlots>("ListBox::FillSaveGameSlots^3"),
        ScMethod<&ListBox_GetItemAtLocation>("ListBox::GetItemAtLocation^2"),
        ScMethod<&ListBox_GetItemText>("ListBox::GetItemText^2"),
        ScMethod<&ListBox_InsertItemAt>("ListBox::InsertItemAt^2"),
        ScMethod<&ListBox_RemoveItem>("ListBox::RemoveItem^1"),
        ScMethod<&ListBox_ScrollDown>("ListBox::ScrollDown^0"),
        ScMethod<&ListBox_ScrollUp>("ListBox::ScrollUp^0"),
        ScMethod<&ListBox_SetItemText>("ListBox::SetItemText^2"),
        ScMethod<&ListBox_SortItems>("ListBox::SortItems^2"),
        ScMethod<&ListBox_GetFont>("ListBox::get_Font"),
        ScMethod<&ListBox_SetFont>("ListBox::set_Font"),
        ScMethod<&ListBox_GetShowBorder>("ListBox::get_ShowBorder"),
        ScMethod<&ListBox_SetShowBorder>("ListBox::set_ShowBorder"),
        ScMethod<&ListBox_GetShowScrollArrows>("ListBox::get_ShowScrollArrows"),
        ScMethod<&ListBox_SetShowScrollArrows>("ListBox::set_ShowScrollArrows"),
        ScMethod<&ListBox_GetItemCount>("ListBox::get_ItemCount"),
        ScMethod<&ListBox_GetItems>("ListBox::geti_Items"),
        ScMethod<&ListBox_SetItemText>("ListBox::seti_Items"),
        ScMethod<&ListBox_GetRowCount>("ListBox::get_RowCount"),
        ScMethod<&ListBox_GetRowHeight>("ListBox::get_RowHeight"),
        ScMethod<&ListBox_GetSaveGameSlots>("ListBox::geti_SaveGameSlots"),
        ScMethod<&ListBox_GetSelectedBackColor>("ListBox::get_SelectedBackColor"),
        ScMethod<&ListBox_SetSelectedBackColor>("ListBox::set_SelectedBackColor"),
        ScMethod<&ListBox_GetSelectedIndex>("ListBox::get_SelectedIndex"),
        ScMethod<&ListBox_SetSelectedIndex>("ListBox::set_SelectedIndex"),
        ScMethod<&ListBox_GetSelectedTextColor>("ListBox::get_SelectedTextColor"),
        ScMethod<&ListBox_SetSelectedTextColor>("ListBox::set_SelectedTextColor"),
        ScMethod<&ListBox_GetTextAlignment>("ListBox::get_TextAlignment"),
        ScMethod<&ListBox_SetTextAlignment>("ListBox::set_TextAlignment"),
        ScMethod<&ListBox_GetTextColor>("ListBox::get_TextColor"),
        ScMethod<&ListBox_SetTextColor>("ListBox::set_TextColor"),
        ScMethod<&ListBox_GetTextOutlineColor>("ListBox::get_TextOutlineColor"),
        ScMethod<&ListBox_SetTextOutlineColor>("ListBox::set_TextOutlineColor"),
        ScMethod<&ListBox_GetTopItem>("ListBox::get_TopItem"),
        ScMethod<&ListBox_SetTopItem>("ListBox::set_TopItem"),
    };

    ccAddExternalFunctions(listbox_api);
}
