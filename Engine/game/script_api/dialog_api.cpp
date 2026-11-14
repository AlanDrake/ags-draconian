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
// Script bindings registered by RegisterDialogAPI().
//
//=============================================================================
#include "ac/game.h"
#include "game/dialog/dialog.h"
#include "game/dynobj/cc_dialog.h"
#include "game/dynobj/cc_gui.h"
#include "game/dynobj/script_string.h"
#include "game/world/character.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Dialog_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptDialog, ccDynamicDialog, Dialog_GetByName, const char);
}

RuntimeScriptValue Sc_Dialog_Stop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Dialog_Stop);
}

RuntimeScriptValue Sc_Dialog_GetCurrentDialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(ScriptDialog, ccDynamicDialog, Dialog_GetCurrentDialog);
}

RuntimeScriptValue Sc_Dialog_GetExecutedOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Dialog_GetExecutedOption);
}

RuntimeScriptValue Sc_Dialog_GetAreOptionsDisplayed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Dialog_GetAreOptionsDisplayed);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetID);
}

RuntimeScriptValue Sc_Dialog_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDialog, const char, myScriptStringImpl, Dialog_GetScriptName);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetOptionCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetOptionCount);
}

// int (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_GetShowTextParser(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDialog, Dialog_GetShowTextParser);
}

// int (ScriptDialog *sd, int sayChosenOption)
RuntimeScriptValue Sc_Dialog_DisplayOptions(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_DisplayOptions);
}

// int (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_GetOptionState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_GetOptionState);
}

// const char* (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_GetOptionText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptDialog, const char, myScriptStringImpl, Dialog_GetOptionText);
}

RuntimeScriptValue Sc_Dialog_SetOptionText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_POBJ(ScriptDialog, Dialog_SetOptionText, const char);
}

// int (ScriptDialog *sd, int option)
RuntimeScriptValue Sc_Dialog_HasOptionBeenChosen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptDialog, Dialog_HasOptionBeenChosen);
}

RuntimeScriptValue Sc_Dialog_SetHasOptionBeenChosen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT_PBOOL(ScriptDialog, Dialog_SetHasOptionBeenChosen);
}

// void (ScriptDialog *sd, int option, int newState)
RuntimeScriptValue Sc_Dialog_SetOptionState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDialog, Dialog_SetOptionState);
}

// void (ScriptDialog *sd)
RuntimeScriptValue Sc_Dialog_Start(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDialog, Dialog_Start);
}

RuntimeScriptValue Sc_Dialog_StartOption(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDialog, Dialog_StartOption);
}

RuntimeScriptValue Sc_DialogOptions_GetTemplateGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(ScriptGUI, ccDynamicGUI, DialogOptions_GetTemplateGUI);
}

RuntimeScriptValue Sc_DialogOptions_SetTemplateGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(DialogOptions_SetTemplateGUI, ScriptGUI);
}

RuntimeScriptValue Sc_DialogOptions_GetGUIX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetGUIX);
}

RuntimeScriptValue Sc_DialogOptions_SetGUIX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetGUIX);
}

RuntimeScriptValue Sc_DialogOptions_GetGUIY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetGUIY);
}

RuntimeScriptValue Sc_DialogOptions_SetGUIY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetGUIY);
}

RuntimeScriptValue Sc_DialogOptions_GetPaddingX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetPaddingX);
}

RuntimeScriptValue Sc_DialogOptions_SetPaddingX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetPaddingX);
}

RuntimeScriptValue Sc_DialogOptions_GetPaddingY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetPaddingY);
}

RuntimeScriptValue Sc_DialogOptions_SetPaddingY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetPaddingY);
}

RuntimeScriptValue Sc_DialogOptions_GetAreDisplayed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(DialogOptions_GetAreDisplayed);
}

RuntimeScriptValue Sc_DialogOptions_GetBulletGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetBulletGraphic);
}

RuntimeScriptValue Sc_DialogOptions_SetBulletGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetBulletGraphic);
}

RuntimeScriptValue Sc_DialogOptions_GetFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetFont);
}

RuntimeScriptValue Sc_DialogOptions_SetFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetFont);
}

RuntimeScriptValue Sc_DialogOptions_GetItemNumbering(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetItemNumbering);
}

RuntimeScriptValue Sc_DialogOptions_SetItemNumbering(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetItemNumbering);
}

RuntimeScriptValue Sc_DialogOptions_GetOverlay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptOverlay, DialogOptions_GetOverlay);
}

RuntimeScriptValue Sc_DialogOptions_GetHighlightColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetHighlightColor);
}

RuntimeScriptValue Sc_DialogOptions_SetHighlightColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetHighlightColor);
}

RuntimeScriptValue Sc_DialogOptions_GetReadColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetReadColor);
}

RuntimeScriptValue Sc_DialogOptions_SetReadColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetReadColor);
}

RuntimeScriptValue Sc_DialogOptions_GetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetTextAlignment);
}

