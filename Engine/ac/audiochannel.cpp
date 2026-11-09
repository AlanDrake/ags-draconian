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
#include "ac/audiochannel.h"

#include "ac/common.h" // quitprintf
#include "ac/dynobj/cc_audioclip.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_audio.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "media/audio/audio_system.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern CCAudioClip ccDynamicAudioClip;

int AudioChannel_GetID(ScriptAudioChannel *channel)
{
    return channel->id;
}

int AudioChannel_GetIsPlaying(ScriptAudioChannel *channel)
{
    if (play.fast_forward)
    {
        return 0;
    }

    return AudioChans::ChannelIsPlaying(channel->id) ? 1 : 0;
}

bool AudioChannel_GetIsPaused(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        return voice->is_paused();
    }
    return false;
}

int AudioChannel_GetPanning(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        return voice->get_panning();
    }
    return 0;
}

void AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning)
{
    if ((newPanning < -100) || (newPanning > 100))
        quitprintf("!AudioChannel.Panning: panning value must be between -100 and 100 (passed=%d)", newPanning);

    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        voice->set_panning(newPanning);
    }
}

ScriptAudioClip *AudioChannel_GetPlayingClip(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice && voice->sourceClipID >= 0)
    {
        return &game.audioClips[voice->sourceClipID];
    }
    return nullptr;
}

int AudioChannel_GetPlayingType(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        return voice->sourceClipType;
    }
    return AUDIOTYPE_UNDEFINED;
}

int AudioChannel_GetPosition(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (!voice)
    {
        return 0;
    }
    return voice->get_pos();
}

int AudioChannel_GetPositionMs(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (!voice)
    {
        return 0;
    }
    return voice->get_pos_ms();
}

int AudioChannel_GetLengthMs(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        return voice->get_length_ms();
    }
    return 0;
}

int AudioChannel_GetVolume(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        return voice->get_volume100();
    }
    return 0;
}

int AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume)
{
    if ((newVolume < 0) || (newVolume > 100))
        quitprintf("!AudioChannel.Volume: new value out of range (supplied: %d, range: 0..100)", newVolume);

    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        voice->set_volume100(newVolume);
    }
    return 0;
}

int AudioChannel_GetSpeed(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        return voice->get_speed();
    }
    return 0;
}

void AudioChannel_SetSpeed(ScriptAudioChannel *channel, int new_speed)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        voice->set_speed(new_speed);
    }
}

void AudioChannel_Stop(ScriptAudioChannel *channel)
{
    if (channel->id == SCHAN_SPEECH && play.IsNonBlockingVoiceSpeech())
        stop_voice_nonblocking();
    else
        stop_or_fade_out_channel(channel->id, -1);
}

void AudioChannel_Pause(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        voice->pause();
    }
}

void AudioChannel_Resume(ScriptAudioChannel *channel)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        voice->resume();
    }
}

void AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition)
{
    if (newPosition < 0)
        quitprintf("!AudioChannel.Seek: invalid seek position %d", newPosition);

    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        voice->seek(newPosition);
    }
}

void AudioChannel_SeekMs(ScriptAudioChannel *channel, int newPosition)
{
    if (newPosition < 0)
        quitprintf("!AudioChannel.SeekMs: invalid seek position %d", newPosition);

    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);
    if (voice)
    {
        voice->seek_ms(newPosition);
    }
}

void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos, int maxDist)
{
    auto *voice = AudioChans::GetChannelIfPlaying(channel->id);

    if (voice)
    {
        if ((maxDist < 1) || (thisroom.Width <= maxDist))
        {
            maxDist = ((xPos > thisroom.Width / 2) ? xPos : (thisroom.Width - xPos)) - AMBIENCE_FULL_DIST;
        }

        voice->xSource = (xPos > 0) ? xPos : -1;
        voice->ySource = yPos;
        voice->maximumPossibleDistanceAway = maxDist;
        if (xPos > 0)
        {
            update_directional_sound_vol();
        }
        else
        {
            voice->apply_directional_modifier(0);
        }
    }
}

void AudioChannel_SetRoomLocation2(ScriptAudioChannel *channel, int xPos, int yPos)
{
    AudioChannel_SetRoomLocation(channel, xPos, yPos, 0);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

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
