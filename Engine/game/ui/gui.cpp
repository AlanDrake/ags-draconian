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
#include "game/ui/gui.h"

#include <algorithm>
#include <cstdio>
#include <numeric>
#include <vector>

#include "debug/debug_log.h"
#include "device/mousew32.h"
#include "game/dialog/display.h"
#include "game/dynobj/cc_gui.h"
#include "game/dynobj/cc_gui_control.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_shader.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/runtime_defines.h"
#include "game/script.h"
#include "game/system.h"
#include "game/ui/gui_control.h"
#include "game/ui/inv_window.h"
#include "game/ui/mouse.h"
#include "game/world/character.h"
#include "game/world/inventory_item.h"
#include "game/world/object.h"
#include "game/world/properties.h"
#include "game/world/room.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/dynobj/script_user_object.h"
#include "script/runtimescriptvalue.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/geometry.h"
#include "util/string_compat.h"


using namespace AGS::Common;
using namespace AGS::Engine;


namespace AGS
{
namespace Engine
{
namespace GUIE
{

void MarkAllGUIForUpdate(bool redraw, bool reset_over_ctrl)
{
    Game &game = Game::Current();
    for (auto &gui : game.Guis)
    {
        if (redraw)
        {
            gui.MarkChanged();
            for (int i = 0; i < gui.GetControlCount(); ++i)
                gui.GetControl(i)->MarkChanged();
        }
        if (reset_over_ctrl)
            gui.ResetOverControl();
    }
}

void MarkForTranslationUpdate()
{
    Game &game = Game::Current();
    for (auto &btn : game.GuiButtons)
    {
        if (btn.IsTranslated())
            btn.MarkChanged();
    }
    for (auto &lbl : game.GuiLabels)
    {
        if (lbl.IsTranslated())
            lbl.MarkChanged();
    }
    for (auto &list : game.GuiListBoxes)
    {
        if (list.IsTranslated())
            list.MarkChanged();
    }
    for (auto &tbox : game.GuiTextBoxes)
    {
        if (tbox.IsTranslated())
            tbox.MarkChanged();
    }
}

void MarkForFontUpdate(int font)
{
    Game &game = Game::Current();
    const bool update_all = (font < 0);
    for (auto &btn : game.GuiButtons)
    {
        if (update_all || btn.GetFont() == font)
            btn.MarkChanged();
    }
    for (auto &lbl : game.GuiLabels)
    {
        if (update_all || lbl.GetFont() == font)
            lbl.MarkChanged();
    }
    for (auto &list : game.GuiListBoxes)
    {
        if (update_all || list.GetFont() == font)
            list.MarkChanged();
    }
    for (auto &tb : game.GuiTextBoxes)
    {
        if (update_all || tb.GetFont() == font)
            tb.MarkChanged();
    }
}

void MarkSpecialLabelsForUpdate(GUILabelMacro macro)
{
    Game &game = Game::Current();
    for (auto &lbl : game.GuiLabels)
    {
        if ((lbl.GetTextMacros() & macro) != 0)
        {
            lbl.MarkChanged();
        }
    }
}

void MarkInventoryForUpdate(int char_id, bool is_player)
{
    Game &game = Game::Current();
    for (auto &btn : game.GuiButtons)
    {
        if (btn.GetPlaceholder() != kButtonPlace_None)
            btn.MarkChanged();
    }
    for (auto &inv : game.GuiInvWindows)
    {
        if ((char_id < 0) || (inv.GetCharacterID() == char_id) || (is_player && inv.GetCharacterID() < 0))
        {
            inv.MarkChanged();
        }
    }
}

void MarkDisabledGUIForUpdate()
{
    Game &game = Game::Current();
    for (auto &gui : game.Guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            auto *control = gui.GetControl(i);
            if (!control->IsEnabled())
                control->MarkChanged();
        }
    }
}

} // namespace GUIE
} // namespace Engine
} // namespace AGS


ScriptGUI *GUI_AsTextWindow(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    // Internally both GUI and TextWindow are implemented by same class
    return game.Guis[sgui->id].IsTextWindow() ? &game.ScGuis[sgui->id] : nullptr;
}

int GUI_GetPopupStyle(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetPopupStyle();
}

void GUI_SetPopupStyle(ScriptGUI *sgui, int popup_style)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    const GUIPopupStyle old_style = game.Guis[sgui->id].GetPopupStyle();
    if (popup_style == old_style)
        return;

    GUIMain &gui = game.Guis[sgui->id];
    gui.SetPopupStyle(static_cast<GUIPopupStyle>(popup_style));

    // Conceal or un-conceal when changing to or from PopupAtY
    if (popup_style == kGUIPopupMouseY)
    {
        gui.SetConceal(true);
    }
    else if (old_style == kGUIPopupMouseY)
    {
        if (gui.IsDisplayed())
            remove_popup_interface(sgui->id);
        gui.SetConceal(false);
    }

    if (gui.IsVisible())
    {
        if (popup_style == kGUIPopupModal)
        {
            PauseGame();
        }
        else if (old_style == kGUIPopupModal)
        {
            UnPauseGame();
        }

        if (gui.IsDisplayed() && game.Play.mouse_input_enabled)
        {
            gui.Poll(services.Input.MouseX, services.Input.MouseY,
                     GetGUIAt(services.Input.MouseX, services.Input.MouseY, kHit_Interactable) == gui.GetID());
        }
    }
}

