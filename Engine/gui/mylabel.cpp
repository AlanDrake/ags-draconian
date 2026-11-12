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
#include "gui/mylabel.h"

#include <cstring>

#include "ac/display.h"
#include "ac/gamesetup.h"
#include "ac/string.h"
#include "font/fonts.h"
#include "gui/guidialogdefines.h"

using namespace AGS;
using namespace AGS::Common;

extern int acdialog_font;

MyLabel::MyLabel(int left, int top, int width, const char *label, int text_height)
{
    snprintf(text, sizeof(text), "%s", label);
    x = left;
    y = top;
    wid = width;
    textheight = text_height;
    hit = textheight;
}

void MyLabel::draw(Bitmap *ds)
{
    int line_y = y;
    color_t text_color = GUI::GetStandardColorForBitmap(0);

    const char *draw_text = skip_voiceover_token(text);
    if (break_up_text_into_lines(draw_text, Lines, wid, acdialog_font) == 0)
        return;
    for (size_t line_id = 0; line_id < Lines.Count(); line_id++)
    {
        wouttext_outline(ds, x, line_y, acdialog_font, text_color, Lines[line_id].GetCStr());
        line_y += textheight;
    }
}

int MyLabel::pressedon(int /*mx*/, int /*my*/)
{
    return 0;
}

int MyLabel::processmessage(int /*mcode*/, int /*wParam*/, intptr_t /*lParam*/)
{
    return -1; // doesn't support messages
}
