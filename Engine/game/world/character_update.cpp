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
#include "debug/debug_log.h"
#include "game/character_def.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/math.h"
#include "game/world/character.h"
#include "game/world/object.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/view_frame.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "util/common.h"

using namespace AGS::Common;

int Character::get_baseline() const
{
    if (Baseline < 1)
    {
        return Y;
    }
    return Baseline;
}

int Character::get_blocking_top() const
{
    if (BlockingHeight > 0)
    {
        return Y - BlockingHeight / 2;
    }
    return Y - 2;
}

int Character::get_blocking_bottom() const
{
    // the blocking_bottom should be 1 less than the top + height
    // since the code does <= checks on it rather than < checks
    if (BlockingHeight > 0)
    {
        return (Y + (BlockingHeight + 1) / 2) - 1;
    }
    return Y + 3;
}

// Tests if frame reached or exceeded the loop, and resets frame back
// to either 1st walking frame if character is walking, or frame 0 otherwise
// (or if there's less than 2 frames in the current loop).
static void ResetFrameIfAtEnd(Game &game, Character *ch)
{
    const int frames_in_loop = game.Views[ch->View].Loops[ch->Loop].FrameCount;
    if (ch->Frame >= frames_in_loop)
    {
        ch->Frame = (ch->IsMoving() && frames_in_loop > 1) ? 1 : 0;
    }
}

// Fixups loop and frame values, in case any of them are set to a value out of the valid range
static void FixupCharacterLoopAndFrame(Game &game, Character *ch, bool switch_from_bad_loop = true)
{
    // If current loop property exceeds number of loops,
    // or if selected loop has no frames, then try select any first loop that has frames.
    // NOTE: although this may seem like a weird solution to a problem,
    // we do so for backwards compatibility; this approximately emulates older games behavior.
    const int view = ch->View;
    if (view < 0 || view >= game.Views.size())
    {
        quitprintf("!Character %s is assigned a invalid view %d (valid range 1..%d)", ch->ScriptName.GetCStr(), view,
                   game.Def().ViewCount);
    }
    if (!switch_from_bad_loop && (ch->Loop < 0 || ch->Loop >= game.Views[view].LoopCount))
    {
        quitprintf("!Character %s is assigned a invalid loop %d in view %d (valid range 0..%d)",
                   ch->ScriptName.GetCStr(), ch->Loop, view, game.Views[view].GetLastLoop());
    }

    if (switch_from_bad_loop && ((ch->Loop < 0 || ch->Loop >= game.Views[view].LoopCount) ||
                                 (game.Views[view].Loops[ch->Loop].FrameCount == 0)))
    {
        int loop = ch->Loop;
        for (loop = 0; (loop < game.Views[view].LoopCount) && (game.Views[view].Loops[loop].FrameCount == 0); ++loop)
        {
            ;
        }
        if (loop == game.Views[view].LoopCount) // view has no frames?!
        {
            quitprintf("!Character %s is assigned view %d that has no frames!", ch->ScriptName.GetCStr(), view);
            loop = 0;
        }
        ch->Loop = loop;
    }

    ResetFrameIfAtEnd(game, ch); // test, in case new loop has less frames
}

void UpdateCharacterMoveAndAnim(Game &game, Room &room, Character *ch, std::vector<int> &following_as_sheep)
{
    if (!ch->is_enabled())
    {
        return;
    }

    // Update turning
    bool is_turning = UpdateCharacterTurning(game, ch);
    // Fixup character's loop and frame prior to any further logic updates;
    // but don't fixup "bad" loops if animated by command, as that leads to confusing behavior.
    FixupCharacterLoopAndFrame(game, ch, !ch->IsAnimating());
    if (is_turning)
    {
        return; // still turning, break
    }

    int doing_nothing = 1;
    UpdateCharacterMoving(game, ch, doing_nothing);

    // Update animating
    if (UpdateCharacterAnimating(game, ch, doing_nothing))
    {
        return; // further update blocked by active animation
    }

    UpdateCharacterFollower(game, room, ch, following_as_sheep, doing_nothing);

    UpdateCharacterIdle(game, ch, doing_nothing);

    ch->ProcessIdleThisTime = false;
}

