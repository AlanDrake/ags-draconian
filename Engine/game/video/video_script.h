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
// VideoPlayer script API.
//
//=============================================================================
#pragma once

struct ScriptVideoPlayer;

// Script API: VideoPlayer::Open
ScriptVideoPlayer *VideoPlayer_Open(const char *filename, bool auto_play, int repeat_style);

// Script API: VideoPlayer::Play
void VideoPlayer_Play(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::Pause
void VideoPlayer_Pause(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::NextFrame
void VideoPlayer_NextFrame(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::SeekFrame
int VideoPlayer_SeekFrame(ScriptVideoPlayer *sc_video, int frame);

// Script API: VideoPlayer::SeekMs
int VideoPlayer_SeekMs(ScriptVideoPlayer *sc_video, int pos);

// Script API: VideoPlayer::Stop
void VideoPlayer_Stop(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_Frame
int VideoPlayer_GetFrame(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_FrameCount
int VideoPlayer_GetFrameCount(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_FrameHeight
int VideoPlayer_GetFrameHeight(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_FrameRate
float VideoPlayer_GetFrameRate(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_FrameWidth
int VideoPlayer_GetFrameWidth(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_Graphic
int VideoPlayer_GetGraphic(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_LengthMs
int VideoPlayer_GetLengthMs(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_Looping
int VideoPlayer_GetLooping(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::set_Looping
void VideoPlayer_SetLooping(ScriptVideoPlayer *sc_video, bool loop);

// Script API: VideoPlayer::get_PositionMs
int VideoPlayer_GetPositionMs(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_Speed
float VideoPlayer_GetSpeed(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::set_Speed
void VideoPlayer_SetSpeed(ScriptVideoPlayer *sc_video, float speed);

// Script API: VideoPlayer::get_State
int VideoPlayer_GetState(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::get_Volume
int VideoPlayer_GetVolume(ScriptVideoPlayer *sc_video);

// Script API: VideoPlayer::set_Volume
void VideoPlayer_SetVolume(ScriptVideoPlayer *sc_video, int volume);

void PlayFlic(int numb, int flags);
void PlayVideo(const char *name, int skip, int flags);
