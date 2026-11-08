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
#include "ac/view.h"

#include <cstring>

#include "util/stream.h"

using namespace AGS::Common;

void ViewFrame::ReadFromFile(Stream *in)
{
    pic = in->ReadInt32();
    xoffs = in->ReadInt16();
    yoffs = in->ReadInt16();
    speed = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    flags = (SpriteTransformFlags)in->ReadInt32();
    sound = in->ReadInt32();
    in->ReadInt32(); // reserved 1
    in->ReadInt32(); // reserved 1
}

ViewLoopNew::ViewLoopNew() : numFrames(0), flags(0) {}

bool ViewLoopNew::RunNextLoop() const
{
    return (flags & LOOPFLAG_RUNNEXTLOOP);
}

void ViewLoopNew::Initialize(int frameCount)
{
    numFrames = frameCount;
    flags = 0;
    // an extra frame is allocated to prevent crashes with empty loops
    frames.resize(numFrames > 0 ? numFrames : 1);
}

void ViewLoopNew::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));
    flags = in->ReadInt32();
    ReadFrames(in);
}

void ViewLoopNew::ReadFrames(Stream *in)
{
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].ReadFromFile(in);
    }
}

ViewStruct::ViewStruct() : numLoops(0) {}

void ViewStruct::Initialize(int loopCount)
{
    numLoops = loopCount;
    loops.resize(numLoops);
}

void ViewStruct::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));

    for (int i = 0; i < numLoops; i++)
    {
        loops[i].ReadFromFile(in);
    }
}
