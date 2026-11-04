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
// The savegame file around the components: SaveGame writes the signature,
// the description (engine and game identity, user text, screenshot) and the
// file format offsets; OpenSavegame reads them back and leaves the stream at
// the components, which then restore the game state. Files that are not
// saves, or saves of unsupported format versions, are refused. The files are
// written to a temporary directory.
//
//=============================================================================
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "game/savegame.h"
#include "game/savegame_components.h"
#include "game/savegame_internal.h"
#include "gfx/bitmap.h"
#include "gtest/gtest.h"
#include "main/engine.h"
#include "main/main.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;

namespace
{

// A small screenshot with a pattern that compression cannot hide
std::unique_ptr<Bitmap> MakeScreenshot()
{
    auto bmp = std::make_unique<Bitmap>(8, 6, 32);
    for (int y = 0; y < bmp->GetHeight(); ++y)
    {
        for (int x = 0; x < bmp->GetWidth(); ++x)
            bmp->PutPixel(x, y, (x * 31 + y * 7) | ((x ^ y) << 8) | 0xFF000000);
    }
    return bmp;
}

std::vector<uint8_t> ReadAllBytes(const String &path)
{
    std::vector<uint8_t> bytes;
    auto in = File::OpenFileRead(path);
    if (!in)
        return bytes;
    bytes.resize(static_cast<size_t>(in->GetLength()));
    in->Read(bytes.data(), bytes.size());
    return bytes;
}

void WriteAllBytes(const String &path, const std::vector<uint8_t> &bytes)
{
    auto out = File::CreateFile(path);
    ASSERT_TRUE(out);
    out->Write(bytes.data(), bytes.size());
}

} // namespace

// Fixture for the savegame file. The engine globals it touches, all reset in TearDown:
// - game.guid, game.gamename, game.uniqueid, game.color_depth: the game identity in the description
// - ResPaths.GamePak.Name, loaded_game_file_version: the game data identity in the description
// - play.messagetime: a value of the game state, to see the components restored
// It writes save files to a directory of its own under the system's temporary directory.
class SavegameFile : public ::testing::Test
{
protected:
    void SetUp() override
    {
        _dir = std::filesystem::temp_directory_path() /
               ("ags_savegame_file_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        std::filesystem::create_directories(_dir);
        snprintf(game.guid, sizeof(game.guid), "%s", "{01234567-89AB-CDEF-0123-456789ABCDEF}");
        game.gamename = "Savegame Test";
        game.uniqueid = 0x5A5A1234;
        game.color_depth = 4;
        ResPaths.GamePak.Name = "test.ags";
        loaded_game_file_version = kGameVersion_Current;
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove_all(_dir, ec);
        game.guid[0] = 0;
        game.gamename = {};
        game.uniqueid = 0;
        game.color_depth = 0;
        ResPaths.GamePak.Name = {};
        loaded_game_file_version = kGameVersion_Undefined;
        play.messagetime = 0;
    }

    String Path(const char *name) const { return String((_dir / name).u8string().c_str()); }

    std::filesystem::path _dir;
};

TEST_F(SavegameFile, DescriptionIsReadBack)
{
    const String path = Path("agssave.001");
    const auto screenshot = MakeScreenshot();
    HSaveError err = SaveGame(path, "Before the boss", screenshot.get(), kSaveCmp_GameState, false);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_TRUE(DoesSavegameExist(path));

    SavegameSource src;
    SavegameDescription desc;
    err = OpenSavegame(path, src, desc, kSvgDesc_All);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(kSvgVersion_Current, src.Version);
    EXPECT_STREQ(get_engine_name(), desc.EngineName.GetCStr());
    EXPECT_STREQ(EngineVersion.LongString.GetCStr(), desc.EngineVersion.LongString.GetCStr());
    EXPECT_STREQ("{01234567-89AB-CDEF-0123-456789ABCDEF}", desc.GameGuid.GetCStr());
    EXPECT_STREQ("Savegame Test", desc.GameTitle.GetCStr());
    EXPECT_STREQ("test.ags", desc.MainDataFilename.GetCStr());
    EXPECT_EQ(kGameVersion_Current, desc.MainDataVersion);
    EXPECT_EQ(32, desc.ColorDepth);
    EXPECT_EQ(0x5A5A1234, desc.LegacyID);
    EXPECT_STREQ("Before the boss", desc.UserText.GetCStr());
    EXPECT_EQ(0u, desc.Format.Flags & kSvgFmt_DeflateComponents);

    // the screenshot comes back pixel for pixel (it is always compressed)
    ASSERT_TRUE(desc.UserImage);
    ASSERT_EQ(screenshot->GetSize(), desc.UserImage->GetSize());
    for (int y = 0; y < screenshot->GetHeight(); ++y)
    {
        for (int x = 0; x < screenshot->GetWidth(); ++x)
            EXPECT_EQ(screenshot->GetPixel(x, y), desc.UserImage->GetPixel(x, y)) << x << "," << y;
    }
    // the stream is left at the game data
    ASSERT_TRUE(src.InputStream);
    EXPECT_EQ(desc.Format.GameDataOffset, src.InputStream->GetPosition());
}

TEST_F(SavegameFile, ComponentsFollowTheDescription)
{
    const String path = Path("agssave.002");
    for (bool compress : {false, true})
    {
        play.messagetime = 4321;
        HSaveError err = SaveGame(path, "text", nullptr, kSaveCmp_GameState, compress);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        play.messagetime = 0;

        SavegameSource src;
        SavegameDescription desc;
        err = OpenSavegame(path, src, desc, kSvgDesc_All);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        EXPECT_EQ(compress, (desc.Format.Flags & kSvgFmt_DeflateComponents) != 0);
        EXPECT_FALSE(desc.UserImage);

        PreservedParams pp;
        RestoredData r_data;
        // SaveGame adds the object sprites to a selection without the dynamic sprites
        const auto written = static_cast<SaveCmpSelection>(kSaveCmp_GameState | kSaveCmp_ObjectSprites);
        err = SavegameComponents::ReadAll(src.InputStream.get(), src.Version, written, pp, r_data);
        ASSERT_TRUE(err) << err->FullMessage().GetCStr();
        EXPECT_EQ(4321, play.messagetime) << "compress " << compress;
    }
}

TEST_F(SavegameFile, DescriptionPartsCanBeLeftOut)
{
    const String path = Path("agssave.003");
    const auto screenshot = MakeScreenshot();
    ASSERT_TRUE(SaveGame(path, "only the text", screenshot.get(), kSaveCmp_GameState, false));

    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc, kSvgDesc_UserText);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_STREQ("only the text", desc.UserText.GetCStr());
    EXPECT_FALSE(desc.UserImage);
    EXPECT_TRUE(desc.GameGuid.IsEmpty());
    EXPECT_TRUE(desc.EngineName.IsEmpty());

