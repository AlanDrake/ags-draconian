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
#include "game/view_def.h"

#include <cstring>

#include "util/stream.h"

using namespace AGS::Common;

void ViewFrameDef::ReadFromFile(Stream *in)
{
    Sprite = in->ReadInt32();
    OffsetX = in->ReadInt16();
    OffsetY = in->ReadInt16();
    Delay = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    Flags = (SpriteTransformFlags)in->ReadInt32();
    LinkedAudio = in->ReadInt32();
    in->ReadInt32(); // reserved 1
    in->ReadInt32(); // reserved 1
}

ViewLoopDef::ViewLoopDef() : FrameCount(0), Flags(0) {}

bool ViewLoopDef::RunNextLoop() const
{
    return (Flags & LOOPFLAG_RUNNEXTLOOP);
}

void ViewLoopDef::Initialize(int frame_count)
{
    FrameCount = frame_count;
    Flags = 0;
    // an extra frame is allocated to prevent crashes with empty loops
    Frames.resize(FrameCount > 0 ? FrameCount : 1);
}

void ViewLoopDef::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));
    Flags = in->ReadInt32();
    ReadFrames(in);
}

void ViewLoopDef::ReadFrames(Stream *in)
{
    for (int i = 0; i < FrameCount; ++i)
    {
        Frames[i].ReadFromFile(in);
    }
}

ViewDef::ViewDef() : LoopCount(0) {}

void ViewDef::Initialize(int loop_count)
{
    LoopCount = loop_count;
    Loops.resize(LoopCount);
}

void ViewDef::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));

    for (int i = 0; i < LoopCount; i++)
    {
        Loops[i].ReadFromFile(in);
    }
}