void GUI_SetVisible(ScriptGUI *sgui, int is_visible)
{
    if (is_visible)
        InterfaceOn(sgui->id);
    else
        InterfaceOff(sgui->id);
}

int GUI_GetVisible(ScriptGUI *sgui)
{
    return (Game::Current().Guis[sgui->id].IsVisible()) ? 1 : 0;
}

bool GUI_GetShown(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].IsDisplayed();
}

int GUI_GetX(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetX();
}

void GUI_SetX(ScriptGUI *sgui, int x)
{
    Game::Current().Guis[sgui->id].SetX(x);
}

int GUI_GetY(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetY();
}

void GUI_SetY(ScriptGUI *sgui, int y)
{
    Game::Current().Guis[sgui->id].SetY(y);
}

void GUI_SetPosition(ScriptGUI *sgui, int x, int y)
{
    Game::Current().Guis[sgui->id].SetPosition(x, y);
}

void GUI_SetSize(ScriptGUI *sgui, int w, int h)
{
    if ((w < 1) || (h < 1))
    {
        debug_script_warn("GUI.SetSize: invalid dimensions (tried to set to %d x %d)", w, h);
        w = std::max(1, w);
        h = std::max(1, h);
    }

    GUIMain *gui = &Game::Current().Guis[sgui->id];
    gui->SetSize(w, h);
}

int GUI_GetWidth(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetWidth();
}

int GUI_GetHeight(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetHeight();
}

void GUI_SetWidth(ScriptGUI *sgui, int new_width)
{
    if (new_width < 1)
    {
        debug_script_warn("GUI.SetWidth: invalid value (tried to set to %d)", new_width);
        new_width = std::max(1, new_width);
    }
    Game::Current().Guis[sgui->id].SetWidth(new_width);
}

void GUI_SetHeight(ScriptGUI *sgui, int new_height)
{
    if (new_height < 1)
    {
        debug_script_warn("GUI.SetHeight: invalid value (tried to set to %d)", new_height);
        new_height = std::max(1, new_height);
    }
    Game::Current().Guis[sgui->id].SetHeight(new_height);
}

void GUI_SetZOrder(ScriptGUI *sgui, int z)
{
    Game::Current().Guis[sgui->id].SetZOrder(z);
    update_gui_zorder();
}

int GUI_GetZOrder(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetZOrder();
}

void *GUI_GetGraphicPosition(ScriptGUI *sgui)
{
    auto &gui = Game::Current().Guis[sgui->id];
    gui.UpdateGraphicSpace();
    std::vector<Point> points;
    gui.GetGraphicSpace().GetTransformedCorners(points, gui.GetSize());
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void *GUI_GetGraphicBoundBox(ScriptGUI *sgui)
{
    auto &gui = Game::Current().Guis[sgui->id];
    gui.UpdateGraphicSpace();
    std::vector<Point> points;
    gui.GetGraphicSpace().GetAABBPoints(points);
    return ScriptStructHelpers::CreateArrayOfPoints(points).Obj();
}

void GUI_SetClickable(ScriptGUI *sgui, int clickable)
{
    Game::Current().Guis[sgui->id].SetClickable(clickable != 0);
}

int GUI_GetClickable(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].IsClickable() ? 1 : 0;
}

int GUI_GetID(ScriptGUI *sgui)
{
    return sgui->id;
}

const char *GUI_GetScriptName(ScriptGUI *sgui)
{
    return CreateNewScriptString(Game::Current().Guis[sgui->id].GetName());
}

GUIControl *GUI_GetiControls(ScriptGUI *sgui, int idx)
{
    Game &game = Game::Current();
    if ((idx < 0) || (idx >= game.Guis[sgui->id].GetControlCount()))
        return nullptr;
    return game.Guis[sgui->id].GetControl(idx);
}

int GUI_GetControlCount(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetControlCount();
}

int GUI_GetPopupYPos(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetPopupAtY();
}

void GUI_SetPopupYPos(ScriptGUI *sgui, int popup_y)
{
    Game &game = Game::Current();
    if (!game.Guis[sgui->id].IsTextWindow())
        game.Guis[sgui->id].SetPopupAtY(popup_y);
}

void GUI_SetOpacity(ScriptGUI *sgui, int alpha)
{
    if ((alpha < 0) || (alpha > 255))
        quit("!GUI.Opacity: alpha value must be between 0 and 255");

    Game::Current().Guis[sgui->id].SetOpacity(alpha);
}

int GUI_GetOpacity(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetOpacity();
}

// [OBSOLETE] the AGS 4.0 Transparency, for compiled 4.0 games
void GUI_SetTransparency(ScriptGUI *sgui, int trans)
{
    if ((trans < 0) || (trans > 100))
        quit("!SetGUITransparency: transparency value must be between 0 and 100");

    Game::Current().Guis[sgui->id].SetOpacity(GfxDef::Trans100ToOpacity(trans));
}

