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
// AGS Character functions
//
//=============================================================================
#include "game/world/character.h"

#include <cstdio>
#include <math.h>

#include "debug/debug_log.h"
#include "game/dialog/dialog.h"
#include "game/dialog/display.h"
#include "game/dialog/lipsync.h"
#include "game/dialog/speech.h"
#include "game/dynobj/cc_character.h"
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_shader.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/navigation/route_finder.h"
#include "game/render/draw.h"
#include "game/render/overlay.h"
#include "game/render/screen_overlay.h"
#include "game/runtime_defines.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/view.h"
#include "game/world/inventory_item.h"
#include "game/world/move_list.h"
#include "game/world/object.h"
#include "game/world/properties.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/room_context.h"
#include "game/world/room_status.h"
#include "game/world/view_frame.h"
#include "game/world/walkable_area.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "main/update.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"
#include "script/runtimescriptvalue.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

//--------------------------------


// Index of the currently thinking character (the rest of the speech state is Game::Speech)
int char_thinking = -1;

// lip-sync speech settings (the rest are Game::LipSync)
int text_lips_offset;
const char *text_lips_text = nullptr;

// **** CHARACTER: FUNCTIONS ****

// Checks if character is currently playing idle anim, and reset it
void stop_character_idling(Character *ch)
{
    if (ch->is_idling())
    {
        Character_UnlockView(ch);
        ch->idleleft = ch->idledelay;
    }
}

// Resets idling timer, and marks for immediate update (in case its persistent idling)
void reset_character_idling_time(Character *ch)
{
    ch->idleleft = ch->idledelay;
    ch->process_idle_this_time = 1;
}

bool IsValidCharacter(int char_id)
{
    return ((char_id >= 0) && (char_id < Game::Current().numcharacters));
}

bool AssertCharacter(const char *apiname, int char_id)
{
    Game &game = Game::Current();
    if ((char_id >= 0) && (char_id < game.numcharacters))
        return true;
    debug_script_error("%s: invalid character id %d (range is 0..%d)", apiname, char_id,
                       game.numcharacters > 0 ? game.numcharacters - 1 : 0);
    return false;
}

void Character_AddInventory(Character *ch, ScriptInvItem *item, int at_index)
{
    Game &game = Game::Current();
    if (item == nullptr)
    {
        quit("!Character.AddInventory: invalid inventory number");
    }

    if (at_index >= SCR_NO_VALUE)
        at_index = -1;

    const int inum = item->id;
    const int quant = ch->get_item_quantity(inum);
    if (quant == INT32_MAX)
    {
        debug_script_warn("Character.AddInventory: char %s, item %s: cannot carry more than %d of one inventory item",
                          ch->scrname.GetCStr(), game.invScriptNames[inum].GetCStr(), INT32_MAX);
        return;
    }

    ch->set_item_quantity(inum, quant + 1);

    if (game.options[OPT_DUPLICATEINV] == 0)
    {
        // Ensure it is only in the list once
        for (const auto &item_id : ch->inventory)
        {
            if (item_id == inum)
            {
                // They already have the item, so don't add it to the list
                if (ch == game.PlayerChar)
                {
                    run_on_event(kScriptEvent_InventoryAdd, inum);
                }
                return;
            }
        }
    }

    if (ch->inventory.size() == MAX_CHAR_INVENTORY)
    {
        debug_script_warn(
            "Character.AddInventory: char %s, item %s: too many inventory items added, max %d can be stored at once",
            ch->scrname.GetCStr(), game.invScriptNames[inum].GetCStr(), MAX_CHAR_INVENTORY);
        return;
    }

    if ((at_index < 0) || (static_cast<uint32_t>(at_index) >= ch->inventory.size()))
    {
        ch->inventory.push_back(inum);
    }
    else
    {
        ch->inventory.insert(ch->inventory.begin() + at_index, inum);
    }
    GUIE::MarkInventoryForUpdate(ch->index_id, ch->index_id == game.playercharacter);
    if (ch == game.PlayerChar)
    {
        run_on_event(kScriptEvent_InventoryAdd, inum);
    }
}

void Character_AddWaypoint(Character *ch, int x, int y)
{
    Game &game = Game::Current();
    if (ch->room != game.DisplayedRoom)
    {
        quitprintf("!MoveCharacterPath: character %s is not in current room %d (it is in room %d)",
                   ch->scrname.GetCStr(), game.DisplayedRoom, ch->room);
    }

    // not already walking, so just do a normal move
    if (ch->walking <= 0)
    {
        Character_Walk(ch, x, y, IN_BACKGROUND, ANYWHERE);
        return;
    }

    MoveList &cmls = *get_movelist(ch->get_movelist_id());

    // They're already walking there anyway
    const Point &last_pos = cmls.GetLastPos();
    if (last_pos == Point(x, y))
        return;

    int move_speed_x, move_speed_y;
    ch->get_effective_walkspeeds(move_speed_x, move_speed_y);
    if ((move_speed_x == 0) && (move_speed_y == 0))
    {
        debug_script_warn("Character::AddWaypoint: called for '%s' with walk speed 0", ch->scrname.GetCStr());
    }

    MoveListOp::AddWaypointDirect(cmls, x, y, move_speed_x, move_speed_y, kMoveStage_Direct);
}

void Character_Animate(Character *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe,
                       int volume)
{
    // If idle view in progress for the character, stop the idle anim;
    // do this prior to the loop check, as the view may switch back to defview here
    stop_character_idling(ch);

    ValidateViewAnimVLF("Character.Animate", ch->scrname.GetCStr(), ch->view, loop, sframe);
    ValidateViewAnimParams("Character.Animate", ch->scrname.GetCStr(), blocking, repeat, direction);

    if (!ch->is_enabled())
    {
        debug_script_warn("%s: character %s is turned off, cannot run animation", "Character.Animate",
                          ch->scrname.GetCStr());
        return;
    }

    animate_character(ch, loop, delay, repeat, direction, sframe, volume);

    if (blocking != 0)
        GameLoopUntilViewAnimEnd(&ch->anim);
}

void Character_Animate5(Character *ch, int loop, int delay, int repeat, int blocking, int direction)
{
    Character_Animate(ch, loop, delay, repeat, blocking, direction, 0 /* first frame */, 100 /* full volume */);
}

void Character_Animate6(Character *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe)
{
    Character_Animate(ch, loop, delay, repeat, blocking, direction, sframe, 100 /* full volume */);
}

void Character_ChangeRoomAutoPosition(Character *ch, int room, int new_pos)
{
    Game &game = Game::Current();
    if (ch->index_id != game.playercharacter)
    {
        quit("!Character.ChangeRoomAutoPosition can only be used with the player character.");
    }

    game.RoomChange.Pos = new_pos;

    if (game.RoomChange.Pos == 0)
    {
        // auto place on other side of screen
        if (ch->x <= Room::Current().Edges.Left + 10)
        {
            game.RoomChange.Pos = 2000;
        }
        else if (ch->x >= Room::Current().Edges.Right - 10)
        {
            game.RoomChange.Pos = 1000;
        }
        else if (ch->y <= Room::Current().Edges.Top + 10)
        {
            game.RoomChange.Pos = 3000;
        }
        else if (ch->y >= Room::Current().Edges.Bottom - 10)
        {
            game.RoomChange.Pos = 4000;
        }

        if (game.RoomChange.Pos < 3000)
            game.RoomChange.Pos += ch->y;
        else
            game.RoomChange.Pos += ch->x;
    }
    NewRoom(room);
}

void Character_ChangeRoom(Character *ch, int room, int x, int y)
{
    Character_ChangeRoomSetLoop(ch, room, x, y, SCR_NO_VALUE);
}

void Character_ChangeRoomSetLoop(Character *ch, int room, int x, int y, int direction)
{
    Game &game = Game::Current();
    if (ch->index_id != game.playercharacter)
    {
        // NewRoomNPC
        if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE))
        {
            ch->x = x;
            ch->y = y;
            if (direction != SCR_NO_VALUE && direction >= 0)
                ch->loop = direction;
        }
        ch->prevroom = ch->room;
        ch->room = room;

        debug_script_log("%s moved to room %d, location %d,%d, loop %d", ch->scrname.GetCStr(), room, ch->x, ch->y,
                         ch->loop);

        return;
    }

    if ((x != SCR_NO_VALUE) && (y != SCR_NO_VALUE))
    {
        // We cannot set character position right away,
        // because room switch will occur only after the script end,
        // and character position may be still changing meanwhile.
        game.RoomChange.Pos = 0;
        // don't check X or Y bounds, so that they can do a
        // walk-in animation if they want
        game.RoomChange.X = x;
        game.RoomChange.Y = y;
        if (direction != SCR_NO_VALUE)
            game.RoomChange.Loop = direction;
    }

    NewRoom(room);
}


// Internal: set the character's active view directly, without changing
// their default view. Does not validate the view number, and expects
// it to be 0-based.
void Character_SetViewDirect(Character *ch, int view, bool is_walk_view)
{
    // if the idle animation is playing we should release the view
    stop_character_idling(ch);
    ch->view = view;
    stop_character_anim(ch);
    ch->frame = 0;
    ch->wait = 0;
    ch->walkwait = 0;
    ch->animwait = 0;
    FindReasonableLoopForCharacter(ch, is_walk_view);
}

void Character_ChangeView(Character *ch, int view)
{
    view--;

    if ((view < 0) || (view >= Game::Current().numviews))
    {
        quit("!ChangeCharacterView: invalid view number specified");
    }

    // if animating, but not idle view, give warning message
    if ((ch->flags & CHF_FIXVIEW) && (ch->idleleft >= 0))
    {
        debug_script_warn("ChangeCharacterView was used while the view was fixed - call ReleaseCharView first");
    }

    debug_script_log("%s: Change view to %d", ch->scrname.GetCStr(), view + 1);
    ch->defview = view;
    Character_SetViewDirect(ch, view, true);
}

enum DirectionalLoop
{
    kDirLoop_Down = 0,
    kDirLoop_Left = 1,
    kDirLoop_Right = 2,
    kDirLoop_Up = 3,
    kDirLoop_DownRight = 4,
    kDirLoop_UpRight = 5,
    kDirLoop_DownLeft = 6,
    kDirLoop_UpLeft = 7,

    kDirLoop_Default = kDirLoop_Down,
    kDirLoop_LastOrthogonal = kDirLoop_Up,
    kDirLoop_Last = kDirLoop_UpLeft,
};

// Internal direction-facing functions

float GetFaceDirRatio(Character *ch)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (ch->face_dir_ratio != 0.f)
        return ch->face_dir_ratio;
    // TODO: cache current area in Character somewhere during early char update
    int onarea = get_walkable_area_at_location(room, ch->x, ch->y);
    if (onarea > 0 && room.WalkAreas[onarea].FaceDirectionRatio != 0.f)
        return room.WalkAreas[onarea].FaceDirectionRatio;
    if (room.State->face_dir_ratio != 0.f)
        return room.State->face_dir_ratio;
    if (game.Play.face_dir_ratio != 0.f)
        return game.Play.face_dir_ratio;
    return 1.f;
}

// Get appropriate character loop for the given direction.
// Note that this requires the loop to have >1 frames so being suitable to animate the character.
DirectionalLoop GetDirectionalLoop(Character *ch, float x_diff, float y_diff, bool move_dir_fw = true)
{
    if (x_diff == 0.f && y_diff == 0.f)
        return static_cast<DirectionalLoop>(ch->loop);

    if (!move_dir_fw)
    {
        x_diff = -x_diff;
        y_diff = -y_diff;
    }

    // TODO: cache this in Character for bit more performance
    float dir_ratio = GetFaceDirRatio(ch);
    assert(dir_ratio != 0.f);
    y_diff *= dir_ratio; // dir ratio is a y/x relation

    const ViewStruct &chview = Game::Current().Views[ch->view];
    const bool has_down_loop = ((chview.numLoops > kDirLoop_Down) && (chview.loops[kDirLoop_Down].numFrames > 0));
    const bool has_up_loop = ((chview.numLoops > kDirLoop_Up) && (chview.loops[kDirLoop_Up].numFrames > 0));
    // NOTE: left and right loops are required to be present at all times (as 3.+ games did)
    const bool has_left_loop = true;
    const bool has_right_loop = true;
    const bool has_diagonal_loops = should_use_diagloops(ch, true /* require >1 frames in loops */);

    const bool want_horizontal = (abs(y_diff) < abs(x_diff)) || (!has_down_loop || !has_up_loop);

    DirectionalLoop next_loop = kDirLoop_Left; // NOTE: the default loop is Left
    if (want_horizontal)
    {
        const bool want_diagonal = has_diagonal_loops && (abs(y_diff) > abs(x_diff) / 2);
        if (!has_left_loop && !has_right_loop)
        {
            next_loop = kDirLoop_Down;
        }
        else if (has_right_loop && (x_diff > 0))
        {
            next_loop = want_diagonal ? (y_diff < 0 ? kDirLoop_UpRight : kDirLoop_DownRight) : kDirLoop_Right;
        }
        else if (has_left_loop && (x_diff <= 0))
        {
            next_loop = want_diagonal ? (y_diff < 0 ? kDirLoop_UpLeft : kDirLoop_DownLeft) : kDirLoop_Left;
        }
    }
    else
    {
        const bool want_diagonal = has_diagonal_loops && (abs(x_diff) > abs(y_diff) / 2);
        if (y_diff > 0 || !has_up_loop)
        {
            next_loop = want_diagonal ? (x_diff < 0 ? kDirLoop_DownLeft : kDirLoop_DownRight) : kDirLoop_Down;
        }
        else
        {
            next_loop = want_diagonal ? (x_diff < 0 ? kDirLoop_UpLeft : kDirLoop_UpRight) : kDirLoop_Up;
        }
    }
    return next_loop;
}

void FaceDirectionalLoop(Character *ch, int direction, int blocking_style)
{
    Game &game = Game::Current();
    // Stop any kind of a side activity: turning, idling
    ch->StopTurning();
    stop_character_idling(ch);

    // Change facing only if the desired direction is different
    if (direction != ch->loop)
    {
        if ((game.options[OPT_CHARTURNWHENFACE] != 0) && ((ch->flags & CHF_TURNWHENFACE) != 0) &&
            (!game.Events.InEntersScreen))
        {
            const bool use_diagloops = should_use_diagloops(ch, false /* allow 1 frame in loops */);
            const int highest_loop = use_diagloops ? kDirLoop_Last : kDirLoop_LastOrthogonal;
            if ((ch->loop <= highest_loop))
            {
                // Turn to face new direction
                Character_StopMoving(ch);
                if (ch->is_enabled())
                {
                    // only do the turning if the character is not hidden
                    // (otherwise GameLoopUntilNotMoving will never return)
                    start_character_turning(ch, direction, use_diagloops);

                    if ((blocking_style == BLOCKING) || (blocking_style == 1))
                    {
                        GameLoopUntilFlagUnset(&ch->RuntimeFlags, kCharf_TurningMask);
                    }
                }
                else
                    ch->loop = direction;
            }
            else
                ch->loop = direction;
        }
        else
            ch->loop = direction;
    }

    ch->frame = 0;
    reset_character_idling_time(ch);
}

void FaceLocationXY(Character *ch, int x, int y, int blocking_style)
{
    debug_script_log("%s: Face location %d,%d", ch->scrname.GetCStr(), x, y);

    const int diffrx = x - ch->x;
    const int diffry = y - ch->y;
    FaceDirectionalLoop(ch, GetDirectionalLoop(ch, diffrx, diffry), blocking_style);
}

// External direction-facing functions with validation

void Character_FaceDirection(Character *ch, int direction, int blocking_style)
{
    if (ch == nullptr)
    {
        quit("!FaceDirection: invalid character specified");
    }

    if (direction < 0 || direction > kDirLoop_Last)
        quit("!FaceDirection: invalid direction specified");

    FaceDirectionalLoop(ch, direction, blocking_style);
}

