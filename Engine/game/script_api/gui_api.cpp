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
// Script bindings registered by RegisterGUIAPI().
//
//=============================================================================
#include <numeric>

#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/ui/gui.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/script_user_object.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterGUIAPI()
{
    ScFnRegister gui_api[] = {
        ScStatic<&GUI_GetAtScreenXY2>("GUI::GetAtScreenXY^2"),
        ScStatic<&GUI_GetAtScreenXY>("GUI::GetAtScreenXY^3"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&GUI_GetByName>("GUI::GetByName^1"),
        ScStatic<&GUI_GetByName>("GUI::GetByScriptName^1"),
        ScStatic<&GUI_GetActiveTextInputControl>("GUI::GetActiveTextInputControl^0"),
        ScStatic<&GUI_ProcessClick>("GUI::ProcessClick^3"),
        ScMethod<&GUI_ScreenToGUIPoint>("GUI::ScreenToGUIPoint"),
        ScMethod<&GUI_GUIToScreenPoint>("GUI::GUIToScreenPoint"),

        ScMethod<&GUI_Centre>("GUI::Centre^0"),
        ScMethod<&GUI_Click>("GUI::Click^1"),
        ScMethod<&GUI_SetPosition>("GUI::SetPosition^2"),
        ScMethod<&GUI_SetSize>("GUI::SetSize^2"),
        ScMethod<&GUI_GetGraphicPosition, ScManagedBy<globalDynamicArray>>("GUI::GetGraphicPosition^0"),
        ScMethod<&GUI_GetGraphicBoundBox, ScManagedBy<globalDynamicArray>>("GUI::GetGraphicBoundBox^0"),
        ScMethod<&GUI_GetProperty>("GUI::GetProperty^1"),
        ScMethod<&GUI_GetTextProperty>("GUI::GetTextProperty^1"),
        ScMethod<&GUI_SetProperty>("GUI::SetProperty^2"),
        ScMethod<&GUI_SetTextProperty>("GUI::SetTextProperty^2"),
        ScMethod<&GUI_GetBackgroundGraphic>("GUI::get_BackgroundGraphic"),
        ScMethod<&GUI_SetBackgroundGraphic>("GUI::set_BackgroundGraphic"),
        ScMethod<&GUI_GetBackgroundColor>("GUI::get_BackgroundColor"),
        ScMethod<&GUI_SetBackgroundColor>("GUI::set_BackgroundColor"),
        ScMethod<&GUI_GetBorderColor>("GUI::get_BorderColor"),
        ScMethod<&GUI_SetBorderColor>("GUI::set_BorderColor"),
        ScMethod<&GUI_GetClickable>("GUI::get_Clickable"),
        ScMethod<&GUI_SetClickable>("GUI::set_Clickable"),
        ScMethod<&GUI_GetControlCount>("GUI::get_ControlCount"),
        ScMethod<&GUI_GetiControls>("GUI::geti_Controls"),
        ScMethod<&GUI_GetHeight>("GUI::get_Height"),
        ScMethod<&GUI_SetHeight>("GUI::set_Height"),
        ScMethod<&GUI_GetID>("GUI::get_ID"),
        ScMethod<&GUI_AsTextWindow>("GUI::get_AsTextWindow"),
        ScMethod<&GUI_GetPopupStyle>("GUI::get_PopupStyle"),
        ScMethod<&GUI_SetPopupStyle>("GUI::set_PopupStyle"),
        ScMethod<&GUI_GetPopupYPos>("GUI::get_PopupYPos"),
        ScMethod<&GUI_SetPopupYPos>("GUI::set_PopupYPos"),
        ScMethod<&GUI_GetScriptName>("GUI::get_ScriptName"),
        ScMethod<&GUI_GetOpacity>("GUI::get_Opacity"),
        ScMethod<&GUI_SetOpacity>("GUI::set_Opacity"),
        // [OBSOLETE] Transparency => Opacity
        ScMethod<&GUI_GetTransparency>("GUI::get_Transparency"),
        ScMethod<&GUI_SetTransparency>("GUI::set_Transparency"),
        ScMethod<&GUI_GetVisible>("GUI::get_Visible"),
        ScMethod<&GUI_SetVisible>("GUI::set_Visible"),
        ScMethod<&GUI_GetWidth>("GUI::get_Width"),
        ScMethod<&GUI_SetWidth>("GUI::set_Width"),
        ScMethod<&GUI_GetX>("GUI::get_X"),
        ScMethod<&GUI_SetX>("GUI::set_X"),
        ScMethod<&GUI_GetY>("GUI::get_Y"),
        ScMethod<&GUI_SetY>("GUI::set_Y"),
        ScMethod<&GUI_GetZOrder>("GUI::get_ZOrder"),
        ScMethod<&GUI_SetZOrder>("GUI::set_ZOrder"),
        ScMethod<&GUI_GetShown>("GUI::get_Shown"),

        ScMethod<&GUI_GetBlendMode>("GUI::get_BlendMode"),
        ScMethod<&GUI_SetBlendMode>("GUI::set_BlendMode"),
        ScMethod<&GUI_GetRotation>("GUI::get_Rotation"),
        ScMethod<&GUI_SetRotation, ScUndefinedResult>("GUI::set_Rotation"),
        ScMethod<&GUI_GetScaleX>("GUI::get_ScaleX"),
        ScMethod<&GUI_SetScaleX, ScUndefinedResult>("GUI::set_ScaleX"),
        ScMethod<&GUI_GetScaleY>("GUI::get_ScaleY"),
        ScMethod<&GUI_SetScaleY, ScUndefinedResult>("GUI::set_ScaleY"),
        ScMethod<&GUI_SetScale, ScUndefinedResult>("GUI::SetScale"),

        ScMethod<&GUI_GetShader>("GUI::get_Shader"),
        ScMethod<&GUI_SetShader>("GUI::set_Shader"),

        ScMethod<&GUI_GetTextColor>("TextWindowGUI::get_TextColor"),
        ScMethod<&GUI_SetTextColor>("TextWindowGUI::set_TextColor"),
        ScMethod<&GUI_GetTextPadding>("TextWindowGUI::get_TextPadding"),
        ScMethod<&GUI_SetTextPadding>("TextWindowGUI::set_TextPadding"),
        ScMethod<&GUI_GetLeftGraphic>("TextWindowGUI::get_LeftGraphic"),
        ScMethod<&GUI_GetTopLeftGraphic>("TextWindowGUI::get_TopLeftGraphic"),
        ScMethod<&GUI_GetTopGraphic>("TextWindowGUI::get_TopGraphic"),
        ScMethod<&GUI_GetTopRightGraphic>("TextWindowGUI::get_TopRightGraphic"),
        ScMethod<&GUI_GetRightGraphic>("TextWindowGUI::get_RightGraphic"),
        ScMethod<&GUI_GetBottomRightGraphic>("TextWindowGUI::get_BottomRightGraphic"),
        ScMethod<&GUI_GetBottomGraphic>("TextWindowGUI::get_BottomGraphic"),
        ScMethod<&GUI_GetBottomLeftGraphic>("TextWindowGUI::get_BottomLeftGraphic"),
    };

    ccAddExternalFunctions(gui_api);
}
