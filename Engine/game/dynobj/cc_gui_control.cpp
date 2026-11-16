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
#include "game/dynobj/cc_gui_control.h"

#include "game/game_context.h"
#include "game/game_init.h"
#include "game/ui/gui.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

// return the type name of the object
const char *CCGUIControl::GetType()
{
    return "GUIControl";
}

size_t CCGUIControl::CalcSerializeSize(const void * /*address*/)
{
    return sizeof(int32_t) * 2;
}

void CCGUIControl::Serialize(const void *address, Stream *out)
{
    const GUIControl *control = static_cast<const GUIControl *>(address);
    out->WriteInt32(control->GetParentID());
    out->WriteInt32(control->GetID());
}

void CCGUIControl::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    Game &game = Game::Current();
    int guinum = in->ReadInt32();
    int objnum = in->ReadInt32();
    int handle = ccRegisterUnserializedPersistentObject(index, game.Guis[guinum].GetControl(objnum), this);
    (game.Handles.GuiControls[guinum])[objnum] = handle;
}