void Character_FaceLocation(Character *ch, int x, int y, int blocking_style)
{
    if (ch == nullptr)
    {
        quit("!FaceLocation: invalid character specified");
    }

    FaceLocationXY(ch, x, y, blocking_style);
}

void Character_FaceObject(Character *ch, ScriptObject *obj, int blocking_style)
{
    Room &room = Room::Current();
    if (obj == nullptr)
        quit("!FaceObject: invalid object specified");

    FaceLocationXY(ch, room.Objs[obj->id].x, room.Objs[obj->id].y, blocking_style);
}

void Character_FaceCharacter(Character *ch, Character *other, int blocking_style)
{
    if (other == nullptr)
    {
        quit("!FaceCharacter: invalid character specified");
    }

    if (ch->room != other->room)
    {
        quitprintf("!FaceCharacter: characters %s and %s are in different rooms (room %d and room %d respectively)",
                   ch->scrname.GetCStr(), other->scrname.GetCStr(), ch->room, other->room);
    }

    FaceLocationXY(ch, other->x, other->y, blocking_style);
}

void Character_FollowCharacter(Character *ch, Character *to_follow, int distaway, int eagerness)
{
    // FOLLOW_ALWAYSONTOP constant limits distaway to 32766
    if ((distaway < 0) || (distaway > FOLLOW_ALWAYSONTOP))
        quitprintf("!FollowCharacterEx: invalid distance: must be 0-%d or FOLLOW_EXACTLY", FOLLOW_ALWAYSONTOP - 1);
    if ((eagerness < 0) || (eagerness > 250))
        quit("!FollowCharacterEx: invalid eagerness: must be 0-250");

    if ((ch->index_id == Game::Current().playercharacter) && (to_follow != nullptr) && (to_follow->room != ch->room))
    {
        quitprintf("!FollowCharacterEx: you cannot tell the player character %s, who is in room %d, to follow a "
                   "character %s who is in another room %d",
                   ch->scrname.GetCStr(), ch->room, to_follow->scrname.GetCStr(), to_follow->room);
    }

    if (to_follow != nullptr)
    {
        debug_script_log("%s: Start following %s (dist %d, eager %d)", ch->scrname.GetCStr(),
                         to_follow->scrname.GetCStr(), distaway, eagerness);
    }
    else
    {
        debug_script_log("%s: Stop following other character", ch->scrname.GetCStr());
    }

    if ((ch->following >= 0) && (ch->follow_dist == FOLLOW_ALWAYSONTOP))
    {
        // if this character was following always-on-top, its baseline will
        // have been changed, so release it.
        ch->baseline = -1;
    }

    ch->SetFollowing(to_follow ? to_follow->index_id : -1, distaway, eagerness, (eagerness == 1));

    if (ch->IsAnimatingRepeatedly())
        debug_script_warn(
            "FollowCharacter called but the sheep is currently animating looped. It may never start to follow.");
}

Character *Character_GetFollowing(Character *ch)
{
    if (ch->following < 0)
    {
        return nullptr;
    }

    return &Game::Current().Characters[ch->following];
}

int GetCharacterWidth(Game &game, int char_id)
{
    Character *ch = &game.Characters[char_id];

    if (ch->width < 1)
    {
        if ((ch->view < 0) || (ch->loop >= game.Views[ch->view].numLoops) ||
            (ch->frame >= game.Views[ch->view].loops[ch->loop].numFrames))
        {
            debug_script_warn("GetCharacterWidth: Character %s has invalid frame: view %d, loop %d, frame %d",
                              ch->scrname.GetCStr(), ch->view + 1, ch->loop, ch->frame);
            return 4;
        }

        return game.SpriteInfos[game.Views[ch->view].loops[ch->loop].frames[ch->frame].pic].Width;
    }
    else
        return ch->width;
}

int GetCharacterHeight(Game &game, int char_id)
{
    Character *ch = &game.Characters[char_id];

    if (ch->height < 1)
    {
        if ((ch->view < 0) || (ch->loop >= game.Views[ch->view].numLoops) ||
            (ch->frame >= game.Views[ch->view].loops[ch->loop].numFrames))
        {
            debug_script_warn("GetCharacterHeight: Character %s has invalid frame: view %d, loop %d, frame %d",
                              ch->scrname.GetCStr(), ch->view + 1, ch->loop, ch->frame);
            return 2;
        }

        return game.SpriteInfos[game.Views[ch->view].loops[ch->loop].frames[ch->frame].pic].Height;
    }
    else
        return ch->height;
}

int Character_IsCollidingWithChar(Character *ch, Character *other)
{
    Game &game = Game::Current();
    if (other == nullptr)
    {
        quit("!AreCharactersColliding: invalid char2");
    }

    if (ch->room != other->room)
    {
        return 0; // not colliding
    }

    if ((ch->y > other->y - 5) && (ch->y < other->y + 5))
    {
        ;
    }
    else
        return 0;

    int w1 = GetCharacterWidth(game, ch->index_id);
    int w2 = GetCharacterWidth(game, other->index_id);

    int xps1 = ch->x - w1 / 2;
    int xps2 = other->x - w2 / 2;

    if ((xps1 >= xps2 - w1) & (xps1 <= xps2 + w2))
        return 1;
    return 0;
}

int Character_IsCollidingWithObject(Character *ch, ScriptObject *obj)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if (obj == nullptr)
    {
        quit("!AreCharObjColliding: invalid object number");
    }

    if (ch->room != game.DisplayedRoom)
    {
        return 0;
    }
    if (!room.Objs[obj->id].is_enabled())
    {
        return 0;
    }

    // TODO: use GraphicSpace and proper transformed coords?

    Bitmap *obj_pic = GetObjectImage(obj->id);
    const int obj_w = obj_pic->GetWidth();
    const int obj_h = obj_pic->GetHeight();
    const Rect obj_rc = RectWH(room.Objs[obj->id].x, room.Objs[obj->id].y - obj_h, obj_w, obj_h);

    Bitmap *ch_pic = GetCharacterImage(ch->index_id);
    const int char_w = ch_pic->GetWidth();
    const int char_h = ch_pic->GetHeight();
    // Only check feet (few bottom pixels)
    // FIXME: why is this hardcoded as "5" pixels, how does this work in hi-res games, how to let customize this?
    const Rect ch_rc = RectWH(ch->x - char_w / 2, ch->GetEffectiveY() - 5, char_w, 5);

    const Rect intersect = IntersectRects(obj_rc, ch_rc);
    if (!intersect.IsEmpty())
    {
        // If no pixel-perfect test is required, then the intersection is enough
        if (game.options[OPT_PIXPERFECT] == 0)
            return 1;

        // Pixel-perfect test: check if they're on a non-transparent bit of the object
        const int ch_maskcolc = ch_pic->GetMaskColor();
        const int obj_maskcol = obj_pic->GetMaskColor();
        for (int ix = intersect.Left, cx = intersect.Left - ch_rc.Left, ox = intersect.Left - obj_rc.Left;
             ix <= intersect.Right; ix += 1)
        {
            for (int iy = intersect.Top, cy = intersect.Top - ch_rc.Top, oy = intersect.Top - obj_rc.Top;
                 iy <= intersect.Bottom; iy += 1)
            {
                int ch_px = ch_pic->GetPixel(ix + cx, iy + cy);
                int obj_px = obj_pic->GetPixel(ix + ox, iy + oy);
                if ((ch_px != ch_maskcolc) && (obj_px != obj_maskcol))
                    return 1;
            }
        }
    }
    return 0;
}

bool Character_IsInteractionAvailable(Character *ch, int mood)
{
    Game &game = Game::Current();
    game.Play.check_interaction_only = 1;
    Character_RunInteraction(ch, mood);
    int ciwas = game.Play.check_interaction_only;
    game.Play.check_interaction_only = 0;
    return (ciwas == 2);
}

void Character_LockViewImpl(Character *ch, const char *api_name, int view, int loop, int frame, const Pointf &anchor,
                            const Point &offset, bool stop_moving)
{
    view--; // convert to 0-based
    AssertView(api_name, ch->scrname.GetCStr(), view);
    AssertViewHasLoops(api_name, ch->scrname.GetCStr(), view);

    if (loop >= 0)
        AssertLoop(api_name, ch->scrname.GetCStr(), view, loop);
    else
        loop = 0;
    if (frame >= 0)
        AssertFrame(api_name, ch->scrname.GetCStr(), view, loop, frame);
    else
        frame = 0;

    stop_character_idling(ch);
    if (stop_moving)
    {
        Character_StopMoving(ch);
    }
    stop_character_anim(ch);
    ch->SetLockedView(view, loop, frame, anchor, offset);
    FindReasonableLoopForCharacter(ch, false);
    debug_script_log("%s: View locked to %d", ch->scrname.GetCStr(), view + 1);
}

void Character_LockViewEx(Character *ch, int view, int stop_moving)
{
    Character_LockViewImpl(ch, "Character.LockView", view, -1, -1, CharacterInfo::GetDefaultSpriteAnchor(), Point(),
                           stop_moving != 0);
}

void Character_LockView(Character *ch, int view)
{
    Character_LockViewEx(ch, view, STOP_MOVING);
}

void Character_LockViewAlignedEx(Character *ch, int view, int loop, int align, int stop_moving)
{
    if ((align & kMAlignAny) == 0)
    {
        debug_script_warn("Character.LockViewAligned: invalid alignment type specified");
    }

    Character_LockViewImpl(ch, "Character.LockViewAligned", view, loop, -1,
                           GfxDef::GetGraphicAnchorFromAlignment(static_cast<FrameAlignment>(align)), Point(),
                           stop_moving != 0);
}

void Character_LockViewAnchored(Character *ch, int view, float x_anchor, float y_anchor, int x_off, int y_off,
                                int stop_moving)
{
    Character_LockViewImpl(ch, "Character.LockViewAnchored", view, -1, -1, Pointf(x_anchor, y_anchor),
                           Point(x_off, y_off), stop_moving != 0);
}

void Character_LockViewFrameEx(Character *ch, int view, int loop, int frame, int stop_moving)
{
    Character_LockViewImpl(ch, "Character.LockViewFrame", view, loop, frame, CharacterInfo::GetDefaultSpriteAnchor(),
                           Point(), stop_moving != 0);
}

void Character_LockViewOffsetEx(Character *ch, int view, int xoffs, int yoffs, int stop_moving)
{
    Character_LockViewImpl(ch, "Character.LockViewOffset", view, -1, -1, CharacterInfo::GetDefaultSpriteAnchor(),
                           Point(xoffs, yoffs), stop_moving != 0);
}

void Character_LoseInventoryImpl(Character *ch, int item_id, int item_inv_index)
{
    Game &game = Game::Current();
    const int quant = ch->get_item_quantity(item_id);
    ch->set_item_quantity(item_id, quant - 1);
    const bool no_item_left = (quant - 1) == 0;

    // If this was a selected item, and its quantity drops to zero,
    // then reset the active inventory and "use inv" cursor
    if ((ch->activeinv == item_id) && no_item_left)
    {
        ch->activeinv = -1;
        if ((ch == game.PlayerChar) && (is_current_cursor_mode(kCursorRole_UseInv)))
        {
            set_cursor_mode(0); // change to the first enabled mode
        }
    }

    const int char_id = ch->index_id;
    if (item_inv_index >= 0)
    {
        // Remove from particular inventory slot
        ch->inventory.erase(ch->inventory.begin() + item_inv_index);
    }
    else
    {
        // Remove one item of this kind (it will be the only item if no duplicates allowed)
        if (no_item_left || (game.options[OPT_DUPLICATEINV] > 0))
        {
            auto it_found = std::find(ch->inventory.begin(), ch->inventory.end(), item_id);
            if (it_found != ch->inventory.end())
            {
                ch->inventory.erase(it_found);
            }
        }
    }
    // Redraw active inventory window(s) for this character
    GUIE::MarkInventoryForUpdate(char_id, char_id == game.playercharacter);

    if (ch == game.PlayerChar)
    {
        run_on_event(kScriptEvent_InventoryLose, item_id);
    }
}

void Character_LoseInventory(Character *ch, ScriptInvItem *item)
{
    if (item == nullptr)
    {
        quit("!LoseInventoryFromCharacter: invalid inventory number");
    }

    const int item_id = item->id;
    if (!ch->has_any_of_item(item_id))
    {
        debug_script_warn("Character.LoseInventory: char %s, item %s: no such item in inventory", ch->scrname.GetCStr(),
                          Game::Current().invScriptNames[item_id].GetCStr());
        return;
    }

    Character_LoseInventoryImpl(ch, item_id, -1);
}

void Character_LoseInventoryAt(Character *ch, int inv_index)
{
    if (ch->inventory.empty())
    {
        debug_script_error("Character.RemoveInventoryAt: char %s has no items", ch->scrname.GetCStr());
        return;
    }
    if (inv_index < 0 || static_cast<size_t>(inv_index) >= ch->inventory.size())
    {
        debug_script_error("Character.RemoveInventoryAt: char %s has no item at index %d, inventory range is 0..%zu",
                           ch->scrname.GetCStr(), inv_index, ch->inventory.size() - 1);
        return;
    }

    Character_LoseInventoryImpl(ch, ch->inventory[inv_index], inv_index);
}

void Character_PlaceOnWalkableArea(Character *ch)
{
    if (Game::Current().DisplayedRoom < 0)
        quit("!Character.PlaceOnWalkableArea: no room is currently loaded");

    Point dst;
    if (FindNearestWalkableAreaForCharacter(Point(ch->x, ch->y), dst))
    {
        ch->x = dst.X;
        ch->y = dst.Y;
    }
}

void Character_RemoveTint(Character *ch)
{
    if (ch->flags & (CHF_HASTINT | CHF_HASLIGHT))
    {
        debug_script_log("Un-tint %s", ch->scrname.GetCStr());
        ch->flags &= ~(CHF_HASTINT | CHF_HASLIGHT);
    }
    else
    {
        debug_script_warn("Character.RemoveTint called but character was not tinted");
    }
}

int Character_GetHasExplicitTint(Character *ch)
{
    return ch->has_explicit_tint();
}

void Character_Say(Character *ch, const char *text)
{
    DisplaySpeechCore(ch->index_id, text);
}

void Character_SayAt(Character *ch, int x, int y, int width, const char *text)
{
    DisplaySpeechAt(x, y, width, ch->index_id, text);
}

ScriptOverlay *Character_SayBackground(Character *ch, const char *text)
{
    int ovltype = DisplaySpeechBackground(ch->index_id, text);
    auto *over = get_overlay(ovltype);
    if (!over)
        quit("!SayBackground internal error: no overlay");
    // Create script object with an internal ref, keep at least until internal timeout
    ScriptOverlay *scover = over->CreateScriptObject();
    ccAddObjectReference(over->GetScriptHandle());
    return scover;
}

// [DEPRECATED] the former script function; still called by gui.cpp, savegame.cpp and Character_SetAsPlayer.
// TODO: call Character_SetActiveInventory(Game::PlayerChar, ...) there instead and delete this function.
void SetActiveInventory(int item_id)
{
    Game &game = Game::Current();
    if (!AssertInvItem("SetActiveInventory", item_id))
    {
        return;
    }

    Character_SetActiveInventory(game.PlayerChar, &game.ScInvItems[item_id]);
}

void Character_SetAsPlayer(Character *ch)
{
    Game &game = Game::Current();
    // Set to same character, so ignore.
    if (game.playercharacter == ch->index_id)
    {
        return;
    }

    setup_player_character(ch->index_id);
    GUIE::MarkInventoryForUpdate(game.playercharacter, true);
    debug_script_log("%s is new player character", game.PlayerChar->scrname.GetCStr());

    // Within game_start, return now
    if (game.DisplayedRoom < 0)
        return;

    if (game.DisplayedRoom != game.PlayerChar->room)
        NewRoom(game.PlayerChar->room);
    else // make sure it doesn't run the region interactions
        game.Play.player_on_region =
            GetRegionIDAtRoom(Room::Current(), game.PlayerChar->x, game.PlayerChar->y, kHit_Interactable);

    if ((game.PlayerChar->activeinv >= 0) && !game.PlayerChar->has_any_of_item(game.PlayerChar->activeinv))
        game.PlayerChar->activeinv = -1;

    // They had inv selected, so change the cursor
    if (is_current_cursor_mode(kCursorRole_UseInv))
    {
        if (game.PlayerChar->activeinv < 0)
            Mouse_SetNextCursor();
        else
            SetActiveInventory(game.PlayerChar->activeinv);
    }
}

