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
#include "ac/dynobj/scriptvideoplayer.h"
#include "ac/video_script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_VideoPlayer_Open(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT2(ScriptVideoPlayer, VideoPlayer_Open, const char);
}

RuntimeScriptValue Sc_VideoPlayer_Play(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Play);
}

RuntimeScriptValue Sc_VideoPlayer_Pause(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Pause);
}

RuntimeScriptValue Sc_VideoPlayer_NextFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_NextFrame);
}

RuntimeScriptValue Sc_VideoPlayer_SeekFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptVideoPlayer, VideoPlayer_SeekFrame);
}

RuntimeScriptValue Sc_VideoPlayer_SeekMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptVideoPlayer, VideoPlayer_SeekMs);
}

RuntimeScriptValue Sc_VideoPlayer_Stop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptVideoPlayer, VideoPlayer_Stop);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrame);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameCount);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameHeight);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameRate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptVideoPlayer, VideoPlayer_GetFrameRate);
}

RuntimeScriptValue Sc_VideoPlayer_GetFrameWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetFrameWidth);
}

RuntimeScriptValue Sc_VideoPlayer_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetGraphic);
}

RuntimeScriptValue Sc_VideoPlayer_GetLengthMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetLengthMs);
}

RuntimeScriptValue Sc_VideoPlayer_GetLooping(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetLooping);
}

RuntimeScriptValue Sc_VideoPlayer_SetLooping(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptVideoPlayer, VideoPlayer_SetLooping);
}

RuntimeScriptValue Sc_VideoPlayer_GetPositionMs(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetPositionMs);
}

RuntimeScriptValue Sc_VideoPlayer_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptVideoPlayer, VideoPlayer_GetSpeed);
}

RuntimeScriptValue Sc_VideoPlayer_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptVideoPlayer, VideoPlayer_SetSpeed);
}

RuntimeScriptValue Sc_VideoPlayer_GetState(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetState);
}

RuntimeScriptValue Sc_VideoPlayer_GetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptVideoPlayer, VideoPlayer_GetVolume);
}

RuntimeScriptValue Sc_VideoPlayer_SetVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptVideoPlayer, VideoPlayer_SetVolume);
}


void RegisterVideoAPI()
{
    ScFnRegister video_api[] = {{"VideoPlayer::Open", API_FN_PAIR(VideoPlayer_Open)},
                                {"VideoPlayer::Play", API_FN_PAIR(VideoPlayer_Play)},
                                {"VideoPlayer::Pause", API_FN_PAIR(VideoPlayer_Pause)},
                                {"VideoPlayer::NextFrame", API_FN_PAIR(VideoPlayer_NextFrame)},
                                {"VideoPlayer::SeekFrame", API_FN_PAIR(VideoPlayer_SeekFrame)},
                                {"VideoPlayer::SeekMs", API_FN_PAIR(VideoPlayer_SeekMs)},
                                {"VideoPlayer::Stop", API_FN_PAIR(VideoPlayer_Stop)},

                                {"VideoPlayer::get_Frame", API_FN_PAIR(VideoPlayer_GetFrame)},
                                {"VideoPlayer::get_FrameCount", API_FN_PAIR(VideoPlayer_GetFrameCount)},
                                {"VideoPlayer::get_FrameHeight", API_FN_PAIR(VideoPlayer_GetFrameHeight)},
                                {"VideoPlayer::get_FrameRate", API_FN_PAIR(VideoPlayer_GetFrameRate)},
                                {"VideoPlayer::get_FrameWidth", API_FN_PAIR(VideoPlayer_GetFrameWidth)},
                                {"VideoPlayer::get_Graphic", API_FN_PAIR(VideoPlayer_GetGraphic)},
                                {"VideoPlayer::get_LengthMs", API_FN_PAIR(VideoPlayer_GetLengthMs)},
                                {"VideoPlayer::get_Looping", API_FN_PAIR(VideoPlayer_GetLooping)},
                                {"VideoPlayer::set_Looping", API_FN_PAIR(VideoPlayer_SetLooping)},
                                {"VideoPlayer::get_PositionMs", API_FN_PAIR(VideoPlayer_GetPositionMs)},
                                {"VideoPlayer::get_Speed", API_FN_PAIR(VideoPlayer_GetSpeed)},
                                {"VideoPlayer::set_Speed", API_FN_PAIR(VideoPlayer_SetSpeed)},
                                {"VideoPlayer::get_State", API_FN_PAIR(VideoPlayer_GetState)},
                                {"VideoPlayer::get_Volume", API_FN_PAIR(VideoPlayer_GetVolume)},
                                {"VideoPlayer::set_Volume", API_FN_PAIR(VideoPlayer_SetVolume)}};

    ccAddExternalFunctions(video_api);
}
