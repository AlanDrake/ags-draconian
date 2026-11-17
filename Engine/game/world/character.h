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

#include "game/character_def.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_overlay.h"
#include "game/render/viewport.h"
#include "game/world/entity_state.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicspace.h"
#include "util/geometry.h"
#include "util/string_types.h"

// The engine's character is one object, Character: its definition's fields (CharacterDef, Common: what the game
// file says, which the character starts as a copy of) and its play state (CharacterState: what exists only while
// the game runs). The definitions themselves are kept as loaded in GameDef::Characters and are not written again.

namespace AGS
{
namespace Common
{
class Stream;
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

// A character's save format (CharacterDef and CharacterState)
// Before 4.0: 0 (3.5.0 pre-alpha); 1 (3.5.0) new movelist format (along with pathfinder);
// 2 (3.6.0.25) animation volume; 3 (3.6.1.9) removed movelists, save externally;
// 4 (3.6.1.15) no limit on character name's length; 3060205 32-bit "following" parameters;
// 3060304 blocking x,y; 3060310 audio panning and speed
enum CharacterSvgVersion
{
    kCharSvgVersion_400 = 4000000,    // extended graphic effects (blend, rotate,...)
    kCharSvgVersion_400_03 = 4000003, // compat with 3.6.1.15 (character name's length)
    kCharSvgVersion_400_09 = 4000009, // 32-bit color properties
    kCharSvgVersion_400_13 = 4000013, // compat with 3.6.2.5 (32-bit "following" parameters)
    kCharSvgVersion_400_14 = 4000014, // proper ARGB color properties
    kCharSvgVersion_400_16 = 4000016, // motion path exposed to script, separate runtime flags, turns
    kCharSvgVersion_400_18 = 4000018, // shaders
    kCharSvgVersion_400_20 = 4000020, // expanded and bit more consistent anim params serialization
    kCharSvgVersion_400_26 = 4000026, // sync with 3.6.3.4 (blocking x,y)
    kCharSvgVersion_400_28 = 4000028, // sync with 3.6.3.10 (audio panning and speed)
    kCharSvgVersion_400_29 = 4000029, // rotation pivot
    kCharSvgVersion_400_33 = 4000033, // removed MAX_INV
    kCharSvgVersion_400_34 = 4000034, // opacity instead of transparency, full tint and light ranges
    kCharSvgVersion_Current = kCharSvgVersion_400_34
};

enum CharacterRuntimeFlags
{
    kCharf_Turning = 0x00000001,    // is turning
    kCharf_TurningCCW = 0x00000002, // turning counter-clockwise
    kCharf_TurningMask = (kCharf_Turning | kCharf_TurningCCW),
};

// The play state of a character: what exists only while the game runs. Character holds it with its definition. The
// parts a room object has too are its bases (entity_state.h): the graphic fields (tint, Zoom, Rotation, the sprite's
// size...), the view animation (Loop, Frame, AnimWait, Anim), the move list and the frame sounds' volume, panning
// and speed.
struct CharacterState : public GraphicState, public ViewAnimState, public MoveState, public FrameSoundState
{
    // These were fields of the game file's character record once, as were AnimWait, Loop, Frame and MoveListID of
    // the parts; the file keeps their slots, unused (CharacterDef::ReadFromFile skips them, WriteToFile writes zeros)
    int PreviousRoom = 0;
    int16_t IdleTimeLeft = 0; // time in seconds left before switching to idle animation
    int ActiveInventory = -1; // selected inventory item
    int16_t BlinkInterval = 0;
    int16_t BlinkTimer = 0;
    int16_t BlinkFrame = 0;
    int Z = 0;        // z-location, for flying etc
    int WalkWait = 0; // number of frames to wait before advancing a move;
                      // also used as a turning counter
    int16_t WalkWaitCounter = 0;

