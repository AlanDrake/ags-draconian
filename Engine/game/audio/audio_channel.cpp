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
#include "game/audio/audio_channel.h"

#include "debug/debug_log.h"
#include "game/audio/audio_clip.h"
#include "game/dialog/speech.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "media/audio/audio_system.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h" // quitprintf

using namespace AGS::Common;

int AudioChannel_GetID(ScriptAudioChannel *channel)
{
    return channel->id;
}

int AudioChannel_GetIsPlaying(ScriptAudioChannel *channel)
{
    if (Game::Current().Play.FastForward)
    {
        return 0;
    }

    return AudioChans::ChannelIsPlaying(channel->id) ? 1 : 0;
}

bool AudioChannel_GetIsPaused(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        return playback->is_paused();
    }
    return false;
}

int AudioChannel_GetPanning(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        return playback->get_panning();
    }
    return 0;
}

void AudioChannel_SetPanning(ScriptAudioChannel *channel, int newPanning)
{
    if ((newPanning < -100) || (newPanning > 100))
        quitprintf("!AudioChannel.Panning: panning value must be between -100 and 100 (passed=%d)", newPanning);

    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        playback->set_panning(newPanning);
    }
}

AudioClip *AudioChannel_GetPlayingClip(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback && playback->sourceClipID >= 0)
    {
        return &Game::Current().AudioClips[playback->sourceClipID];
    }
    return nullptr;
}

int AudioChannel_GetPlayingType(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        return playback->sourceClipType;
    }
    return AUDIOTYPE_UNDEFINED;
}

int AudioChannel_GetPosition(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (!playback)
    {
        return 0;
    }
    return playback->get_pos();
}

int AudioChannel_GetPositionMs(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (!playback)
    {
        return 0;
    }
    return playback->get_pos_ms();
}

int AudioChannel_GetLengthMs(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        return playback->get_length_ms();
    }
    return 0;
}

int AudioChannel_GetVolume(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        return playback->get_volume100();
    }
    return 0;
}

int AudioChannel_SetVolume(ScriptAudioChannel *channel, int newVolume)
{
    if ((newVolume < 0) || (newVolume > 100))
        quitprintf("!AudioChannel.Volume: new value out of range (supplied: %d, range: 0..100)", newVolume);

    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        playback->set_volume100(newVolume);
    }
    return 0;
}

int AudioChannel_GetSpeed(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        return playback->get_speed();
    }
    return 0;
}

void AudioChannel_SetSpeed(ScriptAudioChannel *channel, int new_speed)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        playback->set_speed(new_speed);
    }
}

void AudioChannel_Stop(ScriptAudioChannel *channel)
{
    if (channel->id == SCHAN_SPEECH && Game::Current().Play.IsNonBlockingVoiceSpeech())
        stop_voice_nonblocking();
    else
        stop_or_fade_out_channel(channel->id, -1);
}

void AudioChannel_Pause(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        playback->pause();
    }
}

void AudioChannel_Resume(ScriptAudioChannel *channel)
{
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        playback->resume();
    }
}

void AudioChannel_Seek(ScriptAudioChannel *channel, int newPosition)
{
    if (newPosition < 0)
        quitprintf("!AudioChannel.Seek: invalid seek position %d", newPosition);

    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        playback->seek(newPosition);
    }
}

void AudioChannel_SeekMs(ScriptAudioChannel *channel, int newPosition)
{
    if (newPosition < 0)
        quitprintf("!AudioChannel.SeekMs: invalid seek position %d", newPosition);

    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);
    if (playback)
    {
        playback->seek_ms(newPosition);
    }
}

void AudioChannel_SetRoomLocation(ScriptAudioChannel *channel, int xPos, int yPos, int maxDist)
{
    Room &room = Room::Current();
    auto *playback = AudioChans::GetChannelIfPlaying(channel->id);

    if (playback)
    {
        if ((maxDist < 1) || (room.Width <= maxDist))
        {
            maxDist = ((xPos > room.Width / 2) ? xPos : (room.Width - xPos)) - AMBIENCE_FULL_DIST;
        }

        playback->xSource = (xPos > 0) ? xPos : -1;
        playback->ySource = yPos;
        playback->maximumPossibleDistanceAway = maxDist;
        if (xPos > 0)
        {
            update_directional_sound_vol();
        }
        else
        {
            playback->apply_directional_modifier(0);
        }
    }
}

void AudioChannel_SetRoomLocation2(ScriptAudioChannel *channel, int x, int y)
{
    AudioChannel_SetRoomLocation(channel, x, y, 0);
}
