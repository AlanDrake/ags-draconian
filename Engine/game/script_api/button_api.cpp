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
#include "ac/dynobj/scriptstring.h"
#include "game/ui/button.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// void | GUIButton *btn, int view, int loop, int speed, int repeat
RuntimeScriptValue Sc_Button_Animate4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(GUIButton, Button_Animate4);
}

RuntimeScriptValue Sc_Button_Animate7(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(GUIButton, Button_Animate7);
}

RuntimeScriptValue Sc_Button_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT8(GUIButton, Button_Animate);
}

// const char* | GUIButton *btn
RuntimeScriptValue Sc_Button_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUIButton, const char, myScriptStringImpl, Button_GetText_New);
}

// void | GUIButton *btn, char *buffer
RuntimeScriptValue Sc_Button_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIButton, Button_GetText, char);
}

// void | GUIButton *btn, const char *newtx
RuntimeScriptValue Sc_Button_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUIButton, Button_SetText, const char);
}

// void | GUIButton *btn, int new_font
RuntimeScriptValue Sc_Button_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetFont);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetFont);
}

RuntimeScriptValue Sc_Button_GetColorStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetColorStyle);
}

RuntimeScriptValue Sc_Button_SetColorStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetColorStyle);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetClipImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetClipImage);
}

// void | GUIButton *btn, int newval
RuntimeScriptValue Sc_Button_SetClipImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetClipImage);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetGraphic);
}

RuntimeScriptValue Sc_Button_GetGraphicFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetGraphicFlip);
}

RuntimeScriptValue Sc_Button_SetGraphicFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetGraphicFlip);
}


RuntimeScriptValue Sc_Button_GetIsHighlighted(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_GetIsHighlighted);
}

RuntimeScriptValue Sc_Button_GetIsPushed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_GetIsPushed);
}

RuntimeScriptValue Sc_Button_GetMouseOverBackgroundColor(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverBackgroundColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverBackgroundColor(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverBackgroundColor);
}

RuntimeScriptValue Sc_Button_GetPushedBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedBackgroundColor);
}

RuntimeScriptValue Sc_Button_SetPushedBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedBackgroundColor);
}

RuntimeScriptValue Sc_Button_GetMouseOverBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverBorderColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverBorderColor);
}

RuntimeScriptValue Sc_Button_GetPushedBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedBorderColor);
}

RuntimeScriptValue Sc_Button_SetPushedBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedBorderColor);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetMouseOverGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverGraphic);
}

// void | GUIButton *btn, int slotn
RuntimeScriptValue Sc_Button_SetMouseOverGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverGraphic);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetNormalGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetNormalGraphic);
}

// void | GUIButton *btn, int slotn
RuntimeScriptValue Sc_Button_SetNormalGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetNormalGraphic);
}

// int | GUIButton *btn
RuntimeScriptValue Sc_Button_GetPushedGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedGraphic);
}

// void | GUIButton *btn, int slotn
RuntimeScriptValue Sc_Button_SetPushedGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedGraphic);
}

RuntimeScriptValue Sc_Button_GetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetBorderShadeColor);
}

RuntimeScriptValue Sc_Button_SetBorderShadeColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetBorderShadeColor);
}

RuntimeScriptValue Sc_Button_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextColor);
}

RuntimeScriptValue Sc_Button_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextColor);
}

RuntimeScriptValue Sc_Button_GetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextOutlineColor);
}

RuntimeScriptValue Sc_Button_SetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextOutlineColor);
}

RuntimeScriptValue Sc_Button_GetMouseOverTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetMouseOverTextColor);
}

RuntimeScriptValue Sc_Button_SetMouseOverTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetMouseOverTextColor);
}

RuntimeScriptValue Sc_Button_GetPushedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetPushedTextColor);
}

RuntimeScriptValue Sc_Button_SetPushedTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetPushedTextColor);
}

RuntimeScriptValue Sc_Button_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_Click);
}

RuntimeScriptValue Sc_Button_RunFrameEvent(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(GUIButton, Button_RunFrameEvent);
}

RuntimeScriptValue Sc_Button_IsAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_IsAnimating);
}

RuntimeScriptValue Sc_Button_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextAlignment);
}

RuntimeScriptValue Sc_Button_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextAlignment);
}

RuntimeScriptValue Sc_Button_GetWrapText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(GUIButton, Button_GetWrapText);
}

RuntimeScriptValue Sc_Button_SetWrapText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(GUIButton, Button_SetWrapText);
}

RuntimeScriptValue Sc_Button_GetTextPaddingHorizontal(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextPaddingHorizontal);
}

RuntimeScriptValue Sc_Button_SetTextPaddingHorizontal(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextPaddingHorizontal);
}

RuntimeScriptValue Sc_Button_GetTextPaddingVertical(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetTextPaddingVertical);
}

RuntimeScriptValue Sc_Button_SetTextPaddingVertical(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUIButton, Button_SetTextPaddingVertical);
}

RuntimeScriptValue Sc_Button_GetAnimFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimFrame);
}

RuntimeScriptValue Sc_Button_GetAnimLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimLoop);
}

RuntimeScriptValue Sc_Button_GetAnimView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUIButton, Button_GetAnimView);
}

