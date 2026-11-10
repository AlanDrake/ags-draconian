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
#include <iterator>

#include "ac/gamestructdefines.h"
#include "data/data_helpers.h"
#include "gtest/gtest.h"

using AGS::Common::kScriptAPISourceNames;

// kScriptAPISourceNames is the one list the script compiler and the build tools use to understand
// the API levels written in game projects. It used to be copied into each tool, and the copies
// stopped at v363, so AGS 4 projects could not be compiled or built (docs/refactor/PLAN.md, 7.1b).
TEST(ScriptAPINames, EndsAtCurrentLevel)
{
    ASSERT_GT(std::size(kScriptAPISourceNames), 0u);
    EXPECT_EQ(kScriptAPI_Current, std::end(kScriptAPISourceNames)[-1].Version)
        << "A new kScriptAPI_* level was added without a name in kScriptAPISourceNames";
}

TEST(ScriptAPINames, AscendingAndUnique)
{
    for (size_t i = 1; i < std::size(kScriptAPISourceNames); ++i)
    {
        EXPECT_LT(kScriptAPISourceNames[i - 1].Version, kScriptAPISourceNames[i].Version)
            << kScriptAPISourceNames[i].Name;
        for (size_t j = 0; j < i; ++j)
            EXPECT_STRNE(kScriptAPISourceNames[i].Name, kScriptAPISourceNames[j].Name);
    }
}

TEST(ScriptAPINames, NamesMatchTheEditor)
{
    // The editor writes these exact names (enum AGS.Types.ScriptAPIVersion)
    EXPECT_STREQ("v321", kScriptAPISourceNames[0].Name);
    EXPECT_STREQ("v399", kScriptAPISourceNames[14].Name);
    EXPECT_EQ(kScriptAPI_v399, kScriptAPISourceNames[14].Version);
    EXPECT_STREQ("v400_33", std::end(kScriptAPISourceNames)[-1].Name);
}
