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
// Runtime room object definition.
//
//=============================================================================
#pragma once

#include "game/common_defines.h"
#include "game/room_def.h"
#include "game/runtime_defines.h"
#include "gfx/gfx_def.h"
#include "gfx/graphicspace.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{
class Stream;
}
} // namespace AGS

class Game;

// RoomObject's legacy animation constants
#define LEGACY_OBJANIM_ONCE      (LEGACY_ANIM_ONCE + 1)
#define LEGACY_OBJANIM_REPEAT    (LEGACY_ANIM_REPEAT + 1)
#define LEGACY_OBJANIM_ONCERESET (LEGACY_ANIM_ONCERESET + 1)
#define LEGACY_OBJANIM_BACKWARDS 10


// The play state of a room object: what exists only while the game runs. RoomObject holds it with its definition.
struct RoomObjectState
{
    static constexpr uint16_t NoView = UINT16_MAX;

    int16_t TintRed = 0, TintGreen = 0;     // specific object tint
    int16_t TintBlue = 0, TintLevel = 0;    // script: TintSaturation
    int16_t TintLight = 0;                  // script: TintLuminance, and LightLevel when the object has a light
    int16_t Zoom = 100;                     // zoom level, either manual or from the current area; script: Scaling
    Pointf Pivot                            // relative rotation pivot
        = Pointf(0.5f, 0.5f);               // default: center
    Point PivotOffset;                      // relative rotation pivot offset (in pixels)
    int SpriteWidth = 0, SpriteHeight = 0;  // last used sprite's size
    int FrameOffsetX = 0, FrameOffsetY = 0; // frame offsets (when using a view)
    int16_t Width = 0, Height = 0;          // width/height based on a scaled sprite
    uint16_t View = NoView;                 // only used to track animation - 'Sprite' holds the current sprite
    uint16_t Loop = 0, Frame = 0;
    int16_t AnimWait = 0, MoveListID = -1;
    ViewAnimateParams Anim;
    int AnimationVolume = 100; // default animation sounds volume (relative factor)
    int AudioPanning = 0;      // animation sounds panning (-100 to +100)
    int AudioSpeed = 1000;     // animation sounds speed, ms per second
    int ShaderID = 0;
    int ShaderHandle = 0;   // script shader handle
    float Rotation = 0.f;   // script: GraphicRotation
    int MoveListHandle = 0; // handle to the script movelist
};

// An object of a room, as the game changes it: its definition's fields, which start as a copy of what the room
// file says, and its play state. The objects of a room are in the room's state (RoomState), by object id, and are
// kept between visits to the room. The definition's Sprite (0 to 65535), Baseline and the four values of
// BlockingRect stay within 16 bits while the game runs: the legacy save format holds them so (PLAN.md 5.12).
class RoomObject : public AGS::Common::RoomObjectDef, public RoomObjectState
{
public:
    RoomObject() = default;
    // Starts an object from its definition
    explicit RoomObject(const AGS::Common::RoomObjectDef &def) : RoomObjectDef(def) {}

    // Takes again from the definition what a room state does not keep between visits, and so what a save does not
    // hold: the id, the script name, the room number, the events, the interactions and the custom properties as the
    // room file gives them. The rest of the definition's fields and the play state stay as the game left them.
    void RebindDefinition(const AGS::Common::RoomObjectDef &def);

    int get_width() const;
    int get_height() const;
    int get_baseline() const;

    // Tells if the "enabled" flag is set
    inline bool is_enabled() const { return (Flags & OBJF_ENABLED) != 0; }
    // Tells if the "visible" flag is set
    inline bool is_visible() const { return (Flags & OBJF_VISIBLE) != 0; }
    // Tells if the object is actually meant to be displayed on screen;
    // this combines both "enabled" and "visible" factors.
    inline bool is_displayed() const { return is_enabled() && is_visible(); }
    inline bool has_explicit_light() const { return (Flags & OBJF_HASLIGHT) != 0; }
    inline bool has_explicit_tint() const { return (Flags & OBJF_HASTINT) != 0; }
    inline bool is_moving() const { return MoveListID > 0; }
    inline bool is_animating() const { return Anim.IsValid(); }
    inline int get_anim_repeat() const { return Anim.IsRepeating(); }
    inline bool get_anim_forwards() const { return Anim.IsForward(); }
    inline int get_anim_delay() const { return Anim.Delay; }
    inline void set_enabled(bool on) { Flags = (Flags & ~OBJF_ENABLED) | (OBJF_ENABLED * on); }
    inline void set_visible(bool on) { Flags = (Flags & ~OBJF_VISIBLE) | (OBJF_VISIBLE * on); }
    inline void set_animating(AnimFlowStyle repeat, AnimFlowDirection dir, int delay, int anim_volume = 100)
    {
        Anim = ViewAnimateParams(repeat, dir, delay, anim_volume);
    }

    inline const AGS::Common::GraphicSpace &GetGraphicSpace() const { return _graphicSpace; }
    void UpdateGraphicSpace();

    void UpdateCyclingView(Game &game, int ref_id);
    void OnStopMoving();
    // Calculate wanted frame sound volume based on multiple factors
    int GetFrameSoundVolume() const;
    // Process the current animation frame for the room object:
    // play linked sounds, and so forth.
    void CheckViewFrame();
    // Run the arbitrary frame's event if one is associated and character has a handler
    bool RunFrameEvent(int view, int loop, int frame);

    void ReadFromSavegame(AGS::Common::Stream *in, int cmp_ver);
    void WriteToSavegame(AGS::Common::Stream *out) const;

private:
    AGS::Common::GraphicSpace _graphicSpace;
};
