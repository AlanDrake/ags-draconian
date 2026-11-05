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


Bitmap *wnewblock(Bitmap *src, int x1, int y1, int x2, int y2)
{
    Bitmap *tempbitm;
    int twid = (x2 - x1) + 1, thit = (y2 - y1) + 1;

    if (twid < 1)
        twid = 1;

    if (thit < 1)
        thit = 1;

    tempbitm = BitmapHelper::CreateBitmap(twid, thit);

    if (tempbitm == nullptr)
        return nullptr;

    tempbitm->Blit(src, x1, y1, 0, 0, tempbitm->GetWidth(), tempbitm->GetHeight());
    return tempbitm;
}

void wputblock(Bitmap *ds, int xx, int yy, Bitmap *bll, int xray)
{
    if (xray)
        ds->Blit(bll, xx, yy, kBitmap_Transparency);
    else
        ds->Blit(bll, 0, 0, xx, yy, bll->GetWidth(), bll->GetHeight());
}

Bitmap wputblock_wrapper; // [IKM] argh! :[
void wputblock_raw(Bitmap *ds, int xx, int yy, BITMAP *bll, int xray)
{
    wputblock_wrapper.WrapAllegroBitmap(bll, true);
    if (xray)
        ds->Blit(&wputblock_wrapper, xx, yy, kBitmap_Transparency);
    else
        ds->Blit(&wputblock_wrapper, 0, 0, xx, yy, wputblock_wrapper.GetWidth(), wputblock_wrapper.GetHeight());
}
