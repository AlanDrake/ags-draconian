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
#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "platform/base/engine_config.h"
#include "util/string.h"
#include "util/version.h"

class AGSPlatformDriver;
namespace AGS
{
namespace Common
{
class AssetManager;
}
namespace Engine
{
class IGraphicsDriver;
}
} // namespace AGS

// Defines a package file location
struct PackLocation
{
    AGS::Common::String Name; // filename, for the reference or to use as an ID
    AGS::Common::String Path; // full path
};

// Game resource paths
// TODO: the asset path configuration should certainly be revamped at some
// point, with uniform method of configuring auxiliary paths and packages.
struct ResourcePaths
{
    PackLocation GamePak;        // main game package
    PackLocation AudioPak;       // audio package
    PackLocation SpeechPak;      // voice-over package
    AGS::Common::String DataDir; // path to the data directory
    bool VoiceAvail = false;     // tells whether voice files available in either location
    // Optional directories are represented by pairs of "path" and "filter".
    // Currently are only set for compatibility with Editor (game test runs from IDE),
    // see "--runfromide" command line arg.
    std::vector<std::pair<AGS::Common::String, AGS::Common::String>> OptDataDirs; // optional data directories
    AGS::Common::String VoiceDirSub; // full voice-over directory with optional sub-dir
};

// What lives as long as the process: the engine's configuration and the services of the host (window and
// graphics, platform, game files). The game logic reaches them through EngineServices::Current(); a function
// that uses them more than once takes a reference at its top:
//     EngineServices &services = EngineServices::Current();
// The members describing the loaded game's files (ResPaths, Assets) are set again when another game is run.
struct EngineServices
{
    EngineServices();
    ~EngineServices(); // out of line: AssetManager is incomplete here
    EngineServices(const EngineServices &) = delete;
    EngineServices &operator=(const EngineServices &) = delete;

    // The instance in use
    static EngineServices &Current() { return *_current; }
    // Makes another instance the current one, for a test, and returns the one that was;
    // nullptr brings the engine's own instance back
    static EngineServices *SetCurrent(EngineServices *services);

    AGS::Common::Version Version; // the engine's version
    // The configuration: defaults, the config files, the command line.
    // TODO: it is used for two purposes: storing the config options before the engine is initialized,
    // and storing certain runtime variables. Perhaps the two groups should be separated at some point.
    EngineConfig Config;
    ResourcePaths ResPaths; // where the game's packages and directories are

    // The OS-specific driver; not owned.
    // TODO: AGSPlatformDriver::GetDriver() returns the same instance: one of the two ways could go.
    AGSPlatformDriver *Platform = nullptr;
    // The graphics driver in use, nullptr before the graphics mode is set; owned by its factory
    AGS::Engine::IGraphicsDriver *GfxDriver = nullptr;
    std::unique_ptr<AGS::Common::AssetManager> Assets; // the game's files

    // The state of the game loop that the host changes and reads:
    // above 0 while display_switch_out_suspend is pausing the game after it lost the input focus
    volatile int SwitchingAwayFromGame = 0;
    // whether the game does not have the input focus now
    volatile bool SwitchedAway = false;
    // whether the game is suspended after losing the input focus; the frame wait loops spin while it is set
    volatile bool GameUpdateSuspend = false;
    // requests to stop: set together when the window is closed or the debugger asks to exit
    volatile bool WantExit = false;
    volatile bool AbortEngine = false;
    // set to 1 when the engine exits through its own quit procedure; atexit_handler reports an exit without it
    int ProperExit = 0;

    // Log the script RTTI and the script table of contents when the scripts are loaded (command line)
    bool LogScriptRTTI = false;
    bool LogScriptTOC = false;

private:
    static EngineServices *_current;
};
