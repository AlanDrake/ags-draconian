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

#include "game/event.h"

#include "debug/debug_log.h"
#include "game/dynobj/cc_hotspot.h"
#include "game/dynobj/cc_region.h"
#include "game/dynobj/script_objects.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/render/screen.h"
#include "game/script.h"
#include "game/ui/gui.h"
#include "game/world/hotspot.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_state.h"
#include "gfx/bitmap.h"
#include "gfx/ddb.h"
#include "gfx/graphicsdriver.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "plugin/plugin_engine.h"
#include "script/cc_common.h"
#include "util/common.h"
#include "util/timer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// FIXME: refactor further to stop using Game::SavedViewportBitmap here, maybe move part of the code to
// screen.cpp?

int inside_processevent = 0;

const std::array<ScriptEventCallback, kTS_Num> ScriptEventCb = {{
    {nullptr, 0u, kScCallback_Direct},
    {REP_EXEC_NAME, 0u, kScCallback_CommonUnClaimable},
    {"on_key_press", 2u, kScCallback_CommonClaimable},
    {"on_mouse_click", 3u, kScCallback_CommonClaimable},
    {"on_text_input", 1u, kScCallback_CommonClaimable},
    {"on_event", 5u, kScCallback_CommonClaimable},
    {"on_pointer_down", 3u, kScCallback_CommonClaimable},
    {"on_pointer_move", 3u, kScCallback_CommonClaimable},
    {"on_pointer_up", 3u, kScCallback_CommonClaimable},
}};


void run_claimable_event(const String &tsname, bool includeRoom, int numParams, const RuntimeScriptValue *params,
                         bool *eventWasClaimed)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    *eventWasClaimed = true;
    // Run the room script function, and if it is not claimed,
    // then run the main one
    // We need to remember the Game::Events.Claimed's state, in case
    // this is a nested event
    int event_claimed_old_value = game.Events.Claimed;
    game.Events.Claimed = EVENT_INPROGRESS;

    if (includeRoom && room.Script)
    {
        RunScriptFunction(room.Script.get(), tsname, numParams, params);
        if (game.Events.Claimed == EVENT_CLAIMED)
        {
            game.Events.Claimed = event_claimed_old_value;
            return;
        }
    }

    // run script modules
    for (auto &module_script : game.Scripts.Modules)
    {
        RunScriptFunction(module_script.get(), tsname, numParams, params);
        if (game.Events.Claimed == EVENT_CLAIMED)
        {
            game.Events.Claimed = event_claimed_old_value;
            return;
        }
    }

    game.Events.Claimed = event_claimed_old_value;
    *eventWasClaimed = false;
}

// runs the global script on_event function
void run_on_event(AGSScriptEventType evtype, int data1, int data2, int data3, int data4)
{
    RuntimeScriptValue params[]{evtype, data1, data2, data3, data4};
    RunScriptFunctionAuto(kScTypeGame, "on_event", 5, params);
}

void run_room_event(int id)
{
    auto obj_evt = ObjectEvent(kScTypeRoom);
    run_event_script(obj_evt, &Room::Current().GetEvents(), id);
}

// event list functions
void setevent(const AGSEvent &evt)
{
    Game::Current().Events.Queue.push_back(evt);
}

void force_event(const AGSEvent &evt)
{
    if (inside_processevent)
        process_event(&evt);
    else
        setevent(evt);
}