int GUI_GetTransparency(ScriptGUI *sgui)
{
    return GfxDef::OpacityToTrans100(Game::Current().Guis[sgui->id].GetOpacity());
}

void GUI_Centre(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    GUIMain *gui = &game.Guis[sgui->id];
    int x = game.Viewports.GetUIViewport().GetWidth() / 2 - gui->GetWidth() / 2;
    int y = game.Viewports.GetUIViewport().GetHeight() / 2 - gui->GetHeight() / 2;
    gui->SetPosition(x, y);
}

void GUI_SetBackgroundGraphic(ScriptGUI *sgui, int sprite)
{
    Game &game = Game::Current();
    const int old_sprite = game.Guis[sgui->id].GetBgImage();
    game.Guis[sgui->id].SetBgImage(sprite);
    // NOTE: GUIs currently cannot react to a dynamic sprite change without a direct notification
    replace_sprite_changed_callback(old_sprite, sprite, &game.Guis[sgui->id]);
}

int GUI_GetBackgroundGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    if (game.Guis[sgui->id].GetBgImage() < 1) // "no image" is reported to scripts as 0, not as a negative value
        return 0;
    return game.Guis[sgui->id].GetBgImage();
}

void GUI_SetBackgroundColor(ScriptGUI *sgui, int color)
{
    Game::Current().Guis[sgui->id].SetBgColor(color);
}

int GUI_GetBackgroundColor(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetBgColor();
}

void GUI_SetBorderColor(ScriptGUI *sgui, int color)
{
    Game &game = Game::Current();
    if (game.Guis[sgui->id].IsTextWindow())
        return;
    game.Guis[sgui->id].SetFgColor(color);
}

int GUI_GetBorderColor(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    if (game.Guis[sgui->id].IsTextWindow())
        return 0;
    return game.Guis[sgui->id].GetFgColor();
}

void GUI_SetTextColor(ScriptGUI *sgui, int color)
{
    Game &game = Game::Current();
    if (!game.Guis[sgui->id].IsTextWindow())
        return;
    game.Guis[sgui->id].SetFgColor(color);
}

int GUI_GetTextColor(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    if (!game.Guis[sgui->id].IsTextWindow())
        return 0;
    return game.Guis[sgui->id].GetFgColor();
}

int GUI_GetTextPadding(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetPadding();
}

void GUI_SetTextPadding(ScriptGUI *sgui, int padding)
{
    Game &game = Game::Current();
    if (game.Guis[sgui->id].IsTextWindow())
        game.Guis[sgui->id].SetPadding(padding);
}

int GUI_GetLeftGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_Left) : 0;
}

int GUI_GetTopLeftGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_TopLeft) : 0;
}

int GUI_GetTopGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_Top) : 0;
}

int GUI_GetTopRightGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_TopRight) : 0;
}

int GUI_GetRightGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_Right) : 0;
}

int GUI_GetBottomRightGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_BottomRight) : 0;
}

int GUI_GetBottomGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_Bottom) : 0;
}

int GUI_GetBottomLeftGraphic(ScriptGUI *sgui)
{
    Game &game = Game::Current();
    return game.Guis[sgui->id].IsTextWindow() ? get_but_pic(&game.Guis[sgui->id], kTW_BottomLeft) : 0;
}

int GetGUIAt(int x, int y, int hit_options)
{
    Game &game = Game::Current();
    const bool only_clickable = (hit_options & kHit_Interactable) != 0;
    // Test in the opposite order (from closer to further)
    for (auto g = game.Play.gui_draw_order.crbegin(); g < game.Play.gui_draw_order.crend(); ++g)
    {
        if (game.Guis[*g].IsDisplayed())
        {
            // test in the GUI's own coordinates, which account for its scale and rotation
            const Point local = game.Guis[*g].GetGraphicSpace().WorldToLocal(x, y);
            if ((only_clickable && game.Guis[*g].IsInteractableAt(x, y) ||
                 (!only_clickable && RectWH(game.Guis[*g].GetSize()).IsInside(local))))
            {
                return *g;
            }
        }
    }
    return -1;
}

ScriptGUI *GUI_GetAtScreenXY(int x, int y, int hit_options)
{
    int gui_id = GetGUIAt(x, y, hit_options);
    if (gui_id < 0)
        return nullptr;
    return &Game::Current().ScGuis[gui_id];
}

ScriptGUI *GUI_GetAtScreenXY2(int x, int y)
{
    return GUI_GetAtScreenXY(x, y, true);
}

void GUI_Click(ScriptGUI *sgui, int mbut)
{
    process_interface_click(sgui->id, -1, mbut);
}

void GUI_ProcessClick(int x, int y, int mbut)
{
    int gui_id = gui_get_interactable(x, y);
    if (gui_id >= 0)
    { // simulate mouse click at the given coordinates
        Game::Current().Guis[gui_id].Poll(x, y, true);
        gui_on_mouse_down(gui_id, mbut, x, y);
        gui_on_mouse_up(gui_id, mbut, x, y);
    }
}

