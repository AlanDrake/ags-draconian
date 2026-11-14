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
// Script bindings registered by RegisterContainerAPI().
//
//=============================================================================
#include "ac/dynobj/cc_dynamicarray.h"
#include "game/dynobj/script_dict.h"
#include "game/dynobj/script_set.h"
#include "game/dynobj/script_string.h"
#include "game/lib/script_containers.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;

RuntimeScriptValue Sc_Dict_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDictBase, Dict_Create);
}

RuntimeScriptValue Sc_Dict_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDictBase, Dict_Clear);
}

RuntimeScriptValue Sc_Dict_Contains(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptDictBase, Dict_Contains, const char);
}

RuntimeScriptValue Sc_Dict_Get(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptDictBase, const char, myScriptStringImpl, Dict_Get, const char);
}

RuntimeScriptValue Sc_Dict_Remove(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptDictBase, Dict_Remove, const char);
}

RuntimeScriptValue Sc_Dict_Set(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptDictBase, Dict_Set, const char, const char);
}

RuntimeScriptValue Sc_Dict_GetCompareStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDictBase, Dict_GetCompareStyle);
}

RuntimeScriptValue Sc_Dict_GetSortStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDictBase, Dict_GetSortStyle);
}

RuntimeScriptValue Sc_Dict_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDictBase, Dict_GetItemCount);
}

RuntimeScriptValue Sc_Dict_GetKeysAsArray(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDictBase, void, globalDynamicArray, Dict_GetKeysAsArray);
}

RuntimeScriptValue Sc_Dict_GetValuesAsArray(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptDictBase, void, globalDynamicArray, Dict_GetValuesAsArray);
}

RuntimeScriptValue Sc_Set_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptSetBase, Set_Create);
}

RuntimeScriptValue Sc_Set_Add(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Add, const char);
}

RuntimeScriptValue Sc_Set_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptSetBase, Set_Clear);
}

RuntimeScriptValue Sc_Set_Contains(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Contains, const char);
}

RuntimeScriptValue Sc_Set_Remove(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ(ScriptSetBase, Set_Remove, const char);
}

RuntimeScriptValue Sc_Set_GetCompareStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptSetBase, Set_GetCompareStyle);
}

RuntimeScriptValue Sc_Set_GetSortStyle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptSetBase, Set_GetSortStyle);
}

RuntimeScriptValue Sc_Set_GetItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptSetBase, Set_GetItemCount);
}

RuntimeScriptValue Sc_Set_GetItemsAsArray(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptSetBase, void, globalDynamicArray, Set_GetItemsAsArray);
}


void RegisterContainerAPI()
{
    ScFnRegister container_api[] = {
        // Dictionary
        {"Dictionary::Create", API_FN_PAIR(Dict_Create)},
        {"Dictionary::Clear", API_FN_PAIR(Dict_Clear)},
        {"Dictionary::Contains", API_FN_PAIR(Dict_Contains)},
        {"Dictionary::Get", API_FN_PAIR(Dict_Get)},
        {"Dictionary::Remove", API_FN_PAIR(Dict_Remove)},
        {"Dictionary::Set", API_FN_PAIR(Dict_Set)},
        {"Dictionary::get_CompareStyle", API_FN_PAIR(Dict_GetCompareStyle)},
        {"Dictionary::get_SortStyle", API_FN_PAIR(Dict_GetSortStyle)},
        {"Dictionary::get_ItemCount", API_FN_PAIR(Dict_GetItemCount)},
        {"Dictionary::GetKeysAsArray", API_FN_PAIR(Dict_GetKeysAsArray)},
        {"Dictionary::GetValuesAsArray", API_FN_PAIR(Dict_GetValuesAsArray)},
        // Set
        {"Set::Create", API_FN_PAIR(Set_Create)},
        {"Set::Add", API_FN_PAIR(Set_Add)},
        {"Set::Clear", API_FN_PAIR(Set_Clear)},
        {"Set::Contains", API_FN_PAIR(Set_Contains)},
        {"Set::Remove", API_FN_PAIR(Set_Remove)},
        {"Set::get_CompareStyle", API_FN_PAIR(Set_GetCompareStyle)},
        {"Set::get_SortStyle", API_FN_PAIR(Set_GetSortStyle)},
        {"Set::get_ItemCount", API_FN_PAIR(Set_GetItemCount)},
        {"Set::GetItemsAsArray", API_FN_PAIR(Set_GetItemsAsArray)},
    };

    ccAddExternalFunctions(container_api);
}
