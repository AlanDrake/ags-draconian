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

#include <algorithm>
#include <unordered_map>
#include <vector>

#include "game/common_defines.h" // constants
#include "game/game_version.h"
#include "game/scripteventtable.h"
#include "gfx/gfx_def.h"
#include "util/bbop.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

// Character flags (32-bit)
// Flags marked as "INTERNAL" are marking dynamic character state set by the engine
// TODO: move internal flags to a separate 32-bit field in a runtime character class.
// NOTE: flag meaning is inconsistent: some of them have positive (DO) meaning,
// some older ones have negative (DON'T). TODO: bring them to consistency someday,
// but remember that this involves updating game file formats and converting
// after loading game data and restoring older saves.
#define CHF_MANUALSCALING   0x00000001 // Use explicit scaling property rather than area parameters
#define CHF_FIXVIEW         0x00000002 // View locked
#define CHF_NOINTERACT      0x00000004 // Non-interactable (non-clickable)
#define CHF_NODIAGONAL      0x00000008 // Don't use diagonal walking loops
#define CHF_ALWAYSIDLE      0x00000010 // [UNUSED] meaning unknown
#define CHF_NOLIGHTING      0x00000020 // Ignore Region lighting
#define CHF_NOTURNWHENWALK  0x00000040 // Do not turn step-by-step when walking
#define CHF_NOWALKBEHINDS   0x00000080 // [DEPRECATED], forbidden as breaks draw order
#define CHF_FLIPSPRITE      0x00000100 // [UNUSED] meaning unknown
#define CHF_NOBLOCKING      0x00000200 // Not solid
#define CHF_SCALEMOVESPEED  0x00000400 // Scale move speed with character scaling
#define CHF_NOBLINKANDTHINK 0x00000800 // Don't do blink animation when "thinking"
#define CHF_SCALEVOLUME     0x00001000 // Scale animation volume with character scaling
#define CHF_HASTINT         0x00002000 // Use explicit tint rather than region tint
#define CHF_BEHINDSHEPHERD  0x00004000 // [INTERNAL] z-sort behind leader when following another char
#define CHF_AWAITINGMOVE    0x00008000 // [INTERNAL] (meaning not clear, investigate)
#define CHF_MOVENOTWALK     0x00010000 // [INTERNAL] do not play walking animation while moving
#define CHF_ANTIGLIDE       0x00020000 // Link movement to animation
#define CHF_HASLIGHT        0x00040000 // Use explicit lighting rather than region lighting
#define CHF_TINTLIGHTMASK   (CHF_NOLIGHTING | CHF_HASTINT | CHF_HASLIGHT)
#define CHF_TURNWHENFACE    0x00080000 // Turn step-by-step when changing standing direction
// reserve (skip) 4 bits for compatibility with 3.* branch (CHECKME revise some time later)
#define CHF_ENABLED 0x01000000 // Enabled (can be visible + being updated)
#define CHF_VISIBLE 0x02000000 // Visible (actual visibility requires both ENABLED + VISIBLE)

// Value of walk speed indicating that X and Y speed is the same
// Value of CharacterDef::WalkSpeedY that tells to use WalkSpeedX
#define UNIFORM_WALK_SPEED 0
// Value of "followinfo" field that tells to draw follower char above followed
// Value of CharacterDef::followinfo that tells to keep follower z-sorted above the leading char
#define FOLLOW_ALWAYSONTOP 0x7ffe

// Maximal number of character's face directions (loops)
#define MAX_FACE_DIRECTIONS 8
// Number of base directions
#define NUM_BASE_DIRECTIONS 4

// Length of deprecated character name field, in bytes
#define LEGACY_MAX_CHAR_NAME_LEN 40

// Legacy Character's animation flags, packed in a byte
#define LEGACY_CHANIM_MASK      0xFF
#define LEGACY_CHANIM_ON        0x01
#define LEGACY_CHANIM_REPEAT    0x02
#define LEGACY_CHANIM_BACKWARDS 0x04

// These deprecated values were **added** to CharacterDef's MoveList index;
// TURNING_AROUND was added times the character must turn,
// TURNING_BACKWARDS was added if the character is turning counter-clockwise;
// which meant that the number of MoveList users will be limited by 1000,
// and the number of possible turns limited to 9.
#define LEGACY_CHAR_TURNING_AROUND    1000
#define LEGACY_CHAR_TURNING_BACKWARDS 10000


// Converts character flags (CHF_*) to matching RoomObject flags (OBJF_*)
inline int CharFlagsToObjFlags(int chflags)
{
    using namespace AGS::Common;
    return FlagToFlag(chflags, CHF_NOINTERACT, OBJF_NOINTERACT) |
           FlagToFlag(chflags, CHF_NOWALKBEHINDS, OBJF_NOWALKBEHINDS) | FlagToFlag(chflags, CHF_HASTINT, OBJF_HASTINT) |
           FlagToFlag(chflags, CHF_HASLIGHT, OBJF_HASLIGHT) |
           // following flags are inverse
           FlagToNoFlag(chflags, CHF_NOLIGHTING, OBJF_USEREGIONTINTS) |
           FlagToNoFlag(chflags, CHF_MANUALSCALING, OBJF_USEROOMSCALING) |
           FlagToNoFlag(chflags, CHF_NOBLOCKING, OBJF_SOLID);
}

// Character event indexes
enum CharacterEventID
{
    // an interaction with any cursor mode that normally has a event
    kCharacterEvent_AnyClick,
    // a view frame triggering a custom event
    kCharacterEvent_OnFrameEvent,
};