int GUI_GetBlendMode(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetBlendMode();
}

void GUI_SetBlendMode(ScriptGUI *sgui, int blend_mode)
{
    Game &game = Game::Current();
    game.Guis[sgui->id].SetBlendMode(
        ValidateBlendMode("GUI.BlendMode", game.Guis[sgui->id].GetName().GetCStr(), blend_mode));
}

ScriptShaderInstance *GUI_GetShader(ScriptGUI *sgui)
{
    return static_cast<ScriptShaderInstance *>(
        ccGetObjectAddressFromHandle(Game::Current().Guis[sgui->id].GetShaderHandle()));
}

void GUI_SetShader(ScriptGUI *sgui, ScriptShaderInstance *shader_inst)
{
    auto &gui = Game::Current().Guis[sgui->id];
    gui.SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                  ccReplaceObjectHandle(gui.GetShaderHandle(), shader_inst));
}

float GUI_GetRotation(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetRotation();
}

void GUI_SetRotation(ScriptGUI *sgui, float rotation)
{
    Game::Current().Guis[sgui->id].SetRotation(rotation);
}

float GUI_GetScaleX(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetScale().X;
}

void GUI_SetScaleX(ScriptGUI *sgui, float scale_x)
{
    Game &game = Game::Current();
    game.Guis[sgui->id].SetScale(scale_x, game.Guis[sgui->id].GetScale().Y);
}

float GUI_GetScaleY(ScriptGUI *sgui)
{
    return Game::Current().Guis[sgui->id].GetScale().Y;
}

void GUI_SetScaleY(ScriptGUI *sgui, float scale_y)
{
    Game &game = Game::Current();
    game.Guis[sgui->id].SetScale(game.Guis[sgui->id].GetScale().X, scale_y);
}

void GUI_SetScale(ScriptGUI *sgui, float scale_x, float scale_y)
{
    Game::Current().Guis[sgui->id].SetScale(scale_x, scale_y);
}

int GUI_GetProperty(ScriptGUI *sgui, const char *property)
{
    Game &game = Game::Current();
    return get_int_property(game.guiProps[sgui->id], game.Play.guiProps[sgui->id], property);
}

const char *GUI_GetTextProperty(ScriptGUI *sgui, const char *property)
{
    Game &game = Game::Current();
    return get_text_property_dynamic_string(game.guiProps[sgui->id], game.Play.guiProps[sgui->id], property);
}

bool GUI_SetProperty(ScriptGUI *sgui, const char *property, int value)
{
    return set_int_property(Game::Current().Play.guiProps[sgui->id], property, value);
}

bool GUI_SetTextProperty(ScriptGUI *sgui, const char *property, const char *value)
{
    return set_text_property(Game::Current().Play.guiProps[sgui->id], property, value);
}

//=============================================================================

void remove_popup_interface(int gui_id)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if (game.IfacePopped != gui_id)
        return;
    game.IfacePopped = -1;
    UnPauseGame();
    game.Guis[gui_id].SetConceal(true);
    if (game.Play.mouse_input_enabled && services.Input.MouseY <= game.Guis[gui_id].GetPopupAtY())
        Mouse::SetPosition(Point(services.Input.MouseX, game.Guis[gui_id].GetPopupAtY() + 2));
    if ((!IsInterfaceEnabled()) && (game.Cursor.Look == game.Cursor.Mode))
        // Only change the mouse cursor if it hasn't been specifically changed first
        set_cursor_look(game.GetModeWait());
    else if (IsInterfaceEnabled())
        set_default_cursor_look();

    if (gui_id == game.MouseOnIface)
        game.MouseOnIface = -1;
}

static void run_control_event(GUIControl *control, const String &script_module, int evt_index, int mb_param,
                              bool do_interface_click)
{
    // If the object has a special handler script then run it;
    // otherwise, run interface_click
    // TODO: do not call DoesScriptFunctionExist* every time, remember last result,
    // similar to the interaction event handler test.
    if (control->GetEvents().HasHandler(evt_index) &&
        DoesScriptFunctionExistInModule(script_module, control->GetEvents().GetHandler(evt_index).FunctionName))
    {
        // control-specific event handler
        const ScriptFunctionRef fn_ref(script_module, control->GetEvents().GetHandler(evt_index).FunctionName);
        if (mb_param >= 0)
        {
            RuntimeScriptValue params[]{RuntimeScriptValue().SetScriptObject(control, &ccDynamicGUIControl),
                                        RuntimeScriptValue().SetInt32(mb_param)};
            RunScriptFunctionAuto(kScTypeGame, fn_ref, 2, params);
        }
        else
        {
            RuntimeScriptValue params[]{RuntimeScriptValue().SetScriptObject(control, &ccDynamicGUIControl)};
            RunScriptFunctionAuto(kScTypeGame, fn_ref, 1, params);
        }
    }
    else if (do_interface_click)
    {
        RuntimeScriptValue params[]{control->GetParentID(), control->GetID()};
        RunScriptFunctionAuto(kScTypeGame, "interface_click", 2, params);
    }
}