void Character_SetIdleView(Character *ch, int iview, int itime)
{
    if (iview == 1)
        quit("!SetCharacterIdle: view 1 cannot be used as an idle view, sorry.");

    // if an idle anim is currently playing, release it
    stop_character_idling(ch);

    ch->idleview = iview - 1;
    // make sure they don't appear idle while idle anim is disabled
    if (iview < 1)
        itime = 10;
    ch->idledelay = itime;
    ch->idleleft = itime;

    // if not currently animating, reset the wait counter
    if ((!ch->IsAnimating()) && (!ch->is_moving()))
    {
        ch->wait = 0;
    }

    if (iview >= 1)
    {
        debug_script_log("Set %s idle view to %d (time %d)", ch->scrname.GetCStr(), iview, itime);
    }
    else
    {
        debug_script_log("%s idle view disabled", ch->scrname.GetCStr());
    }
    if (ch->flags & CHF_FIXVIEW)
    {
        debug_script_warn("SetCharacterIdle called while character view locked with SetCharacterView; idle ignored");
        debug_script_log("View locked, idle will not kick in until Released");
    }
    // if they switch to a swimming animation, kick it off immediately
    if (itime == 0)
        ch->process_idle_this_time = 1;
}

bool Character_GetHasExplicitLight(Character *ch)
{
    return ch->has_explicit_light();
}

int Character_GetLightLevel(Character *ch)
{
    return ch->has_explicit_light() ? ch->tint_light : 0;
}

void Character_SetLightLevel(Character *ch, int light_level)
{
    light_level = Math::Clamp(light_level, -255, 255);

    ch->tint_light = light_level;
    ch->flags &= ~CHF_HASTINT;
    ch->flags |= CHF_HASLIGHT;
}

int Character_GetTintRed(Character *ch)
{
    return ch->has_explicit_tint() ? ch->tint_r : 0;
}

int Character_GetTintGreen(Character *ch)
{
    return ch->has_explicit_tint() ? ch->tint_g : 0;
}

int Character_GetTintBlue(Character *ch)
{
    return ch->has_explicit_tint() ? ch->tint_b : 0;
}

int Character_GetTintSaturation(Character *ch)
{
    return ch->has_explicit_tint() ? ch->tint_level : 0;
}

int Character_GetTintLuminance(Character *ch)
{
    return ch->has_explicit_tint() ? ch->tint_light : 0;
}

void Character_SetSpeed(Character *ch, int xspeed, int yspeed)
{
    if ((xspeed == 0) || (yspeed == 0))
        quit("!SetCharacterSpeedEx: invalid speed value");

    xspeed = Math::Clamp(xspeed, (int)INT16_MIN, (int)INT16_MAX);
    yspeed = Math::Clamp(yspeed, (int)INT16_MIN, (int)INT16_MAX);

    uint16_t old_speedx = ch->walkspeed;
    uint16_t old_speedy = ((ch->walkspeed_y == UNIFORM_WALK_SPEED) ? ch->walkspeed : ch->walkspeed_y);

    ch->walkspeed = xspeed;
    if (yspeed == xspeed)
        ch->walkspeed_y = UNIFORM_WALK_SPEED;
    else
        ch->walkspeed_y = yspeed;

    if (ch->is_moving() && (old_speedx != xspeed || old_speedy != yspeed))
    {
        MoveListOp::RecalculateMoveSpeeds(*get_movelist(ch->get_movelist_id()), old_speedx, old_speedy, xspeed, yspeed);
    }
}

void Character_StopMoving(Character *ch)
{
    Character_StopMovingEx(ch, ch->is_moving() && (!get_movelist(ch->get_movelist_id())->IsStageDirect()));
}

void Character_StopMovingEx(Character *ch, bool force_walkable_area)
{
    Game &game = Game::Current();
    // If we have a movelist reference attached, then invalidate and dec refcount
    if (ch->movelist_handle > 0)
    {
        release_script_movelist(ch->movelist_handle);
        ch->movelist_handle = 0;
    }

    int char_id = ch->index_id;
    if (char_id == game.Play.skip_until_char_stops)
    {
        EndSkippingUntilCharStops();
    }

    // A walk at a very small zoom level shows the character half way through a step, and keeps the position
    // of the full step in xwas/ywas (see UpdateCharacterMoving); restore it before stopping
    if (ch->xwas != INVALID_X)
    {
        ch->x = ch->xwas;
        ch->y = ch->ywas;
        ch->xwas = INVALID_X;
    }

    // If requested by a caller, then validate the character position,
    // ensuring that it does not end on a non-walkable area and gets stuck.
    // Because the pathfinder is not ideal, and there are various sources of
    // inaccuracy in the walking logic, in rare cases the character might
    // pass through or even finish on a non-walkable pixel.
    if (force_walkable_area && (ch->room == game.DisplayedRoom))
    {
        // TODO: don't use PlaceOnWalkable, as that may result in moving character
        // into the random direction.
        // For finishing a walk clamp to a precalculated final destination instead
        // (the end of a pathfinder route is walkable, the end of a path given by a script is not
        // guaranteed to be, so the clamp would be limited to routes).
        // For when interrupting a move, consider writing a "backtracing"
        // function that runs along MoveList's current stage back, looking for
        // the first pixel on a walkable area.
        Character_PlaceOnWalkableArea(ch);
    }

    // Always reset any turning
    ch->StopTurning();
    // If the character is currently moving, stop them and reset their state
    if (ch->walking > 0)
    {
        // Switch character state from walking to standing
        remove_movelist(ch->get_movelist_id());
        ch->walking = 0;
        // If the character was animating a walk, then reset their frame to standing
        if (ch->is_moving_walkanim())
        {
            ch->frame = 0;
        }
        // Restart idle timer
        reset_character_idling_time(ch);
        debug_script_log("%s: stop moving", ch->scrname.GetCStr());
    }

    // Always clear the move-related flags (for safety)
    // NOTE: I recall there was a potential case when this flag could remain after Move...
    ch->flags &= ~CHF_MOVENOTWALK;
}

void Character_Tint(Character *ch, int red, int green, int blue, int opacity, int luminance)
{
    if ((red < 0) || (green < 0) || (blue < 0) || (red > 255) || (green > 255) || (blue > 255) || (opacity < 0) ||
        (opacity > 255) || (luminance < 0) || (luminance > 255))
    {
        debug_script_warn("Character.Tint: invalid parameter(s). R,G,B must be 0-255 (passed: %d,%d,%d), opacity & "
                          "luminance 0-255 (passed: %d,%d)",
                          red, green, blue, opacity, luminance);
        return;
    }

    debug_script_log("Set %s tint RGB(%d,%d,%d) %d%%", ch->scrname.GetCStr(), red, green, blue, opacity);

    ch->tint_r = red;
    ch->tint_g = green;
    ch->tint_b = blue;
    ch->tint_level = opacity;
    ch->tint_light = luminance;
    ch->flags &= ~CHF_HASLIGHT;
    ch->flags |= CHF_HASTINT;
}

void Character_Think(Character *ch, const char *text)
{
    DisplayThoughtCore(ch->index_id, text);
}

void Character_UnlockView(Character *ch)
{
    Character_UnlockViewEx(ch, STOP_MOVING);
}

void Character_UnlockViewEx(Character *ch, int stop_moving)
{
    if (ch->flags & CHF_FIXVIEW)
    {
        debug_script_log("%s: Released view back to default", ch->scrname.GetCStr());
    }
    ch->SetUnlockedView();
    if (stop_moving != KEEP_MOVING)
    {
        Character_StopMoving(ch);
    }
    if (ch->view >= 0)
    {
        FindReasonableLoopForCharacter(ch, true);
    }
    stop_character_anim(ch);
    // Restart idle timer
    reset_character_idling_time(ch);
}

// Tests if the given character is permitted to start a move in the room
bool ValidateCharForMove(Character *ch, const char *api_name)
{
    Game &game = Game::Current();
    if (ch->room != game.DisplayedRoom)
    {
        debug_script_warn("%s: specified character %s not in current room (is in %d, current room %d)", api_name,
                          ch->scrname.GetCStr(), ch->room, game.DisplayedRoom);
        return false;
    }
    if (!ch->is_enabled())
    {
        debug_script_warn("%s: character %s is turned off and cannot be moved", api_name, ch->scrname.GetCStr());
        return false;
    }
    return true;
}

// Character_DoMove converts and validates script parameters, and calls corresponding internal character move function
static void Character_DoMove(Character *ch, const char *api_name, void *path_arr, int x, int y, bool use_path,
                             bool walk_straight, int blocking, int ignwal, bool walk_anim, int repeat = kAnimFlow_Once,
                             int direction = FORWARDS)
{
    if (!ValidateCharForMove(ch, api_name))
    {
        return;
    }

    ValidateMoveParams(api_name, ch->scrname.GetCStr(), blocking, ignwal);
    ValidateAnimParams(api_name, ch->scrname.GetCStr(), repeat, direction);

    if (use_path)
    {
        std::vector<Point> path;
        if (!path_arr || !ScriptStructHelpers::ResolveArrayOfPoints(path_arr, path))
        {
            debug_script_warn("%s: path is null, or failed to resolve array of points", api_name);
            Character_StopMoving(ch);
            return;
        }
        move_character(ch, path, walk_anim,
                       RunPathParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction)));
    }
    else if (walk_straight)
    {
        move_character_straight(ch, x, y, walk_anim);
    }
    else
    {
        move_character(ch, x, y, ignwal != 0, walk_anim);
    }

    if (blocking)
        GameLoopUntilNotMoving(&ch->walking);
}

void Character_Walk(Character *ch, int x, int y, int blocking, int ignwal)
{
    Character_DoMove(ch, "Character.Walk", nullptr, x, y, false /* no path */, false /* not straight */, blocking,
                     ignwal, true /* walk anim */);
}

void Character_Move(Character *ch, int x, int y, int blocking, int ignwal)
{
    Character_DoMove(ch, "Character.Move", nullptr, x, y, false /* no path */, false /* not straight */, blocking,
                     ignwal, false /* no anim */);
}

void Character_WalkStraight(Character *ch, int x, int y, int blocking)
{
    Character_DoMove(ch, "Character.WalkStraight", nullptr, x, y, false /* no path */, true /* straight */, blocking,
                     WALKABLE_AREAS, true /* walk anim */);
}

void Character_MoveStraight(Character *ch, int x, int y, int blocking)
{
    Character_DoMove(ch, "Character.MoveStraight", nullptr, x, y, false /* no path */, true /* straight */, blocking,
                     WALKABLE_AREAS, false /* no anim */);
}

void Character_WalkPath(Character *ch, void *path_arr, int blocking, int repeat, int direction)
{
    Character_DoMove(ch, "Character.WalkPath", path_arr, 0, 0, true /* use path */, false /* not straight */, blocking,
                     ANYWHERE, true /* walk anim */, repeat, direction);
}

void Character_MovePath(Character *ch, void *path_arr, int blocking, int repeat, int direction)
{
    Character_DoMove(ch, "Character.WalkPath", path_arr, 0, 0, true /* use path */, false /* not straight */, blocking,
                     ANYWHERE, false /* no anim */, repeat, direction);
}

bool Character_RunFrameEvent(Character *ch, int view, int loop, int frame)
{
    view--; // convert to internal 0-based view ID
    AssertFrame("Character.RunFrameEvent", ch->scrname.GetCStr(), view, loop, frame);
    return ch->RunFrameEvent(view, loop, frame);
}

void Character_RunInteraction(Character *ch, int mood)
{
    Game &game = Game::Current();
    if (!AssertCursorValidForEvent("Character.RunInteraction", mood))
        return;

    // Cursor mode must match the event index in "Interactions" table,
    // except when it does not have a "event" flag
    // TODO: do we need extra flag telling if to trigger "any click" for this mode?
    const int evnt = (game.mcurs[mood].flags & MCF_EVENT) != 0 ? mood : -1;
    const int anyclick_evt = kCharacterEvent_AnyClick;

    // For USE verb: remember active inventory
    if (game.HasCursorRole(mood, kCursorRole_UseInv))
    {
        game.Play.usedinv = game.PlayerChar->activeinv;
    }

    const auto obj_evt = ObjectEvent(kScTypeGame, LOCTYPE_CHAR, ch->index_id,
                                     RuntimeScriptValue().SetScriptObject(ch, &ccDynamicCharacter), mood);
    if ((evnt >= 0) &&
        run_event_script(obj_evt, &game.Characters[ch->index_id].interactions, evnt,
                         &game.Characters[ch->index_id].GetEvents(), anyclick_evt, true /* do unhandled event */) < 0)
    {
        return; // game state changed, don't do "any click"
    }
    // any click on char
    run_event_script(obj_evt, &game.Characters[ch->index_id].GetEvents(), anyclick_evt);
}

