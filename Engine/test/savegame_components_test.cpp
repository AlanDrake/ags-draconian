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
// the empty state of an engine without a game: characters, inventory items,
// dialog options, mouse cursors, audio types and the game setup's saved
// fields with non-default state are saved, cleared and restored; a save made
// for a game with a different number of them is refused, or accepted with the
// restore flags that allow it; prescanning a save counts its content without
// changing the game; a restored game state leaves the running engine's own
// audio pack flag alone. Single objects restored from a hand-made stream: an
// animated overlay and a room state.
//
// Not tested: kSaveRestore_AllowMismatchExtra (more objects in the save than in
// the game). The engine never sets it, and with it ReadCharacters writes the
// extra characters past the end of game.Characters.
//
//=============================================================================
#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "game/customproperties.h"
#include "game/dialog/dialog.h"
#include "game/dialog_def.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/screen_overlay.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/view_def.h"
#include "game/world/character.h"
#include "game/world/room_state.h"
#include "gtest/gtest.h"
#include "platform/base/engine_services.h"
#include "util/field_table.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

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
// - Game::Characters, game.CharacterCount, game.Play.charProps: the characters
// - game.InventoryItems, game.InventoryItemCount, game.Play.invProps: the inventory items
// - Game::Views, game.ViewCount: the views
// - Game::Dialogs, game.DialogCount, game.Play.dialogProps: the dialogs
// - game.MouseCursors, game.MouseCursorCount: the mouse cursors
// - game.AudioClipTypes, game.Play.default_audio_type_volumes: the audio types
// - the game's live options and values that the game state saves (GameSetupFields)
// - EngineServices::Current().SeparateMusicLib
class SavegameContent : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _separateMusicLib = EngineServices::Current().SeparateMusicLib;
        _gameSetup = GameSetupFields::Get();
    }

    void TearDown() override
    {
        SetCounts(0, 0);
        SetViews(0, 0, 0);
        SetDialogs(0, 0);
        SetCursors(0);
        SetAudioTypes(0);
        _gameSetup.Set();
        EngineServices::Current().SeparateMusicLib = _separateMusicLib;
    }

    // The game's live options and values that the game state component saves
    struct GameSetupFields
    {
        std::array<int, GameDef::MAX_OPTIONS> Options = {};
        int PlayerCharacter = 0;
        int DialogBullet = 0;
        int InvHotColor = 0;
        int InvHotCrossColor = 0;
        int InvHotSprite = 0;
        int DefaultLipsyncFrame = 0;

        static GameSetupFields Get()
        {
            Game &game = Game::Current();
            GameSetupFields fields;
            std::copy_n(game.Options, GameDef::MAX_OPTIONS, fields.Options.begin());
            fields.PlayerCharacter = game.PlayerCharacterID;
            fields.DialogBullet = game.DialogBulletSprite;
            fields.InvHotColor = game.InventoryCursorHotspotColor;
            fields.InvHotCrossColor = game.InventoryCursorHotspotCrossColor;
            fields.InvHotSprite = game.InventoryCursorHotspotSprite;
            fields.DefaultLipsyncFrame = game.DefaultLipSyncFrame;
            return fields;
        }

        void Set() const
        {
            Game &game = Game::Current();
            std::copy(Options.begin(), Options.end(), game.Options);
            game.PlayerCharacterID = PlayerCharacter;
            game.DialogBulletSprite = DialogBullet;
            game.InventoryCursorHotspotColor = InvHotColor;
            game.InventoryCursorHotspotCrossColor = InvHotCrossColor;
            game.InventoryCursorHotspotSprite = InvHotSprite;
            game.DefaultLipSyncFrame = DefaultLipsyncFrame;
        }
    };

    bool _separateMusicLib = false;
    GameSetupFields _gameSetup;

    // Makes the game's dialogs, each with the same number of options, all off; option texts are "Dialog d option o"
    static void SetDialogs(int dialog_count, int option_count)
    {
        Game &game = Game::Current();
        game.Dialogs.clear();
        game.Dialogs.resize(dialog_count);
        game.DialogCount = dialog_count;
        game.Play.dialogProps.clear();
        game.Play.dialogProps.resize(dialog_count);
        for (int d = 0; d < dialog_count; ++d)
        {
            game.Dialogs[d].Options.resize(option_count);
            for (int o = 0; o < option_count; ++o)
            {
                auto &opt = game.Dialogs[d].Options[o];
                opt.OriginalText = String::FromFormat("Dialog %d option %d", d, o);
                opt.Text = opt.OriginalText;
            }
        }
    }

    // Resizes the game's mouse cursors, all in their default state
    static void SetCursors(int count)
    {
        Game &game = Game::Current();
        game.MouseCursors.clear();
        game.MouseCursors.resize(count);
        game.MouseCursorCount = count;
    }

    // Resizes the game's audio types, all in their default state with a default volume of 0
    static void SetAudioTypes(int count)
    {
        Game &game = Game::Current();
        game.AudioClipTypes.clear();
        game.AudioClipTypes.resize(count);
        game.Play.default_audio_type_volumes.clear();
        game.Play.default_audio_type_volumes.resize(count);
    }

    // Makes the game's views, each with the same number of loops and frames; every frame has a sprite and
    // an event name, so that a save has frame data and strings to step over
    static void SetViews(int view_count, int loop_count, int frame_count)
    {
        Game &game = Game::Current();
        game.Views.clear();
        game.Views.resize(view_count);
        game.ViewCount = view_count;
        for (auto &view : game.Views)
        {
            view.Initialize(loop_count);
            for (auto &loop : view.Loops)
            {
                loop.Initialize(frame_count);
                for (int frame = 0; frame < frame_count; ++frame)
                {
                    loop.Frames[frame].Sprite = 10 + frame;
                    loop.Frames[frame].EventName = String::FromFormat("step%d", frame);
                }
            }
        }
    }

    // Resizes the game's characters and inventory items, all in their default state
    static void SetCounts(int characters, int items)
    {
        Game &game = Game::Current();
        game.Characters.clear();
        game.Characters.resize(characters);
        game.CharacterCount = characters;
        game.Play.charProps.clear();
        game.Play.charProps.resize(characters);
        game.InventoryItems.clear();
        game.InventoryItems.resize(items);
        game.InventoryItemCount = static_cast<uint16_t>(items);
        game.Play.invProps.clear();
        game.Play.invProps.resize(items);
    }

    // State that no new game has
    static void SetState()
    {
        Game &game = Game::Current();
        for (int i = 0; i < game.CharacterCount; ++i)
        {
            auto &ch = game.Characters[i];
            ch.X = 100 + i;
            ch.Y = 150 + i;
            ch.Room = 3 + i;
            ch.PreviousRoom = 2;
            ch.View = 4;
            ch.Loop = 2;
            ch.Frame = 1;
            ch.Z = -5;
            ch.ActiveInventory = 1;
            ch.WalkSpeedX = 6;
            ch.Opacity = 128;
            ch.Flags = 0x41;
            ch.Name = String::FromFormat("Character %d", i);
            ch.set_item_quantity(1, 2);
            ch.set_item_quantity(4, 1);
            ch.set_item_quantity(9, 7);
            ch.Inventory = {1, 4, 9};
            ch.Zoom = 80;
            ch.TintRed = 10;
            ch.TintLevel = 50;
            ch.Following = 0;
            game.Play.charProps[i]["Mood"] = "angry";
        }
        for (int i = 0; i < game.InventoryItemCount; ++i)
        {
            game.InventoryItems[i].Name = String::FromFormat("Item %d", i);
            game.InventoryItems[i].Sprite = 20 + i;
            game.InventoryItems[i].CursorSprite = 40 + i;
            game.InventoryItems[i].CursorHotspotX = 3;
            game.InventoryItems[i].CursorHotspotY = 4;
            game.Play.invProps[i]["Weight"] = "12";
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
    Game &game = Game::Current();
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
        const auto &ch = game.Characters[i];
        EXPECT_EQ(100 + i, ch.X);
        EXPECT_EQ(150 + i, ch.Y);
        EXPECT_EQ(3 + i, ch.Room);
        EXPECT_EQ(2, ch.PreviousRoom);
        EXPECT_EQ(4, ch.View);
        EXPECT_EQ(2, ch.Loop);
        EXPECT_EQ(1, ch.Frame);
        EXPECT_EQ(-5, ch.Z);
        EXPECT_EQ(1, ch.ActiveInventory);
        EXPECT_EQ(6, ch.WalkSpeedX);
        EXPECT_EQ(128, ch.Opacity);
        EXPECT_EQ(0x41, ch.Flags);
        EXPECT_STREQ(String::FromFormat("Character %d", i).GetCStr(), ch.Name.GetCStr());
        EXPECT_EQ(2, ch.get_item_quantity(1));
        EXPECT_EQ(1, ch.get_item_quantity(4));
        EXPECT_EQ(7, ch.get_item_quantity(9));
        EXPECT_EQ(0, ch.get_item_quantity(2));
        EXPECT_EQ((std::vector<int>{1, 4, 9}), game.Characters[i].Inventory);
        EXPECT_EQ(80, game.Characters[i].Zoom);
        EXPECT_EQ(10, game.Characters[i].TintRed);
        EXPECT_EQ(50, game.Characters[i].TintLevel);
        EXPECT_STREQ("angry", game.Play.charProps[i]["Mood"].GetCStr());
    }
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_STREQ(String::FromFormat("Item %d", i).GetCStr(), game.InventoryItems[i].Name.GetCStr());
        EXPECT_EQ(20 + i, game.InventoryItems[i].Sprite);
        EXPECT_EQ(40 + i, game.InventoryItems[i].CursorSprite);
        EXPECT_EQ(3, game.InventoryItems[i].CursorHotspotX);
        EXPECT_EQ(4, game.InventoryItems[i].CursorHotspotY);
        EXPECT_STREQ("12", game.Play.invProps[i]["Weight"].GetCStr());
    }
    EXPECT_EQ(3u, r_data.DataCounts.Characters);
    EXPECT_EQ(2u, r_data.DataCounts.InventoryItems);
}

