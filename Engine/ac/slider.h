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
// Script API of the Slider GUI control: the implementations behind the
// properties of the Slider script type.
//
//=============================================================================
#pragma once

#include "gui/guislider.h"

using AGS::Common::GUISlider;

// Sets the upper limit of the slider's range; lowers the minimum too if it
// would exceed the new maximum, and clamps the current value into the range.
void Slider_SetMax(GUISlider *slider, int max_value);
// Returns the upper limit of the slider's range.
int Slider_GetMax(GUISlider *slider);
// Sets the lower limit of the slider's range; raises the maximum too if it
// would fall below the new minimum, and clamps the current value into the range.
void Slider_SetMin(GUISlider *slider, int min_value);
// Returns the lower limit of the slider's range.
int Slider_GetMin(GUISlider *slider);
// Sets the slider's value, clamped into its range.
void Slider_SetValue(GUISlider *slider, int value);
// Returns the slider's current value.
int Slider_GetValue(GUISlider *slider);
// Returns the sprite number of the slider's background, or 0 if it has none.
int Slider_GetBackgroundGraphic(GUISlider *slider);
// Sets the sprite of the slider's background (0 for none).
void Slider_SetBackgroundGraphic(GUISlider *slider, int new_image);
// Returns the sprite number of the slider's handle, or 0 if it has none.
int Slider_GetHandleGraphic(GUISlider *slider);
// Sets the sprite of the slider's handle (0 for none).
void Slider_SetHandleGraphic(GUISlider *slider, int new_image);
// Returns the offset of the handle in pixels from its centred position, across
// the slider (vertically for a horizontal one, horizontally for a vertical one).
int Slider_GetHandleOffset(GUISlider *slider);
// Sets the offset of the handle in pixels, see Slider_GetHandleOffset.
void Slider_SetHandleOffset(GUISlider *slider, int new_offset);
