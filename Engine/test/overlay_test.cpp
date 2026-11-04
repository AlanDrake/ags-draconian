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
// Overlay layout as the engine computes it: where a ScreenOverlay's graphic
// lands for its position, sprite offset, anchor, size, scale and rotation, and
// where speech is placed above a character (autoposition_overlay), including
// the clamping at the screen edges and the camera scroll.
//
//=============================================================================
#include <memory>
#include <vector>

#include "ac/characterextras.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/overlay.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/view.h"
#include "game/roomstruct.h"
#include "game/viewport.h"
#include "gfx/bitmap.h"
#include "gtest/gtest.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern RoomStruct thisroom;
extern std::vector<ViewStruct> views;
extern std::vector<CharacterExtras> charextra;
extern int displayed_room;

namespace
{

// Sprites of the fixture: number, width, height
const int kSprite20x10 = 1;
const int kSprite21x11 = 2;
const int kSprite41x21 = 3;
const int kCharacterSprite = 4; // 30 x 50
const int kSpeechSprite = 5;    // 40 x 20

const int kScreenWidth = 320;
const int kScreenHeight = 200;
const int kRoom = 7;

} // namespace

// Fixture for the overlay layout. The engine globals it touches, all reset in TearDown:
// - spriteset and game.SpriteInfos: the sprites above
// - play: the main and UI viewports (320x200), one room viewport and its camera
// - thisroom: the room size (640x400, so the camera can scroll)
// - game.chars, game.numcharacters, charextra, views: one character standing in room kRoom,
//   with a one-frame view
// - displayed_room: kRoom
class OverlayLayout : public ::testing::Test
{
protected:
    void SetUp() override
    {
        AddSprite(kSprite20x10, 20, 10);
        AddSprite(kSprite21x11, 21, 11);
        AddSprite(kSprite41x21, 41, 21);
        AddSprite(kCharacterSprite, 30, 50);
        AddSprite(kSpeechSprite, 40, 20);

        thisroom.Width = 640;
        thisroom.Height = 400;
        play.SetMainViewport(RectWH(0, 0, kScreenWidth, kScreenHeight));
        play.SetUIViewport(RectWH(0, 0, kScreenWidth, kScreenHeight));
        auto viewport = play.CreateRoomViewport();
        auto camera = play.CreateRoomCamera();
        viewport->LinkCamera(camera);
        camera->LinkToViewport(viewport);

        views.resize(1);
        views[0].Initialize(1);
        views[0].loops[0].Initialize(1);
        views[0].loops[0].frames[0].pic = kCharacterSprite;
        game.chars.resize(1);
        game.numcharacters = 1;
        game.chars[0].view = 0;
        game.chars[0].loop = 0;
        game.chars[0].room = kRoom;
        charextra.resize(1);
        displayed_room = kRoom;
    }

    void TearDown() override
    {
        displayed_room = -10;
        charextra.clear();
        game.chars.clear();
        game.numcharacters = 0;
        views.clear();
        play.FreeViewportsAndCameras();
        play.SetUIViewport(Rect());
        play.SetMainViewport(Rect());
        thisroom.Width = 0;
        thisroom.Height = 0;
        spriteset.Reset();
        game.SpriteInfos.clear();
    }

    static void AddSprite(int index, int width, int height)
    {
        ASSERT_TRUE(spriteset.SetSprite(index, std::make_unique<Bitmap>(width, height, 32)));
    }

    // A speech overlay for the character, placed as the engine places it
    static Point AutoPosition(int char_x, int char_y)
    {
        game.chars[0].x = char_x;
        game.chars[0].y = char_y;
        ScreenOverlay over(1);
        over.SetSpriteNum(kSpeechSprite);
        over.SetAutoPosition(0);
        autoposition_overlay(over);
        return Point(over.GetX(), over.GetY());
    }
};

