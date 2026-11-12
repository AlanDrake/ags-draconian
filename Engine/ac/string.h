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
// Script String API implementation.
//
//=============================================================================
#pragma once

#include <stdarg.h>

#include "ac/common.h" // quit
#include "ac/dynobj/scriptstring.h"
#include "util/string.h"

// Check that a supplied buffer from a text script function was not null
inline void VALIDATE_STRING(const char *strin)
{
    if (!strin)
        quit("!String argument was null: make sure you pass a valid string as a buffer.");
}

// Tests if a font number is valid, if not then prints a warning and returns a substitution
int ValidateFontNumber(const char *apiname, int font_num);

const char *CreateNewScriptString(const char *text);
inline const char *CreateNewScriptString(const AGS::Common::String &text)
{
    return CreateNewScriptString(text.GetCStr());
}
inline const char *CreateNewScriptString(ScriptString::Buffer &&buf)
{
    return static_cast<const char *>(ScriptString::Create(std::move(buf)).Obj());
}

int String_IsNullOrEmpty(const char *this_string);
const char *String_Copy(const char *src_string);
const char *String_Append(const char *this_string, const char *extra_text);
const char *String_AppendChar(const char *this_string, int extra_char);
const char *String_ReplaceCharAt(const char *this_string, int index, int new_char);
const char *String_Truncate(const char *this_string, int length);
const char *String_Substring(const char *this_string, int index, int length);
int String_CompareTo(const char *this_string, const char *other_string, int compare_style);
int String_StartsWith(const char *this_string, const char *check_for_string, int compare_style);
int String_EndsWith(const char *this_string, const char *check_for_string, int compare_style);
const char *String_Replace(const char *this_string, const char *look_for_text, const char *replace_with_text,
                           int compare_style);
const char *String_LowerCase(const char *this_string);
const char *String_UpperCase(const char *this_string);
int String_GetChars(const char *this_string, int index);
int String_AsInt(const char *str);
float String_AsFloat(const char *str);
int String_IndexOf(const char *this_string, const char *look_for_text, bool case_sensitive, int start_index, int count);
int String_IndexOf2(const char *s1, const char *s2);

//=============================================================================

class SplitLines;
// Break up the text into lines restricted by the given width;
// returns number of lines, or 0 if text cannot be split well to fit in this width.
// Optionally applies text direction rules (apply_direction param) and reverses the lines if necessary,
// otherwise leaves left-to-right always.
size_t break_up_text_into_lines(const char *todis, bool apply_direction, SplitLines &lines, int wii, int fonnt,
                                size_t max_lines = -1);
inline size_t break_up_text_into_lines(const char *todis, SplitLines &lines, int wii, int fonnt, size_t max_lines = -1)
{
    return break_up_text_into_lines(todis, true, lines, wii, fonnt, max_lines);
}
// Tries if the input string contains a voice-over token ("&N"),
// *optionally* fills the voice_num value (if the valid int pointer is passed),
// and returns the pointer to the text portion after the token.
// If returned pointer equals input pointer, that means that there was no token.
// voice_num must be > 0 for a valid token, it's assigned 0 if no token was found,
// or if there have been a parsing error.
const char *parse_voiceover_token(const char *text, int *voice_num);
inline const char *skip_voiceover_token(const char *text)
{
    return parse_voiceover_token(text, nullptr);
}