void *Character_GetGraphicPosition(Character *ch)
{
    ch->UpdateGraphicSpace();
    std::vector<Point> points;
    ch->GetGraphicSpace().GetTransformedCorners(points, Size(ch->spr_width, ch->spr_height));
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void *Character_GetGraphicBoundBox(Character *ch)
{
    ch->UpdateGraphicSpace();
    std::vector<Point> points;
    ch->GetGraphicSpace().GetAABBPoints(points);
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

int Character_GetProperty(Character *ch, const char *property)
{
    Game &game = Game::Current();
    if (!AssertCharacter("Character.GetProperty", ch->index_id))
    {
        return 0;
    }
    return get_int_property(game.charProps[ch->index_id], game.Play.charProps[ch->index_id], property);
}

void Character_GetPropertyText(Character *ch, const char *property, char *buffer)
{
    Game &game = Game::Current();
    if (!AssertCharacter("Character.GetPropertyText", ch->index_id))
    {
        return;
    }
    get_text_property(game.charProps[ch->index_id], game.Play.charProps[ch->index_id], property, buffer);
}

const char *Character_GetTextProperty(Character *ch, const char *property)
{
    Game &game = Game::Current();
    if (!AssertCharacter("Character.GetTextProperty", ch->index_id))
    {
        return nullptr;
    }
    return get_text_property_dynamic_string(game.charProps[ch->index_id], game.Play.charProps[ch->index_id], property);
}

bool Character_SetProperty(Character *ch, const char *property, int value)
{
    if (!AssertCharacter("Character.SetProperty", ch->index_id))
    {
        return false;
    }
    return set_int_property(Game::Current().Play.charProps[ch->index_id], property, value);
}

bool Character_SetTextProperty(Character *ch, const char *property, const char *value)
{
    if (!AssertCharacter("Character.SetTextProperty", ch->index_id))
    {
        return false;
    }
    return set_text_property(Game::Current().Play.charProps[ch->index_id], property, value);
}

ScriptInvItem *Character_GetActiveInventory(Character *ch)
{
    if (ch->activeinv <= 0)
    {
        return nullptr;
    }

    return &Game::Current().ScInvItems[ch->activeinv];
}

void Character_SetActiveInventory(Character *ch, ScriptInvItem *item)
{
    Game &game = Game::Current();
    if (item == nullptr)
    {
        ch->activeinv = -1;

        if (ch->index_id == game.playercharacter)
        {
            if (is_current_cursor_mode(kCursorRole_UseInv))
                set_cursor_mode(0); // change to the first enabled mode
        }
        GUIE::MarkInventoryForUpdate(ch->index_id, ch->index_id == game.playercharacter);
        return;
    }

    if (!ch->has_any_of_item(item->id))
    {
        debug_script_warn("SetActiveInventory: character doesn't have any of that inventory");
        return;
    }

    ch->activeinv = item->id;

    if (ch->index_id == game.playercharacter)
    {
        // if it's the player character, update mouse cursor
        update_inv_cursor(item->id);
        set_cursor_mode_with_role(kCursorRole_UseInv, -1);
    }
    GUIE::MarkInventoryForUpdate(ch->index_id, ch->index_id == game.playercharacter);
}

int Character_GetAnimating(Character *ch)
{
    return ch->IsAnimating() ? 1 : 0;
}

int Character_GetAnimationSpeed(Character *ch)
{
    return ch->animspeed;
}

void Character_SetAnimationSpeed(Character *ch, int new_value)
{
    ch->animspeed = new_value;
}

int Character_GetAudioPanning(Character *ch)
{
    return ch->audio_panning;
}

void Character_SetAudioPanning(Character *ch, int new_value)
{
    ch->audio_panning = Math::Clamp(new_value, -100, 100);
}

int Character_GetAudioSpeed(Character *ch)
{
    return ch->AudioSpeed;
}

void Character_SetAudioSpeed(Character *ch, int new_value)
{
    ch->AudioSpeed = std::max(0, new_value);
}

int Character_GetAudioVolume(Character *ch)
{
    return ch->audio_volume;
}

void Character_SetAudioVolume(Character *ch, int new_value)
{
    ch->audio_volume = Math::Clamp(new_value, 0, 100);
}

int Character_GetBaseline(Character *ch)
{
    if (ch->baseline < 1)
    {
        return 0;
    }

    return ch->baseline;
}

void Character_SetBaseline(Character *ch, int baseline)
{
    ch->baseline = baseline;
}

int Character_GetBlinkInterval(Character *ch)
{
    return ch->blinkinterval;
}

void Character_SetBlinkInterval(Character *ch, int interval)
{
    if (interval < 0)
        quit("!SetCharacterBlinkView: invalid blink interval");

    ch->blinkinterval = interval;

    if (ch->blinktimer > 0)
    {
        ch->blinktimer = ch->blinkinterval;
    }
}

int Character_GetBlinkView(Character *ch)
{
    return ch->blinkview + 1;
}

void Character_SetBlinkView(Character *ch, int view)
{
    if (((view < 2) || (view > Game::Current().numviews)) && (view != -1))
    {
        quit("!SetCharacterBlinkView: invalid view number");
    }

    ch->blinkview = view - 1;
}

int Character_GetBlinkWhileThinking(Character *ch)
{
    if (ch->flags & CHF_NOBLINKANDTHINK)
    {
        return 0;
    }
    return 1;
}

void Character_SetBlinkWhileThinking(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_NOBLINKANDTHINK;
    if (yesorno == 0)
    {
        ch->flags |= CHF_NOBLINKANDTHINK;
    }
}

int Character_GetBlockingHeight(Character *ch)
{
    return ch->blocking_height;
}

void Character_SetBlockingHeight(Character *ch, int hit)
{
    ch->blocking_height = static_cast<int16_t>(hit);
}

int Character_GetBlockingWidth(Character *ch)
{
    return ch->blocking_width;
}

void Character_SetBlockingWidth(Character *ch, int wid)
{
    ch->blocking_width = static_cast<int16_t>(wid);
}

int Character_GetBlockingRectX(Character *ch)
{
    return Game::Current().Characters[ch->index_id].blocking_x;
}

void Character_SetBlockingRectX(Character *ch, int x)
{
    Game::Current().Characters[ch->index_id].blocking_x = x;
}

int Character_GetBlockingRectY(Character *ch)
{
    return Game::Current().Characters[ch->index_id].blocking_y;
}

void Character_SetBlockingRectY(Character *ch, int y)
{
    Game::Current().Characters[ch->index_id].blocking_y = y;
}

int Character_GetDiagonalWalking(Character *ch)
{
    if (ch->flags & CHF_NODIAGONAL)
    {
        return 0;
    }
    return 1;
}

void Character_SetDiagonalWalking(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_NODIAGONAL;
    if (!yesorno)
        ch->flags |= CHF_NODIAGONAL;
}

int Character_GetClickable(Character *ch)
{
    if (ch->flags & CHF_NOINTERACT)
    {
        return 0;
    }
    return 1;
}

void Character_SetClickable(Character *ch, int clickable)
{
    ch->flags &= ~CHF_NOINTERACT;
    // if they don't want it clickable, set the relevant bit
    if (clickable == 0)
    {
        ch->flags |= CHF_NOINTERACT;
    }
}

int Character_GetID(Character *ch)
{
    return ch->index_id;
}

const char *Character_GetScriptName(Character *ch)
{
    return CreateNewScriptString(Game::Current().Characters[ch->index_id].scrname);
}

bool Character_GetEnabled(Character *ch)
{
    return ch->is_enabled();
}

void Character_SetEnabled(Character *ch, bool new_value)
{
    ch->set_enabled(new_value);
}

int Character_GetFrame(Character *ch)
{
    return ch->frame;
}

void Character_SetFrame(Character *ch, int new_value)
{
    ch->frame = new_value;
}

int Character_GetIdleDelay(Character *ch)
{
    return ch->idledelay;
}

int Character_GetIdleTime(Character *ch)
{
    return ch->idleleft;
}

int Character_GetIdleView(Character *ch)
{
    // convert to a 1-based script view index
    if (ch->idleview < 1)
    {
        return -1;
    }
    return ch->idleview + 1;
}

int Character_GetIInventoryQuantity(Character *ch, int index)
{
    if (!AssertInvItem("Character.InventoryQuantity", index))
        return 0;

    return ch->get_item_quantity(index);
}

int Character_HasInventory(Character *ch, ScriptInvItem *item)
{
    if (item == nullptr)
    {
        quit("!Character.HasInventory: NULL inventory item supplied");
    }

    return ch->has_any_of_item(item->id) ? 1 : 0;
}

void Character_SetIInventoryQuantity(Character *ch, int index, int quant)
{
    Game &game = Game::Current();
    if (!AssertInvItem("Character.InventoryQuantity", index))
        return;

    if ((quant < 0) || (quant > INT16_MAX))
    {
        debug_script_warn("Character.InventoryQuantity: char %s, item %s: invalid quantity %d, valid range is 0..%d",
                          ch->scrname.GetCStr(), game.invScriptNames[index].GetCStr(), quant, INT16_MAX);
        quant = Math::Clamp<int>(quant, 0, INT16_MAX);
    }

    const int old_quant = ch->get_item_quantity(index);
    if (quant == old_quant)
    {
        return;
    }

    ch->set_item_quantity(index, quant);
    if (game.options[OPT_DUPLICATEINV] != 0)
    {
        if (quant > old_quant)
        {
            for (int i = old_quant; i < quant; ++i)
                ch->inventory.push_back(index);
        }
        else
        {
            int cur_quant = old_quant;
            for (size_t last_found = 0; (cur_quant > quant) && (last_found != ch->inventory.size()); --cur_quant)
            {
                auto it_found = std::find(ch->inventory.begin() + last_found, ch->inventory.end(), index);
                last_found = it_found - ch->inventory.begin();
                if (it_found != ch->inventory.end())
                    ch->inventory.erase(it_found);
            }
        }
    }
    else
    {
        if (quant > 0 && old_quant == 0)
        {
            ch->inventory.push_back(index);
        }
        else if (quant == 0 && old_quant > 0)
        {
            auto it_found = std::find(ch->inventory.begin(), ch->inventory.end(), index);
            if (it_found != ch->inventory.end())
                ch->inventory.erase(it_found);
        }
    }

    if (ch == game.PlayerChar)
    {
        if (quant > old_quant)
            run_on_event(kScriptEvent_InventoryAdd, index);
        else
            run_on_event(kScriptEvent_InventoryLose, index);
    }

    GUIE::MarkInventoryForUpdate(ch->index_id, ch->index_id == game.playercharacter);
}

// [DEPRECATED]
int Character_GetIgnoreLighting(Character *ch)
{
    if (ch->flags & CHF_NOLIGHTING)
    {
        return 1;
    }
    return 0;
}

// [DEPRECATED]
void Character_SetIgnoreLighting(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_NOLIGHTING;
    if (yesorno)
        ch->flags |= CHF_NOLIGHTING;
}

int Character_GetManualScaling(Character *ch)
{
    return (ch->flags & CHF_MANUALSCALING) ? 1 : 0;
}

void Character_SetManualScaling(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_MANUALSCALING;
    if (yesorno)
        ch->flags |= CHF_MANUALSCALING;
}

int Character_GetInventoryCount(Character *ch)
{
    return ch->inventory.size();
}

ScriptInvItem *Character_GetInventory(Character *ch, int index)
{
    if (index < 0 || static_cast<uint32_t>(index) >= ch->inventory.size())
    {
        debug_script_warn("Character.Inventory[]: char %s: index %d is out of valid range, current inventory is 0..%d",
                          ch->scrname.GetCStr(), index, (int)ch->inventory.size());
        return nullptr;
    }
    return &Game::Current().ScInvItems[ch->inventory[index]];
}

int Character_GetMovementLinkedToAnimation(Character *ch)
{
    if (ch->flags & CHF_ANTIGLIDE)
    {
        return 1;
    }
    return 0;
}

void Character_SetMovementLinkedToAnimation(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_ANTIGLIDE;
    if (yesorno)
        ch->flags |= CHF_ANTIGLIDE;
}

int Character_GetLoop(Character *ch)
{
    return ch->loop;
}

void Character_SetLoop(Character *ch, int new_value)
{
    AssertLoop("Character.Loop", ch->scrname.GetCStr(), ch->view, new_value);

    ch->loop = new_value;
    if (ch->frame >= Game::Current().Views[ch->view].loops[ch->loop].numFrames)
    {
        ch->frame = 0;
    }
}

int Character_GetMoving(Character *ch)
{
    if (ch->walking)
    {
        return 1;
    }
    return 0;
}

int Character_GetDestinationX(Character *ch)
{
    if (ch->get_movelist_id() > 0)
    {
        MoveList *cmls = get_movelist(ch->get_movelist_id());
        return cmls->GetLastPos().X;
    }
    else
    {
        return ch->x;
    }
}

int Character_GetDestinationY(Character *ch)
{
    if (ch->get_movelist_id() > 0)
    {
        MoveList *cmls = get_movelist(ch->get_movelist_id());
        return cmls->GetLastPos().Y;
    }
    else
    {
        return ch->y;
    }
}

const char *Character_GetName(Character *ch)
{
    return CreateNewScriptString(Game::Current().Characters[ch->index_id].name.GetCStr());
}

void Character_SetName(Character *ch, const char *name)
{
    ch->name = name;
    // Fill legacy name fields, for compatibility with old scripts and plugins
    GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
}

int Character_GetNormalView(Character *ch)
{
    return ch->defview + 1;
}

int Character_GetPreviousRoom(Character *ch)
{
    return ch->prevroom;
}

int Character_GetRoom(Character *ch)
{
    return ch->room;
}


int Character_GetScaleMoveSpeed(Character *ch)
{
    if (ch->flags & CHF_SCALEMOVESPEED)
    {
        return 1;
    }
    return 0;
}

void Character_SetScaleMoveSpeed(Character *ch, int yesorno)
{
    if ((yesorno < 0) || (yesorno > 1))
        quit("Character.ScaleMoveSpeed: value must be true or false (1 or 0)");

    ch->flags &= ~CHF_SCALEMOVESPEED;
    if (yesorno)
        ch->flags |= CHF_SCALEMOVESPEED;
}

int Character_GetScaleVolume(Character *ch)
{
    if (ch->flags & CHF_SCALEVOLUME)
    {
        return 1;
    }
    return 0;
}

void Character_SetScaleVolume(Character *ch, int yesorno)
{
    if ((yesorno < 0) || (yesorno > 1))
        quit("Character.ScaleVolume: value must be true or false (1 or 0)");

    ch->flags &= ~CHF_SCALEVOLUME;
    if (yesorno)
        ch->flags |= CHF_SCALEVOLUME;
}

int Character_GetScaling(Character *ch)
{
    return ch->zoom;
}

void Character_SetScaling(Character *ch, int zoomlevel)
{
    if ((ch->flags & CHF_MANUALSCALING) == 0)
    {
        debug_script_warn("Character.Scaling: cannot set property unless ManualScaling is enabled");
        return;
    }
    int zoom_fixed = Math::Clamp(zoomlevel, 1, (int)(INT16_MAX)); // Character::zoom is int16
    if (zoomlevel != zoom_fixed)
        debug_script_warn("Character.Scaling: scaling level must be between 1 and %d%%, asked for: %d",
                          (int)(INT16_MAX), zoomlevel);

    ch->zoom = zoom_fixed;
}

int Character_GetSolid(Character *ch)
{
    if (ch->flags & CHF_NOBLOCKING)
    {
        return 0;
    }
    return 1;
}

void Character_SetSolid(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_NOBLOCKING;
    if (!yesorno)
        ch->flags |= CHF_NOBLOCKING;
}

int Character_GetSpeaking(Character *ch)
{
    if (get_character_currently_talking() == ch->index_id)
    {
        return 1;
    }

    return 0;
}

int Character_GetSpeechColor(Character *ch)
{
    return ch->talkcolor;
}

void Character_SetSpeechColor(Character *ch, int ncol)
{
    ch->talkcolor = ncol;
}

int Character_GetSpeechAnimationDelay(Character *ch)
{
    Game &game = Game::Current();
    if (game.options[OPT_GLOBALTALKANIMSPD] != 0)
        return game.Play.talkanim_speed;
    else
        return ch->speech_anim_speed;
}

void Character_SetSpeechAnimationDelay(Character *ch, int delay)
{
    if (Game::Current().options[OPT_GLOBALTALKANIMSPD] != 0)
    {
        debug_script_warn("Character.SpeechAnimationDelay cannot be set when global speech animation speed is enabled");
        return;
    }

    ch->speech_anim_speed = delay;
}

int Character_GetIdleAnimationDelay(Character *ch)
{
    return ch->idle_anim_speed;
}

void Character_SetIdleAnimationDelay(Character *ch, int delay)
{
    ch->idle_anim_speed = delay;
}

int Character_GetSpeechView(Character *ch)
{
    return ch->talkview + 1;
}

void Character_SetSpeechView(Character *ch, int view)
{
    if (view == -1)
    {
        ch->talkview = -1;
        return;
    }

    if ((view < 1) || (view > Game::Current().numviews))
    {
        quit("!SetCharacterSpeechView: invalid view number");
    }

    ch->talkview = view - 1;
}

bool Character_GetThinking(Character *ch)
{
    return char_thinking == ch->index_id;
}

int Character_GetThinkingFrame(Character *ch)
{
    if (char_thinking == ch->index_id)
    {
        return ch->thinkview > 0 ? ch->frame : -1;
    }

    debug_script_warn("Character.ThinkingFrame: character is not currently thinking");
    return -1;
}

int Character_GetThinkView(Character *ch)
{
    return ch->thinkview + 1;
}

void Character_SetThinkView(Character *ch, int view)
{
    if (((view < 2) || (view > Game::Current().numviews)) && (view != -1))
    {
        quit("!SetCharacterThinkView: invalid view number");
    }

    ch->thinkview = view - 1;
}

int Character_GetOpacity(Character *ch)
{
    return ch->opacity;
}

void Character_SetOpacity(Character *ch, int opacity)
{
    if ((opacity < 0) || (opacity > 255))
        quit("!Character.Opacity: alpha value must be between 0 and 255");

    ch->opacity = opacity;
}

// [OBSOLETE] the AGS 4.0 Transparency, for compiled 4.0 games
int Character_GetTransparency(Character *ch)
{
    return GfxDef::OpacityToTrans100(ch->opacity);
}

void Character_SetTransparency(Character *ch, int trans)
{
    if ((trans < 0) || (trans > 100))
        quit("!SetCharTransparent: transparency value must be between 0 and 100");

    ch->opacity = GfxDef::Trans100ToOpacity(trans);
}

bool Character_GetVisible(Character *ch)
{
    return ch->is_visible();
}

void Character_SetVisible(Character *ch, bool new_value)
{
    ch->set_visible(new_value);
}

int Character_GetBlendMode(Character *ch)
{
    return ch->blend_mode;
}

void Character_SetBlendMode(Character *ch, int blend_mode)
{
    ch->blend_mode = ValidateBlendMode("Character.BlendMode", ch->scrname.GetCStr(), blend_mode);
}

float Character_GetGraphicAnchorX(Character *ch)
{
    return ch->spr_anchor.X;
}

void Character_SetGraphicAnchorX(Character *ch, float x)
{
    ch->spr_anchor.X = Math::Clamp(x, 0.f, 1.f);
    ch->UpdateEffectiveValues();
}

float Character_GetGraphicAnchorY(Character *ch)
{
    return ch->spr_anchor.Y;
}

void Character_SetGraphicAnchorY(Character *ch, float y)
{
    ch->spr_anchor.Y = Math::Clamp(y, 0.f, 1.f);
    ch->UpdateEffectiveValues();
}

int Character_GetGraphicOffsetX(Character *ch)
{
    return ch->spr_offset.X;
}

void Character_SetGraphicOffsetX(Character *ch, int x)
{
    ch->spr_offset.X = x;
    ch->UpdateEffectiveValues();
}

int Character_GetGraphicOffsetY(Character *ch)
{
    return ch->spr_offset.Y;
}

void Character_SetGraphicOffsetY(Character *ch, int y)
{
    ch->spr_offset.Y = y;
    ch->UpdateEffectiveValues();
}

ScriptShaderInstance *Character_GetShader(Character *ch)
{
    return static_cast<ScriptShaderInstance *>(ccGetObjectAddressFromHandle(ch->shader_handle));
}

void Character_SetShader(Character *ch, ScriptShaderInstance *shader_inst)
{
    ch->shader_id = shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID;
    ch->shader_handle = ccReplaceObjectHandle(ch->shader_handle, shader_inst);
}

float Character_GetRotation(Character *ch)
{
    return ch->rotation;
}

void Character_SetRotation(Character *ch, float degrees)
{
    ch->rotation = Math::ClampAngle360(degrees);
    ch->UpdateGraphicSpace();
}

float Character_GetGraphicPivotX(Character *ch)
{
    return ch->pivot.X;
}

void Character_SetGraphicPivotX(Character *ch, float pivotx)
{
    ch->pivot.X = pivotx;
}

float Character_GetGraphicPivotY(Character *ch)
{
    return ch->pivot.Y;
}

void Character_SetGraphicPivotY(Character *ch, float pivoty)
{
    ch->pivot.Y = pivoty;
}

int Character_GetGraphicPivotOffsetX(Character *ch)
{
    return ch->pivot_offset.X;
}

void Character_SetGraphicPivotOffsetX(Character *ch, int px)
{
    ch->pivot_offset.X = px;
}

int Character_GetGraphicPivotOffsetY(Character *ch)
{
    return ch->pivot_offset.Y;
}

void Character_SetGraphicPivotOffsetY(Character *ch, int py)
{
    ch->pivot_offset.Y = py;
}

float Character_GetFaceDirectionRatio(Character *ch)
{
    return ch->face_dir_ratio;
}

void Character_SetFaceDirectionRatio(Character *ch, float ratio)
{
    ch->face_dir_ratio = ratio;
}

int Character_GetTurnBeforeWalking(Character *ch)
{
    // NOTE: this flag has inverse meaning
    return ((ch->flags & CHF_NOTURNWHENWALK) != 0) ? 0 : 1;
}

void Character_SetTurnBeforeWalking(Character *ch, int on)
{
    // NOTE: this flag has inverse meaning
    if (on)
        ch->flags &= ~CHF_NOTURNWHENWALK;
    else
        ch->flags |= CHF_NOTURNWHENWALK;
}

bool Character_GetTurning(Character *ch)
{
    return ch->IsTurning();
}

int Character_GetTurnWhenFacing(Character *ch)
{
    return ((ch->flags & CHF_TURNWHENFACE) != 0) ? 1 : 0;
}

void Character_SetTurnWhenFacing(Character *ch, int on)
{
    if (on)
        ch->flags |= CHF_TURNWHENFACE;
    else
        ch->flags &= ~CHF_TURNWHENFACE;
}

int Character_GetView(Character *ch)
{
    return ch->view + 1;
}

int Character_GetWalkSpeedX(Character *ch)
{
    return ch->walkspeed;
}

int Character_GetWalkSpeedY(Character *ch)
{
    if (ch->walkspeed_y != UNIFORM_WALK_SPEED)
    {
        return ch->walkspeed_y;
    }

    return ch->walkspeed;
}

int Character_GetX(Character *ch)
{
    return ch->x;
}

void Character_SetX(Character *ch, int new_value)
{
    ch->x = new_value;
    ch->UpdateGraphicSpace();
}

int Character_GetY(Character *ch)
{
    return ch->y;
}

void Character_SetY(Character *ch, int new_value)
{
    ch->y = new_value;
    ch->UpdateGraphicSpace();
}

int Character_GetZ(Character *ch)
{
    return ch->z;
}

void Character_SetZ(Character *ch, int new_value)
{
    ch->z = new_value;
    ch->UpdateGraphicSpace();
}

int Character_GetSpeakingFrame(Character *ch)
{
    Game &game = Game::Current();
    if ((game.Speech.FaceTalking >= 0) && (game.Speech.FaceTalkRepeat))
    {
        if (game.Speech.FaceTalkChar->index_id == ch->index_id)
        {
            return game.Speech.FaceTalkFrame;
        }
    }
    else if (game.Speech.CharSpeakingAnim >= 0)
    {
        if (game.Speech.CharSpeakingAnim == ch->index_id)
        {
            return ch->frame;
        }
    }

    debug_script_warn("Character.SpeakingFrame: character is not playing a speech animation");
    return -1;
}

bool Character_GetUseRegionTint(Character *ch)
{
    return (ch->flags & CHF_NOLIGHTING) == 0;
}

void Character_SetUseRegionTint(Character *ch, int yesorno)
{
    ch->flags &= ~CHF_NOLIGHTING;
    if (!yesorno)
        ch->flags |= CHF_NOLIGHTING;
}

float Character_GetViewAnchorX(Character *ch)
{
    return ch->GetEffectiveGraphicAnchor().X;
}

float Character_GetViewAnchorY(Character *ch)
{
    return ch->GetEffectiveGraphicAnchor().Y;
}

int Character_GetViewOffsetX(Character *ch)
{
    return ch->GetEffectiveGraphicOffset().X;
}

int Character_GetViewOffsetY(Character *ch)
{
    return ch->GetEffectiveGraphicOffset().Y;
}

ScriptMotionPath *Character_GetMotionPath(Character *ch)
{
    const int mslot = ch->get_movelist_id();
    if (mslot <= 0)
        return nullptr;

    if (ch->movelist_handle > 0)
    {
        return static_cast<ScriptMotionPath *>(ccGetObjectAddressFromHandle(ch->movelist_handle));
    }

    auto sc_path = ScriptMotionPath::Create(mslot);
    ccAddObjectReference(sc_path.Handle());
    ch->movelist_handle = sc_path.Handle();
    return static_cast<ScriptMotionPath *>(sc_path.Obj());
}

//=============================================================================

// Order of loops to turn character, from down to down-right clockwise
const int turnlooporder[MAX_FACE_DIRECTIONS] = {kDirLoop_Down, kDirLoop_DownLeft, kDirLoop_Left,  kDirLoop_UpLeft,
                                                kDirLoop_Up,   kDirLoop_UpRight,  kDirLoop_Right, kDirLoop_DownRight};

// Core character move implementation:
// uses a provided path or searches for a path to a given destination;
// starts a move or walk (with automatic animation).
void move_character_impl(Character *ch, const std::vector<Point> *path, int tox, int toy, bool ignwal, bool walk_anim,
                         const RunPathParams &run_params)
{
    assert(run_params.Flow >= kAnimFlow_First && run_params.Flow <= kAnimFlow_Last);
    assert(run_params.Direction >= kAnimDirForward && run_params.Direction <= kAnimDirBackward);
    const int char_id = ch->index_id;
    if (!ValidateCharForMove(ch, "MoveCharacter"))
    {
        return;
    }

    // Stop custom animation always (but idling will be stopped only if pathfinding was a success, below)
    if (ch->IsAnimating() && walk_anim)
        stop_character_anim(ch);

    // If using a path, then update starting and destination parameters,
    // and jump the character to the path's start
    if (path)
    {
        ch->x = run_params.IsForward() ? path->front().X : path->back().X;
        ch->y = run_params.IsForward() ? path->front().Y : path->back().Y;
        tox = run_params.IsForward() ? path->back().X : path->front().X;
        toy = run_params.IsForward() ? path->back().Y : path->front().Y;
    }

    // If has path, then test if it's empty, or has 2 stages where start is identical to end;
    // If no path, then test if destination is identical to the current pos
    if (path && ((path->size() < 2) || (path->size() == 2) && ((*path)[0] == (*path)[1])) ||
        !path && (tox == ch->x) && (toy == ch->y))
    {
        Character_StopMoving(ch);
        debug_script_log("MoveCharacter: %s move path is empty, or is already at destination, not moving",
                         ch->scrname.GetCStr());
        return;
    }

    // By default wait 1 frame before starting to move, to let animation advance once
    int walk_wait_init = 1;
    int anim_wait_init = 0;
    float step_frac_init = 0.f;
    int old_frame = ch->frame;
    // If character was currently walking, then save current "wait" timers
    // and animation frame, and apply later after new walking begins,
    // in order to make it look smoother.
    if (ch->walking)
    {
        walk_wait_init = ch->walkwait;
        anim_wait_init = ch->animwait;
        const auto &movelist = *get_movelist(ch->get_movelist_id());
        if (movelist.GetStageProgress() > 0.f)
            step_frac_init = movelist.GetPixelUnitFraction();
    }

    Character_StopMoving(ch);
    ch->frame = old_frame;
    debug_script_log("MoveCharacter: request move %s: %d,%d to %d,%d", ch->scrname.GetCStr(), ch->x, ch->y, tox, toy);

    int move_speed_x, move_speed_y;
    ch->get_effective_walkspeeds(move_speed_x, move_speed_y);
    if ((move_speed_x == 0) && (move_speed_y == 0))
    {
        debug_script_warn("MoveCharacter: called for '%s' with walk speed 0", ch->scrname.GetCStr());
    }

    MoveList new_mlist;
    bool path_result = false;
    if (path)
    {
        path_result = MoveListOp::CalculateMoveList(new_mlist, *path, move_speed_x, move_speed_y,
                                                    ignwal ? kMoveStage_Direct : 0, run_params);
    }
    else
    {
        MaskRouteFinder *pathfind = get_room_pathfinder();
        pathfind->SetWalkableArea(prepare_walkable_areas(char_id), Room::Current().MaskResolution);
        path_result = Pathfinding::FindRoute(new_mlist, pathfind, ch->x, ch->y, tox, toy, move_speed_x, move_speed_y,
                                             false, ignwal, run_params);
    }

    // Double check that the path actually leads to another position;
    // sometimes pathfinder glitches and returns end point identical to the start point,
    // and we do not want to "twitch" character's state in such case.
    if (path_result)
        path_result &= (new_mlist.GetFirstPos() != new_mlist.GetLastPos());

    // If path available, then add movelist
    const int movelist = path_result ? add_movelist(std::move(new_mlist)) : 0;
    assert(!path_result || movelist > 0);

    // If successful, then start moving
    if (movelist > 0)
    {
        // Stop idling state (if character was in one)
        stop_character_idling(ch);

        // Setup new walk state
        ch->walking = movelist;
        MoveList &mlist = *get_movelist(ch->walking);

        if (step_frac_init > 0.f)
        {
            mlist.SetPixelUnitFraction(step_frac_init);
        }

        // Cancel any pending waits on current animations
        // or if they were already moving, keep the current wait -
        // this prevents a glitch if MoveCharacter is called when they
        // are already moving
        if (walk_anim)
        {
            ch->walkwait = walk_wait_init;
            ch->animwait = anim_wait_init;
            fix_player_sprite(ch, mlist);
        }
        else
        {
            ch->flags |= CHF_MOVENOTWALK;
        }
    }
    else
    {
        // Pathfinder couldn't get a route, stand them still
        if (walk_anim && !ch->is_idling())
        {
            ch->frame = 0;
        }
    }
}

int find_looporder_index(int curloop)
{
    for (int rr = 0; rr < MAX_FACE_DIRECTIONS; ++rr)
    {
        if (turnlooporder[rr] == curloop)
            return rr;
    }
    return 0;
}

bool should_use_diagloops(Character *ch, bool require_animation)
{
    Game &game = Game::Current();
    if ((game.Views[ch->view].numLoops < 8) || ((ch->flags & CHF_NODIAGONAL) != 0))
    {
        return false;
    }
    // Optionally test if they have just provided standing frames for loops 4-7, to
    // provide smoother turning
    if (require_animation && (game.Views[ch->view].loops[4].numFrames < 2))
    {
        return false;
    }
    return true;
}

// Tells if the view has frames in both the Down and the Up loops; false if there are only horizontal animations
bool has_updown_loops(Character *ch)
{
    Game &game = Game::Current();
    // if no loops in the Down animation
    // or no loops in the Up animation
    if ((game.Views[ch->view].loops[0].numFrames < 1) || (game.Views[ch->view].numLoops < 4) ||
        (game.Views[ch->view].loops[3].numFrames < 1))
    {
        return false;
    }
    return true;
}

// Tells if the given directional loop belongs to the left half of the circle;
// note this returns positive for vertical loops too, for the sake of simplicity
static bool is_loop_in_left_half(int loop)
{
    switch (loop)
    {
    case kDirLoop_Down:
    case kDirLoop_DownLeft:
    case kDirLoop_Left:
    case kDirLoop_UpLeft:
    case kDirLoop_Up: return true;
    default: return false;
    }
}

void start_character_turning(Character *ch, int use_loop, bool use_diagloops)
{
    Game &game = Game::Current();
    // work out how far round they have to turn
    int from_idx = find_looporder_index(ch->loop);
    int to_idx = find_looporder_index(use_loop);
    // Default turn order is clockwise. But in certain cases we might want to change this.
    int clock_order = 1;
    // If anti-clockwise is a shorter turn, then we choose anti-clockwise direction.
    if ((to_idx > from_idx) && ((to_idx - from_idx) > MAX_FACE_DIRECTIONS / 2) ||
        (to_idx < from_idx) && ((from_idx - to_idx) < MAX_FACE_DIRECTIONS / 2))
    {
        clock_order = -1;
    }
    // If both directions are equally short, then use the global priority setting.
    else if (std::abs(to_idx - from_idx) == MAX_FACE_DIRECTIONS / 2)
    {
        switch (game.options[OPT_TURNORDERPRIORITY])
        {
        case kScTurnOrder_Clockwise: clock_order = 1; break;
        case kScTurnOrder_CounterClockwise: clock_order = -1; break;
        case kScTurnOrder_Random: clock_order = (rand() % 2) == 0 ? 1 : -1; break;
        // turnlooporder runs Down, DownLeft, Left, UpLeft, Up, ... : from a left-half loop the way through Down is -1
        case kScTurnOrder_FaceDown: clock_order = is_loop_in_left_half(turnlooporder[from_idx]) ? -1 : 1; break;
        default: break;
        }
    }

    int turns = 0;
    for (int ii = from_idx; ii != to_idx; ii += clock_order)
    {
        // Wrap the loop order into range [0..MAX_FACE_DIRECTIONS - 1]
        if (ii < 0)
            ii = MAX_FACE_DIRECTIONS - 1;
        if (ii >= MAX_FACE_DIRECTIONS)
            ii = 0;
        if (ii == to_idx)
        {
            break;
        }
        if ((turnlooporder[ii] >= NUM_BASE_DIRECTIONS) && (!use_diagloops))
            continue; // there are no diagonal loops
        if (turnlooporder[ii] >= game.Views[ch->view].numLoops)
        {
            continue; // no such loop
        }
        if (game.Views[ch->view].loops[turnlooporder[ii]].numFrames < 1)
        {
            continue; // no frames in such loop
        }
        turns++;
    }
    if (turns > 0)
    {
        ch->SetTurning(true, clock_order < 0, turns);
    }
}

void fix_player_sprite(Character *ch, const MoveList &cmls)
{
    Game &game = Game::Current();
    const float xpmove = cmls.GetCurrentSpeed().X;
    const float ypmove = cmls.GetCurrentSpeed().Y;

    // if not moving, do nothing
    if ((xpmove == 0.f) && (ypmove == 0.f))
        return;

    const int use_loop = GetDirectionalLoop(ch, xpmove, ypmove, cmls.GetRunParams().IsForward());

    if ((game.options[OPT_CHARTURNWHENWALK] == 0) || ((ch->flags & CHF_NOTURNWHENWALK) != 0))
    {
        ch->loop = use_loop;
        return;
    }
    if ((ch->loop > kDirLoop_LastOrthogonal) && ((ch->flags & CHF_NODIAGONAL) != 0))
    {
        // They've just been playing an animation with an extended loop number,
        // so don't try and rotate using it
        ch->loop = use_loop;
        return;
    }
    if ((ch->loop >= game.Views[ch->view].numLoops) || (game.Views[ch->view].loops[ch->loop].numFrames < 1) ||
        (!has_updown_loops(ch)))
    {
        // Character is not currently on a valid loop, so don't try to rotate
        // eg. left/right only view, but current loop 0
        ch->loop = use_loop;
        return;
    }
    start_character_turning(ch, use_loop, should_use_diagloops(ch, false /* allow 1 frame in loops */));
}

// Check whether two characters have walked into each other
int has_hit_another_character(Game &game, int source_char)
{
    // if the character who's moving doesn't block, don't bother checking
    if (game.Characters[source_char].flags & CHF_NOBLOCKING)
    {
        return -1;
    }

    for (int ww = 0; ww < game.numcharacters; ww++)
    {
        if (!game.Characters[ww].is_enabled())
            continue;
        if (game.Characters[ww].room != game.DisplayedRoom)
            continue;
        if (ww == source_char)
        {
            continue;
        }
        if (game.Characters[ww].flags & CHF_NOBLOCKING)
            continue;

        if (get_char_blocking_rect(game, ww).IsInside(game.Characters[source_char].x, game.Characters[source_char].y))
        {
            // we are now overlapping character 'ww'
            if ((game.Characters[ww].walking) && ((game.Characters[ww].flags & CHF_AWAITINGMOVE) == 0))
                return ww;
        }
    }
    return -1;
}

// Does the next move from the character's movelist.
// Returns 1 if they are now waiting for another char to move,
// otherwise returns 0
int doNextCharMoveStep(Game &game, Character *ch)
{
    assert(ch->walking > 0);
    if (ch->walking <= 0)
    {
        return 0;
    }

    const int xwas = ch->x, ywas = ch->y;
    MoveList &mlist = *get_movelist(ch->get_movelist_id());
    // For the purpose smooth walk transition keep moving even across multiple stages,
    // until all the "current move" is not depleted OR until we have to turn.
    for (MoveResult mresult = kMoveResult_Continue;
         (mresult = do_movelist_move(ch->walking, ch->x, ch->y, mresult == kMoveResult_Continue)) ==
         kMoveResult_NextStage;)
    {
        // Character just switched to the next path segment, test if we need to turn.
        if (ch->is_moving_walkanim())
        {
            fix_player_sprite(ch, mlist);
        }
        // If there's no move remainder, or if character started to turn,
        // then force reset stage progress to zero (eliminate any unused remainder)
        // and break out.
        if (Math::IsFloatZero(mlist.onpart) || ch->IsTurning())
        {
            // cancel any walk progress transition since the previous stage
            mlist.onpart = 0.f;
            break;
        }
    }

    int ntf = has_hit_another_character(game, ch->index_id);
    if (ntf >= 0)
    {
        ch->walkwait = 30;
        if (game.Characters[ntf].walkspeed < 5)
            ch->walkwait += (5 - game.Characters[ntf].walkspeed) * 5;
        // we are now waiting for the other char to move, so
        // make sure he doesn't stop for us too

        ch->flags |= CHF_AWAITINGMOVE;

        if (ch->is_moving_walkanim())
        {
            ch->frame = 0;
            ch->animwait = ch->walkwait;
        }

        if ((ch->walking < 1) || (ch->turns > 0))
        { /* skip */
        }
        else if (mlist.GetStageDoneSteps() > 0.f)
        {
            mlist.Backward();
            ch->x = xwas;
            ch->y = ywas;
        }

        debug_script_log("%s: Bumped into %s, waiting for them to move", ch->scrname.GetCStr(),
                         game.Characters[ntf].scrname.GetCStr());
        return 1;
    }
    return 0;
}

bool is_char_walking_ndirect(Character *ch)
{
    return ch->is_moving() && (!get_movelist(ch->get_movelist_id())->IsStageDirect());
}

bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst)
{
    Room &room = Room::Current();
    const Point at_pt = Point(room_to_mask_coord(room, src.X), room_to_mask_coord(room, src.Y));
    const Bitmap *mask = room.WalkAreaMask.get();

    int pix_value = 0;
    if (mask->IsOnBitmap(at_pt.X, at_pt.Y))
    {
        pix_value = room.WalkAreaMask->GetPixel(at_pt.X, at_pt.Y);
    }

    // Is already on a walkable area?
    if (pix_value > 0)
    {
        dst = src;
        return true;
    }

    Rect limits = RectWH(mask->GetSize());
    // Adjust scan area to match historical behavior of Character.PlaceOnWalkableArea()
    // Characters that are within Room Edges cannot be moved outside of them
    int right_edge = room_to_mask_coord(room, room.Edges.Right);
    int left_edge = room_to_mask_coord(room, room.Edges.Left);
    int top_edge = room_to_mask_coord(room, room.Edges.Top);
    int bottom_edge = room_to_mask_coord(room, room.Edges.Bottom);

    // Original comment says that edges are ignored if the character is outside,
    // because "people forget to move the edges sometimes".
    if (at_pt.X <= left_edge)
        left_edge = 0;
    if (at_pt.X >= right_edge)
        right_edge = limits.Right;
    if (at_pt.Y <= top_edge)
        top_edge = 0;
    if (at_pt.Y >= bottom_edge)
        bottom_edge = limits.Bottom;

    if (!Pathfinding::FindNearestWalkablePoint(room.WalkAreaMask.get(), at_pt, dst, limits, 0, 1))
    {
        return false;
    }

    dst = Point(mask_to_room_coord(room, dst.X), mask_to_room_coord(room, dst.Y));
    return true;
}

void FindReasonableLoopForCharacter(Character *ch, bool is_walk_view)
{
    Game &game = Game::Current();
    int loop_limit = game.Views[ch->view].numLoops;
    // If it's a standard purpose walking view, then limit the loop to
    // walking directions (depending on "use diagonal loops" character option)
    if (is_walk_view)
    {
        loop_limit = ((ch->flags & CHF_NODIAGONAL) != 0) ? std::min(loop_limit, 4) : std::min(loop_limit, 8);
    }

    if (ch->loop >= loop_limit)
    {
        ch->loop = kDirLoop_Default;
    }
    if (game.Views[ch->view].numLoops < 1)
    {
        quitprintf("!View %d does not have any loops", ch->view + 1);
    }

    // if the current loop has no frames, find one that does
    if (game.Views[ch->view].loops[ch->loop].numFrames < 1)
    {
        for (int i = 0; i < loop_limit; i++)
        {
            if (game.Views[ch->view].loops[i].numFrames > 0)
            {
                ch->loop = i;
                break;
            }
        }
    }
}

void move_character(Character *ch, int tox, int toy, bool ignwal, bool walk_anim)
{
    move_character_impl(ch, nullptr, tox, toy, ignwal, walk_anim, RunPathParams::DefaultOnce());
}

void move_character(Character *ch, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params)
{
    move_character_impl(ch, &path, 0, 0, true /* ignore walls */, walk_anim, run_params);
}

void move_character_straight(Character *ch, int x, int y, bool walk_anim)
{
    MaskRouteFinder *pathfind = get_room_pathfinder();
    pathfind->SetWalkableArea(prepare_walkable_areas(ch->index_id), Room::Current().MaskResolution);
    if (!pathfind->IsWalkableAt(ch->x, ch->y))
    {
        Character_StopMoving(ch);
        debug_script_log("MoveCharacterStraight: %s (%d,%d) is not on a walkable area, not moving",
                         ch->scrname.GetCStr(), ch->x, ch->y);
        return;
    }

    int movetox = x, movetoy = y;
    int lastcx = ch->x, lastcy = ch->y;
    if (!pathfind->CanSeeFrom(ch->x, ch->y, x, y, &lastcx, &lastcy))
    {
        movetox = lastcx;
        movetoy = lastcy;
    }

    std::vector<Point> path = {{ch->x, ch->y}, {movetox, movetoy}};
    move_character_impl(ch, &path, movetox, movetoy, false /* walkable areas */, walk_anim,
                        RunPathParams::DefaultOnce());
}

void walk_character(Character *ch, int tox, int toy, bool ignwal)
{
    move_character(ch, tox, toy, ignwal, true /* animate */);
}

void MoveCharacterToHotspot(int char_id, int hotsp)
{
    Game &game = Game::Current();
    Room &room = Room::Current();
    if ((hotsp < 0) || (hotsp >= MAX_ROOM_HOTSPOTS))
        quit("!MovecharacterToHotspot: invalid hotspot");
    if (room.Hotspots[hotsp].WalkTo.X < 1)
        return;

    walk_character(&game.Characters[char_id], room.Hotspots[hotsp].WalkTo.X, room.Hotspots[hotsp].WalkTo.Y,
                   false /* walkable areas */);
    GameLoopUntilNotMoving(&game.Characters[char_id].walking);
}

int wantMoveNow(Character *ch)
{
    // check most likely case first
    if ((ch->zoom == 100) || ((ch->flags & CHF_SCALEMOVESPEED) == 0))
    {
        return 1;
    }

    // the % checks don't work when the counter is negative, so once
    // it wraps round, correct it
    // FIXME: walkwaitcounter is int16_t and turns negative after 32767 steps, which this loop works around
    // (12000 is a multiple of 4, so the phase of the % 4 checks is kept); an unsigned counter would not need it.
    while (ch->walkwaitcounter < 0)
    {
        ch->walkwaitcounter += 12000;
    }

    // scaling 170-200%, move 175% speed
    if (ch->zoom >= 170)
    {
        if ((ch->walkwaitcounter % 4) >= 1)
        {
            return 2;
        }
        else
            return 1;
    }
    // scaling 140-170%, move 150% speed
    else if (ch->zoom >= 140)
    {
        if ((ch->walkwaitcounter % 2) == 1)
        {
            return 2;
        }
        else
            return 1;
    }
    // scaling 115-140%, move 125% speed
    else if (ch->zoom >= 115)
    {
        if ((ch->walkwaitcounter % 4) >= 3)
        {
            return 2;
        }
        else
            return 1;
    }
    // scaling 80-120%, normal speed
    else if (ch->zoom >= 80)
        return 1;
    // scaling 60-80%, move 75% speed
    if (ch->zoom >= 60)
    {
        if ((ch->walkwaitcounter % 4) >= 1)
        {
            return -1;
        }
        else if (ch->xwas != INVALID_X)
        {
            // move the second half of the movement to make it smoother
            ch->x = ch->xwas;
            ch->y = ch->ywas;
            ch->xwas = INVALID_X;
        }
    }
    // scaling 30-60%, move 50% speed
    else if (ch->zoom >= 30)
    {
        if ((ch->walkwaitcounter % 2) == 1)
        {
            return -1;
        }
        else if (ch->xwas != INVALID_X)
        {
            // move the second half of the movement to make it smoother
            ch->x = ch->xwas;
            ch->y = ch->ywas;
            ch->xwas = INVALID_X;
        }
    }
    // scaling 0-30%, move 25% speed
    else
    {
        if ((ch->walkwaitcounter % 4) >= 3)
        {
            return -1;
        }
        if (((ch->walkwaitcounter % 4) == 1) && (ch->xwas != INVALID_X))
        {
            // move the second half of the movement to make it smoother
            ch->x = ch->xwas;
            ch->y = ch->ywas;
            ch->xwas = INVALID_X;
        }
    }

    return 0;
}

void setup_player_character(int char_id)
{
    Game &game = Game::Current();
    game.playercharacter = char_id;
    game.PlayerChar = &game.Characters[char_id];
    game.ScPlayerCharPtr = ccGetObjectHandleFromAddress(game.PlayerChar);
}

// Animate character internal implementation;
// this function may be called by the game logic too, so we assume
// the arguments must be correct, and do not fix them up as we do for API functions.
void animate_character(Character *ch, int loop, int sppd, int rept, int direction, int sframe, int volume)
{
    Game &game = Game::Current();
    if ((ch->view < 0) || (ch->view > game.numviews) || (loop < 0) || (loop >= game.Views[ch->view].numLoops))
    {
        quitprintf("!AnimateCharacter: invalid view and/or loop\n"
                   "(trying to animate '%s' using view %d (range is 1..%d) and loop %d (view has %d loops)).",
                   ch->scrname.GetCStr(), ch->view + 1, game.numviews, loop, game.Views[ch->view].numLoops);
    }
    // NOTE: there's always frame 0 allocated for safety
    sframe = std::max(0, std::min(sframe, game.Views[ch->view].loops[loop].GetLastFrame()));
    debug_script_log("%s: Start anim view %d loop %d, spd %d, repeat %d, frame: %d", ch->scrname.GetCStr(),
                     ch->view + 1, loop, sppd, rept, sframe);

    Character_StopMoving(ch);

    ch->SetAnimating(static_cast<AnimFlowStyle>(rept), static_cast<AnimFlowDirection>(direction), sppd,
                     Math::Clamp(volume, 0, 100));
    ch->loop = loop;
    ch->frame = SetFirstAnimFrame(ch->view, loop, sframe, static_cast<AnimFlowDirection>(direction));
    ch->wait = sppd + game.Views[ch->view].loops[loop].frames[ch->frame].speed;

    ch->CheckViewFrame();
}

void stop_character_anim(Character *ch)
{
    ch->ResetAnimating();
}

Bitmap *GetCharacterImage(int char_id, bool always_original, bool *is_original)
{
    Bitmap *pic = nullptr;
    if (!always_original)
    {
        // NOTE: the cached image will only be present in software render mode
        pic = get_cached_character_image(char_id);
    }

    if (is_original)
        *is_original = pic == nullptr; // if not used cached image, then use original
    if (!pic)
        pic = GetCharacterSourceImage(Game::Current(), EngineServices::Current(), char_id);
    return pic;
}

Bitmap *GetCharacterSourceImage(Game &game, EngineServices &services, int char_id)
{
    const Character &ch = game.Characters[char_id];
    int sppic = game.Views[ch.view].loops[ch.loop].frames[ch.frame].pic;
    return services.Sprites[sppic];
}

Character *Character_GetAtScreenXY(int x, int y, int hit_options)
{
    Game &game = Game::Current();
    VpPoint vpt = game.Viewports.ScreenToRoom(x, y);
    if (vpt.second < 0)
        return nullptr;
    int chnum = GetCharIDAtRoom(vpt.first.X, vpt.first.Y, hit_options);
    if (chnum < 0)
        return nullptr;
    return &game.Characters[chnum];
}

Character *Character_GetAtScreenXY2(int x, int y)
{
    return Character_GetAtScreenXY(x, y, true);
}

Character *Character_GetAtRoomXY(int x, int y, int hit_options)
{
    int char_id = GetCharIDAtRoom(x, y, hit_options);
    if (char_id < 0)
    {
        return nullptr;
    }
    return &Game::Current().Characters[char_id];
}

Character *Character_GetAtRoomXY2(int x, int y)
{
    return Character_GetAtRoomXY(x, y, true);
}

void update_character_scale(Game &game, Room &room, int char_id)
{
    // Test for valid view and loop
    Character &ch = game.Characters[char_id];
    if (!ch.is_enabled() || ch.room != game.DisplayedRoom)
    {
        return; // not enabled, or in a different room
    }

    if (ch.view < 0)
    {
        quitprintf(
            "!The character '%s' was turned on in the current room (room %d) but has not been assigned a view number.",
            ch.scrname.GetCStr(), game.DisplayedRoom);
    }
    if (ch.loop >= game.Views[ch.view].numLoops)
    {
        // a stale loop can survive a view change, re-select a valid loop
        debug_script_warn("The character '%s' was to be displayed with loop %d of view %d, which does not exist; "
                          "resetting to a valid loop.",
                          ch.scrname.GetCStr(), ch.loop, ch.view + 1);
        FindReasonableLoopForCharacter(&ch, ch.view == ch.defview);
    }
    // If frame is too high -- fallback to the frame 0;
    // there's always at least 1 dummy frame at index 0
    if (ch.frame >= game.Views[ch.view].loops[ch.loop].numFrames)
    {
        ch.frame = 0;
    }

    const ViewFrame &vf = game.Views[ch.view].loops[ch.loop].frames[ch.frame];
    const int pic = vf.pic;
    int zoom, zoom_offs, scale_width, scale_height;
    update_object_scale(game, room, zoom, scale_width, scale_height, ch.x, ch.y, pic, ch.zoom,
                        (ch.flags & CHF_MANUALSCALING) == 0);
    zoom_offs = (game.options[OPT_SCALECHAROFFSETS] != 0) ? zoom : 100;

    // Save calculated properties and recalc GS
    ch.zoom = zoom;
    ch.spr_width = game.SpriteInfos[pic].Width;
    ch.spr_height = game.SpriteInfos[pic].Height;
    ch.frame_xoff = vf.xoffs;
    ch.frame_yoff = vf.yoffs;
    ch.width = scale_width;
    ch.height = scale_height;
    ch.zoom_offs = zoom_offs;
    ch.UpdateGraphicSpace();
}

int GetCharIDAtRoom(int x, int y, int hit_options)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    const bool only_clickable = (hit_options & kHit_Interactable) != 0;

    int cc, sppic, lowestyp = 0, lowestwas = -1;
    for (cc = 0; cc < game.numcharacters; cc++)
    {
        if (game.Characters[cc].room != game.DisplayedRoom)
            continue;
        if (!game.Characters[cc].is_displayed())
            continue; // disabled or not visible
        if (only_clickable && (game.Characters[cc].flags & CHF_NOINTERACT))
            continue;
        if (game.Characters[cc].view < 0)
            continue;
        Character *ch = &game.Characters[cc];

        if ((ch->view < 0) || (ch->loop >= game.Views[ch->view].numLoops) ||
            (ch->frame >= game.Views[ch->view].loops[ch->loop].numFrames))
        {
            continue;
        }

        sppic = game.Views[ch->view].loops[ch->loop].frames[ch->frame].pic;
        int use_width = game.SpriteInfos[sppic].Width;
        int use_height = game.SpriteInfos[sppic].Height;
        // TODO: support mirrored transformation in GraphicSpace
        SpriteTransformFlags sprite_flags = game.Views[ch->view].loops[ch->loop].frames[ch->frame].flags;
        Bitmap *image = GetCharacterSourceImage(game, services, cc);
        // Convert to local object coordinates
        Point local = ch->GetGraphicSpace().WorldToLocal(x, y);
        if (is_pos_in_sprite(game, local.X, local.Y, 0, 0, image, use_width, use_height, sprite_flags) == FALSE)
        {
            continue;
        }

        int use_base = ch->get_baseline();
        if (use_base < lowestyp)
            continue;
        lowestyp = use_base;
        lowestwas = cc;
    }
    game.CharLowestYp = lowestyp;
    return lowestwas;
}