    SavegameDescription env;
    err = OpenSavegame(path, env, kSvgDesc_EnvInfo);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_TRUE(env.UserText.IsEmpty());
    EXPECT_STREQ("Savegame Test", env.GameTitle.GetCStr());
}

TEST_F(SavegameFile, MissingFileIsReported)
{
    const String path = Path("no_such_save.999");
    EXPECT_FALSE(DoesSavegameExist(path));
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_FileOpenFailed, err->Code());
}

TEST_F(SavegameFile, FileThatIsNotASaveIsRefused)
{
    const String path = Path("not_a_save.txt");
    const std::string text = "This is a text file, long enough to look like it has a header of some sort.";
    WriteAllBytes(path, std::vector<uint8_t>(text.begin(), text.end()));
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_SignatureFailed, err->Code());
}

TEST_F(SavegameFile, UnsupportedFormatVersionsAreRefused)
{
    const String path = Path("agssave.004");
    ASSERT_TRUE(SaveGame(path, "text", nullptr, kSaveCmp_GameState, false));
    std::vector<uint8_t> bytes = ReadAllBytes(path);
    const size_t version_pos = SavegameSource::Signature.GetLength();
    ASSERT_GT(bytes.size(), version_pos + 4);
    // the format version follows the signature, little-endian
    const int32_t versions[] = {kSvgVersion_LowestSupported - 1, kSvgVersion_Current + 1};
    for (int32_t version : versions)
    {
        for (int i = 0; i < 4; ++i)
            bytes[version_pos + i] = static_cast<uint8_t>(version >> (i * 8));
        WriteAllBytes(path, bytes);
        SavegameDescription desc;
        HSaveError err = OpenSavegame(path, desc);
        ASSERT_FALSE(err) << version;
        EXPECT_EQ(kSvgErr_FormatVersionNotSupported, err->Code()) << version;
    }
}

TEST_F(SavegameFile, OlderDescriptionFormatIsRead)
{
    // a 3.6.2 save has no file format block after the format version, so its description is
    // found by sizes rather than offsets; make one from a current save by taking the block out
    const String path = Path("agssave.005");
    ASSERT_TRUE(SaveGame(path, "an older save", nullptr, kSaveCmp_GameState, false));
    const std::vector<uint8_t> save = ReadAllBytes(path);
    const size_t version_pos = SavegameSource::Signature.GetLength();
    const size_t fileformat_size = 5 * sizeof(int32_t);
    std::vector<uint8_t> legacy(save.begin(), save.begin() + version_pos);
    for (int i = 0; i < 4; ++i)
        legacy.push_back(static_cast<uint8_t>(kSvgVersion_362 >> (i * 8)));
    legacy.insert(legacy.end(), save.begin() + version_pos + 4 + fileformat_size, save.end());
    WriteAllBytes(path, legacy);

    SavegameSource src;
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, src, desc, kSvgDesc_All);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(kSvgVersion_362, src.Version);
    EXPECT_STREQ("an older save", desc.UserText.GetCStr());
    EXPECT_STREQ("Savegame Test", desc.GameTitle.GetCStr());
    EXPECT_EQ(0x5A5A1234, desc.LegacyID);

    // saves of old engines on Windows Vista and later could start with a "rich media" header
    std::vector<uint8_t> with_header((6 * sizeof(int32_t)) + 16 + (1024 * sizeof(int16_t) * 4), 0xAB);
    with_header.insert(with_header.end(), legacy.begin(), legacy.end());
    WriteAllBytes(path, with_header);
    SavegameDescription desc2;
    err = OpenSavegame(path, desc2, kSvgDesc_UserText);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_STREQ("an older save", desc2.UserText.GetCStr());
}

TEST_F(SavegameFile, SaveInAMissingDirectoryFails)
{
    HSaveError err = SaveGame(Path("no_such_dir/agssave.001"), "text", nullptr, kSaveCmp_GameState, false);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_FileOpenFailed, err->Code());
}
