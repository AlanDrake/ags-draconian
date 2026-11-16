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
#include "game/ui/button.h"

#include <algorithm>
#include <cstdio>

#include "debug/debug_log.h"
#include "game/dynobj/cc_gui_control.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "game/ui/animating_gui_button.h"
#include "game/ui/gui.h"
#include "game/world/object.h"
#include "game/world/view_frame.h"
#include "gui/guimain.h"
#include "main/game_run.h"
#include "util/common.h"

using namespace AGS::Common;

// *** BUTTON FUNCTIONS

std::vector<AnimatingGUIButton> animbuts;

// Update the actual button's image from the current animation frame
void UpdateButtonState(const AnimatingGUIButton &abtn)
{
    // Assign view frame as normal image and reset all the rest
    const auto &vf = Game::Current().Views[abtn.view].loops[abtn.loop].frames[abtn.frame];
    guibuts[abtn.buttonid].SetImages(vf.pic, 0, 0, vf.flags, vf.xoffs, vf.yoffs);
}

void ABut_CheckViewFrame(AnimatingGUIButton &abtn)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(&guibuts[abtn.buttonid], &ccDynamicGUIButton));
    CheckViewFrame(abtn.view, abtn.loop, abtn.frame, abtn.anim.AudioVolume, 0, 1000, objevt,
                   &guibuts[abtn.buttonid].GetEvents(), kButtonEvent_OnFrameEvent);
    UpdateButtonState(abtn);
}

bool Button_RunFrameEventImpl(GUIButton &btn, int view, int loop, int frame)
{
    ObjectEvent objevt(kScTypeGame, RuntimeScriptValue().SetScriptObject(&btn, &ccDynamicGUIButton));
    return RunViewFrameEvent(view, loop, frame, objevt, &btn.GetEvents(), kButtonEvent_OnFrameEvent);
}

void Button_Animate(GUIButton *btn, int view, int loop, int speed, int repeat, int blocking, int direction, int sframe,
                    int volume)
{
    int gui_id = btn->GetParentID();
    int btn_id = btn->GetID();

    view--; // convert to internal 0-based view ID
    ValidateViewAnimVLF("Button.Animate", btn->GetName().GetCStr(), view, loop, sframe);
    ValidateViewAnimParams("Button.Animate", btn->GetName().GetCStr(), blocking, repeat, direction);

    volume = Math::Clamp(volume, 0, 100);

    // if it's already animating, stop it
    FindAndRemoveButtonAnimation(gui_id, btn_id);

    int but_id = guis[gui_id].GetControlID(btn_id);
    AnimatingGUIButton abtn;
    abtn.ongui = gui_id;
    abtn.onguibut = btn_id;
    abtn.buttonid = but_id;
    abtn.view = view;
    abtn.loop = loop;
    abtn.anim =
        ViewAnimateParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction), speed, volume);
    abtn.frame = SetFirstAnimFrame(view, loop, sframe, static_cast<AnimFlowDirection>(direction));
    abtn.wait = abtn.anim.Delay + Game::Current().Views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
    // launch into the first frame, and play the first frame's sound
    ABut_CheckViewFrame(abtn);
    animbuts.push_back(std::move(abtn));

    // Blocking animate
    if (blocking)
    {
        // Wait until the animation completes.
        // Override disabled effect for the animating button and its parent GUI.
        GUI::SetExcludedFromDisabled(btn, true);
        GameLoopUntilButAnimEnd(gui_id, btn_id);
        GUI::SetExcludedFromDisabled(btn, false);
    }
}

void Button_Animate4(GUIButton *btn, int view, int loop, int speed, int repeat)
{
    Button_Animate(btn, view, loop, speed, repeat, IN_BACKGROUND, FORWARDS, 0, 100 /* full volume */);
}

void Button_Animate7(GUIButton *btn, int view, int loop, int speed, int repeat, int blocking, int direction, int sframe)
{
    Button_Animate(btn, view, loop, speed, repeat, blocking, direction, sframe, 100 /* full volume */);
}

