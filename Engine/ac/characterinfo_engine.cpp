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
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h" // GetGameSpeed
#include "ac/math.h"
#include "ac/object.h"
#include "ac/viewframe.h"
#include "debug/debug_log.h"
#include "game/roomstruct.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"

using namespace AGS::Common;

extern std::vector<ViewStruct> views;
extern GameSetupStruct game;
extern int displayed_room;
extern RoomStruct thisroom;
extern int char_speaking_anim;

int CharacterInfo::get_baseline() const
{
    if (baseline < 1)
    {
        return y;
    }
    return baseline;
}

int CharacterInfo::get_blocking_top() const
{
    if (blocking_height > 0)
    {
        return y - blocking_height / 2;
    }
    return y - 2;
}

int CharacterInfo::get_blocking_bottom() const
{
    // the blocking_bottom should be 1 less than the top + height
    // since the code does <= checks on it rather than < checks
    if (blocking_height > 0)
    {
        return (y + (blocking_height + 1) / 2) - 1;
    }
    return y + 3;
}

// Tests if frame reached or exceeded the loop, and resets frame back
// to either 1st walking frame if character is walking, or frame 0 otherwise
// (or if there's less than 2 frames in the current loop).
static void ResetFrameIfAtEnd(CharacterInfo *ch)
{
    const int frames_in_loop = views[ch->view].loops[ch->loop].numFrames;
    if (ch->frame >= frames_in_loop)
    {
        ch->frame = (ch->is_moving() && frames_in_loop > 1) ? 1 : 0;
    }
}

// Fixups loop and frame values, in case any of them are set to a value out of the valid range
static void FixupCharacterLoopAndFrame(CharacterInfo *ch, bool switch_from_bad_loop = true)
{
    // If current loop property exceeds number of loops,
    // or if selected loop has no frames, then try select any first loop that has frames.
    // NOTE: although this may seem like a weird solution to a problem,
    // we do so for backwards compatibility; this approximately emulates older games behavior.
    const int view = ch->view;
    if (view < 0 || view >= views.size())
    {
        quitprintf("!Character %s is assigned a invalid view %d (valid range 1..%d)", ch->scrname.GetCStr(), view,
                   game.numviews);
    }
    if (!switch_from_bad_loop && (ch->loop < 0 || ch->loop >= views[view].numLoops))
    {
        quitprintf("!Character %s is assigned a invalid loop %d in view %d (valid range 0..%d)", ch->scrname.GetCStr(),
                   ch->loop, view, views[view].GetLastLoop());
    }

    if (switch_from_bad_loop &&
        ((ch->loop < 0 || ch->loop >= views[view].numLoops) || (views[view].loops[ch->loop].numFrames == 0)))
    {
        int loop = ch->loop;
        for (loop = 0; (loop < views[view].numLoops) && (views[view].loops[loop].numFrames == 0); ++loop)
        {
            ;
        }
        if (loop == views[view].numLoops) // view has no frames?!
        {
            quitprintf("!Character %s is assigned view %d that has no frames!", ch->scrname.GetCStr(), view);
            loop = 0;
        }
        ch->loop = loop;
    }

    ResetFrameIfAtEnd(ch); // test, in case new loop has less frames
}

void UpdateCharacterMoveAndAnim(CharacterInfo *ch, CharacterExtras *chex, std::vector<int> &following_as_sheep)
{
    if (!ch->is_enabled())
    {
        return;
    }

    // Update turning
    bool is_turning = UpdateCharacterTurning(ch, chex);
    // Fixup character's loop and frame prior to any further logic updates;
    // but don't fixup "bad" loops if animated by command, as that leads to confusing behavior.
    FixupCharacterLoopAndFrame(ch, !chex->IsAnimating());
    if (is_turning)
    {
        return; // still turning, break
    }

    int doing_nothing = 1;
    UpdateCharacterMoving(ch, chex, doing_nothing);

    // Update animating
    if (UpdateCharacterAnimating(ch, chex, doing_nothing))
    {
        return; // further update blocked by active animation
    }

    UpdateCharacterFollower(ch, following_as_sheep, doing_nothing);

    UpdateCharacterIdle(ch, chex, doing_nothing);

    chex->process_idle_this_time = 0;
}

void UpdateFollowingExactlyCharacter(CharacterInfo *ch)
{
    const auto &following = game.chars[charextra[ch->index_id].following];
    ch->x = following.x;
    ch->y = following.y;
    ch->z = following.z;
    ch->room = following.room;
    ch->prevroom = following.prevroom;

    const int usebase = following.get_baseline();
    if (ch->get_follow_sort_behind())
    {
        ch->baseline = usebase - 1;
    }
    else
    {
        ch->baseline = usebase + 1;
    }
}

