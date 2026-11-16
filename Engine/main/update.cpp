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
// Game update procedure
//

#include "main/update.h"

#include <math.h>

#include "device/sys_events.h"
#include "game/dialog/lipsync.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/render/overlay.h"
#include "game/render/screen_overlay.h"
#include "game/world/character.h"
#include "game/world/character_extras.h"
#include "game/world/move_list.h"
#include "game/world/object.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_object.h"
#include "game/world/room_status.h"
#include "game/world/view_frame.h"
#include "game/world/walkable_area.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "util/common.h"
#include "util/timer.h"


using namespace AGS::Common;
using namespace AGS::Engine;


// Receives and modifies movelist_id and entity coordinates
// Returns whether to there's a need to update the sprite due to a change in direction
MoveResult do_movelist_move(short &mslot, int &pos_x, int &pos_y, bool do_step)
{
    assert(mslot > 0);
    if (mslot < 1)
        return kMoveResult_Abort;

    MoveList &cmls = *get_movelist(mslot);
    const int old_stage = cmls.GetStage();
    MoveResult move_result = kMoveResult_Continue;
    if (cmls.Forward(do_step))
    {
        pos_x = cmls.GetCurrentPos().X;
        pos_y = cmls.GetCurrentPos().Y;
        if (cmls.GetStage() != old_stage)
            move_result = kMoveResult_NextStage;
    }
    else
    {
        pos_x = cmls.GetCurrentPos().X;
        pos_y = cmls.GetCurrentPos().Y;
        // reset MoveList, and tell moving object to stop
        remove_movelist(mslot);
        mslot = 0; // movelist 0 means "not moving/walking"
        move_result = kMoveResult_Done;
    }
    return move_result;
}

void update_script_timers()
{
    Game &game = Game::Current();
    // Deprecated "graphical script" timer (use unknown)
    if (game.Play.gscript_timer > 0)
        game.Play.gscript_timer--;

    game.Play.UpdateScriptTimers();
}

void update_cycling_views()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    // update graphics for object if cycling view
    for (uint32_t i = 0; i < room.State->numobj; ++i)
    {
        room.Objs[i].UpdateCyclingView(game, i);
    }
}

// Updates the view of the player character
void update_player_view()
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (game.PlayerChar->flags & CHF_FIXVIEW)
        return; // view is locked

    int onwalkarea = get_walkable_area_at_character(game.playercharacter);
    if (onwalkarea < 0)
        return; // error?

    int areaview = room.WalkAreas[onwalkarea].PlayerView;
    if (areaview > 0)
        game.PlayerChar->view = areaview - 1; // convert to 0-based id
    else if (room.Options.PlayerView > 0)
        game.PlayerChar->view = room.Options.PlayerView - 1; // convert to 0-based id
    else
        game.PlayerChar->view = game.PlayerChar->defview;
}

void update_character_move_and_anim(std::vector<int> &following_as_sheep)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    // move & animate characters
    for (int aa = 0; aa < game.numcharacters; aa++)
    {
        if (!game.chars[aa].is_enabled())
            continue;

        CharacterInfo *ch = &game.chars[aa];
        CharacterExtras *chex = &game.CharExtras[aa];

        UpdateCharacterMoveAndAnim(game, room, ch, chex, following_as_sheep);
    }
}

void update_following_exactly_characters(const std::vector<int> &following_as_sheep)
{
    Game &game = Game::Current();
    // update location of all following_exactly characters
    for (size_t i = 0; i < following_as_sheep.size(); ++i)
    {
        CharacterInfo *ch = &game.chars[following_as_sheep[i]];

        UpdateFollowingExactlyCharacter(game, ch);
    }
}

void update_overlay_timers()
{
    // update overlay timers
    auto &overs = get_overlays();
    for (auto &over : overs)
    {
        if (over.GetTimeout() > 0)
        {
            if (over.UpdateTimeout() == 0)
            {
                remove_screen_overlay(over.GetID());
            }
        }
    }
}

