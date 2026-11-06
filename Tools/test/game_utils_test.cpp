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
#include "data/game_utils.h"
#include "gtest/gtest.h"
#include "util/ini_util.h"
#include "util/string.h"

using AGS::Common::ConfigTree;
using AGS::Common::String;
namespace DataUtil = AGS::DataUtil;

static String ConfigValue(const DataUtil::RuntimeSetup &setup, const char *section, const char *key)
{
    ConfigTree cfg;
    DataUtil::WriteConfig(setup, nullptr, cfg);
    return cfg[section][key];
}

// The engine's names of the graphics drivers (GfxDriverFactory), as the editor writes them
TEST(GameUtils, ConfigGraphicsDriver)
{
    DataUtil::RuntimeSetup setup;
    setup.GraphicsDriver = "Software";
    EXPECT_STREQ("Software", ConfigValue(setup, "graphics", "driver").GetCStr());
    setup.GraphicsDriver = "D3D9";
    EXPECT_STREQ("D3D9", ConfigValue(setup, "graphics", "driver").GetCStr());
    setup.GraphicsDriver = "OpenGL";
    EXPECT_STREQ("OGL", ConfigValue(setup, "graphics", "driver").GetCStr());
    setup.GraphicsDriver = "Unknown";
    EXPECT_STREQ("", ConfigValue(setup, "graphics", "driver").GetCStr());
}

// The mouse speed keeps its fraction, as the editor's float.ToString() writes it
TEST(GameUtils, ConfigMouseSpeed)
{
    DataUtil::RuntimeSetup setup;
    setup.MouseSpeed = 1.f;
    EXPECT_STREQ("1", ConfigValue(setup, "mouse", "speed").GetCStr());
    setup.MouseSpeed = 1.5f;
    EXPECT_STREQ("1.5", ConfigValue(setup, "mouse", "speed").GetCStr());
    setup.MouseSpeed = 0.35f;
    EXPECT_STREQ("0.35", ConfigValue(setup, "mouse", "speed").GetCStr());
    setup.MouseSpeed = 10.f;
    EXPECT_STREQ("10", ConfigValue(setup, "mouse", "speed").GetCStr());
}

// show_fps is for the developer: only a debug build has it
TEST(GameUtils, ConfigShowFpsOnlyInDebugMode)
{
    DataUtil::RuntimeSetup setup;
    setup.ShowFPS = true;
    DataUtil::GameSettings settings{};
    settings.DebugMode = false;
    ConfigTree release_cfg;
    DataUtil::WriteConfig(setup, &settings, release_cfg);
    EXPECT_EQ(0u, release_cfg["misc"].count("show_fps"));
    settings.DebugMode = true;
    ConfigTree debug_cfg;
    DataUtil::WriteConfig(setup, &settings, debug_cfg);
    EXPECT_STREQ("1", debug_cfg["misc"]["show_fps"].GetCStr());
}
