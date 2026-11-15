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
#include "game/lib/utils_script.h"

#include "debug/debug_log.h"
#include "game/game.h"
#include "game/lib/string.h"
#include "platform/base/engine_services.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/string_utils.h"

using namespace AGS::Common;

struct DynStrCmpAuto
{
    DynStrCmpAuto(std::unique_ptr<IStrCmp> &&cmp_impl) : _cmpImpl(std::move(cmp_impl)) {}

    bool operator()(const DynObjectRef &s1, const DynObjectRef &s2) const
    {
        return _cmpImpl->operator()(String::Wrapper(static_cast<const char *>(s1.Obj())),
                                    String::Wrapper(static_cast<const char *>(s2.Obj()))) < 0;
    }

private:
    // It's shared ptr, because STL requires a copy constructor for predicates
    std::shared_ptr<IStrCmp> _cmpImpl;
};

void Utils_SortStrings(void *arrobj, int compare_style, int sort_dir)
{
    const auto scsort_dir = ValidateSortDirection("Utils.SortStrings", sort_dir);
    if (scsort_dir == kScSortNone)
        return;

    std::vector<DynObjectRef> string_objs;
    if (!DynamicArrayHelpers::ResolvePointerArray(arrobj, string_objs))
    {
        debug_script_error("Utils.SortStrings: internal error: provided array is not a pointer array?");
        return;
    }

    compare_style = ValidateStringComparison("Utils.SortStrings", compare_style);
    DynStrCmpAuto dynstr_less(StrUtil::GetStrCmpImplFor(
        true, (compare_style & kScCaseSensitiveFlag) == 0,
        (compare_style & kScLocaleAwareFlag) != 0 ? EngineServices::Current().TextLocaleName.GetCStr() : nullptr));
    if (scsort_dir == kScSortAscending)
        std::sort(string_objs.begin(), string_objs.end(), dynstr_less);
    else
        std::sort(string_objs.rbegin(), string_objs.rend(), dynstr_less);

    int *handle_begin = static_cast<int *>(arrobj);
    int *handle_end = handle_begin + CCDynamicArray::GetHeader(arrobj).GetElemCount();
    int *handle = handle_begin;
    for (auto scstr = string_objs.cbegin(); scstr != string_objs.cend() && handle != handle_end; ++scstr, ++handle)
    {
        *handle = scstr->Handle();
    }
}

void Utils_SortInts(void *arrobj, int sort_dir)
{
    auto scsort_dir = ValidateSortDirection("Utils.SortInts", sort_dir);
    if (scsort_dir == kScSortNone)
        return;

    int *int_begin = static_cast<int *>(arrobj);
    int *int_end = int_begin + CCDynamicArray::GetHeader(arrobj).GetElemCount();
    if (scsort_dir == kScSortAscending)
        std::sort(int_begin, int_end);
    else
        std::sort(int_begin, int_end, std::greater<int>());
}

void Utils_SortFloats(void *arrobj, int sort_dir)
{
    auto scsort_dir = ValidateSortDirection("Utils.SortFloats", sort_dir);
    if (scsort_dir == kScSortNone)
        return;

    float *float_begin = static_cast<float *>(arrobj);
    float *float_end = float_begin + CCDynamicArray::GetHeader(arrobj).GetElemCount();
    if (scsort_dir == kScSortAscending)
        std::sort(float_begin, float_end);
    else
        std::sort(float_begin, float_end, std::greater<float>());
}