void UpdateFollowingExactlyCharacter(Game &game, Character *ch)
{
    const auto &following = game.Characters[ch->Following];
    ch->X = following.X;
    ch->Y = following.Y;
    ch->Z = following.Z;
    ch->Room = following.Room;
    ch->PreviousRoom = following.PreviousRoom;

    const int use_baseline = following.get_baseline();
    if (ch->get_follow_sort_behind())
    {
        ch->Baseline = use_baseline - 1;
    }
    else
    {
        ch->Baseline = use_baseline + 1;
    }
}

bool UpdateCharacterTurning(Game &game, Character *ch)
{
    if (ch->IsTurning())
    {
        const int view = ch->View;
        // Currently rotating to correct direction
        if (ch->WalkWait > 0)
        {
            ch->WalkWait--;
        }
        else
        {
            // Work out which direction is next
            int want_loop = find_looporder_index(ch->Loop) + 1;
            // going anti-clockwise, take one before instead
            if (ch->IsTurningCounterClockwise())
            {
                want_loop -= 2;
            }
            while (1)
            {
                if (want_loop >= 8)
                {
                    want_loop = 0;
                }
                else if (want_loop < 0)
                {
                    want_loop = 7;
                }

                if ((turnlooporder[want_loop] >= game.Views[view].LoopCount) ||
                    (game.Views[view].Loops[turnlooporder[want_loop]].FrameCount < 1) ||
                    ((turnlooporder[want_loop] >= NUM_BASE_DIRECTIONS) && ((ch->Flags & CHF_NODIAGONAL) != 0)))
                {
                    if (ch->IsTurningCounterClockwise())
                    {
                        want_loop--;
                    }
                    else
                    {
                        want_loop++;
                    }
                }
                else
                {
                    break;
                }
            }
            ch->Loop = turnlooporder[want_loop];
            ch->DecrementTurning();
            // if still turning, wait for next frame
            if (ch->Turns > 0)
            {
                ch->WalkWait = ch->AnimationDelay;
            }
            else if (ch->IsMoving())
            {
                ch->WalkWait = 1; // wait 1 frame to let walking animation advance once
            }
            ch->WalkAnimWait = 0;
        }

        return true;
    }

    return false;
}

