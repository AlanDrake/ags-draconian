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

// Tells if the character ID is valid
bool IsValidCharacter(int char_id);
// Asserts the character ID is valid,
// if not then prints a warning to the log; returns assertion result
bool AssertCharacter(const char *apiname, int char_id);

void Character_AddInventory(CharacterInfo *ch, ScriptInvItem *invi, int at_index);
void Character_AddWaypoint(CharacterInfo *ch, int x, int y);
void Character_Animate(CharacterInfo *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe = 0,
                       int volume = 100);
void Character_Animate5(CharacterInfo *ch, int loop, int delay, int repeat, int blocking, int direction);
void Character_ChangeRoomAutoPosition(CharacterInfo *ch, int room, int newPos);
void Character_ChangeRoom(CharacterInfo *ch, int room, int x, int y);
void Character_ChangeRoomSetLoop(CharacterInfo *ch, int room, int x, int y, int direction);
void Character_ChangeView(CharacterInfo *ch, int view);
void Character_SetViewDirect(CharacterInfo *ch, int view, bool is_walk_view);
void Character_FaceDirection(CharacterInfo *ch, int direction, int blockingStyle);
void Character_FaceCharacter(CharacterInfo *ch, CharacterInfo *other, int blockingStyle);
void Character_FaceLocation(CharacterInfo *ch, int x, int y, int blockingStyle);
void Character_FaceObject(CharacterInfo *ch, ScriptObject *obj, int blockingStyle);
void Character_FollowCharacter(CharacterInfo *ch, CharacterInfo *tofollow, int distaway, int eagerness);
int Character_IsCollidingWithChar(CharacterInfo *ch, CharacterInfo *other);
int Character_IsCollidingWithObject(CharacterInfo *ch, ScriptObject *objid);
bool Character_IsInteractionAvailable(CharacterInfo *ch, int mood);
void Character_LockView(CharacterInfo *ch, int view);
void Character_LockViewEx(CharacterInfo *ch, int view, int stopMoving);
void Character_LockViewAlignedEx(CharacterInfo *ch, int view, int loop, int align, int stopMoving);
void Character_LockViewFrameEx(CharacterInfo *ch, int view, int loop, int frame, int stopMoving);
void Character_LockViewOffsetEx(CharacterInfo *ch, int view, int xoffs, int yoffs, int stopMoving);
void Character_LoseInventory(CharacterInfo *ch, ScriptInvItem *invi);
void Character_PlaceOnWalkableArea(CharacterInfo *ch);
void Character_RemoveTint(CharacterInfo *ch);
int Character_GetHasExplicitTint(CharacterInfo *ch);
void Character_Say(CharacterInfo *ch, const char *text);
void Character_SayAt(CharacterInfo *ch, int x, int y, int width, const char *text);
ScriptOverlay *Character_SayBackground(CharacterInfo *ch, const char *text);
void Character_SetAsPlayer(CharacterInfo *ch);
void Character_SetIdleView(CharacterInfo *ch, int iview, int itime);
void Character_SetSpeed(CharacterInfo *ch, int xspeed, int yspeed);
void Character_StopMoving(CharacterInfo *ch);
void Character_StopMovingEx(CharacterInfo *ch, bool force_walkable_area);
void Character_Tint(CharacterInfo *ch, int red, int green, int blue, int opacity, int luminance);
void Character_Think(CharacterInfo *ch, const char *text);
void Character_UnlockView(CharacterInfo *ch);
void Character_UnlockViewEx(CharacterInfo *ch, int stopMoving);
void Character_Walk(CharacterInfo *ch, int x, int y, int blocking, int ignwal);
void Character_Move(CharacterInfo *ch, int x, int y, int blocking, int ignwal);
void Character_WalkStraight(CharacterInfo *ch, int x, int y, int blocking);

void Character_RunInteraction(CharacterInfo *ch, int mood);

// **** CHARACTER: PROPERTIES ****

int Character_GetProperty(CharacterInfo *ch, const char *property);
void Character_GetPropertyText(CharacterInfo *ch, const char *property, char *bufer);
const char *Character_GetTextProperty(CharacterInfo *ch, const char *property);