void RegisterButtonAPI()
{
    ScFnRegister button_api[] = {
        {"Button::Animate^4", API_FN_PAIR(Button_Animate4)},
        {"Button::Animate^7", API_FN_PAIR(Button_Animate7)},
        {"Button::Animate^8", API_FN_PAIR(Button_Animate)},
        {"Button::Click^1", API_FN_PAIR(Button_Click)},
        {"Button::RunFrameEvent^3", API_FN_PAIR(Button_RunFrameEvent)},
        {"Button::GetText^1", API_FN_PAIR(Button_GetText)},
        {"Button::SetText^1", API_FN_PAIR(Button_SetText)},
        {"Button::get_TextAlignment", API_FN_PAIR(Button_GetTextAlignment)},
        {"Button::set_TextAlignment", API_FN_PAIR(Button_SetTextAlignment)},
        {"Button::get_TextPaddingHorizontal", API_FN_PAIR(Button_GetTextPaddingHorizontal)},
        {"Button::set_TextPaddingHorizontal", API_FN_PAIR(Button_SetTextPaddingHorizontal)},
        {"Button::get_TextPaddingVertical", API_FN_PAIR(Button_GetTextPaddingVertical)},
        {"Button::set_TextPaddingVertical", API_FN_PAIR(Button_SetTextPaddingVertical)},
        {"Button::get_WrapText", API_FN_PAIR(Button_GetWrapText)},
        {"Button::set_WrapText", API_FN_PAIR(Button_SetWrapText)},
        {"Button::get_Animating", API_FN_PAIR(Button_IsAnimating)},
        {"Button::get_ColorStyle", API_FN_PAIR(Button_GetColorStyle)},
        {"Button::set_ColorStyle", API_FN_PAIR(Button_SetColorStyle)},
        {"Button::get_ClipImage", API_FN_PAIR(Button_GetClipImage)},
        {"Button::set_ClipImage", API_FN_PAIR(Button_SetClipImage)},
        {"Button::get_Font", API_FN_PAIR(Button_GetFont)},
        {"Button::set_Font", API_FN_PAIR(Button_SetFont)},
        {"Button::get_Frame", API_FN_PAIR(Button_GetAnimFrame)},
        {"Button::get_Graphic", API_FN_PAIR(Button_GetGraphic)},
        {"Button::get_GraphicFlip", API_FN_PAIR(Button_GetGraphicFlip)},
        {"Button::set_GraphicFlip", API_FN_PAIR(Button_SetGraphicFlip)},
        {"Button::get_IsHighlighted", API_FN_PAIR(Button_GetIsHighlighted)},
        {"Button::get_IsPushed", API_FN_PAIR(Button_GetIsPushed)},
        {"Button::get_Loop", API_FN_PAIR(Button_GetAnimLoop)},
        {"Button::get_BorderShadeColor", API_FN_PAIR(Button_GetBorderShadeColor)},
        {"Button::set_BorderShadeColor", API_FN_PAIR(Button_SetBorderShadeColor)},
        {"Button::get_MouseOverBackgroundColor", API_FN_PAIR(Button_GetMouseOverBackgroundColor)},
        {"Button::set_MouseOverBackgroundColor", API_FN_PAIR(Button_SetMouseOverBackgroundColor)},
        {"Button::get_PushedBackgroundColor", API_FN_PAIR(Button_GetPushedBackgroundColor)},
        {"Button::set_PushedBackgroundColor", API_FN_PAIR(Button_SetPushedBackgroundColor)},
        {"Button::get_MouseOverBorderColor", API_FN_PAIR(Button_GetMouseOverBorderColor)},
        {"Button::set_MouseOverBorderColor", API_FN_PAIR(Button_SetMouseOverBorderColor)},
        {"Button::get_PushedBorderColor", API_FN_PAIR(Button_GetPushedBorderColor)},
        {"Button::set_PushedBorderColor", API_FN_PAIR(Button_SetPushedBorderColor)},
        {"Button::get_MouseOverGraphic", API_FN_PAIR(Button_GetMouseOverGraphic)},
        {"Button::set_MouseOverGraphic", API_FN_PAIR(Button_SetMouseOverGraphic)},
        {"Button::get_NormalGraphic", API_FN_PAIR(Button_GetNormalGraphic)},
        {"Button::set_NormalGraphic", API_FN_PAIR(Button_SetNormalGraphic)},
        {"Button::get_PushedGraphic", API_FN_PAIR(Button_GetPushedGraphic)},
        {"Button::set_PushedGraphic", API_FN_PAIR(Button_SetPushedGraphic)},
        {"Button::get_Text", API_FN_PAIR(Button_GetText_New)},
        {"Button::set_Text", API_FN_PAIR(Button_SetText)},
        {"Button::get_TextColor", API_FN_PAIR(Button_GetTextColor)},
        {"Button::set_TextColor", API_FN_PAIR(Button_SetTextColor)},
        {"Button::get_TextOutlineColor", API_FN_PAIR(Button_GetTextOutlineColor)},
        {"Button::set_TextOutlineColor", API_FN_PAIR(Button_SetTextOutlineColor)},
        {"Button::get_MouseOverTextColor", API_FN_PAIR(Button_GetMouseOverTextColor)},
        {"Button::set_MouseOverTextColor", API_FN_PAIR(Button_SetMouseOverTextColor)},
        {"Button::get_PushedTextColor", API_FN_PAIR(Button_GetPushedTextColor)},
        {"Button::set_PushedTextColor", API_FN_PAIR(Button_SetPushedTextColor)},
        {"Button::get_View", API_FN_PAIR(Button_GetAnimView)},
    };

    ccAddExternalFunctions(button_api);
}
