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
// The script API's text formatting: ScriptSprintf formats a script text with script values or with a
// plugin's C variadic arguments. The bindings themselves are generated from the signature of the function
// they call by script/script_binder.h; the function types the VM calls are in script/runtimescriptvalue.h.
//
//=============================================================================
#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <vector>

struct RuntimeScriptValue;

// Sprintf that takes either script values or common argument list from plugin.
// Uses EITHER sc_args/sc_argc or varg_ptr as parameter list, whichever is not
// NULL, with varg_ptr having HIGHER priority.
size_t ScriptSprintf(char *buffer, size_t buf_length, const char *format, const RuntimeScriptValue *sc_args,
                     int32_t sc_argc, va_list *varg_ptr);
// Sprintf that takes script values as arguments
inline size_t ScriptSprintf(char *buffer, size_t buf_length, const char *format, const RuntimeScriptValue *args,
                            int32_t argc)
{
    return ScriptSprintf(buffer, buf_length, format, args, argc, nullptr);
}
// Sprintf that takes script values as arguments, and prints into the resizable vector
inline const char *ScriptSprintf(std::vector<char> &buf, const char *format, const RuntimeScriptValue *args,
                                 int32_t argc)
{
    size_t need_len = ScriptSprintf(nullptr, 0u, format, args, argc, nullptr);
    buf.resize(need_len + 1);
    ScriptSprintf(buf.data(), buf.size(), format, args, argc, nullptr);
    return buf.data();
}
// Variadic sprintf (needed, because all arguments are pushed as pointer-sized values).
// Currently used only when plugin calls exported engine function.
inline size_t ScriptVSprintf(char *buffer, size_t buf_length, const char *format, va_list &arg_ptr)
{
    return ScriptSprintf(buffer, buf_length, format, nullptr, 0, &arg_ptr);
}
// Variadic sprintf that prints into the resizable vector
inline const char *ScriptVSprintf(std::vector<char> &buf, const char *format, va_list &arg_ptr)
{
    size_t need_len = ScriptSprintf(nullptr, 0u, format, nullptr, 0, &arg_ptr);
    buf.resize(need_len + 1);
    ScriptSprintf(buf.data(), buf.size(), format, nullptr, 0, &arg_ptr);
    return buf.data();
}

//-----------------------------------------------------------------------------
// Formatting for the plugin half of a binding that takes a format (the ScPl_* functions, C variadic):
// declares scsf_buffer, the formatted text. API_PLUGIN_SCRIPT_SPRINTF translates the format first (the
// caller includes game/translation.h), API_PLUGIN_SCRIPT_SPRINTF_PURE does not.

#define API_PLUGIN_SCRIPT_SPRINTF(FORMAT_STR)                                                \
    va_list args;                                                                            \
    va_start(args, FORMAT_STR);                                                              \
    std::vector<char> ScSfBuffer;                                                            \
    const char *scsf_buffer = ScriptVSprintf(ScSfBuffer, get_translation(FORMAT_STR), args); \
    va_end(args)

#define API_PLUGIN_SCRIPT_SPRINTF_PURE(FORMAT_STR)                          \
    va_list args;                                                           \
    va_start(args, FORMAT_STR);                                             \
    std::vector<char> ScSfBuffer;                                           \
    const char *scsf_buffer = ScriptVSprintf(ScSfBuffer, FORMAT_STR, args); \
    va_end(args)
