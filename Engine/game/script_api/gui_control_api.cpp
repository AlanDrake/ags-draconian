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
// Script bindings registered by RegisterGUIControlAPI().
//
//=============================================================================
#include "game/dynobj/cc_gui_control.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "game/ui/gui_control.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterGUIControlAPI()
{
    ScFnRegister guicontrol_api[] = {
        ScStatic<&GUIControl_GetAtScreenXY2>("GUIControl::GetAtScreenXY^2"),
        ScStatic<&GUIControl_GetAtScreenXY>("GUIControl::GetAtScreenXY^4"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&GUIControl_GetByName>("GUIControl::GetByName"),
        ScStatic<&GUIControl_GetByName>("GUIControl::GetByScriptName"),

        ScMethod<&GUIControl_BringToFront>("GUIControl::BringToFront^0"),
        ScMethod<&GUIControl_SendToBack>("GUIControl::SendToBack^0"),
        ScMethod<&GUIControl_SetPosition>("GUIControl::SetPosition^2"),
        ScMethod<&GUIControl_SetSize>("GUIControl::SetSize^2"),
        ScMethod<&GUIControl_GetGraphicPosition, ScManagedBy<globalDynamicArray>>("GUIControl::GetGraphicPosition^0"),
        ScMethod<&GUIControl_GetGraphicBoundBox, ScManagedBy<globalDynamicArray>>("GUIControl::GetGraphicBoundBox^0"),
        ScMethod<&GUIControl_GetProperty>("GUIControl::GetProperty^1"),
        ScMethod<&GUIControl_GetTextProperty>("GUIControl::GetTextProperty^1"),
        ScMethod<&GUIControl_SetProperty>("GUIControl::SetProperty^2"),
        ScMethod<&GUIControl_SetTextProperty>("GUIControl::SetTextProperty^2"),
        ScMethod<&GUIControl_GetAsButton>("GUIControl::get_AsButton"),
        ScMethod<&GUIControl_GetAsInvWindow>("GUIControl::get_AsInvWindow"),
        ScMethod<&GUIControl_GetAsLabel>("GUIControl::get_AsLabel"),
        ScMethod<&GUIControl_GetAsListBox>("GUIControl::get_AsListBox"),
        ScMethod<&GUIControl_GetAsSlider>("GUIControl::get_AsSlider"),
        ScMethod<&GUIControl_GetAsTextBox>("GUIControl::get_AsTextBox"),
        ScMethod<&GUIControl_GetClickable>("GUIControl::get_Clickable"),
        ScMethod<&GUIControl_SetClickable>("GUIControl::set_Clickable"),
        ScMethod<&GUIControl_GetEnabled>("GUIControl::get_Enabled"),
        ScMethod<&GUIControl_SetEnabled>("GUIControl::set_Enabled"),
        ScMethod<&GUIControl_GetHeight>("GUIControl::get_Height"),
        ScMethod<&GUIControl_SetHeight>("GUIControl::set_Height"),
        ScMethod<&GUIControl_GetID>("GUIControl::get_ID"),
        ScMethod<&GUIControl_GetOwningGUI, ScManagedBy<ccDynamicGUIControl>>("GUIControl::get_OwningGUI"),
        ScMethod<&GUIControl_GetScriptName>("GUIControl::get_ScriptName"),
        ScMethod<&GUIControl_GetVisible>("GUIControl::get_Visible"),
        ScMethod<&GUIControl_SetVisible>("GUIControl::set_Visible"),
        ScMethod<&GUIControl_GetWidth>("GUIControl::get_Width"),
        ScMethod<&GUIControl_SetWidth>("GUIControl::set_Width"),
        ScMethod<&GUIControl_GetX>("GUIControl::get_X"),
        ScMethod<&GUIControl_SetX>("GUIControl::set_X"),
        ScMethod<&GUIControl_GetY>("GUIControl::get_Y"),
        ScMethod<&GUIControl_SetY>("GUIControl::set_Y"),
        ScMethod<&GUIControl_GetRotation>("GUIControl::get_Rotation"),
        ScMethod<&GUIControl_SetRotation, ScUndefinedResult>("GUIControl::set_Rotation"),
        ScMethod<&GUIControl_GetScaleX>("GUIControl::get_ScaleX"),
        ScMethod<&GUIControl_SetScaleX, ScUndefinedResult>("GUIControl::set_ScaleX"),
        ScMethod<&GUIControl_GetScaleY>("GUIControl::get_ScaleY"),
        ScMethod<&GUIControl_SetScaleY, ScUndefinedResult>("GUIControl::set_ScaleY"),
        ScMethod<&GUIControl_SetScale, ScUndefinedResult>("GUIControl::SetScale"),
        ScMethod<&GUIControl_GetZOrder>("GUIControl::get_ZOrder"),
        ScMethod<&GUIControl_SetZOrder>("GUIControl::set_ZOrder"),
        ScMethod<&GUIControl_GetTranslated>("GUIControl::get_Translated"),
        ScMethod<&GUIControl_SetTranslated>("GUIControl::set_Translated"),
        ScMethod<&GUIControl_GetOpacity>("GUIControl::get_Opacity"),
        ScMethod<&GUIControl_SetOpacity>("GUIControl::set_Opacity"),
        // [OBSOLETE] Transparency => Opacity
        ScMethod<&GUIControl_GetTransparency>("GUIControl::get_Transparency"),
        ScMethod<&GUIControl_SetTransparency>("GUIControl::set_Transparency"),

        ScMethod<&GUIControl_GetBackgroundColor>("GUIControl::get_BackgroundColor"),
        ScMethod<&GUIControl_SetBackgroundColor>("GUIControl::set_BackgroundColor"),
        ScMethod<&GUIControl_GetBorderColor>("GUIControl::get_BorderColor"),
        ScMethod<&GUIControl_SetBorderColor>("GUIControl::set_BorderColor"),
        ScMethod<&GUIControl_GetBorderWidth>("GUIControl::get_BorderWidth"),
        ScMethod<&GUIControl_SetBorderWidth>("GUIControl::set_BorderWidth"),
        ScMethod<&GUIControl_GetPaddingX>("GUIControl::get_PaddingX"),
        ScMethod<&GUIControl_SetPaddingX>("GUIControl::set_PaddingX"),
        ScMethod<&GUIControl_GetPaddingY>("GUIControl::get_PaddingY"),
        ScMethod<&GUIControl_SetPaddingY>("GUIControl::set_PaddingY"),
        ScMethod<&GUIControl_GetShowBorder>("GUIControl::get_ShowBorder"),
        ScMethod<&GUIControl_SetShowBorder>("GUIControl::set_ShowBorder"),
        ScMethod<&GUIControl_GetSolidBackground>("GUIControl::get_SolidBackground"),
        ScMethod<&GUIControl_SetSolidBackground>("GUIControl::set_SolidBackground"),
        ScMethod<&GUIControl_GetBlendMode>("GUIControl::get_BlendMode"),
        ScMethod<&GUIControl_SetBlendMode>("GUIControl::set_BlendMode"),
        ScMethod<&GUIControl_GetShader>("GUIControl::get_Shader"),
        ScMethod<&GUIControl_SetShader>("GUIControl::set_Shader"),
    };

    ccAddExternalFunctions(guicontrol_api);
}
