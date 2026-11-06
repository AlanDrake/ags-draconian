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
#include "gtest/gtest.h"
#include "util/cmdlineopts.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;

TEST(CmdLineOpts, ParserBasics)
{
    ParseResult parseResult = {};
    std::set<String> optParamsWithValues;

    optParamsWithValues = {};
    const char *argv_a[] = {"program", "-a", "b"};
    int argc_a = 3;

    parseResult = Parse(argc_a, argv_a, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("-a"));
    ASSERT_EQ(parseResult.PosArgs.size(), 1);
    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(), "b");
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());


    optParamsWithValues = {};
    const char *argv_b[] = {"program", "b", "-a"};
    int argc_b = 3;

    parseResult = Parse(argc_b, argv_b, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("-a"));
    ASSERT_EQ(parseResult.PosArgs.size(), 1);
    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(), "b");
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());


    optParamsWithValues = {};
    const char *argv_c[] = {"program", "a", "-b", "--long", "pos", "-longsingle", "--long-double"};
    int argc_c = 7;

    parseResult = Parse(argc_c, argv_c, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 4);
    ASSERT_TRUE(parseResult.Opt.count("-b"));
    ASSERT_TRUE(parseResult.Opt.count("--long"));
    ASSERT_TRUE(parseResult.Opt.count("-longsingle"));
    ASSERT_TRUE(parseResult.Opt.count("--long-double"));
    ASSERT_EQ(parseResult.PosArgs.size(), 2);
    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(), "a");
    ASSERT_STREQ(parseResult.PosArgs[1].GetCStr(), "pos");
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());
}


TEST(CmdLineOpts, ParserRaisedHelp)
{
    ParseResult parseResult = {};
    std::set<String> optParamsWithValues;

    optParamsWithValues = {};
    const char *argv_a[] = {"program", "-h", "b"};
    int argc_a = 3;

    parseResult = Parse(argc_a, argv_a, optParamsWithValues);

    ASSERT_TRUE(parseResult.HelpRequested);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("-h"));
    ASSERT_EQ(parseResult.PosArgs.size(), 1);
    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(), "b");
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());


    optParamsWithValues = {};
    const char *argv_b[] = {"program", "--help", "b"};
    int argc_b = 3;

    parseResult = Parse(argc_b, argv_b, optParamsWithValues);

    ASSERT_TRUE(parseResult.HelpRequested);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("--help"));
    ASSERT_EQ(parseResult.PosArgs.size(), 1);
    ASSERT_STREQ(parseResult.PosArgs[0].GetCStr(), "b");
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());
}


TEST(CmdLineOpts, ParserOptWithValues)
{
    ParseResult parseResult = {};
    std::set<String> optParamsWithValues;

    optParamsWithValues = {"-a"};
    const char *argv_a[] = {"program", "-a", "b"};
    int argc_a = 3;

    parseResult = Parse(argc_a, argv_a, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWithValue.size(), 1);
    ASSERT_STREQ(parseResult.OptWithValue[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWithValue[0].second.GetCStr(), "b");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());


    optParamsWithValues = {"-a"};
    const char *argv_b[] = {"program", "-ab"};
    int argc_b = 2;

    parseResult = Parse(argc_b, argv_b, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWithValue.size(), 1);
    ASSERT_STREQ(parseResult.OptWithValue[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWithValue[0].second.GetCStr(), "b");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());


    optParamsWithValues = {"-D"};
    const char *argv_c[] = {"program", "-Db", "-Dccc", "-Dddd", "-D", "eee"};
    int argc_c = 6;

    parseResult = Parse(argc_c, argv_c, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 0);
    ASSERT_EQ(parseResult.OptWithValue.size(), 4);
    ASSERT_STREQ(parseResult.OptWithValue[0].first.GetCStr(), "-D");
    ASSERT_STREQ(parseResult.OptWithValue[0].second.GetCStr(), "b");
    ASSERT_STREQ(parseResult.OptWithValue[1].first.GetCStr(), "-D");
    ASSERT_STREQ(parseResult.OptWithValue[1].second.GetCStr(), "ccc");
    ASSERT_STREQ(parseResult.OptWithValue[2].first.GetCStr(), "-D");
    ASSERT_STREQ(parseResult.OptWithValue[2].second.GetCStr(), "ddd");
    ASSERT_STREQ(parseResult.OptWithValue[3].first.GetCStr(), "-D");
    ASSERT_STREQ(parseResult.OptWithValue[3].second.GetCStr(), "eee");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());
}


