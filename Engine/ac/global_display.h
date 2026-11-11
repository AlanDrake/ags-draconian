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
#pragma once

#include "ac/speech.h"

void Display(AGS_FORMAT_STRING_ARG const char *format, ...) AGS_FORMAT_STRING(1); // applies translation
void DisplaySimple(const char *text);                                             // does not apply translation
void DisplayMB(const char *text);                                                 // forces standard Display message box
void DisplayAt(int x, int y, int width, const char *text);
void DisplayAtY(int ypos, const char *text);
void DisplayTopBar(int ypos, int text_color, int back_color, const char *title, const char *text);
void DisplaySpeechAt(int x, int y, int width, int char_id, const char *text);
int DisplaySpeechBackground(int char_id, const char *text); // [DEPRECATED] but still used by Character_SayBackground
