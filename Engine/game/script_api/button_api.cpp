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
// Script bindings registered by RegisterButtonAPI().
//
//=============================================================================
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/ui/button.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterButtonAPI()
{
    ScFnRegister button_api[] = {
        ScMethod<&Button_Animate4>("Button::Animate^4"),
        ScMethod<&Button_Animate7>("Button::Animate^7"),
        ScMethod<&Button_Animate>("Button::Animate^8"),
        ScMethod<&Button_Click>("Button::Click^1"),
        ScMethod<&Button_RunFrameEvent, ScVoidResult>("Button::RunFrameEvent^3"),
        ScMethod<&Button_GetText>("Button::GetText^1"),
        ScMethod<&Button_SetText>("Button::SetText^1"),
        ScMethod<&Button_GetTextAlignment>("Button::get_TextAlignment"),
        ScMethod<&Button_SetTextAlignment>("Button::set_TextAlignment"),
        ScMethod<&Button_GetTextPaddingHorizontal>("Button::get_TextPaddingHorizontal"),
        ScMethod<&Button_SetTextPaddingHorizontal>("Button::set_TextPaddingHorizontal"),
        ScMethod<&Button_GetTextPaddingVertical>("Button::get_TextPaddingVertical"),
        ScMethod<&Button_SetTextPaddingVertical>("Button::set_TextPaddingVertical"),
        ScMethod<&Button_GetWrapText>("Button::get_WrapText"),
        ScMethod<&Button_SetWrapText>("Button::set_WrapText"),
        ScMethod<&Button_IsAnimating>("Button::get_Animating"),
        ScMethod<&Button_GetColorStyle>("Button::get_ColorStyle"),
        ScMethod<&Button_SetColorStyle>("Button::set_ColorStyle"),
        ScMethod<&Button_GetClipImage>("Button::get_ClipImage"),
        ScMethod<&Button_SetClipImage>("Button::set_ClipImage"),
        ScMethod<&Button_GetFont>("Button::get_Font"),
        ScMethod<&Button_SetFont>("Button::set_Font"),
        ScMethod<&Button_GetAnimFrame>("Button::get_Frame"),
        ScMethod<&Button_GetGraphic>("Button::get_Graphic"),
        ScMethod<&Button_GetGraphicFlip>("Button::get_GraphicFlip"),
        ScMethod<&Button_SetGraphicFlip>("Button::set_GraphicFlip"),
        ScMethod<&Button_GetIsHighlighted>("Button::get_IsHighlighted"),
        ScMethod<&Button_GetIsPushed>("Button::get_IsPushed"),
        ScMethod<&Button_GetAnimLoop>("Button::get_Loop"),
        ScMethod<&Button_GetBorderShadeColor>("Button::get_BorderShadeColor"),
        ScMethod<&Button_SetBorderShadeColor>("Button::set_BorderShadeColor"),
        ScMethod<&Button_GetMouseOverBackgroundColor>("Button::get_MouseOverBackgroundColor"),
        ScMethod<&Button_SetMouseOverBackgroundColor>("Button::set_MouseOverBackgroundColor"),
        ScMethod<&Button_GetPushedBackgroundColor>("Button::get_PushedBackgroundColor"),
        ScMethod<&Button_SetPushedBackgroundColor>("Button::set_PushedBackgroundColor"),
        ScMethod<&Button_GetMouseOverBorderColor>("Button::get_MouseOverBorderColor"),
        ScMethod<&Button_SetMouseOverBorderColor>("Button::set_MouseOverBorderColor"),
        ScMethod<&Button_GetPushedBorderColor>("Button::get_PushedBorderColor"),
        ScMethod<&Button_SetPushedBorderColor>("Button::set_PushedBorderColor"),
        ScMethod<&Button_GetMouseOverGraphic>("Button::get_MouseOverGraphic"),
        ScMethod<&Button_SetMouseOverGraphic>("Button::set_MouseOverGraphic"),
        ScMethod<&Button_GetNormalGraphic>("Button::get_NormalGraphic"),
        ScMethod<&Button_SetNormalGraphic>("Button::set_NormalGraphic"),
        ScMethod<&Button_GetPushedGraphic>("Button::get_PushedGraphic"),
        ScMethod<&Button_SetPushedGraphic>("Button::set_PushedGraphic"),
        ScMethod<&Button_GetText_New>("Button::get_Text"),
        ScMethod<&Button_SetText>("Button::set_Text"),
        ScMethod<&Button_GetTextColor>("Button::get_TextColor"),
        ScMethod<&Button_SetTextColor>("Button::set_TextColor"),
        ScMethod<&Button_GetTextOutlineColor>("Button::get_TextOutlineColor"),
        ScMethod<&Button_SetTextOutlineColor>("Button::set_TextOutlineColor"),
        ScMethod<&Button_GetMouseOverTextColor>("Button::get_MouseOverTextColor"),
        ScMethod<&Button_SetMouseOverTextColor>("Button::set_MouseOverTextColor"),
        ScMethod<&Button_GetPushedTextColor>("Button::get_PushedTextColor"),
        ScMethod<&Button_SetPushedTextColor>("Button::set_PushedTextColor"),
        ScMethod<&Button_GetAnimView>("Button::get_View"),
    };

    ccAddExternalFunctions(button_api);
}
