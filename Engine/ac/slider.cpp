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
#include "ac/slider.h"

#include "ac/common.h"
#include "ac/draw.h"
#include "debug/debug_log.h"

using namespace AGS::Common;

void Slider_SetMax(GUISlider *slider, int max_value)
{
    slider->SetMaxValue(max_value);
}

int Slider_GetMax(GUISlider *slider)
{
    return slider->GetMaxValue();
}

void Slider_SetMin(GUISlider *slider, int min_value)
{
    slider->SetMinValue(min_value);
}

int Slider_GetMin(GUISlider *slider)
{
    return slider->GetMinValue();
}

void Slider_SetValue(GUISlider *slider, int value)
{
    slider->SetValue(value);
}

int Slider_GetValue(GUISlider *slider)
{
    return slider->GetValue();
}

int Slider_GetBackgroundGraphic(GUISlider *slider)
{
    return (slider->GetBgImage() > 0) ? slider->GetBgImage() : 0;
}

void Slider_SetBackgroundGraphic(GUISlider *slider, int new_image)
{
    const int old_image = slider->GetBgImage();
    slider->SetBgImage(new_image);
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_image, new_image, slider);
}

int Slider_GetBorderShadeColor(GUISlider *slider)
{
    return slider->GetBorderShadeColor();
}

void Slider_SetBorderShadeColor(GUISlider *slider, int color)
{
    slider->SetBorderShadeColor(color);
}

int Slider_GetHandleColor(GUISlider *slider)
{
    return slider->GetHandleColor();
}

void Slider_SetHandleColor(GUISlider *slider, int color)
{
    slider->SetHandleColor(color);
}

int Slider_GetHandleGraphic(GUISlider *slider)
{
    return (slider->GetHandleImage() > 0) ? slider->GetHandleImage() : 0;
}

void Slider_SetHandleGraphic(GUISlider *slider, int new_image)
{
    const int old_image = slider->GetHandleImage();
    slider->SetHandleImage(new_image);
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_image, new_image, slider);
}

int Slider_GetHandleOffset(GUISlider *slider)
{
    return slider->GetHandleOffset();
}

void Slider_SetHandleOffset(GUISlider *slider, int new_offset)
{
    slider->SetHandleOffset(new_offset);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetBackgroundGraphic);
}

// void (GUISlider *slider, int new_image)
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

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleGraphic);
}

// void (GUISlider *slider, int new_image)
RuntimeScriptValue Sc_Slider_SetHandleGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleGraphic);
}

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetHandleOffset);
}

// void (GUISlider *slider, int new_offset)
RuntimeScriptValue Sc_Slider_SetHandleOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetHandleOffset);
}

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMax);
}

// void (GUISlider *slider, int max_value)
RuntimeScriptValue Sc_Slider_SetMax(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMax);
}

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetMin);
}

// void (GUISlider *slider, int min_value)
RuntimeScriptValue Sc_Slider_SetMin(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUISlider, Slider_SetMin);
}

// int (GUISlider *slider)
RuntimeScriptValue Sc_Slider_GetValue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUISlider, Slider_GetValue);
}

// void Slider_SetValue(GUISlider *slider, int value)
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