void process_interface_click(int gui_id, int btn_id, int mbut)
{
    Game &game = Game::Current();
    const GUIMain &gui = game.Guis[gui_id];
    if (btn_id < 0)
    {
        // click on GUI background
        RuntimeScriptValue params[]{RuntimeScriptValue().SetScriptObject(&game.ScGuis[gui_id], &ccDynamicGUI),
                                    RuntimeScriptValue().SetInt32(mbut)};
        TryRunScriptHandler(kScTypeGame, gui.GetScriptModule(), gui.GetEvents().GetHandler(kGUIEvent_OnClick), 2,
                            params);
        return;
    }

    // Decide which event to fire when the control is clicked on
    GUIControl *control = game.Guis[gui_id].GetControl(btn_id);
    if (!control)
        return;

    const GUIControlType ctrl_type = game.Guis[gui_id].GetControlType(btn_id);
    switch (ctrl_type)
    {
    case kGUIButton:
    {
        const GUIButton *btn = dynamic_cast<GUIButton *>(control);
        GUIClickAction action_type = btn->GetClickAction(kGUIClickLeft);
        switch (action_type)
        {
        case kGUIAction_SetMode: set_cursor_mode(btn->GetClickData(kGUIClickLeft)); break;
        case kGUIAction_RunScript: run_control_event(control, gui.GetScriptModule(), kButtonEvent_OnClick, mbut, true);
        default: break;
        }
    }
    break;
    case kGUIListBox: run_control_event(control, gui.GetScriptModule(), kListBoxEvent_OnSelChanged, -1, true); break;
    case kGUISlider: run_control_event(control, gui.GetScriptModule(), kSliderEvent_OnChange, -1, true); break;
    case kGUITextBox: run_control_event(control, gui.GetScriptModule(), kTextBoxEvent_OnActivate, -1, true); break;
    default:
        assert(false); // should not happen
        break;
    }
}

bool sort_gui_less(const int g1, const int g2)
{
    Game &game = Game::Current();
    return (game.Guis[g1].GetZOrder() < game.Guis[g2].GetZOrder()) ||
           ((game.Guis[g1].GetZOrder() == game.Guis[g2].GetZOrder()) && (g1 < g2));
}

void update_gui_zorder()
{
    Game &game = Game::Current();
    std::sort(game.Play.gui_draw_order.begin(), game.Play.gui_draw_order.end(), sort_gui_less);
}

void prepare_gui_runtime(bool startup)
{
    Game &game = Game::Current();
    // Trigger all guis and controls to recalculate their dynamic state
    for (auto &gui : game.Guis)
    {
        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            GUIControl *control = gui.GetControl(i);
            control->UpdateVisualState();
            control->SetActivated(false);
        }
    }

    game.Play.gui_draw_order.resize(game.Guis.size());
    std::iota(game.Play.gui_draw_order.begin(), game.Play.gui_draw_order.end(), 0);
    update_gui_zorder();

    // Restore subscription to dynamic sprite notifications
    if (!startup)
    {
        for (auto &gui : game.Guis)
        {
            if (gui.GetBgImage() > 0)
                add_sprite_changed_callback(gui.GetBgImage(), &gui);
        }
        for (auto &btn : game.GuiButtons)
        {
            if (btn.GetNormalImage() > 0)
                add_sprite_changed_callback(btn.GetNormalImage(), &btn);
            if (btn.GetPushedImage() > 0)
                add_sprite_changed_callback(btn.GetPushedImage(), &btn);
            if (btn.GetMouseOverImage() > 0)
                add_sprite_changed_callback(btn.GetMouseOverImage(), &btn);
        }
        for (auto &sld : game.GuiSliders)
        {
            if (sld.GetBgImage() > 0)
                add_sprite_changed_callback(sld.GetBgImage(), &sld);
            if (sld.GetHandleImage() > 0)
                add_sprite_changed_callback(sld.GetHandleImage(), &sld);
        }

        // This is related to InvWindow gui controls, so call from here
        InvItems_RegisterDynamicSpriteCallbacks();
    }

    GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.Options[OPT_DISABLEOFF]);
    GUIE::MarkAllGUIForUpdate(true, true);
}

void update_gui_disabled_status()
{
    // update GUI display status (perhaps we've gone into an interface disabled state)
    const GuiDisableStyle disabled_state_was = GUI::Context.DisabledState;
    GUI::Context.DisabledState = IsInterfaceEnabled() ? kGuiDis_Undefined : GUI::Options.DisabledStyle;

    if (disabled_state_was != GUI::Context.DisabledState)
    {
        // Mark guis for redraw and reset control-under-mouse detection
        const bool keep_visuals =
            (disabled_state_was == kGuiDis_Undefined || disabled_state_was == kGuiDis_Unchanged) &&
            (GUI::Context.DisabledState == kGuiDis_Undefined || GUI::Context.DisabledState == kGuiDis_Unchanged);
        GUIE::MarkAllGUIForUpdate(!keep_visuals, true);
        if (!keep_visuals)
        {
            invalidate_screen();
        }
    }
}

