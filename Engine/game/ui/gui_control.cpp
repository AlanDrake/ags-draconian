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
#include "game/ui/gui_control.h"

#include <vector>

#include "debug/debug_log.h"
#include "game/dynobj/cc_gui.h"
#include "game/dynobj/cc_gui_control.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/world/object.h"
#include "game/world/properties.h"
#include "gfx/gfx_def.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h"

using namespace AGS::Common;

GUIObject *GUIControl_GetAtScreenXY(int x, int y, int gui_hitoptions, int ctrl_hitoptions)
{
    Game &game = Game::Current();
    int gui_id = GetGUIAt(x, y, gui_hitoptions);
    if (gui_id == -1)
    {
        return nullptr;
    }

    int result = game.Guis[gui_id].FindControlAt(x, y, 0, (ctrl_hitoptions & kHit_Interactable));
    if (result < 0)
    {
        return nullptr;
    }

    return game.Guis[gui_id].GetControl(result);
}

GUIObject *GUIControl_GetAtScreenXY2(int x, int y)
{
    // NOTE: historically GUIControl.GetAtScreenXY did not require controls to be clickable (only guis)
    return GUIControl_GetAtScreenXY(x, y, kHit_Interactable, kHit_Any);
}

int GUIControl_GetVisible(GUIControl *control)
{
    return control->IsVisible();
}

void GUIControl_SetVisible(GUIControl *control, int visible)
{
    const bool on = visible != 0;
    if (on != control->IsVisible())
    {
        control->SetVisible(on);
    }
}

int GUIControl_GetClickable(GUIControl *control)
{
    if (control->IsClickable())
    {
        return 1;
    }
    return 0;
}

void GUIControl_SetClickable(GUIControl *control, int enabled)
{
    const bool on = enabled != 0;
    if (on != control->IsClickable())
    {
        control->SetClickable(on);
    }
}

int GUIControl_GetEnabled(GUIControl *control)
{
    return control->IsEnabled() ? 1 : 0;
}

void GUIControl_SetEnabled(GUIControl *control, int enabled)
{
    const bool on = enabled != 0;
    if (on != control->IsEnabled())
    {
        control->SetEnabled(on);
    }
}


int GUIControl_GetID(GUIControl *control)
{
    return control->GetID();
}

const char *GUIControl_GetScriptName(GUIControl *control)
{
    return CreateNewScriptString(control->GetName());
}

ScriptGUI *GUIControl_GetOwningGUI(GUIControl *control)
{
    return &Game::Current().ScGuis[control->GetParentID()];
}

GUIButton *GUIControl_GetAsButton(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUIButton)
    {
        return nullptr;
    }

    return static_cast<GUIButton *>(control);
}

GUIInvWindow *GUIControl_GetAsInvWindow(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUIInvWindow)
    {
        return nullptr;
    }

    return static_cast<GUIInvWindow *>(control);
}

GUILabel *GUIControl_GetAsLabel(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUILabel)
    {
        return nullptr;
    }

    return static_cast<GUILabel *>(control);
}

GUIListBox *GUIControl_GetAsListBox(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUIListBox)
    {
        return nullptr;
    }

    return static_cast<GUIListBox *>(control);
}

GUISlider *GUIControl_GetAsSlider(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUISlider)
    {
        return nullptr;
    }

    return static_cast<GUISlider *>(control);
}

GUITextBox *GUIControl_GetAsTextBox(GUIControl *control)
{
    if (Game::Current().Guis[control->GetParentID()].GetControlType(control->GetID()) != kGUITextBox)
    {
        return nullptr;
    }

    return static_cast<GUITextBox *>(control);
}

int GUIControl_GetX(GUIControl *control)
{
    return control->GetX();
}

void GUIControl_SetX(GUIControl *control, int x)
{
    control->SetX(x);
}

int GUIControl_GetY(GUIControl *control)
{
    return control->GetY();
}

void GUIControl_SetY(GUIControl *control, int y)
{
    control->SetY(y);
}

float GUIControl_GetRotation(GUIControl *control)
{
    return control->GetRotation();
}

void GUIControl_SetRotation(GUIControl *control, float rotation)
{
    control->SetRotation(rotation);
}

