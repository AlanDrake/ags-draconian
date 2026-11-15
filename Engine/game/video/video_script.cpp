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
#include "game/video/video_script.h"

#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "game/dynobj/script_video_player.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game/video/video.h"
#include "media/video/videoplayer.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/path.h"

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

void PlayFlic(int numb, int scr_flags)
{
    EndSkippingUntilCharStops();
    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    // Convert PlayFlic flags to common video flags
    /* NOTE: historically using decimal "flags"
    default (0): the video will be played stretched to screen;
        player cannot skip animation; screen will be cleared
    1: player can press ESC to skip animation
    2: player can press any key or click mouse to skip animation
    10: play the video at original size
    100: do not clear the screen before starting playback
    */
    // NOTE: don't enable frame drop with FLIC, or it will play too fast
    // (also see a note about kVideoState_SetGameFps below)
    int video_flags = kVideo_EnableVideo;
    int state_flags = 0;
    VideoSkipType skip = VideoSkipNone;
    // skip type
    switch (scr_flags % 10)
    {
    case 1: skip = VideoSkipEscape; break;
    case 2: skip = VideoSkipKeyOrMouse; break;
    default: skip = VideoSkipNone; break;
    }
    // video size
    switch ((scr_flags % 100) / 10)
    {
    case 1: /* play original size, no flag */ break;
    default: state_flags |= kVideoState_Stretch;
    }
    // clear screen
    switch ((scr_flags % 1000) / 100)
    {
    case 1: /* don't clear screen, no flag */ break;
    default: state_flags |= kVideoState_ClearScreen;
    }

    // NOTE: do not set kVideoState_SetGameFps for FLIC,
    // it seems like existing games featured FLICs with over-top FPS,
    // but limited it to ~60 FPS using vsync.

    HError err = play_flc_video(numb, video_flags, state_flags, skip);
    if (!err)
        debug_script_warn("Failed to play FLIC %d: %s", numb, err->FullMessage().GetCStr());
}

void PlayVideo(const char *name, int skip, int scr_flags)
{
    EndSkippingUntilCharStops();
    if (play.fast_forward)
        return;
    if (debug_flags & DBG_NOVIDEO)
        return;

    // Convert PlayVideo flags to common video flags
    /* NOTE: historically using decimal "flags"
    default (0): the video will be played at original size,
        video's own audio is playing, game sounds muted;
    1: the video will be stretched to full screen;
    10: keep game audio only, video's own audio muted;
    -- since 3.6.0:
    20: play both game audio and video's own audio
    */
    // TODO: some of these may be a part of config (e.g. kVideo_DropFramesUndecoded)
    int video_flags = kVideo_EnableVideo | kVideo_DropFrames | kVideo_DropFramesUndecoded | kVideo_SyncAudioVideo;
    int state_flags = 0;
    // video size
    switch (scr_flags % 10)
    {
    case 1: state_flags |= kVideoState_Stretch; break;
    default: break;
    }
    // audio option
    switch ((scr_flags % 100) / 10)
    {
    case 1: break;                                    // keep game audio, and no video audio
    case 2: video_flags |= kVideo_EnableAudio; break; // have both game and video audio
    default:                                          // play video audio but stop game audio
        video_flags |= kVideo_EnableAudio;
        state_flags |= kVideoState_StopGameAudio;
        break;
    }

    // if audio is disabled, then don't play any sound on the video either
    if (!EngineServices::Current().Config.AudioEnabled)
        video_flags &= ~kVideo_EnableAudio;

    // for old versions: allow slightly offset video frames
    // original engine's behavior was to adjust FPS to match video's,
    // but we may rethink this later (or add an explicit setting).
    // clearing the screen is necessary for software renderer, whenever
    // the video is not stretched to the full screen.
    state_flags |= kVideoState_SetGameFps | kVideoState_ClearScreen;

    String filename = name;
    String ext = Path::GetFileExtension(filename);
    // We only support OGV currently, starting with 3.6.0. If the (presumably older) game
    // is asking for a different type of video, then try to find a video file of same name
    // but OGV extension. This is a workaround for users who can convert the videos in
    // older games into OGV and still see them being played.
    if (ext.CompareNoCase("ogv") != 0)
        filename = Path::ReplaceExtension(filename, "ogv");
    HError err = play_theora_video(filename, video_flags, state_flags, static_cast<VideoSkipType>(skip));
    if (!err)
        debug_script_warn("Failed to play video '%s': %s", name, err->FullMessage().GetCStr());
}