static bool should_skip_adjust_for_gui(const GUIMain &gui)
{
    return
        // not shown
        !gui.IsDisplayed() ||
        // completely offscreen
        !IsRectInsideRect(Game::Current().Viewports.GetUIViewport(), gui.GetRect()) ||
        // fully transparent: only the GUI's own background and opacity are checked, not its controls (TODO)
        ((gui.GetBgColor() == 0) && (gui.GetBgImage() < 1)) || (gui.GetOpacity() == 0);
}

int adjust_x_for_guis(int x, int y, bool assume_blocking)
{
    Game &game = Game::Current();
    if ((GUI::Options.DisabledStyle == kGuiDis_Off) && (!IsInterfaceEnabled() || assume_blocking))
        return x; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it right a bit
    // TODO: also account for the text's width here, and maybe move the text left when its right part touches a GUI
    for (const auto &gui : game.Guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.GetY() > y) || (gui.GetY() + gui.GetHeight() < y))
            continue;
        // to the left or to the right from the message
        if ((gui.GetX() > x) || (gui.GetX() + gui.GetWidth() < x))
            continue;
        // try to deal with very wide GUIs
        const float gui_width_cap = 0.75f; // NOTE: originally was 280 pixels in 320-wide game
        if (gui.GetWidth() >= game.GetGameRes().Width * gui_width_cap)
            continue;
        // fix coordinates if x is inside the gui
        x = gui.GetX() + gui.GetWidth() + 2;
    }
    return x;
}

int adjust_y_for_guis(int y, bool assume_blocking)
{
    Game &game = Game::Current();
    if ((GUI::Options.DisabledStyle == kGuiDis_Off) && (!IsInterfaceEnabled() || assume_blocking))
        return y; // All GUI off (or will be when the message is displayed)
    // If it's covered by a GUI, move it down a bit
    // TODO: also account for the text's height here, and maybe move the text up when its lower part touches a GUI below
    for (const auto &gui : game.Guis)
    {
        if (should_skip_adjust_for_gui(gui))
            continue;
        // lower or higher than the message
        if ((gui.GetY() > y) || (gui.GetY() + gui.GetHeight() < y))
            continue;
        // ignore tall GUIs (a quarter of the game height or more): the text is not moved for them
        const float gui_height_cap = 0.25f; // NOTE: originally was 50 pixels in 200-height game
        if (gui.GetHeight() >= game.GetGameRes().Height * gui_height_cap)
            continue;
        // fix coordinates if y is inside the gui
        y = gui.GetY() + gui.GetHeight() + 2;
    }
    return y;
}

int gui_get_interactable(int x, int y)
{
    if (GUI::Context.DisabledState == kGuiDis_Off)
        return -1;
    return GetGUIAt(x, y, kHit_Interactable);
}

int gui_on_mouse_move(const int mx, const int my)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    int mouse_over_gui = -1;
    // If all GUIs are off, no GUI is under the mouse or pops up
    if ((game.Options[OPT_DISABLEOFF] == kGuiDis_Off) && (GUI::Context.DisabledState >= 0))
        return mouse_over_gui;

    // Scan for mouse-y-pos GUIs, and pop one up if appropriate
    // Also work out the mouse-over GUI while we're at it
    // The loop goes in draw order, from farthest to nearest, so the last GUI that is
    // interactable at the mouse position (the nearest one) stays in mouse_over_gui;
    // except when the loop stops early, on a GUI that pops up or while a blocking
    // overlay disables the interface: the GUIs after that one are not tested.
    for (int gui_id : game.Play.gui_draw_order)
    {
        if (game.Guis[gui_id].IsInteractableAt(mx, my))
            mouse_over_gui = gui_id;

        if (game.Guis[gui_id].GetPopupStyle() != kGUIPopupMouseY)
            continue;
        if (game.Play.complete_overlay_on > 0)
            break; // interfaces disabled
        if (game.IfacePopped == gui_id)
            continue;
        if (!game.Guis[gui_id].IsVisible())
            continue;
        // Don't allow it to be popped up while skipping cutscene
        if (game.Play.fast_forward)
            continue;

        if (services.Input.MouseY < game.Guis[gui_id].GetPopupAtY())
        {
            set_cursor_look(game.GetModePointer());
            game.Guis[gui_id].SetConceal(false);
            game.IfacePopped = gui_id;
            PauseGame();
            break;
        }
    }
    return mouse_over_gui;
}

// Tells if GUI common controls should react to this mouse button.
// TODO: redesign this all, pass mouse button into GUI OnMouseButtonDown instead,
// handle the button difference there, per control.
inline bool gui_control_should_handle_button(int mbut)
{
    return (Game::Current().Options[OPT_GUICONTROLMOUSEBUT] == 0) || (mbut == kMouseLeft);
}

void gui_on_mouse_hold(const int wasongui, const int wasbutdown)
{
    Game &game = Game::Current();
    for (int i = 0; i < game.Guis[wasongui].GetControlCount(); ++i)
    {
        GUIControl *control = game.Guis[wasongui].GetControl(i);
        if (!control->IsActivated())
            continue;
        // We only handle "hold" event for Sliders, and only if mouse button is not restricted
        if (game.Guis[wasongui].GetControlType(i) != kGUISlider)
            continue;
        if (!gui_control_should_handle_button(wasbutdown))
            continue;
        // GUI Slider repeatedly activates while being dragged
        control->SetActivated(false);
        force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
        break;
    }
}

