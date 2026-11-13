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
// Script bindings registered by RegisterDynamicArrayAPI().
//
//=============================================================================
#include "ac/dynobj/cc_dynamicarray.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

// Deprecated variant of DynamicArray.Length, which emulates a static function;
// was replaced in script compilers around Editor v4.0.0.11, but is kept for backwards compat.
RuntimeScriptValue Sc_DynamicArray_Static_Length(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(DynamicArray_Length, void);
}

RuntimeScriptValue Sc_DynamicArray_Length(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(void, DynamicArray_Length);
}

void RegisterDynamicArrayAPI()
{
    ccAddExternalStaticFunction("__Builtin_DynamicArrayLength^1", Sc_DynamicArray_Static_Length);
    ccAddExternalObjectFunction("__Builtin_DynamicArray::get_Length", Sc_DynamicArray_Length);
}