const char *Button_GetText_New(GUIButton *btn)
{
    return CreateNewScriptString(btn->GetText());
}

void Button_GetText(GUIButton *btn, char *buffer)
{
    snprintf(buffer, MAX_MAXSTRLEN, "%s", btn->GetText().GetCStr());
}

void Button_SetText(GUIButton *btn, const char *new_text)
{
    if (btn->GetText() != new_text)
    {
        btn->SetText(new_text);
    }
}

void Button_SetFont(GUIButton *btn, int new_font)
{
    new_font = ValidateFontNumber("Button.Font", new_font);
    btn->SetFont(new_font);
}

int Button_GetFont(GUIButton *btn)
{
    return btn->GetFont();
}

int Button_GetColorStyle(GUIButton *btn)
{
    if (btn->IsDynamicColors())
    {
        if (btn->IsFlatStyle())
        {
            return eGUIButtonDynamicFlat;
        }
        else
        {
            return eGUIButtonDynamic;
        }
    }
    else
    {
        return eGUIButtonDefault;
    }
}

void Button_SetColorStyle(GUIButton *btn, int style)
{
    const ScriptButtonColorStyle old_style = static_cast<ScriptButtonColorStyle>(Button_GetColorStyle(btn));
    if (old_style != style)
    {
        switch (style)
        {
        case eGUIButtonDefault:
            btn->SetDynamicColors(false);
            btn->SetFlatStyle(false);
            break;
        case eGUIButtonDynamic:
            btn->SetDynamicColors(true);
            btn->SetFlatStyle(false);
            break;
        case eGUIButtonDynamicFlat:
            btn->SetDynamicColors(true);
            btn->SetFlatStyle(true);
            break;
        }
    }
}

int Button_GetClipImage(GUIButton *btn)
{
    return btn->IsClippingImage() ? 1 : 0;
}

void Button_SetClipImage(GUIButton *btn, int new_value)
{
    if (btn->IsClippingImage() != (new_value != 0))
    {
        btn->SetClipImage(new_value != 0);
    }
}

int Button_GetGraphic(GUIButton *btn)
{
    // return currently displayed pic
    if (btn->GetCurrentImage() < 0)
    {
        return btn->GetNormalImage();
    }
    return btn->GetCurrentImage();
}

int Button_GetGraphicFlip(GUIButton *btn)
{
    return btn->GetImageFlip();
}

void Button_SetGraphicFlip(GUIButton *btn, int flip)
{
    btn->SetImageFlip((GraphicFlip)flip);
}

bool Button_GetIsHighlighted(GUIButton *btn)
{
    return btn->IsHighlighted();
}

bool Button_GetIsPushed(GUIButton *btn)
{
    return btn->IsPushed();
}

int Button_GetMouseOverBackgroundColor(GUIButton *btn)
{
    return btn->GetMouseOverBackColor();
}

void Button_SetMouseOverBackgroundColor(GUIButton *btn, int color)
{
    btn->SetMouseOverBackColor(color);
}

int Button_GetPushedBackgroundColor(GUIButton *btn)
{
    return btn->GetPushedBackColor();
}

void Button_SetPushedBackgroundColor(GUIButton *btn, int color)
{
    btn->SetPushedBackColor(color);
}

int Button_GetMouseOverBorderColor(GUIButton *btn)
{
    return btn->GetMouseOverBorderColor();
}

void Button_SetMouseOverBorderColor(GUIButton *btn, int color)
{
    btn->SetMouseOverBorderColor(color);
}

int Button_GetPushedBorderColor(GUIButton *btn)
{
    return btn->GetPushedBorderColor();
}

void Button_SetPushedBorderColor(GUIButton *btn, int color)
{
    btn->SetPushedBorderColor(color);
}

int Button_GetMouseOverGraphic(GUIButton *btn)
{
    return btn->GetMouseOverImage();
}

