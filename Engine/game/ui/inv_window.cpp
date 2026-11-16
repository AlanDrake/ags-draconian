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
#include "game/ui/inv_window.h"

#include "debug/debug_log.h"
#include "device/sys_events.h"
#include "game/character_def.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/ui/gui.h"
#include "game/ui/gui_dialog.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/character_extras.h"
#include "game/world/inventory_item.h"
#include "gfx/sprite_cache.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h"
#include "util/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// *** INV WINDOW FUNCTIONS

void InvWindow_SetCharacterToUse(GUIInvWindow *guii, Character *ch)
{
    if (ch == nullptr)
        guii->SetCharacterID(-1);
    else
        guii->SetCharacterID(ch->index_id);
    // reset to top of list
    guii->SetTopItem(0);
}

Character *InvWindow_GetCharacterToUse(GUIInvWindow *guii)
{
    if (guii->GetCharacterID() < 0)
        return nullptr;

    return &Game::Current().Characters[guii->GetCharacterID()];
}

void InvWindow_SetItemWidth(GUIInvWindow *guii, int new_width)
{
    guii->SetItemWidth(new_width);
    guii->OnResized();
}

int InvWindow_GetItemWidth(GUIInvWindow *guii)
{
    return guii->GetItemWidth();
}

void InvWindow_SetItemHeight(GUIInvWindow *guii, int new_height)
{
    guii->SetItemHeight(new_height);
    guii->OnResized();
}

int InvWindow_GetItemHeight(GUIInvWindow *guii)
{
    return guii->GetItemHeight();
}

void InvWindow_SetTopItem(GUIInvWindow *guii, int topitem)
{
    if (guii->GetTopItem() != topitem)
    {
        guii->SetTopItem(topitem);
    }
}

int InvWindow_GetTopItem(GUIInvWindow *guii)
{
    return guii->GetTopItem();
}

int InvWindow_GetItemsPerRow(GUIInvWindow *guii)
{
    return guii->GetColCount();
}

int InvWindow_GetItemCount(GUIInvWindow *guii)
{
    return Game::Current().Characters[guii->GetCharacterID()].inventory.size();
}

int InvWindow_GetRowCount(GUIInvWindow *guii)
{
    return guii->GetRowCount();
}

void InvWindow_ScrollDown(GUIInvWindow *guii)
{
    if ((Game::Current().Characters[guii->GetCharacterID()].inventory.size()) >
        (guii->GetTopItem() + (guii->GetColCount() * guii->GetRowCount())))
    {
        guii->SetTopItem(guii->GetTopItem() + guii->GetColCount());
    }
}

void InvWindow_ScrollUp(GUIInvWindow *guii)
{
    if (guii->GetTopItem() > 0)
    {
        guii->SetTopItem(std::max(0, guii->GetTopItem() - guii->GetColCount()));
    }
}

ScriptInvItem *InvWindow_GetItemAtIndex(GUIInvWindow *guii, int index)
{
    Game &game = Game::Current();
    if ((index < 0) || (index >= game.Characters[guii->GetCharacterID()].inventory.size()))
        return nullptr;
    return &game.ScInvItems[game.Characters[guii->GetCharacterID()].inventory[index]];
}

int InvWindow_GetItemAtXY(GUIInvWindow *inv, int at_x, int at_y)
{
    Game &game = Game::Current();
    int item_index = inv->GetItemIndexAt(at_x, at_y);
    if ((item_index < 0) || (item_index >= game.Characters[inv->GetCharacterID()].inventory.size()))
        return -1;

    return game.Characters[inv->GetCharacterID()].inventory[item_index];
}
