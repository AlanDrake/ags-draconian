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

namespace AGS
{
namespace Common
{
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

// Animation flow mode
// NOTE: the constant values correspond to the script's RepeatStyle
enum AnimFlowStyle
{
    // For non-initialized animation object
    kAnimFlow_None = -1,
    // Animates once and stops at the *last* frame
    kAnimFlow_Once = 0,
    // Animates infinitely, wrapping from the last frame to the first frame
    kAnimFlow_Repeat = 1,
    // Animates once and stops, resetting to the very first frame
    kAnimFlow_OnceAndReset = 2,
    // Animates once in the starting direction, and another in reverse,
    // stops at the first frame after returning back
    kAnimFlow_OnceAndBack = 3,
    // Animates infinitely, changing direction each time when reaching
    // either last or first frame.
    kAnimFlow_RepeatAlternate = 4,

    kAnimFlow_First = kAnimFlow_Once,
    kAnimFlow_Last = kAnimFlow_RepeatAlternate
};

// Animation direction (current direction)
enum AnimFlowDirection
{
    kAnimDirForward = 0,
    kAnimDirBackward = 1
};

// Defines basic animation parameters; where "animation" may be any continuous action.
struct AnimFlowParams
{
    AnimFlowStyle Flow = kAnimFlow_None;
    AnimFlowDirection InitialDirection = kAnimDirForward;
    AnimFlowDirection Direction = kAnimDirForward;

    // One row per field above (util/field_table.h)
    static AGS::Common::FieldTable<AnimFlowParams> Fields();

    AnimFlowParams() = default;
    AnimFlowParams(AnimFlowStyle flow, AnimFlowDirection dir) : Flow(flow), InitialDirection(dir), Direction(dir) {}
    AnimFlowParams(AnimFlowStyle flow, AnimFlowDirection init_dir, AnimFlowDirection cur_dir)
        : Flow(flow),
          InitialDirection(init_dir),
          Direction(cur_dir)
    {
    }

    static AnimFlowParams DefaultOnce() { return AnimFlowParams(kAnimFlow_Once, kAnimDirForward); }

    inline bool IsValid() const { return Flow != kAnimFlow_None; }
    inline bool IsRepeating() const
    {
        switch (Flow)
        {
        case kAnimFlow_Repeat: return true;
        default: return false;
        }
    }
    inline bool IsForward() const { return Direction == kAnimDirForward; }
};

// RunPathParams is an alias used for clarity of purpose
typedef AnimFlowParams RunPathParams;

// View animation parameters;
// define how the individual view-frame animation should be running
//
// TODO: I suggest remaking this into a parent class for objects that
// do view animations, e.g. a ViewBasedObject class, and inherit runtime
// Character, RoomObject and AnimatedButton from this (PLAN.md 5.1a step 2
// declares the shared fields once, 5.10 the logic). When we have a base class, SetFirstAnimFrame() and CycleViewAnim()
// functions naturally become its methods (probably renamed).
struct ViewAnimateParams : public AnimFlowParams
{
    // General frame delay for this animation
    int Delay = 0;
    // Volume of the frame-linked sounds (relative factor)
    int AudioVolume = 100;

    // One row per field above and one for the base (util/field_table.h)
    static AGS::Common::FieldTable<ViewAnimateParams> Fields();

    ViewAnimateParams() = default;
    ViewAnimateParams(AnimFlowStyle flow, AnimFlowDirection dir, int delay, int avolume)
        : AnimFlowParams(flow, dir),
          Delay(delay),
          AudioVolume(avolume)
    {
    }
    ViewAnimateParams(AnimFlowStyle flow, AnimFlowDirection init_dir, AnimFlowDirection cur_dir, int delay, int avolume)
        : AnimFlowParams(flow, init_dir, cur_dir),
          Delay(delay),
          AudioVolume(avolume)
    {
    }
    ViewAnimateParams(const AnimFlowParams &flow_params, int delay, int avolume)
        : AnimFlowParams(flow_params),
          Delay(delay),
          AudioVolume(avolume)
    {
    }
};
