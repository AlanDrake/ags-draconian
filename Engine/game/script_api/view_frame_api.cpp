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
#include "ac/game.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/dynobj/script_string.h"
#include "game/world/view_frame.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;

RuntimeScriptValue Sc_ViewFrame_GetEventName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptViewFrame, const char, myScriptStringImpl, ViewFrame_GetEventName);
}

RuntimeScriptValue Sc_ViewFrame_SetEventName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewFrame, ViewFrame_SetEventName, const char);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetFlipped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetFlipped);
}

RuntimeScriptValue Sc_ViewFrame_SetFlipped(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetFlipped);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetFrame);
}
// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetGraphic);
}

// void (ScriptViewFrame *svf, int newPic)
RuntimeScriptValue Sc_ViewFrame_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetGraphic);
}

// ScriptAudioClip* (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetLinkedAudio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptViewFrame, ScriptAudioClip, ccDynamicAudioClip, ViewFrame_GetLinkedAudio);
}

// void (ScriptViewFrame *svf, ScriptAudioClip* clip)
RuntimeScriptValue Sc_ViewFrame_SetLinkedAudio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptViewFrame, ViewFrame_SetLinkedAudio, ScriptAudioClip);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetSpeed);
}

RuntimeScriptValue Sc_ViewFrame_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetSpeed);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetLoop);
}

// int (ScriptViewFrame *svf)
RuntimeScriptValue Sc_ViewFrame_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetView);
}

RuntimeScriptValue Sc_ViewFrame_GetXOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetXOffset);
}

RuntimeScriptValue Sc_ViewFrame_SetXOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetXOffset);
}

RuntimeScriptValue Sc_ViewFrame_GetYOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptViewFrame, ViewFrame_GetYOffset);
}

RuntimeScriptValue Sc_ViewFrame_SetYOffset(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptViewFrame, ViewFrame_SetYOffset);
}


void RegisterViewFrameAPI()
{
    ScFnRegister viewframe_api[] = {
        {"ViewFrame::get_EventName", API_FN_PAIR(ViewFrame_GetEventName)},
        {"ViewFrame::set_EventName", API_FN_PAIR(ViewFrame_SetEventName)},
        {"ViewFrame::get_Flipped", API_FN_PAIR(ViewFrame_GetFlipped)},
        {"ViewFrame::set_Flipped", API_FN_PAIR(ViewFrame_SetFlipped)},
        {"ViewFrame::get_Frame", API_FN_PAIR(ViewFrame_GetFrame)},
        {"ViewFrame::get_Graphic", API_FN_PAIR(ViewFrame_GetGraphic)},
        {"ViewFrame::set_Graphic", API_FN_PAIR(ViewFrame_SetGraphic)},
        {"ViewFrame::get_LinkedAudio", API_FN_PAIR(ViewFrame_GetLinkedAudio)},
        {"ViewFrame::set_LinkedAudio", API_FN_PAIR(ViewFrame_SetLinkedAudio)},
        {"ViewFrame::get_Loop", API_FN_PAIR(ViewFrame_GetLoop)},
        {"ViewFrame::get_Speed", API_FN_PAIR(ViewFrame_GetSpeed)},
        {"ViewFrame::set_Speed", API_FN_PAIR(ViewFrame_SetSpeed)},
        {"ViewFrame::get_View", API_FN_PAIR(ViewFrame_GetView)},
        {"ViewFrame::get_XOffset", API_FN_PAIR(ViewFrame_GetXOffset)},
        {"ViewFrame::set_XOffset", API_FN_PAIR(ViewFrame_SetXOffset)},
        {"ViewFrame::get_YOffset", API_FN_PAIR(ViewFrame_GetYOffset)},
        {"ViewFrame::set_YOffset", API_FN_PAIR(ViewFrame_SetYOffset)},
    };

    ccAddExternalFunctions(viewframe_api);
}
