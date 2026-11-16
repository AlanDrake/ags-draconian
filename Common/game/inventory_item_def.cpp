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
#include "game/inventory_item_def.h"

#include "game/common_defines.h"
#include "util/stream.h"

using namespace AGS::Common;

/* static */ ScriptEventSchema InventoryItemDef::_eventSchema = {{{"OnAnyClick", kInventoryEvent_AnyClick}}};

void InventoryItemDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[0];
    new_interactions[STD_MODE_HAND] = old_interactions[1];
    new_interactions[STD_MODE_TALK] = old_interactions[2];
    new_interactions[STD_MODE_USE] = old_interactions[3];
    // "Other click" event handler is assigned to any other standard interaction
    new_interactions[STD_MODE_PICKUP] = old_interactions[4];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[4];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[4];

    interactions.SetScriptModule(_events.GetScriptModule());
    interactions.SetHandlers(std::move(new_interactions));
}

void InventoryItemDef::ReadFromFile(Stream *in, GameDataVersion /*game_ver*/)
{
    name.ReadCount(in, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    in->Seek(3); // alignment padding to int32
    pic = in->ReadInt32();
    cursorPic = in->ReadInt32();
    hotx = in->ReadInt32();
    hoty = in->ReadInt32();
    hotAlign = static_cast<FrameAlignment>(in->ReadInt32()); // since 3.6.3.14
    in->Seek(sizeof(int32_t) * 4);                           // reserved
    flags = in->ReadInt8();
    in->Seek(3); // alignment padding to int32
}

void InventoryItemDef::WriteToFile(Stream *out) const
{
    name.WriteCount(out, LEGACY_MAX_INVENTORY_NAME_LENGTH);
    out->WriteByteCount(0, 3); // alignment padding to int32
    out->WriteInt32(pic);
    out->WriteInt32(cursorPic);
    out->WriteInt32(hotx);
    out->WriteInt32(hoty);
    out->WriteInt32(hotAlign);                   // since 3.6.3.14
    out->WriteByteCount(0, sizeof(int32_t) * 4); // reserved
    out->WriteInt8(flags);
    out->WriteByteCount(0, 3); // alignment padding to int32
}
