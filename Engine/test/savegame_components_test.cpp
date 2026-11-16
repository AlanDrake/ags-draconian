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
#include "game/dialog_topic.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/render/screen_overlay.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
#include "game/view.h"
#include "game/world/character_extras.h"
#include "game/world/room_status.h"
#include "gtest/gtest.h"
#include "platform/base/engine_services.h"
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
// - Game::Characters, game.numcharacters, game.Play.charProps: the characters
// - game.invinfo, game.numinvitems, game.Play.invProps: the inventory items
// - Game::Views, game.numviews: the views
// - Game::Dialogs, game.numdialog, game.Play.dialogProps: the dialogs
// - game.mcurs, game.numcursors: the mouse cursors
// - game.audioClipTypes, game.Play.default_audio_type_volumes: the audio types
// - the GameDef fields that the game state saves (GameSetupFields)
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

    // The GameDef fields that the game state component saves
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
            std::copy_n(game.options, GameDef::MAX_OPTIONS, fields.Options.begin());
            fields.PlayerCharacter = game.playercharacter;
            fields.DialogBullet = game.dialog_bullet;
            fields.InvHotColor = game.inv_hot_color;
            fields.InvHotCrossColor = game.inv_hot_cross_color;
            fields.InvHotSprite = game.inv_hot_sprite;
            fields.DefaultLipsyncFrame = game.default_lipsync_frame;
            return fields;
        }

        void Set() const
        {
            Game &game = Game::Current();
            std::copy(Options.begin(), Options.end(), game.options);
            game.playercharacter = PlayerCharacter;
            game.dialog_bullet = DialogBullet;
            game.inv_hot_color = InvHotColor;
            game.inv_hot_cross_color = InvHotCrossColor;
            game.inv_hot_sprite = InvHotSprite;
            game.default_lipsync_frame = DefaultLipsyncFrame;
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
        game.numdialog = dialog_count;
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
        game.mcurs.clear();
        game.mcurs.resize(count);
        game.numcursors = count;
    }

    // Resizes the game's audio types, all in their default state with a default volume of 0
    static void SetAudioTypes(int count)
    {
        Game &game = Game::Current();
        game.audioClipTypes.clear();
        game.audioClipTypes.resize(count);
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
        game.numviews = view_count;
        for (auto &view : game.Views)
        {
            view.Initialize(loop_count);
            for (auto &loop : view.loops)
            {
                loop.Initialize(frame_count);
                for (int frame = 0; frame < frame_count; ++frame)
                {
                    loop.frames[frame].pic = 10 + frame;
                    loop.frames[frame].event_name = String::FromFormat("step%d", frame);
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
        game.numcharacters = characters;
        game.Play.charProps.clear();
        game.Play.charProps.resize(characters);
        game.invinfo.clear();
        game.invinfo.resize(items);
        game.numinvitems = static_cast<uint16_t>(items);
        game.Play.invProps.clear();
        game.Play.invProps.resize(items);
    }

    // State that no new game has
    static void SetState()
    {
        Game &game = Game::Current();
        for (int i = 0; i < game.numcharacters; ++i)
        {
            auto &ch = game.Characters[i];
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
            ch.inventory = {1, 4, 9};
            ch.zoom = 80;
            ch.tint_r = 10;
            ch.tint_level = 50;
            ch.following = 0;
            game.Play.charProps[i]["Mood"] = "angry";
        }
        for (int i = 0; i < game.numinvitems; ++i)
        {
            game.invinfo[i].name = String::FromFormat("Item %d", i);
            game.invinfo[i].pic = 20 + i;
            game.invinfo[i].cursorPic = 40 + i;
            game.invinfo[i].hotx = 3;
            game.invinfo[i].hoty = 4;
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
        EXPECT_EQ((std::vector<int>{1, 4, 9}), game.Characters[i].inventory);
        EXPECT_EQ(80, game.Characters[i].zoom);
        EXPECT_EQ(10, game.Characters[i].tint_r);
        EXPECT_EQ(50, game.Characters[i].tint_level);
        EXPECT_STREQ("angry", game.Play.charProps[i]["Mood"].GetCStr());
    }
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_STREQ(String::FromFormat("Item %d", i).GetCStr(), game.invinfo[i].name.GetCStr());
        EXPECT_EQ(20 + i, game.invinfo[i].pic);
        EXPECT_EQ(40 + i, game.invinfo[i].cursorPic);
        EXPECT_EQ(3, game.invinfo[i].hotx);
        EXPECT_EQ(4, game.invinfo[i].hoty);
        EXPECT_STREQ("12", game.Play.invProps[i]["Weight"].GetCStr());
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
    EXPECT_EQ(101, game.Characters[1].x);
    EXPECT_EQ(0, game.Characters[2].x);
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
    EXPECT_EQ(0, game.Characters[0].x);
    EXPECT_TRUE(game.invinfo[0].name.IsEmpty());
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
    EXPECT_EQ(0, game.Characters[0].x);
    EXPECT_TRUE(game.invinfo[0].name.IsEmpty());
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
    game.Characters[0].audio_volume = 70;
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
    EXPECT_EQ(70, game.Characters[0].audio_volume);
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
        auto &cur = game.mcurs[i];
        cur.pic = 30 + i;
        cur.hotx = static_cast<short>(5 + i);
        cur.hoty = static_cast<short>(7 + i);
        cur.view = static_cast<short>(2 + i);
        cur.flags = static_cast<uint8_t>(0x11 + i);
        cur.animdelay = 9 + i;
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
        const auto &cur = game.mcurs[i];
        EXPECT_EQ(30 + i, cur.pic);
        EXPECT_EQ(5 + i, cur.hotx);
        EXPECT_EQ(7 + i, cur.hoty);
        EXPECT_EQ(2 + i, cur.view);
        EXPECT_EQ(0x11 + i, cur.flags);
        EXPECT_EQ(9 + i, cur.animdelay);
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
        game.audioClipTypes[i].reservedChannels = 1;
        game.audioClipTypes[i].volume_reduction_while_speech_playing = 25 + i;
        game.audioClipTypes[i].crossfadeSpeed = 3 + i;
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
        EXPECT_EQ(0, game.audioClipTypes[i].reservedChannels);
        EXPECT_EQ(25 + i, game.audioClipTypes[i].volume_reduction_while_speech_playing);
        EXPECT_EQ(3 + i, game.audioClipTypes[i].crossfadeSpeed);
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
        game.options[i] = 100 + i;
    game.options[OPT_LIPSYNCTEXT] = 1;
    game.playercharacter = 2;
    game.dialog_bullet = 14;
    game.inv_hot_color = static_cast<int>(0xFF00FF80u);
    game.inv_hot_cross_color = 0x00123456;
    game.inv_hot_sprite = 17;
    game.default_lipsync_frame = 4;
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, saved, error)) << error;

    GameSetupFields().Set();
    RestoredData r_data;
    HSaveError err = Restore(saved, kSaveCmp_GameState, r_data);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    for (int i = 0; i <= OPT_HIGHESTOPTION_321; ++i)
        EXPECT_EQ(100 + i, game.options[i]) << "option " << i;
    EXPECT_EQ(1, game.options[OPT_LIPSYNCTEXT]);
    EXPECT_EQ(2, game.playercharacter);
    EXPECT_EQ(14, game.dialog_bullet);
    EXPECT_EQ(static_cast<int>(0xFF00FF80u), game.inv_hot_color);
    EXPECT_EQ(0x00123456, game.inv_hot_cross_color);
    EXPECT_EQ(17, game.inv_hot_sprite);
    EXPECT_EQ(4, game.default_lipsync_frame);
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
    // a room state as RoomStatus::WriteToSavegame lays it out, with a walk-area count (1) that differs from
    // the walk-behind count (2), so that reading the two counts swapped misplaces the data after them
    std::vector<uint8_t> saved;
    {
        Stream out(std::make_unique<VectorStream>(saved, kStream_Write));
        out.WriteInt8(1);    // beenhere
        out.WriteInt32(0);   // objects
        out.WriteInt32(0);   // hotspots
        out.WriteInt32(0);   // regions
        out.WriteInt32(1);   // walk areas
        out.WriteInt32(2);   // walk-behinds
        out.WriteInt32(111); // walk-behind baselines
        out.WriteInt32(222);
        WalkareaState walkarea;
        walkarea.FaceDirectionRatio = 0.5f;
        walkarea.WriteToSavegame(&out);
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

    RoomStatus room;
    Stream in(std::make_unique<VectorStream>(saved));
    room.ReadFromSavegame(&in, kRoomStatSvgVersion_Current);
    EXPECT_EQ(1, room.beenhere);
    EXPECT_EQ(111, room.walkbehind_base[0]);
    EXPECT_EQ(222, room.walkbehind_base[1]);
    EXPECT_EQ(0.5f, room.walkareas[0].FaceDirectionRatio);
    EXPECT_EQ(0.25f, room.face_dir_ratio);
    EXPECT_EQ(static_cast<soff_t>(saved.size()), in.GetPosition());
}
