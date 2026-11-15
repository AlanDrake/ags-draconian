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
#include "game/ui/text_box.h"

#include <cstdio>

#include "game/game.h"
#include "game/game_setup_struct.h"
#include "game/lib/string.h"
#include "game/runtime_defines.h"
#include "util/common.h" // quit


// ** TEXT BOX FUNCTIONS

const char *TextBox_GetText_New(GUITextBox *texbox)
{
    return CreateNewScriptString(texbox->GetText());
}

void TextBox_GetText(GUITextBox *texbox, char *buffer)
{
    snprintf(buffer, MAX_MAXSTRLEN, "%s", texbox->GetText().GetCStr());
}

void TextBox_SetText(GUITextBox *texbox, const char *newtex)
{
    texbox->SetText(newtex);
}

int TextBox_GetTextAlignment(GUITextBox *guit)
{
    return guit->GetTextAlignment();
}

void TextBox_SetTextAlignment(GUITextBox *guit, int align)
{
    guit->SetTextAlignment(static_cast<FrameAlignment>(align));
}

int TextBox_GetTextColor(GUITextBox *guit)
{
    return guit->GetTextColor();
}

void TextBox_SetTextColor(GUITextBox *guit, int color)
{
    guit->SetTextColor(color);
}

int TextBox_GetTextOutlineColor(GUITextBox *guit)
{
    return guit->GetTextOutlineColor();
}

void TextBox_SetTextOutlineColor(GUITextBox *guit, int color)
{
    guit->SetTextOutlineColor(color);
}

int TextBox_GetFont(GUITextBox *guit)
{
    return guit->GetFont();
}

void TextBox_SetFont(GUITextBox *guit, int fontnum)
{
    fontnum = ValidateFontNumber("TextBox.Font", fontnum);
    guit->SetFont(fontnum);
}

bool TextBox_GetShowBorder(GUITextBox *guit)
{
    return guit->IsShowBorder();
}

void TextBox_SetShowBorder(GUITextBox *guit, bool on)
{
    if (guit->IsShowBorder() != on)
    {
        guit->SetShowBorder(on);
        guit->MarkChanged();
    }
}
