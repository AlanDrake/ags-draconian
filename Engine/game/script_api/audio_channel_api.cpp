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
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterAudioChannelAPI()
{
    ScFnRegister audiochan_api[] = {
        ScMethod<&AudioChannel_Pause>("AudioChannel::Pause^0"),
        ScMethod<&AudioChannel_Resume>("AudioChannel::Resume^0"),
        ScMethod<&AudioChannel_Seek>("AudioChannel::Seek^1"),
        ScMethod<&AudioChannel_SeekMs>("AudioChannel::SeekMs^1"),
        ScMethod<&AudioChannel_SetRoomLocation2>("AudioChannel::SetRoomLocation^2"),
        ScMethod<&AudioChannel_SetRoomLocation>("AudioChannel::SetRoomLocation^3"),
        ScMethod<&AudioChannel_Stop>("AudioChannel::Stop^0"),
        ScMethod<&AudioChannel_GetID>("AudioChannel::get_ID"),
        ScMethod<&AudioChannel_GetIsPaused>("AudioChannel::get_IsPaused"),
        ScMethod<&AudioChannel_GetIsPlaying>("AudioChannel::get_IsPlaying"),
        ScMethod<&AudioChannel_GetLengthMs>("AudioChannel::get_LengthMs"),
        ScMethod<&AudioChannel_GetPanning>("AudioChannel::get_Panning"),
        ScMethod<&AudioChannel_SetPanning>("AudioChannel::set_Panning"),
        ScMethod<&AudioChannel_GetPlayingClip>("AudioChannel::get_PlayingClip"),
        ScMethod<&AudioChannel_GetPlayingType>("AudioChannel::get_PlayingType"),
        ScMethod<&AudioChannel_GetPosition>("AudioChannel::get_Position"),
        ScMethod<&AudioChannel_GetPositionMs>("AudioChannel::get_PositionMs"),
        ScMethod<&AudioChannel_GetVolume>("AudioChannel::get_Volume"),
        ScMethod<&AudioChannel_SetVolume>("AudioChannel::set_Volume"),
        ScMethod<&AudioChannel_GetSpeed>("AudioChannel::get_Speed"),
        ScMethod<&AudioChannel_SetSpeed>("AudioChannel::set_Speed"),
        // For compatibility with  Ahmet Kamil's (aka Gord10) custom engine
        ScMethod<&AudioChannel_SetSpeed>("AudioChannel::SetSpeed^1"),
    };

    ccAddExternalFunctions(audiochan_api);
}