void gui_on_mouse_up(const int wasongui, const int wasbutdown, const int mx, const int my)
{
    Game &game = Game::Current();
    GUIMain &gui = game.Guis[wasongui];
    gui.OnMouseButtonUp();

    for (int i = 0; i < gui.GetControlCount(); ++i)
    {
        GUIControl *control = gui.GetControl(i);
        if (!control->IsActivated())
            continue;

        control->SetActivated(false);
        if (!IsInterfaceEnabled())
            break;

        bool click_handled = false;
        int ctrl_type = gui.GetControlType(i);
        if ((ctrl_type == kGUIButton) || (ctrl_type == kGUISlider) || (ctrl_type == kGUIListBox))
        {
            if (gui_control_should_handle_button(wasbutdown))
            {
                click_handled = true;
                force_event(AGSEvent_GUI(wasongui, i, static_cast<eAGSMouseButton>(wasbutdown)));
            }
        }
        else if (ctrl_type == kGUIInvWindow)
        {
            click_handled = true;
            Point guipt = gui.GetGraphicSpace().WorldToLocal(mx, my);
            Point gobjpt = control->GetGraphicSpace().WorldToLocal(guipt.X, guipt.Y);
            int item_id = InvWindow_GetItemAtXY(static_cast<GUIInvWindow *>(control), gobjpt.X, gobjpt.Y);
            if (item_id >= 0)
            {
                game.Play.used_inv_on = item_id;
                if (game.Options[OPT_HANDLEINVCLICKS])
                {
                    // Let the script handle the click
                    // LEFTINV is 5, RIGHTINV is 6
                    force_event(AGSEvent_Script(kTS_MouseClick, wasbutdown + 4, mx, my));
                }
                else if (wasbutdown == kMouseRight) // right-click is always Look
                {
                    RunInventoryInteraction(item_id, game.GetModeLook());
                }
                else if (is_current_cursor_mode(kCursorRole_Interact))
                {
                    SetActiveInventory(item_id);
                }
                else
                {
                    RunInventoryInteraction(item_id, game.Cursor.Mode);
                }
            }
        }
        else
        {
            quit("clicked on unknown control type");
        }

        // Built-in behavior for PopupAtY guis: hide one if interacted with any control
        if ((gui.GetPopupStyle() == kGUIPopupMouseY) && click_handled)
            remove_popup_interface(wasongui);
        break;
    }

    run_on_event(kScriptEvent_GUIMouseUp, wasongui, wasbutdown, mx - gui.GetX(), my - gui.GetY());
}

void gui_on_mouse_down(const int gui_id, const int mbut, const int mx, const int my)
{
    Game &game = Game::Current();
    debug_script_log("Mouse click over GUI %d", gui_id);

    // TODO: redesign this, pass mouse button into GUI OnMouseButtonDown instead,
    // handle the button difference there, per control.
    int over_ctrl = game.Guis[gui_id].GetControlUnderMouse();
    if (over_ctrl >= 0)
    {
        // NOTE: we make exception for InvWindow control, as it has a special action for RMB
        if (gui_control_should_handle_button(mbut) || (game.Guis[gui_id].GetControlType(over_ctrl) == kGUIInvWindow))
        {
            game.Guis[gui_id].OnMouseButtonDown(mx, my);
        }
    }
    else
    {
        // run GUI click handler if not on any control
        if (game.Guis[gui_id].GetEvents().GetHandler(kGUIEvent_OnClick).IsEnabled())
            force_event(AGSEvent_GUI(gui_id, -1, static_cast<eAGSMouseButton>(mbut)));
    }

    run_on_event(kScriptEvent_GUIMouseDown, gui_id, mbut, mx - game.Guis[gui_id].GetX(), my - game.Guis[gui_id].GetY());
}

