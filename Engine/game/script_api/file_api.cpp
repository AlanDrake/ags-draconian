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
// Script bindings registered by RegisterFileAPI().
//
//=============================================================================
#include "game/file.h"
#include "game/script_api/bindings.h"
#include "platform/base/agsplatformdriver.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterFileAPI()
{
    ScFnRegister file_api[] = {
        ScStatic<&File_Copy>("File::Copy^2"),
        ScStatic<&File_Delete>("File::Delete^1"),
        ScStatic<&File_Exists>("File::Exists^1"),
        ScStatic<&File_GetFileTime>("File::GetFileTime^1"),
        ScStatic<&File_GetFiles, ScManagedBy<globalDynamicArray>>("File::GetFiles^3"),
        ScStatic<&File_Rename>("File::Rename^2"),
        ScStatic<&sc_OpenFile, ScResultAs<sc_File>>("File::Open^2"),
        ScStatic<&File_ResolvePath>("File::ResolvePath^1"),

        ScMethod<&File_Close>("File::Close^0"),
        ScMethod<&File_ReadInt>("File::ReadInt^0"),
        ScMethod<&File_ReadFloat>("File::ReadFloat^0"),
        ScMethod<&File_ReadRawChar>("File::ReadRawChar^0"),
        ScMethod<&File_ReadRawInt>("File::ReadRawInt^0"),
        ScMethod<&File_ReadRawFloat>("File::ReadRawFloat^0"),
        ScMethod<&File_ReadRawLine>("File::ReadRawLine^1"),
        ScMethod<&File_ReadRawLineBack>("File::ReadRawLineBack^0"),
        ScMethod<&File_ReadString>("File::ReadString^1"),
        ScMethod<&File_ReadStringBack>("File::ReadStringBack^0"),
        ScMethod<&File_WriteInt>("File::WriteInt^1"),
        ScMethod<&File_WriteFloat, ScUndefinedResult>("File::WriteFloat^1"),
        ScMethod<&File_WriteRawChar>("File::WriteRawChar^1"),
        ScMethod<&File_WriteRawFloat, ScUndefinedResult>("File::WriteRawFloat^1"),
        ScMethod<&File_WriteRawInt>("File::WriteRawInt^1"),
        ScMethod<&File_WriteRawLine>("File::WriteRawLine^1"),
        ScMethod<&File_WriteString>("File::WriteString^1"),
        ScMethod<&File_ReadRawBytes>("File::ReadRawBytes^3"),
        ScMethod<&File_WriteRawBytes>("File::WriteRawBytes^3"),
        ScMethod<&File_Seek>("File::Seek^2"),
        ScMethod<&File_GetEOF>("File::get_EOF"),
        ScMethod<&File_GetError>("File::get_Error"),
        ScMethod<&File_GetPosition>("File::get_Position"),
        ScMethod<&File_GetPath>("File::get_Path"),
    };

    ccAddExternalFunctions(file_api);
}
