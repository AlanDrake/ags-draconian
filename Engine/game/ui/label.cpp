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
#include "game/ui/label.h"

#include <cstdio>

#include "game/game.h"
#include "game/game_def.h"
#include "game/lib/string.h"
#include "game/runtime_defines.h"
#include "util/common.h" // quit

using namespace AGS::Common;

// ** LABEL FUNCTIONS

const char *Label_GetText_New(GUILabel *labl)
{
    return CreateNewScriptString(labl->GetText());
}

void Label_GetText(GUILabel *labl, char *buffer)
{
    snprintf(buffer, MAX_MAXSTRLEN, "%s", labl->GetText().GetCStr());
}

void Label_SetText(GUILabel *labl, const char *newtx)
{
    if (labl->GetText() != newtx)
    {
        labl->SetText(newtx);
    }
}

int Label_GetTextAlignment(GUILabel *labl)
{
    return labl->GetTextAlignment();
}

void Label_SetTextAlignment(GUILabel *labl, int align)
{
    FrameAlignment use_align = (FrameAlignment)align;
    labl->SetTextAlignment(use_align);
}

int Label_GetTextColor(GUILabel *labl)
{
    return labl->GetTextColor();
}

void Label_SetTextColor(GUILabel *labl, int color)
{
    labl->SetTextColor(color);
}

int Label_GetTextOutlineColor(GUILabel *labl)
{
    return labl->GetTextOutlineColor();
}

void Label_SetTextOutlineColor(GUILabel *labl, int color)
{
    labl->SetTextOutlineColor(color);
}

int Label_GetFont(GUILabel *labl)
{
    return labl->GetFont();
}

void Label_SetFont(GUILabel *guil, int fontnum)
{
    fontnum = ValidateFontNumber("Label.Font", fontnum);
    guil->SetFont(fontnum);
}
