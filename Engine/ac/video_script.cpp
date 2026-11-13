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
#include "ac/video_script.h"

#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptvideoplayer.h"
#include "debug/debug_log.h"
#include "media/video/video.h"
#include "media/video/videoplayer.h"

using namespace AGS::Common;
using namespace AGS::Engine;

ScriptVideoPlayer *VideoPlayer_Open(const char *filename, bool auto_play, int repeat_style)
{
    int video_id;
    HError err = open_video(filename, kVideo_EnableVideo | kVideo_EnableAudio | kVideo_DropFrames, video_id);
    if (!err)
    {
        debug_script_warn("Failed to play video '%s': %s", filename, err->FullMessage().GetCStr());
        return nullptr;
    }

    VideoControl *video_ctrl = get_video_control(video_id);
    video_ctrl->SetLooping((repeat_style) ? true : false);
    if (auto_play)
    {
        video_ctrl->Play();
    }

    ScriptVideoPlayer *sc_video = new ScriptVideoPlayer(video_id);
    int handle = ccRegisterManagedObject(sc_video, sc_video);
    video_ctrl->SetScriptHandle(handle);
    return sc_video;
}

void VideoPlayer_Play(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->Play();
}

void VideoPlayer_Pause(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->Pause();
}

void VideoPlayer_NextFrame(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->NextFrame();
}

int VideoPlayer_SeekFrame(ScriptVideoPlayer *sc_video, int frame)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SeekFrame(frame);
}

int VideoPlayer_SeekMs(ScriptVideoPlayer *sc_video, int pos)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SeekMs(pos);
}

void VideoPlayer_Stop(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return;
    video_stop(sc_video->GetID());
    sc_video->Invalidate();
}

int VideoPlayer_GetFrame(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrame();
}

int VideoPlayer_GetFrameCount(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameCount();
}

int VideoPlayer_GetFrameHeight(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameSize().Height;
}

float VideoPlayer_GetFrameRate(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0.f;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameRate();
}

int VideoPlayer_GetFrameWidth(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetFrameSize().Width;
}

int VideoPlayer_GetGraphic(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetSpriteID();
}

int VideoPlayer_GetLengthMs(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetDurationMs();
}

int VideoPlayer_GetLooping(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return false;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetLooping();
}

void VideoPlayer_SetLooping(ScriptVideoPlayer *sc_video, bool loop)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->SetLooping(loop);
}

int VideoPlayer_GetPositionMs(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetPositionMs();
}

float VideoPlayer_GetSpeed(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0.f;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetSpeed();
}

void VideoPlayer_SetSpeed(ScriptVideoPlayer *sc_video, float speed)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->SetSpeed(speed);
}

int VideoPlayer_GetState(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return PlaybackState::PlayStateInvalid;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetState();
}

int VideoPlayer_GetVolume(ScriptVideoPlayer *sc_video)
{
    if (sc_video->GetID() < 0)
        return 0;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    return video_ctrl->GetVolume();
}

void VideoPlayer_SetVolume(ScriptVideoPlayer *sc_video, int volume)
{
    if (sc_video->GetID() < 0)
        return;
    VideoControl *video_ctrl = get_video_control(sc_video->GetID());
    video_ctrl->SetVolume(volume);
}

//=============================================================================
