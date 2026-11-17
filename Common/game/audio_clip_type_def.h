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

// Forward declaration
namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

struct AudioClipTypeDef
{
    int ID = 0;
    int ReservedChannels = 0;
    int VolumeReductionWhileSpeechPlaying = 0;
    int CrossfadeSpeed = 0;

    void ReadFromFile(AGS::Common::Stream *in);
};
