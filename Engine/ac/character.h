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
#pragma once

#include <vector>

#include "ac/characterextras.h"
#include "ac/characterinfo.h"
#include "ac/dynobj/scriptobjects.h"
#include "ac/dynobj/scriptoverlay.h"
#include "ac/runtime_defines.h"
#include "game/viewport.h"
#include "util/geometry.h"

// **** CHARACTER: FUNCTIONS ****
//
// A Character_<Name> function implements the script function Character.<Name> of the same name, which scripts
// reach through the Sc_Character_<Name> wrapper registered in RegisterCharacterAPI (character.cpp). A comment
// tells where the script name differs, or the function is not a script function at all.

// Tells if the character ID is valid
bool IsValidCharacter(int char_id);
// Asserts the character ID is valid,
// if not then prints a warning to the log; returns assertion result
bool AssertCharacter(const char *apiname, int char_id);

void Character_AddInventory(CharacterInfo *ch, ScriptInvItem *invi, int at_index);
void Character_AddWaypoint(CharacterInfo *ch, int x, int y);
// Character.Animate with every argument; Animate5 and Animate6 supply the last ones (sframe, volume) by default.
// Stops an idle animation first, and waits for the animation to end if blocking is set.
void Character_Animate(CharacterInfo *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe = 0,
                       int volume = 100);
// Character.Animate without the first frame and volume: starts at frame 0, at full volume.
void Character_Animate5(CharacterInfo *ch, int loop, int delay, int repeat, int blocking, int direction);
// Character.ChangeRoomAutoPosition, the player character only: changes the room and places the character by new_pos,
// or, if new_pos is 0, at the screen edge opposite to the one it stands at.
void Character_ChangeRoomAutoPosition(CharacterInfo *ch, int room, int new_pos);
void Character_ChangeRoom(CharacterInfo *ch, int room, int x, int y);
// Character.ChangeRoom with a direction (loop) the character is to face in the new room;
// Character_ChangeRoom passes SCR_NO_VALUE for none.
void Character_ChangeRoomSetLoop(CharacterInfo *ch, int room, int x, int y, int direction);
void Character_ChangeView(CharacterInfo *ch, int view);
// Not a script function: sets the character's current view without changing its default view.
// The view number is 0-based and is not validated.
void Character_SetViewDirect(CharacterInfo *ch, int view, bool is_walk_view);
void Character_FaceDirection(CharacterInfo *ch, int direction, int blocking_style);
void Character_FaceCharacter(CharacterInfo *ch, CharacterInfo *other, int blocking_style);
void Character_FaceLocation(CharacterInfo *ch, int x, int y, int blocking_style);
void Character_FaceObject(CharacterInfo *ch, ScriptObject *obj, int blocking_style);
void Character_FollowCharacter(CharacterInfo *ch, CharacterInfo *tofollow, int distaway, int eagerness);
int Character_IsCollidingWithChar(CharacterInfo *ch, CharacterInfo *other);
int Character_IsCollidingWithObject(CharacterInfo *ch, ScriptObject *objid);
// Tells if the character has an event handler for the cursor mode (mood), without running it:
// runs Character_RunInteraction in the check-only mode.
bool Character_IsInteractionAvailable(CharacterInfo *ch, int mood);
void Character_LockView(CharacterInfo *ch, int view);
// LockViewEx, LockViewAlignedEx, LockViewFrameEx and LockViewOffsetEx implement the script functions LockView
// (with the stop_moving argument), LockViewAligned, LockViewFrame and LockViewOffset; the view is 1-based.
void Character_LockViewEx(CharacterInfo *ch, int view, int stop_moving);
void Character_LockViewAlignedEx(CharacterInfo *ch, int view, int loop, int align, int stop_moving);
void Character_LockViewFrameEx(CharacterInfo *ch, int view, int loop, int frame, int stop_moving);
void Character_LockViewOffsetEx(CharacterInfo *ch, int view, int xoffs, int yoffs, int stop_moving);
void Character_LoseInventory(CharacterInfo *ch, ScriptInvItem *invi);
void Character_PlaceOnWalkableArea(CharacterInfo *ch);
void Character_RemoveTint(CharacterInfo *ch);
// Character.HasExplicitTint.
int Character_GetHasExplicitTint(CharacterInfo *ch);
void Character_Say(CharacterInfo *ch, const char *text);
void Character_SayAt(CharacterInfo *ch, int x, int y, int width, const char *text);
ScriptOverlay *Character_SayBackground(CharacterInfo *ch, const char *text);
void Character_SetAsPlayer(CharacterInfo *ch);
void Character_SetIdleView(CharacterInfo *ch, int iview, int itime);
// Character.SetWalkSpeed.
void Character_SetSpeed(CharacterInfo *ch, int xspeed, int yspeed);
void Character_StopMoving(CharacterInfo *ch);
// Not a script function: stops the character's move; if force_walkable_area is set, also moves the character
// to the nearest walkable area when it ended on a non-walkable pixel.
void Character_StopMovingEx(CharacterInfo *ch, bool force_walkable_area);
void Character_Tint(CharacterInfo *ch, int red, int green, int blue, int opacity, int luminance);
void Character_Think(CharacterInfo *ch, const char *text);
void Character_UnlockView(CharacterInfo *ch);
void Character_UnlockViewEx(CharacterInfo *ch, int stop_moving);
void Character_Walk(CharacterInfo *ch, int x, int y, int blocking, int ignwal);
void Character_Move(CharacterInfo *ch, int x, int y, int blocking, int ignwal);
void Character_WalkStraight(CharacterInfo *ch, int x, int y, int blocking);