Rect get_char_blocking_rect(Game &game, int char_id)
{
    Character *ch = &game.Characters[char_id];

    int width;
    if (ch->blocking_width < 1)
    {
        width = GetCharacterWidth(game, char_id) - 4;
    }
    else
        width = ch->blocking_width;

    int x = ch->x - width / 2 + game.Characters[ch->index_id].blocking_x;
    int y1 = ch->get_blocking_top() + game.Characters[ch->index_id].blocking_y;
    int y2 = ch->get_blocking_bottom() + game.Characters[ch->index_id].blocking_y;

    return Rect(x, y1, x + width - 1, y2);
}

int check_click_on_character(int x, int y, int mood)
{
    int lowestwas = GetCharIDAtRoom(x, y, kHit_Interactable);
    if (lowestwas >= 0)
    {
        Character_RunInteraction(&Game::Current().Characters[lowestwas], mood);
        return 1;
    }
    return 0;
}

void DisplaySpeechCore(int char_id, const char *displbuf)
{
    Game &game = Game::Current();
    if (displbuf[0] == 0)
    {
        // no text, just update the current character who's speaking
        // this allows the portrait side to be switched with an empty
        // speech line
        game.Play.swap_portrait_lastchar = char_id;
        return;
    }

    // adjust timing of text (so that DisplaySpeech("%s", str) pauses
    // for the length of the string not 2 frames)
    int len = (int)strlen(displbuf);
    if (len > game.TextLayout.SourceTextLength + 3)
        game.TextLayout.SourceTextLength = len;

    DisplaySpeech(displbuf, char_id);
}

