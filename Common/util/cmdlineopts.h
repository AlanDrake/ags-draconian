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
//  Command Line Input Parser
//
//=============================================================================

#pragma once

#include <set>
#include <vector>

#include "platform/platform.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

namespace CmdLineOpts
{

struct ParseResult
{
    bool HelpRequested = false;
    std::vector<std::pair<String, String>> OptWithValue{};
    std::vector<String> PosArgs;
    std::multiset<String> Opt;
    String OptWithMissingValue{};
};

// opt_params_with_values are the options that always take an argument.
// example: `-l Val` or `--long Val` are passed with {"-l","--long"}.
// args are the arguments after the program's name.
ParseResult Parse(const std::vector<String> &args, const std::set<String> &opt_params_with_values);
// The same for the arguments as main() has them: argv[0] is the program's name.
ParseResult Parse(int argc, const char *const argv[], const std::set<String> &opt_params_with_values);

// Sets a command-line program up to work in UTF-8, as the rest of the code does, and returns its
// arguments after the program's name as UTF-8 strings, for Parse. To be called first in main(),
// with main's own arguments.
// On Windows main() gets the arguments converted to the ANSI code page, which loses every
// character outside of it: there they are taken from the process's wide command line, and the
// path functions are told to read the current directory as Unicode too. On other platforms the
// arguments are returned as they are.
std::vector<String> InitProgramArgs(int argc, const char *const argv[]);

#if AGS_PLATFORM_OS_WINDOWS
// Splits a Windows command line into its arguments by the rules of CommandLineToArgvW, the
// program's name included, as UTF-8 strings.
std::vector<String> SplitWindowsCommandLine(const wchar_t *command_line);
#endif

} // namespace CmdLineOpts

} // namespace Common
} // namespace AGS
