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
#include "game/dialog/dialog.h"
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/character.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterDialogAPI()
{
    ScFnRegister dialog_api[] = {
        // Dialog
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&Dialog_GetByName>("Dialog::GetByName"), ScStatic<&Dialog_GetByName>("Dialog::GetByScriptName"),
        ScStatic<&Dialog_Stop>("Dialog::Stop"), ScStatic<&Dialog_GetCurrentDialog>("Dialog::get_CurrentDialog"),
        ScStatic<&Dialog_GetExecutedOption>("Dialog::get_ExecutedOption"),
        ScStatic<&Dialog_GetAreOptionsDisplayed>("Dialog::get_AreOptionsDisplayed"),
        ScMethod<&Dialog_GetID>("Dialog::get_ID"), ScMethod<&Dialog_GetOptionCount>("Dialog::get_OptionCount"),
        ScMethod<&Dialog_GetScriptName>("Dialog::get_ScriptName"),
        ScMethod<&Dialog_GetShowTextParser>("Dialog::get_ShowTextParser"),
        ScMethod<&Dialog_DisplayOptions>("Dialog::DisplayOptions^1"),
        ScMethod<&Dialog_GetOptionState>("Dialog::GetOptionState^1"),
        ScMethod<&Dialog_GetOptionText>("Dialog::GetOptionText^1"),
        ScMethod<&Dialog_SetOptionText>("Dialog::SetOptionText^2"),
        ScMethod<&Dialog_HasOptionBeenChosen>("Dialog::HasOptionBeenChosen^1"),
        ScMethod<&Dialog_SetHasOptionBeenChosen>("Dialog::SetHasOptionBeenChosen^2"),
        ScMethod<&Dialog_SetOptionState>("Dialog::SetOptionState^2"), ScMethod<&Dialog_Start>("Dialog::Start^0"),
        ScMethod<&Dialog_StartOption>("Dialog::StartOption^1"), ScMethod<&Dialog_GetProperty>("Dialog::GetProperty^1"),
        ScMethod<&Dialog_GetTextProperty>("Dialog::GetTextProperty^1"),
        ScMethod<&Dialog_SetProperty>("Dialog::SetProperty^2"),
        ScMethod<&Dialog_SetTextProperty>("Dialog::SetTextProperty^2"),

        // DialogOptions
        ScStatic<&DialogOptions_GetAreDisplayed, ScBoolResult>("DialogOptions::get_AreDisplayed"),
        ScStatic<&DialogOptions_GetBulletGraphic>("DialogOptions::get_BulletGraphic"),
        ScStatic<&DialogOptions_SetBulletGraphic>("DialogOptions::set_BulletGraphic"),
        ScStatic<&DialogOptions_GetDrawMode>("DialogOptions::get_DrawMode"),
        ScStatic<&DialogOptions_SetDrawMode>("DialogOptions::set_DrawMode"),
        ScStatic<&DialogOptions_GetFont>("DialogOptions::get_Font"),
        ScStatic<&DialogOptions_SetFont>("DialogOptions::set_Font"),
        ScStatic<&DialogOptions_GetItemGap>("DialogOptions::get_ItemGap"),
        ScStatic<&DialogOptions_SetItemGap>("DialogOptions::set_ItemGap"),
        ScStatic<&DialogOptions_GetTemplateGUI>("DialogOptions::get_TemplateGUI"),
        ScStatic<&DialogOptions_SetTemplateGUI>("DialogOptions::set_TemplateGUI"),
        ScStatic<&DialogOptions_GetGUIX>("DialogOptions::get_GUIX"),
        ScStatic<&DialogOptions_SetGUIX>("DialogOptions::set_GUIX"),
        ScStatic<&DialogOptions_GetGUIY>("DialogOptions::get_GUIY"),
        ScStatic<&DialogOptions_SetGUIY>("DialogOptions::set_GUIY"),
        ScStatic<&DialogOptions_GetHighlightColor>("DialogOptions::get_HighlightColor"),
        ScStatic<&DialogOptions_SetHighlightColor>("DialogOptions::set_HighlightColor"),
        ScStatic<&DialogOptions_GetMaxGUIWidth>("DialogOptions::get_MaxGUIWidth"),
        ScStatic<&DialogOptions_SetMaxGUIWidth>("DialogOptions::set_MaxGUIWidth"),
        ScStatic<&DialogOptions_GetMinGUIWidth>("DialogOptions::get_MinGUIWidth"),
        ScStatic<&DialogOptions_SetMinGUIWidth>("DialogOptions::set_MinGUIWidth"),
        ScStatic<&DialogOptions_GetItemNumbering>("DialogOptions::get_ItemNumbering"),
        ScStatic<&DialogOptions_SetItemNumbering>("DialogOptions::set_ItemNumbering"),
        ScStatic<&DialogOptions_GetOverlay>("DialogOptions::get_Overlay"),
        ScStatic<&DialogOptions_GetPaddingX>("DialogOptions::get_PaddingX"),
        ScStatic<&DialogOptions_SetPaddingX>("DialogOptions::set_PaddingX"),
        ScStatic<&DialogOptions_GetPaddingY>("DialogOptions::get_PaddingY"),
        ScStatic<&DialogOptions_SetPaddingY>("DialogOptions::set_PaddingY"),
        ScStatic<&DialogOptions_GetReadColor>("DialogOptions::get_ReadColor"),
        ScStatic<&DialogOptions_SetReadColor>("DialogOptions::set_ReadColor"),
        ScStatic<&DialogOptions_GetTextAlignment>("DialogOptions::get_TextAlignment"),
        ScStatic<&DialogOptions_SetTextAlignment>("DialogOptions::set_TextAlignment"),
        ScStatic<&DialogOptions_GetZOrder>("DialogOptions::get_ZOrder"),
        ScStatic<&DialogOptions_SetZOrder>("DialogOptions::set_ZOrder"),

        ScStatic<&DialogOptions_GetHasCustomRender>("DialogOptions::get_HasCustomRender"),
        ScStatic<&DialogOptions_GetItemCount>("DialogOptions::get_ItemCount"),
        ScStatic<&DialogOptions_GetItemOptionID>("DialogOptions::geti_ItemOptionID"),
        ScStatic<&DialogOptions_GetItemX>("DialogOptions::geti_ItemX"),
        ScStatic<&DialogOptions_GetItemY>("DialogOptions::geti_ItemY"),
        ScStatic<&DialogOptions_GetItemWidth>("DialogOptions::geti_ItemWidth"),
        ScStatic<&DialogOptions_GetItemHeight>("DialogOptions::geti_ItemHeight")};

    ccAddExternalFunctions(dialog_api);
}
