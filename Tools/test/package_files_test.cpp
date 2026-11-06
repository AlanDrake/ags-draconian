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
// Tests for the rules by which agsbuild chooses the files of a game's packages, and for the tasks
// which stage and deploy them (docs/refactor/PLAN.md, 6.7d-3).
//
//=============================================================================
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "agsbuild/package_files.h"
#include "data/include_utils.h"
#include "data/mfl_utils.h"
#include "data/multifilelib.h"
#include "gtest/gtest.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

namespace AGSBuild
{

using AGS::Common::AssetLibInfo;
using AGS::Common::FindFile;
using AGS::Common::Stream;
using AGS::Common::StringIMap;
using AGS::DataUtil::MakeAssetLib;
using AGS::DataUtil::MatchPatternPaths;
using AGS::DataUtil::WriteLibrary;
namespace Directory = AGS::Common::Directory;
namespace File = AGS::Common::File;
namespace MFLUtil = AGS::Common::MFLUtil;
namespace Path = AGS::Common::Path;

namespace
{

// Strings that gtest can print
std::vector<std::string> Strs(const std::vector<String> &list)
{
    std::vector<std::string> strs;
    for (const String &s : list)
    {
        strs.emplace_back(s.GetCStr());
    }
    return strs;
}

std::vector<std::string> Sorted(std::vector<std::string> list)
{
    std::sort(list.begin(), list.end());
    return list;
}

void WriteTextFile(const String &path, const char *text)
{
    std::unique_ptr<Stream> out(File::CreateFile(path));
    ASSERT_TRUE(out != nullptr) << path.GetCStr();
    out->Write(text, strlen(text));
}

std::string ReadTextFile(const String &path)
{
    std::unique_ptr<Stream> in(File::OpenFileRead(path));
    if (!in)
    {
        return "<no file>";
    }
    std::string text(static_cast<size_t>(in->GetLength()), '\0');
    in->Read(&text[0], text.size());
    return text;
}

// The files in a folder and its sub-folders, by their paths in the folder
std::vector<std::string> FilesIn(const String &dir)
{
    std::vector<std::string> files;
    for (FindFile ff = FindFile::OpenFilesRecursive(dir); !ff.AtEnd(); ff.Next())
    {
        files.emplace_back(ff.Current().GetCStr());
    }
    return Sorted(files);
}

// Deletes a folder made by a test, with the files and folders in it
void RemoveTestDir(const String &dir)
{
    for (const std::string &file : FilesIn(dir))
    {
        File::DeleteFile(Path::ConcatPaths(dir, file.c_str()));
    }
    std::vector<String> sub_dirs;
    Directory::GetDirs(dir, sub_dirs);
    for (const String &sub_dir : sub_dirs)
    {
        RemoveTestDir(Path::ConcatPaths(dir, sub_dir));
    }
    Directory::DeleteDirectory(dir);
}

// A fresh, empty folder in the working directory
String MakeTestDir(const char *name)
{
    RemoveTestDir(name);
    EXPECT_TRUE(Directory::CreateDirectory(name));
    return name;
}

// Writes a package of three assets of ten bytes into the folder, in parts of at most part_size bytes
void WriteTestPackage(const String &dir, const String &package_name, soff_t part_size)
{
    StringIMap asset_map;
    for (const char *name : {"a.txt", "b.txt", "c.txt"})
    {
        const String path = Path::ConcatPaths(dir, name);
        WriteTextFile(path, "0123456789");
        asset_map[name] = path;
    }
    AssetLibInfo lib;
    ASSERT_TRUE(MakeAssetLib(lib, Path::ConcatPaths(dir, package_name), asset_map, part_size));
    ASSERT_TRUE(WriteLibrary(lib, asset_map, dir, MFLUtil::kMFLVersion_MultiV30, false, false));
    for (const auto &asset : asset_map)
    {
        File::DeleteFile(asset.second);
    }
}

} // namespace

//-----------------------------------------------------------------------------
// Files of the project folder
//-----------------------------------------------------------------------------

TEST(PackageFiles, SelectsPreloadFlicAndVideoFilesOfTheProjectFolder)
{
    const std::vector<String> files = {"Game.agf",  "preload.pcx",   "Preload.PCX.bak", "flic1.flc", "FLIC2.FLI",
                                       "flic.fl",   "flic3.flac",    "aflic1.flc",      "flic.txt",  "intro.ogv",
                                       "Outro.OGV", "video.ogv.txt", "room1.crm",       "music.ogg"};
    EXPECT_EQ(Strs(SelectPackedProjectFiles(files)),
              (std::vector<std::string>{"FLIC2.FLI", "Outro.OGV", "flic.fl", "flic1.flc", "intro.ogv", "preload.pcx"}));
}

//-----------------------------------------------------------------------------
// Custom data folders
//-----------------------------------------------------------------------------

TEST(PackageFiles, SortsFolderNamesAsTheEditorDoes)
{
    // The order which .NET Framework's Array.Sort gives (Windows NLS, CompareStringEx without flags)
    const std::vector<std::string> editor_order = {".",
                                                   "../outside",
                                                   "_x",
                                                   "1x",
                                                   "a",
                                                   "A",
                                                   "b",
                                                   "B",
                                                   "coop",
                                                   "co-op",
                                                   "coop/x",
                                                   "co-op/x",
                                                   "CustomData",
                                                   "CustomData/Nested",
                                                   "customdata/x",
                                                   "data",
                                                   "Data",
                                                   "Data 1",
                                                   "Data(1)",
                                                   "Data.1",
                                                   "Data/_",
                                                   "Data/1",
                                                   "Data/a",
                                                   "DATA/x",
                                                   "Data[1]",
                                                   "Data_1",
                                                   "Data+1",
                                                   "Data1",
                                                   "Data-1",
                                                   "data2",
                                                   "DataA",
                                                   "Extra/Deep",
                                                   "its",
                                                   "it's",
                                                   "music",
                                                   "music10",
                                                   "Music2",
                                                   "Speech/Loose",
                                                   "x/y",
                                                   "x\\y",
                                                   "x_",
                                                   "x1"};
    std::vector<String> names;
    for (auto it = editor_order.rbegin(); it != editor_order.rend(); ++it)
    {
        names.emplace_back(it->c_str());
    }
    std::sort(names.begin(), names.end(),
              [](const String &s1, const String &s2) { return CompareAsEditorSorts(s1, s2) < 0; });
    EXPECT_EQ(Strs(names), editor_order);
    EXPECT_EQ(0, CompareAsEditorSorts("Data/x", "Data/x"));
    EXPECT_GT(CompareAsEditorSorts("b", "A"), 0);
}

TEST(PackageFiles, CustomDataDirsRejectAbsoluteDotAndRestrictedPaths)
{
    const CustomDataDirs dirs =
        SelectCustomDataDirs({"Extra/Deep", "CustomData", "CustomData/Nested", "Speech/Loose", "../outside", "."});
    // Sorted; the nested folder is packed with its parent, without a warning
    EXPECT_EQ(Strs(dirs.Dirs), (std::vector<std::string>{"CustomData", "Extra/Deep"}));
    EXPECT_EQ(Strs(dirs.Warnings),
              (std::vector<std::string>{"Cannot use path containing '.' or '..' as a custom data source: .",
                                        "Cannot use path containing '.' or '..' as a custom data source: ../outside",
                                        "Cannot use restricted location as a custom data source: Speech/Loose"}));
}

TEST(PackageFiles, CustomDataDirsRejectEveryRestrictedFolderInAnyCase)
{
    const CustomDataDirs dirs = SelectCustomDataDirs(
        {"Compiled", "compiled/Data", "_Debug", "AUDIOCACHE", "Speech", "speech\\French", "Speeches", "Data/Speech"});
    EXPECT_EQ(Strs(dirs.Dirs), (std::vector<std::string>{"Data/Speech", "Speeches"}));
    EXPECT_EQ(dirs.Warnings.size(), 6u);
    for (const String &warning : dirs.Warnings)
    {
        EXPECT_TRUE(warning.StartsWith("Cannot use restricted location as a custom data source: "))
            << warning.GetCStr();
    }
}

TEST(PackageFiles, CustomDataDirsRejectAbsolutePaths)
{
    const CustomDataDirs dirs = SelectCustomDataDirs({"/usr/share", "\\\\server\\share", "C:\\Data", "C:Data", "Data"});
    EXPECT_EQ(Strs(dirs.Dirs), (std::vector<std::string>{"Data"}));
    EXPECT_EQ(Strs(dirs.Warnings),
              (std::vector<std::string>{"Cannot use absolute path as a custom data source: /usr/share",
                                        "Cannot use absolute path as a custom data source: \\\\server\\share",
                                        "Cannot use absolute path as a custom data source: C:\\Data",
                                        "Cannot use absolute path as a custom data source: C:Data"}));
}

TEST(PackageFiles, CustomDataDirsSkipRepeatedAndNestedFoldersSilently)
{
    const CustomDataDirs dirs = SelectCustomDataDirs(
        {"Data\\Deep\\Deeper", "data/deep", "Data/Deep/", "Other", "", "Data//Deep", "OTHER/x", "Data/Deeper"});
    // Backslashes, repeated and trailing separators and the letter case do not make another folder;
    // the first in the editor's order ("Data//Deep") gives the name
    EXPECT_EQ(Strs(dirs.Dirs), (std::vector<std::string>{"Data/Deep", "Data/Deeper", "Other"}));
    EXPECT_TRUE(dirs.Warnings.empty());
}

//-----------------------------------------------------------------------------
// Voice packs
//-----------------------------------------------------------------------------

TEST(PackageFiles, VoicePackHoldsVoiceFilesAndLipSyncData)
{
    for (const char *name :
         {"ego1.ogg", "ego2.flac", "ego3.wav", "ego4.mp3", "EGO5.OGG", "syncdata.dat", "SyncData.DAT"})
    {
        EXPECT_TRUE(IsVoicePackFile(name)) << name;
    }
    for (const char *name : {"notes.txt", "ego1.pam", "ego1.dat", "ego1.ogg.bak", "ogg", "ego1.oga", "readme"})
    {
        EXPECT_FALSE(IsVoicePackFile(name)) << name;
    }
}

TEST(PackageFiles, VoicePackIsNamedAfterTheFolderInLowerCase)
{
    EXPECT_STREQ("speech.vox", GetVoicePackName("").GetCStr());
    EXPECT_STREQ("sp_french.vox", GetVoicePackName("French").GetCStr());
    EXPECT_STREQ("sp_old english.vox", GetVoicePackName("Old English").GetCStr());
}

TEST(PackageFiles, VoiceFilesKeepThePathInsideSpeech)
{
    EXPECT_STREQ("ego1.ogg", GetVoiceAssetName({"speech.vox", ""}, "ego1.ogg").GetCStr());
    EXPECT_STREQ("French/ego1.ogg", GetVoiceAssetName({"sp_french.vox", "French"}, "ego1.ogg").GetCStr());
}

TEST(PackageFiles, PlansOneVoicePackForEachSpeechFolder)
{
    std::vector<String> warnings;
    const std::vector<VoicePack> packs = PlanVoicePacks({"Notes", "French", "french", "German"}, warnings);
    ASSERT_EQ(packs.size(), 4u);
    EXPECT_STREQ("speech.vox", packs[0].PackName.GetCStr());
    EXPECT_STREQ("", packs[0].SubDir.GetCStr());
    EXPECT_STREQ("sp_french.vox", packs[1].PackName.GetCStr());
    EXPECT_STREQ("French", packs[1].SubDir.GetCStr());
    EXPECT_STREQ("sp_german.vox", packs[2].PackName.GetCStr());
    EXPECT_STREQ("sp_notes.vox", packs[3].PackName.GetCStr());
    // Two folders of one name can exist on a case-sensitive file system only
    ASSERT_EQ(warnings.size(), 1u);
    EXPECT_STREQ("Speech folder 'french' is not packed: folder 'French' makes the voice pack sp_french.vox",
                 warnings[0].GetCStr());
}

TEST(PackageFiles, RecognisesVoicePacksByName)
{
    for (const char *name : {"speech.vox", "Speech.VOX", "sp_french.vox", "SP_Old English.Vox", "sp_x.vox"})
    {
        EXPECT_TRUE(IsVoicePackName(name)) << name;
    }
    for (const char *name : {"audio.vox", "sp_.vox", "sp_french.vox.bak", "french.vox", "speech.ags", "sp_french"})
    {
        EXPECT_FALSE(IsVoicePackName(name)) << name;
    }
}

TEST(PackageFiles, FindsVoicePacksOfAnEarlierBuild)
{
    const std::vector<String> dir_files = {"game.ags",      "acsetup.cfg",   "audio.vox", "Speech.vox",
                                           "sp_french.vox", "sp_german.vox", "notes.vox", "sp_french.txt"};
    // The pack of a folder which is gone or has no voice files any more, but no other file
    EXPECT_EQ(Strs(FindStaleVoicePacks({"speech.vox", "sp_french.vox"}, dir_files)),
              (std::vector<std::string>{"sp_german.vox"}));
    EXPECT_EQ(Strs(FindStaleVoicePacks({}, dir_files)),
              (std::vector<std::string>{"Speech.vox", "sp_french.vox", "sp_german.vox"}));
    EXPECT_TRUE(FindStaleVoicePacks({"SP_GERMAN.VOX", "sp_french.vox", "speech.vox"}, dir_files).empty());
}

//-----------------------------------------------------------------------------
// Patterns and package parts
//-----------------------------------------------------------------------------

TEST(PackageFiles, IncludePatternMatchesItsFileOnly)
{
    const std::vector<String> files = {"file1.txt",
                                       "CustomData/file1.txt",
                                       "AudioCache/file1.txt",
                                       "file1.txt.bak",
                                       "afile1.txt",
                                       "file1xtxt",
                                       "#notes.txt",
                                       "!important.txt",
                                       "important.txt",
                                       "take [1].ogv",
                                       "take 1.ogv",
                                       "what?.ogv",
                                       "whatX.ogv",
                                       "all*.ogv",
                                       "all of it.ogv",
                                       "Data/Sub/b.txt",
                                       "Other/Data/Sub/b.txt",
                                       "Data/Sub/b.txt2"};
    for (const String &file : files)
    {
        std::vector<String> matches;
        MatchPatternPaths(files, matches, {MakeIncludePattern(file)});
        EXPECT_EQ(Strs(matches), (std::vector<std::string>{file.GetCStr()}))
            << "pattern " << MakeIncludePattern(file).GetCStr();
    }
    EXPECT_STREQ("/Data/take [[]1].ogv", MakeIncludePattern("Data/take [1].ogv").GetCStr());
}

TEST(PackageFiles, IncludePatternIsNotCaseSensitive)
{
    // As the engine looks assets up
    std::vector<String> matches;
    MatchPatternPaths({"Preload.PCX", "Sub/preload.pcx"}, matches, {MakeIncludePattern("preload.pcx")});
    EXPECT_EQ(Strs(matches), (std::vector<std::string>{"Preload.PCX"}));
}

TEST(PackageFiles, RecognisesPackagePartsByName)
{
    EXPECT_TRUE(IsPackagePartName("game.ags", "game.001"));
    EXPECT_TRUE(IsPackagePartName("game.ags", "GAME.255"));
    EXPECT_TRUE(IsPackagePartName("my.game.ags", "my.game.012"));
    EXPECT_TRUE(IsPackagePartName(".ags", ".001"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "game.ags"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "game.01"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "game.0001"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "game.1D3")); // the name the editor gave a part
    EXPECT_FALSE(IsPackagePartName("game.ags", "game2.001"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "game.001.bak"));
    EXPECT_FALSE(IsPackagePartName("game.ags", "audio.vox"));
}

TEST(PackageFiles, FindsPartsOfAnEarlierBuild)
{
    const std::vector<String> dir_files = {"game.ags", "game.001",  "game.002",  "Game.003",
                                           "game.004", "other.001", "audio.vox", "acsetup.cfg"};
    EXPECT_EQ(Strs(FindStalePackageParts("game.ags", {"game.ags", "game.001", "game.002"}, dir_files)),
              (std::vector<std::string>{"Game.003", "game.004"}));
    EXPECT_EQ(Strs(FindStalePackageParts("game.ags", {"game.ags"}, dir_files)),
              (std::vector<std::string>{"Game.003", "game.001", "game.002", "game.004"}));
    EXPECT_TRUE(
        FindStalePackageParts("game.ags", {"game.ags", "game.001", "game.002", "game.003", "game.004"}, dir_files)
            .empty());
}

TEST(PackageFiles, ReadsPartNamesFromThePackage)
{
    const String dir = MakeTestDir("package_files_test_parts");
    WriteTestPackage(dir, "split.ags", 10);
    WriteTestPackage(dir, "whole.ags", 0);
    WriteTextFile(Path::ConcatPaths(dir, "text.ags"), "not a package");

    std::vector<String> parts;
    EXPECT_TRUE(ReadPackagePartNames(Path::ConcatPaths(dir, "split.ags"), parts));
    EXPECT_EQ(Strs(parts), (std::vector<std::string>{"split.ags", "split.001", "split.002"}));
    parts.clear();
    EXPECT_TRUE(ReadPackagePartNames(Path::ConcatPaths(dir, "whole.ags"), parts));
    EXPECT_EQ(Strs(parts), (std::vector<std::string>{"whole.ags"}));
    parts.clear();
    EXPECT_FALSE(ReadPackagePartNames(Path::ConcatPaths(dir, "text.ags"), parts));
    EXPECT_FALSE(ReadPackagePartNames(Path::ConcatPaths(dir, "missing.ags"), parts));
    EXPECT_TRUE(parts.empty());
    RemoveTestDir(dir);
}

//-----------------------------------------------------------------------------
// Tasks
//-----------------------------------------------------------------------------

TEST(PackageFiles, StageTaskLeavesExactlyTheGivenFilesInTheFolder)
{
    const String src_dir = MakeTestDir("package_files_test_src");
    const String stage_dir = MakeTestDir("package_files_test_stage");
    WriteTextFile(Path::ConcatPaths(src_dir, "one.txt"), "one");
    WriteTextFile(Path::ConcatPaths(src_dir, "two.txt"), "two");
    // What an earlier build left: a file which is staged again, and two which are not
    ASSERT_TRUE(Directory::CreateAllDirectories(stage_dir, "Old/Deep"));
    WriteTextFile(Path::ConcatPaths(stage_dir, "one.txt"), "old one");
    WriteTextFile(Path::ConcatPaths(stage_dir, "stale.txt"), "stale");
    WriteTextFile(Path::ConcatPaths(stage_dir, "Old/Deep/stale.txt"), "stale");

    const std::vector<std::pair<String, String>> files = {
        {Path::ConcatPaths(src_dir, "one.txt"), "one.txt"},
        {Path::ConcatPaths(src_dir, "two.txt"), "French/Deep/two.txt"},
        {Path::ConcatPaths(src_dir, "one.txt"), "French/one.txt"}};
    TaskStageFiles task("stage", nullptr, stage_dir, files, {});
    HError err = task.Run();
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());
    EXPECT_EQ(TaskState::Success, task.GetState());