    // FIXME: hide the fields under public interface;
    //        use standard int* types
    Point ViewOffset;        // fixed view offset (from locked view)
    Pointf ViewAnchor        // view anchor (from locked view)
        = Pointf(0.5f, 1.f); // default: middle-bottom
    Point EffectiveOffset;   // effective offset (depends on view state)
    Pointf EffectiveAnchor;  // effective anchor (depends on view state)
    int FrameOffsetX = 0;    // current frame offset
    int FrameOffsetY = 0;
    // Full-step position of a character that is shown half way through a step (very small scaling,
    // see UpdateCharacterMoving); FullStepX == INVALID_X when there is none
    short FullStepX = 0;
    short FullStepY = 0;
    bool ProcessIdleThisTime = false;
    char SlowMoveCounter = 0;
    short WalkAnimWait = 0;  // game loops left before the next frame of the walking animation
    int Following = -1;      // whom do we follow (character id)
    int FollowDistance = 0;  // follow distance, in pixels
    int FollowEagerness = 0; // follow reaction
    // Character's inventory, stores item IDs in the order in which items were added.
    // This may include item duplicates if OPT_DUPLICATEINV is enabled.
    std::vector<int> Inventory;
    // Optional character face direction ratio, 0 = ignore
    float FaceDirectionRatio = 0.f;
    int RuntimeFlags = 0; // runtime character flags (CharacterRuntimeFlags)
    int Turns = 0;        // turning counter
    // The values of the character's custom properties that the script set during play, by property name: they hide
    // the definition's values (GameDef) and the schema's defaults (game/world/properties.h)
    AGS::Common::StringIMap PropertyOverrides;

    // Following fields are derivatives of the above (calculated from these
    // and other factors), and hence are not serialized.
    //
    // zoom factor of sprite offsets: the character's zoom if OPT_SCALECHAROFFSETS is set, otherwise 100
    int OffsetZoom = 100;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<CharacterState> Fields();
};

// A character of the running game: its definition's fields, which start as a copy of what the game file says,
// and its play state. The game's characters are Game::Characters, by character id; their addresses are held by
// the managed pool, scripts and plugins, so the list is built once per loaded game and never resized after.
class Character : public CharacterDef, public CharacterState
{
public:
    Character() = default;
    // Starts a character from its definition
    explicit Character(const CharacterDef &def) : CharacterDef(def) {}

    int get_baseline() const;        // return baseline, or Y if not set
    int get_blocking_top() const;    // return Y - BlockingHeight/2
    int get_blocking_bottom() const; // return Y + BlockingHeight/2

    // Gets current character's movelist id
    inline int get_movelist_id() const { return MoveListID; }
    inline bool is_idling() const { return (IdleTimeLeft < 0); }

    // Tells whether the character is turning, and in which direction
    bool IsTurning() const { return (RuntimeFlags & kCharf_Turning) != 0; }
    bool IsTurningClockwise() const { return IsTurning() && (RuntimeFlags & kCharf_TurningCCW) == 0; }
    bool IsTurningCounterClockwise() const { return IsTurning() && (RuntimeFlags & kCharf_TurningCCW) != 0; }
    // Starts (on) or stops turning; turning with no steps (turn_steps <= 0) is stopped
    void SetTurning(bool on, bool ccw, int turn_steps);
    // Counts one turning step; stops turning when none are left
    void DecrementTurning();
    // Stops turning at once
    void StopTurning() { SetTurning(false, false, 0); }

    // Tells whether the character has a view animation set up
    bool IsAnimating() const { return Anim.IsValid(); }
    // Tells whether the animation repeats
    bool IsAnimatingRepeatedly() const { return Anim.IsRepeating(); }
    // Gets the animation delay
    int GetAnimDelay() const { return Anim.Delay; }
    // NOTE: has to return non-const for CycleViewAnim :/
    // this may be solved by having a base class that handles animation, e.g. ViewBasedObject
    ViewAnimateParams &GetAnimParams() { return Anim; }
    // Sets up a view animation
    void SetAnimating(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int anim_volume = 100);
    // Clears the view animation
    void ResetAnimating();

    // Get current effective graphic anchor, which may be either a freely assigned anchor
    // or an offset from the temporarily locked view
    Pointf GetEffectiveGraphicAnchor() const { return EffectiveAnchor; }
    // Get current effective graphic offset, which may be either a freely assigned offset
    // or an offset from the temporarily locked view
    Point GetEffectiveGraphicOffset() const { return EffectiveOffset; }
    // Get visual Y position, which is calculated as Y - Z (Z scaled by OffsetZoom)
    int GetEffectiveY() const;
    // Calculate wanted frame sound volume based on multiple factors
    int GetFrameSoundVolume() const;
    // Set locked view with anchor and offset; also sets the character's view, loop and frame
    void SetLockedView(int new_view, int new_loop, int new_frame, const Pointf &anchor = Pointf(),
                       const Point &off = Point());
    // Set unlocked view, revert to common anchor and offset; also sets the character's default view
    void SetUnlockedView();
    // Process the current animation frame for the character:
    // play linked sounds, run the frame event, and so forth.
    void CheckViewFrame();
    // Run the arbitrary frame's event if one is associated and character has a handler;
    // returns the result of RunViewFrameEvent
    bool RunFrameEvent(int at_view, int at_loop, int at_frame);
    // Sets up following another character (follow_who); a negative follow_who stops following.
    // Eagerness is ignored, and sort_behind applies, only when distance is FOLLOW_ALWAYSONTOP
    void SetFollowing(int follow_who, int distance = 0, int eagerness = 0, bool sort_behind = false);

