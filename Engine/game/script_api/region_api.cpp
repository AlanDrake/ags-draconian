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
// Script bindings registered by RegisterRegionAPI().
//
//=============================================================================
#include "ac/dynobj/cc_region.h"
#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "ac/region.h"
#include "ac/room.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_Region_GetAtRoomXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, Region_GetAtRoomXY2);
}

RuntimeScriptValue Sc_Region_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptRegion, ccDynamicRegion, Region_GetAtRoomXY);
}

RuntimeScriptValue Sc_Region_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptRegion, ccDynamicRegion, Region_GetAtScreenXY2);
}

RuntimeScriptValue Sc_Region_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptRegion, ccDynamicRegion, Region_GetAtScreenXY);
}

RuntimeScriptValue Sc_Region_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptRegion, ccDynamicRegion, Region_GetByName, const char);
}

RuntimeScriptValue Sc_Region_GetDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDrawingSurface, Region_GetDrawingSurface);
}

RuntimeScriptValue Sc_Region_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptRegion, Region_Tint);
}

// void (ScriptRegion *srr, int red, int green, int blue, int amount)
RuntimeScriptValue Sc_Region_TintNoLum(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptRegion, Region_TintNoLum);
}

// void (ScriptRegion *ssr, int mood)
RuntimeScriptValue Sc_Region_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_RunInteraction);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetEnabled);
}

// void (ScriptRegion *ssr, int enable)
RuntimeScriptValue Sc_Region_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_SetEnabled);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetID);
}

// int (ScriptRegion *ssr)
RuntimeScriptValue Sc_Region_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetLightLevel);
}

// void (ScriptRegion *ssr, int brightness)
RuntimeScriptValue Sc_Region_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRegion, Region_SetLightLevel);
}

RuntimeScriptValue Sc_Region_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRegion, const char, myScriptStringImpl, Region_GetScriptName);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintEnabled);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintBlue);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintGreen);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintRed);
}

// int (ScriptRegion *srr)
RuntimeScriptValue Sc_Region_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintSaturation);
}

RuntimeScriptValue Sc_Region_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRegion, Region_GetTintLuminance);
}

RuntimeScriptValue Sc_Region_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptRegion, Region_GetProperty, const char);
}

RuntimeScriptValue Sc_Region_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptRegion, const char, myScriptStringImpl, Region_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Region_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptRegion, Region_SetProperty, const char);
}

RuntimeScriptValue Sc_Region_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptRegion, Region_SetTextProperty, const char, const char);
}


void RegisterRegionAPI()
{
    ScFnRegister region_api[] = {
        {"Region::GetAtRoomXY^2", API_FN_PAIR(Region_GetAtRoomXY2)},
        {"Region::GetAtRoomXY^3", API_FN_PAIR(Region_GetAtRoomXY)},
        {"Region::GetAtScreenXY^2", API_FN_PAIR(Region_GetAtScreenXY2)},
        {"Region::GetAtScreenXY^3", API_FN_PAIR(Region_GetAtScreenXY)},
        {"Region::GetByScriptName^1", API_FN_PAIR(Region_GetByName)},
        {"Region::GetDrawingSurface", API_FN_PAIR(Region_GetDrawingSurface)},

        {"Region::Tint^4", API_FN_PAIR(Region_TintNoLum)},
        {"Region::Tint^5", API_FN_PAIR(Region_Tint)},
        {"Region::RunInteraction^1", API_FN_PAIR(Region_RunInteraction)},
        {"Region::GetProperty^1", API_FN_PAIR(Region_GetProperty)},
        {"Region::GetTextProperty^1", API_FN_PAIR(Region_GetTextProperty)},
        {"Region::SetProperty^2", API_FN_PAIR(Region_SetProperty)},
        {"Region::SetTextProperty^2", API_FN_PAIR(Region_SetTextProperty)},
        {"Region::get_Enabled", API_FN_PAIR(Region_GetEnabled)},
        {"Region::set_Enabled", API_FN_PAIR(Region_SetEnabled)},
        {"Region::get_ID", API_FN_PAIR(Region_GetID)},
        {"Region::get_LightLevel", API_FN_PAIR(Region_GetLightLevel)},
        {"Region::set_LightLevel", API_FN_PAIR(Region_SetLightLevel)},
        {"Region::get_ScriptName", API_FN_PAIR(Region_GetScriptName)},
        {"Region::get_TintEnabled", API_FN_PAIR(Region_GetTintEnabled)},
        {"Region::get_TintBlue", API_FN_PAIR(Region_GetTintBlue)},
        {"Region::get_TintGreen", API_FN_PAIR(Region_GetTintGreen)},
        {"Region::get_TintRed", API_FN_PAIR(Region_GetTintRed)},
        {"Region::get_TintSaturation", API_FN_PAIR(Region_GetTintSaturation)},
        {"Region::get_TintLuminance", API_FN_PAIR(Region_GetTintLuminance)},
    };

    ccAddExternalFunctions(region_api);
}
