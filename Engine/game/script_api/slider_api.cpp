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
// Script bindings registered by RegisterSliderAPI().
//
//=============================================================================
#include "game/ui/slider.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_Slider_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetBackgroundGraphic);
}

RuntimeScriptValue Sc_Slider_SetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetBackgroundGraphic);
}

RuntimeScriptValue Sc_Slider_GetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetBorderShadeColor);
}

RuntimeScriptValue Sc_Slider_SetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetBorderShadeColor);
}

RuntimeScriptValue Sc_Slider_GetHandleColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleColor);
}

RuntimeScriptValue Sc_Slider_SetHandleColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleColor);
}

RuntimeScriptValue Sc_Slider_GetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleGraphic);
}

RuntimeScriptValue Sc_Slider_SetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleGraphic);
}

RuntimeScriptValue Sc_Slider_GetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleOffset);
}

RuntimeScriptValue Sc_Slider_SetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleOffset);
}

RuntimeScriptValue Sc_Slider_GetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMax);
}

RuntimeScriptValue Sc_Slider_SetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMax);
}

RuntimeScriptValue Sc_Slider_GetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMin);
}

RuntimeScriptValue Sc_Slider_SetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMin);
}

RuntimeScriptValue Sc_Slider_GetValue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetValue);
}

RuntimeScriptValue Sc_Slider_SetValue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetValue);
}


void RegisterSliderAPI()
{
    ScFnRegister slider_api[] = {
        {"Slider::get_BackgroundGraphic", API_FN_PAIR(Slider_GetBackgroundGraphic)},
        {"Slider::set_BackgroundGraphic", API_FN_PAIR(Slider_SetBackgroundGraphic)},
        {"Slider::get_BorderShadeColor", API_FN_PAIR(Slider_GetBorderShadeColor)},
        {"Slider::set_BorderShadeColor", API_FN_PAIR(Slider_SetBorderShadeColor)},
        {"Slider::get_HandleColor", API_FN_PAIR(Slider_GetHandleColor)},
        {"Slider::set_HandleColor", API_FN_PAIR(Slider_SetHandleColor)},
        {"Slider::get_HandleGraphic", API_FN_PAIR(Slider_GetHandleGraphic)},
        {"Slider::set_HandleGraphic", API_FN_PAIR(Slider_SetHandleGraphic)},
        {"Slider::get_HandleOffset", API_FN_PAIR(Slider_GetHandleOffset)},
        {"Slider::set_HandleOffset", API_FN_PAIR(Slider_SetHandleOffset)},
        {"Slider::get_Max", API_FN_PAIR(Slider_GetMax)},
        {"Slider::set_Max", API_FN_PAIR(Slider_SetMax)},
        {"Slider::get_Min", API_FN_PAIR(Slider_GetMin)},
        {"Slider::set_Min", API_FN_PAIR(Slider_SetMin)},
        {"Slider::get_Value", API_FN_PAIR(Slider_GetValue)},
        {"Slider::set_Value", API_FN_PAIR(Slider_SetValue)},
    };

    ccAddExternalFunctions(slider_api);
}
