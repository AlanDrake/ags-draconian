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

#include "game/audio_clip_type.h"

#include "util/stream.h"

using namespace AGS;
using namespace AGS::Common;

using AGS::Common::Stream;

void AudioClipType::ReadFromFile(Stream *in)
{
    id = in->ReadInt32();
    reservedChannels = in->ReadInt32();
    volume_reduction_while_speech_playing = in->ReadInt32();
    crossfadeSpeed = in->ReadInt32();
    in->ReadInt32(); // reserved
}
