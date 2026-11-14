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

#include "ac/dynobj/scriptstring.h"
#include "game/ui/list_box.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// int (GUIListBox *lbb, const char *text)
RuntimeScriptValue Sc_ListBox_AddItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(GUIListBox, ListBox_AddItem, const char);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_Clear);
}

// void (GUIListBox *listbox, const char *filemask)
RuntimeScriptValue Sc_ListBox_FillDirList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIListBox, ListBox_FillDirList, const char);
}

RuntimeScriptValue Sc_ListBox_FillDirList3(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(GUIListBox, ListBox_FillDirList3, const char);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_FillSaveGameList);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(GUIListBox, ListBox_FillSaveGameList2);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameList4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT4(GUIListBox, ListBox_FillSaveGameList4);
}

RuntimeScriptValue Sc_ListBox_FillSaveGameSlots(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(GUIListBox, ListBox_FillSaveGameSlots, void);
}

// int (GUIListBox *listbox, int x, int y)
RuntimeScriptValue Sc_ListBox_GetItemAtLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(GUIListBox, ListBox_GetItemAtLocation);
}

// char *(GUIListBox *listbox, int index, char *buffer)
RuntimeScriptValue Sc_ListBox_GetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT_POBJ(GUIListBox, char, myScriptStringImpl, ListBox_GetItemText, char);
}

// int (GUIListBox *lbb, int index, const char *text)
RuntimeScriptValue Sc_ListBox_InsertItemAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT_POBJ(GUIListBox, ListBox_InsertItemAt, const char);
}

// void (GUIListBox *listbox, int itemIndex)
RuntimeScriptValue Sc_ListBox_RemoveItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_RemoveItem);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollDown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_ScrollDown);
}

// void (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_ScrollUp(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(GUIListBox, ListBox_ScrollUp);
}

// void (GUIListBox *listbox, int index, const char *newtext)
RuntimeScriptValue Sc_ListBox_SetItemText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_POBJ(GUIListBox, ListBox_SetItemText, const char);
}

RuntimeScriptValue Sc_ListBox_SortItems(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(GUIListBox, ListBox_SortItems);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetFont);
}

// void (GUIListBox *listbox, int newfont)
RuntimeScriptValue Sc_ListBox_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetFont);
}

RuntimeScriptValue Sc_ListBox_GetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIListBox, ListBox_GetShowBorder);
}

RuntimeScriptValue Sc_ListBox_SetShowBorder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIListBox, ListBox_SetShowBorder);
}

RuntimeScriptValue Sc_ListBox_GetShowScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIListBox, ListBox_GetShowScrollArrows);
}

RuntimeScriptValue Sc_ListBox_SetShowScrollArrows(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIListBox, ListBox_SetShowScrollArrows);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetItemCount);
}

// const char* (GUIListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetItems(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(GUIListBox, const char, myScriptStringImpl, ListBox_GetItems);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetRowCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetRowCount);
}

RuntimeScriptValue Sc_ListBox_GetRowHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetRowHeight);
}

// int (GUIListBox *listbox, int index)
RuntimeScriptValue Sc_ListBox_GetSaveGameSlots(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(GUIListBox, ListBox_GetSaveGameSlots);
}

RuntimeScriptValue Sc_ListBox_GetSelectedBackColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedBackColor);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedBackColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedBackColor);
}

RuntimeScriptValue Sc_ListBox_GetSelectedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedTextColor);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedTextColor);
}

RuntimeScriptValue Sc_ListBox_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTextAlignment);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTextAlignment);
}

RuntimeScriptValue Sc_ListBox_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTextColor);
}

RuntimeScriptValue Sc_ListBox_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTextColor);
}

RuntimeScriptValue Sc_ListBox_GetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTextOutlineColor);
}

RuntimeScriptValue Sc_ListBox_SetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTextOutlineColor);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetSelectedIndex);
}

// void (GUIListBox *guisl, int newsel)
RuntimeScriptValue Sc_ListBox_SetSelectedIndex(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetSelectedIndex);
}

// int (GUIListBox *listbox)
RuntimeScriptValue Sc_ListBox_GetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIListBox, ListBox_GetTopItem);
}

// void (GUIListBox *guisl, int item)
RuntimeScriptValue Sc_ListBox_SetTopItem(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIListBox, ListBox_SetTopItem);
}


