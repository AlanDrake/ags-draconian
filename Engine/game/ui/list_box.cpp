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
#include "game/ui/list_box.h"

#include <algorithm>
#include <vector>

#include <allegro.h> // find files

#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "game/file.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/ui/gui.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "util/common.h"
#include "util/directory.h"
#include "util/path.h"
#include "util/path_helper.h"

using namespace AGS::Common;

// *** LIST BOX FUNCTIONS

int ListBox_AddItem(GUIListBox *listbox, const char *text)
{
    listbox->AddItem(text);
    return 1;
}

int ListBox_InsertItemAt(GUIListBox *listbox, int index, const char *text)
{
    if (listbox->InsertItem(index, text) < 0)
        return 0;
    return 1;
}

void ListBox_Clear(GUIListBox *listbox)
{
    listbox->Clear();
}

void ListBox_FillDirList3(GUIListBox *listbox, const char *file_mask, int file_sort, int sort_dir)
{
    file_sort = ValidateFileSort("ListBox.FillDirList", file_sort);
    sort_dir = ValidateSortDirection("ListBox.FillDirList", sort_dir);

    std::vector<String> files;
    FillDirList(files, file_mask, (ScriptFileSortStyle)file_sort, (ScriptSortDirection)sort_dir);

    // TODO: method for adding item batch to speed up update
    listbox->Clear();
    for (auto it = files.cbegin(); it != files.cend(); ++it)
    {
        listbox->AddItem(*it);
    }
}

void ListBox_FillDirList(GUIListBox *listbox, const char *file_mask)
{
    ListBox_FillDirList3(listbox, file_mask, kScFileSort_Name, kScSortAscending);
}

int ListBox_GetSaveGameSlots(GUIListBox *listbox, int index)
{
    if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
        quit("!ListBox.SaveGameSlot: index out of range");

    return listbox->GetSavedGameIndex(index);
}

// Fills ListBox with save game slots, inserts descriptions as textual items,
// and fills SavedGameIndex array with corresponding slot numbers
static void ListBox_FillSaveItems(GUIListBox *listbox, const std::vector<SaveListItem> &saves)
{
    listbox->Clear();
    listbox->SetSvgIndex(true);
    // TODO: method for adding item batch to speed up update and auto-set SvgIndex flag
    for (const auto &item : saves)
    {
        listbox->AddItem(item.Description, item.Slot);
    }
}

int ListBox_FillSaveGameList4(GUIListBox *listbox, int min_slot, int max_slot, int save_sort, int sort_dir)
{
    EngineServices &services = EngineServices::Current();
    // Optionally override the max slot
    max_slot = services.Config.Override.MaxSaveSlot > 0 ? services.Config.Override.MaxSaveSlot : max_slot;

    if (!ValidateSaveSlotRange("ListBox.FillSaveGameList", min_slot, max_slot))
    {
        listbox->Clear();
        return 0;
    }

    save_sort = ValidateSaveGameSort("ListBox.FillSaveGameList", save_sort);
    sort_dir = ValidateSortDirection("ListBox.FillSaveGameList", sort_dir);

    std::vector<SaveListItem> saves;
    FillSaveList(saves, min_slot, max_slot, true, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);

    // Fill in the list box
    ListBox_FillSaveItems(listbox, saves);

    // Returns TRUE if the whole range of slots is occupied
    return saves.size() > static_cast<uint32_t>(max_slot - min_slot);
}

int ListBox_FillSaveGameList(GUIListBox *listbox)
{
    return ListBox_FillSaveGameList4(listbox, 0, LEGACY_TOP_LISTEDSAVESLOT, kScFileSort_Time, kScSortDescending);
}

int ListBox_FillSaveGameList2(GUIListBox *listbox, int min_slot, int max_slot)
{
    return ListBox_FillSaveGameList4(listbox, min_slot, max_slot, kScFileSort_Time, kScSortDescending);
}

void ListBox_FillSaveGameSlots(GUIListBox *listbox, void *src_arr, int save_sort, int sort_dir)
{
    const auto &hdr = CCDynamicArray::GetHeader(src_arr);
    if (hdr.GetElemCount() == 0u)
    {
        debug_script_warn("ListBox.FillSaveGameSlots: empty array provided, skip execution");
        return;
    }

    std::vector<int> slots;
    const int *slots_arr = static_cast<const int *>(src_arr);
    slots.insert(slots.end(), slots_arr, slots_arr + hdr.GetElemCount());

    std::vector<SaveListItem> saves;
    FillSaveList(slots, saves, true, (ScriptSaveGameSortStyle)save_sort, (ScriptSortDirection)sort_dir);
    ListBox_FillSaveItems(listbox, saves);
}

int ListBox_GetItemAtLocation(GUIListBox *listbox, int x, int y)
{
    Game &game = Game::Current();
    if (!game.Guis[listbox->GetParentID()].IsDisplayed())
        return -1;

    x = (x - listbox->GetX()) - game.Guis[listbox->GetParentID()].GetX();
    y = (y - listbox->GetY()) - game.Guis[listbox->GetParentID()].GetY();

    if ((x < 0) || (y < 0) || (x >= listbox->GetWidth()) || (y >= listbox->GetHeight()))
        return -1;

    return listbox->GetItemAt(x, y);
}

