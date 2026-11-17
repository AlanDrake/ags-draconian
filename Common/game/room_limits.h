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

// TODO: move the following enums under AGS::Common namespace
// later, when more engine source is put in AGS namespace and
// refactored.

// Room's area mask type
enum RoomAreaMask
{
    kRoomAreaNone = 0,
    kRoomAreaHotspot,
    kRoomAreaWalkBehind,
    kRoomAreaWalkable,
    kRoomAreaRegion,
    kNumRoomAreaTypes,

    kRoomArea_First = kRoomAreaHotspot,
    kRoomArea_Last = kRoomAreaRegion
};

// Max number of room background frames
#define MAX_ROOM_BGFRAMES     256
#define MAX_ROOM_BGFRAMES_321 5

#define MAX_ROOM_HOTSPOTS     50  // v2.62: 20 -> 30; v2.8: -> 50
#define MAX_ROOM_OBJECTS_v300 40  // for some legacy logic support
#define MAX_ROOM_OBJECTS      256 // v3.6.0: 40 -> 256 (now limited by room format)
#define MAX_ROOM_REGIONS      16
#define MAX_WALK_AREAS        16
#define MAX_WALK_BEHINDS      16