// A character as the game file defines it. The engine's running character (Character, Engine/game/world) starts
// as a copy of it and adds the play state.
struct CharacterDef
{
    int NormalView = 0;
    int SpeechView = 0;
    int View = 0;
    int Room = 0;
    int X = 0;
    int Y = 0;
    int Flags = 0;    // CHF_* flags
    int IdleView = 0; // idle view number
    // delay in seconds to wait before switching to idle animation (0 = continuous idling)
    int16_t IdleDelay = 0;
    int16_t Opacity = 255;
    int16_t Baseline = -1;
    int SpeechColor = 0;
    int ThinkView = 0;
    int16_t BlinkView = 0;
    int16_t WalkSpeedY = 0;
    int16_t SpeechAnimationDelay = 0;
    int16_t IdleAnimationDelay = 0; // idle animation delay
    // relative offset of the blocking rect
    int16_t BlockingX = 0;
    int16_t BlockingY = 0;
    int16_t BlockingWidth = 0;
    int16_t BlockingHeight = 0;
    int ID = 0; // this character's numeric ID
    int16_t WalkSpeedX = 0;
    int16_t AnimationSpeed = 0;
    // Quantities of inventory items, by item ID;
    // these may be read from the game data, in case it sets up character start with certain items
    std::unordered_map<int, int> InventoryQuantity;
    AGS::Common::String ScriptName = {}; // script name
    AGS::Common::String Name = {};       // regular name (aka description)
    // Interaction events (cursor-based)
    AGS::Common::ScriptEventHandlers Interactions = {};
    // Graphic options, read from the game file's extensions ("ext_ags399", "v400_charopts2")
    AGS::Common::BlendMode BlendMode = AGS::Common::kBlend_Normal;
    Pointf GraphicAnchor = Pointf(0.5f, 1.f); // graphic anchor (relative alignment); default: middle-bottom
    Point GraphicOffset;                      // fixed sprite offset (translation)

    // Gets default sprite anchor for the character
    static Pointf GetDefaultSpriteAnchor() { return Pointf(0.5f, 1.f); /* middle-bottom */ }
    // Gets a events schema corresponding to this object's type
    static const AGS::Common::ScriptEventSchema &GetEventSchema() { return CharacterDef::_eventSchema; }

    // Tells if the "enabled" flag is set
    inline bool is_enabled() const { return (Flags & CHF_ENABLED) != 0; }
    // Tells if the "visible" flag is set
    inline bool is_visible() const { return (Flags & CHF_VISIBLE) != 0; }
    // Tells if the character is actually meant to be displayed on screen;
    // this combines both "enabled" and "visible" factors.
    inline bool is_displayed() const { return is_enabled() && is_visible(); }
    inline bool is_view_locked() const { return (Flags & CHF_FIXVIEW) != 0; }
    // Returns effective x/y walkspeeds for this character
    void get_effective_walkspeeds(int &walk_speed_x, int &walk_speed_y) const
    {
        walk_speed_x = WalkSpeedX;
        walk_speed_y = ((WalkSpeedY == UNIFORM_WALK_SPEED) ? WalkSpeedX : WalkSpeedY);
    }

    // Is moving *and* playing walking animation
    // FIXME: would be proper to also test is_moving() in is_moving_walkanim()
    // and in is_moving_no_anim(), but it breaks animation logic;
    // some further adjustments would be required in the engine code.
    inline bool is_moving_walkanim() const { return (Flags & CHF_MOVENOTWALK) == 0; }
    // Is moving, but *not* playing walking animation
    inline bool is_moving_no_anim() const { return (Flags & CHF_MOVENOTWALK) != 0; }
    inline bool has_explicit_light() const { return (Flags & CHF_HASLIGHT) != 0; }
    inline bool has_explicit_tint() const { return (Flags & CHF_HASTINT) != 0; }
    inline void set_enabled(bool on) { Flags = (Flags & ~CHF_ENABLED) | (CHF_ENABLED * on); }
    inline void set_visible(bool on) { Flags = (Flags & ~CHF_VISIBLE) | (CHF_VISIBLE * on); }

    // Gets if character follows another, while being drawn behind
    inline bool get_follow_sort_behind() const { return (Flags & CHF_BEHINDSHEPHERD) != 0; }
    // Sets "following sort behind" flag
    void set_following_sortbehind(bool sort_behind = false)
    {
        Flags = (Flags & ~CHF_BEHINDSHEPHERD) | (CHF_BEHINDSHEPHERD * sort_behind);
    }

    bool has_any_of_item(int item_id) const
    {
        return InventoryQuantity.count(item_id) > 0 && InventoryQuantity.at(item_id) > 0;
    }

    int get_item_quantity(int item_id) const
    {
        return InventoryQuantity.count(item_id) > 0 ? InventoryQuantity.at(item_id) : 0;
    }

    void set_item_quantity(int item_id, int quant);

    CharacterDef() = default;
    CharacterDef(const CharacterDef &) = default;
    CharacterDef(CharacterDef &&) = default;

    CharacterDef &operator=(const CharacterDef &) = default;
    CharacterDef &operator=(CharacterDef &&) = default;

    // Provides a script events table
    const AGS::Common::ScriptEventTable &GetEvents() const { return _events; }
    AGS::Common::ScriptEventTable &GetEvents() { return _events; }

    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

    void ReadFromFile(AGS::Common::Stream *in, GameDataVersion data_ver);
    void WriteToFile(AGS::Common::Stream *out) const;

private:
    // Script events schema
    static AGS::Common::ScriptEventSchema _eventSchema;
    // Common events
    AGS::Common::ScriptEventTable _events = AGS::Common::ScriptEventTable(&CharacterDef::_eventSchema);
};
