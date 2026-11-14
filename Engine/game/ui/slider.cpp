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
#include "game/ui/slider.h"

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