bool UpdateCharacterTurning(CharacterInfo *ch, CharacterExtras *chex)
{
    if (chex->IsTurning())
    {
        const int view = ch->view;
        // Currently rotating to correct direction
        if (ch->walkwait > 0)
        {
            ch->walkwait--;
        }
        else
        {
            // Work out which direction is next
            int wantloop = find_looporder_index(ch->loop) + 1;
            // going anti-clockwise, take one before instead
            if (chex->IsTurningCounterClockwise())
            {
                wantloop -= 2;
            }
            while (1)
            {
                if (wantloop >= 8)
                {
                    wantloop = 0;
                }
                else if (wantloop < 0)
                {
                    wantloop = 7;
                }

                if ((turnlooporder[wantloop] >= views[view].numLoops) ||
                    (views[view].loops[turnlooporder[wantloop]].numFrames < 1) ||
                    ((turnlooporder[wantloop] >= NUM_BASE_DIRECTIONS) && ((ch->flags & CHF_NODIAGONAL) != 0)))
                {
                    if (chex->IsTurningCounterClockwise())
                    {
                        wantloop--;
                    }
                    else
                    {
                        wantloop++;
                    }
                }
                else
                {
                    break;
                }
            }
            ch->loop = turnlooporder[wantloop];
            chex->DecrementTurning();
            // if still turning, wait for next frame
            if (chex->turns > 0)
            {
                ch->walkwait = ch->animspeed;
            }
            else if (ch->is_moving())
            {
                ch->walkwait = 1; // wait 1 frame to let walking animation advance once
            }
            chex->animwait = 0;
        }

        return true;
    }

    return false;
}

extern void reset_character_idling_time(CharacterInfo *ch);

void UpdateCharacterMoving(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing)
{
    if (ch->is_moving() && (ch->room == displayed_room))
    {
        const bool was_move_direct = get_movelist(ch->get_movelist_id())->IsStageDirect();

        if (ch->walkwait > 0)
        {
            ch->walkwait--;
        }
        else
        {
            ch->flags &= ~CHF_AWAITINGMOVE;

            // Move the character
            int num_steps = wantMoveNow(ch, chex);

            if ((num_steps) && (chex->xwas != INVALID_X))
            {
                // A half-step position saved by an earlier update (see below) may still be pending, for instance
                // when the zoom level changed in between: restore the full-step position before stepping
                ch->x = chex->xwas;
                ch->y = chex->ywas;
                chex->xwas = INVALID_X;
            }

            const int oldxp = ch->x, oldyp = ch->y;

            for (int ff = 0; ff < abs(num_steps); ff++)
            {
                if (doNextCharMoveStep(ch, chex))
                {
                    break;
                }
                if ((ch->walking == 0) || (chex->turns > 0))
                {
                    break;
                }
            }

            if (num_steps < 0)
            {
                // Very small scaling: show the character half way through the step, to stop the movement being
                // jumpy; the full-step position is kept in xwas/ywas and restored on the next update
                chex->xwas = ch->x;
                chex->ywas = ch->y;
                ch->x = ((ch->x) - oldxp) / 2 + oldxp;
                ch->y = ((ch->y) - oldyp) / 2 + oldyp;
            }
            else if (num_steps > 0)
            {
                chex->xwas = INVALID_X;
            }

            if ((ch->flags & CHF_ANTIGLIDE) == 0)
            {
                ch->walkwaitcounter++;
            }
        }

        // Fixup character's loop, it may be changed when making a walk-move
        FixupCharacterLoopAndFrame(ch);

        doing_nothing = 0; // still walking?

        const int view = ch->view, loop = ch->loop;
        if (ch->walking < 1)
        {
            // Finished walking, stop and reset state
            reset_character_idling_time(ch);
            doing_nothing = 1;
            ch->walkwait = 0;
            const bool was_walk_anim = ch->is_moving_walkanim();
            Character_StopMovingEx(ch, !was_move_direct);
            // The move list is gone and "walking" is already 0 here (do_movelist_move), so Character_StopMovingEx
            // skips its "walking > 0" part, which is the one that resets the frame: this is the only place where
            // the standing frame is set after a finished walk.
            // TODO: Character_StopMoving called by a script on a walking character resets the frame, but not
            // animwait, and does not call CheckViewFrame(), unlike here: the same reset could be shared.
            if (was_walk_anim)
            {
                // use standing pic
                chex->animwait = 0;
                ch->frame = 0;
                chex->CheckViewFrame(ch);
            }
        }
        else if (chex->animwait > 0)
        {
            chex->animwait--;
        }
        else
        {
            if (ch->flags & CHF_ANTIGLIDE)
            {
                ch->walkwaitcounter++;
            }

            if (ch->is_moving_walkanim())
            {
                ch->frame++;
                ResetFrameIfAtEnd(ch); // roll back if exceeded loop

                chex->animwait = views[view].loops[loop].frames[ch->frame].speed + ch->animspeed;

                if (ch->flags & CHF_ANTIGLIDE)
                {
                    ch->walkwait = chex->animwait;
                }
                else
                {
                    ch->walkwait = 0;
                }

                chex->CheckViewFrame(ch);
            }
        }
    }
}