    EXPECT_EQ(FilesIn(stage_dir), (std::vector<std::string>{"French/Deep/two.txt", "French/one.txt", "one.txt"}));
    EXPECT_EQ("one", ReadTextFile(Path::ConcatPaths(stage_dir, "one.txt")));
    EXPECT_EQ("two", ReadTextFile(Path::ConcatPaths(stage_dir, "French/Deep/two.txt")));
    // The sources stay
    EXPECT_EQ(FilesIn(src_dir), (std::vector<std::string>{"one.txt", "two.txt"}));
    RemoveTestDir(src_dir);
    RemoveTestDir(stage_dir);
}

TEST(PackageFiles, StageTaskFailsWithoutASourceFile)
{
    const String stage_dir = MakeTestDir("package_files_test_stage2");
    TaskStageFiles task("stage", nullptr, stage_dir, {{"package_files_test_no_such_file.txt", "file.txt"}}, {});
    EXPECT_FALSE(task.Run());
    EXPECT_EQ(TaskState::Failure, task.GetState());
    RemoveTestDir(stage_dir);
}

TEST(PackageFiles, DeployTaskMovesEveryPartAndDeletesOldOnes)
{
    const String src_dir = MakeTestDir("package_files_test_temp");
    const String out_dir = MakeTestDir("package_files_test_out");
    WriteTestPackage(src_dir, "game.ags", 10);
    // An earlier build of four parts, and files which are not parts of this package
    for (const char *name : {"game.ags", "game.001", "game.002", "game.003", "audio.vox", "other.001"})
    {
        WriteTextFile(Path::ConcatPaths(out_dir, name), "old");
    }

    TaskDeployPackage task("deploy", nullptr, src_dir, out_dir, "game.ags", {});
    HError err = task.Run();
    EXPECT_TRUE(err) << (err ? "" : err->FullMessage().GetCStr());

    EXPECT_TRUE(FilesIn(src_dir).empty());
    EXPECT_EQ(FilesIn(out_dir),
              (std::vector<std::string>{"audio.vox", "game.001", "game.002", "game.ags", "other.001"}));
    // The package in the output folder is the new one, and complete
    std::vector<String> parts;
    EXPECT_TRUE(ReadPackagePartNames(Path::ConcatPaths(out_dir, "game.ags"), parts));
    EXPECT_EQ(Strs(parts), (std::vector<std::string>{"game.ags", "game.001", "game.002"}));
    EXPECT_NE("old", ReadTextFile(Path::ConcatPaths(out_dir, "game.002")));
    EXPECT_EQ("old", ReadTextFile(Path::ConcatPaths(out_dir, "other.001")));
    RemoveTestDir(src_dir);
    RemoveTestDir(out_dir);
}

