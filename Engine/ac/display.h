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
// Functions for displaying various standard textual overlays.
//
//=============================================================================
#pragma once

#include "ac/screenoverlay.h"
#include "gfx/bitmap.h"
#include "gui/guimain.h"
#include "util/string.h"


// The general type and behavior of the displayed text
enum DisplayTextType
{
    kDisplayText_MessageBox,   // a super-blocking message box
    kDisplayText_Speech,       // a blocking character speech
    kDisplayText_NormalOverlay // a custom text overlay
};

// More specific visual look of the displayed text
enum DisplayTextStyle
{
    kDisplayTextStyle_MessageBox, // standard message box
    kDisplayTextStyle_TextWindow, // use text window GUI, if applicable
    kDisplayTextStyle_PlainText,  // plain text without any boxes
    kDisplayTextStyle_Overchar    // display plain text for a character;
                                  // this style have special adjustments
};

// DisplayTextPosition flags define screen text positioning behavior
enum DisplayTextPosition
{
    // display text using explicit position
    kDisplayTextPos_Normal = 0,
    // display text centered on screen along x
    kDisplayTextPos_ScreenCenterX = 0x0001,
    // display text centered on screen along y
    kDisplayTextPos_ScreenCenterY = 0x0002,
    // display text centered on screen along x & y
    kDisplayTextPos_ScreenCenter = (kDisplayTextPos_ScreenCenterX | kDisplayTextPos_ScreenCenterY),
    // display text aligned to a character along x
    kDisplayTextPos_OvercharX = 0x0004,
    // display text aligned to a character along y
    kDisplayTextPos_OvercharY = 0x0008,
    // display text aligned to a character along x & y
    kDisplayTextPos_Overchar = (kDisplayTextPos_OvercharX | kDisplayTextPos_OvercharY),
    // clamp text horizontal placement to the game screen
    kDisplayTextPos_ClampToScreenWidth = 0x0010,
    // clamp text vertical placement to the game screen
    kDisplayTextPos_ClampToScreenHeight = 0x0020,
};

// Whether displayed text is allowed to be shrunk
enum DisplayTextShrink
{
    kDisplayTextShrink_None,
    kDisplayTextShrink_Left,
    kDisplayTextShrink_Right
};

// Various options for displaying a text on screen
struct DisplayTextLooks
{
    DisplayTextStyle Style = kDisplayTextStyle_TextWindow;
    // Center text on screen
    DisplayTextPosition Position = kDisplayTextPos_Normal;
    // Allow to make the resulting overlay smaller than requested
    DisplayTextShrink AllowShrink = kDisplayTextShrink_None;
    // Is this a character's thought; FIXME: merge with Style?
    bool AsThought = false;
    // Preserve explicitly specified text color for custom textual overlays.
    bool PreserveTextColor = false;

    DisplayTextLooks() = default;
    DisplayTextLooks(DisplayTextStyle style) : Style(style) {}
    DisplayTextLooks(DisplayTextStyle style, DisplayTextPosition pos, DisplayTextShrink allow_shrink,
                     bool as_thought = false)
        : Style(style),
          Position(pos),
          AllowShrink(allow_shrink),
          AsThought(as_thought)
    {
    }
};

struct TopBarSettings
{
    AGS::Common::String Text;
    int Font = 0;
    int Height = 0;

    TopBarSettings() = default;
    TopBarSettings(const AGS::Common::String &text, int font, int height) : Text(text), Font(font), Height(height) {}
};

// TODO: this struct seems bit redundant now, but may be merged with other display params
struct DisplayVars
{
    int Linespacing = 0;    // font's line spacing
    int FullTextHeight = 0; // total height of all the text

    DisplayVars() = default;
    DisplayVars(int linespacing, int full_text_height) : Linespacing(linespacing), FullTextHeight(full_text_height) {}
};

// Cast text coordinates received from the script to DisplayTextPosition
DisplayTextPosition get_textpos_from_scriptcoords(int x, int y, bool for_speech);
// Generates a textual image from the given text and parameters;
// see display_main's comment below for parameters description.
// NOTE: this function treats text as-is, not doing any processing over it.
// FIXME: for historical reasons this function also contains position adjustment;
// but this should not be done here at all.
std::unique_ptr<AGS::Common::Bitmap> create_textual_image(const char *text, const DisplayTextLooks &look,
                                                          color_t text_color, int &x, int &y, int &adjusted_x,
                                                          int &adjusted_y, int width, int font,
                                                          const TopBarSettings *topbar);
