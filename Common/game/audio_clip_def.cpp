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

#include "game/audio_clip_def.h"

#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;

/*static*/ AudioFileType AudioClipDef::GetAudioFileTypeFromExt(const char *ext)
{
    if (ags_stricmp(ext, "mp3") == 0)
        return eAudioFileMP3;
    else if (ags_stricmp(ext, "wav") == 0)
        return eAudioFileWAV;
    else if (ags_stricmp(ext, "voc") == 0)
        return eAudioFileVOC;
    else if (ags_stricmp(ext, "mid") == 0)
        return eAudioFileMIDI;
    else if ((ags_stricmp(ext, "mod") == 0) || (ags_stricmp(ext, "xm") == 0) || (ags_stricmp(ext, "s3m") == 0) ||
             (ags_stricmp(ext, "it") == 0))
        return eAudioFileMOD;
    else if (ags_stricmp(ext, "ogg") == 0)
        return eAudioFileOGG;
    else if (ags_stricmp(ext, "flac") == 0)
        return eAudioFileFLAC;
    return eAudioFileUndefined;
}

/*static */ const char *AudioClipDef::GetExtFromAudioFileType(AudioFileType filetype)
{
    switch (filetype)
    {
    case eAudioFileOGG: return "ogg";
    case eAudioFileMP3: return "mp3";
    case eAudioFileWAV: return "wav";
    case eAudioFileVOC: return "voc";
    case eAudioFileMIDI: return "mid";
    case eAudioFileMOD: return "mod";
    case eAudioFileFLAC: return "flac";
    default: return "";
    }
}

void AudioClipDef::ReadFromFile(Stream *in)
{
    ID = in->ReadInt32();
    ScriptName.ReadCount(in, LEGACY_AUDIOCLIP_SCRIPTNAMELENGTH);
    FileName.ReadCount(in, LEGACY_AUDIOCLIP_FILENAMELENGTH);
    BundlingType = static_cast<uint8_t>(in->ReadInt8());
    Type = static_cast<uint8_t>(in->ReadInt8());
    FileType = static_cast<AudioFileType>(in->ReadInt8());
    IsRepeatingByDefault = in->ReadInt8();
    in->ReadInt8(); // alignment padding to int16
    DefaultPriority = in->ReadInt16();
    DefaultVolume = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    in->ReadInt32(); // reserved
}
