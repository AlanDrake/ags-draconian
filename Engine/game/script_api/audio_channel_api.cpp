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
// Script bindings registered by RegisterAudioChannelAPI().
//
//=============================================================================
#include "game/audio/audio_channel.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/game.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetID);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetIsPlaying(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetIsPlaying);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPanning);
}

// void | ScriptAudioChannel *channel, int newPanning
RuntimeScriptValue Sc_AudioChannel_SetPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetPanning);
}

// ScriptAudioClip* | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPlayingClip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptAudioChannel, ScriptAudioClip, ccDynamicAudioClip, AudioChannel_GetPlayingClip);
}

RuntimeScriptValue Sc_AudioChannel_GetPlayingType(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPlayingType);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPosition);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetPositionMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetLengthMs);
}

// int | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetVolume);
}

// int | ScriptAudioChannel *channel, int newVolume
RuntimeScriptValue Sc_AudioChannel_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptAudioChannel, AudioChannel_SetVolume);
}

// void | ScriptAudioChannel *channel
RuntimeScriptValue Sc_AudioChannel_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Stop);
}

// void | ScriptAudioChannel *channel, int newPosition
RuntimeScriptValue Sc_AudioChannel_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_Seek);
}

RuntimeScriptValue Sc_AudioChannel_SeekMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SeekMs);
}

RuntimeScriptValue Sc_AudioChannel_SetRoomLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptAudioChannel, AudioChannel_SetRoomLocation);
}

RuntimeScriptValue Sc_AudioChannel_SetRoomLocation2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptAudioChannel, AudioChannel_SetRoomLocation2);
}

RuntimeScriptValue Sc_AudioChannel_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAudioChannel, AudioChannel_GetSpeed);
}

RuntimeScriptValue Sc_AudioChannel_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptAudioChannel, AudioChannel_SetSpeed);
}

RuntimeScriptValue Sc_AudioChannel_Pause(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Pause);
}

RuntimeScriptValue Sc_AudioChannel_Resume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptAudioChannel, AudioChannel_Resume);
}

RuntimeScriptValue Sc_AudioChannel_GetIsPaused(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAudioChannel, AudioChannel_GetIsPaused);
}

void RegisterAudioChannelAPI()
{
    ScFnRegister audiochan_api[] = {
        {"AudioChannel::Pause^0", API_FN_PAIR(AudioChannel_Pause)},
        {"AudioChannel::Resume^0", API_FN_PAIR(AudioChannel_Resume)},
        {"AudioChannel::Seek^1", API_FN_PAIR(AudioChannel_Seek)},
        {"AudioChannel::SeekMs^1", API_FN_PAIR(AudioChannel_SeekMs)},
        {"AudioChannel::SetRoomLocation^2", API_FN_PAIR(AudioChannel_SetRoomLocation2)},
        {"AudioChannel::SetRoomLocation^3", API_FN_PAIR(AudioChannel_SetRoomLocation)},
        {"AudioChannel::Stop^0", API_FN_PAIR(AudioChannel_Stop)},
        {"AudioChannel::get_ID", API_FN_PAIR(AudioChannel_GetID)},
        {"AudioChannel::get_IsPaused", API_FN_PAIR(AudioChannel_GetIsPaused)},
        {"AudioChannel::get_IsPlaying", API_FN_PAIR(AudioChannel_GetIsPlaying)},
        {"AudioChannel::get_LengthMs", API_FN_PAIR(AudioChannel_GetLengthMs)},
        {"AudioChannel::get_Panning", API_FN_PAIR(AudioChannel_GetPanning)},
        {"AudioChannel::set_Panning", API_FN_PAIR(AudioChannel_SetPanning)},
        {"AudioChannel::get_PlayingClip", API_FN_PAIR(AudioChannel_GetPlayingClip)},
        {"AudioChannel::get_PlayingType", API_FN_PAIR(AudioChannel_GetPlayingType)},
        {"AudioChannel::get_Position", API_FN_PAIR(AudioChannel_GetPosition)},
        {"AudioChannel::get_PositionMs", API_FN_PAIR(AudioChannel_GetPositionMs)},
        {"AudioChannel::get_Volume", API_FN_PAIR(AudioChannel_GetVolume)},
        {"AudioChannel::set_Volume", API_FN_PAIR(AudioChannel_SetVolume)},
        {"AudioChannel::get_Speed", API_FN_PAIR(AudioChannel_GetSpeed)},
        {"AudioChannel::set_Speed", API_FN_PAIR(AudioChannel_SetSpeed)},
        // For compatibility with  Ahmet Kamil's (aka Gord10) custom engine
        {"AudioChannel::SetSpeed^1", API_FN_PAIR(AudioChannel_SetSpeed)},
    };

    ccAddExternalFunctions(audiochan_api);
}
