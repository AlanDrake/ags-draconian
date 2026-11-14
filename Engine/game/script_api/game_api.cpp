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
// Script bindings registered by RegisterGameAPI().
//
//=============================================================================
#include "game/dynobj/all_dynamic_classes.h"
#include "game/dynobj/all_script_classes.h"
#include "game/dynobj/script_camera.h"
#include "game/dynobj/script_game.h"
#include "game/game.h"
#include "game/game_state.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/ui/mouse.h"
#include "plugin/plugin_engine.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

extern RGB palette[256];

void RegisterGameAPI()
{
    ScFnRegister game_api[] = {
        ScStatic<&Game_IsAudioPlaying>("Game::IsAudioPlaying^1"),
        ScStatic<&Game_GetAudioTypeCrossfadeSpeed>("Game::GetAudioTypeCrossfadeSpeed^1"),
        ScStatic<&Game_SetAudioTypeCrossfadeSpeed>("Game::SetAudioTypeCrossfadeSpeed^2"),
        ScStatic<&Game_GetAudioTypeSpeechVolumeDrop>("Game::GetAudioTypeSpeechVolumeDrop^1"),
        ScStatic<&Game_SetAudioTypeSpeechVolumeDrop>("Game::SetAudioTypeSpeechVolumeDrop^2"),
        ScStatic<&Game_GetAudioTypeVolume>("Game::GetAudioTypeVolume^1"),
        ScStatic<&Game_SetAudioTypeVolume>("Game::SetAudioTypeVolume^3"),
        ScStatic<&Game_StopAudio>("Game::StopAudio^1"),
        ScStatic<&Game_ChangeTranslation>("Game::ChangeTranslation^1"),
        ScStatic<&Game_DoOnceOnly>("Game::DoOnceOnly^1"),
        ScStatic<&Game_GetColorFromRGB>("Game::GetColorFromRGB^3"),
        ScStatic<&Game_GetColorFromRGBA>("Game::GetColorFromRGBA^4"),
        ScStatic<&Game_GetLocationName>("Game::GetDisplayNameAt^3"),
        ScStatic<&Game_GetFrameCountForLoop>("Game::GetFrameCountForLoop^2"),
        // [OBSOLETE] GetLocationName => GetDisplayNameAt
        ScStatic<&Game_GetLocationName2>("Game::GetLocationName^2"),
        ScStatic<&Game_GetLocationName>("Game::GetLocationName^3"),
        ScStatic<&Game_GetLoopCountForView>("Game::GetLoopCountForView^1"),
        ScStatic<&Game_GetRunNextSettingForLoop>("Game::GetRunNextSettingForLoop^2"),
        ScStatic<&Game_GetSaveSlotDescription>("Game::GetSaveSlotDescription^1"),
        ScStatic<&Game_GetSaveSlotTime>("Game::GetSaveSlotTime^1"),
        ScStatic<&Game_GetViewFrame>("Game::GetViewFrame^3"),
        ScStatic<&Game_InputBox>("Game::InputBox^1"),
        ScStatic<&Game_SetSaveGameDirectory>("Game::SetSaveGameDirectory^1"),
        ScStatic<&pl_is_plugin_loaded>("Game::IsPluginLoaded"),
        ScStatic<&Game_ChangeSpeechVox>("Game::ChangeSpeechVox"),
        ScStatic<&Game_Pause>("Game::Pause"),
        ScStatic<&Game_PlayVoiceClip>("Game::PlayVoiceClip"),
        ScStatic<&Game_PlayVoiceClipAsType>("Game::PlayVoiceClipAsType"),
        ScStatic<&Game_PrecacheSprite>("Game::PrecacheSprite"),
        ScStatic<&Game_PrecacheView>("Game::PrecacheView"),
        ScStatic<&Game_ResetDoOnceOnly>("Game::ResetDoOnceOnly"),
        ScStatic<&Game_Resume>("Game::Resume"),
        ScStatic<&Game_SimulateKeyPressOld>("Game::SimulateKeyPress^1"),
        ScStatic<&Game_SimulateKeyPress>("Game::SimulateKeyPress^2"),
        ScStatic<&Game_GetSaveSlots, ScManagedBy<globalDynamicArray>>("Game::GetSaveSlots^4"),
        ScStatic<&Game_ScanSaveSlots>("Game::ScanSaveSlots^6"),
        ScStatic<&Game_GetAudioClipCount>("Game::get_AudioClipCount"),
        ScStatic<&Game_GetAudioClip>("Game::geti_AudioClips"),
        ScStatic<&Game_BlockingWaitCounter>("Game::get_BlockingWaitCounter"),
        ScStatic<&Game_BlockingWaitSkipped>("Game::get_BlockingWaitSkipped"),
        ScStatic<&Game_BlockingWaitSkipType>("Game::get_BlockingWaitSkipType"),
        ScStatic<&Game_GetCamera>("Game::get_Camera"),
        ScStatic<&Game_GetCameraCount>("Game::get_CameraCount"),
        ScStatic<&Game_GetAnyCamera>("Game::geti_Cameras"),
        ScStatic<&Game_GetCharacterCount>("Game::get_CharacterCount"),
        ScStatic<&Game_GetDialogCount>("Game::get_DialogCount"),
        ScStatic<&Game_GetFileName>("Game::get_FileName"),
        ScStatic<&Game_GetFontCount>("Game::get_FontCount"),
        ScStatic<&Game_GetGUICount>("Game::get_GUICount"),
        ScStatic<&Game_GetIgnoreUserInputAfterTextTimeoutMs>("Game::get_IgnoreUserInputAfterTextTimeoutMs"),
        ScStatic<&Game_SetIgnoreUserInputAfterTextTimeoutMs>("Game::set_IgnoreUserInputAfterTextTimeoutMs"),
        ScStatic<&Game_InBlockingAction, ScBoolResult>("Game::get_InBlockingAction"),
        ScStatic<&Game_InBlockingWait, ScBoolResult>("Game::get_InBlockingWait"),
        ScStatic<&Game_GetInSkippableCutscene>("Game::get_InSkippableCutscene"),
        ScStatic<&Game_GetInventoryCursorHotspotGraphic>("Game::get_InventoryCursorHotspotGraphic"),
        ScStatic<&Game_SetInventoryCursorHotspotGraphic>("Game::set_InventoryCursorHotspotGraphic"),
        ScStatic<&Game_GetInventoryCursorHotspotColor>("Game::get_InventoryCursorHotspotColor"),
        ScStatic<&Game_SetInventoryCursorHotspotColor>("Game::set_InventoryCursorHotspotColor"),
        ScStatic<&Game_GetInventoryCursorHotspotCrossColor>("Game::get_InventoryCursorHotspotCrossColor"),
        ScStatic<&Game_SetInventoryCursorHotspotCrossColor>("Game::set_InventoryCursorHotspotCrossColor"),
        ScStatic<&Game_GetInventoryItemCount>("Game::get_InventoryItemCount"),
        ScStatic<&Game_GetIsPaused>("Game::get_IsPaused"),
        ScStatic<&Game_GetIsSpriteDynamic>("Game::geti_IsSpriteDynamic"),
        ScStatic<&Game_GetMinimumTextDisplayTimeMs>("Game::get_MinimumTextDisplayTimeMs"),
        ScStatic<&Game_SetMinimumTextDisplayTimeMs>("Game::set_MinimumTextDisplayTimeMs"),
        ScStatic<&Game_GetMouseCursorCount>("Game::get_MouseCursorCount"),
        ScStatic<&Game_GetName>("Game::get_Name"),
        ScStatic<&Game_SetName>("Game::set_Name"),
        ScStatic<&Game_GetNormalFont>("Game::get_NormalFont"),
        ScStatic<&Game_SetNormalFont>("Game::set_NormalFont"),
        ScStatic<&Game_GetSkippingCutscene>("Game::get_SkippingCutscene"),
        ScStatic<&Game_GetSpeechFont>("Game::get_SpeechFont"),
        ScStatic<&Game_SetSpeechFont>("Game::set_SpeechFont"),
        ScStatic<&Game_GetSpeechVoxFilename>("Game::get_SpeechVoxFilename"),
        ScStatic<&Game_GetSpeed>("Game::get_Speed"),
        ScStatic<&Game_SetSpeed>("Game::set_Speed"),
        ScStatic<&Game_GetSpriteWidth>("Game::geti_SpriteWidth"),
        ScStatic<&Game_GetSpriteHeight>("Game::geti_SpriteHeight"),
        ScStatic<&Game_GetSpriteDepth>("Game::geti_SpriteColorDepth"),
        ScStatic<&Game_GetTextReadingSpeed>("Game::get_TextReadingSpeed"),
        ScStatic<&Game_SetTextReadingSpeed>("Game::set_TextReadingSpeed"),
        ScStatic<&Game_GetTickCounter>("Game::get_TickCounter"),
        ScStatic<&Game_GetTopSpriteNumber>("Game::get_TopSpriteNumber"),
        ScStatic<&Game_GetTranslationFilename>("Game::get_TranslationFilename"),
        ScStatic<&Game_GetUseActiveInventoryGraphicForCursor>("Game::get_UseActiveInventoryGraphicForCursor"),
        ScStatic<&Game_SetUseActiveInventoryGraphicForCursor>("Game::set_UseActiveInventoryGraphicForCursor"),
        ScStatic<&Game_GetViewCount>("Game::get_ViewCount"),

        ScStatic<&GameInfo_GetTitle>("GameInfo::get_Title"),
        ScStatic<&GameInfo_GetDescription>("GameInfo::get_Description"),
        ScStatic<&GameInfo_GetDeveloperName>("GameInfo::get_DeveloperName"),
        ScStatic<&GameInfo_GetDeveloperURL>("GameInfo::get_DeveloperURL"),
        ScStatic<&GameInfo_GetGenre>("GameInfo::get_Genre"),
        ScStatic<&GameInfo_GetReleaseDate>("GameInfo::get_ReleaseDate"),
        ScStatic<&GameInfo_GetVersion>("GameInfo::get_Version"),
        ScStatic<&Game_GetFaceDirectionRatio>("Game::get_FaceDirectionRatio"),
        ScStatic<&Game_SetFaceDirectionRatio>("Game::set_FaceDirectionRatio"),
        ScStatic<&Game_GetRoomCount>("Game::get_RoomCount"),
        ScStatic<&Game_GetRoomNumber>("Game::geti_RoomNumbers"),
        ScStatic<&Game_GetRoomName>("Game::geti_RoomNames"),
    };

    ccAddExternalFunctions(game_api);
}

void RegisterStaticObjects()
{
    ccAddExternalScriptObject("game", &play, &GameStaticManager);
    ccAddExternalScriptObject("mouse", &scmouse, &scmouse);
    ccAddExternalScriptObject("palette", &palette[0], &GlobalStaticManager);
}
