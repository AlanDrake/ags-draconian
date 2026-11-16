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
#include "game/world/view_frame.h"

#include "debug/debug_log.h"
#include "game/dynobj/script_string.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_version.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "gfx/bitmap.h"
#include "gfx/sprite_cache.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h" // quitprintf
#include "util/math.h"

using namespace AGS;
using namespace AGS::Common;


const char *ViewFrame_GetEventName(ScriptViewFrame *svf)
{
    return CreateNewScriptString(Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].event_name);
}

void ViewFrame_SetEventName(ScriptViewFrame *svf, const char *name)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].event_name = name;
}

int ViewFrame_GetFlipped(ScriptViewFrame *svf)
{
    // We can return GraphicFlip here, because old boolean value matches horizontal flip
    return GfxDef::GetFlipFromFlags(Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].flags);
}

void ViewFrame_SetFlipped(ScriptViewFrame *svf, int flip)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].flags =
        GfxDef::GetFlagsFromFlip((GraphicFlip)flip);
}

int ViewFrame_GetGraphic(ScriptViewFrame *svf)
{
    return Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].pic;
}

void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].pic = newPic;
}

ScriptAudioClip *ViewFrame_GetLinkedAudio(ScriptViewFrame *svf)
{
    Game &game = Game::Current();
    int sound_index = game.Views[svf->view].loops[svf->loop].frames[svf->frame].sound;
    if (sound_index < 0)
        return nullptr;

    return &game.audioClips[sound_index];
}

void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, ScriptAudioClip *clip)
{
    int newSoundIndex = -1;
    if (clip != nullptr)
        newSoundIndex = clip->id;

    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].sound = newSoundIndex;
}

int ViewFrame_GetSpeed(ScriptViewFrame *svf)
{
    return Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].speed;
}

void ViewFrame_SetSpeed(ScriptViewFrame *svf, int speed)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].speed = speed;
}

int ViewFrame_GetXOffset(ScriptViewFrame *svf)
{
    return Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].xoffs;
}

void ViewFrame_SetXOffset(ScriptViewFrame *svf, int xoff)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].xoffs = xoff;
}

int ViewFrame_GetYOffset(ScriptViewFrame *svf)
{
    return Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].xoffs;
}

void ViewFrame_SetYOffset(ScriptViewFrame *svf, int yoff)
{
    Game::Current().Views[svf->view].loops[svf->loop].frames[svf->frame].yoffs = yoff;
}

int ViewFrame_GetView(ScriptViewFrame *svf)
{
    return svf->view + 1;
}

int ViewFrame_GetLoop(ScriptViewFrame *svf)
{
    return svf->loop;
}

int ViewFrame_GetFrame(ScriptViewFrame *svf)
{
    return svf->frame;
}

//=============================================================================

int CalcFrameSoundVolume(int obj_vol, int anim_vol, int scale)
{
    // We view the audio property relation as the relation of the entities:
    // system -> audio type -> audio emitter (object, character) -> animation's audio
    // therefore the sound volume is a multiplication of factors.
    int frame_vol = 100; // default to full volume
    // Object's animation volume property
    frame_vol = frame_vol * obj_vol / 100;
    // Active animation volume
    frame_vol = frame_vol * anim_vol / 100;
    // Zoom volume scaling (optional)
    // NOTE: historically scales only in 0-100 range :/
    scale = Math::Clamp(scale, 0, 100);
    frame_vol = frame_vol * scale / 100;
    return frame_vol;
}

bool PlayViewFrameSound(int view, int loop, int frame, int sound_volume, int sound_pan, int sound_speed)
{
    Game &game = Game::Current();
    if (game.Views[view].loops[loop].frames[frame].sound < 0)
        return false;

    // play this sound (eg. footstep)
    ScriptAudioChannel *channel = play_audio_clip_by_index(game.Views[view].loops[loop].frames[frame].sound);
    if (channel)
    {
        auto *playback = AudioChans::GetChannel(channel->id);
        if (playback)
        {
            playback->set_volume100(playback->get_volume100() * Math::Clamp(sound_volume, 0, 100) / 100);
            playback->set_panning(sound_pan);
            playback->set_speed(sound_speed);
        }
        return true; // return positive if sound is linked, regardless of the playback success
    }
    return false;
}