void Button_SetMouseOverGraphic(GUIButton *btn, int slot)
{
    slot = std::max(0, slot);
    if (btn->GetMouseOverImage() != slot)
    {
        debug_script_log("GUI %d Button %d mouseover set to slot %d", btn->GetParentID(), btn->GetID(), slot);
    }

    const int old_slot = btn->GetMouseOverImage();
    btn->SetMouseOverImage(slot);
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_slot, slot, btn);

    FindAndRemoveButtonAnimation(btn->GetParentID(), btn->GetID());
}

int Button_GetNormalGraphic(GUIButton *btn)
{
    return btn->GetNormalImage();
}

void Button_SetNormalGraphic(GUIButton *btn, int slot)
{
    Game &game = Game::Current();
    slot = std::max(0, slot);
    if (btn->GetNormalImage() != slot)
    {
        debug_script_log("GUI %d Button %d normal set to slot %d", btn->GetParentID(), btn->GetID(), slot);
    }

    const int old_slot = btn->GetNormalImage();
    // NormalGraphic = 0 will turn the Button into a standard colored button
    if (slot == 0)
    {
        btn->SetNormalImage(slot);
    }
    // Any other sprite - update the clickable area to the same size as the graphic
    else
    {
        const int width = static_cast<size_t>(slot) < game.SpriteInfos.size() ? game.SpriteInfos[slot].Width : 0;
        const int height = static_cast<size_t>(slot) < game.SpriteInfos.size() ? game.SpriteInfos[slot].Height : 0;
        btn->SetNormalImage(slot);
        btn->SetSize(width, height);
    }

    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_slot, slot, btn);

    FindAndRemoveButtonAnimation(btn->GetParentID(), btn->GetID());
}

int Button_GetPushedGraphic(GUIButton *btn)
{
    return btn->GetPushedImage();
}

void Button_SetPushedGraphic(GUIButton *btn, int slot)
{
    slot = std::max(0, slot);
    if (btn->GetPushedImage() != slot)
    {
        debug_script_log("GUI %d Button %d pushed set to slot %d", btn->GetParentID(), btn->GetID(), slot);
    }

    const int old_slot = btn->GetPushedImage();
    btn->SetPushedImage(slot);
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_slot, slot, btn);

    FindAndRemoveButtonAnimation(btn->GetParentID(), btn->GetID());
}

int Button_GetBorderShadeColor(GUIButton *btn)
{
    return btn->GetBorderShadeColor();
}

void Button_SetBorderShadeColor(GUIButton *btn, int color)
{
    btn->SetBorderShadeColor(color);
}

int Button_GetTextColor(GUIButton *btn)
{
    return btn->GetTextColor();
}

void Button_SetTextColor(GUIButton *btn, int color)
{
    btn->SetTextColor(color);
}

int Button_GetTextOutlineColor(GUIButton *btn)
{
    return btn->GetTextOutlineColor();
}

void Button_SetTextOutlineColor(GUIButton *btn, int color)
{
    btn->SetTextOutlineColor(color);
}

int Button_GetMouseOverTextColor(GUIButton *btn)
{
    return btn->GetMouseOverTextColor();
}

void Button_SetMouseOverTextColor(GUIButton *btn, int color)
{
    btn->SetMouseOverTextColor(color);
}

int Button_GetPushedTextColor(GUIButton *btn)
{
    return btn->GetPushedTextColor();
}

void Button_SetPushedTextColor(GUIButton *btn, int color)
{
    btn->SetPushedTextColor(color);
}

// ** start animating buttons code

size_t GetAnimatingButtonCount()
{
    return animbuts.size();
}

AnimatingGUIButton *GetAnimatingButtonByIndex(int idx)
{
    return idx >= 0 && (size_t)idx < animbuts.size() ? &animbuts[idx] : nullptr;
}

void AddButtonAnimation(const AnimatingGUIButton &abtn)
{
    animbuts.push_back(abtn);
}

