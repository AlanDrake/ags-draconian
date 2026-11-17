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
//
//
//
//=============================================================================
#pragma once

#include "game/common_defines.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

enum AudioFileType
{
    eAudioFileUndefined = 0,
    eAudioFileOGG = 1,
    eAudioFileMP3 = 2,
    eAudioFileWAV = 3,
    eAudioFileVOC = 4,
    eAudioFileMIDI = 5,
    eAudioFileMOD = 6,
    eAudioFileFLAC = 7
};

// TODO: consider turning this into generic "asset bundle type"?
enum AudioClipBundle
{
    kAudioBundle_Undefined = 0,
    kAudioBundle_GamePak = 1,
    kAudioBundle_AudioVox = 2,
    kAudioBundle_SpeechVox = 3
};

#define LEGACY_AUDIOCLIP_SCRIPTNAMELENGTH 30
#define LEGACY_AUDIOCLIP_FILENAMELENGTH   15

struct AudioClipDef
{
public:
    int ID = -1;
    AGS::Common::String ScriptName;
    AGS::Common::String FileName;
    uint8_t BundlingType = kAudioBundle_Undefined;
    uint8_t Type = AUDIOTYPE_UNDEFINED;
    AudioFileType FileType = eAudioFileUndefined;
    char IsRepeatingByDefault = 0; // script: AudioClip.DefaultRepeat
    short DefaultPriority = 50;
    short DefaultVolume = 100;

    static AudioFileType GetAudioFileTypeFromExt(const char *ext);
    static const char *GetExtFromAudioFileType(AudioFileType filetype);

    void ReadFromFile(AGS::Common::Stream *in);
};
