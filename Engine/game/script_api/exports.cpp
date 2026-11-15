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
// Registering symbols for the script system
//
//=============================================================================

#include "game/script_api/exports.h"

#include "game/game_struct_defines.h"

void setup_script_exports(ScriptAPIVersion base_api, ScriptAPIVersion compat_api)
{
    RegisterAudioChannelAPI();
    RegisterAudioClipAPI();
    RegisterButtonAPI();
    RegisterCharacterAPI(base_api, compat_api);
    RegisterContainerAPI();
    RegisterDateTimeAPI();
    RegisterDialogAPI();
    RegisterDialogOptionsRenderingAPI();
    RegisterDrawingSurfaceAPI(base_api, compat_api);
    RegisterDynamicArrayAPI();
    RegisterDynamicSpriteAPI();
    RegisterFileAPI();
    RegisterGameAPI();
    RegisterGlobalAPI(base_api, compat_api);
    RegisterGUIAPI();
    RegisterGUIControlAPI();
    RegisterHotspotAPI();
    RegisterInventoryItemAPI();
    RegisterInventoryWindowAPI();
    RegisterJoystickAPI();
    RegisterLabelAPI();
    RegisterListBoxAPI();
    RegisterMathAPI();
    RegisterMouseAPI();
    RegisterObjectAPI();
    RegisterOverlayAPI();
    RegisterParserAPI();
    RegisterPathfinderAPI();
    RegisterRegionAPI();
    RegisterRoomAPI();
    RegisterSaveInfoAPI();
    RegisterScreenAPI();
    RegisterSliderAPI();
    RegisterSpeechAPI(base_api, compat_api);
    RegisterStringAPI();
    RegisterSystemAPI();
    RegisterTextBoxAPI();
    RegisterTouchAPI();
    RegisterUtilsAPI();
    RegisterViewFrameAPI();
    RegisterViewportAPI();
    RegisterVideoAPI();
    RegisterWalkareaAPI();
    RegisterWalkbehindAPI();

    RegisterShaderAPI();

    RegisterStaticObjects();
}
