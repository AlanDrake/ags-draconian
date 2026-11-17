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

#include "game/game_version.h"
#include "game/scripteventtable.h"
#include "util/geometry.h"
#include "util/stream.h"
#include "util/string.h"
#include "util/string_types.h"

namespace AGS
{
namespace Common
{
template <typename T>
struct FieldTable;
} // namespace Common
} // namespace AGS

#define IFLG_STARTWITH                   1
#define LEGACY_MAX_INVENTORY_NAME_LENGTH 25

// Inventory item event indexes
enum InventoryEventID
{
    // an interaction with any cursor mode that normally has a event
    kInventoryEvent_AnyClick = 0
};

struct InventoryItemDef
{
    AGS::Common::String Name = {};
    int Sprite = 0;                                               // script: Graphic
    int CursorSprite = 0, CursorHotspotX = 0, CursorHotspotY = 0; // script: CursorGraphic
    FrameAlignment CursorHotspotAlignment = kAlignNone;
    uint8_t Flags = 0u; // IFLG_STARTWITH
    // Interaction events (cursor-based)
    AGS::Common::ScriptEventHandlers Interactions = {};
    // Common events
    AGS::Common::ScriptEventTable Events = AGS::Common::ScriptEventTable(&InventoryItemDef::_eventSchema);

    // One row per field above (util/field_table.h); the struct stays an aggregate so that they can be counted
    static AGS::Common::FieldTable<InventoryItemDef> Fields();

    // Gets a events schema corresponding to this object's type
    static const AGS::Common::ScriptEventSchema &GetEventSchema() { return InventoryItemDef::_eventSchema; }

    // Provides a script events table
    const AGS::Common::ScriptEventTable &GetEvents() const { return Events; }
    AGS::Common::ScriptEventTable &GetEvents() { return Events; }
    // Remaps old-format interaction list into new event table
    void RemapOldInteractions();

    void ReadFromFile(AGS::Common::Stream *in, GameDataVersion game_ver);
    void WriteToFile(AGS::Common::Stream *out) const;

private:
    // Script events schema
    static AGS::Common::ScriptEventSchema _eventSchema;
};
