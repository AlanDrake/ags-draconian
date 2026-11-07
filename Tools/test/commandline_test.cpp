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
// Tests of agsbuild's command line (docs/refactor/PLAN.md, 6.7d-6): what the editor's runner
// passes is read as meant, anything else is an error that names the argument.
#include <initializer_list>
#include <vector>

#include "agsbuild/commandline.h"
#include "gtest/gtest.h"

using AGS::Common::String;
using AGSBuild::CommandLine;
using AGSBuild::ParseCommandLine;

namespace
{

CommandLine Parse(std::initializer_list<const char *> args)
{
    std::vector<String> list;
    for (const char *arg : args)
    {
        list.emplace_back(arg);
    }
    return ParseCommandLine(list);
}

bool ErrorNames(const CommandLine &cmd, const char *what)
{
    return cmd.Error.FindString(what) != String::NoIndex;
}

} // namespace

// The command line of the editor's BuildToolOptions.ToArguments, every option once
TEST(CommandLine, ReadsWhatTheEditorPasses)
{
    const CommandLine cmd = Parse({"E:/game/Game.agf", "E:/game/_Debug", "--json", "--sys-dir", "E:/ags", "--temp-dir",
                                   "E:/tmp", "--api-ash", "E:/ags/agsdefns.sh", "--header", "E:/h/__Plugin_a.ash",
                                   "--header", "E:/h/b.ash", "--data-only", "--strict", "--threads", "3", "--rebuild"});
    EXPECT_TRUE(cmd.Error.IsEmpty()) << cmd.Error.GetCStr();
    EXPECT_FALSE(cmd.Help);
    EXPECT_TRUE(cmd.Json);
    EXPECT_STREQ(cmd.ProjectFile.GetCStr(), "E:/game/Game.agf");
    EXPECT_STREQ(cmd.OutputDir.GetCStr(), "E:/game/_Debug");
    EXPECT_STREQ(cmd.SysDir.GetCStr(), "E:/ags");
    EXPECT_STREQ(cmd.TempDir.GetCStr(), "E:/tmp");
    EXPECT_STREQ(cmd.ApiHeader.GetCStr(), "E:/ags/agsdefns.sh");
    ASSERT_EQ(cmd.Headers.size(), 2u);
    EXPECT_STREQ(cmd.Headers[0].GetCStr(), "E:/h/__Plugin_a.ash");
    EXPECT_STREQ(cmd.Headers[1].GetCStr(), "E:/h/b.ash");
    EXPECT_TRUE(cmd.DataOnly);
    EXPECT_TRUE(cmd.Strict);
    EXPECT_EQ(cmd.Threads, 3);
    EXPECT_TRUE(cmd.Rebuild);
    EXPECT_FALSE(cmd.Cleanup);
}

TEST(CommandLine, ReadsShortOptionsAndFlags)
{
    const CommandLine cmd = Parse({"-v", "Game.agf", "-s", "tools", "out", "-t", "tmp", "-p", "2", "-c",
                                   "--import-audio", "--import-sprites", "--build-debug", "--build-release"});
    EXPECT_TRUE(cmd.Error.IsEmpty()) << cmd.Error.GetCStr();
    EXPECT_STREQ(cmd.ProjectFile.GetCStr(), "Game.agf");
    EXPECT_STREQ(cmd.OutputDir.GetCStr(), "out");
    EXPECT_STREQ(cmd.SysDir.GetCStr(), "tools");
    EXPECT_STREQ(cmd.TempDir.GetCStr(), "tmp");
    EXPECT_EQ(cmd.Threads, 2);
    EXPECT_TRUE(cmd.Verbose);
    EXPECT_TRUE(cmd.Cleanup);
    EXPECT_TRUE(cmd.ImportAudio);
    EXPECT_TRUE(cmd.ImportSprites);
    EXPECT_TRUE(cmd.BuildDebug);
    EXPECT_TRUE(cmd.BuildRelease);
    EXPECT_FALSE(cmd.Rebuild);
    EXPECT_FALSE(cmd.Json);
    EXPECT_TRUE(cmd.Headers.empty());
}

// An option's value is the next argument, whatever it looks like
TEST(CommandLine, ValueMayStartWithDash)
{
    const CommandLine cmd = Parse({"Game.agf", "out", "--temp-dir", "-odd"});
    EXPECT_TRUE(cmd.Error.IsEmpty()) << cmd.Error.GetCStr();
    EXPECT_STREQ(cmd.TempDir.GetCStr(), "-odd");
}

TEST(CommandLine, UnknownOptionIsAnError)
{
    const CommandLine cmd = Parse({"Game.agf", "out", "--frobnicate"});
    EXPECT_TRUE(ErrorNames(cmd, "'--frobnicate'")) << cmd.Error.GetCStr();
}

// CmdLineOpts reads "-sX" as "-s X"; agsbuild does not guess
TEST(CommandLine, ValueWrittenIntoTheOptionIsAnError)
{
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "-stools"}), "'-stools'"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "--threads=2"}), "'--threads=2'"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "-strict"}), "'-strict'"));
}

TEST(CommandLine, MissingValueIsAnError)
{
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "--header"}), "'--header'"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "--header", ""}), "'--header'"));
}

TEST(CommandLine, ThreadsMustBeANumber)
{
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "-p", "many"}), "'many'"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "-p", "0"}), "'0'"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "-p", "2x"}), "'2x'"));
}

TEST(CommandLine, PositionalArgumentsMustBeTwo)
{
    EXPECT_TRUE(ErrorNames(Parse({}), "not enough arguments"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "--json"}), "not enough arguments"));
    // Before --header existed, the editor's headers were read like this and ignored
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "extra.ash"}), "'extra.ash'"));
}

TEST(CommandLine, DataOnlyBuildsNoSpritesOrAudio)
{
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "--data-only", "--import-sprites"}), "--import-sprites"));
    EXPECT_TRUE(ErrorNames(Parse({"Game.agf", "out", "--import-audio", "--data-only"}), "--import-audio"));
}

// The error of a bad command line is reported as JSON when --json is anywhere in it
TEST(CommandLine, JsonIsKnownWhateverTheError)
{
    EXPECT_TRUE(Parse({"--frobnicate", "--json"}).Json);
    EXPECT_TRUE(Parse({"--json"}).Json);
    const CommandLine cmd = Parse({"Game.agf", "--json", "out", "--header"});
    EXPECT_TRUE(cmd.Json);
    EXPECT_FALSE(cmd.Error.IsEmpty());
}

TEST(CommandLine, HelpWinsOverErrors)
{
    const CommandLine cmd = Parse({"--frobnicate", "--help"});
    EXPECT_TRUE(cmd.Help);
    EXPECT_TRUE(cmd.Error.IsEmpty());
    EXPECT_TRUE(Parse({"-h"}).Help);
}
