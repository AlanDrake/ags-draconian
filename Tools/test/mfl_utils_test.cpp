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
#include <cstring>
#include <memory>
#include <string>

#include "data/mfl_utils.h"
#include "data/multifilelib.h"
#include "gtest/gtest.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

namespace
{

void WriteTextFile(const String &path, const char *text)
{
    std::unique_ptr<Stream> out(File::CreateFile(path));
    out->Write(text, strlen(text));
}

// Packs two small assets into lib_file, appending the library if requested. The asset files are named
// after the running test: ctest runs the tests of this file in parallel, in the same directory
HError PackTwoAssets(const String &lib_file, bool append)
{
    const char *test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();
    const String file_a = String::FromFormat("mfl_test_%s_a.txt", test_name);
    const String file_b = String::FromFormat("mfl_test_%s_b.txt", test_name);
    WriteTextFile(file_a, "first");
    WriteTextFile(file_b, "second");
    StringIMap asset_map;
    asset_map["a.txt"] = file_a;
    asset_map["b.txt"] = file_b;
    AssetLibInfo lib;
    HError err = MakeAssetLib(lib, lib_file, asset_map, 0, append);
    if (err)
        err = WriteLibrary(lib, asset_map, ".", MFLUtil::kMFLVersion_MultiV30, append, false);
    File::DeleteFile(file_a);
    File::DeleteFile(file_b);
    return err;
}

bool FileHasEnder(const String &path)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(path));
    return MFLUtil::HasEnderSig(in.get());
}

} // namespace

TEST(MflUtils, StandaloneLibraryHasNoEnder)
{
    // A file of its own begins with the library, so it needs no ender, as the editor writes it
    const String lib_file = "mfl_test_standalone.vox";
    ASSERT_TRUE(PackTwoAssets(lib_file, false));
    EXPECT_FALSE(FileHasEnder(lib_file));
    EXPECT_TRUE(TestLibraryFile(lib_file, nullptr));
    File::DeleteFile(lib_file);
}

TEST(MflUtils, AppendedLibraryHasEnder)
{
    // A library appended to another file is found from the ender at the file's end
    const String lib_file = "mfl_test_appended.exe";
    WriteTextFile(lib_file, "an executable");
    ASSERT_TRUE(PackTwoAssets(lib_file, true));
    EXPECT_TRUE(FileHasEnder(lib_file));
    std::unique_ptr<Stream> in(File::OpenFileRead(lib_file));
    soff_t lib_offset = 0;
    ASSERT_EQ(MFLUtil::kMFLNoError, MFLUtil::ReadOffset(in.get(), lib_offset));
    EXPECT_EQ(static_cast<soff_t>(strlen("an executable")), lib_offset);
    in.reset();
    File::DeleteFile(lib_file);
}

TEST(MflUtils, LibraryInPartsRecordsWhereEveryAssetIs)
{
    // Three assets in parts of at most eight bytes: one asset in each part, the first part being
    // the library file itself. The table of contents in the first part locates all of them.
    const char *names[] = {"a.txt", "b.txt", "c.txt"};
    const char *texts[] = {"first", "second", "the third"};
    StringIMap asset_map;
    for (int i = 0; i < 3; ++i)
    {
        const String file = String::FromFormat("mfl_test_split_%s", names[i]);
        WriteTextFile(file, texts[i]);
        asset_map[names[i]] = file;
    }
    AssetLibInfo lib;
    ASSERT_TRUE(MakeAssetLib(lib, "mfl_test_split.ags", asset_map, 8));
    HError err = WriteLibrary(lib, asset_map, ".", MFLUtil::kMFLVersion_MultiV30, false, false);
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());

    AssetLibInfo read_lib;
    {
        std::unique_ptr<Stream> in(File::OpenFileRead("mfl_test_split.ags"));
        ASSERT_TRUE(in != nullptr);
        ASSERT_EQ(MFLUtil::kMFLNoError, MFLUtil::ReadHeader(read_lib, in.get()));
    }
    ASSERT_EQ(3u, read_lib.LibFileNames.size());
    EXPECT_STREQ("mfl_test_split.ags", read_lib.LibFileNames[0].GetCStr());
    EXPECT_STREQ("mfl_test_split.001", read_lib.LibFileNames[1].GetCStr());
    EXPECT_STREQ("mfl_test_split.002", read_lib.LibFileNames[2].GetCStr());
    ASSERT_EQ(3u, read_lib.AssetInfos.size());
    for (int i = 0; i < 3; ++i)
    {
        const AssetInfo &asset = read_lib.AssetInfos[i];
        EXPECT_STREQ(names[i], asset.FileName.GetCStr());
        EXPECT_EQ(i, asset.LibUid);
        std::unique_ptr<Stream> in(File::OpenFileRead(read_lib.LibFileNames[asset.LibUid]));
        ASSERT_TRUE(in != nullptr);
        std::string data(static_cast<size_t>(asset.Size), '\0');
        in->Seek(asset.Offset, kSeekBegin);
        in->Read(&data[0], data.size());
        EXPECT_EQ(texts[i], data);
    }

    for (const auto &asset : asset_map)
    {
        File::DeleteFile(asset.second);
    }
    for (const String &part : read_lib.LibFileNames)
    {
        File::DeleteFile(part);
    }
}