TEST(CmdLineOpts, ParserOptWithValuesTrailingEmpty)
{
    ParseResult parseResult = {};
    std::set<String> optParamsWithValues;

    optParamsWithValues = {"-a", "-b", "-c"};
    const char *argv_a[] = {"program", "-a", "PARAMa", "-b", "PARAMb", "-c", "PARAMc", "-v"};
    int argc_a = 8;

    parseResult = Parse(argc_a, argv_a, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("-v"));
    ASSERT_EQ(parseResult.OptWithValue.size(), 3);
    ASSERT_STREQ(parseResult.OptWithValue[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWithValue[0].second.GetCStr(), "PARAMa");
    ASSERT_STREQ(parseResult.OptWithValue[1].first.GetCStr(), "-b");
    ASSERT_STREQ(parseResult.OptWithValue[1].second.GetCStr(), "PARAMb");
    ASSERT_STREQ(parseResult.OptWithValue[2].first.GetCStr(), "-c");
    ASSERT_STREQ(parseResult.OptWithValue[2].second.GetCStr(), "PARAMc");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
    ASSERT_TRUE(parseResult.OptWithMissingValue.IsEmpty());
}

TEST(CmdLineOpts, ParserOptWithValuesMissingLast)
{
    ParseResult parseResult = {};
    std::set<String> optParamsWithValues;

    optParamsWithValues = {"-a", "-b", "-c"};
    const char *argv_a[] = {"program", "-v", "-a", "PARAMa", "-b", "PARAMb", "-c"};
    int argc_a = 7;

    parseResult = Parse(argc_a, argv_a, optParamsWithValues);

    ASSERT_EQ(parseResult.HelpRequested, false);
    ASSERT_EQ(parseResult.Opt.size(), 1);
    ASSERT_TRUE(parseResult.Opt.count("-v"));
    ASSERT_EQ(parseResult.OptWithValue.size(), 2);
    ASSERT_STREQ(parseResult.OptWithValue[0].first.GetCStr(), "-a");
    ASSERT_STREQ(parseResult.OptWithValue[0].second.GetCStr(), "PARAMa");
    ASSERT_STREQ(parseResult.OptWithValue[1].first.GetCStr(), "-b");
    ASSERT_STREQ(parseResult.OptWithValue[1].second.GetCStr(), "PARAMb");
    ASSERT_STREQ(parseResult.OptWithMissingValue.GetCStr(), "-c");
    ASSERT_EQ(parseResult.PosArgs.size(), 0);
}

#if AGS_PLATFORM_OS_WINDOWS
// Where the tools take their arguments from on Windows (InitProgramArgs): the wide command line,
// split by the rules of the C runtime, into UTF-8
TEST(CmdLineOpts, SplitsWindowsCommandLineIntoUtf8)
{
    const std::vector<String> args =
        SplitWindowsCommandLine(L"C:\\tools\\po2tra.exe \"E:\\proj\u00E9 \u0436\u0434\\Parity.po\" out\\ --gamename "
                                L"\"Parit\u00E9 \\\"\u00F8\\\" \U0001F600\" \"\" \"a\\\\\"");
    ASSERT_EQ(args.size(), 7u);
    EXPECT_STREQ(args[0].GetCStr(), "C:\\tools\\po2tra.exe");
    EXPECT_STREQ(args[1].GetCStr(), "E:\\proj\xC3\xA9 \xD0\xB6\xD0\xB4\\Parity.po");
    EXPECT_STREQ(args[2].GetCStr(), "out\\");
    EXPECT_STREQ(args[3].GetCStr(), "--gamename");
    EXPECT_STREQ(args[4].GetCStr(), "Parit\xC3\xA9 \"\xC3\xB8\" \xF0\x9F\x98\x80");
    EXPECT_STREQ(args[5].GetCStr(), "");
    EXPECT_STREQ(args[6].GetCStr(), "a\\");
}
#endif