float GUIControl_GetScaleX(GUIControl *control)
{
    return control->GetScale().X;
}

void GUIControl_SetScaleX(GUIControl *control, float scalex)
{
    control->SetScale(scalex, control->GetScale().Y);
}

float GUIControl_GetScaleY(GUIControl *control)
{
    return control->GetScale().Y;
}

void GUIControl_SetScaleY(GUIControl *control, float scaley)
{
    control->SetScale(control->GetScale().X, scaley);
}

void GUIControl_SetScale(GUIControl *control, float scalex, float scaley)
{
    control->SetScale(scalex, scaley);
}

int GUIControl_GetZOrder(GUIControl *control)
{
    return control->GetZOrder();
}

void GUIControl_SetZOrder(GUIControl *control, int zorder)
{
    Game::Current().Guis[control->GetParentID()].SetControlZOrder(control->GetID(), zorder);
}

void GUIControl_SetPosition(GUIControl *control, int x, int y)
{
    GUIControl_SetX(control, x);
    GUIControl_SetY(control, y);
}


int GUIControl_GetWidth(GUIControl *control)
{
    return control->GetWidth();
}

void GUIControl_SetWidth(GUIControl *control, int new_width)
{
    control->SetWidth(new_width);
}

int GUIControl_GetHeight(GUIControl *control)
{
    return control->GetHeight();
}

void GUIControl_SetHeight(GUIControl *control, int new_height)
{
    control->SetHeight(new_height);
}

void GUIControl_SetSize(GUIControl *control, int new_width, int new_height)
{
    if ((new_width < 2) || (new_height < 2))
    {
        quit("!SetGUIObjectSize: new size is too small (must be at least 2x2)");
    }

    debug_script_log("SetGUIObject %d,%d size %d,%d", control->GetParentID(), control->GetID(), new_width, new_height);
    GUIControl_SetWidth(control, new_width);
    GUIControl_SetHeight(control, new_height);
}

