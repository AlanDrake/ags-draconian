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
#include "game/mouse_cursor.h"

#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

void MouseCursor::ReadFromFile(Stream *in)
{
    pic = in->ReadInt32();
    hotx = in->ReadInt16();
    hoty = in->ReadInt16();
    view = in->ReadInt16();
    name.ReadCount(in, LEGACY_MAX_CURSOR_NAME_LENGTH);
    flags = in->ReadInt8();
    in->Seek(3); // alignment padding to int32
}

void MouseCursor::WriteToFile(Stream *out)
{
    out->WriteInt32(pic);
    out->WriteInt16(hotx);
    out->WriteInt16(hoty);
    out->WriteInt16(view);
    name.WriteCount(out, LEGACY_MAX_CURSOR_NAME_LENGTH);
    out->WriteInt8(flags);
    out->WriteByteCount(0, 3); // alignment padding to int32
}
