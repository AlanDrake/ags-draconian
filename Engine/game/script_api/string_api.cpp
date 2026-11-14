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
// Script bindings registered by RegisterStringAPI().
//
//=============================================================================
#include <allegro.h>

#include "game/lib/string.h"
#include "game/script_api/bindings.h"
#include "game/translation.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// const char* (const char *texx, ...)
const char *ScPl_String_Format(const char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    return CreateNewScriptString(scsf_buffer);
}


void RegisterStringAPI()
{
    ScFnRegister string_api[] = {
        ScStatic<&String_IsNullOrEmpty>("String::IsNullOrEmpty^1"),
        ScStaticFormat<static_cast<const char *(*)(const char *)>(&CreateNewScriptString)>("String::Format^101",
                                                                                           ScPl_String_Format),
        ScStatic<&String_Join>("String::Join^2"),

        ScMethod<&String_Append>("String::Append^1"),
        ScMethod<&String_AppendChar>("String::AppendChar^1"),
        ScMethod<&String_CompareTo>("String::CompareTo^2"),
        ScMethod<&String_IndexOf1>("String::Contains^1"),
        ScMethod<&String_Copy>("String::Copy^0"),
        ScMethod<&String_EndsWith, ScBoolArg<1>>("String::EndsWith^2"),
        ScMethod<&String_IndexOf1>("String::IndexOf^1"),
        ScMethod<&String_IndexOf>("String::IndexOf^4"),
        ScMethod<&String_LowerCase>("String::LowerCase^0"),
        ScMethod<&String_Replace, ScBoolArg<2>>("String::Replace^3"),
        ScMethod<&String_ReplaceCharAt>("String::ReplaceCharAt^2"),
        ScMethod<&String_Reverse>("String::Reverse^0"),
        ScMethod<&String_StartsWith, ScBoolArg<1>>("String::StartsWith^2"),
        ScMethod<&String_Substring>("String::Substring^2"),
        ScMethod<&String_Truncate>("String::Truncate^1"),
        ScMethod<&String_Trim>("String::Trim^0"),
        ScMethod<&String_UpperCase>("String::UpperCase^0"),
        ScMethod<&String_Split, ScManagedBy<globalDynamicArray>>("String::Split^2"),
        ScMethod<&String_AsFloat>("String::get_AsFloat"),
        ScMethod<&String_AsInt>("String::get_AsInt"),
        ScMethod<&String_GetChars>("String::geti_Chars"),
        ScMethod<&String_GetLength>("String::get_Length"),
    };

    ccAddExternalFunctions(string_api);
}
