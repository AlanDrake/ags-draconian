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
#include "ac/inventoryitem.h"

#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/dynobj/cc_inventory.h"
#include "ac/event.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/gui.h"
#include "ac/invwindow.h"
#include "ac/mouse.h"
#include "ac/properties.h"
#include "ac/runtime_defines.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "gui/guidefines.h"
#include "script/runtimescriptvalue.h"
#include "script/script.h"

using namespace AGS::Common;
using namespace AGS::Engine;


extern GameSetupStruct game;
extern std::vector<ScriptInvItem> scrInv;
extern int cur_cursor;
extern CharacterInfo *playerchar;
extern CCInventory ccDynamicInv;


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
    for (int i = 0; i < game.numinvitems; ++i)
    {
        if (game.invinfo[i].pic > 0)
            add_sprite_changed_callback(i, &gl_InvItemSpriteListener);
    }
}


static void set_inv_item_pic(int item_id, int sprite)
{
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
    game.invinfo[item_id].cursorPic = sprite;

    if (is_current_cursor_look(kCursorRole_UseInv) && (playerchar->activeinv == item_id))
    {
        update_inv_cursor(item_id);
        set_cursor_look(cur_cursor);
    }
}

static void set_inv_item_cursorhotspot(int item_id, int hx, int hy, FrameAlignment align)
{
    auto &item = game.invinfo[item_id];
    item.hotx = hx;
    item.hoty = hy;
    item.hotAlign = align;

    // The cursor image may include hotspot marker, therefore update cursor image
    if (is_current_cursor_look(kCursorRole_UseInv) && (playerchar->activeinv == item_id))
    {
        update_inv_cursor(item_id);
        set_cursor_look(cur_cursor);
    }
}

bool AssertInvItem(const char *apiname, int item_id)
{
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
    return game.invinfo[item->id].cursorPic;
}

int InventoryItem_GetCursorHotspotAlignment(ScriptInvItem *item)
{
    return game.invinfo[item->id].hotAlign;
}

void InventoryItem_SetCursorHotspotAlignment(ScriptInvItem *item, int align)
{
    set_inv_item_cursorhotspot(item->id, game.invinfo[item->id].hotx, game.invinfo[item->id].hoty,
                               static_cast<FrameAlignment>(align));
}

int InventoryItem_GetCursorHotspotX(ScriptInvItem *item)
{
    return game.invinfo[item->id].hotx;
}

void InventoryItem_SetCursorHotspotX(ScriptInvItem *item, int hotspot_x)
{
    set_inv_item_cursorhotspot(item->id, hotspot_x, game.invinfo[item->id].hoty, game.invinfo[item->id].hotAlign);
}

int InventoryItem_GetCursorHotspotY(ScriptInvItem *item)
{
    return game.invinfo[item->id].hoty;
}

void InventoryItem_SetCursorHotspotY(ScriptInvItem *item, int hotspot_y)
{
    set_inv_item_cursorhotspot(item->id, game.invinfo[item->id].hotx, hotspot_y, game.invinfo[item->id].hotAlign);
}

void InventoryItem_SetGraphic(ScriptInvItem *item, int sprite)
{
    set_inv_item_pic(item->id, sprite);
}

static void SetInvItemName(int item_id, const char *new_name)
{
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
    return CreateNewScriptString(game.invScriptNames[item->id]);
}

int GetInvAt(int x, int y, int gui_hitoptions, int inv_hitoptions)
{
    int gui_id = GetGUIAt(x, y, gui_hitoptions);
    if (gui_id >= 0)
    {
        int control_id = guis[gui_id].FindControlAt(x, y, 0, inv_hitoptions & kHit_Interactable);
        GUIObject *control = guis[gui_id].GetControl(control_id);
        if (control && (guis[gui_id].GetControlType(control_id) == kGUIInvWindow))
        {
            const int inv_atx = x - guis[gui_id].GetX() - control->GetX();
            const int inv_aty = y - guis[gui_id].GetY() - control->GetY();
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
    return &scrInv[item_id];
}

ScriptInvItem *InventoryItem_GetAtScreenXY2(int x, int y)
{
    return InventoryItem_GetAtScreenXY(x, y, kHit_Interactable, kHit_Interactable);
}

const char *InventoryItem_GetName_New(ScriptInvItem *item)
{
    return CreateNewScriptString(game.invinfo[item->id].name);
}

int InventoryItem_GetGraphic(ScriptInvItem *item)
{
    return game.invinfo[item->id].pic;
}

void RunInventoryInteraction(int item_id, int mood)
{
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
        play.usedinv = playerchar->activeinv;
    }

    const auto obj_evt = ObjectEvent(kScTypeGame, LOCTYPE_INVITEM, item_id,
                                     RuntimeScriptValue().SetScriptObject(&scrInv[item_id], &ccDynamicInv), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &game.invinfo[item_id].interactions, evnt, &game.invinfo[item_id].GetEvents(),
                         anyclick_evt, true /* do unhandled event */) < 0)
        return; // game state changed, don't do "any click"
    // any click on inv item
    run_event_script(obj_evt, &game.invinfo[item_id].GetEvents(), anyclick_evt);
}

static int IsInventoryInteractionAvailable(int item_id, int mood)
{
    if ((item_id < 0) || (item_id >= game.numinvitems))
        quit("!IsInventoryInteractionAvailable: invalid inventory number");

    play.check_interaction_only = 1;

    RunInventoryInteraction(item_id, mood);

    int result = play.check_interaction_only;
    play.check_interaction_only = 0;

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
    return get_int_property(game.invProps[item->id], play.invProps[item->id], property);
}

void InventoryItem_GetPropertyText(ScriptInvItem *item, const char *property, char *buffer)
{
    get_text_property(game.invProps[item->id], play.invProps[item->id], property, buffer);
}

const char *InventoryItem_GetTextProperty(ScriptInvItem *item, const char *property)
{
    return get_text_property_dynamic_string(game.invProps[item->id], play.invProps[item->id], property);
}

bool InventoryItem_SetProperty(ScriptInvItem *item, const char *property, int value)
{
    return set_int_property(play.invProps[item->id], property, value);
}

bool InventoryItem_SetTextProperty(ScriptInvItem *item, const char *property, const char *value)
{
    return set_text_property(play.invProps[item->id], property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/dynobj/scriptstring.h"
#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"


ScriptInvItem *InventoryItem_GetByName(const char *name)
{
    return static_cast<ScriptInvItem *>(ccGetScriptObjectAddress(name, ccDynamicInv.GetType()));
}

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
