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
#include "game/ui/new_control.h"

#include "game/game_context.h"
#include "game/ui/csci_dialog.h"
#include "game/ui/gui_dialog.h"
#include "game/ui/gui_dialog_defines.h"

NewControl::NewControl()
{
    x = y = wid = hit = 0;
    state = 0;
    typeandflags = 0;
    wlevel = 0;
    visible = 1;
    enabled = 1;
    needredraw = 1;
}
int NewControl::mouseisinarea(int mx, int my)
{
    if (Game::Current().BuiltinDialogs.TopWindowHandle != wlevel)
        return 0;

    if ((mx > x) & (mx < x + wid) & (my > y) & (my < y + hit))
        return 1;

    return 0;
}
void NewControl::drawifneeded()
{
    if (Game::Current().BuiltinDialogs.TopWindowHandle != wlevel)
        return;
    if (needredraw)
    {
        needredraw = 0;
        draw(get_gui_screen());
    }
}
void NewControl::drawandmouse()
{
    draw(get_gui_screen());
}
