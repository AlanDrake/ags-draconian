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

#include "ac/dynobj/scriptobjects.h"
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
// through the Sc_GUIControl_<Name> wrapper registered in RegisterGUIControlAPI (guicontrol.cpp); a pair
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
