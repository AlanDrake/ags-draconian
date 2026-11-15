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
//
//
//=============================================================================
#pragma once

#include "util/string_types.h"

using AGS::Common::String;
using AGS::Common::StringMap;

void close_translation();
bool init_translation(const String &lang, const String &fallback_lang);
// Sets the game text language (EngineServices::TextLanguage), finds the matching UTF-8 locale for it
// (EngineServices::TextLocaleName) and gives that to the GUI context
void SetGameTextLanguage(const String &language);
// Returns current translation name, or empty string if default translation is used
String get_translation_name();
// Returns translation map for reading only
const StringMap &get_translation_tree();

// Use the provided text as a key and returns the mapped translation.
// Returns the original text back if no matching translation found.
// WARNING: get_translation returns original char* ptr if no translation is found;
// for that reason make sure that you don't pass temporary buffer there, unless
// you use returned value immediately or save it in another buffer.
const char *get_translation(const char *text);
int IsTranslationAvailable();
