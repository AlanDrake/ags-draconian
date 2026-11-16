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
#include "game/dialog/dialog_options_rendering.h"

#include "debug/debug_log.h"
#include "game/dialog/dialog.h"
#include "game/dialog_def.h"
#include "game/game_context.h"
#include "game/game_struct_defines.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h" // quitprintf

// ** SCRIPT DIALOGOPTIONSRENDERING OBJECT

void DialogOptionsRendering_Update(ScriptDialogOptionsRendering *opt_render)
{
    opt_render->needRepaint = true;
}

bool DialogOptionsRendering_RunActiveOption(ScriptDialogOptionsRendering *opt_render)
{
    opt_render->chosenOptionID = opt_render->activeOptionID;
    return opt_render->chosenOptionID >= 0;
}

int DialogOptionsRendering_GetX(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->x;
}

void DialogOptionsRendering_SetX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
    dlgOptRender->x = newX;
}

int DialogOptionsRendering_GetY(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->y;
}

void DialogOptionsRendering_SetY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
    dlgOptRender->y = newY;
}

int DialogOptionsRendering_GetWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->width;
}

void DialogOptionsRendering_SetWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
    dlgOptRender->width = newWidth;
}

int DialogOptionsRendering_GetHeight(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->height;
}

void DialogOptionsRendering_SetHeight(ScriptDialogOptionsRendering *dlgOptRender, int newHeight)
{
    dlgOptRender->height = newHeight;
}

int DialogOptionsRendering_GetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxX;
}

void DialogOptionsRendering_SetParserTextboxX(ScriptDialogOptionsRendering *dlgOptRender, int newX)
{
    dlgOptRender->parserTextboxX = newX;
}

int DialogOptionsRendering_GetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxY;
}

void DialogOptionsRendering_SetParserTextboxY(ScriptDialogOptionsRendering *dlgOptRender, int newY)
{
    dlgOptRender->parserTextboxY = newY;
}

int DialogOptionsRendering_GetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->parserTextboxWidth;
}

void DialogOptionsRendering_SetParserTextboxWidth(ScriptDialogOptionsRendering *dlgOptRender, int newWidth)
{
    dlgOptRender->parserTextboxWidth = newWidth;
}

ScriptDialog *DialogOptionsRendering_GetDialogToRender(ScriptDialogOptionsRendering *dlgOptRender)
{
    return &Game::Current().ScDialogs[dlgOptRender->dialogID];
}

ScriptDrawingSurface *DialogOptionsRendering_GetSurface(ScriptDialogOptionsRendering *dlgOptRender)
{
    // Enforce that we only return a drawing surface inside "draw" callback
    if (dlgOptRender->isInDraw)
    {
        if (dlgOptRender->surfaceToRenderTo)
            dlgOptRender->surfaceAccessed = true;
        return dlgOptRender->surfaceToRenderTo;
    }
    return nullptr;
}

int DialogOptionsRendering_GetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender)
{
    return dlgOptRender->activeOptionID + 1;
}

void DialogOptionsRendering_SetActiveOptionID(ScriptDialogOptionsRendering *dlgOptRender, int activeOptionID)
{
    Game &game = Game::Current();
    int option_count = game.Dialogs[game.ScDialogs[dlgOptRender->dialogID].id].GetOptionCount();
    if ((activeOptionID < 0) || (activeOptionID > option_count))
        quitprintf("DialogOptionsRenderingInfo.ActiveOptionID: invalid ID specified for this dialog (specified %d, "
                   "valid range: 1..%d)",
                   activeOptionID, option_count);

    if (dlgOptRender->activeOptionID != activeOptionID - 1)
    {
        dlgOptRender->activeOptionID = activeOptionID - 1;
        dlgOptRender->needRepaint = true;
    }
}
