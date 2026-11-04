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
// Contract C3 (docs/refactor/PLAN.md): the names that the engine exports to game scripts and plugins.
// Compiled scripts link against these by name, so dropping or renaming one breaks existing games.
// The test registers the whole API the way game start-up does, writes one line per symbol and
// compares the result with the checked-in snapshot Engine/test/data/script_exports.txt.
//
// To change the API on purpose, regenerate the snapshot and review the diff:
//   AGS_UPDATE_SNAPSHOTS=1 engine_full_test --gtest_filter='ScriptExports.*'
//
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "ac/gamestructdefines.h"
#include "gtest/gtest.h"
#include "script/exports.h"
#include "script/script_runtime.h"
#include "script/systemimports.h"

namespace
{

const char *KindName(const ScriptImport &imp)
{
    if (imp.ValueHint == kScValHint_Handle)
        return "handle";
    switch (imp.Value.Type)
    {
    case kScValStaticFunction: return "function";
    case kScValObjectFunction: return "method";
    case kScValPluginFunction: return "plugin-function";
    case kScValStaticArray: return "static-array";
    case kScValScriptObject: return "object";
    case kScValPluginObject: return "plugin-object";
    default: return "other";
    }
}

// One line per exported symbol: "<name> <kind> <plugin>", sorted by name. The name carries the
// argument count ("Func^2"), which is what the script compiler links against. "plugin" says
// whether plugins can also look the symbol up through IAGSEngine::GetScriptFunctionAddress.
std::string DumpExports()
{
    simp.Clear();
    simp_for_plugin.Clear();
    setup_script_exports(kScriptAPI_Current, kScriptAPI_Current);

    std::vector<std::string> lines;
    for (uint32_t i = 0;; ++i)
    {
        const ScriptImport *imp = simp.GetByIndex(i);
        if (!imp)
            break;
        if (imp->Name.IsEmpty())
            continue;
        const bool for_plugin = simp_for_plugin.GetByName(imp->Name) != nullptr;
        lines.push_back(std::string(imp->Name.GetCStr()) + " " + KindName(*imp) + (for_plugin ? " plugin" : " -"));
    }
    std::sort(lines.begin(), lines.end());

    std::string out;
    for (const std::string &line : lines)
        out += line + "\n";
    return out;
}

std::string ReadFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    // The snapshot may have been checked out with CRLF line endings
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

std::vector<std::string> SplitLines(const std::string &text)
{
    std::vector<std::string> lines;
    std::istringstream ss(text);
    for (std::string line; std::getline(ss, line);)
        lines.push_back(line);
    return lines;
}

} // namespace

TEST(ScriptExports, MatchSnapshot)
{
    const std::string path = SCRIPT_EXPORTS_SNAPSHOT;
    const std::string actual = DumpExports();
    ASSERT_FALSE(actual.empty());

    if (std::getenv("AGS_UPDATE_SNAPSHOTS"))
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.good()) << "cannot write " << path;
        out << actual;
        return;
    }

    const std::string expected = ReadFile(path);
    ASSERT_FALSE(expected.empty()) << "missing snapshot " << path;
    if (actual == expected)
        return;

    // Report the symbols that differ rather than the two whole tables
    std::vector<std::string> exp_lines = SplitLines(expected);
    std::vector<std::string> act_lines = SplitLines(actual);
    std::sort(exp_lines.begin(), exp_lines.end());
    std::vector<std::string> removed;
    std::vector<std::string> added;
    std::set_difference(exp_lines.begin(), exp_lines.end(), act_lines.begin(), act_lines.end(),
                        std::back_inserter(removed));
    std::set_difference(act_lines.begin(), act_lines.end(), exp_lines.begin(), exp_lines.end(),
                        std::back_inserter(added));
    std::string report = "The script API differs from " + path + ".\n";
    report += "If this is deliberate, regenerate it with AGS_UPDATE_SNAPSHOTS=1.\n";
    for (const std::string &line : removed)
        report += "  - " + line + "\n";
    for (const std::string &line : added)
        report += "  + " + line + "\n";
    ADD_FAILURE() << report;
}

// Every name the AGS 4.0.0.30 release exported must still resolve, by the same lookup the engine
// uses when it links a compiled script (an exact match or a simpler variant of the name). The
// release registered some names without their "^N" argument count; a compiled script imports
// those as "name^N", which the fork resolves if it has any overload of the name.
TEST(ScriptExports, ReleaseExportsResolve)
{
    simp.Clear();
    simp_for_plugin.Clear();
    setup_script_exports(kScriptAPI_v400_24, kScriptAPI_v400_24);

    const std::string path = RELEASE_EXPORTS_4_0_0_30;
    std::vector<std::string> names;
    for (const std::string &line : SplitLines(ReadFile(path)))
        if (!line.empty() && line[0] != '#')
            names.push_back(line);
    ASSERT_GT(names.size(), 1000u) << "missing list " << path;

    std::set<std::string> fork_names; // without the "^N"
    for (uint32_t i = 0; const ScriptImport *imp = simp.GetByIndex(i); ++i)
    {
        const std::string full = imp->Name.GetCStr();
        fork_names.insert(full.substr(0, full.find('^')));
    }

    std::string missing;
    for (const std::string &name : names)
    {
        const bool resolves = (name.find('^') == std::string::npos) ? fork_names.count(name) > 0
                                                                    : simp.GetIndexOfAny(name.c_str()) != UINT32_MAX;
        if (!resolves)
            missing += "  " + name + "\n";
    }
    EXPECT_TRUE(missing.empty()) << "Exports of the 4.0.0.30 release that compiled games can no longer link:\n"
                                 << missing;
}
