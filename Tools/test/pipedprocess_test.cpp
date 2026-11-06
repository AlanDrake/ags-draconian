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
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "agsbuild/pipedprocess.h"
#include "gtest/gtest.h"
#include "util/cmdlineopts.h"
#include "util/file.h"
#include "util/memorystream.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGSBuild;

namespace
{

// Arguments which a command line must quote or escape: spaces, quotes, backslashes before a quote
// and at the end, the characters of a POSIX shell, and text outside ASCII (UTF-8)
const std::vector<String> &TrickyArgs()
{
    static const std::vector<String> args = {
        "plain",
        "",
        "with space",
        "Parit\xC3\xA9 \xD0\xB6\xD0\xB4 \"\xC3\xB8\"",
        "E:\\proj\xC3\xA9 \xD0\xB6\xD0\xB4\\",
        "E:\\dir\\",
        "back\\\\\"slash \\\\",
        "tab\there",
        "it's $HOME `id` *.ogg; a|b & c",
        "emoji \xF0\x9F\x98\x80",
        "%PATH% ^",
    };
    return args;
}

// A command that prints `lines` lines of 40+ characters each, so the output is much larger
// than the pipe buffer (1 KB)
void GetChattyCommand(int lines, String &exe, String &params)
{
#if AGS_PLATFORM_OS_WINDOWS
    exe = "cmd";
    params = String::FromFormat("/c for /L %%i in (1,1,%d) do @echo 0123456789abcdef0123456789abcdef line %%i", lines);
#else
    exe = "sh";
    params = String::FromFormat("-c 'i=0; while [ $i -lt %d ]; do echo 0123456789abcdef0123456789abcdef line $i; "
                                "i=$((i+1)); done'",
                                lines);
#endif
}

size_t RunChatty(int lines)
{
    String exe, params;
    GetChattyCommand(lines, exe, params);
    std::vector<uint8_t> log_buf;
    TextStreamWriter log(std::make_unique<Stream>(std::make_unique<VectorStream>(log_buf, kStream_Write)));
    std::vector<uint8_t> out;
    PipedProcess proc(exe, params, &log);
    HError err = proc.RunSync(&out);
    return err ? out.size() : 0;
}

} // namespace

// Several processes with large output, started from parallel threads as agsbuild does. This used
// to hang on Windows: a read asked for more bytes than the pipe had, and blocked forever because
// agsbuild still held the pipe's write end (docs/refactor/PLAN.md, 7.1b).
TEST(PipedProcess, ParallelLargeOutputCompletes)
{
    const int line_count = 2000, process_count = 6;
    std::vector<std::future<size_t>> runs;
    for (int i = 0; i < process_count; ++i)
        runs.push_back(std::async(std::launch::async, RunChatty, line_count));

    for (auto &run : runs)
    {
        if (run.wait_for(std::chrono::seconds(120)) != std::future_status::ready)
        {
            // A blocked thread can't be cancelled, so stop here instead of hanging the test run
            std::fprintf(stderr, "PipedProcess.ParallelLargeOutputCompletes: a process run did not finish\n");
            std::abort();
        }
        EXPECT_GE(run.get(), static_cast<size_t>(line_count) * 40u) << "process output was cut short";
    }
}

// Quoted only where needed; backslashes are doubled only before a quote, the closing one included
TEST(PipedProcess, QuotesWindowsArg)
{
    EXPECT_STREQ(QuoteWindowsArg("plain-1.0/x.y").GetCStr(), "plain-1.0/x.y");
    EXPECT_STREQ(QuoteWindowsArg("E:\\dir\\").GetCStr(), "E:\\dir\\");
    EXPECT_STREQ(QuoteWindowsArg("").GetCStr(), "\"\"");
    EXPECT_STREQ(QuoteWindowsArg("my dir\\").GetCStr(), "\"my dir\\\\\"");
    EXPECT_STREQ(QuoteWindowsArg("say \"hi\"").GetCStr(), "\"say \\\"hi\\\"\"");
    EXPECT_STREQ(QuoteWindowsArg("a\\\\\"b").GetCStr(), "\"a\\\\\\\\\\\"b\"");
    EXPECT_STREQ(QuoteWindowsArg("a\\\\b c").GetCStr(), "\"a\\\\b c\"");
}

