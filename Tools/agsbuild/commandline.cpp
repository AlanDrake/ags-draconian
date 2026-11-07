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
#include "commandline.h"

#include <algorithm>
#include <iterator>

#include "platform/platform.h"
#include "util/string_utils.h"

using AGS::Common::String;
namespace StrUtil = AGS::Common::StrUtil;

namespace AGSBuild
{

namespace
{

enum class Option
{
    ApiHeader,
    SysDir,
    TempDir,
    Threads,
    Header,
    Json,
    DataOnly,
    Cleanup,
    Rebuild,
    ImportAudio,
    ImportSprites,
    BuildDebug,
    BuildRelease,
    Strict,
    Verbose
};

struct OptionSpec
{
    const char *Name;
    Option Id;
    bool HasValue;
};

// Every option agsbuild knows; CmdLineOpts would take anything else, or read "-sX" as "-s X"
const OptionSpec kKnownOptions[] = {
    {"--api-ash", Option::ApiHeader, true},
    {"-s", Option::SysDir, true},
    {"--sys-dir", Option::SysDir, true},
    {"-t", Option::TempDir, true},
    {"--temp-dir", Option::TempDir, true},
    {"-p", Option::Threads, true},
    {"--threads", Option::Threads, true},
    {"--header", Option::Header, true},
    {"--json", Option::Json, false},
    {"--data-only", Option::DataOnly, false},
    {"-c", Option::Cleanup, false},
    {"--cleanup", Option::Cleanup, false},
    {"--rebuild", Option::Rebuild, false},
    {"--import-audio", Option::ImportAudio, false},
    {"--import-sprites", Option::ImportSprites, false},
    {"--build-debug", Option::BuildDebug, false},
    {"--build-release", Option::BuildRelease, false},
    {"--strict", Option::Strict, false},
    {"-v", Option::Verbose, false},
    {"--verbose", Option::Verbose, false},
};

bool IsHelpOption(const String &arg)
{
    return arg == "-h" || arg == "--help"
#if AGS_PLATFORM_OS_WINDOWS
           || arg == "/?" || arg == "-?"
#endif
        ;
}

const OptionSpec *FindOption(const String &arg)
{
    const auto it = std::find_if(std::begin(kKnownOptions), std::end(kKnownOptions),
                                 [&arg](const OptionSpec &spec) { return arg == spec.Name; });
    return it != std::end(kKnownOptions) ? &*it : nullptr;
}

void SetFlag(CommandLine &cmd, Option id)
{
    switch (id)
    {
    case Option::Json: cmd.Json = true; break;
    case Option::DataOnly: cmd.DataOnly = true; break;
    case Option::Cleanup: cmd.Cleanup = true; break;
    case Option::Rebuild: cmd.Rebuild = true; break;
    case Option::ImportAudio: cmd.ImportAudio = true; break;
    case Option::ImportSprites: cmd.ImportSprites = true; break;
    case Option::BuildDebug: cmd.BuildDebug = true; break;
    case Option::BuildRelease: cmd.BuildRelease = true; break;
    case Option::Strict: cmd.Strict = true; break;
    case Option::Verbose: cmd.Verbose = true; break;
    default: break;
    }
}

// Returns the error, or an empty string
String SetValue(CommandLine &cmd, const OptionSpec &spec, const String &value)
{
    if (value.IsEmpty())
    {
        return String::FromFormat("option '%s' needs a value", spec.Name);
    }
    switch (spec.Id)
    {
    case Option::ApiHeader: cmd.ApiHeader = value; break;
    case Option::SysDir: cmd.SysDir = value; break;
    case Option::TempDir: cmd.TempDir = value; break;
    case Option::Header: cmd.Headers.push_back(value); break;
    case Option::Threads:
        if (StrUtil::StringToInt(value, cmd.Threads, 0) != StrUtil::kNoError || cmd.Threads < 1)
        {
            return String::FromFormat("option '%s' takes a number of 1 or more, not '%s'", spec.Name, value.GetCStr());
        }
        break;
    default: break;
    }
    return String();
}

// The options that do not go together; returns the error, or an empty string
String CheckCombinations(const CommandLine &cmd)
{
    if (cmd.DataOnly && cmd.ImportSprites)
    {
        return "--import-sprites cannot be used with --data-only, which builds no sprites";
    }
    if (cmd.DataOnly && cmd.ImportAudio)
    {
        return "--import-audio cannot be used with --data-only, which builds no audio";
    }
    return String();
}

} // namespace

CommandLine ParseCommandLine(const std::vector<String> &args)
{
    CommandLine cmd;
    cmd.Help = std::any_of(args.begin(), args.end(), IsHelpOption);
    cmd.Json = std::find(args.begin(), args.end(), String("--json")) != args.end();
    if (cmd.Help)
    {
        return cmd;
    }

    std::vector<String> positional;
    for (size_t i = 0; i < args.size(); ++i)
    {
        const String &arg = args[i];
        if (arg.IsEmpty() || arg[0u] != '-')
        {
            positional.push_back(arg);
            continue;
        }
        const OptionSpec *spec = FindOption(arg);
        if (!spec)
        {
            cmd.Error = String::FromFormat("unknown option '%s'", arg.GetCStr());
            return cmd;
        }
        if (!spec->HasValue)
        {
            SetFlag(cmd, spec->Id);
            continue;
        }
        if (i + 1 >= args.size())
        {
            cmd.Error = String::FromFormat("option '%s' needs a value", spec->Name);
            return cmd;
        }
        cmd.Error = SetValue(cmd, *spec, args[++i]);
        if (!cmd.Error.IsEmpty())
        {
            return cmd;
        }
    }

    if (positional.size() < 2)
    {
        cmd.Error = "not enough arguments: the project file and the output folder are needed";
        return cmd;
    }
    if (positional.size() > 2)
    {
        cmd.Error = String::FromFormat("unexpected argument '%s'", positional[2].GetCStr());
        return cmd;
    }
    cmd.ProjectFile = positional[0];
    cmd.OutputDir = positional[1];
    cmd.Error = CheckCombinations(cmd);
    return cmd;
}

} // namespace AGSBuild