void update_speech_and_messages()
{
    Game &game = Game::Current();
    bool is_voice_playing = false;
    if (game.Play.speech_voice_blocking)
    {
        auto *playback = AudioChans::GetChannel(SCHAN_SPEECH);
        is_voice_playing = playback && playback->is_ready();
    }
    // determine if speech text should be removed
    // TODO: this code is similar to the Display's box update (see DisplayMessageState::Run),
    // can we merge these two pieces of code? a kind of uniform DisplayText state?
    if (game.Play.messagetime >= 0)
    {
        game.Play.messagetime--;
        // extend life of text if the voice hasn't finished yet
        if (game.Play.speech_voice_blocking && !game.Play.speech_in_post_state)
        {
            if ((is_voice_playing) && (game.Play.fast_forward == 0))
            {
                if (game.Play.messagetime <= 1)
                    game.Play.messagetime = 1;
            }
            // if the voice has finished, and we're not required to wait for text, then remove the speech
            else if (!game.Play.speech_always_wait_for_text)
            {
                // if the voice has finished, remove the speech
                game.Play.messagetime = 0;
            }
        }

        // Enter speech post-state: optionally increase final waiting time
        if (!game.Play.speech_in_post_state && (game.Play.fast_forward == 0) && (game.Play.messagetime < 1))
        {
            game.Play.speech_in_post_state = true;
            if (game.Play.speech_display_post_time_ms > 0)
            {
                game.Play.messagetime = ::lround(game.Play.speech_display_post_time_ms * get_game_fps() / 1000.0f);
            }
        }

        if (game.Play.messagetime < 1)
        {
            if (game.Play.fast_forward > 0)
            {
                remove_screen_overlay(game.Play.text_overlay_on);
                game.Play.SetWaitSkipResult(SKIP_AUTOTIMER);
            }
            else if (game.Play.speech_skip_style & SKIP_AUTOTIMER)
            {
                remove_screen_overlay(game.Play.text_overlay_on);
                game.Play.SetWaitSkipResult(SKIP_AUTOTIMER);
                game.Play.SetIgnoreInput(game.Play.ignore_user_input_after_text_timeout_ms);
            }
        }
    }
}

bool has_voice_lipsync()
{
    Game &game = Game::Current();
    return game.Play.speech_voice_blocking && (game.LipSync.CurLine >= 0);
}

int update_voice_lipsync(int frame)
{
    Game &game = Game::Current();
    if (!has_voice_lipsync())
        return frame; // no change

    auto *playback = AudioChans::GetChannel(SCHAN_SPEECH);
    const int voice_pos_ms = playback ? playback->get_pos_ms() : -1;
    if (voice_pos_ms < 0)
        return frame; // no change

    // check voice lip sync
    if (game.LipSync.CurLinePhoneme >= game.LipSync.Lines[game.LipSync.CurLine].numPhonemes)
    {
        // the lip-sync has finished, so just stay idle
        return frame;
    }
    else
    {
        while ((game.LipSync.CurLinePhoneme < game.LipSync.Lines[game.LipSync.CurLine].numPhonemes) &&
               ((game.LipSync.CurLinePhoneme < 0) ||
                (voice_pos_ms >= game.LipSync.Lines[game.LipSync.CurLine].endtimeoffs[game.LipSync.CurLinePhoneme])))
        {
            game.LipSync.CurLinePhoneme++;
            if (game.LipSync.CurLinePhoneme >= game.LipSync.Lines[game.LipSync.CurLine].numPhonemes)
                frame = game.default_lipsync_frame;
            else
                frame = game.LipSync.Lines[game.LipSync.CurLine].frame[game.LipSync.CurLinePhoneme];

            if (frame >= game.Views[game.Speech.FaceTalkView].loops[game.Speech.FaceTalkLoop].numFrames)
                frame = 0;
        }
        return frame;
    }
}

