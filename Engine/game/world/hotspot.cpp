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
#include "game/world/hotspot.h"

#include "debug/debug_log.h"
#include "game/dynobj/cc_hotspot.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/roomstruct.h"
#include "game/script.h"
#include "game/ui/gui.h"
#include "game/world/character.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "game/world/room_status.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"
#include "script/script_runtime.h"
#include "util/common.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void DisableHotspot(int hsnum)
{
    if ((hsnum < 1) | (hsnum >= MAX_ROOM_HOTSPOTS))
        quit("!DisableHotspot: invalid hotspot specified");
    croom->hotspot[hsnum].Enabled = false;
    debug_script_log("Hotspot %d disabled", hsnum);
}

void EnableHotspot(int hsnum)
{
    if ((hsnum < 1) | (hsnum >= MAX_ROOM_HOTSPOTS))
        quit("!EnableHotspot: invalid hotspot specified");
    croom->hotspot[hsnum].Enabled = true;
    debug_script_log("Hotspot %d re-enabled", hsnum);
}

void Hotspot_SetEnabled(ScriptHotspot *hss, int new_value)
{
    if (new_value)
        EnableHotspot(hss->id);
    else
        DisableHotspot(hss->id);
}

int Hotspot_GetEnabled(ScriptHotspot *hss)
{
    return croom->hotspot[hss->id].Enabled ? 1 : 0;
}

int Hotspot_GetID(ScriptHotspot *hss)
{
    return hss->id;
}

const char *Hotspot_GetScriptName(ScriptHotspot *hss)
{
    return CreateNewScriptString(thisroom.Hotspots[hss->id].ScriptName);
}

