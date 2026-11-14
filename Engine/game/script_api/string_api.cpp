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

#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/translation.h"
#include "game/dynobj/script_string.h"
#include "game/lib/string.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// int (const char *thisString)
RuntimeScriptValue Sc_String_IsNullOrEmpty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(String_IsNullOrEmpty, const char);
}

// const char * (void* arrobj, const char *separator)
RuntimeScriptValue Sc_String_Join(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ2(const char, myScriptStringImpl, String_Join, void, const char);
}

// const char* (const char *thisString, const char *extrabit)
RuntimeScriptValue Sc_String_Append(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(const char, const char, myScriptStringImpl, String_Append, const char);
}

// const char* (const char *thisString, char extraOne)
RuntimeScriptValue Sc_String_AppendChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_AppendChar);
}

// int (const char *thisString, const char *otherString, bool caseSensitive)
RuntimeScriptValue Sc_String_CompareTo(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PINT(const char, String_CompareTo, const char);
}

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_String_IndexOf1(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(const char, String_IndexOf1, const char);
}

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_String_IndexOf(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL_PINT2(const char, String_IndexOf, const char);
}

// const char* (const char *srcString)
RuntimeScriptValue Sc_String_Copy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Copy);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_EndsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_EndsWith, const char);
}

// const char* (const char *texx, ...)
RuntimeScriptValue Sc_String_Format(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(String_Format, 1);
    return RuntimeScriptValue().SetScriptObject((void *)CreateNewScriptString(scsf_buffer), &myScriptStringImpl);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_LowerCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_LowerCase);
}

// const char* (const char *thisString, const char *lookForText, const char *replaceWithText, bool caseSensitive)
RuntimeScriptValue Sc_String_Replace(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ2_PBOOL(const char, const char, myScriptStringImpl, String_Replace, const char, const char);
}

// const char* (const char *thisString, int index, char newChar)
RuntimeScriptValue Sc_String_ReplaceCharAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_ReplaceCharAt);
}

RuntimeScriptValue Sc_String_Reverse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Reverse);
}

// int (const char *thisString, const char *checkForString, bool caseSensitive)
RuntimeScriptValue Sc_String_StartsWith(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PBOOL(const char, String_StartsWith, const char);
}

// const char* (const char *thisString, int index, int length)
RuntimeScriptValue Sc_String_Substring(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT2(const char, const char, myScriptStringImpl, String_Substring);
}

// const char* (const char *thisString, int length)
RuntimeScriptValue Sc_String_Truncate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(const char, const char, myScriptStringImpl, String_Truncate);
}

RuntimeScriptValue Sc_String_Trim(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_Trim);
}

// const char* (const char *thisString)
RuntimeScriptValue Sc_String_UpperCase(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(const char, const char, myScriptStringImpl, String_UpperCase);
}

// const char* [] (const char *thisString, const char* separator)
RuntimeScriptValue Sc_String_Split(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ_PINT(const char, void, globalDynamicArray, String_Split, const char);
}

// int (char*stino)
RuntimeScriptValue Sc_String_AsInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(const char, String_AsInt);
}

// FLOAT_RETURN_TYPE (const char *theString);
RuntimeScriptValue Sc_String_AsFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(const char, String_AsFloat);
}

// int (const char *texx, int index)
RuntimeScriptValue Sc_String_GetChars(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(const char, String_GetChars);
}

RuntimeScriptValue Sc_String_GetLength(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_SELF(String_GetLength);
    return RuntimeScriptValue().SetInt32(String_GetLength((const char *)self));
}

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
        {"String::IsNullOrEmpty^1", API_FN_PAIR(String_IsNullOrEmpty)},
        {"String::Format^101", Sc_String_Format, ScPl_String_Format},
        {"String::Join^2", API_FN_PAIR(String_Join)},

        {"String::Append^1", API_FN_PAIR(String_Append)},
        {"String::AppendChar^1", API_FN_PAIR(String_AppendChar)},
        {"String::CompareTo^2", API_FN_PAIR(String_CompareTo)},
        {"String::Contains^1", API_FN_PAIR(String_IndexOf1)},
        {"String::Copy^0", API_FN_PAIR(String_Copy)},
        {"String::EndsWith^2", API_FN_PAIR(String_EndsWith)},
        {"String::IndexOf^1", API_FN_PAIR(String_IndexOf1)},
        {"String::IndexOf^4", API_FN_PAIR(String_IndexOf)},
        {"String::LowerCase^0", API_FN_PAIR(String_LowerCase)},
        {"String::Replace^3", API_FN_PAIR(String_Replace)},
        {"String::ReplaceCharAt^2", API_FN_PAIR(String_ReplaceCharAt)},
        {"String::Reverse^0", API_FN_PAIR(String_Reverse)},
        {"String::StartsWith^2", API_FN_PAIR(String_StartsWith)},
        {"String::Substring^2", API_FN_PAIR(String_Substring)},
        {"String::Truncate^1", API_FN_PAIR(String_Truncate)},
        {"String::Trim^0", API_FN_PAIR(String_Trim)},
        {"String::UpperCase^0", API_FN_PAIR(String_UpperCase)},
        {"String::Split^2", API_FN_PAIR(String_Split)},
        {"String::get_AsFloat", API_FN_PAIR(String_AsFloat)},
        {"String::get_AsInt", API_FN_PAIR(String_AsInt)},
        {"String::geti_Chars", API_FN_PAIR(String_GetChars)},
        {"String::get_Length", API_FN_PAIR(String_GetLength)},
    };

    ccAddExternalFunctions(string_api);
}
