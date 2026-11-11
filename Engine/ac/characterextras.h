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
// CharacterExtras is a separate runtime character data. Historically it was
// separated from the design-time CharacterInfo, because the latter is exposed
// to script API and plugin API in such way that its memory layout could not
// be changed at all. Although, today this is less of an issue (see comment
// to CharacterInfo).
//
// TODO: in the long run it will be beneficial to remake this into a more
// explicit runtime Character class, while perhaps keeping CharacterInfo only
// to load design-time data.
//
//=============================================================================
#pragma once

#include <vector>

#include "ac/characterinfo.h"
#include "ac/runtime_defines.h"
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

struct CharacterInfo;

enum CharacterRuntimeFlags
{
    kCharf_Turning = 0x00000001,    // is turning
    kCharf_TurningCCW = 0x00000002, // turning counter-clockwise
    kCharf_TurningMask = (kCharf_Turning | kCharf_TurningCCW),
};

// The CharacterInfo struct size is fixed because it's exposed to script
// and plugin API, therefore new stuff has to go here
// TODO: now safe to merge with CharacterInfo into one class;
// but it's better to leave a separate "character data" struct, loaded from the game file,
// and a Character class, that does all things runtime.
class CharacterExtras
{
public:
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
    short animwait = 0;         // current ticks counter, before advancing an animation
    int audio_volume = 100;     // default animation volume (relative factor)
    int cur_audio_volume = 100; // current animation sound volume (relative factor)
    int audio_panning = 0;
    int audio_speed = 1000;
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
    int flags = 0;           // runtime character flags (CharacterRuntimeFlags)
    int turns = 0;           // turning counter

    // Following fields are derivatives of the above (calculated from these
    // and other factors), and hence are not serialized.
    //
    // zoom factor of sprite offsets: the character's zoom if OPT_SCALECHAROFFSETS is set, otherwise 100
    int zoom_offs = 100;

    // Tells whether the character is turning, and in which direction
    bool IsTurning() const { return (flags & kCharf_Turning) != 0; }
    bool IsTurningClockwise() const { return IsTurning() && (flags & kCharf_TurningCCW) == 0; }
    bool IsTurningCounterClockwise() const { return IsTurning() && (flags & kCharf_TurningCCW) != 0; }
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
    int GetEffectiveY(const CharacterInfo *ch) const;
    // Calculate wanted frame sound volume based on multiple factors
    int GetFrameSoundVolume(const CharacterInfo *ch) const;
    // Set locked view with anchor and offset; also sets the character's view, loop and frame
    void SetLockedView(CharacterInfo *ch, int view, int loop, int frame, const Pointf &anchor = Pointf(),
                       const Point &off = Point());
    // Set unlocked view, revert to common anchor and offset; also sets the character's default view
    void SetUnlockedView(CharacterInfo *ch);
    // Process the current animation frame for the character:
    // play linked sounds, run the frame event, and so forth.
    void CheckViewFrame(CharacterInfo *ch);
    // Run the arbitrary frame's event if one is associated and character has a handler;
    // returns the result of RunViewFrameEvent
    bool RunFrameEvent(CharacterInfo *ch, int view, int loop, int frame);
    // Sets up following another character (follow_who); a negative follow_who stops following.
    // Eagerness is ignored, and sort_behind applies, only when distance is FOLLOW_ALWAYSONTOP
    void SetFollowing(CharacterInfo *ch, int follow_who, int distance = 0, int eagerness = 0, bool sort_behind = false);

    // Gets the character's graphic space, as of the last UpdateGraphicSpace
    inline const AGS::Common::GraphicSpace &GetGraphicSpace() const { return _gs; }

    // Recalculates the effective offset and anchor: the locked view's if the view is locked, else the sprite's
    void UpdateEffectiveValues(const CharacterInfo *ch);
    // Recalculates the graphic space from the character position and the sprite, scaling and rotation fields
    void UpdateGraphicSpace(const CharacterInfo *ch);

    // Read character extra data from saves.
    void ReadFromSavegame(CharacterInfo *ch, AGS::Common::Stream *in, CharacterSvgVersion save_ver);
    // Write character extra data to saves.
    void WriteToSavegame(const CharacterInfo *ch, AGS::Common::Stream *out) const;

private:
    AGS::Common::GraphicSpace _gs;
};
