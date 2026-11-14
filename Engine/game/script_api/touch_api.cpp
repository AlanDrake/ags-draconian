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
// Script bindings registered by RegisterTouchAPI().
//
//=============================================================================
#include "game/dynobj/script_touch_pointer.h"
#include "game/input/touch.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

void RegisterTouchAPI()
{
    ScFnRegister touch_api[] = {
        ScStatic<&Touch_GetTouchPointerCount>("Touch::get_TouchPointerCount"),
        ScStatic<&Touch_GetTouchPointers>("Touch::geti_TouchPointers"),

        ScMethod<&TouchPointer_GetID>("TouchPointer::get_ID"),
        ScMethod<&TouchPointer_GetIsDown>("TouchPointer::get_IsDown"),
        ScMethod<&TouchPointer_GetX>("TouchPointer::get_X"),
        ScMethod<&TouchPointer_GetY>("TouchPointer::get_Y"),
    };

    ccAddExternalFunctions(touch_api);
}
