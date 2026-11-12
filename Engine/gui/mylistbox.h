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

#include "gui/newcontrol.h"

#define MAXLISTITEM 300
#define ARROWWIDTH  8

// A list box of the legacy built-in dialogs: up to MAXLISTITEM items in rows of textheight pixels,
// with a scroll arrow pair of ARROWWIDTH pixels at the right. Handles the CLB_* messages and CTB_KEYPRESS.
struct MyListBox : public NewControl
{
    int items, topitem, numonscreen, selected;
    char *itemnames[MAXLISTITEM];
    int textheight; // TODO: use font instead

    MyListBox(int left, int top, int width, int height, int textheight);
    // Frees all item texts and empties the list.
    void clearlist();
    ~MyListBox() override;

    void draw(AGS::Common::Bitmap *ds) override;
    int pressedon(int mx, int my) override;
    // Appends a copy of the text; quits when MAXLISTITEM items are already there.
    void additem(char *text);
    int processmessage(int mcode, int wParam, intptr_t ipParam) override;
};
