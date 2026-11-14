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
// Script bindings registered by RegisterTextBoxAPI().
//
//=============================================================================
#include "game/script_api/bindings.h"
#include "game/ui/text_box.h"
#include "script/script_runtime.h"

void RegisterTextBoxAPI()
{
    ScFnRegister textbox_api[] = {
        ScMethod<&TextBox_GetText>("TextBox::GetText^1"),
        ScMethod<&TextBox_SetText>("TextBox::SetText^1"),
        ScMethod<&TextBox_GetFont>("TextBox::get_Font"),
        ScMethod<&TextBox_SetFont>("TextBox::set_Font"),
        ScMethod<&TextBox_GetShowBorder>("TextBox::get_ShowBorder"),
        ScMethod<&TextBox_SetShowBorder>("TextBox::set_ShowBorder"),
        ScMethod<&TextBox_GetText_New>("TextBox::get_Text"),
        ScMethod<&TextBox_SetText>("TextBox::set_Text"),
        ScMethod<&TextBox_GetTextAlignment>("TextBox::get_TextAlignment"),
        ScMethod<&TextBox_SetTextAlignment>("TextBox::set_TextAlignment"),
        ScMethod<&TextBox_GetTextColor>("TextBox::get_TextColor"),
        ScMethod<&TextBox_SetTextColor>("TextBox::set_TextColor"),
        ScMethod<&TextBox_GetTextOutlineColor>("TextBox::get_TextOutlineColor"),
        ScMethod<&TextBox_SetTextOutlineColor>("TextBox::set_TextOutlineColor"),
    };

    ccAddExternalFunctions(textbox_api);
}