GUITextBox *gui_get_active_textbox()
{
    Game &game = Game::Current();
    if (!GUI::IsEnabledState())
        return nullptr;

    for (auto &gui : game.Guis)
    {
        if (!gui.IsDisplayed())
            continue;

        for (int i = 0; i < gui.GetControlCount(); ++i)
        {
            if (gui.GetControlType(i) == kGUITextBox)
            {
                auto *textbox = static_cast<GUITextBox *>(gui.GetControl(i));
                if (textbox && textbox->IsEnabled() && textbox->IsVisible())
                    return textbox;
            }
        }
    }
    return nullptr;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"


ScriptGUI *GUI_GetByName(const char *name)
{
    return static_cast<ScriptGUI *>(ccGetScriptObjectAddress(name, ccDynamicGUI.GetType()));
}

GUITextBox *GUI_GetActiveTextInputControl()
{
    return gui_get_active_textbox();
}

ScriptUserObject *GUI_ScreenToGUIPoint(ScriptGUI *sgui, int screen_x, int screen_y, bool clip_to_gui)
{
    GUIMain &gui = Game::Current().Guis[sgui->id];
    Point pt = gui.GetGraphicSpace().WorldToLocal(screen_x, screen_y);
    if (clip_to_gui && !RectWH(gui.GetX(), gui.GetY(), gui.GetWidth(), gui.GetHeight()).IsInside(pt))
        return nullptr;
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

ScriptUserObject *GUI_GUIToScreenPoint(ScriptGUI *sgui, int gui_x, int gui_y, bool clip_to_gui)
{
    GUIMain &gui = Game::Current().Guis[sgui->id];
    if (clip_to_gui && !RectWH(gui.GetX(), gui.GetY(), gui.GetWidth(), gui.GetHeight()).IsInside(gui_x, gui_y))
        return nullptr;
    Point pt = gui.GetGraphicSpace().LocalToWorld(gui_x, gui_y);
    return ScriptStructHelpers::CreatePoint(pt.X, pt.Y);
}

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int FindGUIID(const char *GUIName)
{
    Game &game = Game::Current();
    for (int ii = 0; ii < game.GuiCount; ii++)
    {
        if (game.Guis[ii].GetName().IsEmpty())
            continue;
        if (game.Guis[ii].GetName() == GUIName)
            return ii;
        if ((game.Guis[ii].GetName()[0u] == 'g') && (ags_stricmp(game.Guis[ii].GetName().GetCStr() + 1, GUIName) == 0))
            return ii;
    }
    quit("FindGUIID: No matching GUI found: GUI may have been deleted");
    return -1;
}

void InterfaceOn(int ifn)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if ((ifn < 0) | (ifn >= game.GuiCount))
        quit("!GUIOn: invalid GUI specified");

    EndSkippingUntilCharStops();

    if (game.Guis[ifn].IsVisible())
    {
        return;
    }
    game.Guis[ifn].SetVisible(true);
    debug_script_log("GUI %d turned on", ifn);
    // modal interface
    if (game.Guis[ifn].GetPopupStyle() == kGUIPopupModal)
        PauseGame();
    if (game.Play.mouse_input_enabled)
        game.Guis[ifn].Poll(services.Input.MouseX, services.Input.MouseY,
                            GetGUIAt(services.Input.MouseX, services.Input.MouseY, kHit_Interactable) == ifn);
}

void InterfaceOff(int ifn)
{
    Game &game = Game::Current();
    if ((ifn < 0) | (ifn >= game.GuiCount))
        quit("!GUIOff: invalid GUI specified");
    if (!game.Guis[ifn].IsVisible())
    {
        return;
    }
    debug_script_log("GUI %d turned off", ifn);
    game.Guis[ifn].SetVisible(false);
    // modal interface
    if (game.Guis[ifn].GetPopupStyle() == kGUIPopupModal)
        UnPauseGame();
}

void DisableInterfaceEx(bool update_cursor)
{
    Game &game = Game::Current();
    game.Play.disabled_user_interface++;
    if (game.Play.disabled_user_interface == 1) // if just switched to disabled
    {
        // If GUI looks change when disabled, then mark all of them for redraw
        GUIE::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
        // Also mark "overhotspot" labels for update, as their text gets reset
        // to empty string when the interface is disabled
        GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
    }
    // We update cursor even if interface was already disabled prior,
    // because disabled status may had been toggled by internal game logic too
    if (update_cursor)
    {
        set_cursor_look(game.GetModeWait());
    }
}

void DisableInterface()
{
    DisableInterfaceEx(true);
}

void EnableInterfaceEx(bool update_cursor)
{
    Game &game = Game::Current();
    game.Play.disabled_user_interface--;
    if (game.Play.disabled_user_interface < 1) // if just switched to enabled
    {
        game.Play.disabled_user_interface = 0; // clamp to 0
        // If GUI looks change when disabled, then mark all of them for redraw
        GUIE::MarkAllGUIForUpdate(GUI::Options.DisabledStyle != kGuiDis_Unchanged, true);
        // Also mark "overhotspot" labels for update, as their text gets reset
        // to empty string when the interface is disabled
        GUIE::MarkSpecialLabelsForUpdate(kLabelMacro_Overhotspot);
        if (update_cursor)
        {
            set_default_cursor_look();
        }
    }
}

void EnableInterface()
{
    EnableInterfaceEx(true);
}

// Returns 1 if user interface is enabled, 0 if disabled
int IsInterfaceEnabled()
{
    return (Game::Current().Play.disabled_user_interface > 0) ? 0 : 1;
}

void SetTextWindowGUI(int guinum)
{
    Game &game = Game::Current();
    if ((guinum < -1) | (guinum >= game.GuiCount))
        quit("!SetTextWindowGUI: invalid GUI number");

    if (guinum < 0)
        ; // disable it
    else if (!game.Guis[guinum].IsTextWindow())
        quit("!SetTextWindowGUI: specified GUI is not a text window");

    if (game.Play.speech_textwindow_gui == game.Options[OPT_TWCUSTOM])
        game.Play.speech_textwindow_gui = guinum;
    game.Options[OPT_TWCUSTOM] = guinum;
}