// Every field of a character that is a part of the game's state comes back from a save. The rows of
// CharacterDef and CharacterState say which fields those are, so a field added to either struct is checked
// here without a line of this test changing.
TEST_F(SavegameContent, EveryCharacterFieldIsRestored)
{
    Game &game = Game::Current();
    SetCounts(1, 0);
    AlterFields<CharacterDef>(game.Characters[0]);
    AlterFields<CharacterState>(game.Characters[0]);
    const Character before = game.Characters[0];
    EXPECT_TRUE(FieldsAreDistinct<CharacterState>(before));
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, saved, error)) << error;

    SetCounts(1, 0);
    // The script name is the game's: a save does not hold it
    game.Characters[0].ScriptName = before.ScriptName;
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();

    std::string lost;
    for (const String &line : DiffFields<CharacterDef>(before, game.Characters[0], false))
        lost += std::string(line.GetCStr()) + "\n";
    for (const String &line : DiffFields<CharacterState>(before, game.Characters[0], false))
        lost += std::string(line.GetCStr()) + "\n";
    EXPECT_EQ("", lost);
}

// A character's "process idle this time" flag is one byte of the save: 1 when set, 0 when not; a byte that
// is neither reads as not set, as the engine only ever tests for 1
TEST_F(SavegameContent, ProcessIdleThisTimeIsOneByte)
{
    Game &game = Game::Current();
    SetCounts(1, 0);
    game.Characters[0].ProcessIdleThisTime = true;
    std::vector<uint8_t> on_bytes;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, on_bytes, error)) << error;
    game.Characters[0].ProcessIdleThisTime = false;
    std::vector<uint8_t> off_bytes;
    ASSERT_TRUE(Save(kSaveCmp_Characters, off_bytes, error)) << error;

    ASSERT_EQ(on_bytes.size(), off_bytes.size());
    size_t at = on_bytes.size();
    for (size_t i = 0; i < on_bytes.size(); ++i)
    {
        if (on_bytes[i] != off_bytes[i])
        {
            ASSERT_EQ(on_bytes.size(), at) << "a second byte differs, at " << i;
            at = i;
        }
    }
    ASSERT_LT(at, on_bytes.size());
    EXPECT_EQ(1u, on_bytes[at]);
    EXPECT_EQ(0u, off_bytes[at]);

    for (const auto &[byte, expected] : {std::pair<uint8_t, bool>{0, false}, {1, true}, {2, false}, {255, false}})
    {
        std::vector<uint8_t> saved = on_bytes;
        saved[at] = byte;
        SetCounts(1, 0);
        RestoredData r_data;
        HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        EXPECT_EQ(expected, game.Characters[0].ProcessIdleThisTime) << "byte " << int(byte);
    }
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
    Game &game = Game::Current();
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
    EXPECT_EQ(101, game.Characters[1].X);
    EXPECT_EQ(0, game.Characters[2].X);
}

