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

#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/cc_gui.h"
#include "ac/dynobj/cc_guicontrol.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/dynobj/scriptuserobject.h"
#include "ac/game.h"
#include "ac/gui.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_GUI_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptGUI, ccDynamicGUI, GUI_GetByName, const char);
}

RuntimeScriptValue Sc_GUI_GetActiveTextInputControl(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(GUITextBox, ccDynamicGUIControl, GUI_GetActiveTextInputControl);
}

// void GUI_Centre(ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_Centre(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptGUI, GUI_Centre);
}

RuntimeScriptValue Sc_GUI_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptGUI, ccDynamicGUI, GUI_GetAtScreenXY2);
}

RuntimeScriptValue Sc_GUI_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptGUI, ccDynamicGUI, GUI_GetAtScreenXY);
}

// void (ScriptGUI *sgui, int x, int y)
RuntimeScriptValue Sc_GUI_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetPosition);
}

// void (ScriptGUI *sgui, int width, int height)
RuntimeScriptValue Sc_GUI_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptGUI, GUI_SetSize);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundGraphic);
}

// void (ScriptGUI *sgui, int sprite)
RuntimeScriptValue Sc_GUI_SetBackgroundGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundGraphic);
}

RuntimeScriptValue Sc_GUI_GetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_SetBackgroundColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBackgroundColor);
}

RuntimeScriptValue Sc_GUI_GetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBorderColor);
}

RuntimeScriptValue Sc_GUI_SetBorderColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBorderColor);
}

RuntimeScriptValue Sc_GUI_GetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextColor);
}

RuntimeScriptValue Sc_GUI_SetTextColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextColor);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetClickable);
}

// void (ScriptGUI *sgui, int clickable)
RuntimeScriptValue Sc_GUI_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetClickable);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetControlCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetControlCount);
}

// GUIControl* (ScriptGUI *sgui, int idx)
RuntimeScriptValue Sc_GUI_GetiControls(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptGUI, GUIControl, ccDynamicGUIControl, GUI_GetiControls);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetHeight);
}

// void (ScriptGUI *sgui, int new_height)
RuntimeScriptValue Sc_GUI_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetHeight);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetID);
}

RuntimeScriptValue Sc_GUI_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, const char, myScriptStringImpl, GUI_GetScriptName);
}

RuntimeScriptValue Sc_GUI_GetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupYPos);
}

RuntimeScriptValue Sc_GUI_SetPopupYPos(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetPopupYPos);
}

RuntimeScriptValue Sc_GUI_GetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTextPadding);
}

RuntimeScriptValue Sc_GUI_SetTextPadding(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTextPadding);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetOpacity);
}

// void (ScriptGUI *sgui, int alpha)
RuntimeScriptValue Sc_GUI_SetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetOpacity);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTransparency);
}

// void (ScriptGUI *sgui, int trans)
RuntimeScriptValue Sc_GUI_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetTransparency);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetVisible);
}

// void (ScriptGUI *sgui, int is_visible)
RuntimeScriptValue Sc_GUI_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetVisible);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetWidth);
}

// void (ScriptGUI *sgui, int new_width)
RuntimeScriptValue Sc_GUI_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetWidth);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetX);
}

// void (ScriptGUI *sgui, int x)
RuntimeScriptValue Sc_GUI_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetX);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetY);
}

// void (ScriptGUI *sgui, int y)
RuntimeScriptValue Sc_GUI_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetY);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetZOrder);
}

// void (ScriptGUI *sgui, int z)
RuntimeScriptValue Sc_GUI_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetZOrder);
}

RuntimeScriptValue Sc_GUI_GetGraphicPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, void, globalDynamicArray, GUI_GetGraphicPosition);
}

RuntimeScriptValue Sc_GUI_GetGraphicBoundBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, void, globalDynamicArray, GUI_GetGraphicBoundBox);
}

RuntimeScriptValue Sc_GUI_AsTextWindow(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptGUI, ScriptGUI, ccDynamicGUI, GUI_AsTextWindow);
}

RuntimeScriptValue Sc_GUI_GetPopupStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetPopupStyle);
}

RuntimeScriptValue Sc_GUI_SetPopupStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetPopupStyle);
}

RuntimeScriptValue Sc_GUI_Click(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_Click);
}

RuntimeScriptValue Sc_GUI_ProcessClick(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(GUI_ProcessClick);
}

RuntimeScriptValue Sc_GUI_GetShown(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptGUI, GUI_GetShown);
}

// int (ScriptGUI *sgui)
RuntimeScriptValue Sc_GUI_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBlendMode);
}

// void (ScriptGUI *sgui, int blend_mode)
RuntimeScriptValue Sc_GUI_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptGUI, GUI_SetBlendMode);
}