void DisplayThoughtCore(int char_id, const char *displbuf)
{
    Game &game = Game::Current();
    // adjust timing of text (so that DisplayThought("%s", str) pauses
    // for the length of the string not 2 frames)
    int len = (int)strlen(displbuf);
    if (len > game.TextLayout.SourceTextLength + 3)
        game.TextLayout.SourceTextLength = len;

    int width = -1;
    if ((game.options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts) || (game.Characters[char_id].thinkview <= 0))
    {
        // lucasarts-style, so we want a speech bubble actually above
        // their head (or if they have no think anim in Sierra-style)
        width = game.Play.speech_bubble_width;
    }

    display_speech(displbuf, char_id, -1, -1, width, true /*auto-pos*/, true /* is thought */);
}

void display_speech(const char *text, int char_id, int x, int y, int width, bool auto_position, bool is_thought)
{
    Game &game = Game::Current();
    if (!AssertCharacter("DisplaySpeech", char_id))
    {
        return;
    }

    Character *speaking_char = &game.Characters[char_id];
    if ((speaking_char->view < 0) || (speaking_char->view >= game.numviews))
    {
        quit("!DisplaySpeech: character has invalid view");
    }

    if (game.Play.screen_is_faded_out)
        debug_script_warn("Blocking Say call during fade-out.");
    if (game.Play.text_overlay_on > 0)
    {
        debug_script_warn("DisplaySpeech: speech was already displayed (nested DisplaySpeech, perhaps room script and "
                          "global script conflict?)");
        return;
    }

    debug_script_log("%s: \"%s\"", speaking_char->scrname.GetCStr(), text);

    EndSkippingUntilCharStops();

    game.Speech.SaidSpeechLine = 1;

    if (game.Play.bgspeech_stay_on_display == 0)
    {
        // remove any background speech
        auto &overs = get_overlays();
        for (auto &over : overs)
        {
            if (over.GetTimeout() > 0)
                remove_screen_overlay(over.GetID());
        }
    }
    game.Speech.SaidText = 1;

    set_our_eip(150);

    int is_pause = 1;
    // if the message is all .'s, don't display anything
    for (size_t i = 0; text[i] != 0; i++)
    {
        if (text[i] != '.')
        {
            is_pause = 0;
            break;
        }
    }

    game.Play.messagetime = GetTextDisplayTime(text);
    game.Play.speech_in_post_state = false;

    if (is_pause)
    {
        postpone_scheduled_music_update_by(
            std::chrono::milliseconds(game.Play.messagetime * 1000 / game.FramesPerSecond));
        // Set a post-state right away, as we only need to wait for a messagetime timer
        game.Play.speech_in_post_state = true;
        GameLoopUntilValueIsNegative(&game.Play.messagetime);
        post_display_cleanup();
        return;
    }

    DisplayTextStyle disp_style = kDisplayTextStyle_Overchar;
    // If the character is in this room, then default to aligning the speech
    // to the character position; but if it's not then center the speech on screen
    // NOTE: clamping the freely positioned speech (see SayAt) to the screen width
    // is a historical behavior. It's not known why but it was not clamped to height;
    // this has to be kept in mind if backwards compatibility matters.
    DisplayTextPosition disp_pos =
        (DisplayTextPosition)((auto_position
                                   ? get_textpos_from_scriptcoords(x, y, (speaking_char->room == game.DisplayedRoom))
                                   : kDisplayTextPos_Normal) |
                              kDisplayTextPos_ClampToScreenWidth);
    const color_t text_color = speaking_char->talkcolor;

    DisplayTextShrink allow_shrink = kDisplayTextShrink_None;
    bool align_hcenter = false; // whether to align text by centering over position

    const Rect ui_view = game.Viewports.GetUIViewport();
    int bwidth = width;
    if (bwidth < 0)
        bwidth = ui_view.GetWidth() / 2 + ui_view.GetWidth() / 4;

    set_our_eip(151);

    int use_view = speaking_char->talkview;
    if (is_thought)
    {
        use_view = speaking_char->thinkview;
        // view 0 is not valid for think views
        if (use_view == 0)
        {
            use_view = -1;
        }
        // speech bubble can shrink to fit
        allow_shrink = kDisplayTextShrink_Left;
    }

    if (use_view >= game.numviews)
    {
        quitprintf("!Character.Say: attempted to use view %d for animation, but it does not exist", use_view + 1);
    }

    if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
        remove_screen_overlay(OVER_COMPLETE);
    set_our_eip(1500);

    if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_LucasArts)
        allow_shrink = kDisplayTextShrink_Left;

    // If has a valid speech view, and idle anim in progress for the character, then stop it
    if (use_view >= 0)
    {
        stop_character_idling(speaking_char);
    }

    // TODO: consider turning certain speech styles into autoplaced overlays
    // in the future; but that would require a large refactor of all the coordinate
    // calculations below, and inside display_main.
    int tdxp = x, tdyp = y;
    int old_view = -1, oldloop = -1;
    int ovr_type = 0;
    text_lips_offset = 0;
    text_lips_text = text;
    std::unique_ptr<Bitmap> closeupface;
    int char_frame_was = 0;
    const bool view_was_locked = (speaking_char->flags & CHF_FIXVIEW);
    game.Speech.CharSpeaking = speaking_char->index_id;

    // Start voice-over, if requested by the tokens in speech text
    try_auto_play_speech(text, text, char_id);

    if (speaking_char->room == game.DisplayedRoom)
    {
        // If the character is in this room, go for it - otherwise
        // run the "else" clause which  does text in the middle of
        // the screen.
        set_our_eip(1501);

        if (speaking_char->walking)
        {
            Character_StopMoving(speaking_char);
        }

        // save the frame we need to go back to
        // if they were moving, this will be 0 (because we just called
        // StopMoving); otherwise, it might be a specific animation
        // frame which we should return to
        if (view_was_locked)
            char_frame_was = speaking_char->frame;

        if ((speaking_char->view < 0) || game.Views[speaking_char->view].numLoops == 0)
        {
            quitprintf("!Character %s current view %d is invalid, or has no loops.", speaking_char->scrname.GetCStr(),
                       speaking_char->view + 1);
        }
        // If current view is missing a loop - use loop 0
        if (speaking_char->loop >= game.Views[speaking_char->view].numLoops)
        {
            debug_script_warn("Character %s current view %d does not have necessary loop %d; switching to loop 0.",
                              speaking_char->scrname.GetCStr(), speaking_char->view + 1, speaking_char->loop);
            speaking_char->loop = 0;
        }

        set_our_eip(1504);

        // Calculate speech position based on character's position on screen
        // (an assumption here, but this must be the case if it's LA-style speech)
        auto view = FindNearestViewport(char_id);
        if (tdxp < 0)
            tdxp = view->RoomToScreen(speaking_char->x, 0).first.X;
        if (tdxp < 2)
            tdxp = 2;
        // tell it to align it by center
        align_hcenter = auto_position && (x < 0);

        if (tdyp < 0)
        {
            int sppic = game.Views[speaking_char->view].loops[speaking_char->loop].frames[0].pic;
            int height = (speaking_char->height < 1) ? game.SpriteInfos[sppic].Height : speaking_char->height;
            tdyp = view->RoomToScreen(0, speaking_char->GetEffectiveY() - height).first.Y - 5;
            if (is_thought) // if it's a thought, lift it a bit further up
                tdyp -= 10;
        }
        if (tdyp < 5)
            tdyp = 5;

        set_our_eip(152);

        if ((game.options[OPT_SPEECHTYPE] > kSpeechStyle_LucasArts) && (use_view >= 0) &&
            (game.Views[use_view].numLoops > 0))
        {
            // Sierra-style close-up portrait
            disp_pos = kDisplayTextPos_Normal;

            if (game.Play.swap_portrait_lastchar != char_id)
            {
                // if the portraits are set to Alternate, OR they are
                // set to Left but swap_portrait has been set to 1 (the old
                // method for enabling it), then swap them round
                if ((game.options[OPT_PORTRAITSIDE] == PORTRAIT_ALTERNATE) ||
                    ((game.options[OPT_PORTRAITSIDE] == 0) && (game.Play.swap_portrait_side > 0)))
                {
                    if (game.Play.swap_portrait_side == 2)
                        game.Play.swap_portrait_side = 1;
                    else
                        game.Play.swap_portrait_side = 2;
                }

                if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION)
                {
                    // Portrait side based on character X-positions
                    if (game.Play.swap_portrait_lastchar < 0)
                    {
                        // No previous character been spoken to
                        // therefore, assume it's the player
                        if (game.playercharacter != char_id &&
                            game.Characters[game.playercharacter].room == speaking_char->room &&
                            game.Characters[game.playercharacter].is_enabled())
                        {
                            game.Play.swap_portrait_lastchar = game.playercharacter;
                        }
                        else
                        {
                            // The player's not here. Find another character in this room
                            // that it could be
                            for (int ce = 0; ce < game.numcharacters; ce++)
                            {
                                if ((game.Characters[ce].room == speaking_char->room) &&
                                    (game.Characters[ce].is_enabled()) && (ce != char_id))
                                {
                                    game.Play.swap_portrait_lastchar = ce;
                                    break;
                                }
                            }
                        }
                    }

                    if (game.Play.swap_portrait_lastchar >= 0)
                    {
                        // if this character is right of the one before, put the
                        // portrait on the right
                        if (speaking_char->x > game.Characters[game.Play.swap_portrait_lastchar].x)
                        {
                            game.Play.swap_portrait_side = -1;
                        }
                        else
                            game.Play.swap_portrait_side = 0;
                    }
                }
                game.Play.swap_portrait_lastlastchar = game.Play.swap_portrait_lastchar;
                game.Play.swap_portrait_lastchar = char_id;
            }
            else
                // If the portrait side is based on the character's X position and the same character is
                // speaking, compare against the previous *previous* character to see where the speech should be
                if (game.options[OPT_PORTRAITSIDE] == PORTRAIT_XPOSITION && game.Play.swap_portrait_lastlastchar >= 0)
                {
                    if (speaking_char->x > game.Characters[game.Play.swap_portrait_lastlastchar].x)
                    {
                        game.Play.swap_portrait_side = -1;
                    }
                    else
                        game.Play.swap_portrait_side = 0;
                }

            // Determine whether to display the portrait on the left or right
            int portrait_on_right = 0;

            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
            {
            } // always on left with QFG-style speech
            else if ((game.Play.swap_portrait_side == 1) || (game.Play.swap_portrait_side == -1) ||
                     (game.options[OPT_PORTRAITSIDE] == PORTRAIT_RIGHT))
                portrait_on_right = 1;


            int bigx = 0, bigy = 0, kk;
            ViewStruct *viptr = &game.Views[use_view];
            for (kk = 0; kk < viptr->loops[0].numFrames; kk++)
            {
                int tw = game.SpriteInfos[viptr->loops[0].frames[kk].pic].Width;
                if (tw > bigx)
                    bigx = tw;
                tw = game.SpriteInfos[viptr->loops[0].frames[kk].pic].Height;
                if (tw > bigy)
                    bigy = tw;
            }

            // if they accidentally used a large full-screen image as the sierra-style
            // talk view, correct it
            if ((game.options[OPT_SPEECHTYPE] != kSpeechStyle_QFG4) && (bigx > ui_view.GetWidth() - 50))
                bigx = ui_view.GetWidth() - 50;

            if (width > 0)
            {
                bwidth = width - bigx;
            }

            set_our_eip(153);
            int ovr_yp = 20;
            int view_frame_x = 0;
            int view_frame_y = 0;
            game.Speech.Qfg4OverridePlacementX = false;
            game.Speech.Qfg4OverridePlacementY = false;

            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
            {
                // QFG4-style whole screen picture
                closeupface.reset(BitmapHelper::CreateBitmap(ui_view.GetWidth(), ui_view.GetHeight()));
                closeupface->Clear(0);
                if (x < 0 && game.Play.speech_portrait_placement)
                {
                    game.Speech.Qfg4OverridePlacementX = true;
                    view_frame_x = game.Play.speech_portrait_x;
                }
                if (y < 0 && game.Play.speech_portrait_placement)
                {
                    game.Speech.Qfg4OverridePlacementY = true;
                    view_frame_y = game.Play.speech_portrait_y;
                }
                else
                {
                    view_frame_y = ui_view.GetHeight() / 2 - game.SpriteInfos[viptr->loops[0].frames[0].pic].Height / 2;
                }
                bigx = ui_view.GetWidth() / 2 - 20;
                ovr_type = OVER_COMPLETE;
                ovr_yp = 0;
                tdyp = -1; // center vertically
            }
            else
            {
                // KQ6-style close-up face picture
                if (y < 0 && game.Play.speech_portrait_placement)
                {
                    ovr_yp = game.Play.speech_portrait_y;
                }
                else if (y < 0)
                {
                    ovr_yp = adjust_y_for_guis(ovr_yp, true /* displayspeech is always blocking */);
                }
                else
                    ovr_yp = y;

                closeupface.reset(BitmapHelper::CreateTransparentBitmap(bigx + 1, bigy + 1));
                ovr_type = OVER_PICTURE;

                if (y < 0)
                {
                    tdyp = ovr_yp + get_textwindow_top_border_height(game.Play.speech_textwindow_gui);
                }
            }
            const ViewFrame *vf = &viptr->loops[0].frames[0];
            DrawViewFrame(closeupface.get(), vf, view_frame_x, view_frame_y);

            int overlay_x = 10;
            if (x < 0)
            {
                tdxp = bigx + get_textwindow_border_width(game.Play.speech_textwindow_gui) / 2;
                if (game.Play.speech_portrait_placement)
                {
                    overlay_x = game.Play.speech_portrait_x;
                    tdxp += overlay_x + 6;
                }
                else
                {
                    tdxp += 16;
                }

                int max_width =
                    (ui_view.GetWidth() - tdxp) - 5 - get_textwindow_border_width(game.Play.speech_textwindow_gui) / 2;

                if (bwidth > max_width)
                {
                    bwidth = max_width;
                }
            }
            else
            {
                tdxp = x + bigx + 8;
                overlay_x = x;
            }

            // allow the text box to be shrunk to fit the text
            allow_shrink = kDisplayTextShrink_Left;

            // if the portrait's on the right, swap it round
            if (portrait_on_right)
            {
                if ((x < 0) || (width < 0))
                {
                    tdxp = 9;
                    if (game.Play.speech_portrait_placement)
                    {
                        overlay_x = (ui_view.GetWidth() - bigx) - game.Play.speech_portrait_x;
                        int max_width =
                            overlay_x - tdxp - 9 - get_textwindow_border_width(game.Play.speech_textwindow_gui) / 2;
                        if (bwidth > max_width)
                        {
                            bwidth = max_width;
                        }
                    }
                    else
                    {
                        overlay_x = (ui_view.GetWidth() - bigx) - 5;
                    }
                }
                else
                {
                    overlay_x = (x + width - bigx) - 5;
                    tdxp = x;
                }
                tdxp += get_textwindow_border_width(game.Play.speech_textwindow_gui) / 2;
                allow_shrink = kDisplayTextShrink_Right;
            }
            if (game.options[OPT_SPEECHTYPE] == kSpeechStyle_QFG4)
                overlay_x = 0;
            game.Speech.FaceTalking = add_screen_overlay(false, overlay_x, ovr_yp, ovr_type, std::move(closeupface));
            game.Speech.FaceTalkView = use_view;
            game.Speech.FaceTalkLoop = 0;
            game.Speech.FaceTalkFrame = 0;
            game.Speech.FaceTalkWait =
                viptr->loops[0].frames[0].speed + Character_GetSpeechAnimationDelay(speaking_char);
            game.Speech.FaceTalkRepeat = (is_thought) ? 0 : 1;
            game.Speech.FaceTalkBlinkLoop = 0;
            game.Speech.FaceTalkAllowBlink = 1;
            if ((is_thought) && (speaking_char->flags & CHF_NOBLINKANDTHINK))
            {
                game.Speech.FaceTalkAllowBlink = 0;
            }
            game.Speech.FaceTalkChar = &game.Characters[char_id];
            if (game.Speech.FaceTalkChar->blinktimer < 0)
                game.Speech.FaceTalkChar->blinktimer = game.Speech.FaceTalkChar->blinkinterval;
            disp_style = kDisplayTextStyle_TextWindow;
            // Process the first portrait view frame
            const int frame_vol = game.Speech.FaceTalkChar->GetFrameSoundVolume();
            CheckViewFrame(game.Speech.FaceTalkView, game.Speech.FaceTalkLoop, game.Speech.FaceTalkFrame, frame_vol);
        }
        else if (use_view >= 0)
        {
            // Lucasarts-style speech
            set_our_eip(154);

            old_view = speaking_char->view;
            oldloop = speaking_char->loop;

            speaking_char->SetAnimating(is_thought ? kAnimFlow_Once
                                                   : kAnimFlow_Repeat, // only repeat if speech, not thought
                                        kAnimDirForward,               // always forwards
                                        Character_GetSpeechAnimationDelay(speaking_char));

            speaking_char->view = use_view;
            speaking_char->frame = 0;
            speaking_char->flags |= CHF_FIXVIEW;

            if ((speaking_char->view < 0) || game.Views[speaking_char->view].numLoops == 0)
            {
                quitprintf("!Character %s speech view %d is invalid, or has no loops.",
                           speaking_char->scrname.GetCStr(), speaking_char->view + 1);
            }
            // If speech view is missing a loop - use loop 0
            if (speaking_char->loop >= game.Views[speaking_char->view].numLoops)
            {
                debug_script_warn("Character %s speech view %d does not have necessary loop %d; switching to loop 0.",
                                  speaking_char->scrname.GetCStr(), speaking_char->view + 1, speaking_char->loop);
                speaking_char->loop = 0;
            }

            game.Speech.FaceTalkBlinkLoop = speaking_char->loop;

            // set up the speed of the first frame
            speaking_char->wait = Character_GetSpeechAnimationDelay(speaking_char) +
                                  game.Views[speaking_char->view].loops[speaking_char->loop].frames[0].speed;

            if (width < 0)
            {
                bwidth = ui_view.GetWidth() / 2 + ui_view.GetWidth() / 6;
                // If they are close to the screen edge, make the text narrower
                int relx = game.Viewports.RoomToScreenX(speaking_char->x);
                if ((relx < ui_view.GetWidth() / 4) || (relx > ui_view.GetWidth() - (ui_view.GetWidth() / 4)))
                    bwidth -= ui_view.GetWidth() / 5;
            }
            if (!is_thought) // set up the lip sync if not thinking
                game.Speech.CharSpeakingAnim = char_id;
        }
    }
    else
    {
        // If the character is in another room, then center the speech on screen
        allow_shrink = kDisplayTextShrink_Left;
    }

    // If initial argument was NOT requiring a autoposition,
    // but further calculation set it to be centered, then make it so here
    // (NOTE: this assumes that a valid width is also passed)
    if ((x >= 0) && align_hcenter)
    {
        tdxp -= width / 2;
    }

    // if they used DisplaySpeechAt, then use the supplied width
    if ((width > 0) && (!is_thought))
    {
        allow_shrink = kDisplayTextShrink_None;
    }

    if (is_thought)
        char_thinking = char_id;

    set_our_eip(155);
    display_main(tdxp, tdyp, bwidth, text, nullptr, kDisplayText_Speech, 0 /* no overid */,
                 DisplayTextLooks(disp_style, disp_pos, allow_shrink, is_thought), game.Play.speech_font, text_color,
                 -1 /* don't autoplace */);
    set_our_eip(156);
    if (game.Speech.FaceTalking >= 0)
        remove_screen_overlay(game.Speech.FaceTalking);
    game.Speech.FaceTalking = -1;
    game.Speech.FaceTalkChar = nullptr;
    set_our_eip(157);
    // Following will restore character's main view after Lucasarts-style speech
    if (old_view >= 0)
    {
        // Restore the character's locked view (if one was locked prior to the speech)
        speaking_char->flags &= ~CHF_FIXVIEW;
        if (view_was_locked)
            speaking_char->flags |= CHF_FIXVIEW;
        speaking_char->view = old_view;
        stop_character_anim(speaking_char);
        speaking_char->frame = char_frame_was;
        speaking_char->wait = 0;
        // Restart idle timer
        reset_character_idling_time(speaking_char);
    }
    game.Speech.CharSpeaking = -1;
    game.Speech.CharSpeakingAnim = -1;
    char_thinking = -1;
    // Stop any blocking voice-over, if was started by this function
    if (game.Play.IsBlockingVoiceSpeech())
        stop_voice_speech();
}

