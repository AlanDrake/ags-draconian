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
// A component of a version older than the first 4.0 save format is refused.
//
//=============================================================================
#include <memory>
#include <string>
#include <vector>

#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/save/savegame.h"
#include "game/save/savegame_components.h"
#include "game/save/savegame_internal.h"
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
    // the dynamic sprites component reads the sprite table through the sprite cache, as the engine has it
    init_sprite_cache();
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
    init_sprite_cache();
    Stream in(std::make_unique<VectorStream>(buf));
    PreservedParams pp;
    RestoredData r_data;
    HSaveError err = SavegameComponents::ReadAll(&in, kSvgVersion_Current, cmp, pp, r_data);
    if (!err)
        error = err->FullMessage().GetCStr();
    return static_cast<bool>(err);
}

// A component list with one component of that name and format version, and no data
std::vector<uint8_t> MakeComponentList(const char *name, int32_t version)
{
    std::vector<uint8_t> buf;
    auto append_text = [&buf](const std::string &text) { buf.insert(buf.end(), text.begin(), text.end()); };
    auto append_int = [&buf](int32_t value)
    {
        for (int i = 0; i < 4; ++i)
        {
            buf.push_back(static_cast<uint8_t>(value >> (i * 8)));
        }
    };
    append_text("<Components>");
    append_text(std::string("<") + name + ">");
    append_int(6 * sizeof(int32_t)); // header size
    append_int(0);                   // flags
    append_int(version);
    append_int(0); // data size
    append_int(0); // uncompressed size
    append_int(0); // checksum
    append_text(std::string("</") + name + ">");
    append_text("</Components>");
    return buf;
}

// The script timer that SetPlayState starts, and its ticks left
const int kTimerId = 3;
const int kTimerTicks = 40;

// Some of the state that the game state component saves, with values that no new game has: among them the three
// script variables that are flags (game.no_multiloop_repeat, game.disable_antialiasing, game.disable_dialog_parser), the
// fields that the save holds as bits of a word, and a script timer
void SetPlayState()
{
    Game &game = Game::Current();
    game.Play.NoMultiLoopRepeat = 1;
    game.Play.DisableAntialiasing = 1;
    game.Play.DisableDialogParser = 1;
    game.Play.IsMouseInputEnabled = false;
    game.Play.IsMouseCursorShown = false;
    game.Play.AlwaysWaitForSpeechText = true;
    game.Play.HasSpeechVoice = true;
    game.Play.IsSpeechVoiceBlocking = true;
    SetTimer(kTimerId, kTimerTicks);
    game.Play.UsedMode = 7;
    game.Play.MessageTimeLeft = 321;
    game.Play.UsedInventory = 4;
    game.Play.TextReadingSpeed = 17;
    game.Play.GlobalSpeechAnimationDelay = 5;
    game.Play.TextShadowColor = 12;
    game.Play.FastForward = 1;
    game.Play.LipSyncSpeed = 6;
    game.Play.SpeechAnimationStopTimeMargin = 13;
    game.Play.TextReadingSpeedModifier = -3;
    game.Play.GameSpeedModifier = 25;
    game.Play.SpeechBubbleWidth = 99;
    game.Play.DialogOptionsPaddingX = 8;
    game.Play.TopBarBackColor = 31;
    game.Play.TopBarFont = 2;
    game.Play.BackgroundSpeechGameSpeed = 1;
}

// The state that SetPlayState sets, as a new game has it
void ClearPlayState()
{
    Game &game = Game::Current();
    game.Play.NoMultiLoopRepeat = 0;
    game.Play.DisableAntialiasing = 0;
    game.Play.DisableDialogParser = 0;
    game.Play.IsMouseInputEnabled = true;
    game.Play.IsMouseCursorShown = true;
    game.Play.AlwaysWaitForSpeechText = false;
    game.Play.HasSpeechVoice = false;
    game.Play.IsSpeechVoiceBlocking = false;
    SetTimer(kTimerId, 0);
    game.Play.UsedMode = 0;
    game.Play.MessageTimeLeft = 0;
    game.Play.UsedInventory = 0;
    game.Play.TextReadingSpeed = 0;
    game.Play.GlobalSpeechAnimationDelay = 0;
    game.Play.TextShadowColor = 0;
    game.Play.FastForward = 0;
    game.Play.LipSyncSpeed = 0;
    game.Play.SpeechAnimationStopTimeMargin = 0;
    game.Play.TextReadingSpeedModifier = 0;
    game.Play.GameSpeedModifier = 0;
    game.Play.SpeechBubbleWidth = 0;
    game.Play.DialogOptionsPaddingX = 0;
    game.Play.TopBarBackColor = 0;
    game.Play.TopBarFont = 0;
    game.Play.BackgroundSpeechGameSpeed = 0;
}

