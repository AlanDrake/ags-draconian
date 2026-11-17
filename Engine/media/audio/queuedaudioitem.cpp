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

#include "media/audio/queuedaudioitem.h"

#include "game/common_defines.h"
#include "util/field_table.h"
#include "util/stream.h"

using namespace AGS::Common;

namespace
{

constexpr FieldRow<QueuedAudioItem> kQueuedAudioItemFields[] = {
    Field<&QueuedAudioItem::audioClipIndex>("audioClipIndex"),
    Field<&QueuedAudioItem::priority>("priority"),
    Field<&QueuedAudioItem::repeat>("repeat"),
    // the clip loaded ahead of its turn, an owning pointer: a save does not hold it, a restore drops it (RestoreAudio)
    OpaqueField<&QueuedAudioItem::cachedClip>("cachedClip"),
};
// The rows are not counted (FieldsCoverStruct): MSVC does not initialize the std::unique_ptr member from the value that
// converts to any type, with which the count tries the struct's aggregate initialization, so the count finds 3 members

} // namespace

FieldTable<QueuedAudioItem> QueuedAudioItem::Fields()
{
    return kQueuedAudioItemFields;
}

void QueuedAudioItem::ReadFromSavegame(Stream *in)
{
    audioClipIndex = in->ReadInt16();
    priority = in->ReadInt16();
    repeat = in->ReadBool();
    in->ReadInt32(); // reserved
}

void QueuedAudioItem::WriteToSavegame(Stream *out) const
{
    out->WriteInt16(audioClipIndex);
    out->WriteInt16(priority);
    out->WriteBool(repeat);
    out->WriteInt32(0); // reserved
}