TEST_F(SavegameContent, PrescanCountsWithoutRestoring)
{
    Game &game = Game::Current();
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
    EXPECT_EQ(0, game.Characters[0].X);
    EXPECT_TRUE(game.InventoryItems[0].Name.IsEmpty());
}

TEST_F(SavegameContent, PrescanStepsOverViewFrames)
{
    // the prescan skips each loop's frames and their event names, and counts every loop of every view
    SetViews(2, 3, 4);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Views, saved, error)) << error;

    Stream in(std::make_unique<VectorStream>(saved));
    PreservedParams pp;
    RestoredData r_data;
    HSaveError err = SavegameComponents::PrescanAll(&in, kSvgVersion_Current, kSaveCmp_Views, pp, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(2u, r_data.DataCounts.Views);
    EXPECT_EQ((std::vector<uint32_t>{3u, 3u}), r_data.DataCounts.ViewLoops);
    EXPECT_EQ((std::vector<uint32_t>{12u, 12u}), r_data.DataCounts.ViewFrames);
}

TEST_F(SavegameContent, UnselectedComponentsAreSkipped)
{
    Game &game = Game::Current();
    // a save holds components that the restore leaves out (ignored components, a retry without some)
    SetCounts(2, 2);
    SetState();
    game.Play.messagetime = 4321;
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(static_cast<SaveCmpSelection>(kSaveCmp_GameState | kCharactersAndItems), saved, error)) << error;

    SetCounts(2, 2);
    game.Play.messagetime = 0;
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_GameState, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(4321, game.Play.messagetime);
    EXPECT_EQ(0, game.Characters[0].X);
    EXPECT_TRUE(game.InventoryItems[0].Name.IsEmpty());
    game.Play.messagetime = 0;
}

