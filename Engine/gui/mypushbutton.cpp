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
#include "gui/mypushbutton.h"

#include <cstring>

#include "ac/common.h"
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gui/guidialog.h"
#include "gui/guidialogdefines.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "platform/base/agsplatformdriver.h"

using namespace AGS::Common;

extern int windowbackgroundcolor, pushbuttondarkcolor;
extern int pushbuttonlightcolor;
extern int cbuttfont;

MyPushButton::MyPushButton(int left, int top, int width, int height, const char *caption)
{
    x = left;
    y = top;
    wid = width;
    hit = height + 1;
    state = 0;
    snprintf(text, sizeof(text), "%s", caption);
}

void MyPushButton::draw(Bitmap *ds)
{
    color_t text_color = GUI::GetStandardColorForBitmap(0);
    color_t draw_color = GUI::GetStandardColorForBitmap(COL254);
    ds->FillRect(Rect(x, y, x + wid, y + hit), draw_color);
    if (state == 0)
        draw_color = GUI::GetStandardColorForBitmap(pushbuttondarkcolor);
    else
        draw_color = GUI::GetStandardColorForBitmap(pushbuttonlightcolor);

    ds->DrawRect(Rect(x, y, x + wid, y + hit), draw_color);
    if (state == 0)
        draw_color = GUI::GetStandardColorForBitmap(pushbuttonlightcolor);
    else
        draw_color = GUI::GetStandardColorForBitmap(pushbuttondarkcolor);

    ds->DrawLine(Line(x, y, x + wid - 1, y), draw_color);
    ds->DrawLine(Line(x, y, x, y + hit - 1), draw_color);
    wouttextxy(ds, x + (wid / 2 - get_text_width(text, cbuttfont) / 2), y + 2, cbuttfont, text_color, text);
    if (typeandflags & CNF_DEFAULT)
        draw_color = GUI::GetStandardColorForBitmap(0);
    else
        draw_color = GUI::GetStandardColorForBitmap(windowbackgroundcolor);

    ds->DrawRect(Rect(x - 1, y - 1, x + wid + 1, y + hit + 1), draw_color);
}

int MyPushButton::pressedon(int mx, int my)
{
    int old_state;
    while (!ags_misbuttondown(kMouseLeft) == 0)
    {
        old_state = state;
        state = mouseisinarea(mx, my);
        update_polled_stuff();
        if (old_state != state)
        {
            draw(get_gui_screen());
        }

        refresh_gui_screen();

        WaitForNextFrame();
    }
    old_state = state;
    state = 0;
    draw(get_gui_screen());
    return old_state;
}

int MyPushButton::processmessage(int /*mcode*/, int /*wParam*/, intptr_t /*lParam*/)
{
    return -1; // doesn't support messages
}
