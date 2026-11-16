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
#include "game/world/inventory_item.h"

#include "debug/debug_log.h"
#include "game/character_def.h"
#include "game/dynobj/cc_inventory.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/runtime_defines.h"
#include "game/script.h"
#include "game/ui/gui.h"
#include "game/ui/inv_window.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/properties.h"
#include "gui/guidefines.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;


// TODO: ideally, it's the InvWindow gui controls that should be reacting
// to the inventory item looks update. For that we'd probably need to subscribe
// InvWindow for inventory item change, somehow (or character inventory change).
class InvItemDynamicSpriteListener : public ISpriteUser
{
public:
    void OnSpriteUpdate(int sprite_num) override
    {
        // NOTE: we currently don't distinguish whether the item is displayed anywhere or not
        AGS::Engine::GUIE::MarkInventoryForUpdate(-1, false);
    }
} gl_InvItemSpriteListener;

void InvItems_RegisterDynamicSpriteCallbacks()
{
    Game &game = Game::Current();
    for (int i = 0; i < game.numinvitems; ++i)
    {
        if (game.invinfo[i].pic > 0)
            add_sprite_changed_callback(i, &gl_InvItemSpriteListener);
    }
}


static void set_inv_item_pic(int item_id, int sprite)
{
    Game &game = Game::Current();
    if ((item_id < 0) || (item_id >= game.numinvitems))
        quit("!SetInvItemPic: invalid inventory item specified");

    if (game.invinfo[item_id].pic == sprite)
        return;

    const int old_pic = game.invinfo[item_id].pic;
    game.invinfo[item_id].pic = sprite;
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_pic, sprite, &gl_InvItemSpriteListener);

    GUIE::MarkInventoryForUpdate(-1, false);
}

void set_inv_item_cursorpic(int item_id, int sprite)
{
    Game &game = Game::Current();
    game.invinfo[item_id].cursorPic = sprite;

    if (is_current_cursor_look(kCursorRole_UseInv) && (game.PlayerChar->ActiveInventory == item_id))
    {
        update_inv_cursor(item_id);
        set_cursor_look(game.Cursor.Look);
    }
}

static void set_inv_item_cursorhotspot(int item_id, int hx, int hy, FrameAlignment align)
{
    Game &game = Game::Current();
    auto &item = game.invinfo[item_id];
    item.hotx = hx;
    item.hoty = hy;
    item.hotAlign = align;

    // The cursor image may include hotspot marker, therefore update cursor image
    if (is_current_cursor_look(kCursorRole_UseInv) && (game.PlayerChar->ActiveInventory == item_id))
    {
        update_inv_cursor(item_id);
        set_cursor_look(game.Cursor.Look);
    }
}

bool AssertInvItem(const char *apiname, int item_id)
{
    Game &game = Game::Current();
    if ((item_id >= 0) && (item_id < game.numinvitems))
        return true;
    debug_script_warn("%s: invalid inventory id %d (range is 0..%d)", apiname, item_id, game.numinvitems - 1);
    return false;
}

void InventoryItem_SetCursorGraphic(ScriptInvItem *item, int sprite)
{
    set_inv_item_cursorpic(item->id, sprite);
}

int InventoryItem_GetCursorGraphic(ScriptInvItem *item)
{
    return Game::Current().invinfo[item->id].cursorPic;
}

int InventoryItem_GetCursorHotspotAlignment(ScriptInvItem *item)
{
    return Game::Current().invinfo[item->id].hotAlign;
}

void InventoryItem_SetCursorHotspotAlignment(ScriptInvItem *item, int align)
{
    Game &game = Game::Current();
    set_inv_item_cursorhotspot(item->id, game.invinfo[item->id].hotx, game.invinfo[item->id].hoty,
                               static_cast<FrameAlignment>(align));
}

int InventoryItem_GetCursorHotspotX(ScriptInvItem *item)
{
    return Game::Current().invinfo[item->id].hotx;
}

void InventoryItem_SetCursorHotspotX(ScriptInvItem *item, int hotspot_x)
{
    Game &game = Game::Current();
    set_inv_item_cursorhotspot(item->id, hotspot_x, game.invinfo[item->id].hoty, game.invinfo[item->id].hotAlign);
}

int InventoryItem_GetCursorHotspotY(ScriptInvItem *item)
{
    return Game::Current().invinfo[item->id].hoty;
}

void InventoryItem_SetCursorHotspotY(ScriptInvItem *item, int hotspot_y)
{
    Game &game = Game::Current();
    set_inv_item_cursorhotspot(item->id, game.invinfo[item->id].hotx, hotspot_y, game.invinfo[item->id].hotAlign);
}

void InventoryItem_SetGraphic(ScriptInvItem *item, int sprite)
{
    set_inv_item_pic(item->id, sprite);
}

