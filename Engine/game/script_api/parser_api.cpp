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
// Script bindings registered by RegisterParserAPI().
//
//=============================================================================
#include <locale>

#include "ac/dynobj/scriptstring.h"
#include "game/dialog/parser.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

// int (const char *wordToFind)
RuntimeScriptValue Sc_Parser_FindWordID(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Parser_FindWordID, const char);
}

// void  (char*text)
RuntimeScriptValue Sc_ParseText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(ParseText, /*const*/ char);
}

// const char* ()
RuntimeScriptValue Sc_Parser_SaidUnknownWord(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Parser_SaidUnknownWord);
}

RuntimeScriptValue Sc_Parser_Said(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Parser_Said, const char);
}

void RegisterParserAPI()
{
    ScFnRegister parser_api[] = {
        {"Parser::FindWordID^1", API_FN_PAIR(Parser_FindWordID)},
        {"Parser::ParseText^1", API_FN_PAIR(ParseText)},
        {"Parser::SaidUnknownWord^0", API_FN_PAIR(Parser_SaidUnknownWord)},
        {"Parser::Said^1", API_FN_PAIR(Parser_Said)},
    };

    ccAddExternalFunctions(parser_api);
}
