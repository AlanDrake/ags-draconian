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

#include "game/audio_clip_type_def.h"

#include "util/field_table.h"
#include "util/stream.h"

using namespace AGS;
using namespace AGS::Common;

using AGS::Common::Stream;

namespace
{

// The roles are those of the live audio type, the engine's copy of the definition made when the game starts
// (StartFromDefinitions). Saved: the save's "Audio" component holds it. FromDefinition: a save does not hold it, the
// game's start gives the definition's; nothing writes it during play.
constexpr FieldRow<AudioClipTypeDef> kAudioClipTypeDefFields[] = {
    Field<&AudioClipTypeDef::ID>("ID", FieldRole::FromDefinition),
    Field<&AudioClipTypeDef::ReservedChannels>("ReservedChannels", FieldRole::FromDefinition),
    Field<&AudioClipTypeDef::VolumeReductionWhileSpeechPlaying>("VolumeReductionWhileSpeechPlaying"),
    Field<&AudioClipTypeDef::CrossfadeSpeed>("CrossfadeSpeed"),
};
static_assert(FieldsCoverStruct(kAudioClipTypeDefFields),
              "AudioClipTypeDef: a field has no row in kAudioClipTypeDefFields");

} // namespace

FieldTable<AudioClipTypeDef> AudioClipTypeDef::Fields()
{
    return kAudioClipTypeDefFields;
}

void AudioClipTypeDef::ReadFromFile(Stream *in)
{
    ID = in->ReadInt32();
    ReservedChannels = in->ReadInt32();
    VolumeReductionWhileSpeechPlaying = in->ReadInt32();
    CrossfadeSpeed = in->ReadInt32();
    in->ReadInt32(); // reserved
}