// In single quotes where needed, a single quote closes them, is escaped and opens them again
TEST(PipedProcess, QuotesPosixShellArg)
{
    EXPECT_STREQ(QuotePosixShellArg("plain-1.0/x.y").GetCStr(), "plain-1.0/x.y");
    EXPECT_STREQ(QuotePosixShellArg("").GetCStr(), "''");
    EXPECT_STREQ(QuotePosixShellArg("$HOME `id` \"x\" \\").GetCStr(), "'$HOME `id` \"x\" \\'");
    EXPECT_STREQ(QuotePosixShellArg("it's").GetCStr(), "'it'\\''s'");
}

#if AGS_PLATFORM_OS_WINDOWS
// The command line agsbuild makes, read back as the tools read it (CmdLineOpts::InitProgramArgs)
TEST(PipedProcess, WindowsArgsReadBackAsGiven)
{
    ProcessArgs args;
    for (const auto &arg : TrickyArgs())
    {
        args.Add(arg);
    }
    std::vector<wchar_t> command_line;
    StrUtil::ConvertUtf8ToWstr(String::FromFormat("tool.exe %s", args.GetParams().GetCStr()).GetCStr(), command_line);

    const std::vector<String> read_args = CmdLineOpts::SplitWindowsCommandLine(command_line.data());
    ASSERT_EQ(read_args.size(), TrickyArgs().size() + 1);
    for (size_t i = 0; i < TrickyArgs().size(); ++i)
    {
        EXPECT_STREQ(read_args[i + 1].GetCStr(), TrickyArgs()[i].GetCStr());
    }
}
#endif

#if AGS_PLATFORM_OS_WINDOWS
// A tool inherits its own pipes only, not the other inheritable handles of agsbuild: when the
// editor runs agsbuild, those are the editor's pipes, which a tool would keep open while it runs.
// The child, this test program with "--print-handle-path" (gtest_main.cc), tells which file the
// value of an inheritable handle of the test is open to in the child: none.
TEST(PipedProcess, ChildInheritsOnlyItsPipes)
{
    const char *file_name = "inherit_probe.txt";
    SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
    HANDLE file = CreateFileW(L"inherit_probe.txt", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    ASSERT_NE(INVALID_HANDLE_VALUE, file);

    const String handle_value =
        String::FromFormat("%llx", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(file)));
    std::vector<uint8_t> log_buf;
    TextStreamWriter log(std::make_unique<Stream>(std::make_unique<VectorStream>(log_buf, kStream_Write)));
    std::vector<uint8_t> out;
    PipedProcess proc(File::GetThisExePath(), ProcessArgs{"--print-handle-path", handle_value}.GetParams(), &log);
    const bool ran = static_cast<bool>(proc.RunSync(&out));
    CloseHandle(file);
    File::DeleteFile(file_name);

    ASSERT_TRUE(ran);
    const String text(reinterpret_cast<const char *>(out.data()), out.size());
    EXPECT_TRUE(text.StartsWith("path:none")) << "the child has the test's handle: " << text.GetCStr();
}
#endif

// A child process gets the arguments as they were given, by the way the tools read them: this
// test program, with "--print-args" (gtest_main.cc), started as agsbuild starts the tools
TEST(PipedProcess, ChildGetsArgsAsGiven)
{
    const String exe = File::GetThisExePath();
    ASSERT_FALSE(exe.IsEmpty());
    ProcessArgs args{"--print-args"};
    for (const auto &arg : TrickyArgs())
    {
        args.Add(arg);
    }
    std::vector<uint8_t> log_buf;
    TextStreamWriter log(std::make_unique<Stream>(std::make_unique<VectorStream>(log_buf, kStream_Write)));
    std::vector<uint8_t> out;
    PipedProcess proc(exe, args.GetParams(), &log);
    ASSERT_TRUE(proc.RunSync(&out));
    EXPECT_EQ(proc.GetExitCode(), 0);

    // Lines of "arg:<hex>"
    std::vector<String> read_args;
    const String text(reinterpret_cast<const char *>(out.data()), out.size());
    for (String line : text.Split('\n'))
    {
        line.TrimRight('\r');
        if (!line.StartsWith("arg:"))
        {
            continue;
        }
        std::string arg;
        for (size_t i = 4; i + 1 < line.GetLength(); i += 2)
        {
            arg += static_cast<char>(strtol(line.Mid(i, 2).GetCStr(), nullptr, 16));
        }
        read_args.emplace_back(arg.c_str());
    }
    ASSERT_EQ(read_args.size(), TrickyArgs().size()) << text.GetCStr();
    for (size_t i = 0; i < TrickyArgs().size(); ++i)
    {
        EXPECT_STREQ(read_args[i].GetCStr(), TrickyArgs()[i].GetCStr());
    }
}
