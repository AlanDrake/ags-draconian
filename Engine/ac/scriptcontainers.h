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
#pragma once

class ScriptDictBase;
class ScriptSetBase;

// Script API: Dictionary::Create
ScriptDictBase *Dict_Create(int sort_style, int compare_style);

// Script API: Dictionary::Clear
void Dict_Clear(ScriptDictBase *dic);

// Script API: Dictionary::Contains
bool Dict_Contains(ScriptDictBase *dic, const char *key);

// Script API: Dictionary::Get
const char *Dict_Get(ScriptDictBase *dic, const char *key);

// Script API: Dictionary::Remove
bool Dict_Remove(ScriptDictBase *dic, const char *key);

// Script API: Dictionary::Set
bool Dict_Set(ScriptDictBase *dic, const char *key, const char *value);

// Script API: Dictionary::get_CompareStyle
int Dict_GetCompareStyle(ScriptDictBase *dic);

// Script API: Dictionary::get_SortStyle
int Dict_GetSortStyle(ScriptDictBase *dic);

// Script API: Dictionary::get_ItemCount
int Dict_GetItemCount(ScriptDictBase *dic);

// Script API: Dictionary::GetKeysAsArray
void *Dict_GetKeysAsArray(ScriptDictBase *dic);

// Script API: Dictionary::GetValuesAsArray
void *Dict_GetValuesAsArray(ScriptDictBase *dic);

// Script API: Set::Create
ScriptSetBase *Set_Create(int sort_style, int compare_style);

// Script API: Set::Add
bool Set_Add(ScriptSetBase *set, const char *item);

// Script API: Set::Clear
void Set_Clear(ScriptSetBase *set);

// Script API: Set::Contains
bool Set_Contains(ScriptSetBase *set, const char *item);

// Script API: Set::Remove
bool Set_Remove(ScriptSetBase *set, const char *item);

// Script API: Set::get_CompareStyle
int Set_GetCompareStyle(ScriptSetBase *set);

// Script API: Set::get_SortStyle
int Set_GetSortStyle(ScriptSetBase *set);

// Script API: Set::get_ItemCount
int Set_GetItemCount(ScriptSetBase *set);

// Script API: Set::GetItemsAsArray
void *Set_GetItemsAsArray(ScriptSetBase *set);
