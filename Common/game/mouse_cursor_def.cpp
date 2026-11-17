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
#include "game/mouse_cursor_def.h"

#include <type_traits>

#include "util/field_table.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

// The struct gets a field table, so it must stay an aggregate (util/field_table.h)
static_assert(std::is_aggregate_v<MouseCursorDef>, "MouseCursorDef must stay an aggregate");

namespace
{

// The roles are those of the live cursor, the engine's copy of the definition made when the game starts
// (StartFromDefinitions). Saved: the save's "Mouse Cursors" component holds it (each value in 32 bits). FromDefinition:
// a save does not hold it, the game's start gives the definition's; nothing writes it during play.
constexpr FieldRow<MouseCursorDef> kMouseCursorDefFields[] = {
    Field<&MouseCursorDef::Flags>("Flags"),
    Field<&MouseCursorDef::Role>("Role", FieldRole::FromDefinition),
    Field<&MouseCursorDef::Sprite>("Sprite"),
    Field<&MouseCursorDef::HotspotX>("HotspotX"),
    Field<&MouseCursorDef::HotspotY>("HotspotY"),
    Field<&MouseCursorDef::View>("View"),
    Field<&MouseCursorDef::AnimationDelay>("AnimationDelay"),
    Field<&MouseCursorDef::ScriptName>("ScriptName", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kMouseCursorDefFields), "MouseCursorDef: a field has no row in kMouseCursorDefFields");

} // namespace

FieldTable<MouseCursorDef> MouseCursorDef::Fields()
{
    return kMouseCursorDefFields;
}

void MouseCursorDef::ReadFromFile(Stream *in)
{
    Sprite = in->ReadInt32();
    HotspotX = in->ReadInt16();
    HotspotY = in->ReadInt16();
    View = in->ReadInt16();
    ScriptName.ReadCount(in, LEGACY_MAX_CURSOR_NAME_LENGTH);
    Flags = in->ReadInt8();
    in->Seek(3); // alignment padding to int32
}

void MouseCursorDef::WriteToFile(Stream *out)
{
    out->WriteInt32(Sprite);
    out->WriteInt16(HotspotX);
    out->WriteInt16(HotspotY);
    out->WriteInt16(View);
    ScriptName.WriteCount(out, LEGACY_MAX_CURSOR_NAME_LENGTH);
    out->WriteInt8(Flags);
    out->WriteByteCount(0, 3); // alignment padding to int32
}
