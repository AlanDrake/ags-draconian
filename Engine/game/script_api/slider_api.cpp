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
#include "game/script_api/bindings.h"
#include "game/ui/slider.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterSliderAPI()
{
    ScFnRegister slider_api[] = {
        ScMethod<&Slider_GetBackgroundGraphic>("Slider::get_BackgroundGraphic"),
        ScMethod<&Slider_SetBackgroundGraphic>("Slider::set_BackgroundGraphic"),
        ScMethod<&Slider_GetBorderShadeColor>("Slider::get_BorderShadeColor"),
        ScMethod<&Slider_SetBorderShadeColor>("Slider::set_BorderShadeColor"),
        ScMethod<&Slider_GetHandleColor>("Slider::get_HandleColor"),
        ScMethod<&Slider_SetHandleColor>("Slider::set_HandleColor"),
        ScMethod<&Slider_GetHandleGraphic>("Slider::get_HandleGraphic"),
        ScMethod<&Slider_SetHandleGraphic>("Slider::set_HandleGraphic"),
        ScMethod<&Slider_GetHandleOffset>("Slider::get_HandleOffset"),
        ScMethod<&Slider_SetHandleOffset>("Slider::set_HandleOffset"),
        ScMethod<&Slider_GetMax>("Slider::get_Max"),
        ScMethod<&Slider_SetMax>("Slider::set_Max"),
        ScMethod<&Slider_GetMin>("Slider::get_Min"),
        ScMethod<&Slider_SetMin>("Slider::set_Min"),
        ScMethod<&Slider_GetValue>("Slider::get_Value"),
        ScMethod<&Slider_SetValue>("Slider::set_Value"),
    };

    ccAddExternalFunctions(slider_api);
}
