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
#pragma once

#include "util/geometry.h"

enum class TouchPhase
{
    Undefined,
    Up,
    Motion,
    Down
};

// TouchInput defines the instant change in touch input (touch event)
struct TouchInput
{
    int PointerID = -1;
    TouchPhase Phase = TouchPhase::Undefined;
    Point Position;
};

void OnTouchPointerDown(int pointer_id, const Point &position);
void OnTouchPointerMotion(int pointer_id, const Point &position);
void OnTouchPointerUp(int pointer_id, const Point &position);
// Reset all the registered touch pointer states; use this when resetting input state
void ResetAllTouchPointers();

struct ScriptTouchPointer;

// Script API: Touch::get_TouchPointerCount
int Touch_GetTouchPointerCount();

// Script API: Touch::geti_TouchPointers
ScriptTouchPointer *Touch_GetTouchPointers(int pointer_id);

// Script API: TouchPointer::get_ID
int TouchPointer_GetID(ScriptTouchPointer *tp);

// Script API: TouchPointer::get_IsDown
bool TouchPointer_GetIsDown(ScriptTouchPointer *tp);

// Script API: TouchPointer::get_X
int TouchPointer_GetX(ScriptTouchPointer *tp);

// Script API: TouchPointer::get_Y
int TouchPointer_GetY(ScriptTouchPointer *tp);