int get_character_currently_talking()
{
    Game &game = Game::Current();
    if ((game.Speech.FaceTalking >= 0) && (game.Speech.FaceTalkRepeat))
        return game.Speech.FaceTalkChar->index_id;
    else if (game.Speech.CharSpeaking >= 0)
        return game.Speech.CharSpeaking;

    return -1;
}

void DisplaySpeech(const char *text, int char_id)
{
    display_speech(text, char_id, -1, -1, -1, true /*auto-pos*/, false /* not thought */);
}

// Calculate which frame of the loop to use for this character of
// speech
int GetLipSyncFrame(const char *curtex, int *stroffs)
{
    Game &game = Game::Current();
    size_t bestfit_len = 0;
    int bestfit = game.default_lipsync_frame;
    for (int i = 0; i < MAXLIPSYNCFRAMES; i++)
    {
        char *tptr = game.lipSyncFrameLetters[i];
        while (tptr[0] != 0)
        {
            size_t lenthisbit = strlen(tptr);
            if (strchr(tptr, '/'))
                lenthisbit = strchr(tptr, '/') - tptr;

            if ((ags_strnicmp(curtex, tptr, lenthisbit) == 0) && (lenthisbit > bestfit_len))
            {
                bestfit = i;
                bestfit_len = lenthisbit;
            }
            tptr += lenthisbit;
            while (tptr[0] == '/')
                tptr++;
        }
    }
    // If it's an unknown character, use the default frame
    if (bestfit_len == 0)
        bestfit_len = 1;
    *stroffs += bestfit_len;
    return bestfit;
}

