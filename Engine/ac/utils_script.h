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
// Functions of utils_script.cpp that the script bindings call.
//
//=============================================================================
#pragma once

// Script API: Utils::SortStrings^3
void Utils_SortStrings(void *arrobj, int compare_style, int sort_dir);

// Script API: Utils::SortInts^2
void Utils_SortInts(void *arrobj, int sort_dir);

// Script API: Utils::SortFloats^2
void Utils_SortFloats(void *arrobj, int sort_dir);
