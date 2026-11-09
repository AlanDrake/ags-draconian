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

AGS::Common::GUIObject *GUIControl_GetAtScreenXY(int x, int y, int gui_hitoptions, int ctrl_hitoptions);
int GUIControl_GetVisible(GUIControl *control);
void GUIControl_SetVisible(GUIControl *control, int visible);
int GUIControl_GetClickable(GUIControl *control);
void GUIControl_SetClickable(GUIControl *control, int enabled);
int GUIControl_GetEnabled(GUIControl *control);
void GUIControl_SetEnabled(GUIControl *control, int enabled);
int GUIControl_GetID(GUIControl *control);
ScriptGUI *GUIControl_GetOwningGUI(GUIControl *control);
GUIButton *GUIControl_GetAsButton(GUIControl *control);
GUIInvWindow *GUIControl_GetAsInvWindow(GUIControl *control);
GUILabel *GUIControl_GetAsLabel(GUIControl *control);
GUIListBox *GUIControl_GetAsListBox(GUIControl *control);
GUISlider *GUIControl_GetAsSlider(GUIControl *control);
GUITextBox *GUIControl_GetAsTextBox(GUIControl *control);
int GUIControl_GetX(GUIControl *control);
void GUIControl_SetX(GUIControl *control, int x);
int GUIControl_GetY(GUIControl *control);
void GUIControl_SetY(GUIControl *control, int y);
int GUIControl_GetZOrder(GUIControl *control);
void GUIControl_SetZOrder(GUIControl *control, int zorder);
void GUIControl_SetPosition(GUIControl *control, int x, int y);
int GUIControl_GetWidth(GUIControl *control);
void GUIControl_SetWidth(GUIControl *control, int newwid);
int GUIControl_GetHeight(GUIControl *control);
void GUIControl_SetHeight(GUIControl *control, int newhit);
void GUIControl_SetSize(GUIControl *control, int newwid, int newhit);
void GUIControl_SendToBack(GUIControl *control);
void GUIControl_BringToFront(GUIControl *control);
