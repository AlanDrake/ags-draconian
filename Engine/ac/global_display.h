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

// Shows a message box (or player's speech, if the game has OPT_ALWAYSSPCH set) with a printf-style text;
// the format string is translated, and the result goes through DisplayAtY, which translates it once more.
void Display(AGS_FORMAT_STRING_ARG const char *format, ...) AGS_FORMAT_STRING(1);
// Same as DisplayAtY with the y position chosen automatically, with a ready (script-formatted) text
void DisplaySimple(const char *text);
// Shows a standard message box at the automatic position, never as speech
void DisplayMB(const char *text);
// Shows a message box of the given width at x, y; width < 1 is half of the UI viewport, x < 0 centers the box;
// does not translate the text
void DisplayAt(int x, int y, int width, const char *text);
// Shows a message box (or player's speech, if the game has OPT_ALWAYSSPCH set) at the vertical position ypos,
// -1 is automatic; translates the text. Quits with an error if ypos is out of the UI viewport
void DisplayAtY(int ypos, const char *text);
// Shows a message box with a top bar titled with the translated title; the y position, text color and back
// color of the bar are stored in the game state (play.top_bar_*) and stay for the later calls: a value not
// above 0 keeps the stored one. The text is translated like in DisplayAtY; with OPT_ALWAYSSPCH set it is
// displayed as speech of the player (without the bar).
void DisplayTopBar(int ypos, int text_color, int back_color, const char *title, const char *text);
// Shows the text as speech of the character char_id, at x, y with the given width (translates the text)
void DisplaySpeechAt(int x, int y, int width, int char_id, const char *text);
// Shows the text as the character's background speech and returns the id of the overlay it makes;
// removes the character's earlier background speech first.
// [DEPRECATED] but still used by Character_SayBackground
int DisplaySpeechBackground(int char_id, const char *text);