// Character.RunInteraction: runs the character's event for the cursor mode (mood) from the Interactions table,
// then the "any click" event, unless the first one changed the game state.
void Character_RunInteraction(CharacterInfo *ch, int mood);

// **** CHARACTER: PROPERTIES ****
//
// Character_Get<Name> and Character_Set<Name> implement the script property Character.<Name> (Character.get_<Name>
// and set_<Name> in the registration), unless a comment says otherwise.

// Character.GetProperty, GetPropertyText and GetTextProperty: read a custom property of the character.
int Character_GetProperty(CharacterInfo *ch, const char *property);
void Character_GetPropertyText(CharacterInfo *ch, const char *property, char *bufer);
const char *Character_GetTextProperty(CharacterInfo *ch, const char *property);

ScriptInvItem *Character_GetActiveInventory(CharacterInfo *ch);
void Character_SetActiveInventory(CharacterInfo *ch, ScriptInvItem *item);
// [DEPRECATED] the former script function that sets the player character's active inventory item;
// still called by the engine (gui.cpp, savegame.cpp, Character_SetAsPlayer).
void SetActiveInventory(int item_id);
int Character_GetAnimating(CharacterInfo *ch);
int Character_GetAnimationSpeed(CharacterInfo *ch);
void Character_SetAnimationSpeed(CharacterInfo *ch, int newval);
int Character_GetBaseline(CharacterInfo *ch);
void Character_SetBaseline(CharacterInfo *ch, int basel);
int Character_GetBlinkInterval(CharacterInfo *ch);
void Character_SetBlinkInterval(CharacterInfo *ch, int interval);
int Character_GetBlinkView(CharacterInfo *ch);
void Character_SetBlinkView(CharacterInfo *ch, int view);
int Character_GetBlinkWhileThinking(CharacterInfo *ch);
void Character_SetBlinkWhileThinking(CharacterInfo *ch, int yesorno);
int Character_GetBlockingHeight(CharacterInfo *ch);
void Character_SetBlockingHeight(CharacterInfo *ch, int hit);
int Character_GetBlockingWidth(CharacterInfo *ch);
void Character_SetBlockingWidth(CharacterInfo *ch, int wid);
// Character.DiagonalLoops.
int Character_GetDiagonalWalking(CharacterInfo *ch);
void Character_SetDiagonalWalking(CharacterInfo *ch, int yesorno);
int Character_GetClickable(CharacterInfo *ch);
void Character_SetClickable(CharacterInfo *ch, int clik);
int Character_GetID(CharacterInfo *ch);
int Character_GetFrame(CharacterInfo *ch);
void Character_SetFrame(CharacterInfo *ch, int newval);
int Character_GetIdleView(CharacterInfo *ch);
// Character.InventoryQuantity[] (an indexed property: the index is the inventory item ID).
int Character_GetIInventoryQuantity(CharacterInfo *ch, int index);
int Character_HasInventory(CharacterInfo *ch, ScriptInvItem *invi);
void Character_SetIInventoryQuantity(CharacterInfo *ch, int index, int quant);
int Character_GetIgnoreLighting(CharacterInfo *ch);
void Character_SetIgnoreLighting(CharacterInfo *ch, int yesorno);
void Character_SetManualScaling(CharacterInfo *ch, int yesorno);
int Character_GetMovementLinkedToAnimation(CharacterInfo *ch);
void Character_SetMovementLinkedToAnimation(CharacterInfo *ch, int yesorno);
int Character_GetLoop(CharacterInfo *ch);
void Character_SetLoop(CharacterInfo *ch, int newval);
int Character_GetMoving(CharacterInfo *ch);
// Character.DisplayName, and the obsolete Character.Name, which is registered to the same functions.
const char *Character_GetName(CharacterInfo *ch);
void Character_SetName(CharacterInfo *ch, const char *name);
int Character_GetNormalView(CharacterInfo *ch);
int Character_GetPreviousRoom(CharacterInfo *ch);
int Character_GetRoom(CharacterInfo *ch);
int Character_GetScaleMoveSpeed(CharacterInfo *ch);
void Character_SetScaleMoveSpeed(CharacterInfo *ch, int yesorno);
int Character_GetScaleVolume(CharacterInfo *ch);
void Character_SetScaleVolume(CharacterInfo *ch, int yesorno);
int Character_GetScaling(CharacterInfo *ch);
void Character_SetScaling(CharacterInfo *ch, int zoomlevel);
int Character_GetSolid(CharacterInfo *ch);
void Character_SetSolid(CharacterInfo *ch, int yesorno);
int Character_GetSpeaking(CharacterInfo *ch);
int Character_GetSpeechColor(CharacterInfo *ch);
void Character_SetSpeechColor(CharacterInfo *ch, int ncol);
int Character_GetSpeechAnimationDelay(CharacterInfo *ch);
void Character_SetSpeechAnimationDelay(CharacterInfo *ch, int delay);
int Character_GetSpeechView(CharacterInfo *ch);
void Character_SetSpeechView(CharacterInfo *ch, int view);
int Character_GetThinkView(CharacterInfo *ch);
void Character_SetThinkView(CharacterInfo *ch, int view);
int Character_GetOpacity(CharacterInfo *ch);
void Character_SetOpacity(CharacterInfo *ch, int opacity);
int Character_GetTurnBeforeWalking(CharacterInfo *ch);
void Character_SetTurnBeforeWalking(CharacterInfo *ch, int on);
int Character_GetView(CharacterInfo *ch);
int Character_GetWalkSpeedX(CharacterInfo *ch);
int Character_GetWalkSpeedY(CharacterInfo *ch);
int Character_GetX(CharacterInfo *ch);
void Character_SetX(CharacterInfo *ch, int newval);
int Character_GetY(CharacterInfo *ch);
void Character_SetY(CharacterInfo *ch, int newval);
int Character_GetZ(CharacterInfo *ch);
void Character_SetZ(CharacterInfo *ch, int newval);
int Character_GetSpeakingFrame(CharacterInfo *ch);
int Character_GetBlendMode(CharacterInfo *ch);
void Character_SetBlendMode(CharacterInfo *ch, int blend_mode);

