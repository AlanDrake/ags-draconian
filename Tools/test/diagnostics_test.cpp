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
#include <string>
#include <vector>

#include "agsbuild/diagnostics.h"
#include "gtest/gtest.h"

using namespace AGSBuild::Diag;

TEST(Diagnostics, CompilerError)
{
    Diagnostic d;
    ASSERT_TRUE(ParseDiagnosticLine("Error: compile failed at room1.asc, line 12 : Undefined symbol 'x'", d));
    EXPECT_EQ(Severity::Error, d.Level);
    EXPECT_EQ("room1.asc", d.File);
    EXPECT_EQ(12, d.Line);
    EXPECT_EQ("Undefined symbol 'x'", d.Message);
}

TEST(Diagnostics, PreprocessorErrorAndWarning)
{
    Diagnostic d;
    ASSERT_TRUE(ParseDiagnosticLine("Error: preprocessor failed at GlobalScript.asc, line 3 : bad macro", d));
    EXPECT_EQ(Severity::Error, d.Level);
    EXPECT_EQ("GlobalScript.asc", d.File);
    EXPECT_EQ(3, d.Line);
    EXPECT_EQ("bad macro", d.Message);

    // Room scripts are named the way the Windows editor does
    ASSERT_TRUE(ParseDiagnosticLine("Warning: issue at Rooms\\1\\room1.asc, line 5 : unused", d));
    EXPECT_EQ(Severity::Warning, d.Level);
    EXPECT_EQ("Rooms\\1\\room1.asc", d.File);
    EXPECT_EQ(5, d.Line);
    EXPECT_EQ("unused", d.Message);

    ASSERT_TRUE(ParseDiagnosticLine("Info: at a.asc, line 1 : note", d));
    EXPECT_EQ(Severity::Info, d.Level);
    EXPECT_EQ("a.asc", d.File);
    EXPECT_EQ(1, d.Line);

    // A dialog is named as the compiler names its script, as agf2dlgasc reports it
    ASSERT_TRUE(ParseDiagnosticLine("Error: at Dialog 3, line 12 : Unknown character: alice", d));
    EXPECT_EQ("Dialog 3", d.File);
    EXPECT_EQ(12, d.Line);
    EXPECT_EQ("Unknown character: alice", d.Message);
}

TEST(Diagnostics, MessagesWithoutLocation)
{
    Diagnostic d;
    ASSERT_TRUE(ParseDiagnosticLine("Error: failed to open file for reading: Game.agf", d));
    EXPECT_EQ(Severity::Error, d.Level);
    EXPECT_EQ("", d.File);
    EXPECT_EQ(0, d.Line);
    EXPECT_EQ("failed to open file for reading: Game.agf", d.Message);

    ASSERT_TRUE(ParseDiagnosticLine("warning: variable 'a' is never used\r", d));
    EXPECT_EQ(Severity::Warning, d.Level);
    EXPECT_EQ("variable 'a' is never used", d.Message);

    // " at " without a line number is a part of the message
    ASSERT_TRUE(ParseDiagnosticLine("Error: failed to write at the end, line x : y", d));
    EXPECT_EQ("", d.File);
    EXPECT_EQ(0, d.Line);
    EXPECT_EQ("failed to write at the end, line x : y", d.Message);
}

TEST(Diagnostics, OtherLinesAreNotDiagnostics)
{
    Diagnostic d;
    EXPECT_FALSE(ParseDiagnosticLine("", d));
    EXPECT_FALSE(ParseDiagnosticLine("Running: agscc a.asc", d));
    EXPECT_FALSE(ParseDiagnosticLine("Task: agscc:a.asc", d));
    EXPECT_FALSE(ParseDiagnosticLine("Success", d));
    EXPECT_FALSE(ParseDiagnosticLine("Errors: 3", d));
    EXPECT_FALSE(ParseDiagnosticLine("Process exited with error: code 1", d));
}

TEST(Diagnostics, ExtractFromLog)
{
    const std::string log = "Task: agscc:a.asc\nRunning: agscc a.asc\r\n"
                            "Warning: issue at a.asc, line 2 : w\r\n"
                            "Error: compile failed at a.asc, line 7 : e\r\n"
                            "\tExit code: 255\n\nFailure: Process exited with error code 255\n";
    const auto list = ExtractDiagnostics(log);
    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(Severity::Warning, list[0].Level);
    EXPECT_EQ(2, list[0].Line);
    EXPECT_EQ(Severity::Error, list[1].Level);
    EXPECT_EQ(7, list[1].Line);
    EXPECT_EQ("e", list[1].Message);
}