bool UpdateCharacterAnimating(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing)
{
    // animating or idling, and not walking with a walk animation
    if (((chex->IsAnimating()) || (ch->is_idling())) && (!ch->is_moving() || ch->is_moving_no_anim()) &&
        (ch->room == displayed_room))
    {
        doing_nothing = 0;
        // idle anim doesn't count as doing something
        if (ch->is_idling())
        {
            doing_nothing = 1;
        }

        const int view = ch->view;
        const bool is_char_speaking = (char_speaking_anim == ch->index_id);

        if (ch->wait > 0)
        {
            ch->wait--;
        }
        else if (is_char_speaking && (game.options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts) && has_voice_lipsync())
        {
            const int new_frame = update_voice_lipsync(ch->frame);
            if (ch->frame != new_frame)
            {
                ch->frame = new_frame;
                chex->CheckViewFrame(ch);
            }
        }
        else if (is_char_speaking && (game.options[OPT_LIPSYNCTEXT] != 0))
        {
            // currently talking with lip-sync speech
            int fraa = ch->frame;
            ch->wait = update_lip_sync(view, ch->loop, &fraa) - 1;
            // closed mouth at end of sentence
            // NOTE: standard lip-sync is synchronized with text timer, not voice file
            if (play.speech_in_post_state ||
                ((play.messagetime >= 0) && (play.messagetime < play.close_mouth_speech_time)))
            {
                ch->frame = 0;
            }

            if (ch->frame != fraa)
            {
                ch->frame = fraa;
                chex->CheckViewFrame(ch);
            }

            return true;
        }
        else
        {
            // Normal view animation
            const int oldframe = ch->frame;

            bool done_anim = false;
            if ((ch->index_id == char_speaking_anim) &&
                (play.speech_in_post_state || ((!play.speech_voice_blocking) && (play.close_mouth_speech_time > 0) &&
                                               (play.messagetime < play.close_mouth_speech_time))))
            {
                // finished talking - stop animation
                done_anim = true;
                ch->frame = 0;
            }
            else
            {
                if (!CycleViewAnim(view, ch->loop, ch->frame, chex->GetAnimParams()))
                {
                    done_anim = true; // finished animating
                    // end of idle anim
                    if (ch->is_idling())
                    {
                        // constant anim, reset (need this cos animating==0)
                        if (ch->idledelay == 0)
                        {
                            ch->frame = 0;
                        }
                        // one-off anim, stop
                        else
                        {
                            Character_UnlockView(ch);
                        }
                    }
                }
            }

            ch->wait = views[view].loops[ch->loop].frames[ch->frame].speed;
            // An idle animation has no animation parameters in chex (they are cleared when it starts, see
            // UpdateCharacterIdle), so its delay comes from idle_anim_speed instead of GetAnimDelay()
            if (ch->is_idling())
            {
                ch->wait += ch->idle_anim_speed;
            }
            else
            {
                ch->wait += chex->GetAnimDelay();
            }

            if (ch->frame != oldframe)
            {
                chex->CheckViewFrame(ch);
            }

            if (done_anim)
            {
                stop_character_anim(ch);
            }
        }
    }

    return false;
}