//=============================================================================

class MoveList;
namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// Configures and starts character animation.
void animate_character(CharacterInfo *ch, int loopn, int sppd, int rept, int direction = 0, int sframe = 0,
                       int volume = 100);
// Clears the character's animation state, which stops the animation started by animate_character
void stop_character_anim(CharacterInfo *ch);
// Returns the position of the loop in turnlooporder (the order of loops to turn the character through);
// 0 if the loop is not there.
int find_looporder_index(int curloop);
// Tells if character should use diagonal loops;
// optionally require that diagonal loops have an actual animation (> 1 frame)
bool should_use_diagloops(CharacterInfo *ch, bool require_animation);
// Tells if the view has frames in both the Down and the Up loops; false if it has only horizontal animations
bool has_updown_loops(CharacterInfo *ch);
// Starts turning the character from its current loop to useloop, one step of turnlooporder per update,
// in the shorter direction (equal ones are decided by the "turn order priority" game option). Loops that do not
// exist or have no frames are skipped, so are the diagonal ones if use_diagloops is false; the turning is not
// started if no step is needed.
void start_character_turning(CharacterInfo *ch, int useloop, bool use_diagloops);
// Sets the loop of a walking character according to the direction of its current move, either right away or,
// when the game turns characters while they walk, by turning it step by step. Does nothing if the move has no speed.
void fix_player_sprite(CharacterInfo *ch, const MoveList &cmls);
// Check whether two characters have walked into each other
int has_hit_another_character(int source_char);
// Does the next move from the character's movelist.
// Returns 1 if the character is now waiting for another one to move, otherwise 0.
int doNextCharMoveStep(CharacterInfo *ch, CharacterExtras *chex);
// Tells if character is currently moving, in eWalkableAreas mode
bool is_char_walking_ndirect(CharacterInfo *ch);
// Finds the walkable point nearest to src (src itself if it is walkable) and writes it to dst; returns false if
// there is none. Room edges limit the search, unless src is already beyond them.
bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst);
// Makes sure the character's loop exists in its view and has frames: otherwise takes the default loop, or the first
// one that has frames. With is_walk_view only the walking loops count (the first 4, or 8 if diagonals are allowed).
void FindReasonableLoopForCharacter(CharacterInfo *ch, bool is_walk_view);
// Start character walk or move; calculate path using destination and optionally "ignore walls" flag
void move_character(CharacterInfo *ch, int tox, int toy, bool ignwal, bool walk_anim);
// Start character walk or move, using a predefined path
void move_character(CharacterInfo *ch, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params);
// Start character walk or move along the straight line without pathfinding, until any non-passable area is met
void move_character_straight(CharacterInfo *ch, int x, int y, bool walk_anim);
// Start character walk; calculate path using destination and optionally "ignore walls" flag
void walk_character(CharacterInfo *ch, int tox, int toy, bool ignwal);
// Walks the character to the walk-to point of the hotspot and waits until it stops;
// does nothing if the hotspot has no walk-to point.
void MoveCharacterToHotspot(int char_id, int hotsp);
// Tells how many steps the character makes this update, as its speed scales with its zoom level
// (CHF_SCALEMOVESPEED): 1 normally, 2 when faster, 0 when it skips the update, and a negative value for
// abs(value) steps made half-way, with the position saved to be completed on the next update (UpdateCharacterMoving).
int wantMoveNow(CharacterInfo *ch, CharacterExtras *chex);
// Makes the character the player: sets game.playercharacter, playerchar and the script pointer to it.
void setup_player_character(int charid);
// Width and height of the character's sprite: the scaled size calculated by update_character_scale, or the size of
// the current frame until then; 4 and 2 if the frame is invalid.
int GetCharacterWidth(int charid);
int GetCharacterHeight(int charid);

