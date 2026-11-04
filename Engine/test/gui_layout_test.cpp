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
// GUI layout as the engine computes it: the screen area a GUI covers for its
// position and scale, which GUI and which control is under a screen point
// (draw order, visibility, clickability, scaled GUIs and controls), and
// centring a GUI on the screen.
//
//=============================================================================
#include <memory>
#include <vector>

#include "ac/dynobj/scriptobjects.h"
#include "ac/gamestate.h"
#include "ac/gui.h"
#include "ac/runtime_defines.h"
#include "gtest/gtest.h"
#include "gui/guibutton.h"
#include "gui/guimain.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern std::vector<GUIMain> guis;

namespace
{

const int kScreenWidth = 320;
const int kScreenHeight = 200;

} // namespace

// Fixture for the GUI layout. The engine globals it touches, all reset in TearDown:
// - guis: two GUIs, 0 at (10,20) 100x50 and 1 at (60,40) 100x50, both visible and clickable;
//   GUI 0 has two buttons, (10,10) 30x20 and (30,15) 30x20, the second drawn on top
// - play.gui_draw_order: GUI 0 below GUI 1
// - play's UI viewport: 320x200
// - GUI::Options.ClipControls: on, so control areas do not depend on fonts and sprites
// - GUI::Context.DisabledState: GUIs enabled
class GuiLayout : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _wasClipControls = GUI::Options.ClipControls;
        GUI::Options.ClipControls = true;
        GUI::Context.DisabledState = kGuiDis_Undefined;
        play.SetUIViewport(RectWH(0, 0, kScreenWidth, kScreenHeight));

        guis.resize(2);
        for (int i = 0; i < 2; ++i)
        {
            guis[i].SetID(i);
            guis[i].SetVisible(true);
            guis[i].SetClickable(true);
            guis[i].SetSize(100, 50);
        }
        guis[0].SetPosition(10, 20);
        guis[1].SetPosition(60, 40);
        play.gui_draw_order = {0, 1};

        for (int i = 0; i < 2; ++i)
        {
            _buttons[i].SetID(i);
            _buttons[i].SetParentID(0);
            _buttons[i].SetVisible(true);
            _buttons[i].SetClickable(true);
            _buttons[i].SetSize(30, 20);
            _buttons[i].SetZOrder(i);
            guis[0].AddControl(kGUIButton, i, &_buttons[i]);
        }
        _buttons[0].SetPosition(10, 10);
        _buttons[1].SetPosition(30, 15);
        guis[0].ResortZOrder();
    }

    void TearDown() override
    {
        guis.clear();
        play.gui_draw_order.clear();
        play.SetUIViewport(Rect());
        GUI::Options.ClipControls = _wasClipControls;
    }

    bool _wasClipControls = true;
    GUIButton _buttons[2];
};

TEST_F(GuiLayout, GuiCoversItsRectangle)
{
    EXPECT_EQ(Rect(10, 20, 109, 69), guis[0].GetGraphicSpace().AABB());
    EXPECT_TRUE(guis[0].IsInteractableAt(10, 20));
    EXPECT_TRUE(guis[0].IsInteractableAt(109, 69));
    EXPECT_FALSE(guis[0].IsInteractableAt(110, 69));
    EXPECT_FALSE(guis[0].IsInteractableAt(109, 70));
    EXPECT_FALSE(guis[0].IsInteractableAt(9, 20));
}

TEST_F(GuiLayout, ScaledGuiCoversTheScaledArea)
{
    guis[0].SetScale(2.f, 2.f);
    EXPECT_EQ(Size(100, 50), guis[0].GetSize());
    const Rect &aabb = guis[0].GetGraphicSpace().AABB();
    EXPECT_EQ(10, aabb.Left);
    EXPECT_EQ(20, aabb.Top);
    EXPECT_EQ(10 + 99 * 2, aabb.Right);
    EXPECT_EQ(20 + 49 * 2, aabb.Bottom);
    EXPECT_TRUE(guis[0].IsInteractableAt(200, 110));
    EXPECT_FALSE(guis[0].IsInteractableAt(211, 110));
}

TEST_F(GuiLayout, HiddenOrUnclickableGuiIsNotInteractable)
{
    guis[0].SetClickable(false);
    EXPECT_FALSE(guis[0].IsInteractableAt(50, 30));
    guis[0].SetClickable(true);
    guis[0].SetVisible(false);
    EXPECT_FALSE(guis[0].IsInteractableAt(50, 30));
    guis[0].SetVisible(true);
    guis[0].SetConceal(true);
    EXPECT_FALSE(guis[0].IsInteractableAt(50, 30));
}