TEST_F(SavegameContent, RestoreKeepsTheRunningEnginesMusicPack)
{
    // the game state holds whether the saving engine had a separate audio pack; the restoring engine keeps its own
    EngineServices &services = EngineServices::Current();
    services.SeparateMusicLib = true;
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, saved, error)) << error;

    services.SeparateMusicLib = false;
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_GameState, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_FALSE(services.SeparateMusicLib);
}

TEST_F(SavegameContent, RestoredCharactersSaveTheSameBytes)
{
    SetCounts(3, 2);
    SetState();
    std::vector<uint8_t> first, second;
    std::string error;
    ASSERT_TRUE(Save(kCharactersAndItems, first, error)) << error;
    SetCounts(3, 2);
    RestoredData r_data;
    HSaveError err = Restore(first, kCharactersAndItems, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    ASSERT_TRUE(Save(kCharactersAndItems, second, error)) << error;
    EXPECT_EQ(first.size(), second.size());
    EXPECT_TRUE(first == second);
}

TEST_F(SavegameContent, CharacterAnimationVolumeIsRestored)
{
    Game &game = Game::Current();
    SetCounts(1, 0);
    game.Characters[0].AnimationVolume = 70;
    game.Characters[0].SetAnimating(kAnimFlow_Repeat, kAnimDirBackward, 3, 40);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Characters, saved, error)) << error;

    SetCounts(1, 0);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Characters, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    const ViewAnimateParams &anim = game.Characters[0].GetAnimParams();
    EXPECT_EQ(kAnimFlow_Repeat, anim.Flow);
    EXPECT_EQ(kAnimDirBackward, anim.Direction);
    EXPECT_EQ(3, anim.Delay);
    EXPECT_EQ(70, game.Characters[0].AnimationVolume);
    EXPECT_EQ(40, anim.AudioVolume);
}

TEST_F(SavegameContent, DialogOptionsAreRestored)
{
    Game &game = Game::Current();
    // the flags of every option; the text only of an option whose text was changed (DFLG_TEXTSET)
    SetDialogs(2, 3);
    for (int d = 0; d < 2; ++d)
    {
        auto &opts = game.Dialogs[d].Options;
        opts[0].Flags = DFLG_ON | DFLG_HASBEENCHOSEN;
        opts[1].Flags = DFLG_ON | DFLG_TEXTSET;
        opts[1].Text = String::FromFormat("Changed %d", d);
        opts[2].Flags = DFLG_OFFPERM | DFLG_NOREPEAT;
        game.Play.dialogProps[d]["Topic"] = "weather";
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Dialogs, saved, error)) << error;

    SetDialogs(2, 3);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Dialogs, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int d = 0; d < 2; ++d)
    {
        const auto &opts = game.Dialogs[d].Options;
        EXPECT_EQ(DFLG_ON | DFLG_HASBEENCHOSEN, opts[0].Flags);
        EXPECT_EQ(DFLG_ON | DFLG_TEXTSET, opts[1].Flags);
        EXPECT_EQ(DFLG_OFFPERM | DFLG_NOREPEAT, opts[2].Flags);
        EXPECT_STREQ(String::FromFormat("Dialog %d option 0", d).GetCStr(), opts[0].Text.GetCStr());
        EXPECT_STREQ(String::FromFormat("Changed %d", d).GetCStr(), opts[1].Text.GetCStr());
        EXPECT_STREQ(String::FromFormat("Dialog %d option 2", d).GetCStr(), opts[2].Text.GetCStr());
        EXPECT_STREQ("weather", game.Play.dialogProps[d]["Topic"].GetCStr());
    }
    EXPECT_EQ(2u, r_data.DataCounts.Dialogs);
    EXPECT_EQ((std::vector<uint32_t>{3u, 3u}), r_data.DataCounts.DialogOptions);
}

