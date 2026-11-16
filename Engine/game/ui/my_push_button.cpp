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
#include "game/ui/my_push_button.h"

#include <cstring>

#include "device/sys_events.h"
#include "font/fonts.h"
#include "game/game_context.h"
#include "game/ui/csci_dialog.h"
#include "game/ui/gui_dialog.h"
#include "game/ui/gui_dialog_defines.h"
#include "game/ui/mouse.h"
#include "gfx/bitmap.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "platform/base/agsplatformdriver.h"
#include "util/common.h"
#include "util/timer.h"

using namespace AGS::Common;

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
    Game &game = Game::Current();
    color_t text_color = GUI::GetStandardColorForBitmap(0);
    color_t draw_color = GUI::GetStandardColorForBitmap(COL254);
    ds->FillRect(Rect(x, y, x + wid, y + hit), draw_color);
    if (state == 0)
        draw_color = GUI::GetStandardColorForBitmap(game.BuiltinDialogs.PushButtonDarkColor);
    else
        draw_color = GUI::GetStandardColorForBitmap(game.BuiltinDialogs.PushButtonLightColor);

    ds->DrawRect(Rect(x, y, x + wid, y + hit), draw_color);
    if (state == 0)
        draw_color = GUI::GetStandardColorForBitmap(game.BuiltinDialogs.PushButtonLightColor);
    else
        draw_color = GUI::GetStandardColorForBitmap(game.BuiltinDialogs.PushButtonDarkColor);

    ds->DrawLine(Line(x, y, x + wid - 1, y), draw_color);
    ds->DrawLine(Line(x, y, x, y + hit - 1), draw_color);
    wouttextxy(ds, x + (wid / 2 - get_text_width(text, game.BuiltinDialogs.ButtonFont) / 2), y + 2,
               game.BuiltinDialogs.ButtonFont, text_color, text);
    if (typeandflags & CNF_DEFAULT)
        draw_color = GUI::GetStandardColorForBitmap(0);
    else
        draw_color = GUI::GetStandardColorForBitmap(game.BuiltinDialogs.WindowBackgroundColor);

    ds->DrawRect(Rect(x - 1, y - 1, x + wid + 1, y + hit + 1), draw_color);
}

int MyPushButton::pressedon(int /*mx*/, int /*my*/)
{
    Game &game = Game::Current();
    int old_state;
    while (ags_misbuttondown(kMouseLeft))
    {
        old_state = state;
        // the button is pressed only while the mouse is over it, as it moves
        ags_domouse();
        state = mouseisinarea(::mousex - game.BuiltinDialogs.WinX, ::mousey - game.BuiltinDialogs.WinY);
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
