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
#include <stdio.h>

#include "gtest/gtest.h"
#include "test/spriteimport_env.h"
#include "util/cmdlineopts.h"

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

GTEST_API_ int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--print-args") == 0)
    {
        return PrintArgs(argc, argv);
    }
    printf("Running main() from %s\n", __FILE__);
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new SpriteImportEnv());
    return RUN_ALL_TESTS();
}
