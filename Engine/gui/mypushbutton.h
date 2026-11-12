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

struct MyPushButton : public NewControl
{
    char text[50];
    MyPushButton(int left, int top, int width, int height, const char *caption);
    void draw(AGS::Common::Bitmap *ds) override;
    int pressedon(int mx, int my) override;
    int processmessage(int mcode, int wParam, intptr_t ipParam) override;
};