void process_event(const AGSEvent *evp)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    const int room_was = game.Play.RoomChanges;

    RuntimeScriptValue rval_null;
    if (evp->Type == kAGSEvent_Script)
    {
        cc_clear_error();
        const auto &ts = evp->Data.Script;
        if (ts.CbType < kTS_None || ts.CbType >= kTS_Num)
        { // invalid type, internal error?
            quit("process_event: kAGSEvent_Script: unknown callback type");
            return;
        }
        RuntimeScriptValue params[3]{ts.Arg1, ts.Arg2, ts.Arg3};
        RunScriptFunctionAuto(kScTypeGame, ScriptEventCb[ts.CbType].FnName, ScriptEventCb[ts.CbType].ArgCount, params);
    }
    else if (evp->Type == kAGSEvent_NewRoom)
    {
        NewRoom(evp->Data.Newroom.RoomID);
    }
    else if (evp->Type == kAGSEvent_Object)
    {
        const auto &obj = evp->Data.Object;
        ScriptEventsBase *obj_events = nullptr;
        ObjectEvent obj_evt;

        switch (obj.ObjEvType)
        {
        case kObjEventType_Hotspot:
        {
            const int hotspot_id = obj.ObjID;
            obj_events = &room.Hotspots[hotspot_id].GetEvents();
            obj_evt =
                ObjectEvent(kScTypeRoom, LOCTYPE_HOTSPOT, hotspot_id,
                            RuntimeScriptValue().SetScriptObject(&game.ScHotspots[hotspot_id], &ccDynamicHotspot));
            //Debug::Printf("Running hotspot interaction for hotspot %d, event %d", evp->data2, evp->data3);
            break;
        }
        case kObjEventType_Region:
        {
            const int region_id = obj.ObjID;
            obj_events = &room.Regions[region_id].GetEvents();
            obj_evt = ObjectEvent(kScTypeRoom, LOCTYPE_NOTHING, region_id,
                                  RuntimeScriptValue().SetScriptObject(&game.ScRegions[region_id], &ccDynamicRegion));
            break;
        }
        case kObjEventType_Room:
        {
            obj_events = &room.GetEvents();
            obj_evt = ObjectEvent(kScTypeRoom);
            if (obj.ObjEvent == kRoomEvent_BeforeFadein)
            {
                game.Events.InEntersScreen = true;
                run_on_event(kScriptEvent_RoomEnter, game.DisplayedRoom);
            }
            else if (obj.ObjEvent == kRoomEvent_FirstEnter)
            {
                game.Events.InRoomTransition = false;
                GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
            }
            else if (obj.ObjEvent == kRoomEvent_AfterFadein)
            {
                game.Events.InRoomTransition = false;
                GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
                run_on_event(kScriptEvent_RoomAfterFadein, game.DisplayedRoom);
            }
            //Debug::Printf("Running room interaction, event %d", evp->data3);
            break;
        }
        default:
            // invalid type, internal error?
            quit("process_event: RunEvBlock: unknown evb type");
            break;
        }

        // If the room was changed inside a on_event call above,
        // then skip running further interaction scripts
        if (room_was != game.Play.RoomChanges)
        {
            if ((obj.ObjEvType == kObjEventType_Room) && (obj.ObjEvent == kRoomEvent_BeforeFadein))
                game.Events.InEntersScreen = false;
            return;
        }

        assert(obj_events);
        if (obj_events)
        {
            run_event_script(obj_evt, obj_events, obj.ObjEvent);
        }

        if ((obj.ObjEvType == kObjEventType_Room) && (obj.ObjEvent == kRoomEvent_BeforeFadein))
            game.Events.InEntersScreen = false;
    }
    else if (evp->Type == kAGSEvent_FadeIn)
    {
        current_fade_in_effect();
    }
    else if (evp->Type == kAGSEvent_GUI)
    {
        const auto &gui = evp->Data.Gui;
        process_interface_click(gui.GuiID, gui.GuiObjID, gui.Mbtn);
    }
    else
    {
        quit("process_event: unknown event to process");
    }
}

void processallevents()
{
    Game &game = Game::Current();
    if (inside_processevent)
    {
        game.Events.Queue.clear(); // flush queued events
        return;
    }

    // Make a copy of the events to process them safely.
    // WARNING: engine may actually add more events to Game::Events.Queue,
    // and they must NOT be processed here, but instead discarded at the end
    // of this function; otherwise game may glitch.
    // TODO: need to redesign engine events system?
    std::vector<AGSEvent> evtcopy = game.Events.Queue;

    const int room_was = game.Play.RoomChanges;

    inside_processevent++;

    for (size_t i = 0; i < evtcopy.size(); ++i)
    {
        process_event(&evtcopy[i]);

        if (room_was != game.Play.RoomChanges)
            break; // changed room, so discard other events
    }

    game.Events.Queue.clear();
    inside_processevent--;
}

// end event list functions


void ClaimEvent()
{
    Game &game = Game::Current();
    if (game.Events.Claimed == EVENT_NONE)
        quit("!ClaimEvent: no event to claim");

    game.Events.Claimed = EVENT_CLAIMED;
}

bool AssertCursorValidForEvent(const char *api_name, int mode)
{
    Game &game = Game::Current();
    const GameDef &game_def = game.Def();
    if (mode < 0 || mode >= game_def.MouseCursorCount)
    {
        debug_script_warn("%s: invalid mode number %d, valid range is 0..%d", api_name, mode,
                          game_def.MouseCursorCount - 1);
        return false;
    }
    return true;
}