TEST_F(SavegameContent, SaveWithFewerDialogOptionsIsRefused)
{
    SetDialogs(1, 2);
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Dialogs, saved, error)) << error;

    SetDialogs(1, 3);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Dialogs, r_data);
    ASSERT_FALSE(err);
    EXPECT_TRUE(HasErrorCode(err, kSvgErr_GameContentAssertion));
}

TEST_F(SavegameContent, MouseCursorsAreRestored)
{
    Game &game = Game::Current();
    // the cursor's script name is game data and not saved
    SetCursors(2);
    for (int i = 0; i < 2; ++i)
    {
        auto &cur = game.MouseCursors[i];
        cur.Sprite = 30 + i;
        cur.HotspotX = static_cast<short>(5 + i);
        cur.HotspotY = static_cast<short>(7 + i);
        cur.View = static_cast<short>(2 + i);
        cur.Flags = static_cast<uint8_t>(0x11 + i);
        cur.AnimationDelay = 9 + i;
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Cursors, saved, error)) << error;

    SetCursors(2);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Cursors, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i < 2; ++i)
    {
        const auto &cur = game.MouseCursors[i];
        EXPECT_EQ(30 + i, cur.Sprite);
        EXPECT_EQ(5 + i, cur.HotspotX);
        EXPECT_EQ(7 + i, cur.HotspotY);
        EXPECT_EQ(2 + i, cur.View);
        EXPECT_EQ(0x11 + i, cur.Flags);
        EXPECT_EQ(9 + i, cur.AnimationDelay);
    }
    EXPECT_EQ(2u, r_data.DataCounts.Cursors);
}

TEST_F(SavegameContent, AudioTypesAreRestored)
{
    Game &game = Game::Current();
    // the speech volume reduction, the crossfade speed and the default volume; the reserved channels are game data
    SetAudioTypes(2);
    for (int i = 0; i < 2; ++i)
    {
        game.AudioClipTypes[i].ReservedChannels = 1;
        game.AudioClipTypes[i].VolumeReductionWhileSpeechPlaying = 25 + i;
        game.AudioClipTypes[i].CrossfadeSpeed = 3 + i;
        game.Play.default_audio_type_volumes[i] = 60 + i;
    }
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_Audio, saved, error)) << error;

    SetAudioTypes(2);
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_Audio, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_EQ(0, game.AudioClipTypes[i].ReservedChannels);
        EXPECT_EQ(25 + i, game.AudioClipTypes[i].VolumeReductionWhileSpeechPlaying);
        EXPECT_EQ(3 + i, game.AudioClipTypes[i].CrossfadeSpeed);
        EXPECT_EQ(60 + i, game.Play.default_audio_type_volumes[i]);
    }
    EXPECT_EQ(2u, r_data.DataCounts.AudioClipTypes);
}

TEST_F(SavegameContent, GameSetupIsRestored)
{
    Game &game = Game::Current();
    // the options up to OPT_HIGHESTOPTION_321 and OPT_LIPSYNCTEXT, the player, the dialog bullet, the inventory
    // cursor's hotspot marker and the default lip-sync frame
    for (int i = 0; i <= OPT_HIGHESTOPTION_321; ++i)
        game.Options[i] = 100 + i;
    game.Options[OPT_LIPSYNCTEXT] = 1;
    game.PlayerCharacterID = 2;
    game.DialogBulletSprite = 14;
    game.InventoryCursorHotspotColor = static_cast<int>(0xFF00FF80u);
    game.InventoryCursorHotspotCrossColor = 0x00123456;
    game.InventoryCursorHotspotSprite = 17;
    game.DefaultLipSyncFrame = 4;
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, saved, error)) << error;

    GameSetupFields().Set();
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_GameState, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i <= OPT_HIGHESTOPTION_321; ++i)
        EXPECT_EQ(100 + i, game.Options[i]) << "option " << i;
    EXPECT_EQ(1, game.Options[OPT_LIPSYNCTEXT]);
    EXPECT_EQ(2, game.PlayerCharacterID);
    EXPECT_EQ(14, game.DialogBulletSprite);
    EXPECT_EQ(static_cast<int>(0xFF00FF80u), game.InventoryCursorHotspotColor);
    EXPECT_EQ(0x00123456, game.InventoryCursorHotspotCrossColor);
    EXPECT_EQ(17, game.InventoryCursorHotspotSprite);
    EXPECT_EQ(4, game.DefaultLipSyncFrame);
}

