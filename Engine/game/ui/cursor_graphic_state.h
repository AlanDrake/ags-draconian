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

#include <memory>

namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// A collection of properties depicting the current mouse cursor look
// TODO: this looks quite like the limited ScreenOverlay, merge these classes;
// store cursor's generated graphic as a dynamic sprite too
struct CursorGraphicState
{
public:
    // Gets actual cursor's image, whether owned by cursor or by a sprite reference
    AGS::Common::Bitmap *GetImage() const;
    // Get sprite id, or 0 if none set
    int GetSpriteNum() const { return _sprnum; }
    // Assigns an exclusive image to the cursor state
    void SetImage(std::unique_ptr<AGS::Common::Bitmap> pic);
    // Assigns a shared sprite to the cursor state
    void SetSpriteNum(int sprnum);
    // Tells if the cursor has graphically changed recently
    bool HasChanged() const { return _hasChanged; }
    // Manually marks GUI as graphically changed
    void MarkChanged() { _hasChanged = true; }
    // Clears changed flag
    void ClearChanged() { _hasChanged = false; }

private:
    void ResetImage();

    // Current cursor sprite ID, may be a >= 0 for a sprite or -1 for a generated bitmap
    // TODO: refactor, replace these with ObjTexture, and move to draw.cpp
    int _sprnum = -1;
    // Generated cursor image, used to create a modified cursor with
    // hotspot crosshair drawn over it, or when an invalid pic index set.
    // TODO: store this as dynamic sprite instead, see generated sprites for text overlays.
    std::unique_ptr<AGS::Common::Bitmap> _genImage;
    bool _hasChanged = false;
};