// Updates text-based lipsync
int update_lip_sync(int talkview, int talkloop, int *talkframeptr)
{
    Game &game = Game::Current();
    int talkframe = talkframeptr[0];
    int talkwait = 0;

    // lip-sync speech
    const char *now_saying = &text_lips_text[text_lips_offset];
    // if it's an apostrophe, skip it (we'll, I'll, etc)
    if (now_saying[0] == '\'')
    {
        text_lips_offset++;
        now_saying++;
    }

    if (text_lips_offset >= (int)strlen(text_lips_text))
        talkframe = 0;
    else
    {
        talkframe = GetLipSyncFrame(now_saying, &text_lips_offset);
        if (talkframe >= game.Views[talkview].loops[talkloop].numFrames)
            talkframe = 0;
    }

    talkwait = game.LipSync.LoopsPerCharacter + game.Views[talkview].loops[talkloop].frames[talkframe].speed;

    talkframeptr[0] = talkframe;
    return talkwait;
}

void restore_characters()
{
    Game &game = Game::Current();
    for (int i = 0; i < game.numcharacters; ++i)
    {
        game.Characters[i].zoom_offs = (game.options[OPT_SCALECHAROFFSETS] != 0) ? game.Characters[i].zoom : 100;
    }
}

Rect GetCharacterRoomBBox(int char_id, bool use_frame_0)
{
    Game &game = Game::Current();
    int width, height;
    const Character &ch = game.Characters[char_id];
    int frame = use_frame_0 ? 0 : ch.frame;
    int pic = game.Views[ch.view].loops[ch.loop].frames[frame].pic;
    scale_sprite_size(game, pic, ch.zoom, &width, &height);
    return RectWH(ch.x - width / 2, ch.y - height, width, height);
}

PViewport FindNearestViewport(int char_id)
{
    Game &game = Game::Current();
    Rect bbox = GetCharacterRoomBBox(char_id, true);
    float min_dist = -1.f;
    PViewport nearest_view;
    for (int i = 0; i < game.Viewports.GetRoomViewportCount(); ++i)
    {
        auto view = game.Viewports.GetRoomViewport(i);
        if (!view->IsVisible())
            continue;
        auto cam = view->GetCamera();
        if (!cam)
            continue;
        Rect camr = cam->GetRect();
        float dist = DistanceBetween(bbox, camr);
        if (dist == 0.f)
            return view;
        if (min_dist < 0.f || dist < min_dist)
        {
            min_dist = dist;
            nearest_view = view;
        }
    }
    return nearest_view ? nearest_view : game.Viewports.GetRoomViewport(0);
}

void UpdateInventory()
{
    Game &game = Game::Current();
    for (int cc = 0; cc < game.numcharacters; cc++)
    {
        auto &ch = game.Characters[cc];
        ch.inventory.clear();
        // Iterate through all inv items, adding them once
        // (or multiple times if requested) to the list.
        for (int item = 0; (item < game.numinvitems) && (ch.inventory.size() != MAX_CHAR_INVENTORY); ++item)
        {
            int item_count = ch.get_item_quantity(item);
            if ((game.options[OPT_DUPLICATEINV] == 0) && (item_count > 1))
                item_count = 1;

            for (int i = 0; (i < item_count) && (ch.inventory.size() != MAX_CHAR_INVENTORY); ++i)
            {
                ch.inventory.push_back(item);
            }
        }
    }
    GUIE::MarkInventoryForUpdate(game.playercharacter, true);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "game/dynobj/script_string.h"
#include "script/script_runtime.h"

Character *Character_GetByName(const char *name)
{
    return static_cast<Character *>(ccGetScriptObjectAddress(name, ccDynamicCharacter.GetType()));
}
