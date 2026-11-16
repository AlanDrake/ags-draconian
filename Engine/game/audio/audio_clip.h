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

using AudioClip = AudioClipDef;

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