TEST(PackageFiles, DeployTaskOfAWholePackageDeletesAllOldParts)
{
    const String src_dir = MakeTestDir("package_files_test_temp2");
    const String out_dir = MakeTestDir("package_files_test_out2");
    WriteTestPackage(src_dir, "game.ags", 0);
    for (const char *name : {"game.001", "game.002"})
    {
        WriteTextFile(Path::ConcatPaths(out_dir, name), "old");
    }

    TaskDeployPackage task("deploy", nullptr, src_dir, out_dir, "game.ags", {});
    EXPECT_TRUE(task.Run());
    EXPECT_EQ(FilesIn(out_dir), (std::vector<std::string>{"game.ags"}));
    RemoveTestDir(src_dir);
    RemoveTestDir(out_dir);
}

TEST(PackageFiles, DeployTaskFailsWithoutAPackageAndKeepsTheOutput)
{
    const String src_dir = MakeTestDir("package_files_test_temp3");
    const String out_dir = MakeTestDir("package_files_test_out3");
    WriteTextFile(Path::ConcatPaths(out_dir, "game.001"), "old");

    TaskDeployPackage task("deploy", nullptr, src_dir, out_dir, "game.ags", {});
    EXPECT_FALSE(task.Run());
    EXPECT_EQ(FilesIn(out_dir), (std::vector<std::string>{"game.001"}));
    RemoveTestDir(src_dir);
    RemoveTestDir(out_dir);
}

} // namespace AGSBuild
