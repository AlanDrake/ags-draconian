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
#include "gui/mylistbox.h"

#include <cstring>

#include "ac/common.h"
#include "ac/gamesetup.h"
#include "ac/keycode.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gui/guidialog.h"
#include "gui/guidialogdefines.h"
#include "gui/guimain.h"

using namespace AGS::Common;

extern int windowbackgroundcolor;
extern int cbuttfont;
extern int smcode;

MyListBox::MyListBox(int left, int top, int width, int height, int textheight_)
{
    x = left;
    y = top;
    wid = width;
    hit = height;
    textheight = textheight_;
    hit -= (hit - 4) % textheight; // so that the height less the 4 border pixels is a whole number of rows
    numonscreen = (hit - 4) / textheight;
    items = 0;
    topitem = 0;
    selected = -1;
    memset(itemnames, 0, sizeof(itemnames));
}

void MyListBox::clearlist()
{
    for (int item_id = 0; item_id < items; item_id++)
        free(itemnames[item_id]);

    items = 0;
}

MyListBox::~MyListBox()
{
    clearlist();
}

void MyListBox::draw(Bitmap *ds)
{
    color_t draw_color = GUI::GetStandardColorForBitmap(windowbackgroundcolor);
    ds->FillRect(Rect(x, y, x + wid, y + hit), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    ds->DrawRect(Rect(x, y, x + wid, y + hit), draw_color);

    int old_width = wid;
    wid -= ARROWWIDTH;
    ds->DrawLine(Line(x + wid, y, x + wid, y + hit), draw_color); // draw the up/down arrows
    ds->DrawLine(Line(x + wid, y + hit / 2, x + old_width, y + hit / 2), draw_color);

    int mid_x = x + wid + (old_width - wid) / 2;
    if (topitem < 1)
        draw_color = GUI::GetStandardColorForBitmap(7);

    ds->DrawLine(Line(mid_x, y + 2, mid_x, y + 10), draw_color); // up arrow
    ds->DrawLine(Line(mid_x - 1, y + 3, mid_x + 1, y + 3), draw_color);
    ds->DrawLine(Line(mid_x - 2, y + 4, mid_x + 2, y + 4), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    if (topitem + numonscreen >= items)
        draw_color = GUI::GetStandardColorForBitmap(7);

    ds->DrawLine(Line(mid_x, y + hit - 10, mid_x, y + hit - 3), draw_color); // down arrow
    ds->DrawLine(Line(mid_x - 1, y + hit - 4, mid_x + 1, y + hit - 4), draw_color);
    ds->DrawLine(Line(mid_x - 2, y + hit - 5, mid_x + 2, y + hit - 5), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);

    for (int row = 0; row < numonscreen; row++)
    {
        int item_id = row + topitem;
        if (item_id >= items)
            break;

        int item_y = y + 2 + row * textheight;
        color_t text_color;
        if (item_id == selected)
        {
            draw_color = GUI::GetStandardColorForBitmap(0);
            ds->FillRect(Rect(x, item_y, x + wid, item_y + textheight - 1), draw_color);
            text_color = GUI::GetStandardColorForBitmap(7);
        }
        else
            text_color = GUI::GetStandardColorForBitmap(0);

        wouttextxy(ds, x + 2, item_y, cbuttfont, text_color, itemnames[item_id]);
    }
    wid = old_width;
}

int MyListBox::pressedon(int mx, int my)
{
    if (mx > x + wid - ARROWWIDTH)
    {
        if ((my - y < hit / 2) & (topitem > 0))
            topitem--;
        else if ((my - y > hit / 2) & (topitem + numonscreen < items))
            topitem++;
    }
    else
    {
        selected = ((my - y) - 2) / textheight + topitem;
        if (selected >= items)
            selected = items - 1;
    }

    draw(get_gui_screen());
    smcode = CM_SELCHANGE;
    return 0;
}

void MyListBox::additem(char *text)
{
    if (items >= MAXLISTITEM)
        quit("!CSCIUSER16: Too many items added to listbox");
    itemnames[items] = (char *)malloc(strlen(text) + 1);
    strcpy(itemnames[items], text);
    items++;
    needredraw = 1;
}

int MyListBox::processmessage(int mcode, int wParam, intptr_t ipParam)
{
    if (mcode == CLB_ADDITEM)
    {
        additem((char *)ipParam);
    }
    else if (mcode == CLB_CLEAR)
        clearlist();
    else if (mcode == CLB_GETCURSEL)
        return selected;
    else if (mcode == CLB_SETCURSEL)
    {
        selected = wParam;

        if ((selected < topitem) && (selected >= 0))
            topitem = selected;

        if (topitem + numonscreen <= selected)
            topitem = (selected + 1) - numonscreen;
    }
    else if (mcode == CLB_GETTEXT)
        strcpy((char *)ipParam, itemnames[wParam]);
    else if (mcode == CLB_SETTEXT)
    {
        if (wParam < items)
            free(itemnames[wParam]);

        char *new_text = (char *)ipParam;
        itemnames[wParam] = (char *)malloc(strlen(new_text) + 2);
        strcpy(itemnames[wParam], new_text);
    }
    else if (mcode == CTB_KEYPRESS)
    {
        if ((wParam == eAGSKeyCodeDownArrow) && (selected < items - 1))
            selected++;

        if ((wParam == eAGSKeyCodeUpArrow) && (selected > 0))
            selected--;

        if (wParam == eAGSKeyCodePageUp)
            selected -= (numonscreen - 1);

        if (wParam == eAGSKeyCodePageDown)
            selected += (numonscreen - 1);

        if ((selected < 0) && (items > 0))
            selected = 0;

        if (selected >= items)
            selected = items - 1;

        if ((selected < topitem) & (selected >= 0))
            topitem = selected;

        if (topitem + numonscreen <= selected)
            topitem = (selected + 1) - numonscreen;

        drawandmouse();
        smcode = CM_SELCHANGE;
    }
    else
        return -1;

    return 0;
}
