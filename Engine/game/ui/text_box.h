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

#include "gui/guitextbox.h"

using AGS::Common::GUITextBox;

const char *TextBox_GetText_New(GUITextBox *texbox);
void TextBox_GetText(GUITextBox *texbox, char *buffer);
void TextBox_SetText(GUITextBox *texbox, const char *newtex);
int TextBox_GetTextColor(GUITextBox *guit);
void TextBox_SetTextColor(GUITextBox *guit, int colr);
int TextBox_GetFont(GUITextBox *guit);
void TextBox_SetFont(GUITextBox *guit, int fontnum);

// Script API: TextBox::get_TextAlignment
int TextBox_GetTextAlignment(GUITextBox *guit);

// Script API: TextBox::set_TextAlignment
void TextBox_SetTextAlignment(GUITextBox *guit, int align);

// Script API: TextBox::get_TextOutlineColor
int TextBox_GetTextOutlineColor(GUITextBox *guit);

// Script API: TextBox::set_TextOutlineColor
void TextBox_SetTextOutlineColor(GUITextBox *guit, int color);

// Script API: TextBox::get_ShowBorder
bool TextBox_GetShowBorder(GUITextBox *guit);

// Script API: TextBox::set_ShowBorder
void TextBox_SetShowBorder(GUITextBox *guit, bool on);