// The tools print "Error: failed to X:" and the reason on the next line (spriteimport, agspak, ...)
TEST(Diagnostics, ReasonOnTheNextLine)
{
    const std::string log = "Error: failed to import sprite(s) from source file 'a.png':\r\n"
                            "Failed to load image file a.png\r\n"
                            "Error: failed to write pack file:\n"
                            "\tExit code: 255\n"
                            "Error:\n"
                            "the reason alone\n"
                            "Error: failed to gather list of files:\n"
                            "Error: no such folder\n";
    const auto list = ExtractDiagnostics(log);
    ASSERT_EQ(5u, list.size());
    EXPECT_EQ("failed to import sprite(s) from source file 'a.png': Failed to load image file a.png", list[0].Message);
    // An indented line is agsbuild's, not the reason
    EXPECT_EQ("failed to write pack file:", list[1].Message);
    EXPECT_EQ("the reason alone", list[2].Message);
    // A diagnostic is no reason of the one before
    EXPECT_EQ("failed to gather list of files:", list[3].Message);
    EXPECT_EQ("no such folder", list[4].Message);
}

TEST(Diagnostics, BlankLineEndsTheMessage)
{
    const auto list = ExtractDiagnostics("Warning: a warning ends here:\n\nnot its reason\n");
    ASSERT_EQ(1u, list.size());
    EXPECT_EQ("a warning ends here:", list[0].Message);
}

// The editor shows the message: there always is one
TEST(Diagnostics, EmittedDiagnosticHasAMessage)
{
    std::vector<Diagnostic> seen;
    SetDiagnosticObserver([&seen](const Diagnostic &d) { seen.push_back(d); });
    Diagnostic d;
    d.Task = "copy-rooms";
    EmitDiagnostic(d);
    d.Task.clear();
    EmitDiagnostic(d);
    SetDiagnosticObserver(nullptr);
    ASSERT_EQ(2u, seen.size());
    EXPECT_EQ("(no message from the task copy-rooms)", seen[0].Message);
    EXPECT_EQ("(no message)", seen[1].Message);
}

TEST(Diagnostics, JsonStrings)
{
    EXPECT_EQ("\"plain\"", JsonString("plain"));
    EXPECT_EQ("\"a\\\"b\\\\c\"", JsonString("a\"b\\c"));
    EXPECT_EQ("\"l1\\nl2\\t\\r\"", JsonString("l1\nl2\t\r"));
    EXPECT_EQ("\"\\u0001\"", JsonString("\x01"));
    // valid UTF-8 stays, each byte which is not UTF-8 (for example ANSI text) becomes a question mark
    EXPECT_EQ("\"caf\xC3\xA9\"", JsonString("caf\xC3\xA9"));
    EXPECT_EQ("\"caf?\"", JsonString("caf\xE9"));
    EXPECT_EQ("\"\xE2\x82\xAC??\"", JsonString("\xE2\x82\xAC\xE2\x82"));
}

TEST(Diagnostics, DiagnosticJson)
{
    Diagnostic d;
    d.Level = Severity::Error;
    d.Tool = "agscc";
    d.Task = "agscc:a.asc";
    d.File = "a.asc";
    d.Line = 7;
    d.Message = "bad \"x\"";
    EXPECT_EQ("{\"event\":\"diagnostic\",\"severity\":\"error\",\"tool\":\"agscc\",\"task\":\"agscc:a.asc\","
              "\"file\":\"a.asc\",\"line\":7,\"message\":\"bad \\\"x\\\"\"}",
              ToJson(d));

    Diagnostic w;
    w.Level = Severity::Warning;
    w.Tool = "po2tra";
    w.Task = "po2tra:fr";
    w.Message = "m";
    EXPECT_EQ("{\"event\":\"diagnostic\",\"severity\":\"warning\",\"tool\":\"po2tra\",\"task\":\"po2tra:fr\","
              "\"message\":\"m\"}",
              ToJson(w));
}
