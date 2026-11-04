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
// Compatibility contract C4 (docs/refactor/PLAN.md): the binary plugin API. Binary plugins call
// IAGSEngine through its vtable and read the structs in agsplugin.h by layout, so the order and
// the signatures of the declarations are the ABI. The test compares the header's declarations,
// in order and without comments, and the AGS_* functions the engine looks up in a plugin with
// test/data/plugin_api.txt. Regenerate it with AGS_UPDATE_SNAPSHOTS=1 after a deliberate change
// (appending to the end of IAGSEngine is the compatible kind).
//
//=============================================================================
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace
{

std::string ReadFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

std::string StripComments(const std::string &src)
{
    std::string out;
    for (size_t i = 0; i < src.size();)
    {
        if (src.compare(i, 2, "//") == 0)
        {
            i = src.find('\n', i);
            if (i == std::string::npos)
                break;
        }
        else if (src.compare(i, 2, "/*") == 0)
        {
            const size_t end = src.find("*/", i + 2);
            i = (end == std::string::npos) ? src.size() : end + 2;
            out += ' ';
        }
        else if (src[i] == '"')
        {
            const size_t end = src.find('"', i + 1);
            const size_t stop = (end == std::string::npos) ? src.size() : end + 1;
            out.append(src, i, stop - i);
            i = stop;
        }
        else
        {
            out += src[i++];
        }
    }
    return out;
}

void AddStatement(std::vector<std::string> &out, const std::string &text)
{
    std::string s;
    for (char c : text)
    {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!s.empty() && s.back() != ' ')
                s += ' ';
        }
        else
        {
            s += c;
        }
    }
    while (!s.empty() && s.back() == ' ')
        s.pop_back();
    if (!s.empty())
        out.push_back(s);
}

// One declaration per line: preprocessor lines as they are, the rest split after ; { }
std::vector<std::string> HeaderDeclarations(const std::string &header)
{
    std::vector<std::string> out;
    std::istringstream lines(StripComments(header));
    std::string pending;
    for (std::string line; std::getline(lines, line);)
    {
        const size_t first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line[first] == '#')
        {
            AddStatement(out, pending);
            pending.clear();
            AddStatement(out, line);
            continue;
        }
        for (char c : line)
        {
            pending += c;
            if (c == ';' || c == '{' || c == '}')
            {
                AddStatement(out, pending);
                pending.clear();
            }
        }
        pending += ' ';
    }
    AddStatement(out, pending);
    return out;
}

std::string DumpPluginApi()
{
    std::string out;
    for (const std::string &decl : HeaderDeclarations(ReadFile(PLUGIN_API_HEADER)))
        out += decl + "\n";

    // The plugin functions the engine resolves by name
    const std::string source = ReadFile(PLUGIN_ENGINE_SOURCE);
    const std::regex lookup("GetFunctionAddress\\(\"(AGS_\\w+)\"\\)");
    std::set<std::string> entries;
    for (std::sregex_iterator it(source.begin(), source.end(), lookup), end; it != end; ++it)
        entries.insert((*it)[1].str());
    for (const std::string &name : entries)
        out += "entry " + name + "\n";
    return out;
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

TEST(PluginApi, MatchSnapshot)
{
    const std::string path = PLUGIN_API_SNAPSHOT;
    const std::string actual = DumpPluginApi();
    ASSERT_NE(actual.find("class IAGSEngine {"), std::string::npos) << "agsplugin.h was not read";
    ASSERT_NE(actual.find("entry AGS_EngineStartup"), std::string::npos) << "agsplugin.cpp was not read";

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

    // Report the first declaration that differs: everything after it moved
    const std::vector<std::string> exp_lines = SplitLines(expected);
    const std::vector<std::string> act_lines = SplitLines(actual);
    size_t i = 0;
    while (i < exp_lines.size() && i < act_lines.size() && exp_lines[i] == act_lines[i])
        ++i;
    std::string report = "The plugin API differs from " + path + " at declaration " + std::to_string(i + 1) + ".\n";
    report += "If this is deliberate and compatible, regenerate it with AGS_UPDATE_SNAPSHOTS=1.\n";
    report += "  expected: " + (i < exp_lines.size() ? exp_lines[i] : std::string("(end)")) + "\n";
    report += "  actual:   " + (i < act_lines.size() ? act_lines[i] : std::string("(end)")) + "\n";
    ADD_FAILURE() << report;
}
