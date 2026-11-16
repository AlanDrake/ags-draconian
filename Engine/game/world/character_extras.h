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
// The engine's character: Character, a definition from the game file
// (CharacterInfo) and its play state (CharacterExtras) in one object.
// The play state was a separate struct historically, because CharacterInfo
// was exposed to the script API and plugin API in such way that its memory
// layout could not be changed at all.
//
//=============================================================================
#pragma once

#include <vector>

#include "game/character_info.h"
#include "game/runtime_defines.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicspace.h"

// Forward declaration
namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

// A character's save format (CharacterInfo and CharacterExtras)
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

// The play state of a character: what exists only while the game runs. Character holds it with its definition.
struct CharacterExtras
{
    // FIXME: hide the fields under public interface;
    //        use standard int* types
    Point spr_offset;         // fixed sprite offset (translation)
    Pointf spr_anchor         // graphic anchor (relative alignment)
        = Pointf(0.5f, 1.f);  // default: middle-bottom
    Point view_offset;        // fixed view offset (from locked view)
    Pointf view_anchor        // view anchor (from locked view)
        = Pointf(0.5f, 1.f);  // default: middle-bottom
    Point eff_offset;         // effective offset (depends on view state)
    Pointf eff_anchor;        // effective anchor (depends on view state)
    Pointf pivot              // relative rotation pivot
        = Pointf(0.5f, 0.5f); // default: center
    Point pivot_offset;       // relative rotation pivot offset (in pixels)
    int spr_width = 0;        // current sprite size
    int spr_height = 0;
    int frame_xoff = 0; // current frame offset
    int frame_yoff = 0;
    short width = 0; // width/height (includes character scaling!)
    short height = 0;
    short zoom = 100;
    // Full-step position of a character that is shown half way through a step (very small scaling,
    // see UpdateCharacterMoving); xwas == INVALID_X when there is none
    short xwas = 0;
    short ywas = 0;
    short tint_r = 0;
    short tint_g = 0;
    short tint_b = 0;
    short tint_level = 0;
    short tint_light = 0;
    char process_idle_this_time = 0;
    char slow_move_counter = 0;
    ViewAnimateParams anim;
    short animwait = 0;     // current ticks counter, before advancing an animation
    int audio_volume = 100; // default animation volume (relative factor)
    int audio_panning = 0;
    int AudioSpeed = 1000;
    int following = -1;       // whom do we follow (character id)
    int follow_dist = 0;      // follow distance, in pixels
    int follow_eagerness = 0; // follow reaction
    // Character's inventory, stores item IDs in the order in which items were added.
    // This may include item duplicates if OPT_DUPLICATEINV is enabled.
    std::vector<int> inventory;
    AGS::Common::BlendMode blend_mode = AGS::Common::kBlend_Normal;
    int shader_id = 0;
    int shader_handle = 0; // script shader handle
    float rotation = 0.f;
    // Optional character face direction ratio, 0 = ignore
    float face_dir_ratio = 0.f;
    int movelist_handle = 0; // handle to the script movelist
    int RuntimeFlags = 0;    // runtime character flags (CharacterRuntimeFlags)
    int turns = 0;           // turning counter

    // Following fields are derivatives of the above (calculated from these
    // and other factors), and hence are not serialized.
    //
    // zoom factor of sprite offsets: the character's zoom if OPT_SCALECHAROFFSETS is set, otherwise 100
    int zoom_offs = 100;
};

// A character of the running game: its definition's fields, which start as a copy of what the game file says,
// and its play state. The game's characters are Game::Characters, by character id; their addresses are held by
// the managed pool, scripts and plugins, so the list is built once per loaded game and never resized after.
class Character : public CharacterInfo, public CharacterExtras
{
public:
    Character() = default;
    // Starts a character from its definition
    explicit Character(const CharacterInfo &def) : CharacterInfo(def) {}

    int get_baseline() const;        // return baseline, or Y if not set
    int get_blocking_top() const;    // return Y - BlockingHeight/2
    int get_blocking_bottom() const; // return Y + BlockingHeight/2

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
    bool IsAnimating() const { return anim.IsValid(); }
    // Tells whether the animation repeats
    bool IsAnimatingRepeatedly() const { return anim.IsRepeating(); }
    // Gets the animation delay
    int GetAnimDelay() const { return anim.Delay; }
    // NOTE: has to return non-const for CycleViewAnim :/
    // this may be solved by having a base class that handles animation, e.g. ViewBasedObject
    ViewAnimateParams &GetAnimParams() { return anim; }
    // Sets up a view animation
    void SetAnimating(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int anim_volume = 100);
    // Clears the view animation
    void ResetAnimating();

    // Get current effective graphic anchor, which may be either a freely assigned anchor
    // or an offset from the temporarily locked view
    Pointf GetEffectiveGraphicAnchor() const { return eff_anchor; }
    // Get current effective graphic offset, which may be either a freely assigned offset
    // or an offset from the temporarily locked view
    Point GetEffectiveGraphicOffset() const { return eff_offset; }
    // Get visual Y position, which is calculated as Y - Z (Z scaled by zoom_offs)
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
    // ReadCharacterInfo in savegame_components.cpp)
    void ReadFromSavegame(AGS::Common::Stream *in, CharacterSvgVersion save_ver);
    // Writes the play state part of the character to a save
    void WriteToSavegame(AGS::Common::Stream *out) const;

private:
    AGS::Common::GraphicSpace _graphicSpace;
};
