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

#include "game/audio_clip_def.h"
#include "game/dynobj/script_objects.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

// The play state of an audio clip: what exists only while the game runs. AudioClip holds it with its definition.
struct AudioClipState
{
    // The values of the clip's custom properties that the script set during play, by property name: they hide the
    // definition's values (GameDef) and the schema's defaults (game/world/properties.h). A save does not hold them, on
    // purpose (audio clips are resources, not a part of the game's state): a restore clears them.
    AGS::Common::StringIMap PropertyOverrides;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<AudioClipState> Fields();
};

// An audio clip of the running game: its definition's fields, as the game file says, and its play state. The game's
// clips are Game::AudioClips, by clip index. A clip is its own script object: the managed pool, the script exports and
// the audio code hold the elements' addresses, so the list is built once per game and not resized until unload_game.
class AudioClip : public AudioClipDef, public AudioClipState
{
public:
    AudioClip() = default;
    // Starts a clip from its definition
    explicit AudioClip(const AudioClipDef &def) : AudioClipDef(def) {}
};

int AudioClip_GetFileType(AudioClip *clip);
int AudioClip_GetType(AudioClip *clip);
int AudioClip_GetIsAvailable(AudioClip *clip);
void AudioClip_Stop(AudioClip *clip);
ScriptAudioChannel *AudioClip_Play(AudioClip *clip, int priority, int repeat);
ScriptAudioChannel *AudioClip_PlayFrom(AudioClip *clip, int position, int priority, int repeat);
ScriptAudioChannel *AudioClip_PlayQueued(AudioClip *clip, int priority, int repeat);

// Script API: AudioClip::get_ID
int AudioClip_GetID(AudioClip *clip);

// Script API: AudioClip::get_ScriptName
const char *AudioClip_GetScriptName(AudioClip *clip);

// Script API: AudioClip::get_DefaultPriority
int AudioClip_GetDefaultPriority(AudioClip *clip);

// Script API: AudioClip::get_DefaultRepeat
int AudioClip_GetDefaultRepeat(AudioClip *clip);

// Script API: AudioClip::get_DefaultVolume
int AudioClip_GetDefaultVolume(AudioClip *clip);

// Script API: AudioClip::PlayOnChannel^3
ScriptAudioChannel *AudioClip_PlayOnChannel(AudioClip *clip, int chan, int priority, int repeat);

// Script API: AudioClip::PlayAsType^4
ScriptAudioChannel *AudioClip_PlayAsType(AudioClip *clip, int type, int chan, int priority, int repeat);

// Script API: AudioClip::GetProperty^1
int AudioClip_GetProperty(AudioClip *clip, const char *property);

// Script API: AudioClip::GetTextProperty^1
const char *AudioClip_GetTextProperty(AudioClip *clip, const char *property);

// Script API: AudioClip::SetProperty^2
bool AudioClip_SetProperty(AudioClip *clip, const char *property, int value);

// Script API: AudioClip::SetTextProperty^2
bool AudioClip_SetTextProperty(AudioClip *clip, const char *property, const char *value);

// Script API: AudioClip::GetByName, AudioClip::GetByScriptName
AudioClip *AudioClip_GetByName(const char *name);
