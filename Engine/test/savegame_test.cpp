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
// Savegame round trip (issue #2814): the game state is set up, written as a
// save, then the state is cleared and restored from the save, and written once
// more. The two saves must be the same bytes, and the restored state must be
// what was saved.
//
// The tests run in the process without a loaded game, so they work with the
// state that exists at start: the engine's global objects with their defaults.
// The script component is left out, because it needs the game's scripts.
//
//=============================================================================
#include <memory>
#include <string>
#include <vector>

#include "ac/gamesetupstructbase.h"
#include "ac/gamestate.h"
#include "game/savegame.h"
#include "game/savegame_components.h"
#include "game/savegame_internal.h"
#include "gtest/gtest.h"
#include "util/memorystream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace
{

// Components that can be saved and restored without a game loaded (see above)
struct ComponentInfo
{
    const char *Name;
    SaveCmpSelection Selection;
};

const ComponentInfo kComponents[] = {
    {"GameState", kSaveCmp_GameState},
    {"Audio", kSaveCmp_Audio},
    {"Characters", kSaveCmp_Characters},
    {"Dialogs", kSaveCmp_Dialogs},
    {"GUI", kSaveCmp_GUI},
    {"InvItems", kSaveCmp_InvItems},
    {"Cursors", kSaveCmp_Cursors},
    {"Views", kSaveCmp_Views},
    {"DynamicSprites", kSaveCmp_DynamicSprites},
    {"Overlays", kSaveCmp_Overlays},
    {"Rooms", kSaveCmp_Rooms},
    {"ThisRoom", kSaveCmp_ThisRoom},
    {"Plugins", kSaveCmp_Plugins},
    {"ObjectSprites", kSaveCmp_ObjectSprites},
};

const SaveCmpSelection kAllWorkingComponents = static_cast<SaveCmpSelection>(kSaveCmp_All & ~kSaveCmp_Scripts);

// Writes the components to a buffer
bool Save(SaveCmpSelection cmp, bool compress, std::vector<uint8_t> &buf, std::string &error)
{
    buf.clear();
    Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
    HSaveError err = SavegameComponents::WriteAllCommon(&out, cmp, compress);
    if (!err)
        error = err->FullMessage().GetCStr();
    return static_cast<bool>(err);
}

// Restores the components from a buffer
bool Restore(const std::vector<uint8_t> &buf, SaveCmpSelection cmp, std::string &error)
{
    Stream in(std::make_unique<VectorStream>(buf));
    PreservedParams pp;
    RestoredData r_data;
    HSaveError err = SavegameComponents::ReadAll(&in, kSvgVersion_Current, cmp, pp, r_data);
    if (!err)
        error = err->FullMessage().GetCStr();
    return static_cast<bool>(err);
}

// Some of the state that the game state component saves, with values that no new game has
void SetPlayState()
{
    play.usedmode = 7;
    play.messagetime = 321;
    play.usedinv = 4;
    play.text_speed = 17;
    play.talkanim_speed = 5;
    play.speech_text_shadow = 12;
    play.fast_forward = 1;
    play.lipsync_speed = 6;
    play.close_mouth_speech_time = 13;
    play.text_speed_modifier = -3;
    play.game_speed_modifier = 25;
    play.speech_bubble_width = 99;
    play.dialog_options_pad_x = 8;
    play.top_bar_backcolor = 31;
    play.top_bar_font = 2;
    play.bgspeech_game_speed = 1;
}

void ClearPlayState()
{
    play.usedmode = 0;
    play.messagetime = 0;
    play.usedinv = 0;
    play.text_speed = 0;
    play.talkanim_speed = 0;
    play.speech_text_shadow = 0;
    play.fast_forward = 0;
    play.lipsync_speed = 0;
    play.close_mouth_speech_time = 0;
    play.text_speed_modifier = 0;
    play.game_speed_modifier = 0;
    play.speech_bubble_width = 0;
    play.dialog_options_pad_x = 0;
    play.top_bar_backcolor = 0;
    play.top_bar_font = 0;
    play.bgspeech_game_speed = 0;
}

void ExpectPlayState()
{
    EXPECT_EQ(7, play.usedmode);
    EXPECT_EQ(321, play.messagetime);
    EXPECT_EQ(4, play.usedinv);
    EXPECT_EQ(17, play.text_speed);
    EXPECT_EQ(5, play.talkanim_speed);
    EXPECT_EQ(12, play.speech_text_shadow);
    EXPECT_EQ(1, play.fast_forward);
    EXPECT_EQ(6, play.lipsync_speed);
    EXPECT_EQ(13, play.close_mouth_speech_time);
    EXPECT_EQ(-3, play.text_speed_modifier);
    EXPECT_EQ(25, play.game_speed_modifier);
    EXPECT_EQ(99, play.speech_bubble_width);
    EXPECT_EQ(8, play.dialog_options_pad_x);
    EXPECT_EQ(31, play.top_bar_backcolor);
    EXPECT_EQ(2, play.top_bar_font);
    EXPECT_EQ(1, play.bgspeech_game_speed);
}

} // namespace