void update_sierra_speech()
{
    Game &game = Game::Current();
    // Update sierra-style portrait (if active)
    if ((game.Speech.FaceTalking >= 0) && (game.Play.fast_forward == 0))
    {
        int updatedFrame = 0;

        if ((game.Speech.FaceTalkChar->blinkview > 0) && (game.Speech.FaceTalkAllowBlink))
        {
            if (game.Speech.FaceTalkChar->blinktimer > 0)
            {
                // countdown to playing blink anim
                game.Speech.FaceTalkChar->blinktimer--;
                if (game.Speech.FaceTalkChar->blinktimer == 0)
                {
                    game.Speech.FaceTalkChar->blinkframe = 0;
                    game.Speech.FaceTalkChar->blinktimer = -1;
                    updatedFrame = 2;
                }
            }
            else if (game.Speech.FaceTalkChar->blinktimer < 0)
            {
                // currently playing blink anim
                if (game.Speech.FaceTalkChar->blinktimer < ((0 - 6) - game.Views[game.Speech.FaceTalkChar->blinkview]
                                                                          .loops[game.Speech.FaceTalkBlinkLoop]
                                                                          .frames[game.Speech.FaceTalkChar->blinkframe]
                                                                          .speed))
                {
                    // time to advance to next frame
                    game.Speech.FaceTalkChar->blinktimer = -1;
                    game.Speech.FaceTalkChar->blinkframe++;
                    updatedFrame = 2;
                    if (game.Speech.FaceTalkChar->blinkframe >=
                        game.Views[game.Speech.FaceTalkChar->blinkview].loops[game.Speech.FaceTalkBlinkLoop].numFrames)
                    {
                        game.Speech.FaceTalkChar->blinkframe = 0;
                        game.Speech.FaceTalkChar->blinktimer = game.Speech.FaceTalkChar->blinkinterval;
                    }
                }
                else
                    game.Speech.FaceTalkChar->blinktimer--;
            }
        }

        const int old_facetalkframe = game.Speech.FaceTalkFrame;
        if (has_voice_lipsync())
        {
            game.Speech.FaceTalkFrame = update_voice_lipsync(game.Speech.FaceTalkFrame);
        }
        else if (game.Speech.FaceTalkWait > 0)
        {
            game.Speech.FaceTalkWait--;
        }
        // don't animate if the speech has finished
        else if ((game.Play.messagetime < 1) && (game.Speech.FaceTalkFrame == 0) &&
                 // if game.Play.close_mouth_speech_time = 0, this means animation should play till
                 // the speech ends; but this should not work in voice mode, and also if the
                 // speech is in the "post" state
                 (game.Play.speech_voice_blocking || game.Play.speech_in_post_state ||
                  game.Play.close_mouth_speech_time > 0))
        {
        }
        else
        {
            // Close mouth at end of sentence: if speech has entered the "post" state,
            // or if this is a text only mode and close_mouth_speech_time is set
            if (game.Play.speech_in_post_state ||
                (!game.Play.speech_voice_blocking && (game.Play.messagetime < game.Play.close_mouth_speech_time) &&
                 (game.Play.close_mouth_speech_time > 0)))
            {
                game.Speech.FaceTalkFrame = 0;
                game.Speech.FaceTalkWait = game.Play.messagetime;
            }
            else if ((game.options[OPT_LIPSYNCTEXT]) && (game.Speech.FaceTalkRepeat > 0))
            {
                // lip-sync speech (and not a thought)
                game.Speech.FaceTalkWait =
                    update_lip_sync(game.Speech.FaceTalkView, game.Speech.FaceTalkLoop, &game.Speech.FaceTalkFrame);
                // It is actually displayed for Game::Speech.FaceTalkWait+1 loops
                // (because when it's 1, it gets --'d then wait for next time)
                game.Speech.FaceTalkWait--;
            }
            else
            {
                // normal non-lip-sync
                game.Speech.FaceTalkFrame++;
                if ((game.Speech.FaceTalkFrame >=
                     game.Views[game.Speech.FaceTalkView].loops[game.Speech.FaceTalkLoop].numFrames) ||
                    (!game.Play.speech_voice_blocking && (game.Play.messagetime < 1) &&
                     (game.Play.close_mouth_speech_time > 0)))
                {
                    if ((game.Speech.FaceTalkFrame >=
                         game.Views[game.Speech.FaceTalkView].loops[game.Speech.FaceTalkLoop].numFrames) &&
                        (game.Views[game.Speech.FaceTalkView].loops[game.Speech.FaceTalkLoop].RunNextLoop()))
                    {
                        game.Speech.FaceTalkLoop++;
                    }
                    else
                    {
                        game.Speech.FaceTalkLoop = 0;
                    }
                    game.Speech.FaceTalkFrame = 0;
                    if (!game.Speech.FaceTalkRepeat)
                        game.Speech.FaceTalkWait = 999999;
                }
                if ((game.Speech.FaceTalkFrame != 0) || (game.Speech.FaceTalkRepeat == 1))
                    game.Speech.FaceTalkWait = game.Views[game.Speech.FaceTalkView]
                                                   .loops[game.Speech.FaceTalkLoop]
                                                   .frames[game.Speech.FaceTalkFrame]
                                                   .speed +
                                               Character_GetSpeechAnimationDelay(game.Speech.FaceTalkChar);
            }
        }

        if (game.Speech.FaceTalkFrame != old_facetalkframe)
        {
            updatedFrame |= 1;
        }

        // is_text_overlay might be 0 if it was only just destroyed this loop
        if ((updatedFrame) && (game.Play.text_overlay_on > 0))
        {
            const auto &talking_chex = game.CharExtras[game.Speech.FaceTalkChar->index_id];
            const int frame_vol = talking_chex.GetFrameSoundVolume(game.Speech.FaceTalkChar);
            if (updatedFrame & 1)
                CheckViewFrame(game.Speech.FaceTalkView, game.Speech.FaceTalkLoop, game.Speech.FaceTalkFrame,
                               frame_vol);
            if (updatedFrame & 2)
                CheckViewFrame(game.Speech.FaceTalkChar->blinkview, game.Speech.FaceTalkBlinkLoop,
                               game.Speech.FaceTalkChar->blinkframe, frame_vol);

            int thisPic = game.Views[game.Speech.FaceTalkView]
                              .loops[game.Speech.FaceTalkLoop]
                              .frames[game.Speech.FaceTalkFrame]
                              .pic;
            int view_frame_x = 0;
            int view_frame_y = 0;

            auto *face_over = get_overlay(game.Speech.FaceTalking);
            assert(face_over != nullptr);
            Bitmap *frame_pic = EngineServices::Current().Sprites[face_over->GetSpriteNum()];
            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
            {
                // QFG4-style fullscreen dialog
                if (game.Speech.Qfg4OverridePlacementX)
                {
                    view_frame_x = game.Play.speech_portrait_x;
                }
                if (game.Speech.Qfg4OverridePlacementY)
                {
                    view_frame_y = game.Play.speech_portrait_y;
                }
                else
                {
                    view_frame_y = (frame_pic->GetHeight() / 2) - (game.SpriteInfos[thisPic].Height / 2);
                }
                frame_pic->Clear(0);
            }
            else
            {
                frame_pic->ClearTransparent();
            }

            const ViewFrame *face_vf =
                &game.Views[game.Speech.FaceTalkView].loops[game.Speech.FaceTalkLoop].frames[game.Speech.FaceTalkFrame];
            DrawViewFrame(frame_pic, face_vf, view_frame_x, view_frame_y);

            if ((game.Speech.FaceTalkChar->blinkview > 0) && (game.Speech.FaceTalkChar->blinktimer < 0))
            {
                ViewFrame *blink_vf = &game.Views[game.Speech.FaceTalkChar->blinkview]
                                           .loops[game.Speech.FaceTalkBlinkLoop]
                                           .frames[game.Speech.FaceTalkChar->blinkframe];
                // draw the blinking sprite on top
                DrawViewFrame(frame_pic, blink_vf, view_frame_x, view_frame_y);
            }

            // Make sure overlay texture will get updated on screen
            game_sprite_updated(face_over->GetSpriteNum());
            face_over->MarkChanged();
        } // end if updatedFrame
    }
}

void update_game_objects()
{
    set_our_eip(20);

    update_script_timers();

    update_cycling_views();

    set_our_eip(21);

    update_player_view();

    set_our_eip(22);

    std::vector<int> following_as_sheep;

    update_character_move_and_anim(following_as_sheep);

    update_following_exactly_characters(following_as_sheep);

    set_our_eip(23);

    update_overlay_timers();

    update_speech_and_messages();

    set_our_eip(24);

    update_sierra_speech();

    set_our_eip(25);
}
