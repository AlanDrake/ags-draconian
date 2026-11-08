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
// Compatibility contract C1 (docs/refactor/PLAN.md): game data built by released AGS 4 editors
// keeps loading. The fixtures in data/compat/<version>/ are the Sierra-style template built by
// each release (data/compat/make_fixtures.py); the engine runs them in the compat_* ctest cases.
// Game data older than 3.99 and rooms older than 4.0 (decision D1) are refused when their header is
// read.
//
//=============================================================================
#include <memory>
#include <utility>

#include "ac/dialogtopic.h"
#include "ac/gamesetupstruct.h"
#include "ac/view.h"
#include "data/assetmanager.h"
#include "game/main_game_file.h"
#include "game/room_file.h"
#include "game/roomstruct.h"
#include "gtest/gtest.h"
#include "gui/guimain.h"
#include "script/cc_common.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

// The script loader reports errors through these; the engine and the tools define their own.
// Not in common_stubs.cpp: the MSBuild Tools.Test links that with Tools/data/cc_script_stubs.cpp.
String cc_format_error(const String &message)
{
    return message;
}

String cc_get_callstack(int max_lines)
{
    return {};
}

namespace
{

struct CompatFixture
{
    const char *Version;      // fixture directory
    const char *CompiledWith; // the editor's version string in the game data
    GameDataVersion DataVersion;
    RoomFileVersion RoomVersion;
};

// The earliest AGS 4 release available and the latest; 3.99 alphas are not covered (LOG.md, 1.2)
const CompatFixture CompatFixtures[] = {
    {"4.0.0.27", "4.00.00.27", kGameVersion_400_27, kRoomVersion_400_26},
    {"4.0.0.30", "4.00.00.30", kGameVersion_400_29, kRoomVersion_400_26},
};

String FixturePackage(const CompatFixture &fx)
{
    return Path::ConcatPaths(Path::ConcatPaths(COMPAT_FIXTURES_DIR, fx.Version), "game.ags");
}

class GameCompat : public ::testing::TestWithParam<CompatFixture>
{
};

} // namespace

TEST_P(GameCompat, MainGameData)
{
    const CompatFixture &fx = GetParam();
    AssetManager mgr;
    ASSERT_EQ(mgr.AddLibrary(FixturePackage(fx)), kAssetNoError) << FixturePackage(fx).GetCStr();

    MainGameSource src;
    HGameFileError err = OpenMainGameFileFromDefaultAsset(src, &mgr);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(src.DataVersion, fx.DataVersion);
    EXPECT_STREQ(src.CompiledWith.GetCStr(), fx.CompiledWith);

    GameSetupStruct game;
    LoadedGameEntities ents(game);
    err = ReadGameData(ents, std::move(src.InputStream), src.DataVersion, src.CompiledWith);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    err = UpdateGameData(ents, src.DataVersion);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();

    EXPECT_STREQ(game.gamename.GetCStr(), "Sierra-style");
    ASSERT_EQ(game.numcharacters, 1);
    EXPECT_STREQ(game.chars[0].scrname.GetCStr(), "cEgo");
    ASSERT_EQ(game.numinvitems, 3); // item 0 is reserved
    EXPECT_STREQ(game.invScriptNames[1].GetCStr(), "iBlueCup");
    EXPECT_STREQ(game.invScriptNames[2].GetCStr(), "iKey");
    ASSERT_EQ(game.numgui, 9);
    ASSERT_EQ(ents.Guis.size(), 9u);
    EXPECT_STREQ(ents.Guis[0].GetName().GetCStr(), "gStatusline");
    EXPECT_STREQ(ents.Guis[8].GetName().GetCStr(), "gExitGame");
    EXPECT_EQ(game.numviews, 1);
    EXPECT_EQ(ents.Views.size(), 1u);
    EXPECT_EQ(game.numdialog, 0);
    EXPECT_EQ(game.numfonts, 3);
    EXPECT_EQ(game.numcursors, 10);
    ASSERT_TRUE(ents.GlobalScript);
    EXPECT_TRUE(ents.DialogScript);
    ASSERT_EQ(ents.ScriptModules.size(), 1u); // KeyboardMovement
    ASSERT_EQ(ents.ScriptModuleNames.size(), 1u);
    EXPECT_STREQ(ents.ScriptModuleNames[0].GetCStr(), "KeyboardMovement.asc");
}

