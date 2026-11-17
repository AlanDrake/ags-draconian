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
// The play-state fields that a character and a room object both have, with one meaning, type and starting value:
// declared once, and a base of both state structs, CharacterState and RoomObjectState (RUNTIME-SPLIT.md 7). Plain
// data: the logic that uses them is still each type's own (PLAN.md 5.10). Each part has a field table
// (util/field_table.h; the tables are in object.cpp) and is one row of the table of a struct that derives from it.
//
//=============================================================================
#pragma once

#include <cstdint>

#include "game/runtime_defines.h"
#include "util/geometry.h"

namespace AGS
{
namespace Common
{
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

// How the entity is drawn: its tint or light, scaling, rotation, shader and the size of its sprite
struct GraphicState
{
    int16_t TintRed = 0;
    int16_t TintGreen = 0;
    int16_t TintBlue = 0;
    int16_t TintLevel = 0;             // script: TintSaturation
    int16_t TintLight = 0;             // script: TintLuminance, and LightLevel when the entity has a light
    int16_t Zoom = 100;                // zoom level, either manual or from the current area; script: Scaling
    float Rotation = 0.f;              // script: GraphicRotation
    Pointf Pivot = Pointf(0.5f, 0.5f); // relative rotation pivot; default: center
    Point PivotOffset;                 // relative rotation pivot offset (in pixels)
    int ShaderID = 0;
    int ShaderHandle = 0; // script shader handle
    int SpriteWidth = 0;  // the current sprite's size, as the update last found it
    int SpriteHeight = 0;
    int16_t Width = 0; // the size the sprite is drawn at, with the scaling
    int16_t Height = 0;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<GraphicState> Fields();
};

// Where the entity is in a view animation, and how the animation runs
struct ViewAnimState
{
    uint16_t Loop = 0;
    uint16_t Frame = 0;
    // Game loops left before the next frame of an animation. A room object's stays within 16 bits, as the legacy
    // save format holds it (PLAN.md 5.12): the code that sets it narrows the value.
    int AnimWait = 0;
    ViewAnimateParams Anim;

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<ViewAnimState> Fields();
};

// The entity's move along a path
struct MoveState
{
    // No move list: what an entity starts with and a finished move leaves in MoveListID. A valid index is above it
    static constexpr int16_t kNoMoveList = 0;

    // Which move list the entity is running: its index in the game's move lists (get_movelist), always above
    // kNoMoveList. Ask IsMoving(); never test it for non-zero. A save holds the raw number (see the save readers).
    int16_t MoveListID = kNoMoveList;
    int MoveListHandle = 0; // handle to the script movelist

    // Tells whether the entity is running a move list: moving along a path, or (a character) turning between its
    // segments
    bool IsMoving() const { return MoveListID > kNoMoveList; }

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<MoveState> Fields();
};

// How the sounds linked to the frames of the entity's animation play
struct FrameSoundState
{
    int AnimationVolume = 100; // default animation sounds volume (relative factor)
    int AudioPanning = 0;      // animation sounds panning (-100 to +100)
    int AudioSpeed = 1000;     // animation sounds speed, ms per second

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<FrameSoundState> Fields();
};
