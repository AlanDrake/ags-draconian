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
// Script bindings registered by RegisterVideoAPI().
//
//=============================================================================
#include "game/dynobj/script_video_player.h"
#include "game/script_api/bindings.h"
#include "game/video/video_script.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterVideoAPI()
{
    ScFnRegister video_api[] = {ScStatic<&VideoPlayer_Open, ScIntArg<1>>("VideoPlayer::Open"),
                                ScMethod<&VideoPlayer_Play>("VideoPlayer::Play"),
                                ScMethod<&VideoPlayer_Pause>("VideoPlayer::Pause"),
                                ScMethod<&VideoPlayer_NextFrame>("VideoPlayer::NextFrame"),
                                ScMethod<&VideoPlayer_SeekFrame>("VideoPlayer::SeekFrame"),
                                ScMethod<&VideoPlayer_SeekMs>("VideoPlayer::SeekMs"),
                                ScMethod<&VideoPlayer_Stop>("VideoPlayer::Stop"),

                                ScMethod<&VideoPlayer_GetFrame>("VideoPlayer::get_Frame"),
                                ScMethod<&VideoPlayer_GetFrameCount>("VideoPlayer::get_FrameCount"),
                                ScMethod<&VideoPlayer_GetFrameHeight>("VideoPlayer::get_FrameHeight"),
                                ScMethod<&VideoPlayer_GetFrameRate>("VideoPlayer::get_FrameRate"),
                                ScMethod<&VideoPlayer_GetFrameWidth>("VideoPlayer::get_FrameWidth"),
                                ScMethod<&VideoPlayer_GetGraphic>("VideoPlayer::get_Graphic"),
                                ScMethod<&VideoPlayer_GetLengthMs>("VideoPlayer::get_LengthMs"),
                                ScMethod<&VideoPlayer_GetLooping>("VideoPlayer::get_Looping"),
                                ScMethod<&VideoPlayer_SetLooping>("VideoPlayer::set_Looping"),
                                ScMethod<&VideoPlayer_GetPositionMs>("VideoPlayer::get_PositionMs"),
                                ScMethod<&VideoPlayer_GetSpeed>("VideoPlayer::get_Speed"),
                                ScMethod<&VideoPlayer_SetSpeed, ScUndefinedResult>("VideoPlayer::set_Speed"),
                                ScMethod<&VideoPlayer_GetState>("VideoPlayer::get_State"),
                                ScMethod<&VideoPlayer_GetVolume>("VideoPlayer::get_Volume"),
                                ScMethod<&VideoPlayer_SetVolume>("VideoPlayer::set_Volume")};

    ccAddExternalFunctions(video_api);
}