TEST_P(GameCompat, Room)
{
    const CompatFixture &fx = GetParam();
    AssetManager mgr;
    ASSERT_EQ(mgr.AddLibrary(FixturePackage(fx)), kAssetNoError) << FixturePackage(fx).GetCStr();

    RoomDataSource src("room1.crm", mgr.OpenAsset("room1.crm"));
    ASSERT_TRUE(src.InputStream);
    HRoomFileError err = OpenRoomFile(src);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    // The room format version that task 3.4 must keep loading
    EXPECT_EQ(src.DataVersion, fx.RoomVersion);

    RoomStruct room;
    err = ReadRoomData(&room, std::move(src.InputStream), src.DataVersion);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();

    EXPECT_EQ(room.Width, 320);
    EXPECT_EQ(room.Height, 200);
    EXPECT_EQ(room.BgFrameCount, 1u);
    // The counts are the format's fixed slot numbers, every slot is written
    EXPECT_EQ(room.HotspotCount, 50u);
    EXPECT_EQ(room.RegionCount, 16u);
    EXPECT_EQ(room.WalkAreaCount, 16u);
    EXPECT_EQ(room.WalkBehindCount, 16u);
    EXPECT_STREQ(room.Hotspots[1].ScriptName.GetCStr(), "hGlowingOrb");
    EXPECT_EQ(room.Objects.size(), 0u);
    ASSERT_TRUE(room.CompiledScript);
}

INSTANTIATE_TEST_SUITE_P(Releases, GameCompat, ::testing::ValuesIn(CompatFixtures),
                         [](const ::testing::TestParamInfo<CompatFixture> &fixture)
                         {
                             std::string name = std::string("v") + fixture.param.Version;
                             for (char &c : name)
                                 if (c == '.')
                                     c = '_';
                             return name;
                         });

namespace
{

// Writes a main game file header as the tools do (WriteHeaderBlock in Tools/data/data_file_writer.cpp)
// and opens it: signature, data format version, the editor's version string, no required engine caps.
HGameFileError OpenGameHeader(int data_version, const char *compiled_with, MainGameSource &src)
{
    const String filename = String::FromFormat("GameHeader_%d.dta", data_version);
    {
        std::unique_ptr<Stream> out = File::CreateFile(filename);
        if (!out)
        {
            return new MainGameFileError(kMGFErr_FileOpenFailed, "Could not create %s", filename.GetCStr());
        }
        out->Write(MainGameSource::Signature.GetCStr(), MainGameSource::Signature.GetLength());
        out->WriteInt32(data_version);
        StrUtil::WriteString(compiled_with, out.get());
        out->WriteInt32(0);
    }
    HGameFileError err = OpenMainGameFile(filename, src);
    src.InputStream.reset(); // close the file before deleting it
    File::DeleteFile(filename);
    return err;
}

} // namespace

// D1: game data older than 3.99 is refused when the header is read, with an error that names the
// file's format version and the oldest supported one
TEST(GameDataVersion, OlderThan399IsRefused)
{
    // The first and the last 3.6 data formats
    const std::pair<int, const char *> versions[] = {{3060021, "3.6.0.21"}, {3060316, "3.6.3.16"}};
    for (const auto &version : versions)
    {
        MainGameSource src;
        HGameFileError err = OpenGameHeader(version.first, version.second, src);
        ASSERT_FALSE(err) << version.first;
        EXPECT_EQ(err->Code(), kMGFErr_FormatVersionNotSupported);
        const String message = err->FullMessage();
        EXPECT_TRUE(message.FindString(version.second) != String::NoIndex) << message.GetCStr();
        EXPECT_TRUE(message.FindString(String::FromFormat("%d", version.first)) != String::NoIndex)
            << message.GetCStr();
        EXPECT_TRUE(message.FindString(String::FromFormat("%d", kGameVersion_399)) != String::NoIndex)
            << message.GetCStr();
    }
}