TEST_F(OverlayLayout, SpriteAtTheOverlayPosition)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10);
    over.SetFixedPosition(50, 40);
    over.UpdateGraphicSpace();
    EXPECT_EQ(Size(20, 10), over.GetGraphicSize());
    EXPECT_EQ(Size(20, 10), over.GetDestinationSize());
    EXPECT_EQ(Size(20, 10), over.GetScaledSize());
    EXPECT_EQ(Point(50, 40), over.GetGraphicSpace().TopLeft());
    EXPECT_EQ(Rect(50, 40, 69, 49), over.GetGraphicSpace().AABB());
}

TEST_F(OverlayLayout, SpriteOffsetMovesTheGraphicNotThePosition)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10, 3, -2);
    over.SetFixedPosition(50, 40);
    over.UpdateGraphicSpace();
    EXPECT_EQ(50, over.GetX());
    EXPECT_EQ(40, over.GetY());
    EXPECT_EQ(Point(53, 38), over.GetDrawPos());
    EXPECT_EQ(Rect(53, 38, 72, 47), over.GetGraphicSpace().AABB());
}

TEST_F(OverlayLayout, AnchorAlignsTheSpriteToThePosition)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite21x11);
    over.SetFixedPosition(100, 100);
    // bottom centre
    over.SetSpriteAnchor(Pointf(0.5f, 1.f));
    over.UpdateGraphicSpace();
    EXPECT_EQ(Rect(90, 90, 110, 100), over.GetGraphicSpace().AABB());
    // bottom right
    over.SetSpriteAnchor(Pointf(1.f, 1.f));
    over.UpdateGraphicSpace();
    EXPECT_EQ(Rect(80, 90, 100, 100), over.GetGraphicSpace().AABB());
    // the anchor is clamped to the sprite
    over.SetSpriteAnchor(Pointf(2.f, -1.f));
    EXPECT_EQ(Pointf(1.f, 0.f), over.GetSpriteAnchor());
}

TEST_F(OverlayLayout, ScaleMultipliesTheSize)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10);
    over.SetFixedPosition(50, 40);
    over.SetScale(2.f, 3.f);
    over.UpdateGraphicSpace();
    EXPECT_EQ(Size(20, 10), over.GetDestinationSize());
    EXPECT_EQ(Size(40, 30), over.GetScaledSize());
    // the box is the transformed corner pixels: the last pixel's origin, 19 * 2 and 9 * 3 away
    EXPECT_EQ(Rect(50, 40, 88, 67), over.GetGraphicSpace().AABB());
}

TEST_F(OverlayLayout, ScaleWithAnAnchor)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite21x11);
    over.SetFixedPosition(100, 100);
    over.SetSpriteAnchor(Pointf(0.5f, 1.f));
    over.SetScale(2.f, 2.f);
    over.UpdateGraphicSpace();
    EXPECT_EQ(Size(42, 22), over.GetScaledSize());
    // the anchor point stays at the position: (42 - 1) / 2 to the left, 22 - 1 up
    const Rect &aabb = over.GetGraphicSpace().AABB();
    EXPECT_EQ(79, aabb.Left);
    EXPECT_EQ(79, aabb.Top);
    EXPECT_EQ(119, aabb.Right);
    EXPECT_EQ(99, aabb.Bottom);
}

TEST_F(OverlayLayout, DestinationSizeTurnsAutoSizeOff)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10);
    EXPECT_TRUE(over.IsAutoSize());
    // auto size follows the sprite
    over.SetSpriteNum(kSprite41x21);
    EXPECT_EQ(Size(41, 21), over.GetDestinationSize());
    // a fixed size does not
    over.SetDestinationSize(30, 15);
    EXPECT_FALSE(over.IsAutoSize());
    over.SetSpriteNum(kSprite20x10);
    EXPECT_EQ(Size(30, 15), over.GetDestinationSize());
    over.SetScale(2.f, 2.f);
    EXPECT_EQ(Size(60, 30), over.GetScaledSize());
    // turning auto size back on takes the sprite's size again
    over.SetAutoSize(true);
    EXPECT_EQ(Size(20, 10), over.GetDestinationSize());
    EXPECT_EQ(Size(40, 20), over.GetScaledSize());
}

