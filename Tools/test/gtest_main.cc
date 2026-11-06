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
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <vector>

#include "platform/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif
#include "gtest/gtest.h"
#include "test/spriteimport_env.h"
#include "util/cmdlineopts.h"
#include "util/string_utils.h"

// The child process of the PipedProcess tests (pipedprocess_test.cpp): prints the arguments after
// "--print-args" as the tools get them (CmdLineOpts::InitProgramArgs), one per line, in hex
static int PrintArgs(int argc, char **argv)
{
    const auto args = AGS::Common::CmdLineOpts::InitProgramArgs(argc, argv);
    for (size_t i = 1; i < args.size(); ++i)
    {
        printf("arg:");
        for (const char *ch = args[i].GetCStr(); *ch; ++ch)
        {
            printf("%02x", static_cast<unsigned char>(*ch));
        }
        printf("\n");
    }
    return 0;
}

#if AGS_PLATFORM_OS_WINDOWS
// The child process of PipedProcess.ChildInheritsOnlyItsPipes: prints the path of the file which
// the handle of this value is open to in this process, or "none"
static int PrintHandlePath(const char *handle_value)
{
    const auto handle = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(strtoull(handle_value, nullptr, 16)));
    std::vector<wchar_t> path(4096);
    const DWORD len = GetFinalPathNameByHandleW(handle, path.data(), static_cast<DWORD>(path.size()), 0);
    std::vector<char> utf8;
    if (len > 0 && len < path.size())
    {
        AGS::Common::StrUtil::ConvertWstrToUtf8(path.data(), utf8);
    }
    printf("path:%s\n", utf8.empty() ? "none" : utf8.data());
    return 0;
}
#endif

GTEST_API_ int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--print-args") == 0)
    {
        return PrintArgs(argc, argv);
    }
#if AGS_PLATFORM_OS_WINDOWS
    if (argc > 2 && strcmp(argv[1], "--print-handle-path") == 0)
    {
        return PrintHandlePath(argv[2]);
    }
#endif
    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new SpriteImportEnv());
    return RUN_ALL_TESTS();
}
