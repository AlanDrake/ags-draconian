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

#include "gui/guibutton.h"

using AGS::Common::GUIButton;
struct AnimatingGUIButton;

// **** BUTTON: FUNCTIONS ****
//
// A Button_<Name> function implements the script function Button.<Name>, which scripts reach through the
// Sc_Button_<Name> wrapper registered in RegisterButtonAPI (button.cpp); a pair Button_Get<Name> and Button_Set<Name>
// implements the script property Button.<Name> (get_<Name> and set_<Name>). btn is the script button the function is
// called on. A comment tells where the script name or behaviour differs.

// Button.Animate: starts the animation of a view loop on the button, replacing the one it already has. The view is
// 1-based as in the script; with a non-zero blocking the call returns when the animation ends, and the button stays
// enabled meanwhile even if its GUI is disabled. Volume is clamped to 0-100. sframe is the first frame.
void Button_Animate(GUIButton *btn, int view, int loop, int speed, int repeat, int blocking, int direction,
                    int sframe = 0, int volume = 100);
// Button.Animate with four arguments: in the background, forward, from frame 0, at full volume.
void Button_Animate4(GUIButton *btn, int view, int loop, int speed, int repeat);
// Button.Text: Button_GetText_New returns the text as a new script string; Button.GetText copies it into the script's
// buffer (at most MAX_MAXSTRLEN - 1 characters). The setter changes the button only when the text differs.
const char *Button_GetText_New(GUIButton *btn);
void Button_GetText(GUIButton *btn, char *buffer);
void Button_SetText(GUIButton *btn, const char *newtx);
// Button.Font: the setter validates the font number.
void Button_SetFont(GUIButton *btn, int new_font);
int Button_GetFont(GUIButton *btn);
// Button.ClipImage: whether the image is clipped to the button; the getter returns 1 or 0, the setter takes any
// non-zero value as "on".
int Button_GetClipImage(GUIButton *btn);
void Button_SetClipImage(GUIButton *btn, int newval);
// Button.Graphic: the sprite displayed now, which follows the button's state and its animation; the normal sprite
// when none has been displayed yet. Read-only.
int Button_GetGraphic(GUIButton *btn);
// Button.MouseOverGraphic, NormalGraphic and PushedGraphic: sprite slots. A setter turns a negative slot into 0, logs
// the change, stops the button's animation and registers the button for the slot's sprite-change notifications.
// Setting NormalGraphic to a slot other than 0 also resizes the button to the sprite's size; 0 makes it a plain
// coloured button.
int Button_GetMouseOverGraphic(GUIButton *btn);
void Button_SetMouseOverGraphic(GUIButton *btn, int slotn);
int Button_GetNormalGraphic(GUIButton *btn);
void Button_SetNormalGraphic(GUIButton *btn, int slotn);
int Button_GetPushedGraphic(GUIButton *btn);
void Button_SetPushedGraphic(GUIButton *btn, int slotn);
// Button.TextColor: the colour of the button's text.
int Button_GetTextColor(GUIButton *btn);
void Button_SetTextColor(GUIButton *btn, int newcol);

// **** BUTTON: ANIMATIONS ****
//
// The running animations of all the buttons, kept in one list; the game loop updates them even when the game is
// paused, and saves write and read them. A button has at most one.

// Advances the animation at index bu by one tick; returns true while it continues, false when it has ended (the caller
// then removes it with StopButtonAnimation).
bool UpdateAnimatingButton(int bu);
// The number of running animations.
size_t GetAnimatingButtonCount();
// The animation at the given index, or nullptr if the index is out of range.
AnimatingGUIButton *GetAnimatingButtonByIndex(int idxn);
// Appends a copy of the animation to the list (the save loader uses it); does not look for one on the same button.
void AddButtonAnimation(const AnimatingGUIButton &abtn);
// Removes the animation at the given index, which must be valid.
void StopButtonAnimation(int idxn);
// Returns the index of the animation of the button objn (its ID within its GUI) on the GUI gui_id, or -1 if the button
// is not animating.
int FindButtonAnimation(int gui_id, int objn);
// Stops the animation of the given button, if there is one.
void FindAndRemoveButtonAnimation(int gui_id, int objn);
// Removes all the animations (a loaded save starts from none).
void RemoveAllButtonAnimations();
