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
#include "game/dialog/speech.h"
#include "game/dynobj/script_overlay.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister speech_api[] = {
        ScStatic<&Speech_GetAlwaysWaitForText>("Speech::get_AlwaysWaitForText"),
        ScStatic<&Speech_SetAlwaysWaitForText>("Speech::set_AlwaysWaitForText"),
        ScStatic<&Speech_GetAnimationStopTimeMargin>("Speech::get_AnimationStopTimeMargin"),
        ScStatic<&Speech_SetAnimationStopTimeMargin>("Speech::set_AnimationStopTimeMargin"),
        ScStatic<&Speech_GetCustomPortraitPlacement>("Speech::get_CustomPortraitPlacement"),
        ScStatic<&Speech_SetCustomPortraitPlacement>("Speech::set_CustomPortraitPlacement"),
        ScStatic<&Speech_GetDisplayPostTimeMs>("Speech::get_DisplayPostTimeMs"),
        ScStatic<&Speech_SetDisplayPostTimeMs>("Speech::set_DisplayPostTimeMs"),
        ScStatic<&Speech_GetGlobalSpeechAnimationDelay>("Speech::get_GlobalSpeechAnimationDelay"),
        ScStatic<&Speech_SetGlobalSpeechAnimationDelay>("Speech::set_GlobalSpeechAnimationDelay"),
        ScStatic<&Speech_GetPortraitOverlay>("Speech::get_PortraitOverlay"),
        ScStatic<&Speech_GetPortraitXOffset>("Speech::get_PortraitXOffset"),
        ScStatic<&Speech_SetPortraitXOffset>("Speech::set_PortraitXOffset"),
        ScStatic<&Speech_GetPortraitY>("Speech::get_PortraitY"),
        ScStatic<&Speech_SetPortraitY>("Speech::set_PortraitY"),
        ScStatic<&Speech_GetSkipKey>("Speech::get_SkipKey"),
        ScStatic<&Speech_SetSkipKey>("Speech::set_SkipKey"),
        ScStatic<&Speech_GetSkipStyle>("Speech::get_SkipStyle"),
        ScStatic<&Speech_SetSkipStyle>("Speech::set_SkipStyle"),
        ScStatic<&Speech_GetSpeakingCharacter>("Speech::get_SpeakingCharacter"),
        ScStatic<&Speech_GetOverlayZOrder>("Speech::get_OverlayZOrder"),
        ScStatic<&Speech_SetOverlayZOrder>("Speech::set_OverlayZOrder"),
        ScStatic<&Speech_GetStyle>("Speech::get_Style"),
        ScStatic<&Speech_SetStyle>("Speech::set_Style"),
        ScStatic<&Speech_GetTextAlignment>("Speech::get_TextAlignment"),
        ScStatic<&Speech_SetTextAlignment>("Speech::set_TextAlignment"),
        ScStatic<&Speech_GetTextOverlay>("Speech::get_TextOverlay"),
        ScStatic<&Speech_GetUseGlobalSpeechAnimationDelay>("Speech::get_UseGlobalSpeechAnimationDelay"),
        ScStatic<&Speech_SetUseGlobalSpeechAnimationDelay>("Speech::set_UseGlobalSpeechAnimationDelay"),
        ScStatic<&Speech_GetVoiceMode>("Speech::get_VoiceMode"),
        ScStatic<&Speech_SetVoiceMode>("Speech::set_VoiceMode"),
    };

    ccAddExternalFunctions(speech_api);
}
