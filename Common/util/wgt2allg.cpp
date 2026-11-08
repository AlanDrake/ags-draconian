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
#include "util/wgt2allg.h"

#include "gfx/bitmap.h"

using namespace AGS::Common;


Bitmap wputblock_wrapper; // [IKM] argh! :[
void wputblock_raw(Bitmap *ds, int xx, int yy, BITMAP *bll, int xray)
{
    wputblock_wrapper.WrapAllegroBitmap(bll, true);
    if (xray)
        ds->Blit(&wputblock_wrapper, xx, yy, kBitmap_Transparency);
    else
        ds->Blit(&wputblock_wrapper, 0, 0, xx, yy, wputblock_wrapper.GetWidth(), wputblock_wrapper.GetHeight());
}
