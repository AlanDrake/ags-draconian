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
//
//
//=============================================================================
#pragma once

#include "gui/guislider.h"

using AGS::Common::GUISlider;

void Slider_SetMax(GUISlider *slider, int max_value);
int Slider_GetMax(GUISlider *slider);
void Slider_SetMin(GUISlider *slider, int min_value);
int Slider_GetMin(GUISlider *slider);
void Slider_SetValue(GUISlider *slider, int value);
int Slider_GetValue(GUISlider *slider);
int Slider_GetBackgroundGraphic(GUISlider *slider);
void Slider_SetBackgroundGraphic(GUISlider *slider, int new_image);
int Slider_GetHandleGraphic(GUISlider *slider);
void Slider_SetHandleGraphic(GUISlider *slider, int new_image);
int Slider_GetHandleOffset(GUISlider *slider);
void Slider_SetHandleOffset(GUISlider *slider, int new_offset);