TEST_F(OverlayLayout, StretchedSpriteFillsTheDestinationSize)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10);
    over.SetFixedPosition(0, 0);
    over.SetDestinationSize(60, 20);
    over.UpdateGraphicSpace();
    EXPECT_EQ(Rect(0, 0, 57, 18), over.GetGraphicSpace().AABB());
}

TEST_F(OverlayLayout, RotationTurnsTheBoxAroundTheCentre)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite21x11);
    over.SetFixedPosition(100, 100);
    over.SetRotation(90.f);
    over.UpdateGraphicSpace();
    const Rect &aabb = over.GetGraphicSpace().AABB();
    // a 21x11 box turned by a quarter is 11x21, around the same centre
    EXPECT_NEAR(10, aabb.GetWidth() - 1, 1);
    EXPECT_NEAR(20, aabb.GetHeight() - 1, 1);
    EXPECT_NEAR(110, (aabb.Left + aabb.Right) / 2, 1);
    EXPECT_NEAR(105, (aabb.Top + aabb.Bottom) / 2, 1);
    // half a turn keeps the box
    over.SetRotation(180.f);
    over.UpdateGraphicSpace();
    EXPECT_NEAR(20, over.GetGraphicSpace().AABB().GetWidth() - 1, 1);
    EXPECT_NEAR(10, over.GetGraphicSpace().AABB().GetHeight() - 1, 1);
}

TEST_F(OverlayLayout, PivotOffsetIsRelativeToTheScaledSize)
{
    ScreenOverlay over(1);
    over.SetSpriteNum(kSprite20x10);
    over.SetPivotOffset(Point(5, -5));
    EXPECT_EQ(Pointf(0.75f, 0.f), over.GetEffectivePivot());
    over.SetScale(2.f, 2.f);
    EXPECT_EQ(Pointf(0.625f, 0.25f), over.GetEffectivePivot());
}

TEST_F(OverlayLayout, SpeechIsCentredAboveTheCharacter)
{
    // the character's sprite is 30x50 and its feet at (100,150): the head is at y 100;
    // the 40x20 speech goes 5 pixels above the head, centred on the character
    EXPECT_EQ(Point(80, 75), AutoPosition(100, 150));
}

TEST_F(OverlayLayout, SpeechStaysOnTheScreen)
{
    // left edge
    EXPECT_EQ(Point(0, 75), AutoPosition(5, 150));
    // right edge: one pixel off the edge
    EXPECT_EQ(Point(kScreenWidth - 40 - 1, 75), AutoPosition(kScreenWidth - 5, 150));
    // top: at least 5 pixels down
    EXPECT_EQ(Point(80, 5), AutoPosition(100, 60));
}

TEST_F(OverlayLayout, SpeechFollowsTheCamera)
{
    play.GetRoomCamera(0)->SetAt(60, 30);
    EXPECT_EQ(Point(80 - 60, 75 - 30), AutoPosition(100, 150));
}

TEST_F(OverlayLayout, SpeechUsesTheCharacterHeightAndZ)
{
    // a character's height, when known, is used instead of its sprite's
    charextra[0].height = 80;
    EXPECT_EQ(Point(80, 45), AutoPosition(100, 150));
    charextra[0].height = 0;
    // a flying character
    game.chars[0].z = 30;
    EXPECT_EQ(Point(80, 45), AutoPosition(100, 150));
    game.chars[0].z = 0;
}

TEST_F(OverlayLayout, SpeechOfACharacterInAnotherRoomIsCentred)
{
    game.chars[0].room = kRoom + 1;
    EXPECT_EQ(Point(kScreenWidth / 2 - 20, kScreenHeight / 2 - 10), AutoPosition(100, 150));
}
