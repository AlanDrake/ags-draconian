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
// Script bindings registered by RegisterSpeechAPI().
//
//=============================================================================
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/scriptoverlay.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "game/dialog/speech.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

RuntimeScriptValue Sc_Speech_GetAlwaysWaitForText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Speech_GetAlwaysWaitForText);
}

RuntimeScriptValue Sc_Speech_SetAlwaysWaitForText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Speech_SetAlwaysWaitForText);
}

RuntimeScriptValue Sc_Speech_GetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetAnimationStopTimeMargin);
}

RuntimeScriptValue Sc_Speech_SetAnimationStopTimeMargin(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetAnimationStopTimeMargin);
}

RuntimeScriptValue Sc_Speech_GetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetCustomPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_SetCustomPortraitPlacement(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetCustomPortraitPlacement);
}

RuntimeScriptValue Sc_Speech_GetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetDisplayPostTimeMs);
}

RuntimeScriptValue Sc_Speech_SetDisplayPostTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetDisplayPostTimeMs);
}

RuntimeScriptValue Sc_Speech_GetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_SetGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_GetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetPortraitXOffset);
}

RuntimeScriptValue Sc_Speech_SetPortraitXOffset(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetPortraitXOffset);
}

RuntimeScriptValue Sc_Speech_GetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetPortraitY);
}

RuntimeScriptValue Sc_Speech_SetPortraitY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetPortraitY);
}

RuntimeScriptValue Sc_Speech_GetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetStyle);
}

RuntimeScriptValue Sc_Speech_SetStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetStyle);
}

RuntimeScriptValue Sc_Speech_GetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetSkipKey);
}

RuntimeScriptValue Sc_Speech_SetSkipKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetSkipKey);
}

RuntimeScriptValue Sc_Speech_GetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetSkipStyle);
}

extern RuntimeScriptValue Sc_Speech_SetSkipStyle(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(SetSkipSpeech, 1);
    Speech_SetSkipStyle((SkipSpeechStyle)params[0].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Speech_GetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetTextAlignment);
}

RuntimeScriptValue Sc_Speech_SetTextAlignment(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetTextAlignment);
}

RuntimeScriptValue Sc_Speech_GetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetUseGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_SetUseGlobalSpeechAnimationDelay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetUseGlobalSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Speech_GetVoiceMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetVoiceMode);
}

RuntimeScriptValue Sc_Speech_SetVoiceMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetVoiceMode);
}

RuntimeScriptValue Sc_Speech_GetTextOverlay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptOverlay, Speech_GetTextOverlay);
}

RuntimeScriptValue Sc_Speech_GetPortraitOverlay(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptOverlay, Speech_GetPortraitOverlay);
}

RuntimeScriptValue Sc_Speech_GetSpeakingCharacter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(CharacterInfo, ccDynamicCharacter, Speech_GetSpeakingCharacter);
}

RuntimeScriptValue Sc_Speech_GetOverlayZOrder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Speech_GetOverlayZOrder);
}

RuntimeScriptValue Sc_Speech_SetOverlayZOrder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Speech_SetOverlayZOrder);
}


void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister speech_api[] = {
        {"Speech::get_AlwaysWaitForText", API_FN_PAIR(Speech_GetAlwaysWaitForText)},
        {"Speech::set_AlwaysWaitForText", API_FN_PAIR(Speech_SetAlwaysWaitForText)},
        {"Speech::get_AnimationStopTimeMargin", API_FN_PAIR(Speech_GetAnimationStopTimeMargin)},
        {"Speech::set_AnimationStopTimeMargin", API_FN_PAIR(Speech_SetAnimationStopTimeMargin)},
        {"Speech::get_CustomPortraitPlacement", API_FN_PAIR(Speech_GetCustomPortraitPlacement)},
        {"Speech::set_CustomPortraitPlacement", API_FN_PAIR(Speech_SetCustomPortraitPlacement)},
        {"Speech::get_DisplayPostTimeMs", API_FN_PAIR(Speech_GetDisplayPostTimeMs)},
        {"Speech::set_DisplayPostTimeMs", API_FN_PAIR(Speech_SetDisplayPostTimeMs)},
        {"Speech::get_GlobalSpeechAnimationDelay", API_FN_PAIR(Speech_GetGlobalSpeechAnimationDelay)},
        {"Speech::set_GlobalSpeechAnimationDelay", API_FN_PAIR(Speech_SetGlobalSpeechAnimationDelay)},
        {"Speech::get_PortraitOverlay", API_FN_PAIR(Speech_GetPortraitOverlay)},
        {"Speech::get_PortraitXOffset", API_FN_PAIR(Speech_GetPortraitXOffset)},
        {"Speech::set_PortraitXOffset", API_FN_PAIR(Speech_SetPortraitXOffset)},
        {"Speech::get_PortraitY", API_FN_PAIR(Speech_GetPortraitY)},
        {"Speech::set_PortraitY", API_FN_PAIR(Speech_SetPortraitY)},
        {"Speech::get_SkipKey", API_FN_PAIR(Speech_GetSkipKey)},
        {"Speech::set_SkipKey", API_FN_PAIR(Speech_SetSkipKey)},
        {"Speech::get_SkipStyle", API_FN_PAIR(Speech_GetSkipStyle)},
        {"Speech::set_SkipStyle", API_FN_PAIR(Speech_SetSkipStyle)},
        {"Speech::get_SpeakingCharacter", API_FN_PAIR(Speech_GetSpeakingCharacter)},
        {"Speech::get_OverlayZOrder", API_FN_PAIR(Speech_GetOverlayZOrder)},
        {"Speech::set_OverlayZOrder", API_FN_PAIR(Speech_SetOverlayZOrder)},
        {"Speech::get_Style", API_FN_PAIR(Speech_GetStyle)},
        {"Speech::set_Style", API_FN_PAIR(Speech_SetStyle)},
        {"Speech::get_TextAlignment", API_FN_PAIR(Speech_GetTextAlignment)},
        {"Speech::set_TextAlignment", API_FN_PAIR(Speech_SetTextAlignment)},
        {"Speech::get_TextOverlay", API_FN_PAIR(Speech_GetTextOverlay)},
        {"Speech::get_UseGlobalSpeechAnimationDelay", API_FN_PAIR(Speech_GetUseGlobalSpeechAnimationDelay)},
        {"Speech::set_UseGlobalSpeechAnimationDelay", API_FN_PAIR(Speech_SetUseGlobalSpeechAnimationDelay)},
        {"Speech::get_VoiceMode", API_FN_PAIR(Speech_GetVoiceMode)},
        {"Speech::set_VoiceMode", API_FN_PAIR(Speech_SetVoiceMode)},
    };

    ccAddExternalFunctions(speech_api);
}