    // Gets the character's graphic space, as of the last UpdateGraphicSpace
    inline const AGS::Common::GraphicSpace &GetGraphicSpace() const { return _graphicSpace; }

    // Recalculates the effective offset and anchor: the locked view's if the view is locked, else the sprite's
    void UpdateEffectiveValues();
    // Recalculates the graphic space from the character position and the sprite, scaling and rotation fields
    void UpdateGraphicSpace();

    // Reads the play state part of the character from a save (the definition part is read by the save code,
    // ReadCharacterDef in savegame_components.cpp)
    void ReadFromSavegame(AGS::Common::Stream *in, CharacterSvgVersion save_ver);
    // Writes the play state part of the character to a save
    void WriteToSavegame(AGS::Common::Stream *out) const;

private:
    AGS::Common::GraphicSpace _graphicSpace;
};

// **** CHARACTER: FUNCTIONS ****
//
// A Character_<Name> function implements the script function Character.<Name> of the same name, which scripts
// reach through the binding registered in RegisterCharacterAPI (character_api.cpp). A comment
// tells where the script name differs, or the function is not a script function at all.

// Tells if the character ID is valid
bool IsValidCharacter(int char_id);
// Asserts the character ID is valid,
// if not then prints a warning to the log; returns assertion result
bool AssertCharacter(const char *apiname, int char_id);

void Character_AddInventory(Character *ch, ScriptInvItem *item, int at_index);
void Character_AddWaypoint(Character *ch, int x, int y);
// Character.Animate with every argument; Animate5 and Animate6 supply the last ones (sframe, volume) by default.
// Stops an idle animation first, and waits for the animation to end if blocking is set.
void Character_Animate(Character *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe = 0,
                       int volume = 100);
// Character.Animate without the first frame and volume: starts at frame 0, at full volume.
void Character_Animate5(Character *ch, int loop, int delay, int repeat, int blocking, int direction);
// Character.ChangeRoomAutoPosition, the player character only: changes the room and places the character by new_pos,
// or, if new_pos is 0, at the screen edge opposite to the one it stands at.
void Character_ChangeRoomAutoPosition(Character *ch, int room, int new_pos);
void Character_ChangeRoom(Character *ch, int room, int x, int y);
// Character.ChangeRoom with a direction (loop) the character is to face in the new room;
// Character_ChangeRoom passes SCR_NO_VALUE for none.
void Character_ChangeRoomSetLoop(Character *ch, int room, int x, int y, int direction);
void Character_ChangeView(Character *ch, int view);
// Not a script function: sets the character's current view without changing its default view.
// The view number is 0-based and is not validated.
void Character_SetViewDirect(Character *ch, int view, bool is_walk_view);
void Character_FaceDirection(Character *ch, int direction, int blocking_style);
void Character_FaceCharacter(Character *ch, Character *other, int blocking_style);
void Character_FaceLocation(Character *ch, int x, int y, int blocking_style);
void Character_FaceObject(Character *ch, ScriptObject *obj, int blocking_style);
void Character_FollowCharacter(Character *ch, Character *to_follow, int distaway, int eagerness);
int Character_IsCollidingWithChar(Character *ch, Character *other);
int Character_IsCollidingWithObject(Character *ch, ScriptObject *obj);
// Tells if the character has an event handler for the cursor mode (mood), without running it:
// runs Character_RunInteraction in the check-only mode.
bool Character_IsInteractionAvailable(Character *ch, int mood);
void Character_LockView(Character *ch, int view);
// LockViewEx, LockViewAlignedEx, LockViewFrameEx and LockViewOffsetEx implement the script functions LockView
// (with the stop_moving argument), LockViewAligned, LockViewFrame and LockViewOffset; the view is 1-based.
void Character_LockViewEx(Character *ch, int view, int stop_moving);
void Character_LockViewAlignedEx(Character *ch, int view, int loop, int align, int stop_moving);
void Character_LockViewFrameEx(Character *ch, int view, int loop, int frame, int stop_moving);
void Character_LockViewOffsetEx(Character *ch, int view, int xoffs, int yoffs, int stop_moving);
void Character_LoseInventory(Character *ch, ScriptInvItem *item);
void Character_PlaceOnWalkableArea(Character *ch);
void Character_RemoveTint(Character *ch);
// Character.HasExplicitTint.
int Character_GetHasExplicitTint(Character *ch);
void Character_Say(Character *ch, const char *text);
void Character_SayAt(Character *ch, int x, int y, int width, const char *text);
ScriptOverlay *Character_SayBackground(Character *ch, const char *text);
void Character_SetAsPlayer(Character *ch);
void Character_SetIdleView(Character *ch, int iview, int itime);
// Character.SetWalkSpeed.
void Character_SetSpeed(Character *ch, int xspeed, int yspeed);
void Character_StopMoving(Character *ch);
// Not a script function: stops the character's move; if force_walkable_area is set, also moves the character
// to the nearest walkable area when it ended on a non-walkable pixel.
void Character_StopMovingEx(Character *ch, bool force_walkable_area);
void Character_Tint(Character *ch, int red, int green, int blue, int opacity, int luminance);
void Character_Think(Character *ch, const char *text);
void Character_UnlockView(Character *ch);
void Character_UnlockViewEx(Character *ch, int stop_moving);
void Character_Walk(Character *ch, int x, int y, int blocking, int ignwal);
void Character_Move(Character *ch, int x, int y, int blocking, int ignwal);
void Character_WalkStraight(Character *ch, int x, int y, int blocking);

// Character.RunInteraction: runs the character's event for the cursor mode (mood) from the Interactions table,
// then the "any click" event, unless the first one changed the game state.
void Character_RunInteraction(Character *ch, int mood);

// **** CHARACTER: PROPERTIES ****
//
// Character_Get<Name> and Character_Set<Name> implement the script property Character.<Name> (Character.get_<Name>
// and set_<Name> in the registration), unless a comment says otherwise.

// Character.GetProperty, GetPropertyText and GetTextProperty: read a custom property of the character.
int Character_GetProperty(Character *ch, const char *property);
void Character_GetPropertyText(Character *ch, const char *property, char *buffer);
const char *Character_GetTextProperty(Character *ch, const char *property);

ScriptInvItem *Character_GetActiveInventory(Character *ch);
void Character_SetActiveInventory(Character *ch, ScriptInvItem *item);
// [DEPRECATED] the former script function that sets the player character's active inventory item;
// still called by the engine (gui.cpp, savegame.cpp, Character_SetAsPlayer).
void SetActiveInventory(int item_id);
int Character_GetAnimating(Character *ch);
int Character_GetAnimationSpeed(Character *ch);
void Character_SetAnimationSpeed(Character *ch, int new_value);
int Character_GetBaseline(Character *ch);
void Character_SetBaseline(Character *ch, int baseline);
int Character_GetBlinkInterval(Character *ch);
void Character_SetBlinkInterval(Character *ch, int interval);
int Character_GetBlinkView(Character *ch);
void Character_SetBlinkView(Character *ch, int view);
int Character_GetBlinkWhileThinking(Character *ch);
void Character_SetBlinkWhileThinking(Character *ch, int yesorno);
int Character_GetBlockingHeight(Character *ch);
void Character_SetBlockingHeight(Character *ch, int hit);
int Character_GetBlockingWidth(Character *ch);
void Character_SetBlockingWidth(Character *ch, int wid);
// Character.DiagonalLoops.
int Character_GetDiagonalWalking(Character *ch);
void Character_SetDiagonalWalking(Character *ch, int yesorno);
int Character_GetClickable(Character *ch);
void Character_SetClickable(Character *ch, int clickable);
int Character_GetID(Character *ch);
int Character_GetFrame(Character *ch);
void Character_SetFrame(Character *ch, int new_value);
int Character_GetIdleView(Character *ch);
// Character.InventoryQuantity[] (an indexed property: the index is the inventory item ID).
int Character_GetIInventoryQuantity(Character *ch, int index);
int Character_HasInventory(Character *ch, ScriptInvItem *item);
void Character_SetIInventoryQuantity(Character *ch, int index, int quant);
int Character_GetIgnoreLighting(Character *ch);
void Character_SetIgnoreLighting(Character *ch, int yesorno);
void Character_SetManualScaling(Character *ch, int yesorno);
int Character_GetMovementLinkedToAnimation(Character *ch);
void Character_SetMovementLinkedToAnimation(Character *ch, int yesorno);
int Character_GetLoop(Character *ch);
void Character_SetLoop(Character *ch, int new_value);
int Character_GetMoving(Character *ch);
// Character.DisplayName, and the obsolete Character.Name, which is registered to the same functions.
const char *Character_GetName(Character *ch);
void Character_SetName(Character *ch, const char *name);
int Character_GetNormalView(Character *ch);
int Character_GetPreviousRoom(Character *ch);
int Character_GetRoom(Character *ch);
int Character_GetScaleMoveSpeed(Character *ch);
void Character_SetScaleMoveSpeed(Character *ch, int yesorno);
int Character_GetScaleVolume(Character *ch);
void Character_SetScaleVolume(Character *ch, int yesorno);
int Character_GetScaling(Character *ch);
void Character_SetScaling(Character *ch, int zoomlevel);
int Character_GetSolid(Character *ch);
void Character_SetSolid(Character *ch, int yesorno);
int Character_GetSpeaking(Character *ch);
int Character_GetSpeechColor(Character *ch);
void Character_SetSpeechColor(Character *ch, int ncol);
int Character_GetSpeechAnimationDelay(Character *ch);
void Character_SetSpeechAnimationDelay(Character *ch, int delay);
int Character_GetSpeechView(Character *ch);
void Character_SetSpeechView(Character *ch, int view);
int Character_GetThinkView(Character *ch);
void Character_SetThinkView(Character *ch, int view);
int Character_GetOpacity(Character *ch);
void Character_SetOpacity(Character *ch, int opacity);
int Character_GetTurnBeforeWalking(Character *ch);
void Character_SetTurnBeforeWalking(Character *ch, int on);
int Character_GetView(Character *ch);
int Character_GetWalkSpeedX(Character *ch);
int Character_GetWalkSpeedY(Character *ch);
int Character_GetX(Character *ch);
void Character_SetX(Character *ch, int new_value);
int Character_GetY(Character *ch);
void Character_SetY(Character *ch, int new_value);
int Character_GetZ(Character *ch);
void Character_SetZ(Character *ch, int new_value);
int Character_GetSpeakingFrame(Character *ch);
int Character_GetBlendMode(Character *ch);
void Character_SetBlendMode(Character *ch, int blend_mode);

//=============================================================================

struct EngineServices;
class Game;
class MoveList;
class Room;
namespace AGS
{
namespace Common
{
class Bitmap;
}
} // namespace AGS

// Configures and starts character animation.
void animate_character(Character *ch, int loop, int sppd, int rept, int direction = 0, int sframe = 0,
                       int volume = 100);
// Clears the character's animation state, which stops the animation started by animate_character
void stop_character_anim(Character *ch);
// Returns the position of the loop in turnlooporder (the order of loops to turn the character through);
// 0 if the loop is not there.
int find_looporder_index(int curloop);
// Tells if character should use diagonal loops;
// optionally require that diagonal loops have an actual animation (> 1 frame)
bool should_use_diagloops(Character *ch, bool require_animation);
// Tells if the view has frames in both the Down and the Up loops; false if it has only horizontal animations
bool has_updown_loops(Character *ch);
// Starts turning the character from its current loop to use_loop, one step of turnlooporder per update,
// in the shorter direction (equal ones are decided by the "turn order priority" game option). Loops that do not
// exist or have no frames are skipped, so are the diagonal ones if use_diagloops is false; the turning is not
// started if no step is needed.
void start_character_turning(Character *ch, int use_loop, bool use_diagloops);
// Sets the loop of a walking character according to the direction of its current move, either right away or,
// when the game turns characters while they walk, by turning it step by step. Does nothing if the move has no speed.
void fix_player_sprite(Character *ch, const MoveList &cmls);
// Check whether two characters have walked into each other
int has_hit_another_character(Game &game, int source_char);
// Does the next move from the character's movelist.
// Returns 1 if the character is now waiting for another one to move, otherwise 0.
int doNextCharMoveStep(Game &game, Character *ch);
// Tells if character is currently moving, in eWalkableAreas mode
bool is_char_walking_ndirect(Character *ch);
// Finds the walkable point nearest to src (src itself if it is walkable) and writes it to dst; returns false if
// there is none. Room edges limit the search, unless src is already beyond them.
bool FindNearestWalkableAreaForCharacter(const Point &src, Point &dst);
// Makes sure the character's loop exists in its view and has frames: otherwise takes the default loop, or the first
// one that has frames. With is_walk_view only the walking loops count (the first 4, or 8 if diagonals are allowed).
void FindReasonableLoopForCharacter(Character *ch, bool is_walk_view);
// Start character walk or move; calculate path using destination and optionally "ignore walls" flag
void move_character(Character *ch, int tox, int toy, bool ignwal, bool walk_anim);
// Start character walk or move, using a predefined path
void move_character(Character *ch, const std::vector<Point> &path, bool walk_anim, const RunPathParams &run_params);
// Start character walk or move along the straight line without pathfinding, until any non-passable area is met
void move_character_straight(Character *ch, int x, int y, bool walk_anim);
// Start character walk; calculate path using destination and optionally "ignore walls" flag
void walk_character(Character *ch, int tox, int toy, bool ignwal);
// Walks the character to the walk-to point of the hotspot and waits until it stops;
// does nothing if the hotspot has no walk-to point.
void MoveCharacterToHotspot(int char_id, int hotsp);
// Tells how many steps the character makes this update, as its speed scales with its zoom level
// (CHF_SCALEMOVESPEED): 1 normally, 2 when faster, 0 when it skips the update, and a negative value for
// abs(value) steps made half-way, with the position saved to be completed on the next update (UpdateCharacterMoving).
int wantMoveNow(Character *ch);
// Makes the character the player: sets game.PlayerCharacterID, Game::PlayerChar and the script pointer to it.
void setup_player_character(int char_id);
// Width and height of the character's sprite: the scaled size calculated by update_character_scale, or the size of
// the current frame until then; 4 and 2 if the frame is invalid.
int GetCharacterWidth(Game &game, int char_id);
int GetCharacterHeight(Game &game, int char_id);

// Get current character's image; this may return either source image, or cached transformed image
AGS::Common::Bitmap *GetCharacterImage(int char_id, bool always_original = false, bool *is_original = nullptr);
// Gets current source image (untransformed) for the character
AGS::Common::Bitmap *GetCharacterSourceImage(Game &game, EngineServices &services, int char_id);
// Deduces the character's scale, accounting for both manual scaling and the room region effects;
// calculates the resulting sprite size. Does nothing for a disabled character or one in another room.
void update_character_scale(Game &game, Room &room, int char_id);
// Get character ID at the given room coordinates
int GetCharIDAtRoom(int x, int y, int hit_options);
// Returns the character's blocking rectangle in room coordinates: the blocking width (by default the sprite's
// width less 4) and the blocking top and bottom, with the blocking offsets applied.
Rect get_char_blocking_rect(Game &game, int char_id);
// Runs the interaction for the cursor mode (mood) on the character at the given room position, if there is one;
// returns 1 if there was a character, otherwise 0.
int check_click_on_character(int x, int y, int mood);
// The core of Character.Say and Character.Think, taking the formatted text. An empty speech text only marks
// the character as the last one that spoke, which switches the portrait side.
void DisplaySpeechCore(int char_id, const char *displbuf);
void DisplayThoughtCore(int char_id, const char *displbuf);
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
Rect GetCharacterRoomBBox(int char_id, bool use_frame_0 = false);
// Find a closest viewport given character is to. Checks viewports in their order in game's array,
// and returns either first viewport character's bounding box intersects with (or rather with its camera),
// or the one that is least far away from its camera; calculated as a perpendicular distance between two AABBs.
PViewport FindNearestViewport(int char_id);

//
// Character update functions, defined in character_update.cpp and called from the game update.
// TODO: make these methods of Character.
//
// Updates one character for one game tick: turning, moving, animating, following, idling, in this order; does
// nothing if the character is disabled. Characters following another "always on top" are added to
// following_as_sheep, to be updated afterwards with UpdateFollowingExactlyCharacter.
void UpdateCharacterMoveAndAnim(Game &game, Room &room, Character *ch, std::vector<int> &following_as_sheep);
// Copies the position, room and previous room of the character it follows "always on top", and puts its baseline
// one behind or in front of the followed one's.
void UpdateFollowingExactlyCharacter(Game &game, Character *ch);
// Makes the next turn step of a turning character; returns true if the character was turning.
bool UpdateCharacterTurning(Game &game, Character *ch);
// Makes the move steps of a moving character (see wantMoveNow), and stops it when the move is finished;
// doing_nothing is cleared while the character is still moving.
void UpdateCharacterMoving(Game &game, Character *ch, int &doing_nothing);
// Advances the animation of an animating or idling character, including the lip-sync of a speaking one; returns
// true if the rest of the character's update is to be skipped. doing_nothing is cleared, unless it is an idle animation.
bool UpdateCharacterAnimating(Game &game, Character *ch, int &doing_nothing);
// Counts the idle time of a character that does nothing and starts its idle animation when the time is up.
void UpdateCharacterIdle(Game &game, Character *ch, int &doing_nothing);
// Makes a character that follows another one walk after it, or change room with it, when it has nothing else to do.
void UpdateCharacterFollower(Game &game, Room &room, Character *ch, std::vector<int> &following_as_sheep,
                             int &doing_nothing);
// Rebuilds every character's list of inventory items (Character::Inventory) from the item quantities.
void UpdateInventory();

// Resets idling timer, and marks for immediate update (in case its persistent idling)
void reset_character_idling_time(Character *ch);

// Order of loops to turn the character through, in a circle from down, clockwise
extern const int turnlooporder[8];

class ScriptMotionPath;
class ScriptShaderInstance;

// Script API: Character::Animate^6
void Character_Animate6(Character *ch, int loop, int delay, int repeat, int blocking, int direction, int sframe);

// Script API: Character::get_Following
Character *Character_GetFollowing(Character *ch);

// Script API: Character::LockViewAnchored^6
void Character_LockViewAnchored(Character *ch, int view, float x_anchor, float y_anchor, int x_off, int y_off,
                                int stop_moving);

// Script API: Character::LoseInventoryAt^1
void Character_LoseInventoryAt(Character *ch, int inv_index);

// Script API: Character::get_HasExplicitLight
bool Character_GetHasExplicitLight(Character *ch);

// Script API: Character::get_LightLevel
int Character_GetLightLevel(Character *ch);

// Script API: Character::SetLightLevel^1
void Character_SetLightLevel(Character *ch, int light_level);

// Script API: Character::get_TintRed
int Character_GetTintRed(Character *ch);

// Script API: Character::get_TintGreen
int Character_GetTintGreen(Character *ch);

// Script API: Character::get_TintBlue
int Character_GetTintBlue(Character *ch);

// Script API: Character::get_TintSaturation
int Character_GetTintSaturation(Character *ch);

// Script API: Character::get_TintLuminance
int Character_GetTintLuminance(Character *ch);

// Script API: Character::MoveStraight^3
void Character_MoveStraight(Character *ch, int x, int y, int blocking);

// Script API: Character::WalkPath^4
void Character_WalkPath(Character *ch, void *path_arr, int blocking, int repeat, int direction);

// Script API: Character::MovePath^4
void Character_MovePath(Character *ch, void *path_arr, int blocking, int repeat, int direction);

// Script API: Character::RunFrameEvent^3
bool Character_RunFrameEvent(Character *ch, int view, int loop, int frame);

// Script API: Character::GetGraphicPosition^0
void *Character_GetGraphicPosition(Character *ch);

// Script API: Character::GetGraphicBoundBox^0
void *Character_GetGraphicBoundBox(Character *ch);

// Script API: Character::SetProperty^2
bool Character_SetProperty(Character *ch, const char *property, int value);

// Script API: Character::SetTextProperty^2
bool Character_SetTextProperty(Character *ch, const char *property, const char *value);

// Script API: Character::get_AudioPanning
int Character_GetAudioPanning(Character *ch);

// Script API: Character::set_AudioPanning
void Character_SetAudioPanning(Character *ch, int new_value);

// Script API: Character::get_AudioSpeed
int Character_GetAudioSpeed(Character *ch);

// Script API: Character::set_AudioSpeed
void Character_SetAudioSpeed(Character *ch, int new_value);

// Script API: Character::get_AnimationVolume, Character::get_AudioVolume
int Character_GetAudioVolume(Character *ch);

// Script API: Character::set_AnimationVolume, Character::set_AudioVolume
void Character_SetAudioVolume(Character *ch, int new_value);

// Script API: Character::get_BlockingRectX
int Character_GetBlockingRectX(Character *ch);

// Script API: Character::set_BlockingRectX
void Character_SetBlockingRectX(Character *ch, int x);

// Script API: Character::get_BlockingRectY
int Character_GetBlockingRectY(Character *ch);

// Script API: Character::set_BlockingRectY
void Character_SetBlockingRectY(Character *ch, int y);

// Script API: Character::get_ScriptName
const char *Character_GetScriptName(Character *ch);

// Script API: Character::get_Enabled
bool Character_GetEnabled(Character *ch);

// Script API: Character::set_Enabled
void Character_SetEnabled(Character *ch, bool new_value);

// Script API: Character::get_IdleDelay
int Character_GetIdleDelay(Character *ch);

// Script API: Character::get_IdleTime
int Character_GetIdleTime(Character *ch);

// Script API: Character::get_ManualScaling
int Character_GetManualScaling(Character *ch);

// Script API: Character::get_InventoryCount
int Character_GetInventoryCount(Character *ch);

// Script API: Character::geti_Inventory
ScriptInvItem *Character_GetInventory(Character *ch, int index);

// Script API: Character::get_DestinationX
int Character_GetDestinationX(Character *ch);

// Script API: Character::get_DestinationY
int Character_GetDestinationY(Character *ch);

// Script API: Character::get_IdleAnimationDelay
int Character_GetIdleAnimationDelay(Character *ch);

// Script API: Character::set_IdleAnimationDelay
void Character_SetIdleAnimationDelay(Character *ch, int delay);

// Script API: Character::get_Thinking
bool Character_GetThinking(Character *ch);

// Script API: Character::get_ThinkingFrame
int Character_GetThinkingFrame(Character *ch);

// Script API: Character::get_Transparency
int Character_GetTransparency(Character *ch);

// Script API: Character::set_Transparency
void Character_SetTransparency(Character *ch, int trans);

// Script API: Character::get_Visible
bool Character_GetVisible(Character *ch);

// Script API: Character::set_Visible
void Character_SetVisible(Character *ch, bool new_value);

// Script API: Character::get_GraphicAnchorX
float Character_GetGraphicAnchorX(Character *ch);

// Script API: Character::set_GraphicAnchorX
void Character_SetGraphicAnchorX(Character *ch, float x);

// Script API: Character::get_GraphicAnchorY
float Character_GetGraphicAnchorY(Character *ch);

// Script API: Character::set_GraphicAnchorY
void Character_SetGraphicAnchorY(Character *ch, float y);

// Script API: Character::get_GraphicOffsetX
int Character_GetGraphicOffsetX(Character *ch);

// Script API: Character::set_GraphicOffsetX
void Character_SetGraphicOffsetX(Character *ch, int x);

// Script API: Character::get_GraphicOffsetY
int Character_GetGraphicOffsetY(Character *ch);

// Script API: Character::set_GraphicOffsetY
void Character_SetGraphicOffsetY(Character *ch, int y);

// Script API: Character::get_Shader
ScriptShaderInstance *Character_GetShader(Character *ch);

// Script API: Character::set_Shader
void Character_SetShader(Character *ch, ScriptShaderInstance *shader_inst);

// Script API: Character::get_GraphicRotation
float Character_GetRotation(Character *ch);

// Script API: Character::set_GraphicRotation
void Character_SetRotation(Character *ch, float degrees);

// Script API: Character::get_GraphicPivotX
float Character_GetGraphicPivotX(Character *ch);

// Script API: Character::set_GraphicPivotX
void Character_SetGraphicPivotX(Character *ch, float pivotx);

// Script API: Character::get_GraphicPivotY
float Character_GetGraphicPivotY(Character *ch);

// Script API: Character::set_GraphicPivotY
void Character_SetGraphicPivotY(Character *ch, float pivoty);

// Script API: Character::get_GraphicPivotOffsetX
int Character_GetGraphicPivotOffsetX(Character *ch);

// Script API: Character::set_GraphicPivotOffsetX
void Character_SetGraphicPivotOffsetX(Character *ch, int px);

// Script API: Character::get_GraphicPivotOffsetY
int Character_GetGraphicPivotOffsetY(Character *ch);

// Script API: Character::set_GraphicPivotOffsetY
void Character_SetGraphicPivotOffsetY(Character *ch, int py);

// Script API: Character::get_FaceDirectionRatio
float Character_GetFaceDirectionRatio(Character *ch);

// Script API: Character::set_FaceDirectionRatio
void Character_SetFaceDirectionRatio(Character *ch, float ratio);

// Script API: Character::get_Turning
bool Character_GetTurning(Character *ch);

// Script API: Character::get_TurnWhenFacing
int Character_GetTurnWhenFacing(Character *ch);

// Script API: Character::set_TurnWhenFacing
void Character_SetTurnWhenFacing(Character *ch, int on);

// Script API: Character::get_UseRegionTint
bool Character_GetUseRegionTint(Character *ch);

// Script API: Character::set_UseRegionTint
void Character_SetUseRegionTint(Character *ch, int yesorno);

// Script API: Character::get_ViewAnchorX
float Character_GetViewAnchorX(Character *ch);

// Script API: Character::get_ViewAnchorY
float Character_GetViewAnchorY(Character *ch);

// Script API: Character::get_ViewOffsetX
int Character_GetViewOffsetX(Character *ch);

// Script API: Character::get_ViewOffsetY
int Character_GetViewOffsetY(Character *ch);

// Script API: Character::get_MotionPath
ScriptMotionPath *Character_GetMotionPath(Character *ch);

// Script API: Character::GetAtScreenXY^3
Character *Character_GetAtScreenXY(int x, int y, int hit_options);

// Script API: Character::GetAtScreenXY^2
Character *Character_GetAtScreenXY2(int x, int y);

// Script API: Character::GetAtRoomXY^3
Character *Character_GetAtRoomXY(int x, int y, int hit_options);

// Script API: Character::GetAtRoomXY^2
Character *Character_GetAtRoomXY2(int x, int y);

// Script API: Character::GetByName, Character::GetByScriptName
Character *Character_GetByName(const char *name);