void RegisterListBoxAPI()
{
    ScFnRegister listbox_api[] = {
        {"ListBox::AddItem^1", API_FN_PAIR(ListBox_AddItem)},
        {"ListBox::Clear^0", API_FN_PAIR(ListBox_Clear)},
        {"ListBox::FillDirList^1", API_FN_PAIR(ListBox_FillDirList)},
        {"ListBox::FillDirList^3", API_FN_PAIR(ListBox_FillDirList3)},
        {"ListBox::FillSaveGameList^0", API_FN_PAIR(ListBox_FillSaveGameList)},
        {"ListBox::FillSaveGameList^2", API_FN_PAIR(ListBox_FillSaveGameList2)},
        {"ListBox::FillSaveGameList^4", API_FN_PAIR(ListBox_FillSaveGameList4)},
        {"ListBox::FillSaveGameSlots^3", API_FN_PAIR(ListBox_FillSaveGameSlots)},
        {"ListBox::GetItemAtLocation^2", API_FN_PAIR(ListBox_GetItemAtLocation)},
        {"ListBox::GetItemText^2", API_FN_PAIR(ListBox_GetItemText)},
        {"ListBox::InsertItemAt^2", API_FN_PAIR(ListBox_InsertItemAt)},
        {"ListBox::RemoveItem^1", API_FN_PAIR(ListBox_RemoveItem)},
        {"ListBox::ScrollDown^0", API_FN_PAIR(ListBox_ScrollDown)},
        {"ListBox::ScrollUp^0", API_FN_PAIR(ListBox_ScrollUp)},
        {"ListBox::SetItemText^2", API_FN_PAIR(ListBox_SetItemText)},
        {"ListBox::SortItems^2", API_FN_PAIR(ListBox_SortItems)},
        {"ListBox::get_Font", API_FN_PAIR(ListBox_GetFont)},
        {"ListBox::set_Font", API_FN_PAIR(ListBox_SetFont)},
        {"ListBox::get_ShowBorder", API_FN_PAIR(ListBox_GetShowBorder)},
        {"ListBox::set_ShowBorder", API_FN_PAIR(ListBox_SetShowBorder)},
        {"ListBox::get_ShowScrollArrows", API_FN_PAIR(ListBox_GetShowScrollArrows)},
        {"ListBox::set_ShowScrollArrows", API_FN_PAIR(ListBox_SetShowScrollArrows)},
        {"ListBox::get_ItemCount", API_FN_PAIR(ListBox_GetItemCount)},
        {"ListBox::geti_Items", API_FN_PAIR(ListBox_GetItems)},
        {"ListBox::seti_Items", API_FN_PAIR(ListBox_SetItemText)},
        {"ListBox::get_RowCount", API_FN_PAIR(ListBox_GetRowCount)},
        {"ListBox::get_RowHeight", API_FN_PAIR(ListBox_GetRowHeight)},
        {"ListBox::geti_SaveGameSlots", API_FN_PAIR(ListBox_GetSaveGameSlots)},
        {"ListBox::get_SelectedBackColor", API_FN_PAIR(ListBox_GetSelectedBackColor)},
        {"ListBox::set_SelectedBackColor", API_FN_PAIR(ListBox_SetSelectedBackColor)},
        {"ListBox::get_SelectedIndex", API_FN_PAIR(ListBox_GetSelectedIndex)},
        {"ListBox::set_SelectedIndex", API_FN_PAIR(ListBox_SetSelectedIndex)},
        {"ListBox::get_SelectedTextColor", API_FN_PAIR(ListBox_GetSelectedTextColor)},
        {"ListBox::set_SelectedTextColor", API_FN_PAIR(ListBox_SetSelectedTextColor)},
        {"ListBox::get_TextAlignment", API_FN_PAIR(ListBox_GetTextAlignment)},
        {"ListBox::set_TextAlignment", API_FN_PAIR(ListBox_SetTextAlignment)},
        {"ListBox::get_TextColor", API_FN_PAIR(ListBox_GetTextColor)},
        {"ListBox::set_TextColor", API_FN_PAIR(ListBox_SetTextColor)},
        {"ListBox::get_TextOutlineColor", API_FN_PAIR(ListBox_GetTextOutlineColor)},
        {"ListBox::set_TextOutlineColor", API_FN_PAIR(ListBox_SetTextOutlineColor)},
        {"ListBox::get_TopItem", API_FN_PAIR(ListBox_GetTopItem)},
        {"ListBox::set_TopItem", API_FN_PAIR(ListBox_SetTopItem)},
    };

    ccAddExternalFunctions(listbox_api);
}