bool RunViewFrameEvent(int view, int loop, int frame, const ObjectEvent &obj_evt, Common::ScriptEventsBase *handlers,
                       int evnt)
{
    Game &game = Game::Current();
    if (!game.Views[view].loops[loop].frames[frame].event_name.IsEmpty() && obj_evt && handlers->HasHandler(evnt))
    {
        ObjectEvent evt_ext = obj_evt;
        evt_ext.Params[1] = RuntimeScriptValue().SetInt32(view);
        evt_ext.Params[2] = RuntimeScriptValue().SetInt32(loop);
        evt_ext.Params[3] = RuntimeScriptValue().SetInt32(frame);
        auto dyn_str = ScriptString::Create(game.Views[view].loops[loop].frames[frame].event_name.GetCStr());
        evt_ext.Params[4] = RuntimeScriptValue().SetScriptObject(dyn_str.Obj(), dyn_str.Mgr());
        evt_ext.ParamCount = 5;
        // This event is run on either blocking or non-blocking thread
        run_event_script_always(evt_ext, handlers, evnt);
        return true;
    }

    return false;
}

void CheckViewFrame(int view, int loop, int frame, int sound_volume, int sound_pan, int sound_speed)
{
    CheckViewFrame(view, loop, frame, sound_volume, sound_pan, sound_speed, ObjectEvent(), nullptr, 0);
}

// Handle the new animation frame (play linked sounds, etc)
void CheckViewFrame(int view, int loop, int frame, int sound_volume, int sound_pan, int sound_speed,
                    const ObjectEvent &obj_evt, ScriptEventsBase *handlers, int evnt)
{
    PlayViewFrameSound(view, loop, frame, sound_volume, sound_pan, sound_speed);
    RunViewFrameEvent(view, loop, frame, obj_evt, handlers, evnt);
}

// Note: the following function is only used for speech views in update_sierra_speech() and _displayspeech()
// draws a view frame, flipped if appropriate
void DrawViewFrame(Bitmap *ds, const ViewFrame *vframe, int x, int y)
{
    Bitmap *vf_bmp = EngineServices::Current().Sprites[vframe->pic];
    if ((ds->GetColorDepth() == 32) && (vf_bmp->GetColorDepth() == 32))
    {
        Bitmap *src = vf_bmp;
        if (GfxDef::FlagsHaveFlip(vframe->flags))
        {
            src = new Bitmap(vf_bmp->GetWidth(), vf_bmp->GetHeight(), vf_bmp->GetColorDepth());
            src->FlipBlt(vf_bmp, 0, 0, GfxDef::GetFlipFromFlags(vframe->flags));
        }
        draw_sprite_support_alpha(ds, x, y, src);
        if (src != vf_bmp)
            delete src;
    }
    else
    {
        if (GfxDef::FlagsHaveFlip(vframe->flags))
            ds->FlipBlt(vf_bmp, x, y, GfxDef::GetFlipFromFlags(vframe->flags));
        else
            ds->Blit(vf_bmp, x, y, Common::kBitmap_Transparency);
    }
}

GraphicFlip GetViewFrameFlip(int view, int loop, int frame, int pic)
{
    Game &game = Game::Current();
    if (view < 0 || static_cast<size_t>(view) >= game.Views.size())
        return kFlip_None;
    const ViewStruct &vs = game.Views[view];
    if (loop < 0 || loop >= vs.numLoops || frame < 0 || frame >= vs.loops[loop].numFrames)
        return kFlip_None;
    const ViewFrame &vf = vs.loops[loop].frames[frame];
    if (pic >= 0 && vf.pic != pic)
        return kFlip_None;
    return GfxDef::GetFlipFromFlags(vf.flags);
}

#include "script/script_runtime.h"