RuntimeScriptValue Sc_DialogOptions_SetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetTextAlignment);
}

RuntimeScriptValue Sc_DialogOptions_GetZOrder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetZOrder);
}

RuntimeScriptValue Sc_DialogOptions_SetZOrder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetZOrder);
}

RuntimeScriptValue Sc_DialogOptions_GetItemGap(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetItemGap);
}

RuntimeScriptValue Sc_DialogOptions_SetItemGap(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetItemGap);
}

RuntimeScriptValue Sc_DialogOptions_GetMaxGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetMaxGUIWidth);
}

RuntimeScriptValue Sc_DialogOptions_SetMaxGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetMaxGUIWidth);
}

RuntimeScriptValue Sc_DialogOptions_GetMinGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetMinGUIWidth);
}

RuntimeScriptValue Sc_DialogOptions_SetMinGUIWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetMinGUIWidth);
}

RuntimeScriptValue Sc_DialogOptions_GetDrawMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetDrawMode);
}

RuntimeScriptValue Sc_DialogOptions_SetDrawMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DialogOptions_SetDrawMode);
}

RuntimeScriptValue Sc_DialogOptions_GetHasCustomRender(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(DialogOptions_GetHasCustomRender);
}

RuntimeScriptValue Sc_DialogOptions_GetItemCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(DialogOptions_GetItemCount);
}

RuntimeScriptValue Sc_DialogOptions_GetItemOptionID(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(DialogOptions_GetItemOptionID);
}

RuntimeScriptValue Sc_DialogOptions_GetItemX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(DialogOptions_GetItemX);
}

RuntimeScriptValue Sc_DialogOptions_GetItemY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(DialogOptions_GetItemY);
}

RuntimeScriptValue Sc_DialogOptions_GetItemWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(DialogOptions_GetItemWidth);
}

RuntimeScriptValue Sc_DialogOptions_GetItemHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(DialogOptions_GetItemHeight);
}

RuntimeScriptValue Sc_Dialog_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptDialog, Dialog_GetProperty, const char);
}

RuntimeScriptValue Sc_Dialog_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptDialog, const char, myScriptStringImpl, Dialog_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Dialog_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptDialog, Dialog_SetProperty, const char);
}

RuntimeScriptValue Sc_Dialog_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptDialog, Dialog_SetTextProperty, const char, const char);
}