char *ListBox_GetItemText(GUIListBox *listbox, int index, char *buffer)
{
    if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
        quit("!ListBoxGetItemText: invalid item specified");
    snprintf(buffer, MAX_MAXSTRLEN, "%s", listbox->GetItem(index).GetCStr());
    return buffer;
}

const char *ListBox_GetItems(GUIListBox *listbox, int index)
{
    if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
        quit("!ListBox.Items: invalid index specified");

    return CreateNewScriptString(listbox->GetItem(index));
}

void ListBox_SetItemText(GUIListBox *listbox, int index, const char *new_text)
{
    if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
        quit("!ListBoxSetItemText: invalid item specified");

    if (listbox->GetItem(index) != new_text)
    {
        listbox->SetItemText(index, new_text);
    }
}

void ListBox_SortItems(GUIListBox *listbox, int compare_style, int sort_dir)
{
    compare_style = ValidateStringComparison("ListBox.SortItems", compare_style);
    sort_dir = ValidateSortDirection("ListBox.SortItems", sort_dir);
    if (sort_dir != kScSortNone)
    {
        listbox->SortItems((compare_style & kScCaseSensitiveFlag) == 0, (compare_style & kScLocaleAwareFlag) != 0,
                           (sort_dir != kScSortDescending));
    }
}

void ListBox_RemoveItem(GUIListBox *listbox, int index)
{
    if ((index < 0) || (static_cast<uint32_t>(index) >= listbox->GetItemCount()))
        quit("!ListBoxRemove: invalid listindex specified");

    listbox->RemoveItem(index);
}

int ListBox_GetItemCount(GUIListBox *listbox)
{
    return listbox->GetItemCount();
}

int ListBox_GetFont(GUIListBox *listbox)
{
    return listbox->GetFont();
}

void ListBox_SetFont(GUIListBox *listbox, int new_font)
{
    new_font = ValidateFontNumber("ListBox.Font", new_font);
    listbox->SetFont(new_font);
}

bool ListBox_GetShowBorder(GUIListBox *listbox)
{
    return listbox->IsShowBorder();
}

void ListBox_SetShowBorder(GUIListBox *listbox, bool new_value)
{
    if (listbox->IsShowBorder() != new_value)
    {
        listbox->SetShowBorder(new_value);
    }
}

bool ListBox_GetShowScrollArrows(GUIListBox *listbox)
{
    return listbox->AreArrowsShown();
}

void ListBox_SetShowScrollArrows(GUIListBox *listbox, bool new_value)
{
    if (listbox->AreArrowsShown() != new_value)
    {
        listbox->SetShowArrows(new_value);
    }
}

int ListBox_GetSelectedBackColor(GUIListBox *listbox)
{
    return listbox->GetSelectedBgColor();
}

void ListBox_SetSelectedBackColor(GUIListBox *listbox, int color)
{
    listbox->SetSelectedBgColor(color);
}

int ListBox_GetSelectedTextColor(GUIListBox *listbox)
{
    return listbox->GetSelectedTextColor();
}

void ListBox_SetSelectedTextColor(GUIListBox *listbox, int color)
{
    listbox->SetSelectedTextColor(color);
}

int ListBox_GetTextAlignment(GUIListBox *listbox)
{
    return listbox->GetTextAlignment();
}

void ListBox_SetTextAlignment(GUIListBox *listbox, int align)
{
    listbox->SetTextAlignment(static_cast<FrameAlignment>(align));
}

int ListBox_GetTextColor(GUIListBox *listbox)
{
    return listbox->GetTextColor();
}

void ListBox_SetTextColor(GUIListBox *listbox, int color)
{
    listbox->SetTextColor(color);
}

int ListBox_GetTextOutlineColor(GUIListBox *listbox)
{
    return listbox->GetTextOutlineColor();
}

void ListBox_SetTextOutlineColor(GUIListBox *listbox, int color)
{
    listbox->SetTextOutlineColor(color);
}

int ListBox_GetSelectedIndex(GUIListBox *listbox)
{
    return listbox->GetSelectedItem();
}

void ListBox_SetSelectedIndex(GUIListBox *listbox, int new_index)
{
    listbox->SetSelectedItem(new_index);
}

int ListBox_GetTopItem(GUIListBox *listbox)
{
    return listbox->GetTopItem();
}

void ListBox_SetTopItem(GUIListBox *listbox, int item)
{
    if ((item >= listbox->GetItemCount()) || (item < 0))
    {
        item = Math::Clamp<uint32_t>(item, 0u, listbox->GetItemCount());
        debug_script_warn("ListBoxSetTopItem: tried to set top to beyond top or bottom of list");
    }
    listbox->SetTopItem(item);
}

int ListBox_GetRowCount(GUIListBox *listbox)
{
    return listbox->GetVisibleItemCount();
}

int ListBox_GetRowHeight(GUIListBox *listbox)
{
    return listbox->GetRowHeight();
}

void ListBox_ScrollDown(GUIListBox *listbox)
{
    if (listbox->GetTopItem() + listbox->GetVisibleItemCount() < listbox->GetItemCount())
    {
        listbox->SetTopItem(listbox->GetTopItem() + 1);
    }
}

void ListBox_ScrollUp(GUIListBox *listbox)
{
    if (listbox->GetTopItem() > 0)
    {
        listbox->SetTopItem(listbox->GetTopItem() - 1);
    }
}


#include "game/dynobj/script_string.h"
