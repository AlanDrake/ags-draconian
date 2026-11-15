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
// Containers script API.
//
//=============================================================================
#include "game/lib/script_containers.h"

#include "game/dynobj/script_dict.h"
#include "game/dynobj/script_set.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/cc_script_object.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/bbop.h"

//=============================================================================
//
// Dictionary of strings script API.
//
//=============================================================================

// NOTE: locale_aware only makes sense in unicode mode, and it only makes
// a difference for sorted containers, because the unsorted containers
// calculate hash, which does not care about order of letters in alphabet.
using namespace AGS;
using namespace AGS::Common;

ScriptDictBase *Dict_CreateImpl(ScriptSortStyle sort_style, ScriptStringComparison compare_style)
{
    EngineServices &services = EngineServices::Current();
    const bool sorted = sort_style == kScSorted;
    const bool case_sensitive = (compare_style & kScCaseSensitiveFlag) != 0;
    const bool locale_aware = (compare_style & kScLocaleAwareFlag) != 0;

    ScriptDictBase *dic;
    if (sorted)
    {
        if (case_sensitive)
            dic = locale_aware ? (ScriptDictBase *)new ScriptDictLocaleAware(
                                     LexographicalStrLess(services.TextLocaleName.GetCStr()))
                               : new ScriptDict();
        else
            dic = locale_aware ? (ScriptDictBase *)new ScriptDictLocaleAwareCI(
                                     LexographicalStrLessNoCase(services.TextLocaleName.GetCStr()))
                               : new ScriptDictUtf8CI();
    }
    else
    {
        if (case_sensitive)
            dic = new ScriptHashDict();
        else
            dic = new ScriptHashDictUtf8CI();
    }
    return dic;
}

ScriptDictBase *Dict_Create(int sort_style, int compare_style)
{
    ScriptDictBase *dic = Dict_CreateImpl(ValidateSortStyle("Dictionary.Create", sort_style),
                                          ValidateStringComparison("Dictionary.Create", compare_style));
    ccRegisterManagedObject(dic, dic);
    return dic;
}

// TODO: we need memory streams
ScriptDictBase *Dict_Unserialize(int index, AGS::Common::Stream *in, size_t data_sz)
{
    if (data_sz < sizeof(int32_t) * 2)
        quit("Dict_Unserialize: not enough data."); // TODO: don't quit, return error
    const int sort_style = in->ReadInt32();
    const int compare_style = in->ReadInt32();
    ScriptDictBase *dic =
        Dict_CreateImpl(static_cast<ScriptSortStyle>(sort_style), static_cast<ScriptStringComparison>(compare_style));
    dic->Unserialize(index, in, data_sz - sizeof(int32_t) * 2);
    return dic;
}

void Dict_Clear(ScriptDictBase *dic)
{
    dic->Clear();
}

bool Dict_Contains(ScriptDictBase *dic, const char *key)
{
    return dic->Contains(key);
}

const char *Dict_Get(ScriptDictBase *dic, const char *key)
{
    auto *str = dic->Get(key);
    return str ? CreateNewScriptString(str) : nullptr;
}

bool Dict_Remove(ScriptDictBase *dic, const char *key)
{
    return dic->Remove(key);
}

bool Dict_Set(ScriptDictBase *dic, const char *key, const char *value)
{
    return dic->Set(key, value);
}

int Dict_GetCompareStyle(ScriptDictBase *dic)
{
    return dic->GetStringCompareStyle();
}

int Dict_GetSortStyle(ScriptDictBase *dic)
{
    return dic->GetSortStyle();
}

int Dict_GetItemCount(ScriptDictBase *dic)
{
    return dic->GetItemCount();
}

void *Dict_GetKeysAsArray(ScriptDictBase *dic)
{
    std::vector<const char *> items;
    dic->GetKeys(items);
    if (items.size() == 0)
        return nullptr;
    DynObjectRef arr = DynamicArrayHelpers::CreateStringArray(items);
    return arr.Obj();
}

void *Dict_GetValuesAsArray(ScriptDictBase *dic)
{
    std::vector<const char *> items;
    dic->GetValues(items);
    if (items.size() == 0)
        return nullptr;
    DynObjectRef arr = DynamicArrayHelpers::CreateStringArray(items);
    return arr.Obj();
}

//=============================================================================
//
// Set of strings script API.
//
//=============================================================================

// NOTE: locale_aware only makes sense in unicode mode, and it only makes
// a difference for sorted containers, because the unsorted containers
// calculate hash, which does not care about order of letters in alphabet.
ScriptSetBase *Set_CreateImpl(ScriptSortStyle sort_style, ScriptStringComparison compare_style)
{
    EngineServices &services = EngineServices::Current();
    const bool sorted = sort_style == kScSorted;
    const bool case_sensitive = (compare_style & kScCaseSensitiveFlag) != 0;
    const bool locale_aware = (compare_style & kScLocaleAwareFlag) != 0;

    ScriptSetBase *set;
    if (sorted)
    {
        if (case_sensitive)
            set =
                locale_aware
                    ? (ScriptSetBase *)new ScriptSetLocaleAware(LexographicalStrLess(services.TextLocaleName.GetCStr()))
                    : new ScriptSet();
        else
            set = locale_aware ? (ScriptSetBase *)new ScriptSetLocaleAwareCI(
                                     LexographicalStrLessNoCase(services.TextLocaleName.GetCStr()))
                               : new ScriptSetUtf8CI();
    }
    else
    {
        if (case_sensitive)
            set = new ScriptHashSet();
        else
            set = new ScriptHashSetUtf8CI();
    }
    return set;
}

ScriptSetBase *Set_Create(int sort_style, int compare_style)
{
    ScriptSetBase *set = Set_CreateImpl(ValidateSortStyle("Set.Create", sort_style),
                                        ValidateStringComparison("Set.Create", compare_style));
    ccRegisterManagedObject(set, set);
    return set;
}

// TODO: we need memory streams
ScriptSetBase *Set_Unserialize(int index, AGS::Common::Stream *in, size_t data_sz)
{
    if (data_sz < sizeof(int32_t) * 2)
        quit("Set_Unserialize: not enough data."); // TODO: don't quit, return error
    const int sort_style = in->ReadInt32();
    const int compare_style = in->ReadInt32();
    ScriptSetBase *set =
        Set_CreateImpl(static_cast<ScriptSortStyle>(sort_style), static_cast<ScriptStringComparison>(compare_style));
    set->Unserialize(index, in, data_sz - sizeof(int32_t) * 2);
    return set;
}

bool Set_Add(ScriptSetBase *set, const char *item)
{
    return set->Add(item);
}

void Set_Clear(ScriptSetBase *set)
{
    set->Clear();
}

bool Set_Contains(ScriptSetBase *set, const char *item)
{
    return set->Contains(item);
}

bool Set_Remove(ScriptSetBase *set, const char *item)
{
    return set->Remove(item);
}

int Set_GetCompareStyle(ScriptSetBase *set)
{
    return set->GetStringCompareStyle();
}

int Set_GetSortStyle(ScriptSetBase *set)
{
    return set->GetSortStyle();
}

int Set_GetItemCount(ScriptSetBase *set)
{
    return set->GetItemCount();
}

void *Set_GetItemsAsArray(ScriptSetBase *set)
{
    std::vector<const char *> items;
    set->GetItems(items);
    if (items.size() == 0)
        return nullptr;
    DynObjectRef arr = DynamicArrayHelpers::CreateStringArray(items);
    return arr.Obj();
}
