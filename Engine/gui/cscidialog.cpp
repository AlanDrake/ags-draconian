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
#include "gui/cscidialog.h"

#include <cctype>

#include "ac/common.h"
#include "ac/draw.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/keycode.h"
#include "ac/mouse.h"
#include "ac/runtime_defines.h"
#include "ac/sys_events.h"
#include "ac/timer.h"
#include "font/fonts.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gui/guidialog.h"
#include "gui/guimain.h"
#include "gui/mycontrols.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern IGraphicsDriver *gfxDriver;


//-----------------------------------------------------------------------------
// DIALOG SYSTEM STUFF below

int windowbackgroundcolor = COL254, pushbuttondarkcolor = COL255;
int pushbuttonlightcolor = COL253;
int topwindowhandle = -1;
int cbuttfont;

int acdialog_font;

int smcode = 0;

#define MAXCONTROLS      20
#define MAXSCREENWINDOWS 5
NewControl *vobjs[MAXCONTROLS];
OnScreenWindow oswi[MAXSCREENWINDOWS];

int controlid = 0;


//-----------------------------------------------------------------------------

void __my_wbutt(Bitmap *dst, int x1, int y1, int x2, int y2)
{
    color_t draw_color = GUI::GetStandardColorForBitmap(COL254);
    dst->FillRect(Rect(x1, y1, x2, y2), draw_color);
    draw_color = GUI::GetStandardColorForBitmap(0);
    dst->DrawRect(Rect(x1, y1, x2, y2), draw_color);
}

//-----------------------------------------------------------------------------

OnScreenWindow::OnScreenWindow()
{
    x = y = 0;
    handle = -1;
    oldtop = -1;
}

int windowcount = 0, curswas = 0;
int win_x = 0, win_y = 0, win_width = 0, win_height = 0;
// CLNUP dialogs are based on 320x200 coords, will need to at least center them
int CSCIDrawWindow(int x, int y, int width, int height)
{
    ignore_bounds++;
    int win_id = -1;
    for (int i = 0; i < MAXSCREENWINDOWS; i++)
    {
        if (oswi[i].handle < 0)
        {
            win_id = i;
            break;
        }
    }

    if (win_id < 0)
        quit("Too many windows created.");

    windowcount++;
    x -= 2;
    y -= 2;
    width += 4;
    height += 4;
    Bitmap *dst = prepare_gui_screen(x, y, width, height, true);
    oswi[win_id].x = x;
    oswi[win_id].y = y;
    __my_wbutt(dst, 0, 0, width - 1, height - 1); // fills the whole enlarged area and frames it
    oswi[win_id].oldtop = topwindowhandle;
    topwindowhandle = win_id;
    oswi[win_id].handle = topwindowhandle;
    win_x = x;
    win_y = y;
    win_width = width;
    win_height = height;
    return win_id;
}

void CSCIEraseWindow(int win_id)
{
    ignore_bounds--;
    topwindowhandle = oswi[win_id].oldtop;
    oswi[win_id].handle = -1;
    windowcount--;
    clear_gui_screen();
}

