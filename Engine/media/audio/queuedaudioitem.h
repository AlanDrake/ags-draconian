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
#include "media/audio/soundclip.h"

namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

struct QueuedAudioItem
{
    short audioClipIndex = -1;
    short priority = 0;
    bool repeat = false;
    std::unique_ptr<SoundClip> cachedClip;

    void ReadFromSavegame(AGS::Common::Stream *in);
    void WriteToSavegame(AGS::Common::Stream *out) const;
};