TEST(GameDataVersion, From399IsOpened)
{
    MainGameSource src;
    HGameFileError err = OpenGameHeader(kGameVersion_399, "3.99.0.0", src);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(src.DataVersion, kGameVersion_399);
    EXPECT_STREQ(src.CompiledWith.GetCStr(), "3.99.0.0");
}

TEST(GameDataVersion, NewerThanEngineIsRefused)
{
    MainGameSource src;
    HGameFileError err = OpenGameHeader(kGameVersion_Current + 1, "9.0.0.0", src);
    ASSERT_FALSE(err);
    EXPECT_EQ(err->Code(), kMGFErr_FormatVersionNotSupported);
}

namespace
{

// Writes a room file header and opens it. A pre-4.0 room starts with its 16-bit version; a 4.0 room
// writes 0xFFFF there, then the signature, the 32-bit version and the editor's version string
// (WriteRoomHeader).
HRoomFileError OpenRoomHeader(int data_version, bool extended_header, RoomDataSource &src)
{
    const String filename = String::FromFormat("RoomHeader_%d.crm", data_version);
    {
        std::unique_ptr<Stream> out = File::CreateFile(filename);
        if (!out)
        {
            return new RoomFileError(kRoomFileErr_FileOpenFailed, "Could not create %s", filename.GetCStr());
        }
        if (extended_header)
        {
            out->WriteInt16(-1);
            out->Write(RoomDataSource::Signature.GetCStr(), RoomDataSource::Signature.GetLength());
            out->WriteInt32(data_version);
            StrUtil::WriteString("4.00.00.34", out.get());
        }
        else
        {
            out->WriteInt16(static_cast<int16_t>(data_version));
        }
    }
    HRoomFileError err = OpenRoomFile(filename, src);
    src.InputStream.reset(); // close the file before deleting it
    File::DeleteFile(filename);
    return err;
}

} // namespace

// D1: rooms older than 4.0 are refused when the header is read, with an error that names the file's
// format version and the oldest supported one
TEST(RoomDataVersion, OlderThan400IsRefused)
{
    // 3.5.0.8 (the oldest that upstream reads), 3.99 (the 4.0 alphas) and 3.6.3.15 (the last 3.x format),
    // all with the 16-bit header they were written with
    for (int version : {33, 40, 36315})
    {
        RoomDataSource src;
        HRoomFileError err = OpenRoomHeader(version, false, src);
        ASSERT_FALSE(err) << version;
        EXPECT_EQ(err->Code(), kRoomFileErr_FormatNotSupported);
        const String message = err->FullMessage();
        EXPECT_TRUE(message.FindString(String::FromFormat("version: %d,", version)) != String::NoIndex)
            << message.GetCStr();
        EXPECT_TRUE(message.FindString(String::FromFormat("supported %d -", kRoomVersion_400)) != String::NoIndex)
            << message.GetCStr();
    }
}

TEST(RoomDataVersion, From400IsOpened)
{
    RoomDataSource src;
    HRoomFileError err = OpenRoomHeader(kRoomVersion_400, true, src);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(src.DataVersion, kRoomVersion_400);
    EXPECT_STREQ(src.CompiledWith.GetCStr(), "4.00.00.34");
}

TEST(RoomDataVersion, NewerThanEngineIsRefused)
{
    RoomDataSource src;
    HRoomFileError err = OpenRoomHeader(kRoomVersion_Current + 1, true, src);
    ASSERT_FALSE(err);
    EXPECT_EQ(err->Code(), kRoomFileErr_FormatNotSupported);
}