int CSCIWaitMessage(CSCIMessage *msg)
{
    for (int i = 0; i < MAXCONTROLS; i++)
    {
        if (vobjs[i] != nullptr)
        {
            vobjs[i]->drawifneeded();
        }
    }

    prepare_gui_screen(win_x, win_y, win_width, win_height, true);

    while (1)
    {
        sys_evt_process_pending();

        update_audio_system_on_game_loop();
        refresh_gui_screen();

        msg->id = -1;
        msg->code = 0;
        smcode = 0;
        // NOTE: CSCIWaitMessage is supposed to report only single message,
        // therefore we cannot process all buffered key presses here
        // (unless the whole dialog system is rewritten).
        KeyInput key_input;
        if (run_service_key_controls(key_input) && !play.IsIgnoringInput())
        {
            int key = key_input.Key;
            int unichar = key_input.UChar;
            if (key == eAGSKeyCodeReturn)
            {
                msg->id = finddefaultcontrol(CNF_DEFAULT);
                msg->code = CM_COMMAND;
            }
            else if (key == eAGSKeyCodeEscape)
            {
                msg->id = finddefaultcontrol(CNF_CANCEL);
                msg->code = CM_COMMAND;
            }
            else if ((unichar == 0) && (key < eAGSKeyCodeSpace) && (key != eAGSKeyCodeBackspace))
                ;
            else if ((key >= eAGSKeyCodeUpArrow) & (key <= eAGSKeyCodePageDown) &
                     (finddefaultcontrol(CNT_LISTBOX) >= 0))
                vobjs[finddefaultcontrol(CNT_LISTBOX)]->processmessage(CTB_KEYPRESS, key, 0);
            else if (finddefaultcontrol(CNT_TEXTBOX) >= 0)
                vobjs[finddefaultcontrol(CNT_TEXTBOX)]->processmessage(CTB_KEYPRESS, key, unichar);

            if (msg->id < 0)
            {
                msg->code = CM_KEYPRESS;
                msg->wParam = key;
            }
        }

        eAGSMouseButton mouse_btn;
        if (run_service_mb_controls(mouse_btn) && (mouse_btn > kMouseNone) && !play.IsIgnoringInput())
        {
            if (checkcontrols())
            {
                msg->id = controlid;
                msg->code = CM_COMMAND;
            }
        }

        if (smcode)
        {
            msg->code = smcode;
            msg->id = controlid;
        }

        if (msg->code > 0)
            break;

        WaitForNextFrame();
    }

    return 0;
}

int CSCICreateControl(int typeandflags, int x, int y, int width, int height, const char *title)
{
    int ctrl_id = -1;
    for (int i = 1; i < MAXCONTROLS; i++)
    {
        if (vobjs[i] == nullptr)
        {
            ctrl_id = i;
            break;
        }
    }

    if (ctrl_id < 0)
        quit("Too many controls created");

    int type = typeandflags & 0x00ff; // 256 control types
    if (type == CNT_PUSHBUTTON)
    {
        if (width == -1)
            width = get_text_width(title, cbuttfont) + 20;

        vobjs[ctrl_id] = new MyPushButton(x, y, width, height, title);
    }
    else if (type == CNT_LISTBOX)
    {
        vobjs[ctrl_id] = new MyListBox(x, y, width, height, play.std_gui_textheight);
    }
    else if (type == CNT_LABEL)
    {
        vobjs[ctrl_id] = new MyLabel(x, y, width, title, play.std_gui_textheight);
    }
    else if (type == CNT_TEXTBOX)
    {
        vobjs[ctrl_id] = new MyTextBox(x, y, width, title, play.std_gui_textheight);
    }
    else
        quit("Unknown control type requested");

    vobjs[ctrl_id]->typeandflags = typeandflags;
    vobjs[ctrl_id]->wlevel = topwindowhandle;
    vobjs[ctrl_id]->draw(get_gui_screen());
    return ctrl_id;
}

void CSCIDeleteControl(int ctrl_id)
{
    delete vobjs[ctrl_id];
    vobjs[ctrl_id] = nullptr;
}

int CSCISendControlMessage(int ctrl_id, int msg, int w_param, intptr_t ip_param)
{
    if (vobjs[ctrl_id] == nullptr)
        return -1;
    return vobjs[ctrl_id]->processmessage(msg, w_param, ip_param);
}

// TODO: this is silly, make a uniform formula
int checkcontrols()
{
    if (!play.mouse_input_enabled)
        return 0;

    // NOTE: this is because old code was working with full game screen
    const int mx = ::mousex - win_x;
    const int my = ::mousey - win_y;

    smcode = 0;
    for (int i = 0; i < MAXCONTROLS; i++)
    {
        if (vobjs[i] != nullptr)
        {
            if (vobjs[i]->mouseisinarea(mx, my))
            {
                controlid = i;
                return vobjs[i]->pressedon(mx, my);
            }
        }
    }
    return 0;
}

int finddefaultcontrol(int flag_mask)
{
    for (int i = 0; i < MAXCONTROLS; i++)
    {
        if (vobjs[i] == nullptr)
            continue;

        if (vobjs[i]->wlevel != topwindowhandle)
            continue;

        if (vobjs[i]->typeandflags & flag_mask)
            return i;
    }

    return -1;
}