ScriptInvItem *Character_GetActiveInventory(CharacterInfo *ch);
void Character_SetActiveInventory(CharacterInfo *ch, ScriptInvItem *item);
void SetActiveInventory(int item_id); //[DEPRECATED] but used from few other functions
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
void Character_SetBlinkWhileThinking(CharacterInfo *ch, int yesOrNo);
int Character_GetBlockingHeight(CharacterInfo *ch);
void Character_SetBlockingHeight(CharacterInfo *ch, int hit);
int Character_GetBlockingWidth(CharacterInfo *ch);
void Character_SetBlockingWidth(CharacterInfo *ch, int wid);
int Character_GetDiagonalWalking(CharacterInfo *ch);
void Character_SetDiagonalWalking(CharacterInfo *ch, int yesorno);
int Character_GetClickable(CharacterInfo *ch);
void Character_SetClickable(CharacterInfo *ch, int clik);
int Character_GetID(CharacterInfo *ch);
int Character_GetFrame(CharacterInfo *ch);
void Character_SetFrame(CharacterInfo *ch, int newval);
int Character_GetIdleView(CharacterInfo *ch);
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
const char *Character_GetName(CharacterInfo *ch);
void Character_SetName(CharacterInfo *ch, const char *newName);
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
void Character_SetSpeechAnimationDelay(CharacterInfo *ch, int newDelay);
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
// Clears up animation parameters
void stop_character_anim(CharacterInfo *ch);
int find_looporder_index(int curloop);
// Tells if character should use diagonal loops;
// optionally require that diagonal loops have an actual animation (> 1 frame)
bool should_use_diagloops(CharacterInfo *ch, bool require_animation);
// returns if they have vertical loops, or only horizontal ones
bool has_updown_loops(CharacterInfo *ch);
void start_character_turning(CharacterInfo *ch, int useloop, bool use_diagloops);
void fix_player_sprite(CharacterInfo *ch, const MoveList &cmls);
// Check whether two characters have walked into each other
int has_hit_another_character(int sourceChar);
int doNextCharMoveStep(CharacterInfo *ch, CharacterExtras *chex);
// Tells if character is currently moving, in eWalkableAreas mode
bool is_char_walking_ndirect(CharacterInfo *ch);
bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst);
void FindReasonableLoopForCharacter(CharacterInfo *ch, bool is_walk_view);
// Start character walk or move; calculate path using destination and optionally "ignore walls" flag
void move_character(CharacterInfo *ch, int tox, int toy, bool ignwal, bool walk_anim);
// Start character walk or move, using a predefined path
void move_character(CharacterInfo *ch, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params);
// Start character walk or move along the straight line without pathfinding, until any non-passable area is met
void move_character_straight(CharacterInfo *ch, int x, int y, bool walk_anim);
// Start character walk; calculate path using destination and optionally "ignore walls" flag
void walk_character(CharacterInfo *ch, int tox, int toy, bool ignwal);
void MoveCharacterToHotspot(int char_id, int hotsp);
int wantMoveNow(CharacterInfo *ch, CharacterExtras *chex);
void setup_player_character(int charid);
int GetCharacterWidth(int charid);
int GetCharacterHeight(int charid);

// Get current character's image; this may return either source image, or cached transformed image
AGS::Common::Bitmap *GetCharacterImage(int charid, bool always_original = false, bool *is_original = nullptr);
// Gets current source image (untransformed) for the character
AGS::Common::Bitmap *GetCharacterSourceImage(int charid);
// Deduces room object's scale, accounting for both manual scaling and the room region effects;
// calculates resulting sprite size.
void update_character_scale(int charid);
// Get character ID at the given room coordinates
int GetCharIDAtRoom(int x, int y, int hit_options);
Rect get_char_blocking_rect(int charid);
int check_click_on_character(int x, int y, int mood);
void DisplaySpeechCore(int chid, const char *displbuf);
void DisplayThoughtCore(int chid, const char *displbuf);
// Displays character speech or thought message.
// When auto-position flag is on, passing negative coordinates will request
// auto-position on the respective axis. The nature of auto-pos will depend
// on a speech style. E.g. LA speech aligns above character's head.
void display_speech(const char *text, int char_id, int x, int y, int width, bool auto_position, bool is_thought);
int get_character_currently_talking();
void DisplaySpeech(const char *text, int char_id);
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
// Character update functions
// TODO: move these into a runtime Character class, when there will be a proper one,
// merging CharacterInfo and CharacterExtras.
//
void UpdateCharacterMoveAndAnim(CharacterInfo *ch, CharacterExtras *chex, std::vector<int> &followingAsSheep);
void UpdateFollowingExactlyCharacter(CharacterInfo *ch);
bool UpdateCharacterTurning(CharacterInfo *ch, CharacterExtras *chex);
void UpdateCharacterMoving(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
bool UpdateCharacterAnimating(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
void UpdateCharacterIdle(CharacterInfo *ch, CharacterExtras *chex, int &doing_nothing);
void UpdateCharacterFollower(CharacterInfo *ch, std::vector<int> &followingAsSheep, int &doing_nothing);
void UpdateInventory();

extern CharacterInfo *playerchar;
extern int32_t _sc_PlayerCharPtr;

// order of loops to turn character in circle from down to down
extern const int turnlooporder[8];
