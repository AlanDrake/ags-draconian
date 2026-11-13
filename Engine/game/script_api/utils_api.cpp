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
// Script bindings registered by RegisterUtilsAPI().
//
//=============================================================================
#include "ac/utils_script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_Utils_SortStrings(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT2(Utils_SortStrings, void);
}

RuntimeScriptValue Sc_Utils_SortInts(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(Utils_SortInts, void);
}

RuntimeScriptValue Sc_Utils_SortFloats(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(Utils_SortFloats, void);
}

void RegisterUtilsAPI()
{
    ScFnRegister utils_api[] = {
        {"Utils::SortStrings^3", API_FN_PAIR(Utils_SortStrings)},
        {"Utils::SortInts^2", API_FN_PAIR(Utils_SortInts)},
        {"Utils::SortFloats^2", API_FN_PAIR(Utils_SortFloats)},
    };

    ccAddExternalFunctions(utils_api);
}