RuntimeScriptValue Sc_GUI_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptGUI, ScriptShaderInstance, GUI_GetShader);
}

RuntimeScriptValue Sc_GUI_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptGUI, GUI_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_GUI_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetRotation);
}

RuntimeScriptValue Sc_GUI_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetRotation);
}

RuntimeScriptValue Sc_GUI_GetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetScaleX);
}

RuntimeScriptValue Sc_GUI_SetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetScaleX);
}

RuntimeScriptValue Sc_GUI_GetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptGUI, GUI_GetScaleY);
}

RuntimeScriptValue Sc_GUI_SetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptGUI, GUI_SetScaleY);
}

RuntimeScriptValue Sc_GUI_SetScale(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT2(ScriptGUI, GUI_SetScale);
}

RuntimeScriptValue Sc_GUI_ScreenToGUIPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptGUI, ScriptUserObject, GUI_ScreenToGUIPoint);
}

RuntimeScriptValue Sc_GUI_GUIToScreenPoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO_PINT2_PBOOL(ScriptGUI, ScriptUserObject, GUI_GUIToScreenPoint);
}

RuntimeScriptValue Sc_GUI_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptGUI, GUI_GetProperty, const char);
}

RuntimeScriptValue Sc_GUI_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptGUI, const char, myScriptStringImpl, GUI_GetTextProperty, const char);
}

RuntimeScriptValue Sc_GUI_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptGUI, GUI_SetProperty, const char);
}

RuntimeScriptValue Sc_GUI_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptGUI, GUI_SetTextProperty, const char, const char);
}

RuntimeScriptValue Sc_GUI_GetLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetLeftGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopLeftGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopGraphic);
}

RuntimeScriptValue Sc_GUI_GetTopRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetTopRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomRightGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomRightGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomGraphic);
}

RuntimeScriptValue Sc_GUI_GetBottomLeftGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptGUI, GUI_GetBottomLeftGraphic);
}


