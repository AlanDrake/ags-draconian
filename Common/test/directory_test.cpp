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
#include <cstdlib>
#include <memory>

#include "platform/platform.h"
#if !AGS_PLATFORM_OS_WINDOWS
#include <unistd.h>
#endif
#include "gtest/gtest.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"

using AGS::Common::Stream;
using AGS::Common::String;
namespace Directory = AGS::Common::Directory;
namespace File = AGS::Common::File;
namespace Path = AGS::Common::Path;

namespace
{

void WriteTestFile(const String &path)
{
    std::unique_ptr<Stream> out(File::CreateFile(path));
    ASSERT_TRUE(out != nullptr) << path.GetCStr();
    out->Write("data", 4);
}

// A fresh folder in the working directory
String MakeTestDir(const char *name)
{
    Directory::DeleteDirectoryTree(name);
    EXPECT_TRUE(Directory::CreateDirectory(name));
    return name;
}

// Makes link_path a link to the directory target; false if this system does not let the test make one
bool MakeDirectoryLink(const String &target, const String &link_path)
{
#if AGS_PLATFORM_OS_WINDOWS
    // A junction, which needs no privileges, unlike a symbolic link
    const String command = String::FromFormat("mklink /J \"%s\" \"%s\" >nul", link_path.GetCStr(), target.GetCStr());
    return std::system(command.GetCStr()) == 0 && File::IsDirectory(link_path);
#else
    return symlink(Path::MakeAbsolutePath(target).GetCStr(), link_path.GetCStr()) == 0;
#endif
}

} // namespace

TEST(Directory, DeleteDirectoryTreeDeletesEverythingInIt)
{
    const String dir = MakeTestDir("delete_tree_test");
    ASSERT_TRUE(Directory::CreateAllDirectories(dir, "a/b/c"));
    ASSERT_TRUE(Directory::CreateAllDirectories(dir, "empty"));
    WriteTestFile(Path::ConcatPaths(dir, "top.txt"));
    WriteTestFile(Path::ConcatPaths(dir, "a/one.txt"));
    WriteTestFile(Path::ConcatPaths(dir, "a/b/c/deep.txt"));

    String failed_path;
    EXPECT_TRUE(Directory::DeleteDirectoryTree(dir, &failed_path));
    EXPECT_TRUE(failed_path.IsEmpty()) << failed_path.GetCStr();
    EXPECT_FALSE(File::IsDirectory(dir));
}

TEST(Directory, DeleteDirectoryTreeOfNothingSucceeds)
{
    String failed_path;
    EXPECT_TRUE(Directory::DeleteDirectoryTree("delete_tree_test_none", &failed_path));
    EXPECT_TRUE(failed_path.IsEmpty());
}

// Asked to delete a directory, it does not delete a file
TEST(Directory, DeleteDirectoryTreeLeavesAFile)
{
    const String dir = MakeTestDir("delete_tree_test_file");
    const String file = Path::ConcatPaths(dir, "file.txt");
    WriteTestFile(file);

    String failed_path;
    EXPECT_FALSE(Directory::DeleteDirectoryTree(file, &failed_path));
    EXPECT_STREQ(failed_path.GetCStr(), file.GetCStr());
    EXPECT_TRUE(File::IsFile(file));
    EXPECT_TRUE(Directory::DeleteDirectoryTree(dir));
}

// A link to a directory is deleted, the directory it points to and its files stay
TEST(Directory, DeleteDirectoryTreeDoesNotFollowLinks)
{
    const String target = MakeTestDir("delete_tree_test_target");
    WriteTestFile(Path::ConcatPaths(target, "keep.txt"));
    const String dir = MakeTestDir("delete_tree_test_links");
    if (!MakeDirectoryLink(target, Path::ConcatPaths(dir, "link")))
    {
        Directory::DeleteDirectoryTree(dir);
        Directory::DeleteDirectoryTree(target);
        // Not GTEST_SKIP: the googletest of the MSBuild projects is older than it
        SUCCEED() << "cannot make a link to a directory here: nothing checked";
        return;
    }

    EXPECT_TRUE(Directory::DeleteDirectoryTree(dir));
    EXPECT_FALSE(File::IsDirectory(dir));
    EXPECT_TRUE(File::IsFile(Path::ConcatPaths(target, "keep.txt")));
    EXPECT_TRUE(Directory::DeleteDirectoryTree(target));
}