int GetHotspotPointX(int hotspot)
{
    if ((hotspot < 0) || (hotspot >= MAX_ROOM_HOTSPOTS))
        quit("!GetHotspotPointX: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkTo.X < 1)
        return -1;

    return thisroom.Hotspots[hotspot].WalkTo.X;
}

int GetHotspotPointY(int hotspot)
{
    if ((hotspot < 0) || (hotspot >= MAX_ROOM_HOTSPOTS))
        quit("!GetHotspotPointY: invalid hotspot");

    if (thisroom.Hotspots[hotspot].WalkTo.X < 1) // TODO: there was "x" here, why?
        return -1;

    return thisroom.Hotspots[hotspot].WalkTo.Y;
}

int Hotspot_GetWalkToX(ScriptHotspot *hss)
{
    return GetHotspotPointX(hss->id);
}

int Hotspot_GetWalkToY(ScriptHotspot *hss)
{
    return GetHotspotPointY(hss->id);
}

ScriptHotspot *Hotspot_GetAtRoomXY(int x, int y, int hit_options)
{
    return &scrHotspot[GetHotspotIDAtRoom(x, y, hit_options)];
}

ScriptHotspot *Hotspot_GetAtRoomXY2(int x, int y)
{
    return Hotspot_GetAtRoomXY(x, y, kHit_Interactable);
}

ScriptHotspot *Hotspot_GetAtScreenXY(int x, int y, int hit_options)
{
    VpPoint vpt = play.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return nullptr;
    return &scrHotspot[GetHotspotIDAtRoom(vpt.first.X, vpt.first.Y, hit_options)];
}

ScriptHotspot *Hotspot_GetAtScreenXY2(int x, int y)
{
    return Hotspot_GetAtScreenXY(x, y, kHit_Interactable);
}

ScriptHotspot *Hotspot_GetByName(const char *name)
{
    return static_cast<ScriptHotspot *>(ccGetScriptObjectAddress(name, ccDynamicHotspot.GetType()));
}

const char *Hotspot_GetName_New(ScriptHotspot *hss)
{
    if ((hss->id < 0) || (hss->id >= MAX_ROOM_HOTSPOTS))
        quit("!Hotspot.Name: invalid hotspot number");
    return CreateNewScriptString(croom->hotspot[hss->id].Name);
}

void Hotspot_SetName(ScriptHotspot *hss, const char *new_name)
{
    if ((hss->id < 0) || (hss->id >= MAX_ROOM_HOTSPOTS))
        quit("!Hotspot.Name: invalid hotspot number");
    croom->hotspot[hss->id].Name = new_name;
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

void RunHotspotInteraction(int hotspothere, int mood)
{
    if (!AssertCursorValidForEvent("Hotspot.RunInteraction", mood))
        return;

    // Cursor mode must match the event index in "Interactions" table
    const int evnt = (game.mcurs[mood].flags & MCF_EVENT) != 0 ? mood : -1;
    const int anyclick_evt = kHotspotEvent_AnyClick;

    // For USE verb: remember active inventory
    if (game.HasCursorRole(mood, kCursorRole_UseInv))
    {
        play.usedinv = playerchar->activeinv;
    }

    if ((game.options[OPT_WALKONLOOK] == 0) && (game.HasCursorRole(mood, kCursorRole_Look)))
        ;
    else if (play.auto_use_walkto_points == 0)
        ;
    else if ((!game.HasCursorRole(mood, kCursorRole_Walk)) && (play.check_interaction_only == 0))
        MoveCharacterToHotspot(game.playercharacter, hotspothere);

    const auto obj_evt =
        ObjectEvent(kScTypeRoom, LOCTYPE_HOTSPOT, hotspothere,
                    RuntimeScriptValue().SetScriptObject(&scrHotspot[hotspothere], &ccDynamicHotspot), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &thisroom.Hotspots[hotspothere].Interactions, evnt,
                         &thisroom.Hotspots[hotspothere].GetEvents(), anyclick_evt, true /* do unhandled event */) < 0)
        return; // game state changed, don't do "any click"
    // any click on hotspot
    run_event_script(obj_evt, &thisroom.Hotspots[hotspothere].GetEvents(), anyclick_evt);
}

bool Hotspot_IsInteractionAvailable(ScriptHotspot *hhot, int mood)
{
    play.check_interaction_only = 1;
    RunHotspotInteraction(hhot->id, mood);
    int ciwas = play.check_interaction_only;
    play.check_interaction_only = 0;
    return (ciwas == 2);
}

void Hotspot_RunInteraction(ScriptHotspot *hss, int mood)
{
    RunHotspotInteraction(hss->id, mood);
}

int Hotspot_GetProperty(ScriptHotspot *hss, const char *property)
{
    return get_int_property(thisroom.Hotspots[hss->id].Properties, croom->hsProps[hss->id], property);
}

void Hotspot_GetPropertyText(ScriptHotspot *hss, const char *property, char *bufer)
{
    get_text_property(thisroom.Hotspots[hss->id].Properties, croom->hsProps[hss->id], property, bufer);
}

const char *Hotspot_GetTextProperty(ScriptHotspot *hss, const char *property)
{
    return get_text_property_dynamic_string(thisroom.Hotspots[hss->id].Properties, croom->hsProps[hss->id], property);
}

bool Hotspot_SetProperty(ScriptHotspot *hss, const char *property, int value)
{
    return set_int_property(croom->hsProps[hss->id], property, value);
}

bool Hotspot_SetTextProperty(ScriptHotspot *hss, const char *property, const char *value)
{
    return set_text_property(croom->hsProps[hss->id], property, value);
}

int GetHotspotIDAtRoom(int xpp, int ypp, int hit_options)
{
    const bool only_clickable = (hit_options & kHit_Interactable) != 0;
    int onhs = thisroom.HotspotMask->GetPixel(room_to_mask_coord(xpp), room_to_mask_coord(ypp));
    if (onhs <= 0 || onhs >= MAX_ROOM_HOTSPOTS)
        return 0;
    if (only_clickable && !croom->hotspot[onhs].Enabled)
        return 0;
    return onhs;
}
