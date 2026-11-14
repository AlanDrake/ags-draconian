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
// Script bindings registered by RegisterLabelAPI().
//
//=============================================================================
#include "game/script_api/bindings.h"
#include "game/ui/label.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterLabelAPI()
{
    ScFnRegister label_api[] = {
        ScMethod<&Label_GetText>("Label::GetText^1"),
        ScMethod<&Label_SetText>("Label::SetText^1"),
        ScMethod<&Label_GetTextAlignment>("Label::get_TextAlignment"),
        ScMethod<&Label_SetTextAlignment>("Label::set_TextAlignment"),
        ScMethod<&Label_GetFont>("Label::get_Font"),
        ScMethod<&Label_SetFont>("Label::set_Font"),
        ScMethod<&Label_GetText_New>("Label::get_Text"),
        ScMethod<&Label_SetText>("Label::set_Text"),
        ScMethod<&Label_GetTextColor>("Label::get_TextColor"),
        ScMethod<&Label_SetTextColor>("Label::set_TextColor"),
        ScMethod<&Label_GetTextOutlineColor>("Label::get_TextOutlineColor"),
        ScMethod<&Label_SetTextOutlineColor>("Label::set_TextOutlineColor"),
    };

    ccAddExternalFunctions(label_api);
}