void UpdateCharacterMoving(Game &game, Character *ch, int &doing_nothing)
{
    if (ch->IsMoving() && (ch->Room == game.DisplayedRoom))
    {
        const bool was_move_direct = get_movelist(ch->get_movelist_id())->IsStageDirect();

        if (ch->WalkWait > 0)
        {
            ch->WalkWait--;
        }
        else
        {
            ch->Flags &= ~CHF_AWAITINGMOVE;

            // Move the character
            int num_steps = wantMoveNow(ch);

            if ((num_steps) && (ch->FullStepX != INVALID_X))
            {
                // A half-step position saved by an earlier update (see below) may still be pending, for instance
                // when the zoom level changed in between: restore the full-step position before stepping
                ch->X = ch->FullStepX;
                ch->Y = ch->FullStepY;
                ch->FullStepX = INVALID_X;
            }

            const int oldxp = ch->X, oldyp = ch->Y;

            for (int ff = 0; ff < abs(num_steps); ff++)
            {
                if (doNextCharMoveStep(game, ch))
                {
                    break;
                }
                if ((!ch->IsMoving()) || (ch->Turns > 0))
                {
                    break;
                }
            }

            if (num_steps < 0)
            {
                // Very small scaling: show the character half way through the step, to stop the movement being
                // jumpy; the full-step position is kept in FullStepX/FullStepY and restored on the next update
                ch->FullStepX = ch->X;
                ch->FullStepY = ch->Y;
                ch->X = ((ch->X) - oldxp) / 2 + oldxp;
                ch->Y = ((ch->Y) - oldyp) / 2 + oldyp;
            }
            else if (num_steps > 0)
            {
                ch->FullStepX = INVALID_X;
            }

            if ((ch->Flags & CHF_ANTIGLIDE) == 0)
            {
                ch->WalkWaitCounter++;
            }
        }

        // Fixup character's loop, it may be changed when making a walk-move
        FixupCharacterLoopAndFrame(game, ch);

        doing_nothing = 0; // still walking?

        const int view = ch->View, loop = ch->Loop;
        if (!ch->IsMoving())
        {
            // Finished walking, stop and reset state
            reset_character_idling_time(ch);
            doing_nothing = 1;
            ch->WalkWait = 0;
            const bool was_walk_anim = ch->is_moving_walkanim();
            Character_StopMovingEx(ch, !was_move_direct);
            // The move list is gone and "MoveListID" is already 0 here (do_movelist_move), so Character_StopMovingEx
            // skips its "IsMoving()" part, which is the one that resets the frame: this is the only place where
            // the standing frame is set after a finished walk.
            // TODO: Character_StopMoving called by a script on a walking character resets the frame, but not
            // WalkAnimWait, and does not call CheckViewFrame(), unlike here: the same reset could be shared.
            if (was_walk_anim)
            {
                // use standing pic
                ch->WalkAnimWait = 0;
                ch->Frame = 0;
                ch->CheckViewFrame();
            }
        }
        else if (ch->WalkAnimWait > 0)
        {
            ch->WalkAnimWait--;
        }
        else
        {
            if (ch->Flags & CHF_ANTIGLIDE)
            {
                ch->WalkWaitCounter++;
            }

            if (ch->is_moving_walkanim())
            {
                ch->Frame++;
                ResetFrameIfAtEnd(game, ch); // roll back if exceeded loop

                ch->WalkAnimWait = game.Views[view].Loops[loop].Frames[ch->Frame].Delay + ch->AnimationDelay;

                if (ch->Flags & CHF_ANTIGLIDE)
                {
                    ch->WalkWait = ch->WalkAnimWait;
                }
                else
                {
                    ch->WalkWait = 0;
                }

                ch->CheckViewFrame();
            }
        }
    }
}

