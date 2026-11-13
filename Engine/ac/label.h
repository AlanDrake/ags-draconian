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

#include "gui/guilabel.h"

using AGS::Common::GUILabel;

const char *Label_GetText_New(GUILabel *labl);
void Label_GetText(GUILabel *labl, char *buffer);
void Label_SetText(GUILabel *labl, const char *newtx);
int Label_GetTextColor(GUILabel *labl);
void Label_SetTextColor(GUILabel *labl, int colr);
int Label_GetFont(GUILabel *labl);
void Label_SetFont(GUILabel *guil, int fontnum);

// Script API: Label::get_TextAlignment
int Label_GetTextAlignment(GUILabel *labl);

// Script API: Label::set_TextAlignment
void Label_SetTextAlignment(GUILabel *labl, int align);

// Script API: Label::get_TextOutlineColor
int Label_GetTextOutlineColor(GUILabel *labl);

// Script API: Label::set_TextOutlineColor
void Label_SetTextOutlineColor(GUILabel *labl, int color);