TEST_F(GuiLayout, GuiAtAPointFollowsTheDrawOrder)
{
    // only GUI 0
    EXPECT_EQ(0, GetGUIAt(20, 25, kHit_Interactable));
    // both: the one drawn on top
    EXPECT_EQ(1, GetGUIAt(80, 50, kHit_Interactable));
    play.gui_draw_order = {1, 0};
    EXPECT_EQ(0, GetGUIAt(80, 50, kHit_Interactable));
    // none
    EXPECT_EQ(-1, GetGUIAt(300, 190, kHit_Interactable));
}

TEST_F(GuiLayout, GuiAtAPointSkipsHiddenAndUnclickableGuis)
{
    guis[1].SetClickable(false);
    EXPECT_EQ(0, GetGUIAt(80, 50, kHit_Interactable));
    // "any" hit includes GUIs that are not clickable
    EXPECT_EQ(1, GetGUIAt(80, 50, kHit_Any));
    guis[1].SetVisible(false);
    EXPECT_EQ(0, GetGUIAt(80, 50, kHit_Any));
}

TEST_F(GuiLayout, CentreUsesTheUiViewport)
{
    ScriptGUI sgui;
    sgui.id = 0;
    GUI_Centre(&sgui);
    EXPECT_EQ(Point(kScreenWidth / 2 - 50, kScreenHeight / 2 - 25), guis[0].GetPosition());
    // odd sizes round down
    guis[0].SetSize(101, 51);
    GUI_Centre(&sgui);
    EXPECT_EQ(Point(110, 75), guis[0].GetPosition());
    // a GUI larger than the screen goes off its top-left
    guis[0].SetSize(400, 300);
    GUI_Centre(&sgui);
    EXPECT_EQ(Point(-40, -50), guis[0].GetPosition());
}

TEST_F(GuiLayout, ControlAtAPointUsesTheGuiCoordinates)
{
    // button 0 covers GUI-local (10,10)-(39,29), screen (20,30)-(49,49)
    EXPECT_EQ(0, guis[0].FindControlAt(20, 30));
    EXPECT_EQ(0, guis[0].FindControlAt(39, 34));
    // button 1, on top, covers local (30,15)-(59,34), screen (40,35)-(69,54)
    EXPECT_EQ(1, guis[0].FindControlAt(45, 40));
    EXPECT_EQ(1, guis[0].FindControlAt(69, 54));
    EXPECT_EQ(-1, guis[0].FindControlAt(70, 55));
    EXPECT_EQ(-1, guis[0].FindControlAt(15, 25));
}

TEST_F(GuiLayout, ControlAtAPointFollowsTheControlZOrder)
{
    EXPECT_EQ(1, guis[0].FindControlAt(45, 40));
    guis[0].BringControlToFront(0);
    EXPECT_EQ(0, guis[0].FindControlAt(45, 40));
}

TEST_F(GuiLayout, ControlAtAPointSkipsHiddenAndUnclickableControls)
{
    _buttons[1].SetVisible(false);
    EXPECT_EQ(0, guis[0].FindControlAt(45, 40));
    _buttons[1].SetVisible(true);
    _buttons[1].SetClickable(false);
    EXPECT_EQ(0, guis[0].FindControlAt(45, 40));
    // unless any control is asked for
    EXPECT_EQ(1, guis[0].FindControlAt(45, 40, 0, false));
}

TEST_F(GuiLayout, ControlsOfAScaledGuiAreScaled)
{
    guis[0].SetScale(2.f, 2.f);
    // button 0 at local (10,10) is at screen (10 + 20, 20 + 20)
    EXPECT_EQ(-1, guis[0].FindControlAt(25, 35));
    EXPECT_EQ(0, guis[0].FindControlAt(30, 40));
    EXPECT_EQ(0, guis[0].FindControlAt(68, 58));
    // button 1 covers local x up to 59: screen up to 10 + 59 * 2 (+1 inside the pixel)
    EXPECT_EQ(1, guis[0].FindControlAt(129, 89));
    EXPECT_EQ(-1, guis[0].FindControlAt(131, 91));
}

TEST_F(GuiLayout, ScaledControlCoversMore)
{
    _buttons[0].SetScale(2.f, 2.f);
    // local (10,10) + 2 * (30,20): x up to 10 + 29 * 2, inside the GUI
    EXPECT_EQ(0, guis[0].FindControlAt(10 + 10 + 58, 20 + 12));
    EXPECT_EQ(-1, guis[0].FindControlAt(10 + 10 + 62, 20 + 12));
}
