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
#pragma once

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptTouchPointer final : CCBasicObject
{
public:
    ScriptTouchPointer(int pointer_id) : _pointerID(pointer_id) {}

    int GetPointerID() const { return _pointerID; }

    const char *GetType() override;
    int Dispose(void *address, bool force) override;

private:
    int _pointerID = -1;
};