void ExpectPlayState()
{
    Game &game = Game::Current();
    EXPECT_EQ(1, game.Play.NoMultiLoopRepeat);
    EXPECT_EQ(1, game.Play.DisableAntialiasing);
    EXPECT_EQ(1, game.Play.DisableDialogParser);
    EXPECT_FALSE(game.Play.IsMouseInputEnabled);
    EXPECT_FALSE(game.Play.IsMouseCursorShown);
    EXPECT_TRUE(game.Play.AlwaysWaitForSpeechText);
    EXPECT_TRUE(game.Play.HasSpeechVoice);
    EXPECT_TRUE(game.Play.IsSpeechVoiceBlocking);
    EXPECT_EQ(kTimerTicks, GetTimerPos(kTimerId));
    EXPECT_EQ(7, game.Play.UsedMode);
    EXPECT_EQ(321, game.Play.MessageTimeLeft);
    EXPECT_EQ(4, game.Play.UsedInventory);
    EXPECT_EQ(17, game.Play.TextReadingSpeed);
    EXPECT_EQ(5, game.Play.GlobalSpeechAnimationDelay);
    EXPECT_EQ(12, game.Play.TextShadowColor);
    EXPECT_EQ(1, game.Play.FastForward);
    EXPECT_EQ(6, game.Play.LipSyncSpeed);
    EXPECT_EQ(13, game.Play.SpeechAnimationStopTimeMargin);
    EXPECT_EQ(-3, game.Play.TextReadingSpeedModifier);
    EXPECT_EQ(25, game.Play.GameSpeedModifier);
    EXPECT_EQ(99, game.Play.SpeechBubbleWidth);
    EXPECT_EQ(8, game.Play.DialogOptionsPaddingX);
    EXPECT_EQ(31, game.Play.TopBarBackColor);
    EXPECT_EQ(2, game.Play.TopBarFont);
    EXPECT_EQ(1, game.Play.BackgroundSpeechGameSpeed);
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

TEST(Savegame, ComponentOlderThan400IsRefused)
{
    // the lowest version of every component whose 4.0 format has one above 0
    const struct
    {
        const char *Name;
        int32_t Lowest;
    } floors[] = {
        {"Game State", 4000000},        {"Audio", 2},
        {"Characters", 4000000},        {"GUI", 4000000},
        {"Mouse Cursors", 1},           {"Overlays", 4000000},
        {"AnimatedOverlays", 4000024},  {"Room States", 4000003},
        {"Loaded Room State", 4000003}, {"Move Lists", 4000000},
        {"Managed Pool", 4000000},      {"RTTI", 4000000},
    };
    for (const auto &floor : floors)
    {
        const std::vector<uint8_t> buf = MakeComponentList(floor.Name, floor.Lowest - 1);
        Stream in(std::make_unique<VectorStream>(buf));
        PreservedParams pp;
        RestoredData r_data;
        HSaveError err = SavegameComponents::ReadAll(&in, kSvgVersion_Current, kSaveCmp_All, pp, r_data);
        ASSERT_FALSE(err) << floor.Name;
        // the component's error is wrapped in the list's
        bool version_refused = err->Code() == kSvgErr_UnsupportedComponentVersion;
        for (PError cause = err->PreviousError(); cause; cause = cause->PreviousError())
        {
            version_refused |= cause->Code() == kSvgErr_UnsupportedComponentVersion;
        }
        EXPECT_TRUE(version_refused) << floor.Name << ": " << err->FullMessage().GetCStr();
        const std::string expected = String::FromFormat("supported: %d - ", floor.Lowest).GetCStr();
        EXPECT_NE(std::string::npos, std::string(err->FullMessage().GetCStr()).find(expected))
            << floor.Name << ": " << err->FullMessage().GetCStr();
    }
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