// Get current character's image; this may return either source image, or cached transformed image
AGS::Common::Bitmap *GetCharacterImage(int charid, bool always_original = false, bool *is_original = nullptr);
// Gets current source image (untransformed) for the character
AGS::Common::Bitmap *GetCharacterSourceImage(int charid);
// Deduces the character's scale, accounting for both manual scaling and the room region effects;
// calculates the resulting sprite size. Does nothing for a disabled character or one in another room.
void update_character_scale(int charid);
// Get character ID at the given room coordinates
int GetCharIDAtRoom(int x, int y, int hit_options);
// Returns the character's blocking rectangle in room coordinates: the blocking width (by default the sprite's
// width less 4) and the blocking top and bottom, with the blocking offsets applied.
Rect get_char_blocking_rect(int charid);
// Runs the interaction for the cursor mode (mood) on the character at the given room position, if there is one;
// returns 1 if there was a character, otherwise 0.
int check_click_on_character(int x, int y, int mood);
// The core of Character.Say and Character.Think, taking the formatted text. An empty speech text only marks
// the character as the last one that spoke, which switches the portrait side.
void DisplaySpeechCore(int chid, const char *displbuf);
void DisplayThoughtCore(int chid, const char *displbuf);
// Displays character speech or thought message.
// When auto-position flag is on, passing negative coordinates will request
// auto-position on the respective axis. The nature of auto-pos will depend
// on a speech style. E.g. LA speech aligns above character's head.
void display_speech(const char *text, int char_id, int x, int y, int width, bool auto_position, bool is_thought);
// Returns the ID of the character that is speaking now (including the Sierra-style portrait talking); -1 if none.
int get_character_currently_talking();
// Displays speech of the character with automatic position, without a given width.
void DisplaySpeech(const char *text, int char_id);
// Moves the text-based lip-sync one step: takes the next letters of the speech text, writes the frame they
// map to into *talkframeptr and returns the delay before the next step.
int update_lip_sync(int talkview, int talkloop, int *talkframeptr);

