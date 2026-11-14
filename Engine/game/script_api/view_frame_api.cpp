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
// Script bindings registered by RegisterViewFrameAPI().
//
//=============================================================================
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/view_frame.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;

void RegisterViewFrameAPI()
{
    ScFnRegister viewframe_api[] = {
        ScMethod<&ViewFrame_GetEventName>("ViewFrame::get_EventName"),
        ScMethod<&ViewFrame_SetEventName>("ViewFrame::set_EventName"),
        ScMethod<&ViewFrame_GetFlipped>("ViewFrame::get_Flipped"),
        ScMethod<&ViewFrame_SetFlipped>("ViewFrame::set_Flipped"),
        ScMethod<&ViewFrame_GetFrame>("ViewFrame::get_Frame"),
        ScMethod<&ViewFrame_GetGraphic>("ViewFrame::get_Graphic"),
        ScMethod<&ViewFrame_SetGraphic>("ViewFrame::set_Graphic"),
        ScMethod<&ViewFrame_GetLinkedAudio>("ViewFrame::get_LinkedAudio"),
        ScMethod<&ViewFrame_SetLinkedAudio>("ViewFrame::set_LinkedAudio"),
        ScMethod<&ViewFrame_GetLoop>("ViewFrame::get_Loop"),
        ScMethod<&ViewFrame_GetSpeed>("ViewFrame::get_Speed"),
        ScMethod<&ViewFrame_SetSpeed>("ViewFrame::set_Speed"),
        ScMethod<&ViewFrame_GetView>("ViewFrame::get_View"),
        ScMethod<&ViewFrame_GetXOffset>("ViewFrame::get_XOffset"),
        ScMethod<&ViewFrame_SetXOffset>("ViewFrame::set_XOffset"),
        ScMethod<&ViewFrame_GetYOffset>("ViewFrame::get_YOffset"),
        ScMethod<&ViewFrame_SetYOffset>("ViewFrame::set_YOffset"),
    };

    ccAddExternalFunctions(viewframe_api);
}
