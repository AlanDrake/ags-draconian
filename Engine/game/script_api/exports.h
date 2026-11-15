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
#pragma once

#include "game/game_struct_defines.h"

// Registers every type's script functions and the static script objects: calls each function below.
void setup_script_exports(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);

// One per file of game/script_api/, defined there: registers the script functions of that type.
void RegisterAudioChannelAPI();
void RegisterAudioClipAPI();
void RegisterButtonAPI();
void RegisterCharacterAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
void RegisterContainerAPI();
void RegisterDateTimeAPI();
void RegisterDialogAPI();
void RegisterDialogOptionsRenderingAPI();
void RegisterDrawingSurfaceAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
void RegisterDynamicArrayAPI();
void RegisterDynamicSpriteAPI();
void RegisterFileAPI();
void RegisterGameAPI();
void RegisterGlobalAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
void RegisterGUIAPI();
void RegisterGUIControlAPI();
void RegisterHotspotAPI();
void RegisterInventoryItemAPI();
void RegisterInventoryWindowAPI();
void RegisterJoystickAPI();
void RegisterLabelAPI();
void RegisterListBoxAPI();
void RegisterMathAPI();
void RegisterMouseAPI();
void RegisterObjectAPI();
void RegisterOverlayAPI();
void RegisterParserAPI();
void RegisterPathfinderAPI();
void RegisterRegionAPI();
void RegisterRoomAPI();
void RegisterSaveInfoAPI();
void RegisterScreenAPI();
void RegisterShaderAPI();
void RegisterSliderAPI();
void RegisterSpeechAPI(ScriptAPIVersion base_api, ScriptAPIVersion compat_api);
void RegisterStringAPI();
void RegisterSystemAPI();
void RegisterTextBoxAPI();
void RegisterTouchAPI();
void RegisterUtilsAPI();
void RegisterViewFrameAPI();
void RegisterViewportAPI();
void RegisterVideoAPI();
void RegisterWalkareaAPI();
void RegisterWalkbehindAPI();
// Registers the static script objects (game_api.cpp).
void RegisterStaticObjects();
