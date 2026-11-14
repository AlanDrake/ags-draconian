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
#include "ac/dynobj/scriptstring.h"
#include "game/ui/label.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// void (GUILabel *labl, char *buffer)
RuntimeScriptValue Sc_Label_GetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUILabel, Label_GetText, char);
}

// void (GUILabel *labl, const char *newtx)
RuntimeScriptValue Sc_Label_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(GUILabel, Label_SetText, const char);
}

RuntimeScriptValue Sc_Label_GetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetTextAlignment);
}

RuntimeScriptValue Sc_Label_SetTextAlignment(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetTextAlignment);
}


// int (GUILabel *labl)
RuntimeScriptValue Sc_Label_GetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetFont);
}

// void (GUILabel *guil, int fontnum)
RuntimeScriptValue Sc_Label_SetFont(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetFont);
}

// const char* (GUILabel *labl)
RuntimeScriptValue Sc_Label_GetText_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(GUILabel, const char, myScriptStringImpl, Label_GetText_New);
}

RuntimeScriptValue Sc_Label_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetTextColor);
}

RuntimeScriptValue Sc_Label_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetTextColor);
}

RuntimeScriptValue Sc_Label_GetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(GUILabel, Label_GetTextOutlineColor);
}

RuntimeScriptValue Sc_Label_SetTextOutlineColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(GUILabel, Label_SetTextOutlineColor);
}


void RegisterLabelAPI()
{
    ScFnRegister label_api[] = {
        {"Label::GetText^1", API_FN_PAIR(Label_GetText)},
        {"Label::SetText^1", API_FN_PAIR(Label_SetText)},
        {"Label::get_TextAlignment", API_FN_PAIR(Label_GetTextAlignment)},
        {"Label::set_TextAlignment", API_FN_PAIR(Label_SetTextAlignment)},
        {"Label::get_Font", API_FN_PAIR(Label_GetFont)},
        {"Label::set_Font", API_FN_PAIR(Label_SetFont)},
        {"Label::get_Text", API_FN_PAIR(Label_GetText_New)},
        {"Label::set_Text", API_FN_PAIR(Label_SetText)},
        {"Label::get_TextColor", API_FN_PAIR(Label_GetTextColor)},
        {"Label::set_TextColor", API_FN_PAIR(Label_SetTextColor)},
        {"Label::get_TextOutlineColor", API_FN_PAIR(Label_GetTextOutlineColor)},
        {"Label::set_TextOutlineColor", API_FN_PAIR(Label_SetTextOutlineColor)},
    };

    ccAddExternalFunctions(label_api);
}