// Recalculate dynamic character properties, e.g. after restoring a game save
void restore_characters();

// Calculates character's bounding box in room coordinates (takes only in-room transform into account)
// use_frame_0 optionally tells to use frame 0 of current loop instead of current frame.
Rect GetCharacterRoomBBox(int charid, bool use_frame_0 = false);
// Find a closest viewport given character is to. Checks viewports in their order in game's array,
// and returns either first viewport character's bounding box intersects with (or rather with its camera),
// or the one that is least far away from its camera; calculated as a perpendicular distance between two AABBs.
PViewport FindNearestViewport(int charid);

//
// Character update functions, defined in characterinfo_engine.cpp and called from the game update.
// TODO: move these into a runtime Character class, when there will be a proper one,
// merging CharacterInfo and CharacterExtras.
//
// Updates one character for one game tick: turning, moving, animating, following, idling, in this order; does
// nothing if the character is disabled. Characters following another "always on top" are added to
// following_as_sheep, to be updated afterwards with UpdateFollowingExactlyCharacter.
void UpdateCharacterMoveAndAnim(CharacterInfo *ch, CharacterExtras *chex, std::vector<int> &following_as_sheep);
// Copies the position, room and previous room of the character it follows "always on top", and puts its baseline
// one behind or in front of the followed one's.
void UpdateFollowingExactlyCharacter(CharacterInfo *ch);
// Makes the next turn step of a turning character; returns true if the character was turning.
bool UpdateCharacterTurning(CharacterInfo *ch, CharacterExtras *chex);
// Makes the move steps of a moving character (see wantMoveNow), and stops it when the move is finished;
// doing_nothing is cleared while the character is still moving.
void UpdateCharacterMoving(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
// Advances the animation of an animating or idling character, including the lip-sync of a speaking one; returns
// true if the rest of the character's update is to be skipped. doing_nothing is cleared, unless it is an idle animation.
bool UpdateCharacterAnimating(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
// Counts the idle time of a character that does nothing and starts its idle animation when the time is up.
void UpdateCharacterIdle(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
// Makes a character that follows another one walk after it, or change room with it, when it has nothing else to do.
void UpdateCharacterFollower(CharacterInfo *ch, std::vector<int> &following_as_sheep, int &doing_nothing);
// Rebuilds every character's list of inventory items (CharacterExtras::inventory) from the item quantities.
void UpdateInventory();

// The player character, and its handle for scripts (the script object "player").
extern CharacterInfo *playerchar;
extern int32_t _sc_PlayerCharPtr;

// Order of loops to turn the character through, in a circle from down, clockwise
extern const int turnlooporder[8];