bool UpdateCharacterAnimating(Game &game, Character *ch, int &doing_nothing)
{
    // animating or idling, and not walking with a walk animation
    if (((ch->IsAnimating()) || (ch->is_idling())) && (!ch->IsMoving() || ch->is_moving_no_anim()) &&
        (ch->Room == game.DisplayedRoom))
    {
        doing_nothing = 0;
        // idle anim doesn't count as doing something
        if (ch->is_idling())
        {
            doing_nothing = 1;
        }

        const int view = ch->View;
        const bool is_char_speaking = (game.Speech.CharSpeakingAnim == ch->ID);

        if (ch->AnimWait > 0)
        {
            ch->AnimWait--;
        }
        else if (is_char_speaking && (game.Options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts) && has_voice_lipsync())
        {
            const int new_frame = update_voice_lipsync(ch->Frame);
            if (ch->Frame != new_frame)
            {
                ch->Frame = new_frame;
                ch->CheckViewFrame();
            }
        }
        else if (is_char_speaking && (game.Options[OPT_LIPSYNCTEXT] != 0))
        {
            // currently talking with lip-sync speech
            int fraa = ch->Frame;
            ch->AnimWait = update_lip_sync(view, ch->Loop, &fraa) - 1;
            // closed mouth at end of sentence
            // NOTE: standard lip-sync is synchronized with text timer, not voice file
            if (game.Play.IsSpeechInPostState ||
                ((game.Play.MessageTimeLeft >= 0) &&
                 (game.Play.MessageTimeLeft < game.Play.SpeechAnimationStopTimeMargin)))
            {
                ch->Frame = 0;
            }

            if (ch->Frame != fraa)
            {
                ch->Frame = fraa;
                ch->CheckViewFrame();
            }

            return true;
        }
        else
        {
            // Normal view animation
            const int old_frame = ch->Frame;

            bool done_anim = false;
            if ((ch->ID == game.Speech.CharSpeakingAnim) &&
                (game.Play.IsSpeechInPostState ||
                 ((!game.Play.IsSpeechVoiceBlocking) && (game.Play.SpeechAnimationStopTimeMargin > 0) &&
                  (game.Play.MessageTimeLeft < game.Play.SpeechAnimationStopTimeMargin))))
            {
                // finished talking - stop animation
                done_anim = true;
                ch->Frame = 0;
            }
            else
            {
                if (!CycleViewAnim(game, view, ch->Loop, ch->Frame, ch->GetAnimParams()))
                {
                    done_anim = true; // finished animating
                    // end of idle anim
                    if (ch->is_idling())
                    {
                        // constant anim, reset (need this cos animating==0)
                        if (ch->IdleDelay == 0)
                        {
                            ch->Frame = 0;
                        }
                        // one-off anim, stop
                        else
                        {
                            Character_UnlockView(ch);
                        }
                    }
                }
            }

            ch->AnimWait = game.Views[view].Loops[ch->Loop].Frames[ch->Frame].Delay;
            // An idle animation has no animation parameters in ch->Anim (they are cleared when it starts, see
            // UpdateCharacterIdle), so its delay comes from IdleAnimationDelay instead of GetAnimDelay()
            if (ch->is_idling())
            {
                ch->AnimWait += ch->IdleAnimationDelay;
            }
            else
            {
                ch->AnimWait += ch->GetAnimDelay();
            }

            if (ch->Frame != old_frame)
            {
                ch->CheckViewFrame();
            }

            if (done_anim)
            {
                stop_character_anim(ch);
            }
        }
    }

    return false;
}

void UpdateCharacterFollower(Game &game, Room &room, Character *ch, std::vector<int> &following_as_sheep,
                             int &doing_nothing)
{
    const int following = ch->Following;
    const int distaway = ch->FollowDistance;
    const int eagerness = ch->FollowEagerness;

    if ((following >= 0) && (distaway == FOLLOW_ALWAYSONTOP))
    {
        // an always-on-top follow
        following_as_sheep.push_back(ch->ID);
    }
    // not moving, but should be following another character
    else if ((following >= 0) && (doing_nothing == 1))
    {
        // either of the two characters is disabled: do nothing
        if (!game.Characters[following].is_enabled() || !ch->is_enabled())
        {
            return;
        }
        if (ch->Room < 0)
        {
            // A negative room is the countdown set below (-FollowingRoomDelay), the delay before the follower
            // appears in the room the followed character is in. It counts up by one per update while the
            // character has nothing else to do, and the follower enters at 0.
            ch->Room++;
            if (ch->Room == 0)
            {
                // appear in the new room
                ch->Room = game.Characters[following].Room;
                ch->X = room.EnteredAtX;
                ch->Y = room.EnteredAtY;
            }
        }
        // wait a bit, so we're not constantly walking
        else if (Random(100) < eagerness)
        {
            return;
        }
        // the followed character has changed room
        else if (ch->Room != game.Characters[following].Room)
        {
            ch->PreviousRoom = ch->Room;
            ch->Room = game.Characters[following].Room;

            if (ch->Room == game.DisplayedRoom)
            {
                // only move to the room-entered position if coming into
                // the current room
                if (room.EnteredAtX > (room.Width - 8))
                {
                    ch->X = room.Width + 8;
                    ch->Y = room.EnteredAtY;
                }
                else if (room.EnteredAtX < 8)
                {
                    ch->X = -8;
                    ch->Y = room.EnteredAtY;
                }
                else if (room.EnteredAtY > (room.Height - 8))
                {
                    ch->Y = room.Height + 8;
                    ch->X = room.EnteredAtX;
                }
                else if (room.EnteredAtY < room.Edges.Top + 8)
                {
                    ch->Y = room.Edges.Top + 1;
                    ch->X = room.EnteredAtX;
                }
                else
                {
                    // not at one of the edges
                    // delay for a few seconds to let the player move
                    ch->Room = -game.Play.FollowingRoomDelay;
                }
                if (ch->Room >= 0)
                {
                    walk_character(ch, room.EnteredAtX, room.EnteredAtY, true /* ignwal */);
                    doing_nothing = 0;
                }
            }
        }
        else if (ch->Room != game.DisplayedRoom)
        {
            // if the character is following another character and
            // neither is in the current room, don't try to move
        }
        else if ((abs(game.Characters[following].X - ch->X) > distaway + 30) ||
                 (abs(game.Characters[following].Y - ch->Y) > distaway + 30) || (eagerness == 0))
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
            walk_character(ch, game.Characters[following].X + goxoffs, game.Characters[following].Y + (Random(50) - 25),
                           false /* walk areas */);

            doing_nothing = 0;
        }
    }
}

