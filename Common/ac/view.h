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

#include <vector>
#include "gfx/gfx_def.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

struct ViewFrame
{
    int pic = 0;
    int16_t xoffs = 0;
    int16_t yoffs = 0;
    int16_t speed = 0;
    AGS::Common::SpriteTransformFlags flags = AGS::Common::kSprTf_None;
    int sound = -1;                   // play sound when this frame comes round
    int reserved_for_future[2] = {0}; // kept only for plugin api // CLNUP: may remove in ags4?
    AGS::Common::String event_name;

    ViewFrame() = default;

    void ReadFromFile(AGS::Common::Stream *in);
    void WriteToFile(AGS::Common::Stream *out) const;
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
    // NOTE: we still need numFrames:
    // as we always allocate at least 1 frame for safety, to avoid crashes,
    // but have to report "logical" number of frames for the engine API.
    int numFrames;
    int flags;
    std::vector<ViewFrame> frames;

    // Returns last frame, or 0 if there's no frames in this loop
    int GetLastFrame() const { return numFrames > 0 ? numFrames - 1 : 0; }

    ViewLoopNew();
    void Initialize(int frameCount);
    void Dispose();
    // Tells if this loop has a "Run next loop after this" flag
    bool RunNextLoop() const;
    void WriteToFile(AGS::Common::Stream *out) const;
    void ReadFromFile(AGS::Common::Stream *in);
    void WriteFrames(AGS::Common::Stream *out) const;
    void ReadFrames(AGS::Common::Stream *in);
};

struct ViewStruct
{
    int numLoops;
    std::vector<ViewLoopNew> loops;

    // Returns last loop, or 0 if there's no loops in this view
    int GetLastLoop() const { return numLoops > 0 ? numLoops - 1 : 0; }

    ViewStruct();
    void Initialize(int loopCount);
    void Dispose();
    void WriteToFile(AGS::Common::Stream *out) const;
    void ReadFromFile(AGS::Common::Stream *in);
};