static void SetInvItemName(int item_id, const char *new_name)
{
    Game &game = Game::Current();
    if ((item_id < 0) || (item_id >= game.numinvitems))
        quit("!SetInvName: invalid inventory item specified");

    game.invinfo[item_id].name = new_name;
    // might need to redraw the GUI if it has the inv item name on it
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

void InventoryItem_SetName(ScriptInvItem *item, const char *new_name)
{
    SetInvItemName(item->id, new_name);
}

int InventoryItem_GetID(ScriptInvItem *item)
{
    return item->id;
}

const char *InventoryItem_GetScriptName(ScriptInvItem *item)
{
    return CreateNewScriptString(Game::Current().invScriptNames[item->id]);
}

int GetInvAt(int x, int y, int gui_hitoptions, int inv_hitoptions)
{
    Game &game = Game::Current();
    int gui_id = GetGUIAt(x, y, gui_hitoptions);
    if (gui_id >= 0)
    {
        int control_id = game.Guis[gui_id].FindControlAt(x, y, 0, inv_hitoptions & kHit_Interactable);
        GUIObject *control = game.Guis[gui_id].GetControl(control_id);
        if (control && (game.Guis[gui_id].GetControlType(control_id) == kGUIInvWindow))
        {
            const int inv_atx = x - game.Guis[gui_id].GetX() - control->GetX();
            const int inv_aty = y - game.Guis[gui_id].GetY() - control->GetY();
            return InvWindow_GetItemAtXY(static_cast<GUIInvWindow *>(control), inv_atx, inv_aty);
        }
    }
    return -1;
}

ScriptInvItem *InventoryItem_GetAtScreenXY(int x, int y, int gui_hitoptions, int inv_hitoptions)
{
    int item_id = GetInvAt(x, y, gui_hitoptions, inv_hitoptions);
    if (item_id <= 0)
        return nullptr;
    return &Game::Current().ScInvItems[item_id];
}

ScriptInvItem *InventoryItem_GetAtScreenXY2(int x, int y)
{
    return InventoryItem_GetAtScreenXY(x, y, kHit_Interactable, kHit_Interactable);
}

const char *InventoryItem_GetName_New(ScriptInvItem *item)
{
    return CreateNewScriptString(Game::Current().invinfo[item->id].name);
}

int InventoryItem_GetGraphic(ScriptInvItem *item)
{
    return Game::Current().invinfo[item->id].pic;
}

void RunInventoryInteraction(int item_id, int mood)
{
    Game &game = Game::Current();
    if ((item_id < 0) || (item_id >= game.numinvitems))
        quit("!RunInventoryInteraction: invalid inventory number");
    if (!AssertCursorValidForEvent("InventoryItem.RunInteraction", mood))
        return;

    // Cursor mode must match the event index in "Interactions" table
    const int evnt = (game.mcurs[mood].flags & MCF_EVENT) != 0 ? mood : -1;
    const int anyclick_evt = kInventoryEvent_AnyClick;

    // For USE verb: remember active inventory
    if (game.HasCursorRole(mood, kCursorRole_UseInv))
    {
        game.Play.usedinv = game.PlayerChar->ActiveInventory;
    }

    const auto obj_evt =
        ObjectEvent(kScTypeGame, LOCTYPE_INVITEM, item_id,
                    RuntimeScriptValue().SetScriptObject(&game.ScInvItems[item_id], &ccDynamicInv), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &game.invinfo[item_id].interactions, evnt, &game.invinfo[item_id].GetEvents(),
                         anyclick_evt, true /* do unhandled event */) < 0)
        return; // game state changed, don't do "any click"
    // any click on inv item
    run_event_script(obj_evt, &game.invinfo[item_id].GetEvents(), anyclick_evt);
}

static int IsInventoryInteractionAvailable(int item_id, int mood)
{
    Game &game = Game::Current();
    if ((item_id < 0) || (item_id >= game.numinvitems))
        quit("!IsInventoryInteractionAvailable: invalid inventory number");

    game.Play.check_interaction_only = 1;

    RunInventoryInteraction(item_id, mood);

    int result = game.Play.check_interaction_only;
    game.Play.check_interaction_only = 0;

    if (result == 2)
        return 1;

    return 0;
}

void InventoryItem_RunInteraction(ScriptInvItem *item, int mood)
{
    RunInventoryInteraction(item->id, mood);
}

int InventoryItem_CheckInteractionAvailable(ScriptInvItem *item, int mood)
{
    return IsInventoryInteractionAvailable(item->id, mood);
}

int InventoryItem_GetProperty(ScriptInvItem *item, const char *property)
{
    Game &game = Game::Current();
    return get_int_property(game.invProps[item->id], game.Play.invProps[item->id], property);
}

void InventoryItem_GetPropertyText(ScriptInvItem *item, const char *property, char *buffer)
{
    Game &game = Game::Current();
    get_text_property(game.invProps[item->id], game.Play.invProps[item->id], property, buffer);
}

const char *InventoryItem_GetTextProperty(ScriptInvItem *item, const char *property)
{
    Game &game = Game::Current();
    return get_text_property_dynamic_string(game.invProps[item->id], game.Play.invProps[item->id], property);
}

bool InventoryItem_SetProperty(ScriptInvItem *item, const char *property, int value)
{
    return set_int_property(Game::Current().Play.invProps[item->id], property, value);
}

bool InventoryItem_SetTextProperty(ScriptInvItem *item, const char *property, const char *value)
{
    return set_text_property(Game::Current().Play.invProps[item->id], property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"


ScriptInvItem *InventoryItem_GetByName(const char *name)
{
    return static_cast<ScriptInvItem *>(ccGetScriptObjectAddress(name, ccDynamicInv.GetType()));
}