void RegisterDialogAPI()
{
    ScFnRegister dialog_api[] = {// Dialog
                                 // [OBSOLETE] GetByName => GetByScriptName
                                 {"Dialog::GetByName", API_FN_PAIR(Dialog_GetByName)},
                                 {"Dialog::GetByScriptName", API_FN_PAIR(Dialog_GetByName)},
                                 {"Dialog::Stop", API_FN_PAIR(Dialog_Stop)},
                                 {"Dialog::get_CurrentDialog", API_FN_PAIR(Dialog_GetCurrentDialog)},
                                 {"Dialog::get_ExecutedOption", API_FN_PAIR(Dialog_GetExecutedOption)},
                                 {"Dialog::get_AreOptionsDisplayed", API_FN_PAIR(Dialog_GetAreOptionsDisplayed)},
                                 {"Dialog::get_ID", API_FN_PAIR(Dialog_GetID)},
                                 {"Dialog::get_OptionCount", API_FN_PAIR(Dialog_GetOptionCount)},
                                 {"Dialog::get_ScriptName", API_FN_PAIR(Dialog_GetScriptName)},
                                 {"Dialog::get_ShowTextParser", API_FN_PAIR(Dialog_GetShowTextParser)},
                                 {"Dialog::DisplayOptions^1", API_FN_PAIR(Dialog_DisplayOptions)},
                                 {"Dialog::GetOptionState^1", API_FN_PAIR(Dialog_GetOptionState)},
                                 {"Dialog::GetOptionText^1", API_FN_PAIR(Dialog_GetOptionText)},
                                 {"Dialog::SetOptionText^2", API_FN_PAIR(Dialog_SetOptionText)},
                                 {"Dialog::HasOptionBeenChosen^1", API_FN_PAIR(Dialog_HasOptionBeenChosen)},
                                 {"Dialog::SetHasOptionBeenChosen^2", API_FN_PAIR(Dialog_SetHasOptionBeenChosen)},
                                 {"Dialog::SetOptionState^2", API_FN_PAIR(Dialog_SetOptionState)},
                                 {"Dialog::Start^0", API_FN_PAIR(Dialog_Start)},
                                 {"Dialog::StartOption^1", API_FN_PAIR(Dialog_StartOption)},
                                 {"Dialog::GetProperty^1", API_FN_PAIR(Dialog_GetProperty)},
                                 {"Dialog::GetTextProperty^1", API_FN_PAIR(Dialog_GetTextProperty)},
                                 {"Dialog::SetProperty^2", API_FN_PAIR(Dialog_SetProperty)},
                                 {"Dialog::SetTextProperty^2", API_FN_PAIR(Dialog_SetTextProperty)},

                                 // DialogOptions
                                 {"DialogOptions::get_AreDisplayed", API_FN_PAIR(DialogOptions_GetAreDisplayed)},
                                 {"DialogOptions::get_BulletGraphic", API_FN_PAIR(DialogOptions_GetBulletGraphic)},
                                 {"DialogOptions::set_BulletGraphic", API_FN_PAIR(DialogOptions_SetBulletGraphic)},
                                 {"DialogOptions::get_DrawMode", API_FN_PAIR(DialogOptions_GetDrawMode)},
                                 {"DialogOptions::set_DrawMode", API_FN_PAIR(DialogOptions_SetDrawMode)},
                                 {"DialogOptions::get_Font", API_FN_PAIR(DialogOptions_GetFont)},
                                 {"DialogOptions::set_Font", API_FN_PAIR(DialogOptions_SetFont)},
                                 {"DialogOptions::get_ItemGap", API_FN_PAIR(DialogOptions_GetItemGap)},
                                 {"DialogOptions::set_ItemGap", API_FN_PAIR(DialogOptions_SetItemGap)},
                                 {"DialogOptions::get_TemplateGUI", API_FN_PAIR(DialogOptions_GetTemplateGUI)},
                                 {"DialogOptions::set_TemplateGUI", API_FN_PAIR(DialogOptions_SetTemplateGUI)},
                                 {"DialogOptions::get_GUIX", API_FN_PAIR(DialogOptions_GetGUIX)},
                                 {"DialogOptions::set_GUIX", API_FN_PAIR(DialogOptions_SetGUIX)},
                                 {"DialogOptions::get_GUIY", API_FN_PAIR(DialogOptions_GetGUIY)},
                                 {"DialogOptions::set_GUIY", API_FN_PAIR(DialogOptions_SetGUIY)},
                                 {"DialogOptions::get_HighlightColor", API_FN_PAIR(DialogOptions_GetHighlightColor)},
                                 {"DialogOptions::set_HighlightColor", API_FN_PAIR(DialogOptions_SetHighlightColor)},
                                 {"DialogOptions::get_MaxGUIWidth", API_FN_PAIR(DialogOptions_GetMaxGUIWidth)},
                                 {"DialogOptions::set_MaxGUIWidth", API_FN_PAIR(DialogOptions_SetMaxGUIWidth)},
                                 {"DialogOptions::get_MinGUIWidth", API_FN_PAIR(DialogOptions_GetMinGUIWidth)},
                                 {"DialogOptions::set_MinGUIWidth", API_FN_PAIR(DialogOptions_SetMinGUIWidth)},
                                 {"DialogOptions::get_ItemNumbering", API_FN_PAIR(DialogOptions_GetItemNumbering)},
                                 {"DialogOptions::set_ItemNumbering", API_FN_PAIR(DialogOptions_SetItemNumbering)},
                                 {"DialogOptions::get_Overlay", API_FN_PAIR(DialogOptions_GetOverlay)},
                                 {"DialogOptions::get_PaddingX", API_FN_PAIR(DialogOptions_GetPaddingX)},
                                 {"DialogOptions::set_PaddingX", API_FN_PAIR(DialogOptions_SetPaddingX)},
                                 {"DialogOptions::get_PaddingY", API_FN_PAIR(DialogOptions_GetPaddingY)},
                                 {"DialogOptions::set_PaddingY", API_FN_PAIR(DialogOptions_SetPaddingY)},
                                 {"DialogOptions::get_ReadColor", API_FN_PAIR(DialogOptions_GetReadColor)},
                                 {"DialogOptions::set_ReadColor", API_FN_PAIR(DialogOptions_SetReadColor)},
                                 {"DialogOptions::get_TextAlignment", API_FN_PAIR(DialogOptions_GetTextAlignment)},
                                 {"DialogOptions::set_TextAlignment", API_FN_PAIR(DialogOptions_SetTextAlignment)},
                                 {"DialogOptions::get_ZOrder", API_FN_PAIR(DialogOptions_GetZOrder)},
                                 {"DialogOptions::set_ZOrder", API_FN_PAIR(DialogOptions_SetZOrder)},

                                 {"DialogOptions::get_HasCustomRender", API_FN_PAIR(DialogOptions_GetHasCustomRender)},
                                 {"DialogOptions::get_ItemCount", API_FN_PAIR(DialogOptions_GetItemCount)},
                                 {"DialogOptions::geti_ItemOptionID", API_FN_PAIR(DialogOptions_GetItemOptionID)},
                                 {"DialogOptions::geti_ItemX", API_FN_PAIR(DialogOptions_GetItemX)},
                                 {"DialogOptions::geti_ItemY", API_FN_PAIR(DialogOptions_GetItemY)},
                                 {"DialogOptions::geti_ItemWidth", API_FN_PAIR(DialogOptions_GetItemWidth)},
                                 {"DialogOptions::geti_ItemHeight", API_FN_PAIR(DialogOptions_GetItemHeight)}};

    ccAddExternalFunctions(dialog_api);
}
