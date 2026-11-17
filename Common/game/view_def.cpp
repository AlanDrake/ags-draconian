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
#include "game/view_def.h"

#include <cstring>
#include <type_traits>

#include "util/field_table.h"
#include "util/stream.h"

using namespace AGS::Common;

// The structs get a field table, so they must stay aggregates (util/field_table.h)
static_assert(std::is_aggregate_v<ViewFrameDef>, "ViewFrameDef must stay an aggregate");
static_assert(std::is_aggregate_v<ViewLoopDef>, "ViewLoopDef must stay an aggregate");
static_assert(std::is_aggregate_v<ViewDef>, "ViewDef must stay an aggregate");

namespace
{

// The roles are those of the live view, the engine's copy of the definition made when the game starts
// (StartFromDefinitions). Saved: the save's "Views" component holds it. FromDefinition: a save does not hold it, the
// game's start gives the definition's; nothing writes it during play.
constexpr FieldRow<ViewFrameDef> kViewFrameDefFields[] = {
    Field<&ViewFrameDef::Sprite>("Sprite"),
    // the offsets and the delay in 16 bits in the save, as in the struct
    Field<&ViewFrameDef::OffsetX>("OffsetX"),
    Field<&ViewFrameDef::OffsetY>("OffsetY"),
    Field<&ViewFrameDef::Delay>("Delay"),
    Field<&ViewFrameDef::Flags>("Flags"),
    Field<&ViewFrameDef::LinkedAudio>("LinkedAudio"),
    // never read from the game file nor written: always zero
    Field<&ViewFrameDef::ReservedForFuture>("ReservedForFuture", FieldRole::FromDefinition),
    Field<&ViewFrameDef::EventName>("EventName"),
};
static_assert(FieldsCoverStruct(kViewFrameDefFields), "ViewFrameDef: a field has no row in kViewFrameDefFields");

// The counts and the lists' lengths are the definition's: a save holds the counts only to compare them with the
// game's (ReadViews, HandleGameContentMismatch), and the saved fields of each frame, which the frame's rows say. AlterFields does not reach the elements of a list (util/field_table.h): a test alters the frames themselves.
constexpr FieldRow<ViewLoopDef> kViewLoopDefFields[] = {
    Field<&ViewLoopDef::FrameCount>("FrameCount", FieldRole::FromDefinition),
    Field<&ViewLoopDef::Flags>("Flags", FieldRole::FromDefinition),
    Field<&ViewLoopDef::Frames>("Frames", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kViewLoopDefFields), "ViewLoopDef: a field has no row in kViewLoopDefFields");

constexpr FieldRow<ViewDef> kViewDefFields[] = {
    Field<&ViewDef::LoopCount>("LoopCount", FieldRole::FromDefinition),
    Field<&ViewDef::Loops>("Loops", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kViewDefFields), "ViewDef: a field has no row in kViewDefFields");

} // namespace

FieldTable<ViewFrameDef> ViewFrameDef::Fields()
{
    return kViewFrameDefFields;
}

FieldTable<ViewLoopDef> ViewLoopDef::Fields()
{
    return kViewLoopDefFields;
}

FieldTable<ViewDef> ViewDef::Fields()
{
    return kViewDefFields;
}

void ViewFrameDef::ReadFromFile(Stream *in)
{
    Sprite = in->ReadInt32();
    OffsetX = in->ReadInt16();
    OffsetY = in->ReadInt16();
    Delay = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    Flags = (SpriteTransformFlags)in->ReadInt32();
    LinkedAudio = in->ReadInt32();
    in->ReadInt32(); // reserved 1
    in->ReadInt32(); // reserved 1
}

bool ViewLoopDef::RunNextLoop() const
{
    return (Flags & LOOPFLAG_RUNNEXTLOOP);
}

void ViewLoopDef::Initialize(int frame_count)
{
    FrameCount = frame_count;
    Flags = 0;
    // an extra frame is allocated to prevent crashes with empty loops
    Frames.resize(FrameCount > 0 ? FrameCount : 1);
}

void ViewLoopDef::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));
    Flags = in->ReadInt32();
    ReadFrames(in);
}

void ViewLoopDef::ReadFrames(Stream *in)
{
    for (int i = 0; i < FrameCount; ++i)
    {
        Frames[i].ReadFromFile(in);
    }
}

void ViewDef::Initialize(int loop_count)
{
    LoopCount = loop_count;
    Loops.resize(LoopCount);
}

void ViewDef::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));

    for (int i = 0; i < LoopCount; i++)
    {
        Loops[i].ReadFromFile(in);
    }
}
