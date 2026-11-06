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
#include "data/dialogscriptconv.h"
#include "gtest/gtest.h"

using AGS::DataUtil::CharacterRef;
using AGS::DataUtil::DialogRef;
using AGS::DataUtil::DialogScriptConverter;
using AGS::DataUtil::GameRef;

namespace
{

GameRef MakeGame()
{
    GameRef game;
    CharacterRef bob;
    bob.ID = 0;
    bob.ScriptName = "cBob";
    game.Characters.push_back(bob);
    return game;
}

DialogRef MakeDialog()
{
    DialogRef dialog;
    dialog.ID = 2;
    dialog.OptionCount = 1;
    return dialog;
}

} // namespace

TEST(DialogScriptConverter, SpeechBecomesAStringLiteral)
{
    const GameRef game = MakeGame();
    const DialogRef dialog = MakeDialog();
    // Quotes in the text are escaped; a line without text says "..."
    DialogScriptConverter conv("@S\n"
                               "bob: \"Aye,\" he said. It's so.\n"
                               "narrator:\n"
                               "player: Hi // a comment\n"
                               "return\n",
                               game, dialog);
    EXPECT_STREQ("function _run_dialog2(int entryPoint) { if (entryPoint == 0) {\n"
                 "cBob.Say(\"\\\"Aye,\\\" he said. It's so.\");\n"
                 "Display(\"...\");\n"
                 "player.Say(\"Hi\");\n"
                 "return RUN_DIALOG_RETURN; }\n"
                 "return RUN_DIALOG_RETURN; }\n",
                 conv.Convert().GetCStr());
    EXPECT_TRUE(conv.GetErrors().empty());
}

TEST(DialogScriptConverter, KeepsTheLineNumbersOfTheDialogScript)
{
    const GameRef game = MakeGame();
    const DialogRef dialog = MakeDialog();
    // A line of the dialog script gives one line, so that line 4 here is line 4 there; the last
    // entry point has no return and is closed
    DialogScriptConverter conv("// A comment\n"
                               "\n"
                               "@S\n"
                               "  this.SetOptionState(1, eOptionOff);\n"
                               "stop\n"
                               "@1 // the option\n"
                               "\n"
                               "bob: Bye",
                               game, dialog);
    EXPECT_STREQ("function _run_dialog2(int entryPoint) { \n"
                 "\n"
                 "if (entryPoint == 0) {\n"
                 "  dialog[2].SetOptionState(1, eOptionOff);\n"
                 "return RUN_DIALOG_STOP_DIALOG; }\n"
                 "else if (entryPoint == 1) {\n"
                 "\n"
                 "cBob.Say(\"Bye\");\n"
                 "}\n"
                 "return RUN_DIALOG_RETURN; }\n",
                 conv.Convert().GetCStr());
    EXPECT_TRUE(conv.GetErrors().empty());

    // A dialog without a script
    DialogScriptConverter empty("", game, dialog);
    EXPECT_STREQ("function _run_dialog2(int entryPoint) { return RUN_DIALOG_RETURN; }\n", empty.Convert().GetCStr());
}

TEST(DialogScriptConverter, ReportsAnUnknownCharacter)
{
    const GameRef game = MakeGame();
    const DialogRef dialog = MakeDialog();
    DialogScriptConverter conv("@S\nalice: Hello\nstop\n", game, dialog);
    conv.Convert();
    ASSERT_EQ(1u, conv.GetErrors().size());
    EXPECT_TRUE(conv.GetErrors()[0].Error);
    EXPECT_STREQ("Dialog 2", conv.GetErrors()[0].ScriptName.GetCStr());
    EXPECT_EQ(2u, conv.GetErrors()[0].LineNumber);
}

TEST(DialogScriptConverter, FindsCharactersWithoutCase)
{
    // The name is the script name, or the script name without its "c", in any case
    GameRef game = MakeGame();
    CharacterRef ann;
    ann.ID = 1;
    ann.ScriptName = "CAnn";
    game.Characters.push_back(ann);
    const DialogRef dialog = MakeDialog();
    DialogScriptConverter conv("@S\nBOB: One\ncbob: Two\nann: Three\nreturn\n", game, dialog);
    EXPECT_STREQ("function _run_dialog2(int entryPoint) { if (entryPoint == 0) {\n"
                 "cBob.Say(\"One\");\n"
                 "cBob.Say(\"Two\");\n"
                 "CAnn.Say(\"Three\");\n"
                 "return RUN_DIALOG_RETURN; }\n"
                 "return RUN_DIALOG_RETURN; }\n",
                 conv.Convert().GetCStr());
    EXPECT_TRUE(conv.GetErrors().empty());
}
