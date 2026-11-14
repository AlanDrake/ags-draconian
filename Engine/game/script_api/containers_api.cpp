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
#include "game/dynobj/script_dict.h"
#include "game/dynobj/script_set.h"
#include "game/lib/script_containers.h"
#include "game/script_api/bindings.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;

void RegisterContainerAPI()
{
    ScFnRegister container_api[] = {
        // Dictionary
        ScStatic<&Dict_Create>("Dictionary::Create"),
        ScMethod<&Dict_Clear>("Dictionary::Clear"),
        ScMethod<&Dict_Contains>("Dictionary::Contains"),
        ScMethod<&Dict_Get>("Dictionary::Get"),
        ScMethod<&Dict_Remove>("Dictionary::Remove"),
        ScMethod<&Dict_Set>("Dictionary::Set"),
        ScMethod<&Dict_GetCompareStyle>("Dictionary::get_CompareStyle"),
        ScMethod<&Dict_GetSortStyle>("Dictionary::get_SortStyle"),
        ScMethod<&Dict_GetItemCount>("Dictionary::get_ItemCount"),
        ScMethod<&Dict_GetKeysAsArray, ScManagedBy<globalDynamicArray>>("Dictionary::GetKeysAsArray"),
        ScMethod<&Dict_GetValuesAsArray, ScManagedBy<globalDynamicArray>>("Dictionary::GetValuesAsArray"),
        // Set
        ScStatic<&Set_Create>("Set::Create"),
        ScMethod<&Set_Add>("Set::Add"),
        ScMethod<&Set_Clear>("Set::Clear"),
        ScMethod<&Set_Contains>("Set::Contains"),
        ScMethod<&Set_Remove>("Set::Remove"),
        ScMethod<&Set_GetCompareStyle>("Set::get_CompareStyle"),
        ScMethod<&Set_GetSortStyle>("Set::get_SortStyle"),
        ScMethod<&Set_GetItemCount>("Set::get_ItemCount"),
        ScMethod<&Set_GetItemsAsArray, ScManagedBy<globalDynamicArray>>("Set::GetItemsAsArray"),
    };

    ccAddExternalFunctions(container_api);
}
