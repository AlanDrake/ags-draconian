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
// Script bindings registered by RegisterHotspotAPI().
//
//=============================================================================
#include "game/dynobj/cc_hotspot.h"
#include "game/game.h"
#include "game/lib/string.h"
#include "game/script.h"
#include "game/world/hotspot.h"
#include "game/world/room.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Hotspot_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptHotspot, ccDynamicHotspot, Hotspot_GetAtRoomXY);
}

RuntimeScriptValue Sc_Hotspot_GetAtRoomXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptHotspot, ccDynamicHotspot, Hotspot_GetAtRoomXY2);
}

RuntimeScriptValue Sc_Hotspot_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptHotspot, ccDynamicHotspot, Hotspot_GetAtScreenXY);
}

RuntimeScriptValue Sc_Hotspot_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptHotspot, ccDynamicHotspot, Hotspot_GetAtScreenXY2);
}

RuntimeScriptValue Sc_Hotspot_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptHotspot, ccDynamicHotspot, Hotspot_GetByName, const char);
}

RuntimeScriptValue Sc_Hotspot_GetDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDrawingSurface, Hotspot_GetDrawingSurface);
}

// int  (ScriptHotspot *hss, const char *property)
RuntimeScriptValue Sc_Hotspot_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptHotspot, Hotspot_GetProperty, const char);
}

// void  (ScriptHotspot *hss, const char *property, char *bufer)
RuntimeScriptValue Sc_Hotspot_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(ScriptHotspot, Hotspot_GetPropertyText, const char, char);
}

// const char* (ScriptHotspot *hss, const char *property)
RuntimeScriptValue Sc_Hotspot_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptHotspot, const char, myScriptStringImpl, Hotspot_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Hotspot_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptHotspot, Hotspot_SetProperty, const char);
}

RuntimeScriptValue Sc_Hotspot_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptHotspot, Hotspot_SetTextProperty, const char, const char);
}

RuntimeScriptValue Sc_Hotspot_IsInteractionAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT(ScriptHotspot, Hotspot_IsInteractionAvailable);
}

// void  (ScriptHotspot *hss, int mood)
RuntimeScriptValue Sc_Hotspot_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptHotspot, Hotspot_RunInteraction);
}

// int (ScriptHotspot *hss)
RuntimeScriptValue Sc_Hotspot_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptHotspot, Hotspot_GetEnabled);
}

// void (ScriptHotspot *hss, int newval)
RuntimeScriptValue Sc_Hotspot_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptHotspot, Hotspot_SetEnabled);
}

// int (ScriptHotspot *hss)
RuntimeScriptValue Sc_Hotspot_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptHotspot, Hotspot_GetID);
}

RuntimeScriptValue Sc_Hotspot_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptHotspot, const char, myScriptStringImpl, Hotspot_GetScriptName);
}

// const char* (ScriptHotspot *hss)
RuntimeScriptValue Sc_Hotspot_GetName_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptHotspot, const char, myScriptStringImpl, Hotspot_GetName_New);
}

RuntimeScriptValue Sc_Hotspot_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptHotspot, Hotspot_SetName, const char);
}

// int (ScriptHotspot *hss)
RuntimeScriptValue Sc_Hotspot_GetWalkToX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptHotspot, Hotspot_GetWalkToX);
}

// int (ScriptHotspot *hss)
RuntimeScriptValue Sc_Hotspot_GetWalkToY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptHotspot, Hotspot_GetWalkToY);
}


void RegisterHotspotAPI()
{
    ScFnRegister hotspot_api[] = {
        {"Hotspot::GetAtRoomXY^2", API_FN_PAIR(Hotspot_GetAtRoomXY2)},
        {"Hotspot::GetAtScreenXY^2", API_FN_PAIR(Hotspot_GetAtScreenXY2)},
        {"Hotspot::GetAtRoomXY^3", API_FN_PAIR(Hotspot_GetAtRoomXY)},
        {"Hotspot::GetAtScreenXY^3", API_FN_PAIR(Hotspot_GetAtScreenXY)},
        // [OBSOLETE] GetByName => GetByScriptName
        {"Hotspot::GetByName", API_FN_PAIR(Hotspot_GetByName)},
        {"Hotspot::GetByScriptName", API_FN_PAIR(Hotspot_GetByName)},
        {"Hotspot::GetDrawingSurface", API_FN_PAIR(Hotspot_GetDrawingSurface)},

        {"Hotspot::GetProperty^1", API_FN_PAIR(Hotspot_GetProperty)},
        {"Hotspot::GetPropertyText^2", API_FN_PAIR(Hotspot_GetPropertyText)},
        {"Hotspot::GetTextProperty^1", API_FN_PAIR(Hotspot_GetTextProperty)},
        {"Hotspot::SetProperty^2", API_FN_PAIR(Hotspot_SetProperty)},
        {"Hotspot::SetTextProperty^2", API_FN_PAIR(Hotspot_SetTextProperty)},
        {"Hotspot::IsInteractionAvailable^1", API_FN_PAIR(Hotspot_IsInteractionAvailable)},
        {"Hotspot::RunInteraction^1", API_FN_PAIR(Hotspot_RunInteraction)},
        {"Hotspot::get_DisplayName", API_FN_PAIR(Hotspot_GetName_New)},
        {"Hotspot::set_DisplayName", API_FN_PAIR(Hotspot_SetName)},
        {"Hotspot::get_Enabled", API_FN_PAIR(Hotspot_GetEnabled)},
        {"Hotspot::set_Enabled", API_FN_PAIR(Hotspot_SetEnabled)},
        {"Hotspot::get_ID", API_FN_PAIR(Hotspot_GetID)},
        // [OBSOLETE] Name => DisplayName
        {"Hotspot::get_Name", API_FN_PAIR(Hotspot_GetName_New)},
        {"Hotspot::set_Name", API_FN_PAIR(Hotspot_SetName)},
        {"Hotspot::get_ScriptName", API_FN_PAIR(Hotspot_GetScriptName)},
        {"Hotspot::get_WalkToX", API_FN_PAIR(Hotspot_GetWalkToX)},
        {"Hotspot::get_WalkToY", API_FN_PAIR(Hotspot_GetWalkToY)},
    };

    ccAddExternalFunctions(hotspot_api);
}
