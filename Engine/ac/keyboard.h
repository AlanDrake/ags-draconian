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
// Keyboard state for the script API: which keys are held down.
//
//=============================================================================
#pragma once

// Returns nonzero if the key with the given script key code is held down.
int IsKeyPressed(int keycode);
// Returns true if any key is held down.
bool IsAnyKeyPressed();
// Returns a new script array (int, script key codes) of the keys held down now.
void *GetPressedKeys();
