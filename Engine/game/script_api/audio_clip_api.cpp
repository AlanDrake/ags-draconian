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
// Script bindings registered by RegisterAudioClipAPI().
//
//=============================================================================
#include "game/audio/audio_clip.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;

void RegisterAudioClipAPI()
{
    ScFnRegister audioclip_api[] = {
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&AudioClip_GetByName>("AudioClip::GetByName"),
        ScStatic<&AudioClip_GetByName>("AudioClip::GetByScriptName"),
        ScMethod<&AudioClip_Play>("AudioClip::Play^2"),
        ScMethod<&AudioClip_PlayFrom>("AudioClip::PlayFrom^3"),
        ScMethod<&AudioClip_PlayQueued>("AudioClip::PlayQueued^2"),
        ScMethod<&AudioClip_PlayOnChannel>("AudioClip::PlayOnChannel^3"),
        ScMethod<&AudioClip_PlayAsType>("AudioClip::PlayAsType^4"),
        ScMethod<&AudioClip_Stop>("AudioClip::Stop^0"),
        ScMethod<&AudioClip_GetProperty>("AudioClip::GetProperty^1"),
        ScMethod<&AudioClip_GetTextProperty>("AudioClip::GetTextProperty^1"),
        ScMethod<&AudioClip_SetProperty>("AudioClip::SetProperty^2"),
        ScMethod<&AudioClip_SetTextProperty>("AudioClip::SetTextProperty^2"),
        ScMethod<&AudioClip_GetID>("AudioClip::get_ID"),
        ScMethod<&AudioClip_GetDefaultPriority>("AudioClip::get_DefaultPriority"),
        ScMethod<&AudioClip_GetDefaultRepeat>("AudioClip::get_DefaultRepeat"),
        ScMethod<&AudioClip_GetDefaultVolume>("AudioClip::get_DefaultVolume"),
        ScMethod<&AudioClip_GetFileType>("AudioClip::get_FileType"),
        ScMethod<&AudioClip_GetIsAvailable>("AudioClip::get_IsAvailable"),
        ScMethod<&AudioClip_GetScriptName>("AudioClip::get_ScriptName"),
        ScMethod<&AudioClip_GetType>("AudioClip::get_Type"),
    };

    ccAddExternalFunctions(audioclip_api);
}
