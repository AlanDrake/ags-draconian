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
#include "game/lib/utils_script.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterUtilsAPI()
{
    ScFnRegister utils_api[] = {
        ScStatic<&Utils_SortStrings>("Utils::SortStrings^3"),
        ScStatic<&Utils_SortInts>("Utils::SortInts^2"),
        ScStatic<&Utils_SortFloats>("Utils::SortFloats^2"),
    };

    ccAddExternalFunctions(utils_api);
}
