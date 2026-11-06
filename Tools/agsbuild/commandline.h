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
// agsbuild's command line.
//
//=============================================================================
#pragma once

#include <vector>

#include "util/string.h"

namespace AGSBuild
{

// What the command line asks for, as it is written: paths are not made absolute here
struct CommandLine
{
    // -h, --help (also /? and -? on Windows): only print the help
    bool Help = false;
    // --json
    bool Json = false;
    // The two positional arguments
    AGS::Common::String ProjectFile;
    AGS::Common::String OutputDir;
    // --api-ash, -s/--sys-dir, -t/--temp-dir; empty if not given
    AGS::Common::String ApiHeader;
    AGS::Common::String SysDir;
    AGS::Common::String TempDir;
    // --header, in the order given
    std::vector<AGS::Common::String> Headers;
    // -p/--threads, 0 if not given
    int Threads = 0;
    bool DataOnly = false;
    bool Cleanup = false;
    bool ImportAudio = false;
    bool ImportSprites = false;
    bool BuildDebug = false;
    bool BuildRelease = false;
    bool Strict = false;
    bool Verbose = false;
    // Why the command line cannot be used, empty if it can
    AGS::Common::String Error;
};

// Reads agsbuild's arguments (those after the program's name). An option must be one that
// agsbuild knows, written in full; an option with a value takes the next argument, whatever it
// is ("-s dir", not "-sdir"); there must be exactly two positional arguments. The first problem
// found is told in Error. Json is set when "--json" is among the arguments, even if the command
// line has an error, so that the error can be reported as JSON.
CommandLine ParseCommandLine(const std::vector<AGS::Common::String> &args);

} // namespace AGSBuild