void UpdateCharacterIdle(Game &game, Character *ch, int &doing_nothing)
{
    // no idle animation, already playing the idle animation, or not in the current room
    if ((ch->IdleView < 1) || ch->is_idling() || (ch->Room != game.DisplayedRoom))
    {
        return;
    }
    // they are moving or animating (or the view is locked), so
    // reset idle timeout
    if ((doing_nothing == 0) || ((ch->Flags & CHF_FIXVIEW) != 0))
    {
        ch->IdleTimeLeft = ch->IdleDelay;
    }
    // count idle time
    else if ((get_loop_counter() % GetGameSpeed() == 0) || ch->ProcessIdleThisTime)
    {
        ch->IdleTimeLeft--;
        if (ch->IdleTimeLeft == -1)
        {
            int use_loop = ch->Loop;
            debug_script_log("%s: Now idle (view %d)", ch->ScriptName.GetCStr(), ch->IdleView + 1);
            Character_LockView(ch, ch->IdleView + 1);
            // Character_LockView stops the idling (IdleTimeLeft is -1 here) and resets IdleTimeLeft: mark it as idling again
            ch->IdleTimeLeft = -2;
            int max_loops = game.Views[ch->IdleView].LoopCount;
            // if the char is set to "no diagonal loops", don't try
            // to use diagonal idle loops either
            if ((max_loops > 4) && (!should_use_diagloops(ch, false /* allow 1-frame loops */)))
            {
                max_loops = 4;
            }
            // If it's not a "swimming"-type idleanim, choose a random loop
            // if there arent enough loops to do the current one.
            if ((ch->IdleDelay > 0) && (use_loop >= max_loops))
            {
                do
                {
                    use_loop = rand() % max_loops;
                    // don't select a loop which is a continuation of a previous one
                } while ((use_loop > 0) && (game.Views[ch->IdleView].Loops[use_loop - 1].RunNextLoop()));
            }
            // Normal idle anim - just reset to loop 0 if not enough to
            // use the current one
            else if (use_loop >= max_loops)
            {
                use_loop = 0;
            }

            animate_character(ch, use_loop, ch->IdleAnimationDelay, (ch->IdleDelay == 0) ? 1 : 0 /* repeat */);

            // Don't keep the animation parameters while the idle anim plays: ch->IsAnimating() stays false, and
            // the idling state is told by IdleTimeLeft < 0 alone (is_idling()).
            // TODO: the idle animation's delay could live in ch->Anim like the other animations' (see
            // UpdateCharacterAnimating), which would drop the IdleAnimationDelay special case.
            ch->Anim = ViewAnimateParams();
        }
    } // end do idle animation
}
