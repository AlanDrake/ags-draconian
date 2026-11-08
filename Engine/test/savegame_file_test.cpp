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
#include <chrono>
#include <cstdio>
#include <cstdlib>
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
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
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
        // a name of its own, also when ctest runs the tests in parallel processes
        const auto *test = ::testing::UnitTest::GetInstance()->current_test_info();
        _dir = Path::ConcatPaths(TempRoot(),
                                 String::FromFormat("ags_savegame_file_test_%s_%llu", test->name(),
                                                    static_cast<unsigned long long>(
                                                        std::chrono::steady_clock::now().time_since_epoch().count())));
        ASSERT_TRUE(Directory::CreateDirectory(_dir)) << _dir.GetCStr();
        snprintf(game.guid, sizeof(game.guid), "%s", "{01234567-89AB-CDEF-0123-456789ABCDEF}");
        game.gamename = "Savegame Test";
        game.uniqueid = 0x5A5A1234;
        game.color_depth = 4;
        ResPaths.GamePak.Name = "test.ags";
        loaded_game_file_version = kGameVersion_Current;
    }

    void TearDown() override
    {
        for (const String &file : _files)
            File::DeleteFile(file);
        Directory::DeleteDirectory(_dir);
        game.guid[0] = 0;
        game.gamename = {};
        game.uniqueid = 0;
        game.color_depth = 0;
        ResPaths.GamePak.Name = {};
        loaded_game_file_version = kGameVersion_Undefined;
        play.messagetime = 0;
    }

    // The system's temporary directory
    static String TempRoot()
    {
        for (const char *var : {"TMPDIR", "TEMP", "TMP"})
        {
            const char *value = getenv(var);
            if (value && *value)
                return value;
        }
        return "/tmp";
    }

    // A file in the test's directory, deleted at the end of the test
    String FilePath(const char *name)
    {
        _files.push_back(Path::ConcatPaths(_dir, name));
        return _files.back();
    }

    // Sets the format version, which follows the signature, little-endian
    static void SetFormatVersion(std::vector<uint8_t> &bytes, int32_t version)
    {
        const size_t version_pos = SavegameSource::Signature.GetLength();
        ASSERT_GE(bytes.size(), version_pos + 4);
        for (int i = 0; i < 4; ++i)
        {
            bytes[version_pos + i] = static_cast<uint8_t>(version >> (i * 8));
        }
    }

    String _dir;
    std::vector<String> _files;
};

TEST_F(SavegameFile, DescriptionIsReadBack)
{
    const String path = FilePath("agssave.001");
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
    const String path = FilePath("agssave.002");
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
    const String path = FilePath("agssave.003");
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
    const String path = FilePath("no_such_save.999");
    EXPECT_FALSE(DoesSavegameExist(path));
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_FileOpenFailed, err->Code());
}

TEST_F(SavegameFile, FileThatIsNotASaveIsRefused)
{
    const String path = FilePath("not_a_save.txt");
    const std::string text = "This is a text file, long enough to look like it has a header of some sort.";
    WriteAllBytes(path, std::vector<uint8_t>(text.begin(), text.end()));
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_SignatureFailed, err->Code());
}

TEST_F(SavegameFile, SaveOlderThan400IsRefused)
{
    const String path = FilePath("agssave.004");
    ASSERT_TRUE(SaveGame(path, "text", nullptr, kSaveCmp_GameState, false));
    std::vector<uint8_t> bytes = ReadAllBytes(path);
    ASSERT_GT(bytes.size(), SavegameSource::Signature.GetLength() + 4);
    // 3.6.3 (which has the file format block too), 3.6.2, 3.5.1, the 3.99.99 engines, the first block format
    const int32_t versions[] = {3060300, 3060200, 13, 3999999, 20, 9};
    for (int32_t version : versions)
    {
        SetFormatVersion(bytes, version);
        WriteAllBytes(path, bytes);
        SavegameDescription desc;
        HSaveError err = OpenSavegame(path, desc);
        ASSERT_FALSE(err) << version;
        EXPECT_EQ(kSvgErr_FormatVersionNotSupported, err->Code()) << version;
        const std::string message = err->FullMessage().GetCStr();
        EXPECT_NE(std::string::npos, message.find(String::FromFormat("Required: %d,", version).GetCStr())) << message;
        EXPECT_NE(std::string::npos,
                  message.find(String::FromFormat("supported: 4000000 - %d.", kSvgVersion_Current).GetCStr()))
            << message;
    }
}

TEST_F(SavegameFile, SaveNewerThanTheEngineIsRefused)
{
    const String path = FilePath("agssave.004");
    ASSERT_TRUE(SaveGame(path, "text", nullptr, kSaveCmp_GameState, false));
    std::vector<uint8_t> bytes = ReadAllBytes(path);
    SetFormatVersion(bytes, kSvgVersion_Current + 1);
    WriteAllBytes(path, bytes);
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_FormatVersionNotSupported, err->Code());
}

TEST_F(SavegameFile, DescriptionWithoutFileFormatBlockIsRead)
{
    // a 4.0 save older than 4.0.0.18 has no file format block after the format version, so its
    // description is found by sizes rather than offsets; make one from a current save by taking
    // the block out
    const String path = FilePath("agssave.005");
    ASSERT_TRUE(SaveGame(path, "an older save", nullptr, kSaveCmp_GameState, false));
    const std::vector<uint8_t> save = ReadAllBytes(path);
    const size_t version_pos = SavegameSource::Signature.GetLength();
    const size_t fileformat_size = 5 * sizeof(int32_t);
    std::vector<uint8_t> legacy(save.begin(), save.begin() + version_pos + 4);
    SetFormatVersion(legacy, kSvgVersion_400);
    legacy.insert(legacy.end(), save.begin() + version_pos + 4 + fileformat_size, save.end());
    WriteAllBytes(path, legacy);

    SavegameSource src;
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, src, desc, kSvgDesc_All);
    ASSERT_TRUE(err) << err->FullMessage().GetCStr();
    EXPECT_EQ(kSvgVersion_400, src.Version);
    EXPECT_STREQ("an older save", desc.UserText.GetCStr());
    EXPECT_STREQ("Savegame Test", desc.GameTitle.GetCStr());
    EXPECT_EQ(0x5A5A1234, desc.LegacyID);
}

TEST_F(SavegameFile, RichMediaHeaderIsNotRead)
{
    // saves of engines before 3.6 on Windows Vista and later could start with a "rich media" header;
    // no 4.0 engine wrote it, so such a file is not taken for a save
    const String path = FilePath("agssave.006");
    ASSERT_TRUE(SaveGame(path, "text", nullptr, kSaveCmp_GameState, false));
    std::vector<uint8_t> with_header((6 * sizeof(int32_t)) + 16 + (1024 * sizeof(int16_t) * 4), 0xAB);
    const std::vector<uint8_t> save = ReadAllBytes(path);
    with_header.insert(with_header.end(), save.begin(), save.end());
    WriteAllBytes(path, with_header);
    SavegameDescription desc;
    HSaveError err = OpenSavegame(path, desc, kSvgDesc_UserText);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_SignatureFailed, err->Code());
}

TEST_F(SavegameFile, SaveInAMissingDirectoryFails)
{
    HSaveError err = SaveGame(FilePath("no_such_dir/agssave.001"), "text", nullptr, kSaveCmp_GameState, false);
    ASSERT_FALSE(err);
    EXPECT_EQ(kSvgErr_FileOpenFailed, err->Code());
}
