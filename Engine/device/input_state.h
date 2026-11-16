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
// The state of the input devices (device/mousew32.cpp, device/sys_events.cpp), held by EngineServices::Input.
//
//=============================================================================
#pragma once

struct InputState
{
    // The cursor's position in game coordinates, updated by Mouse::Poll.
    int MouseX = 0;
    int MouseY = 0;
    // Above 0 while the cursor may leave the movement limits set with Mouse::SetMoveLimit (a counter).
    int IgnoreBounds = 0;
    // TODO: hide these later? (bad design with run_service_key_controls,
    // but also need to clear them in ags_clear_input_state())
    int ModKeys = 0;           // accumulated mod flags
    bool ModKeysFired = false; // tells whether mod combination had been used for action
    // TODO: hide these later after refactoring mousew32.cpp
    volatile int SysMouseX = 0; // mouse x position
    volatile int SysMouseY = 0; // mouse y position
    volatile int SysMouseZ = 0; // mouse wheel position
};
