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
#include "ac/global_display.h"

#include <cstdarg>
#include <cstdio>

#include "ac/character.h"
#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_screen.h"
#include "ac/global_translation.h"
#include "ac/overlay.h"
#include "ac/runtime_defines.h"
#include "ac/speech.h"
#include "ac/string.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "game/roomstruct.h"
#include "main/game_run.h"

using namespace AGS::Common;

extern RoomStruct thisroom;
extern GameSetupStruct game;

void DisplayAtYImpl(int ypos, const char *text, const TopBarSettings *topbar, bool as_speech);

void Display(AGS_FORMAT_STRING_ARG const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    String text = String::FromFormatV(get_translation(format), ap);
    va_end(ap);
    DisplayAtY(-1, text.GetCStr());
}

void DisplaySimple(const char *text)
{
    DisplayAtY(-1, text);
}

void DisplayMB(const char *text)
{
    DisplayAtYImpl(-1, text, nullptr, false);
}

void DisplayTopBar(int ypos, int text_color, int back_color, const char *title, const char *text)
{
    // FIXME: refactor source_text_length and get rid of this ugly hack!
    const int real_text_sourcelen = source_text_length;
    source_text_length = real_text_sourcelen;

    if (ypos > 0)
        play.top_bar_ypos = ypos;
    if (text_color > 0)
        play.top_bar_textcolor = text_color;
    if (back_color > 0)
        play.top_bar_backcolor = back_color;

    int font;
    // they want to customize the font
    if (play.top_bar_font >= 0)
        font = play.top_bar_font;
    else
        font = FONT_NORMAL;
    int height = get_font_height_outlined(font) + play.top_bar_borderwidth * 2 + 1;

    const TopBarSettings topbar(get_translation(title), FONT_NORMAL, height);

    // DisplaySpeech normally sets this up, but since we're not going via it...
    if (play.speech_skip_style & SKIP_AUTOTIMER)
        play.messagetime = GetTextDisplayTime(text);

    DisplayAtYImpl(play.top_bar_ypos, text, &topbar, game.options[OPT_ALWAYSSPCH] != 0);
}

void DisplayAt(int x, int y, int width, const char *text)
{
    if (play.screen_is_faded_out)
        debug_script_warn("Blocking Display call during fade-out.");

    if (width < 1)
        width = play.GetUIViewport().GetWidth() / 2;
    if (x < 0)
        x = play.GetUIViewport().GetWidth() / 2 - width / 2;
    display_at(x, y, width, text, nullptr);
}

void DisplayAtYImpl(int ypos, const char *text, const TopBarSettings *topbar, bool as_speech)
{
    const Rect &ui_view = play.GetUIViewport();
    if ((ypos < -1) || (ypos >= ui_view.GetHeight()))
        quitprintf("!DisplayAtY: invalid Y co-ordinate supplied (used: %d; valid: 0..%d)", ypos, ui_view.GetHeight());
    if (play.screen_is_faded_out)
        debug_script_warn("Blocking Display call during fade-out.");

    // Display("") ... a bit of a stupid thing to do, so ignore it
    if (text[0] == 0)
        return;

    if (as_speech)
    {
        // Message displayed as speech, using current speech style
        DisplaySpeechAt(-1, ypos, -1, game.playercharacter, text);
    }
    else
    {
        // Normal "Display" in text box
        display_at(-1, ypos, ui_view.GetWidth() / 2 + ui_view.GetWidth() / 4, get_translation(text), topbar);
    }
}

void DisplayAtY(int ypos, const char *text)
{
    DisplayAtYImpl(ypos, text, nullptr, game.options[OPT_ALWAYSSPCH] != 0);
}

void DisplaySpeechAt(int x, int y, int width, int char_id, const char *text)
{
    display_speech(get_translation(text), char_id, x, y, width, true /*not auto-pos*/, false /* not thought */);
}

// [DEPRECATED] left only for use in Display, replace/merge with modern function
static int CreateTextOverlay(int x, int y, int width, int font, int text_color, const char *text, int over_type,
                             DisplayTextStyle style, int speech_for_char)
{
    // allow DisplaySpeechBackground to be shrunk
    DisplayTextShrink allow_shrink = (speech_for_char >= 0) ? kDisplayTextShrink_Left : kDisplayTextShrink_None;
    auto *over = Overlay_CreateTextCore(false, x, y, width, font, text_color, text, over_type, style, allow_shrink,
                                        speech_for_char);
    return over ? over->GetID() : 0;
}

// [DEPRECATED] but still used by Character_SayBackground, might merge since there are no other instances
int DisplaySpeechBackground(int char_id, const char *text)
{
    // remove any previous background speech for this character
    // TODO: have a map character -> bg speech over?
    const auto &overs = get_overlays();
    for (size_t i = 0; i < overs.size(); ++i)
    {
        if (overs[i].GetCharacterRef() == char_id)
        {
            remove_screen_overlay(i);
            break;
        }
    }

    int over_id =
        CreateTextOverlay(0, 0, play.GetUIViewport().GetWidth() / 2, FONT_SPEECH, game.chars[char_id].talkcolor,
                          get_translation(text), OVER_CUSTOM, kDisplayTextStyle_Overchar, char_id);

    auto *over = get_overlay(over_id);
    over->SetAsSpeech(char_id, GetTextDisplayTime(text, 1));
    return over_id;
}
