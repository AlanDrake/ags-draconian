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
// Savegame components with game content, which savegame_test.cpp leaves in
// the empty state of an engine without a game: characters and inventory
// items with non-default state are saved, cleared and restored; a save made
// for a game with a different number of them is refused, or accepted with the
// restore flags that allow it; prescanning a save counts its content without
// changing the game.
//
// Not tested: kSaveRestore_AllowMismatchExtra (more objects in the save than in
// the game). The engine never sets it, and with it ReadCharacters writes the
// extra characters past the end of game.chars.
//
//=============================================================================
#include <memory>
#include <string>
#include <vector>

#include "ac/characterextras.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "game/savegame.h"
#include "game/savegame_components.h"
#include "game/savegame_internal.h"
#include "gtest/gtest.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern std::vector<CharacterExtras> charextra;

namespace
{

// Tells whether the error, or one that caused it, has this code (component errors wrap their cause)
bool HasErrorCode(const HSaveError &err, SavegameErrorType code)
{
    if (err->Code() == code)
        return true;
    for (PError cause = err->PreviousError(); cause; cause = cause->PreviousError())
    {
        if (cause->Code() == code)
            return true;
    }
    return false;
}

const SaveCmpSelection kCharactersAndItems = static_cast<SaveCmpSelection>(kSaveCmp_Characters | kSaveCmp_InvItems);

} // namespace

// Fixture for the savegame components with game content. The engine globals it touches, all reset in TearDown:
// - game.chars, game.numcharacters, charextra, play.charProps: the characters
// - game.invinfo, game.numinvitems, play.invProps: the inventory items
class SavegameContent : public ::testing::Test
{
protected:
    void TearDown() override { SetCounts(0, 0); }

    // Resizes the game's characters and inventory items, all in their default state
    static void SetCounts(int characters, int items)
    {
        game.chars.clear();
        game.chars.resize(characters);
        game.numcharacters = characters;
        charextra.clear();
        charextra.resize(characters);
        play.charProps.clear();
        play.charProps.resize(characters);
        game.invinfo.clear();
        game.invinfo.resize(items);
        game.numinvitems = static_cast<uint16_t>(items);
        play.invProps.clear();
        play.invProps.resize(items);
    }

    // State that no new game has
    static void SetState()
    {
        for (int i = 0; i < game.numcharacters; ++i)
        {
            auto &ch = game.chars[i];
            ch.x = 100 + i;
            ch.y = 150 + i;
            ch.room = 3 + i;
            ch.prevroom = 2;
            ch.view = 4;
            ch.loop = 2;
            ch.frame = 1;
            ch.z = -5;
            ch.activeinv = 1;
            ch.walkspeed = 6;
            ch.opacity = 128;
            ch.flags = 0x41;
            ch.name = String::FromFormat("Character %d", i);
            ch.set_item_quantity(1, 2);
            ch.set_item_quantity(4, 1);
            ch.set_item_quantity(9, 7);
            auto &ex = charextra[i];
            ex.inventory = {1, 4, 9};
            ex.zoom = 80;
            ex.tint_r = 10;
            ex.tint_level = 50;
            ex.following = 0;
            play.charProps[i]["Mood"] = "angry";
        }
        for (int i = 0; i < game.numinvitems; ++i)
        {
            game.invinfo[i].name = String::FromFormat("Item %d", i);
            game.invinfo[i].pic = 20 + i;
            game.invinfo[i].cursorPic = 40 + i;
            game.invinfo[i].hotx = 3;
            game.invinfo[i].hoty = 4;
            play.invProps[i]["Weight"] = "12";
        }
    }

    static bool Save(SaveCmpSelection cmp, std::vector<uint8_t> &buf, std::string &error)
    {
        buf.clear();
        Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
        HSaveError err = SavegameComponents::WriteAllCommon(&out, cmp, false);
        if (!err)
            error = err->FullMessage().GetCStr();
        return static_cast<bool>(err);
    }

    static HSaveError Restore(const std::vector<uint8_t> &buf, SaveCmpSelection cmp, RestoredData &r_data)
    {
        Stream in(std::make_unique<VectorStream>(buf));
        PreservedParams pp;
        return SavegameComponents::ReadAll(&in, kSvgVersion_Current, cmp, pp, r_data);
    }
};

