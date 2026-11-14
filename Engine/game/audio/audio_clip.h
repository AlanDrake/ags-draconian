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

#include "ac/dynobj/scriptaudioclip.h"
#include "game/dynobj/script_objects.h"

int AudioClip_GetFileType(ScriptAudioClip *clip);
int AudioClip_GetType(ScriptAudioClip *clip);
int AudioClip_GetIsAvailable(ScriptAudioClip *clip);
void AudioClip_Stop(ScriptAudioClip *clip);
ScriptAudioChannel *AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat);
ScriptAudioChannel *AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat);
ScriptAudioChannel *AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat);

// Script API: AudioClip::get_ID
int AudioClip_GetID(ScriptAudioClip *clip);

// Script API: AudioClip::get_ScriptName
const char *AudioClip_GetScriptName(ScriptAudioClip *clip);

// Script API: AudioClip::get_DefaultPriority
int AudioClip_GetDefaultPriority(ScriptAudioClip *clip);

// Script API: AudioClip::get_DefaultRepeat
int AudioClip_GetDefaultRepeat(ScriptAudioClip *clip);

// Script API: AudioClip::get_DefaultVolume
int AudioClip_GetDefaultVolume(ScriptAudioClip *clip);

// Script API: AudioClip::PlayOnChannel^3
ScriptAudioChannel *AudioClip_PlayOnChannel(ScriptAudioClip *clip, int chan, int priority, int repeat);

// Script API: AudioClip::PlayAsType^4
ScriptAudioChannel *AudioClip_PlayAsType(ScriptAudioClip *clip, int type, int chan, int priority, int repeat);

// Script API: AudioClip::GetProperty^1
int AudioClip_GetProperty(ScriptAudioClip *clip, const char *property);

// Script API: AudioClip::GetTextProperty^1
const char *AudioClip_GetTextProperty(ScriptAudioClip *clip, const char *property);

// Script API: AudioClip::SetProperty^2
bool AudioClip_SetProperty(ScriptAudioClip *clip, const char *property, int value);

// Script API: AudioClip::SetTextProperty^2
bool AudioClip_SetTextProperty(ScriptAudioClip *clip, const char *property, const char *value);

// Script API: AudioClip::GetByName, AudioClip::GetByScriptName
ScriptAudioClip *AudioClip_GetByName(const char *name);
