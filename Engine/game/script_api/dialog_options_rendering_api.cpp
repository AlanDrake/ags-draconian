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
// Script bindings registered by RegisterDialogOptionsRenderingAPI().
//
//=============================================================================
#include "game/dialog/dialog_options_rendering.h"
#include "game/dynobj/cc_dialog.h"
#include "game/game.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

RuntimeScriptValue Sc_DialogOptionsRendering_Update(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDialogOptionsRendering, DialogOptionsRendering_Update);
}

RuntimeScriptValue Sc_DialogOptionsRendering_RunActiveOption(void *self, const RuntimeScriptValue *params,
                                                             int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptDialogOptionsRendering, DialogOptionsRendering_RunActiveOption);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetActiveOptionID(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetActiveOptionID);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID)
RuntimeScriptValue Sc_DialogOptionsRendering_SetActiveOptionID(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetActiveOptionID);
}

// ScriptDialog* (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetDialogToRender(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDialogOptionsRendering, ScriptDialog, ccDynamicDialog,
                    DialogOptionsRendering_GetDialogToRender);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetHeight(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetHeight);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newHeight)
RuntimeScriptValue Sc_DialogOptionsRendering_SetHeight(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetHeight);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxX(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxX);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newX)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxX(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxX);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxY(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxY);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newY)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxY(void *self, const RuntimeScriptValue *params,
                                                               int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxY);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetParserTextboxWidth(void *self, const RuntimeScriptValue *params,
                                                                   int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetParserTextboxWidth);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
RuntimeScriptValue Sc_DialogOptionsRendering_SetParserTextboxWidth(void *self, const RuntimeScriptValue *params,
                                                                   int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetParserTextboxWidth);
}

// ScriptDrawingSurface* (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetSurface(void *self, const RuntimeScriptValue *params,
                                                        int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDialogOptionsRendering, ScriptDrawingSurface, DialogOptionsRendering_GetSurface);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetWidth);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
RuntimeScriptValue Sc_DialogOptionsRendering_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetWidth);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetX);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newX)
RuntimeScriptValue Sc_DialogOptionsRendering_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetX);
}

// int (ScriptDialogOptionsRendering *dlgOptRender)
RuntimeScriptValue Sc_DialogOptionsRendering_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialogOptionsRendering, DialogOptionsRendering_GetY);
}

// void (ScriptDialogOptionsRendering *dlgOptRender, int newY)
RuntimeScriptValue Sc_DialogOptionsRendering_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialogOptionsRendering, DialogOptionsRendering_SetY);
}


void RegisterDialogOptionsRenderingAPI()
{
    ScFnRegister dialogopt_api[] = {
        {"DialogOptionsRenderingInfo::Update^0", API_FN_PAIR(DialogOptionsRendering_Update)},
        {"DialogOptionsRenderingInfo::RunActiveOption^0", API_FN_PAIR(DialogOptionsRendering_RunActiveOption)},
        {"DialogOptionsRenderingInfo::get_ActiveOptionID", API_FN_PAIR(DialogOptionsRendering_GetActiveOptionID)},
        {"DialogOptionsRenderingInfo::set_ActiveOptionID", API_FN_PAIR(DialogOptionsRendering_SetActiveOptionID)},
        {"DialogOptionsRenderingInfo::get_DialogToRender", API_FN_PAIR(DialogOptionsRendering_GetDialogToRender)},
        {"DialogOptionsRenderingInfo::get_Height", API_FN_PAIR(DialogOptionsRendering_GetHeight)},
        {"DialogOptionsRenderingInfo::set_Height", API_FN_PAIR(DialogOptionsRendering_SetHeight)},
        {"DialogOptionsRenderingInfo::get_ParserTextBoxX", API_FN_PAIR(DialogOptionsRendering_GetParserTextboxX)},
        {"DialogOptionsRenderingInfo::set_ParserTextBoxX", API_FN_PAIR(DialogOptionsRendering_SetParserTextboxX)},
        {"DialogOptionsRenderingInfo::get_ParserTextBoxY", API_FN_PAIR(DialogOptionsRendering_GetParserTextboxY)},
        {"DialogOptionsRenderingInfo::set_ParserTextBoxY", API_FN_PAIR(DialogOptionsRendering_SetParserTextboxY)},
        {"DialogOptionsRenderingInfo::get_ParserTextBoxWidth",
         API_FN_PAIR(DialogOptionsRendering_GetParserTextboxWidth)},
        {"DialogOptionsRenderingInfo::set_ParserTextBoxWidth",
         API_FN_PAIR(DialogOptionsRendering_SetParserTextboxWidth)},
        {"DialogOptionsRenderingInfo::get_Surface", API_FN_PAIR(DialogOptionsRendering_GetSurface)},
        {"DialogOptionsRenderingInfo::get_Width", API_FN_PAIR(DialogOptionsRendering_GetWidth)},
        {"DialogOptionsRenderingInfo::set_Width", API_FN_PAIR(DialogOptionsRendering_SetWidth)},
        {"DialogOptionsRenderingInfo::get_X", API_FN_PAIR(DialogOptionsRendering_GetX)},
        {"DialogOptionsRenderingInfo::set_X", API_FN_PAIR(DialogOptionsRendering_SetX)},
        {"DialogOptionsRenderingInfo::get_Y", API_FN_PAIR(DialogOptionsRendering_GetY)},
        {"DialogOptionsRenderingInfo::set_Y", API_FN_PAIR(DialogOptionsRendering_SetY)},
    };

    ccAddExternalFunctions(dialogopt_api);
}
