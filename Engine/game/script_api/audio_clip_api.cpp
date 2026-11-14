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
// Script bindings registered by RegisterAudioClipAPI().
//
//=============================================================================
#include "ac/game.h"
#include "game/audio/audio_clip.h"
#include "game/dynobj/cc_audio_channel.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/script_string.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_AudioClip_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptAudioClip, ccDynamicAudioClip, AudioClip_GetByName, const char);
}

RuntimeScriptValue Sc_AudioClip_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetID);
}

RuntimeScriptValue Sc_AudioClip_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioClip, const char, myScriptStringImpl, AudioClip_GetScriptName);
}

RuntimeScriptValue Sc_AudioClip_GetDefaultPriority(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetDefaultPriority);
}

RuntimeScriptValue Sc_AudioClip_GetDefaultRepeat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetDefaultRepeat);
}

RuntimeScriptValue Sc_AudioClip_GetDefaultVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetDefaultVolume);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetFileType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetFileType);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetType);
}

// int | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_GetIsAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioClip, AudioClip_GetIsAvailable);
}

// void | ScriptAudioClip *clip
RuntimeScriptValue Sc_AudioClip_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioClip, AudioClip_Stop);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_Play(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_Play);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int position, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayFrom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT3(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayFrom);
}

// ScriptAudioChannel* | ScriptAudioClip *clip, int priority, int repeat
RuntimeScriptValue Sc_AudioClip_PlayQueued(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayQueued);
}

RuntimeScriptValue Sc_AudioClip_PlayOnChannel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT3(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayOnChannel);
}

RuntimeScriptValue Sc_AudioClip_PlayAsType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptAudioClip, ScriptAudioChannel, ccDynamicAudio, AudioClip_PlayAsType);
}

RuntimeScriptValue Sc_AudioClip_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptAudioClip, AudioClip_GetProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptAudioClip, const char, myScriptStringImpl, AudioClip_GetTextProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptAudioClip, AudioClip_SetProperty, const char);
}

RuntimeScriptValue Sc_AudioClip_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptAudioClip, AudioClip_SetTextProperty, const char, const char);
}

void RegisterAudioClipAPI()
{
    ScFnRegister audioclip_api[] = {
        // [OBSOLETE] GetByName => GetByScriptName
        {"AudioClip::GetByName", API_FN_PAIR(AudioClip_GetByName)},
        {"AudioClip::GetByScriptName", API_FN_PAIR(AudioClip_GetByName)},
        {"AudioClip::Play^2", API_FN_PAIR(AudioClip_Play)},
        {"AudioClip::PlayFrom^3", API_FN_PAIR(AudioClip_PlayFrom)},
        {"AudioClip::PlayQueued^2", API_FN_PAIR(AudioClip_PlayQueued)},
        {"AudioClip::PlayOnChannel^3", API_FN_PAIR(AudioClip_PlayOnChannel)},
        {"AudioClip::PlayAsType^4", API_FN_PAIR(AudioClip_PlayAsType)},
        {"AudioClip::Stop^0", API_FN_PAIR(AudioClip_Stop)},
        {"AudioClip::GetProperty^1", API_FN_PAIR(AudioClip_GetProperty)},
        {"AudioClip::GetTextProperty^1", API_FN_PAIR(AudioClip_GetTextProperty)},
        {"AudioClip::SetProperty^2", API_FN_PAIR(AudioClip_SetProperty)},
        {"AudioClip::SetTextProperty^2", API_FN_PAIR(AudioClip_SetTextProperty)},
        {"AudioClip::get_ID", API_FN_PAIR(AudioClip_GetID)},
        {"AudioClip::get_DefaultPriority", API_FN_PAIR(AudioClip_GetDefaultPriority)},
        {"AudioClip::get_DefaultRepeat", API_FN_PAIR(AudioClip_GetDefaultRepeat)},
        {"AudioClip::get_DefaultVolume", API_FN_PAIR(AudioClip_GetDefaultVolume)},
        {"AudioClip::get_FileType", API_FN_PAIR(AudioClip_GetFileType)},
        {"AudioClip::get_IsAvailable", API_FN_PAIR(AudioClip_GetIsAvailable)},
        {"AudioClip::get_ScriptName", API_FN_PAIR(AudioClip_GetScriptName)},
        {"AudioClip::get_Type", API_FN_PAIR(AudioClip_GetType)},
    };

    ccAddExternalFunctions(audioclip_api);
}
