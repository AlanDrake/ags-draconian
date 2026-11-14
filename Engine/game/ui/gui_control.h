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
//
//
//=============================================================================
#pragma once

#include "game/dynobj/script_objects.h"
#include "gui/guibutton.h"
#include "gui/guicontrol.h"
#include "gui/guiinv.h"
#include "gui/guilabel.h"
#include "gui/guilistbox.h"
#include "gui/guislider.h"
#include "gui/guitextbox.h"

using AGS::Common::GUIControl;
using AGS::Common::GUIButton;
using AGS::Common::GUIInvWindow;
using AGS::Common::GUILabel;
using AGS::Common::GUIListBox;
using AGS::Common::GUISlider;
using AGS::Common::GUITextBox;

// Gets the control at the given screen coordinates, or nullptr if there is none. The GUI is found by GetGUIAt and then
// its topmost visible control at the point; with kHit_Interactable in gui_hitoptions only a GUI that can be interacted
// with counts, and in ctrl_hitoptions only a clickable control.
AGS::Common::GUIObject *GUIControl_GetAtScreenXY(int x, int y, int gui_hitoptions, int ctrl_hitoptions);

// **** GUICONTROL: FUNCTIONS ****
//
// A GUIControl_<Name> function implements the script function GUIControl.<Name> of the same name, which scripts reach
// through the binding registered in RegisterGUIControlAPI (gui_control_api.cpp); a pair
// GUIControl_Get<Name> and GUIControl_Set<Name> implements the script property GUIControl.<Name> (get_<Name> and
// set_<Name>). control is the script control the function is called on. A comment tells where the script name differs.
// The setters of Visible, Clickable and Enabled take any non-zero value as "on".

// GUIControl.Visible: the control's own visibility flag.
int GUIControl_GetVisible(GUIControl *control);
void GUIControl_SetVisible(GUIControl *control, int visible);
// GUIControl.Clickable: whether the control reacts to the mouse; the getter returns 1 or 0.
int GUIControl_GetClickable(GUIControl *control);
void GUIControl_SetClickable(GUIControl *control, int enabled);
// GUIControl.Enabled: the control's own enabled flag.
int GUIControl_GetEnabled(GUIControl *control);
void GUIControl_SetEnabled(GUIControl *control, int enabled);
// GUIControl.ID: the control's index in its GUI, which is not the same as the order of drawing (ZOrder).
int GUIControl_GetID(GUIControl *control);
// GUIControl.OwningGUI: the script object of the GUI the control belongs to.
ScriptGUI *GUIControl_GetOwningGUI(GUIControl *control);
// GUIControl.AsButton, AsInvWindow, AsLabel, AsListBox, AsSlider, AsTextBox: the control as the more specific type,
// or nullptr if it is of another type.
GUIButton *GUIControl_GetAsButton(GUIControl *control);
GUIInvWindow *GUIControl_GetAsInvWindow(GUIControl *control);
GUILabel *GUIControl_GetAsLabel(GUIControl *control);
GUIListBox *GUIControl_GetAsListBox(GUIControl *control);
GUISlider *GUIControl_GetAsSlider(GUIControl *control);
GUITextBox *GUIControl_GetAsTextBox(GUIControl *control);
// GUIControl.X and Y: the control's position in its GUI, in the GUI's own coordinates.
int GUIControl_GetX(GUIControl *control);
void GUIControl_SetX(GUIControl *control, int x);
int GUIControl_GetY(GUIControl *control);
void GUIControl_SetY(GUIControl *control, int y);
// GUIControl.ZOrder: the control's place in the drawing order of its GUI; setting it reorders the GUI's controls.
int GUIControl_GetZOrder(GUIControl *control);
void GUIControl_SetZOrder(GUIControl *control, int zorder);
// GUIControl.SetPosition: sets X and Y.
void GUIControl_SetPosition(GUIControl *control, int x, int y);
// GUIControl.Width and Height, SetSize: the control's size; SetSize is a script error that ends the game if either
// side is less than 2, the setters of one side do not check.
int GUIControl_GetWidth(GUIControl *control);
void GUIControl_SetWidth(GUIControl *control, int new_width);
int GUIControl_GetHeight(GUIControl *control);
void GUIControl_SetHeight(GUIControl *control, int new_height);
void GUIControl_SetSize(GUIControl *control, int new_width, int new_height);
// GUIControl.SendToBack and BringToFront: move the control to the bottom or the top of its GUI's drawing order.
void GUIControl_SendToBack(GUIControl *control);
void GUIControl_BringToFront(GUIControl *control);

class ScriptShaderInstance;