// Creates a textual overlay using the given parameters;
// * disp_type tells the wanted type of overlay and behavior (blocking, etc);
// * over_id - OVER_CUSTOM for auto overlay, but allows to pass actual overlay id
//    to use, valid only if kDisplayText_NormalOverlay;
//    FIXME: find a way to not pass over_id at all, this logic is bad
// * style - more specific desired look (use text window, etc);
// * pass y < 0 to find Y co-ord automatically (see get_textpos_from_scriptcoords);
// autoplace_at_char - tells whether overlay should autoposition itself whenever game updates.
// NOTE: this function treats the text as-is; it assumes that any processing
// (translation, parsing voice token) was done prior to its call.
// TODO: refactor this collection of args into few args + 1-2 structs with extended params.
ScreenOverlay *display_main(int x, int y, int width, const char *text, const TopBarSettings *topbar,
                            DisplayTextType disp_type, int over_id, const DisplayTextLooks &look, int font,
                            color_t text_color, int autoplace_at_char = -1, bool roomlayer = false);
// Displays a standard blocking message box at a given position
void display_at(int x, int y, int width, const char *text, const TopBarSettings *topbar);
// Cleans up display message state
void post_display_cleanup();
// Tests the given string for the voice-over tags and plays cue clip for the given character;
// will assign replacement string, which will be blank string if game is in "voice-only" mode
// and clip was started, or string cleaned from voice-over tags which is safe to display on screen.
// Returns whether voice-over clip was started successfully.
bool try_auto_play_speech(const char *text, const char *&replace_text, int char_id);
// Calculates meaningful length of the displayed text
int GetTextDisplayLength(const char *text);
// Calculates number of game loops for displaying a text on screen;
// uses source_text_length instead of the text's own length, if that is set (and resets it);
// can_be_relative is 1 for background speech, which keeps its time relative to a fixed 40 fps
// when play.bgspeech_game_speed is set.
// Also stores the lipsync frame duration in loops_per_character.
int GetTextDisplayTime(const char *text, int can_be_relative = 0);
// Draw an (optionally) outlined text
void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int font, color_t text_color, color_t outline_color,
                      AGS::Common::BlendMode blend_mode, const char *text);
// Draw an (optionally) outlined text, using default outline color
void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int font, color_t text_color,
                      AGS::Common::BlendMode blend_mode, const char *text);
// Draw an (optionally) outlined text, using default outline color and normal blend mode
void wouttext_outline(AGS::Common::Bitmap *ds, int x, int y, int font, color_t text_color, const char *text);
// Draw an (optionally) outlined text using a horizontal alignment
void wouttext_aligned(AGS::Common::Bitmap *ds, int x, int y, int area_width, int font, color_t text_color,
                      int outline_color, const char *text, HorAlignment align);
// Draw an (optionally) outlined text using a horizontal alignment and default outline color
void wouttext_aligned(AGS::Common::Bitmap *ds, int x, int y, int area_width, int font, color_t text_color,
                      const char *text, HorAlignment align);
// Draws a text window's border sprite at x, y, shifted by off_x/off_y sprite widths/heights
// (-1 shifts to the left or up of the point, 0 leaves it)
void do_corner(AGS::Common::Bitmap *ds, int sprite_id, int x, int y, int off_x, int off_y);
// Returns the normal image of the button that makes the given border of a text window GUI,
// or 0 if the GUI has no such control
int get_but_pic(AGS::Common::GUIMain *gui, AGS::Common::GUITextWindowBorder border);
// Draws the background and borders of a text window into the rectangle x1,y1 - x2,y2 (inclusive)
// of the bitmap; gui is the text window GUI, or null for the standard window
void draw_button_background(AGS::Common::Bitmap *ds, int x1, int y1, int x2, int y2, AGS::Common::GUIMain *gui);
// Calculates the width that the left and right border of the text window GUI take up
int get_textwindow_border_width(int gui_id);
// Gets the height of the text window's top border
int get_textwindow_top_border_height(int gui_id);
// draw_text_window: draws the normal or custom text window
// creates and returns a new bitmap
// assigns text start x & y pos in the first two parameters;
// x, y and width are also adjusted to the size of the borders (custom text window only)
std::unique_ptr<AGS::Common::Bitmap> draw_text_window(int *text_x, int *text_y, int *x, int *y, int *width,
                                                      color_t *set_text_color, int override_height, int gui_id,
                                                      const DisplayVars &disp);
// Same as draw_text_window, with a top bar (name label) on top of the window, if topbar is given
std::unique_ptr<AGS::Common::Bitmap> draw_text_window_and_bar(const TopBarSettings *topbar, const DisplayVars &disp,
                                                              int *text_x, int *text_y, int *x, int *y, int *width,
                                                              color_t *set_text_color, int gui_id = -1);
// Gets the padding of a text window GUI; negative gui_id stands for the game's custom text window,
// zero or an invalid one gives the default padding
int get_textwindow_padding(int gui_id);

// The efficient length of the last source text prepared for display
extern int source_text_length;