void UpdateCharacterFollower(CharacterInfo *ch, std::vector<int> &following_as_sheep, int &doing_nothing)
{
    const CharacterExtras *chex = &charextra[ch->index_id];
    const int following = chex->following;
    const int distaway = chex->follow_dist;
    const int eagerness = chex->follow_eagerness;

    if ((following >= 0) && (distaway == FOLLOW_ALWAYSONTOP))
    {
        // an always-on-top follow
        following_as_sheep.push_back(ch->index_id);
    }
    // not moving, but should be following another character
    else if ((following >= 0) && (doing_nothing == 1))
    {
        // either of the two characters is disabled: do nothing
        if (!game.chars[following].is_enabled() || !ch->is_enabled())
        {
            ;
        }
        else if (ch->room < 0)
        {
            // A negative room is the countdown set below (-follow_change_room_timer), the delay before the follower
            // appears in the room the followed character is in. It counts up by one per update while the
            // character has nothing else to do, and the follower enters at 0.
            ch->room++;
            if (ch->room == 0)
            {
                // appear in the new room
                ch->room = game.chars[following].room;
                ch->x = play.entered_at_x;
                ch->y = play.entered_at_y;
            }
        }
        // wait a bit, so we're not constantly walking
        else if (Random(100) < eagerness)
        {
            ;
        }
        // the followed character has changed room
        else if (ch->room != game.chars[following].room)
        {
            ch->prevroom = ch->room;
            ch->room = game.chars[following].room;

            if (ch->room == displayed_room)
            {
                // only move to the room-entered position if coming into
                // the current room
                if (play.entered_at_x > (thisroom.Width - 8))
                {
                    ch->x = thisroom.Width + 8;
                    ch->y = play.entered_at_y;
                }
                else if (play.entered_at_x < 8)
                {
                    ch->x = -8;
                    ch->y = play.entered_at_y;
                }
                else if (play.entered_at_y > (thisroom.Height - 8))
                {
                    ch->y = thisroom.Height + 8;
                    ch->x = play.entered_at_x;
                }
                else if (play.entered_at_y < thisroom.Edges.Top + 8)
                {
                    ch->y = thisroom.Edges.Top + 1;
                    ch->x = play.entered_at_x;
                }
                else
                {
                    // not at one of the edges
                    // delay for a few seconds to let the player move
                    ch->room = -play.follow_change_room_timer;
                }
                if (ch->room >= 0)
                {
                    walk_character(ch, play.entered_at_x, play.entered_at_y, true /* ignwal */);
                    doing_nothing = 0;
                }
            }
        }
        else if (ch->room != displayed_room)
        {
            // if the character is following another character and
            // neither is in the current room, don't try to move
        }
        else if ((abs(game.chars[following].x - ch->x) > distaway + 30) ||
                 (abs(game.chars[following].y - ch->y) > distaway + 30) || (eagerness == 0))
        {
            // in same room
            int goxoffs = (Random(50) - 25);
            // make sure he's not standing on top of the other man
            if (goxoffs < 0)
            {
                goxoffs -= distaway;
            }
            else
            {
                goxoffs += distaway;
            }
            walk_character(ch, game.chars[following].x + goxoffs, game.chars[following].y + (Random(50) - 25),
                           false /* walk areas */);

            doing_nothing = 0;
        }
    }
}

void UpdateCharacterIdle(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing)
{
    // no idle animation, so skip this bit
    if (ch->idleview < 1)
    {
        ;
    }
    // currently playing idle anim
    else if (ch->is_idling())
    {
        ;
    }
    // not in the current room
    else if (ch->room != displayed_room)
    {
        ;
    }
    // they are moving or animating (or the view is locked), so
    // reset idle timeout
    else if ((doing_nothing == 0) || ((ch->flags & CHF_FIXVIEW) != 0))
    {
        ch->idleleft = ch->idledelay;
    }
    // count idle time
    else if ((get_loop_counter() % GetGameSpeed() == 0) || (chex->process_idle_this_time == 1))
    {
        ch->idleleft--;
        if (ch->idleleft == -1)
        {
            int useloop = ch->loop;
            debug_script_log("%s: Now idle (view %d)", ch->scrname.GetCStr(), ch->idleview + 1);
            Character_LockView(ch, ch->idleview + 1);
            // Character_LockView stops the idling (idleleft is -1 here) and resets idleleft: mark it as idling again
            ch->idleleft = -2;
            int max_loops = views[ch->idleview].numLoops;
            // if the char is set to "no diagonal loops", don't try
            // to use diagonal idle loops either
            if ((max_loops > 4) && (!should_use_diagloops(ch, false /* allow 1-frame loops */)))
            {
                max_loops = 4;
            }
            // If it's not a "swimming"-type idleanim, choose a random loop
            // if there arent enough loops to do the current one.
            if ((ch->idledelay > 0) && (useloop >= max_loops))
            {
                do
                {
                    useloop = rand() % max_loops;
                    // don't select a loop which is a continuation of a previous one
                } while ((useloop > 0) && (views[ch->idleview].loops[useloop - 1].RunNextLoop()));
            }
            // Normal idle anim - just reset to loop 0 if not enough to
            // use the current one
            else if (useloop >= max_loops)
            {
                useloop = 0;
            }

            animate_character(ch, useloop, ch->idle_anim_speed, (ch->idledelay == 0) ? 1 : 0 /* repeat */);

            // Don't keep the animation parameters while the idle anim plays: chex->IsAnimating() stays false, and
            // the idling state is told by idleleft < 0 alone (is_idling()).
            // TODO: the idle animation's delay could live in chex->anim like the other animations' (see
            // UpdateCharacterAnimating), which would drop the idle_anim_speed special case.
            chex->anim = ViewAnimateParams();
        }
    } // end do idle animation
}