TEST(Savegame, RestoredStateIsWhatWasSaved)
{
    SetPlayState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, false, saved, error)) << error;

    ClearPlayState();
    ASSERT_TRUE(Restore(saved, kSaveCmp_GameState, error)) << error;
    ExpectPlayState();
    ClearPlayState();
}

TEST(Savegame, SaveRestoreSaveGivesTheSameBytes)
{
    SetPlayState();
    std::vector<uint8_t> first, second;
    std::string error;
    ASSERT_TRUE(Save(kAllWorkingComponents, false, first, error)) << error;

    ClearPlayState();
    ASSERT_TRUE(Restore(first, kAllWorkingComponents, error)) << error;
    ASSERT_TRUE(Save(kAllWorkingComponents, false, second, error)) << error;
    ClearPlayState();
    EXPECT_EQ(first.size(), second.size());
    EXPECT_TRUE(first == second);
}

TEST(Savegame, EveryComponentSavesAndRestoresTheSameBytes)
{
    const size_t count = sizeof(kComponents) / sizeof(kComponents[0]);
    for (size_t i = 0; i < count; ++i)
    {
        std::vector<uint8_t> first, second;
        std::string error;
        ASSERT_TRUE(Save(kComponents[i].Selection, false, first, error)) << kComponents[i].Name << ": " << error;
        EXPECT_FALSE(first.empty()) << kComponents[i].Name;
        ASSERT_TRUE(Restore(first, kComponents[i].Selection, error)) << kComponents[i].Name << ": " << error;
        ASSERT_TRUE(Save(kComponents[i].Selection, false, second, error)) << kComponents[i].Name << ": " << error;
        EXPECT_TRUE(first == second) << kComponents[i].Name;
    }
}

TEST(Savegame, CompressedSaveRestoresTheSameStateAsPlain)
{
    SetPlayState();
    std::vector<uint8_t> plain, compressed, again;
    std::string error;
    ASSERT_TRUE(Save(kAllWorkingComponents, false, plain, error)) << error;
    ASSERT_TRUE(Save(kAllWorkingComponents, true, compressed, error)) << error;
    EXPECT_LT(compressed.size(), plain.size());

    ClearPlayState();
    ASSERT_TRUE(Restore(compressed, kAllWorkingComponents, error)) << error;
    ExpectPlayState();
    // saving the state restored from the compressed save gives the plain save back
    ASSERT_TRUE(Save(kAllWorkingComponents, false, again, error)) << error;
    EXPECT_TRUE(plain == again);
    ClearPlayState();
}

TEST(Savegame, SaveWithNoComponentsIsJustTheList)
{
    std::vector<uint8_t> buf;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_None, false, buf, error)) << error;
    // the opening and closing tags of the component list, and nothing between
    EXPECT_GT(buf.size(), 0u);
    EXPECT_LT(buf.size(), 200u);
    ASSERT_TRUE(Restore(buf, kSaveCmp_None, error)) << error;
}

TEST(Savegame, TruncatedSaveIsRefused)
{
    SetPlayState();
    std::vector<uint8_t> saved;
    std::string error;
    ASSERT_TRUE(Save(kSaveCmp_GameState, false, saved, error)) << error;
    ClearPlayState();

    const size_t cuts[] = {saved.size() / 2, saved.size() - 1, 20};
    for (size_t i = 0; i < sizeof(cuts) / sizeof(cuts[0]); ++i)
    {
        std::vector<uint8_t> cut(saved.begin(), saved.begin() + cuts[i]);
        EXPECT_FALSE(Restore(cut, kSaveCmp_GameState, error)) << "cut at " << cuts[i] << " of " << saved.size();
    }
}