TEST(SavegameState, AnimatedOverlayWaitIsRestored)
{
    // an overlay animation as AnimatedOverlay::WriteToSavegame writes it, 7 ticks before its next frame
    std::vector<uint8_t> saved;
    {
        Stream out(std::make_unique<VectorStream>(saved, kStream_Write));
        out.WriteInt32(3); // overlay id
        out.WriteInt32(kAnimOver_PauseWithGame);
        out.WriteInt16(2); // view
        out.WriteInt16(1); // loop
        out.WriteInt16(4); // frame
        out.WriteInt8(kAnimFlow_Repeat);
        out.WriteInt8(kAnimDirForward);
        out.WriteInt8(kAnimDirBackward);
        out.WriteInt8(0);
        out.WriteInt16(5); // delay
        out.WriteInt16(7); // wait
        out.WriteInt8(60); // audio volume
        out.WriteInt8(0);
        out.WriteInt8(0);
        out.WriteInt8(0);
    }

    AnimatedOverlay over;
    {
        Stream in(std::make_unique<VectorStream>(saved));
        over.ReadFromSavegame(&in, kAnimOverSvgVersion_40024);
    }
    EXPECT_EQ(3, over.GetOverID());
    EXPECT_EQ(2, over.GetView());
    EXPECT_EQ(1, over.GetLoop());
    EXPECT_EQ(4, over.GetFrame());
    // the wait counter has no getter: the restored overlay must write back the same bytes
    std::vector<uint8_t> again;
    {
        Stream out(std::make_unique<VectorStream>(again, kStream_Write));
        over.WriteToSavegame(&out);
    }
    EXPECT_TRUE(saved == again);
}

TEST(SavegameState, RoomStatusReadsTheCountsInTheWrittenOrder)
{
    // a room state as RoomState::WriteToSavegame lays it out, with a walk-area count (1) that differs from
    // the walk-behind count (2), so that reading the two counts swapped misplaces the data after them
    std::vector<uint8_t> saved;
    {
        Stream out(std::make_unique<VectorStream>(saved, kStream_Write));
        out.WriteInt8(1);    // IsVisited
        out.WriteInt32(0);   // objects
        out.WriteInt32(0);   // hotspots
        out.WriteInt32(0);   // regions
        out.WriteInt32(1);   // walk areas
        out.WriteInt32(2);   // walk-behinds
        out.WriteInt32(111); // walk-behind baselines
        out.WriteInt32(222);
        out.WriteInt32(0); // the walk area: 3 reserved values
        out.WriteInt32(0);
        out.WriteInt32(0);
        out.WriteFloat32(0.5f);                      // its face direction ratio
        Properties::WriteValues(StringIMap(), &out); // room properties
        out.WriteInt32(0);                           // script data size
        out.WriteInt32(kRoomStatSvgVersion_Current); // content format
        out.WriteInt32(0);
        out.WriteInt32(0);
        out.WriteInt32(0);
        out.WriteFloat32(0.25f); // face direction ratio
        out.WriteInt32(0);       // background shader id
        out.WriteInt32(0);       // background shader handle
        out.WriteInt32(0);
        Properties::WriteValues(StringIMap(), &out); // the walk area's properties
    }

    RoomState room;
    Stream in(std::make_unique<VectorStream>(saved));
    room.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    EXPECT_EQ(1, room.IsVisited);
    EXPECT_EQ(111, room.WalkBehinds[0].Baseline);
    EXPECT_EQ(222, room.WalkBehinds[1].Baseline);
    EXPECT_EQ(0.5f, room.WalkAreas[0].FaceDirectionRatio);
    EXPECT_EQ(0.25f, room.FaceDirectionRatio);
    EXPECT_EQ(static_cast<soff_t>(saved.size()), in.GetPosition());
}