TEST_F(SavegameContent, CharactersAndItemsAreRestored)
{
    SetCounts(3, 2);
    SetState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kCharactersAndItems, saved, error)) << error;

    SetCounts(3, 2);
    RestoredData r_data;
    HSaveError err = Restore(saved, kCharactersAndItems, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i < 3; ++i)
    {
        const auto &ch = game.chars[i];
        EXPECT_EQ(100 + i, ch.x);
        EXPECT_EQ(150 + i, ch.y);
        EXPECT_EQ(3 + i, ch.room);
        EXPECT_EQ(2, ch.prevroom);
        EXPECT_EQ(4, ch.view);
        EXPECT_EQ(2, ch.loop);
        EXPECT_EQ(1, ch.frame);
        EXPECT_EQ(-5, ch.z);
        EXPECT_EQ(1, ch.activeinv);
        EXPECT_EQ(6, ch.walkspeed);
        EXPECT_EQ(128, ch.opacity);
        EXPECT_EQ(0x41, ch.flags);
        EXPECT_STREQ(String::FromFormat("Character %d", i).GetCStr(), ch.name.GetCStr());
        EXPECT_EQ(2, ch.get_item_quantity(1));
        EXPECT_EQ(1, ch.get_item_quantity(4));
        EXPECT_EQ(7, ch.get_item_quantity(9));
        EXPECT_EQ(0, ch.get_item_quantity(2));
        EXPECT_EQ((std::vector<int>{1, 4, 9}), charextra[i].inventory);
        EXPECT_EQ(80, charextra[i].zoom);
        EXPECT_EQ(10, charextra[i].tint_r);
        EXPECT_EQ(50, charextra[i].tint_level);
        EXPECT_STREQ("angry", play.charProps[i]["Mood"].GetCStr());
    }
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_STREQ(String::FromFormat("Item %d", i).GetCStr(), game.invinfo[i].name.GetCStr());
        EXPECT_EQ(20 + i, game.invinfo[i].pic);
        EXPECT_EQ(40 + i, game.invinfo[i].cursorPic);
        EXPECT_EQ(3, game.invinfo[i].hotx);
        EXPECT_EQ(4, game.invinfo[i].hoty);
        EXPECT_STREQ("12", play.invProps[i]["Weight"].GetCStr());
    }
    EXPECT_EQ(3u, r_data.DataCounts.Characters);
    EXPECT_EQ(2u, r_data.DataCounts.InventoryItems);
}

TEST_F(SavegameContent, SaveWithFewerCharactersIsRefused)
{
    SetCounts(2, 2);
    SetState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, saved, error)) << error;

    SetCounts(3, 2);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
    ASSERT_FALSE(err);
    EXPECT_TRUE(HasErrorCode(err, kSvgErr_GameContentAssertion));
    EXPECT_NE(std::string::npos,
              std::string(err->FullMessage().GetCStr()).find("Mismatching number of Characters (game: 3, save: 2)"))
        << err->FullMessage().GetCStr();
}

TEST_F(SavegameContent, SaveWithMoreCharactersIsRefused)
{
    SetCounts(3, 2);
    SetState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, saved, error)) << error;

    SetCounts(2, 2);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
    ASSERT_FALSE(err);
    EXPECT_TRUE(HasErrorCode(err, kSvgErr_GameContentAssertion));
}

TEST_F(SavegameContent, FewerCharactersAllowedOnACleanGame)
{
    SetCounts(2, 2);
    SetState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, saved, error)) << error;

    // allowed, but only on a clean game: asks to retry with one
    SetCounts(3, 2);
    RestoredData r_data;
    r_data.Result.RestoreFlags = kSaveRestore_AllowMismatchLess;
    HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
    ASSERT_FALSE(err);
    EXPECT_TRUE(r_data.Result.Feedback.RetryWithClearGame);

    // on a clean game the save's characters are restored, the rest keeps the defaults
    SetCounts(3, 2);
    RestoredData clean_data;
    clean_data.Result.RestoreFlags =
        static_cast<SaveRestorationFlags>(kSaveRestore_AllowMismatchLess | kSaveRestore_ClearData);
    err = Restore(saved, kSaveCmp_Characters, clean_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_NE(0, clean_data.Result.RestoreFlags & kSaveRestore_MissingDataInSave);
    EXPECT_FALSE(clean_data.Result.FirstMismatchError.IsEmpty());
    EXPECT_EQ(2u, clean_data.DataCounts.Characters);
    EXPECT_EQ(101, game.chars[1].x);
    EXPECT_EQ(0, game.chars[2].x);
}

TEST_F(SavegameContent, PrescanCountsWithoutRestoring)
{
    SetCounts(3, 2);
    SetState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kCharactersAndItems, saved, error)) << error;

    SetCounts(3, 2);
    Stream in(std::make_unique<VectorStream>(saved));
    PreservedParams pp;
    RestoredData r_data;
    r_data.Result.RestoreFlags = kSaveRestore_ClearData;
    HSaveError err = SavegameComponents::PrescanAll(&in, kSvgVersion_Current, kCharactersAndItems, pp, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(3u, r_data.DataCounts.Characters);
    EXPECT_EQ(2u, r_data.DataCounts.InventoryItems);
    // the game is untouched
    EXPECT_EQ(0, game.chars[0].x);
    EXPECT_TRUE(game.invinfo[0].name.IsEmpty());
}