void RegisterGUIAPI()
{
    ScFnRegister gui_api[] = {
        {"GUI::GetAtScreenXY^2", API_FN_PAIR(GUI_GetAtScreenXY2)},
        {"GUI::GetAtScreenXY^3", API_FN_PAIR(GUI_GetAtScreenXY)},
        // [OBSOLETE] GetByName => GetByScriptName
        {"GUI::GetByName^1", API_FN_PAIR(GUI_GetByName)},
        {"GUI::GetByScriptName^1", API_FN_PAIR(GUI_GetByName)},
        {"GUI::GetActiveTextInputControl^0", API_FN_PAIR(GUI_GetActiveTextInputControl)},
        {"GUI::ProcessClick^3", API_FN_PAIR(GUI_ProcessClick)},
        {"GUI::ScreenToGUIPoint", API_FN_PAIR(GUI_ScreenToGUIPoint)},
        {"GUI::GUIToScreenPoint", API_FN_PAIR(GUI_GUIToScreenPoint)},

        {"GUI::Centre^0", API_FN_PAIR(GUI_Centre)},
        {"GUI::Click^1", API_FN_PAIR(GUI_Click)},
        {"GUI::SetPosition^2", API_FN_PAIR(GUI_SetPosition)},
        {"GUI::SetSize^2", API_FN_PAIR(GUI_SetSize)},
        {"GUI::GetGraphicPosition^0", API_FN_PAIR(GUI_GetGraphicPosition)},
        {"GUI::GetGraphicBoundBox^0", API_FN_PAIR(GUI_GetGraphicBoundBox)},
        {"GUI::GetProperty^1", API_FN_PAIR(GUI_GetProperty)},
        {"GUI::GetTextProperty^1", API_FN_PAIR(GUI_GetTextProperty)},
        {"GUI::SetProperty^2", API_FN_PAIR(GUI_SetProperty)},
        {"GUI::SetTextProperty^2", API_FN_PAIR(GUI_SetTextProperty)},
        {"GUI::get_BackgroundGraphic", API_FN_PAIR(GUI_GetBackgroundGraphic)},
        {"GUI::set_BackgroundGraphic", API_FN_PAIR(GUI_SetBackgroundGraphic)},
        {"GUI::get_BackgroundColor", API_FN_PAIR(GUI_GetBackgroundColor)},
        {"GUI::set_BackgroundColor", API_FN_PAIR(GUI_SetBackgroundColor)},
        {"GUI::get_BorderColor", API_FN_PAIR(GUI_GetBorderColor)},
        {"GUI::set_BorderColor", API_FN_PAIR(GUI_SetBorderColor)},
        {"GUI::get_Clickable", API_FN_PAIR(GUI_GetClickable)},
        {"GUI::set_Clickable", API_FN_PAIR(GUI_SetClickable)},
        {"GUI::get_ControlCount", API_FN_PAIR(GUI_GetControlCount)},
        {"GUI::geti_Controls", API_FN_PAIR(GUI_GetiControls)},
        {"GUI::get_Height", API_FN_PAIR(GUI_GetHeight)},
        {"GUI::set_Height", API_FN_PAIR(GUI_SetHeight)},
        {"GUI::get_ID", API_FN_PAIR(GUI_GetID)},
        {"GUI::get_AsTextWindow", API_FN_PAIR(GUI_AsTextWindow)},
        {"GUI::get_PopupStyle", API_FN_PAIR(GUI_GetPopupStyle)},
        {"GUI::set_PopupStyle", API_FN_PAIR(GUI_SetPopupStyle)},
        {"GUI::get_PopupYPos", API_FN_PAIR(GUI_GetPopupYPos)},
        {"GUI::set_PopupYPos", API_FN_PAIR(GUI_SetPopupYPos)},
        {"GUI::get_ScriptName", API_FN_PAIR(GUI_GetScriptName)},
        {"GUI::get_Opacity", API_FN_PAIR(GUI_GetOpacity)},
        {"GUI::set_Opacity", API_FN_PAIR(GUI_SetOpacity)},
        // [OBSOLETE] Transparency => Opacity
        {"GUI::get_Transparency", API_FN_PAIR(GUI_GetTransparency)},
        {"GUI::set_Transparency", API_FN_PAIR(GUI_SetTransparency)},
        {"GUI::get_Visible", API_FN_PAIR(GUI_GetVisible)},
        {"GUI::set_Visible", API_FN_PAIR(GUI_SetVisible)},
        {"GUI::get_Width", API_FN_PAIR(GUI_GetWidth)},
        {"GUI::set_Width", API_FN_PAIR(GUI_SetWidth)},
        {"GUI::get_X", API_FN_PAIR(GUI_GetX)},
        {"GUI::set_X", API_FN_PAIR(GUI_SetX)},
        {"GUI::get_Y", API_FN_PAIR(GUI_GetY)},
        {"GUI::set_Y", API_FN_PAIR(GUI_SetY)},
        {"GUI::get_ZOrder", API_FN_PAIR(GUI_GetZOrder)},
        {"GUI::set_ZOrder", API_FN_PAIR(GUI_SetZOrder)},
        {"GUI::get_Shown", API_FN_PAIR(GUI_GetShown)},

        {"GUI::get_BlendMode", API_FN_PAIR(GUI_GetBlendMode)},
        {"GUI::set_BlendMode", API_FN_PAIR(GUI_SetBlendMode)},
        {"GUI::get_Rotation", API_FN_PAIR(GUI_GetRotation)},
        {"GUI::set_Rotation", API_FN_PAIR(GUI_SetRotation)},
        {"GUI::get_ScaleX", API_FN_PAIR(GUI_GetScaleX)},
        {"GUI::set_ScaleX", API_FN_PAIR(GUI_SetScaleX)},
        {"GUI::get_ScaleY", API_FN_PAIR(GUI_GetScaleY)},
        {"GUI::set_ScaleY", API_FN_PAIR(GUI_SetScaleY)},
        {"GUI::SetScale", API_FN_PAIR(GUI_SetScale)},

        {"GUI::get_Shader", API_FN_PAIR(GUI_GetShader)},
        {"GUI::set_Shader", API_FN_PAIR(GUI_SetShader)},

        {"TextWindowGUI::get_TextColor", API_FN_PAIR(GUI_GetTextColor)},
        {"TextWindowGUI::set_TextColor", API_FN_PAIR(GUI_SetTextColor)},
        {"TextWindowGUI::get_TextPadding", API_FN_PAIR(GUI_GetTextPadding)},
        {"TextWindowGUI::set_TextPadding", API_FN_PAIR(GUI_SetTextPadding)},
        {"TextWindowGUI::get_LeftGraphic", API_FN_PAIR(GUI_GetLeftGraphic)},
        {"TextWindowGUI::get_TopLeftGraphic", API_FN_PAIR(GUI_GetTopLeftGraphic)},
        {"TextWindowGUI::get_TopGraphic", API_FN_PAIR(GUI_GetTopGraphic)},
        {"TextWindowGUI::get_TopRightGraphic", API_FN_PAIR(GUI_GetTopRightGraphic)},
        {"TextWindowGUI::get_RightGraphic", API_FN_PAIR(GUI_GetRightGraphic)},
        {"TextWindowGUI::get_BottomRightGraphic", API_FN_PAIR(GUI_GetBottomRightGraphic)},
        {"TextWindowGUI::get_BottomGraphic", API_FN_PAIR(GUI_GetBottomGraphic)},
        {"TextWindowGUI::get_BottomLeftGraphic", API_FN_PAIR(GUI_GetBottomLeftGraphic)},
    };

    ccAddExternalFunctions(gui_api);
}
