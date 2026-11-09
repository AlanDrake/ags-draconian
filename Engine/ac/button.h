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
#pragma once

#include "gui/guibutton.h"

using AGS::Common::GUIButton;
struct AnimatingGUIButton;

void Button_Animate(GUIButton *btn, int view, int loop, int speed, int repeat, int blocking, int direction,
                    int sframe = 0, int volume = 100);
void Button_Animate4(GUIButton *btn, int view, int loop, int speed, int repeat);
const char *Button_GetText_New(GUIButton *btn);
void Button_GetText(GUIButton *btn, char *buffer);
void Button_SetText(GUIButton *btn, const char *newtx);
void Button_SetFont(GUIButton *btn, int new_font);
int Button_GetFont(GUIButton *btn);
int Button_GetClipImage(GUIButton *btn);
void Button_SetClipImage(GUIButton *btn, int newval);
int Button_GetGraphic(GUIButton *btn);
int Button_GetMouseOverGraphic(GUIButton *btn);
void Button_SetMouseOverGraphic(GUIButton *btn, int slotn);
int Button_GetNormalGraphic(GUIButton *btn);
void Button_SetNormalGraphic(GUIButton *btn, int slotn);
int Button_GetPushedGraphic(GUIButton *btn);
void Button_SetPushedGraphic(GUIButton *btn, int slotn);
int Button_GetTextColor(GUIButton *btn);
void Button_SetTextColor(GUIButton *btn, int newcol);

// Update button's animation, returns whether the animation continues
bool UpdateAnimatingButton(int bu);
size_t GetAnimatingButtonCount();
AnimatingGUIButton *GetAnimatingButtonByIndex(int idxn);
void AddButtonAnimation(const AnimatingGUIButton &abtn);
void StopButtonAnimation(int idxn);
int FindButtonAnimation(int gui_id, int objn);
void FindAndRemoveButtonAnimation(int gui_id, int objn);
void RemoveAllButtonAnimations();
