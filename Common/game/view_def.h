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
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

struct ViewFrameDef
{
    int Sprite = 0;
    int16_t OffsetX = 0;
    int16_t OffsetY = 0;
    int16_t Delay = 0; // script: ViewFrame.Speed; added to the animation's delay
    AGS::Common::SpriteTransformFlags Flags = AGS::Common::kSprTf_None;
    int LinkedAudio = -1;           // play sound when this frame comes round
    int ReservedForFuture[2] = {0}; // kept only for plugin api // CLNUP: may remove in ags4?
    AGS::Common::String EventName;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<ViewFrameDef> Fields();

    void ReadFromFile(AGS::Common::Stream *in);
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopDef
{
    // NOTE: we still need numFrames:
    // as we always allocate at least 1 frame for safety, to avoid crashes,
    // but have to report "logical" number of frames for the engine API.
    int FrameCount = 0;
    int Flags = 0;
    std::vector<ViewFrameDef> Frames;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<ViewLoopDef> Fields();

    // Returns last frame, or 0 if there's no frames in this loop
    int GetLastFrame() const { return FrameCount > 0 ? FrameCount - 1 : 0; }

    void Initialize(int frame_count);
    // Tells if this loop has a "Run next loop after this" flag
    bool RunNextLoop() const;
    void ReadFromFile(AGS::Common::Stream *in);
    void ReadFrames(AGS::Common::Stream *in);
};

struct ViewDef
{
    int LoopCount = 0;
    std::vector<ViewLoopDef> Loops;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<ViewDef> Fields();

    // Returns last loop, or 0 if there's no loops in this view
    int GetLastLoop() const { return LoopCount > 0 ? LoopCount - 1 : 0; }

    void Initialize(int loop_count);
    void ReadFromFile(AGS::Common::Stream *in);
};