void *GUIControl_GetGraphicPosition(GUIControl *control)
{
    control->UpdateGraphicSpace();
    std::vector<Point> points;
    control->GetGraphicSpace().GetTransformedCorners(points, control->GetSize());
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void *GUIControl_GetGraphicBoundBox(GUIControl *control)
{
    control->UpdateGraphicSpace();
    std::vector<Point> points;
    control->GetGraphicSpace().GetAABBPoints(points);
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void GUIControl_SendToBack(GUIControl *control)
{
    Game::Current().Guis[control->GetParentID()].SendControlToBack(control->GetID());
}

void GUIControl_BringToFront(GUIControl *control)
{
    Game::Current().Guis[control->GetParentID()].BringControlToFront(control->GetID());
}

int GUIControl_GetBackgroundColor(GUIControl *control)
{
    return control->GetBackColor();
}

void GUIControl_SetBackgroundColor(GUIControl *control, int color)
{
    control->SetBackColor(color);
}

int GUIControl_GetBorderColor(GUIControl *control)
{
    return control->GetBorderColor();
}

void GUIControl_SetBorderColor(GUIControl *control, int color)
{
    control->SetBorderColor(color);
}

int GUIControl_GetBorderWidth(GUIControl *control)
{
    return control->GetBorderWidth();
}

void GUIControl_SetBorderWidth(GUIControl *control, int size)
{
    control->SetBorderWidth(size);
}

int GUIControl_GetPaddingX(GUIControl *control)
{
    return control->GetPaddingX();
}

void GUIControl_SetPaddingX(GUIControl *control, int padding)
{
    control->SetPaddingX(padding);
}

int GUIControl_GetPaddingY(GUIControl *control)
{
    return control->GetPaddingY();
}

void GUIControl_SetPaddingY(GUIControl *control, int padding)
{
    control->SetPaddingY(padding);
}

bool GUIControl_GetShowBorder(GUIControl *control)
{
    return control->IsShowBorder();
}

void GUIControl_SetShowBorder(GUIControl *control, bool on)
{
    control->SetShowBorder(on);
}

bool GUIControl_GetSolidBackground(GUIControl *control)
{
    return control->IsSolidBackground();
}

void GUIControl_SetSolidBackground(GUIControl *control, bool on)
{
    control->SetSolidBackground(on);
}

bool GUIControl_GetTranslated(GUIControl *control)
{
    return control->IsTranslated();
}

void GUIControl_SetTranslated(GUIControl *control, bool is_translated)
{
    control->SetTranslated(is_translated);
}

int GUIControl_GetOpacity(GUIControl *control)
{
    return control->GetOpacity();
}

void GUIControl_SetOpacity(GUIControl *control, int alpha)
{
    if ((alpha < 0) || (alpha > 255))
    {
        quit("!GUIControl.Opacity: alpha value must be between 0 and 255");
    }
    control->SetOpacity(alpha);
}

// [OBSOLETE] the AGS 4.0 Transparency, for compiled 4.0 games
int GUIControl_GetTransparency(GUIControl *control)
{
    return GfxDef::OpacityToTrans100(control->GetOpacity());
}

void GUIControl_SetTransparency(GUIControl *control, int trans)
{
    if ((trans < 0) || (trans > 100))
        quit("!SetGUITransparency: transparency value must be between 0 and 100");
    control->SetOpacity(GfxDef::Trans100ToOpacity(trans));
}

int GUIControl_GetBlendMode(GUIControl *control)
{
    return control->GetBlendMode();
}

void GUIControl_SetBlendMode(GUIControl *control, int blend_mode)
{
    control->SetBlendMode(ValidateBlendMode("GUIControl.BlendMode", control->GetName().GetCStr(), blend_mode));
}

int GUIControl_GetProperty(GUIControl *control, const char *property)
{
    Game &game = Game::Current();
    int ctrl_type = game.Guis[control->GetParentID()].GetControlType(control->GetID());
    int ctrl_id = game.Guis[control->GetParentID()].GetControlID(control->GetID());
    return get_int_property(game.Def().GuiControlProperties[ctrl_type][ctrl_id],
                            game.Play.GuiControlProperties[ctrl_type][ctrl_id], property);
}

const char *GUIControl_GetTextProperty(GUIControl *control, const char *property)
{
    Game &game = Game::Current();
    int ctrl_type = game.Guis[control->GetParentID()].GetControlType(control->GetID());
    int ctrl_id = game.Guis[control->GetParentID()].GetControlID(control->GetID());
    return get_text_property_dynamic_string(game.Def().GuiControlProperties[ctrl_type][ctrl_id],
                                            game.Play.GuiControlProperties[ctrl_type][ctrl_id], property);
}

bool GUIControl_SetProperty(GUIControl *control, const char *property, int value)
{
    Game &game = Game::Current();
    int ctrl_type = game.Guis[control->GetParentID()].GetControlType(control->GetID());
    int ctrl_id = game.Guis[control->GetParentID()].GetControlID(control->GetID());
    return set_int_property(game.Play.GuiControlProperties[ctrl_type][ctrl_id], property, value);
}

bool GUIControl_SetTextProperty(GUIControl *control, const char *property, const char *value)
{
    Game &game = Game::Current();
    int ctrl_type = game.Guis[control->GetParentID()].GetControlType(control->GetID());
    int ctrl_id = game.Guis[control->GetParentID()].GetControlID(control->GetID());
    return set_text_property(game.Play.GuiControlProperties[ctrl_type][ctrl_id], property, value);
}

ScriptShaderInstance *GUIControl_GetShader(GUIControl *control)
{
    return static_cast<ScriptShaderInstance *>(ccGetObjectAddressFromHandle(control->GetShaderHandle()));
}

void GUIControl_SetShader(GUIControl *control, ScriptShaderInstance *shader_inst)
{
    control->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                       ccReplaceObjectHandle(control->GetShaderHandle(), shader_inst));
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"


GUIControl *GUIControl_GetByName(const char *name)
{
    // A control script name is imported with the manager of the control's own type, never the base GUIControl
    // one, so the object must be of one of these six.
    const static std::vector<String> typenames = {ccDynamicGUIButton.GetType(), ccDynamicGUIInvWindow.GetType(),
                                                  ccDynamicGUILabel.GetType(),  ccDynamicGUIListBox.GetType(),
                                                  ccDynamicGUISlider.GetType(), ccDynamicGUITextBox.GetType()};
    return static_cast<GUIControl *>(ccGetScriptObjectAddress(name, typenames));
}
