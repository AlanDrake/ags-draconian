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
#include "game/input/keyboard.h"

#include "device/sys_events.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "util/keycode.h"

int IsKeyPressed(int keycode)
{
    return ags_iskeydown(static_cast<eAGSKeyCode>(keycode));
}

bool IsAnyKeyPressed()
{
    return ags_isanykeydown();
}

void *GetPressedKeys()
{
    const auto &keys = ags_getkeysdown();
    DynObjectRef arr = DynamicArrayHelpers::CreateArray(sizeof(int), keys.size());
    if (arr)
    {
        for (size_t i = 0; i < keys.size(); ++i)
            *(static_cast<uint32_t *>(arr.Obj()) + i) =
                AGSKeyToScriptKey(sdl_key_to_ags_key(keys[i].sym, keys[i].scancode, 0u /* ignore mod */));
    }
    return arr.Obj();
}