// Returns true while the animation continues, false when it has ended
bool UpdateAnimatingButton(int bu)
{
    AnimatingGUIButton &abtn = animbuts[bu];
    if (abtn.wait > 0)
    {
        abtn.wait--;
        return true;
    }
    if (!CycleViewAnim(abtn.view, abtn.loop, abtn.frame, abtn.anim))
        return false;

    ABut_CheckViewFrame(abtn);
    abtn.wait = abtn.anim.Delay + Game::Current().Views[abtn.view].loops[abtn.loop].frames[abtn.frame].speed;
    return true;
}

void StopButtonAnimation(int idx)
{
    animbuts.erase(animbuts.begin() + idx);
}

void RemoveAllButtonAnimations()
{
    animbuts.clear();
}

// Returns the index of the AnimatingGUIButton object corresponding to the
// given button ID; returns -1 if no such animation exists
int FindButtonAnimation(int gui_id, int btn_id)
{
    for (size_t i = 0; i < animbuts.size(); ++i)
    {
        if (animbuts[i].ongui == gui_id && animbuts[i].onguibut == btn_id)
        {
            return i;
        }
    }
    return -1;
}

void FindAndRemoveButtonAnimation(int gui_id, int btn_id)
{
    int idx = FindButtonAnimation(gui_id, btn_id);
    if (idx >= 0)
        StopButtonAnimation(idx);
}

// ** end animating buttons code

void Button_Click(GUIButton *btn, int mbut)
{
    process_interface_click(btn->GetParentID(), btn->GetID(), mbut);
}

bool Button_RunFrameEvent(GUIButton *btn, int view, int loop, int frame)
{
    view--; // convert to internal 0-based view ID
    AssertFrame("Button.RunFrameEvent", btn->GetName().GetCStr(), view, loop, frame);
    return Button_RunFrameEventImpl(*btn, view, loop, frame);
}

bool Button_IsAnimating(GUIButton *btn)
{
    return FindButtonAnimation(btn->GetParentID(), btn->GetID()) >= 0;
}

// NOTE: in correspondence to similar functions for Character & Object,
// GetView returns (view index + 1), while GetLoop and GetFrame return
// zero-based index and 0 in case of no animation.
int Button_GetAnimView(GUIButton *btn)
{
    int idx = FindButtonAnimation(btn->GetParentID(), btn->GetID());
    return idx >= 0 ? animbuts[idx].view + 1 : 0;
}

int Button_GetAnimLoop(GUIButton *btn)
{
    int idx = FindButtonAnimation(btn->GetParentID(), btn->GetID());
    return idx >= 0 ? animbuts[idx].loop : 0;
}

int Button_GetAnimFrame(GUIButton *btn)
{
    int idx = FindButtonAnimation(btn->GetParentID(), btn->GetID());
    return idx >= 0 ? animbuts[idx].frame : 0;
}

int Button_GetTextAlignment(GUIButton *btn)
{
    return btn->GetTextAlignment();
}

void Button_SetTextAlignment(GUIButton *btn, int align)
{
    btn->SetTextAlignment((FrameAlignment)align);
}

int Button_GetTextPaddingHorizontal(GUIButton *btn)
{
    return btn->GetPaddingX();
}

void Button_SetTextPaddingHorizontal(GUIButton *btn, int pad)
{
    btn->SetPaddingX(pad);
}

int Button_GetTextPaddingVertical(GUIButton *btn)
{
    return btn->GetPaddingY();
}

void Button_SetTextPaddingVertical(GUIButton *btn, int pad)
{
    btn->SetPaddingY(pad);
}

bool Button_GetWrapText(GUIButton *btn)
{
    return btn->IsWrapText();
}

void Button_SetWrapText(GUIButton *btn, bool wrap)
{
    if (btn->IsWrapText() != wrap)
    {
        btn->SetWrapText(wrap);
        btn->MarkChanged();
    }
}

#include "game/dynobj/script_string.h"
#include "script/script_runtime.h"