// Script API: GUIControl::GetAtScreenXY^2
AGS::Common::GUIObject *GUIControl_GetAtScreenXY2(int x, int y);

// Script API: GUIControl::get_ScriptName
const char *GUIControl_GetScriptName(GUIControl *control);

// Script API: GUIControl::get_Rotation
float GUIControl_GetRotation(GUIControl *control);

// Script API: GUIControl::set_Rotation
void GUIControl_SetRotation(GUIControl *control, float rotation);

// Script API: GUIControl::get_ScaleX
float GUIControl_GetScaleX(GUIControl *control);

// Script API: GUIControl::set_ScaleX
void GUIControl_SetScaleX(GUIControl *control, float scalex);

// Script API: GUIControl::get_ScaleY
float GUIControl_GetScaleY(GUIControl *control);

// Script API: GUIControl::set_ScaleY
void GUIControl_SetScaleY(GUIControl *control, float scaley);

// Script API: GUIControl::SetScale
void GUIControl_SetScale(GUIControl *control, float scalex, float scaley);

// Script API: GUIControl::GetGraphicPosition^0
void *GUIControl_GetGraphicPosition(GUIControl *control);

// Script API: GUIControl::GetGraphicBoundBox^0
void *GUIControl_GetGraphicBoundBox(GUIControl *control);

// Script API: GUIControl::get_BackgroundColor
int GUIControl_GetBackgroundColor(GUIControl *control);

// Script API: GUIControl::set_BackgroundColor
void GUIControl_SetBackgroundColor(GUIControl *control, int color);

// Script API: GUIControl::get_BorderColor
int GUIControl_GetBorderColor(GUIControl *control);

// Script API: GUIControl::set_BorderColor
void GUIControl_SetBorderColor(GUIControl *control, int color);

// Script API: GUIControl::get_BorderWidth
int GUIControl_GetBorderWidth(GUIControl *control);

// Script API: GUIControl::set_BorderWidth
void GUIControl_SetBorderWidth(GUIControl *control, int size);

// Script API: GUIControl::get_PaddingX
int GUIControl_GetPaddingX(GUIControl *control);

// Script API: GUIControl::set_PaddingX
void GUIControl_SetPaddingX(GUIControl *control, int padding);

// Script API: GUIControl::get_PaddingY
int GUIControl_GetPaddingY(GUIControl *control);

// Script API: GUIControl::set_PaddingY
void GUIControl_SetPaddingY(GUIControl *control, int padding);

// Script API: GUIControl::get_ShowBorder
bool GUIControl_GetShowBorder(GUIControl *control);

// Script API: GUIControl::set_ShowBorder
void GUIControl_SetShowBorder(GUIControl *control, bool on);

// Script API: GUIControl::get_SolidBackground
bool GUIControl_GetSolidBackground(GUIControl *control);

// Script API: GUIControl::set_SolidBackground
void GUIControl_SetSolidBackground(GUIControl *control, bool on);

// Script API: GUIControl::get_Translated
bool GUIControl_GetTranslated(GUIControl *control);

// Script API: GUIControl::set_Translated
void GUIControl_SetTranslated(GUIControl *control, bool is_translated);

// Script API: GUIControl::get_Opacity
int GUIControl_GetOpacity(GUIControl *control);

// Script API: GUIControl::set_Opacity
void GUIControl_SetOpacity(GUIControl *control, int alpha);

// Script API: GUIControl::get_Transparency
int GUIControl_GetTransparency(GUIControl *control);

// Script API: GUIControl::set_Transparency
void GUIControl_SetTransparency(GUIControl *control, int trans);

// Script API: GUIControl::get_BlendMode
int GUIControl_GetBlendMode(GUIControl *control);

// Script API: GUIControl::set_BlendMode
void GUIControl_SetBlendMode(GUIControl *control, int blend_mode);

// Script API: GUIControl::GetProperty^1
int GUIControl_GetProperty(GUIControl *control, const char *property);

// Script API: GUIControl::GetTextProperty^1
const char *GUIControl_GetTextProperty(GUIControl *control, const char *property);

// Script API: GUIControl::SetProperty^2
bool GUIControl_SetProperty(GUIControl *control, const char *property, int value);

// Script API: GUIControl::SetTextProperty^2
bool GUIControl_SetTextProperty(GUIControl *control, const char *property, const char *value);

// Script API: GUIControl::get_Shader
ScriptShaderInstance *GUIControl_GetShader(GUIControl *control);

// Script API: GUIControl::set_Shader
void GUIControl_SetShader(GUIControl *control, ScriptShaderInstance *shader_inst);

// Script API: GUIControl::GetByName, GUIControl::GetByScriptName
GUIControl *GUIControl_GetByName(const char *name);
