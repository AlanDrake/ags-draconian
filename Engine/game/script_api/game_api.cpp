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
#include "ac/draw.h"
#include "ac/dynobj/all_dynamicclasses.h"
#include "ac/dynobj/all_scriptclasses.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptcamera.h"
#include "ac/dynobj/scriptgame.h"
#include "ac/game.h"
#include "ac/gamestate.h"
#include "game/ui/mouse.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

extern RGB palette[256];

// int  (int audioType);
RuntimeScriptValue Sc_Game_IsAudioPlaying(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_IsAudioPlaying);
}

RuntimeScriptValue Sc_Game_GetAudioTypeCrossfadeSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetAudioTypeCrossfadeSpeed);
}

RuntimeScriptValue Sc_Game_SetAudioTypeCrossfadeSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Game_SetAudioTypeCrossfadeSpeed);
}

RuntimeScriptValue Sc_Game_GetAudioTypeSpeechVolumeDrop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetAudioTypeSpeechVolumeDrop);
}

RuntimeScriptValue Sc_Game_SetAudioTypeSpeechVolumeDrop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Game_SetAudioTypeSpeechVolumeDrop);
}

RuntimeScriptValue Sc_Game_GetAudioTypeVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetAudioTypeVolume);
}

RuntimeScriptValue Sc_Game_SetAudioTypeVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Game_SetAudioTypeVolume);
}

// void (int audioType)
RuntimeScriptValue Sc_Game_StopAudio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_StopAudio);
}

// int (const char *newFilename)
RuntimeScriptValue Sc_Game_ChangeTranslation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_ChangeTranslation, const char);
}

RuntimeScriptValue Sc_Game_ChangeSpeechVox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ(Game_ChangeSpeechVox, const char);
}

// int (const char *token)
RuntimeScriptValue Sc_Game_DoOnceOnly(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_DoOnceOnly, const char);
}

RuntimeScriptValue Sc_Game_ResetDoOnceOnly(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Game_ResetDoOnceOnly);
}

// int (int red, int grn, int blu)
RuntimeScriptValue Sc_Game_GetColorFromRGB(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(Game_GetColorFromRGB);
}

RuntimeScriptValue Sc_Game_GetColorFromRGBA(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(Game_GetColorFromRGBA);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetFrameCountForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetFrameCountForLoop);
}

RuntimeScriptValue Sc_Game_GetLocationName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(const char, myScriptStringImpl, Game_GetLocationName);
}

RuntimeScriptValue Sc_Game_GetLocationName2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(const char, myScriptStringImpl, Game_GetLocationName2);
}

// int (int viewNumber)
RuntimeScriptValue Sc_Game_GetLoopCountForView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetLoopCountForView);
}

// int (int viewNumber, int loopNumber)
RuntimeScriptValue Sc_Game_GetRunNextSettingForLoop(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(Game_GetRunNextSettingForLoop);
}

// const char* (int slnum)
RuntimeScriptValue Sc_Game_GetSaveSlotDescription(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetSaveSlotDescription);
}

RuntimeScriptValue Sc_Game_GetSaveSlotTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDateTime, Game_GetSaveSlotTime);
}

// ScriptViewFrame* (int viewNumber, int loopNumber, int frame)
RuntimeScriptValue Sc_Game_GetViewFrame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptViewFrame, Game_GetViewFrame);
}

// const char* (const char *msg)
RuntimeScriptValue Sc_Game_InputBox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, Game_InputBox, const char);
}

// int (const char *newFolder)
RuntimeScriptValue Sc_Game_SetSaveGameDirectory(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Game_SetSaveGameDirectory, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetCharacterCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetCharacterCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetDialogCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetDialogCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetFileName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetFileName);
}

// int ()
RuntimeScriptValue Sc_Game_GetFontCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetFontCount);
}

// int ()
RuntimeScriptValue Sc_Game_GetGUICount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetGUICount);
}

// int ()
RuntimeScriptValue Sc_Game_GetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetIgnoreUserInputAfterTextTimeoutMs);
}

// void (int newValueMs)
RuntimeScriptValue Sc_Game_SetIgnoreUserInputAfterTextTimeoutMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetIgnoreUserInputAfterTextTimeoutMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetInSkippableCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInSkippableCutscene);
}

RuntimeScriptValue Sc_Game_GetInventoryCursorHotspotGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryCursorHotspotGraphic);
}

RuntimeScriptValue Sc_Game_SetInventoryCursorHotspotGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetInventoryCursorHotspotGraphic);
}

RuntimeScriptValue Sc_Game_GetInventoryCursorHotspotColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryCursorHotspotColor);
}

RuntimeScriptValue Sc_Game_SetInventoryCursorHotspotColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetInventoryCursorHotspotColor);
}

RuntimeScriptValue Sc_Game_GetInventoryCursorHotspotCrossColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryCursorHotspotCrossColor);
}

RuntimeScriptValue Sc_Game_SetInventoryCursorHotspotCrossColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetInventoryCursorHotspotCrossColor);
}

RuntimeScriptValue Sc_Game_GetInventoryItemCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetInventoryItemCount);
}

RuntimeScriptValue Sc_Game_GetIsPaused(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Game_GetIsPaused);
}

// int ()
RuntimeScriptValue Sc_Game_GetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMinimumTextDisplayTimeMs);
}

// void (int newTextMinTime)
RuntimeScriptValue Sc_Game_SetMinimumTextDisplayTimeMs(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetMinimumTextDisplayTimeMs);
}

// int ()
RuntimeScriptValue Sc_Game_GetMouseCursorCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetMouseCursorCount);
}

// const char *()
RuntimeScriptValue Sc_Game_GetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetName);
}

// void (const char *newName)
RuntimeScriptValue Sc_Game_SetName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(Game_SetName, const char);
}

// int ()
RuntimeScriptValue Sc_Game_GetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetNormalFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_Game_SetNormalFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetNormalFont);
}

// int ()
RuntimeScriptValue Sc_Game_GetSkippingCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSkippingCutscene);
}

// int ()
RuntimeScriptValue Sc_Game_GetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSpeechFont);
}

// void  (int fontnum);
RuntimeScriptValue Sc_Game_SetSpeechFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetSpeechFont);
}

RuntimeScriptValue Sc_Game_GetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetSpeed);
}

RuntimeScriptValue Sc_Game_SetSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetSpeed);
}

RuntimeScriptValue Sc_Game_GetTopSpriteNumber(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetTopSpriteNumber);
}

RuntimeScriptValue Sc_Game_GetSpriteWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteWidth);
}

RuntimeScriptValue Sc_Game_GetSpriteHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteHeight);
}

RuntimeScriptValue Sc_Game_GetSpriteDepth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetSpriteDepth);
}

RuntimeScriptValue Sc_Game_GetIsSpriteDynamic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_PINT(Game_GetIsSpriteDynamic);
}

// int ()
RuntimeScriptValue Sc_Game_GetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetTextReadingSpeed);
}

// void (int newTextSpeed)
RuntimeScriptValue Sc_Game_SetTextReadingSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SetTextReadingSpeed);
}

RuntimeScriptValue Sc_Game_GetTickCounter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetTickCounter);
}

// const char* ()
RuntimeScriptValue Sc_Game_GetTranslationFilename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetTranslationFilename);
}

RuntimeScriptValue Sc_Game_GetSpeechVoxFilename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, Game_GetSpeechVoxFilename);
}

RuntimeScriptValue Sc_Game_GetUseActiveInventoryGraphicForCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Game_GetUseActiveInventoryGraphicForCursor);
}

RuntimeScriptValue Sc_Game_SetUseActiveInventoryGraphicForCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(Game_SetUseActiveInventoryGraphicForCursor);
}

RuntimeScriptValue Sc_Game_GetViewCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetViewCount);
}

RuntimeScriptValue Sc_Game_GetAudioClipCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetAudioClipCount);
}

RuntimeScriptValue Sc_Game_GetAudioClip(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptAudioClip, ccDynamicAudioClip, Game_GetAudioClip);
}

RuntimeScriptValue Sc_Game_IsPluginLoaded(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL_POBJ(pl_is_plugin_loaded, const char);
}

RuntimeScriptValue Sc_Game_Pause(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Game_Pause);
}

RuntimeScriptValue Sc_Game_Resume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(Game_Resume);
}

RuntimeScriptValue Sc_Game_PlayVoiceClip(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ_PINT_PBOOL(ScriptAudioChannel, ccDynamicAudio, Game_PlayVoiceClip, CharacterInfo);
}

RuntimeScriptValue Sc_Game_PlayVoiceClipAsType(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ_PINT5(ScriptAudioChannel, ccDynamicAudio, Game_PlayVoiceClipAsType, CharacterInfo);
}

RuntimeScriptValue Sc_Game_GetCamera(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptCamera, Game_GetCamera);
}

RuntimeScriptValue Sc_Game_GetCameraCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetCameraCount);
}

RuntimeScriptValue Sc_Game_GetAnyCamera(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptCamera, Game_GetAnyCamera);
}

RuntimeScriptValue Sc_Game_SimulateKeyPress(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(Game_SimulateKeyPress);
}

RuntimeScriptValue Sc_Game_SimulateKeyPressOld(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_SimulateKeyPressOld);
}

RuntimeScriptValue Sc_Game_BlockingWaitCounter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_BlockingWaitCounter);
}

RuntimeScriptValue Sc_Game_BlockingWaitSkipped(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_BlockingWaitSkipped);
}

RuntimeScriptValue Sc_Game_BlockingWaitSkipType(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_BlockingWaitSkipType);
}

RuntimeScriptValue Sc_Game_InBlockingAction(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Game_InBlockingAction);
}

RuntimeScriptValue Sc_Game_InBlockingWait(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(Game_InBlockingWait);
}

RuntimeScriptValue Sc_Game_PrecacheSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(Game_PrecacheSprite);
}

RuntimeScriptValue Sc_Game_PrecacheView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(Game_PrecacheView);
}

RuntimeScriptValue Sc_Game_GetFaceDirectionRatio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT(Game_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Game_SetFaceDirectionRatio(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PFLOAT(Game_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Game_GetRoomCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(Game_GetRoomCount);
}

RuntimeScriptValue Sc_Game_GetRoomNumber(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(Game_GetRoomNumber);
}

RuntimeScriptValue Sc_Game_GetRoomName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(const char, myScriptStringImpl, Game_GetRoomName);
}

RuntimeScriptValue Sc_Game_GetSaveSlots(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT4(void, globalDynamicArray, Game_GetSaveSlots);
}

RuntimeScriptValue Sc_Game_ScanSaveSlots(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT5(Game_ScanSaveSlots, void);
}

RuntimeScriptValue Sc_GameInfo_GetTitle(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetTitle);
}

RuntimeScriptValue Sc_GameInfo_GetDescription(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDescription);
}

RuntimeScriptValue Sc_GameInfo_GetDeveloperName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDeveloperName);
}

RuntimeScriptValue Sc_GameInfo_GetDeveloperURL(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetDeveloperURL);
}

RuntimeScriptValue Sc_GameInfo_GetGenre(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetGenre);
}

RuntimeScriptValue Sc_GameInfo_GetReleaseDate(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDateTime, GameInfo_GetReleaseDate);
}

RuntimeScriptValue Sc_GameInfo_GetVersion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, GameInfo_GetVersion);
}


void RegisterGameAPI()
{
    ScFnRegister game_api[] = {
        {"Game::IsAudioPlaying^1", API_FN_PAIR(Game_IsAudioPlaying)},
        {"Game::GetAudioTypeCrossfadeSpeed^1", API_FN_PAIR(Game_GetAudioTypeCrossfadeSpeed)},
        {"Game::SetAudioTypeCrossfadeSpeed^2", API_FN_PAIR(Game_SetAudioTypeCrossfadeSpeed)},
        {"Game::GetAudioTypeSpeechVolumeDrop^1", API_FN_PAIR(Game_GetAudioTypeSpeechVolumeDrop)},
        {"Game::SetAudioTypeSpeechVolumeDrop^2", API_FN_PAIR(Game_SetAudioTypeSpeechVolumeDrop)},
        {"Game::GetAudioTypeVolume^1", API_FN_PAIR(Game_GetAudioTypeVolume)},
        {"Game::SetAudioTypeVolume^3", API_FN_PAIR(Game_SetAudioTypeVolume)},
        {"Game::StopAudio^1", API_FN_PAIR(Game_StopAudio)},
        {"Game::ChangeTranslation^1", API_FN_PAIR(Game_ChangeTranslation)},
        {"Game::DoOnceOnly^1", API_FN_PAIR(Game_DoOnceOnly)},
        {"Game::GetColorFromRGB^3", API_FN_PAIR(Game_GetColorFromRGB)},
        {"Game::GetColorFromRGBA^4", API_FN_PAIR(Game_GetColorFromRGBA)},
        {"Game::GetDisplayNameAt^3", API_FN_PAIR(Game_GetLocationName)},
        {"Game::GetFrameCountForLoop^2", API_FN_PAIR(Game_GetFrameCountForLoop)},
        // [OBSOLETE] GetLocationName => GetDisplayNameAt
        {"Game::GetLocationName^2", API_FN_PAIR(Game_GetLocationName2)},
        {"Game::GetLocationName^3", API_FN_PAIR(Game_GetLocationName)},
        {"Game::GetLoopCountForView^1", API_FN_PAIR(Game_GetLoopCountForView)},
        {"Game::GetRunNextSettingForLoop^2", API_FN_PAIR(Game_GetRunNextSettingForLoop)},
        {"Game::GetSaveSlotDescription^1", API_FN_PAIR(Game_GetSaveSlotDescription)},
        {"Game::GetSaveSlotTime^1", API_FN_PAIR(Game_GetSaveSlotTime)},
        {"Game::GetViewFrame^3", API_FN_PAIR(Game_GetViewFrame)},
        {"Game::InputBox^1", API_FN_PAIR(Game_InputBox)},
        {"Game::SetSaveGameDirectory^1", API_FN_PAIR(Game_SetSaveGameDirectory)},
        {"Game::IsPluginLoaded", Sc_Game_IsPluginLoaded, pl_is_plugin_loaded},
        {"Game::ChangeSpeechVox", API_FN_PAIR(Game_ChangeSpeechVox)},
        {"Game::Pause", API_FN_PAIR(Game_Pause)},
        {"Game::PlayVoiceClip", API_FN_PAIR(Game_PlayVoiceClip)},
        {"Game::PlayVoiceClipAsType", API_FN_PAIR(Game_PlayVoiceClipAsType)},
        {"Game::PrecacheSprite", API_FN_PAIR(Game_PrecacheSprite)},
        {"Game::PrecacheView", API_FN_PAIR(Game_PrecacheView)},
        {"Game::ResetDoOnceOnly", API_FN_PAIR(Game_ResetDoOnceOnly)},
        {"Game::Resume", API_FN_PAIR(Game_Resume)},
        {"Game::SimulateKeyPress^1", API_FN_PAIR(Game_SimulateKeyPressOld)},
        {"Game::SimulateKeyPress^2", API_FN_PAIR(Game_SimulateKeyPress)},
        {"Game::GetSaveSlots^4", API_FN_PAIR(Game_GetSaveSlots)},
        {"Game::ScanSaveSlots^6", API_FN_PAIR(Game_ScanSaveSlots)},
        {"Game::get_AudioClipCount", API_FN_PAIR(Game_GetAudioClipCount)},
        {"Game::geti_AudioClips", API_FN_PAIR(Game_GetAudioClip)},
        {"Game::get_BlockingWaitCounter", API_FN_PAIR(Game_BlockingWaitCounter)},
        {"Game::get_BlockingWaitSkipped", API_FN_PAIR(Game_BlockingWaitSkipped)},
        {"Game::get_BlockingWaitSkipType", API_FN_PAIR(Game_BlockingWaitSkipType)},
        {"Game::get_Camera", API_FN_PAIR(Game_GetCamera)},
        {"Game::get_CameraCount", API_FN_PAIR(Game_GetCameraCount)},
        {"Game::geti_Cameras", API_FN_PAIR(Game_GetAnyCamera)},
        {"Game::get_CharacterCount", API_FN_PAIR(Game_GetCharacterCount)},
        {"Game::get_DialogCount", API_FN_PAIR(Game_GetDialogCount)},
        {"Game::get_FileName", API_FN_PAIR(Game_GetFileName)},
        {"Game::get_FontCount", API_FN_PAIR(Game_GetFontCount)},
        {"Game::get_GUICount", API_FN_PAIR(Game_GetGUICount)},
        {"Game::get_IgnoreUserInputAfterTextTimeoutMs", API_FN_PAIR(Game_GetIgnoreUserInputAfterTextTimeoutMs)},
        {"Game::set_IgnoreUserInputAfterTextTimeoutMs", API_FN_PAIR(Game_SetIgnoreUserInputAfterTextTimeoutMs)},
        {"Game::get_InBlockingAction", API_FN_PAIR(Game_InBlockingAction)},
        {"Game::get_InBlockingWait", API_FN_PAIR(Game_InBlockingWait)},
        {"Game::get_InSkippableCutscene", API_FN_PAIR(Game_GetInSkippableCutscene)},
        {"Game::get_InventoryCursorHotspotGraphic", API_FN_PAIR(Game_GetInventoryCursorHotspotGraphic)},
        {"Game::set_InventoryCursorHotspotGraphic", API_FN_PAIR(Game_SetInventoryCursorHotspotGraphic)},
        {"Game::get_InventoryCursorHotspotColor", API_FN_PAIR(Game_GetInventoryCursorHotspotColor)},
        {"Game::set_InventoryCursorHotspotColor", API_FN_PAIR(Game_SetInventoryCursorHotspotColor)},
        {"Game::get_InventoryCursorHotspotCrossColor", API_FN_PAIR(Game_GetInventoryCursorHotspotCrossColor)},
        {"Game::set_InventoryCursorHotspotCrossColor", API_FN_PAIR(Game_SetInventoryCursorHotspotCrossColor)},
        {"Game::get_InventoryItemCount", API_FN_PAIR(Game_GetInventoryItemCount)},
        {"Game::get_IsPaused", API_FN_PAIR(Game_GetIsPaused)},
        {"Game::geti_IsSpriteDynamic", API_FN_PAIR(Game_GetIsSpriteDynamic)},
        {"Game::get_MinimumTextDisplayTimeMs", API_FN_PAIR(Game_GetMinimumTextDisplayTimeMs)},
        {"Game::set_MinimumTextDisplayTimeMs", API_FN_PAIR(Game_SetMinimumTextDisplayTimeMs)},
        {"Game::get_MouseCursorCount", API_FN_PAIR(Game_GetMouseCursorCount)},
        {"Game::get_Name", API_FN_PAIR(Game_GetName)},
        {"Game::set_Name", API_FN_PAIR(Game_SetName)},
        {"Game::get_NormalFont", API_FN_PAIR(Game_GetNormalFont)},
        {"Game::set_NormalFont", API_FN_PAIR(Game_SetNormalFont)},
        {"Game::get_SkippingCutscene", API_FN_PAIR(Game_GetSkippingCutscene)},
        {"Game::get_SpeechFont", API_FN_PAIR(Game_GetSpeechFont)},
        {"Game::set_SpeechFont", API_FN_PAIR(Game_SetSpeechFont)},
        {"Game::get_SpeechVoxFilename", API_FN_PAIR(Game_GetSpeechVoxFilename)},
        {"Game::get_Speed", API_FN_PAIR(Game_GetSpeed)},
        {"Game::set_Speed", API_FN_PAIR(Game_SetSpeed)},
        {"Game::geti_SpriteWidth", API_FN_PAIR(Game_GetSpriteWidth)},
        {"Game::geti_SpriteHeight", API_FN_PAIR(Game_GetSpriteHeight)},
        {"Game::geti_SpriteColorDepth", API_FN_PAIR(Game_GetSpriteDepth)},
        {"Game::get_TextReadingSpeed", API_FN_PAIR(Game_GetTextReadingSpeed)},
        {"Game::set_TextReadingSpeed", API_FN_PAIR(Game_SetTextReadingSpeed)},
        {"Game::get_TickCounter", API_FN_PAIR(Game_GetTickCounter)},
        {"Game::get_TopSpriteNumber", API_FN_PAIR(Game_GetTopSpriteNumber)},
        {"Game::get_TranslationFilename", API_FN_PAIR(Game_GetTranslationFilename)},
        {"Game::get_UseActiveInventoryGraphicForCursor", API_FN_PAIR(Game_GetUseActiveInventoryGraphicForCursor)},
        {"Game::set_UseActiveInventoryGraphicForCursor", API_FN_PAIR(Game_SetUseActiveInventoryGraphicForCursor)},
        {"Game::get_ViewCount", API_FN_PAIR(Game_GetViewCount)},

        {"GameInfo::get_Title", API_FN_PAIR(GameInfo_GetTitle)},
        {"GameInfo::get_Description", API_FN_PAIR(GameInfo_GetDescription)},
        {"GameInfo::get_DeveloperName", API_FN_PAIR(GameInfo_GetDeveloperName)},
        {"GameInfo::get_DeveloperURL", API_FN_PAIR(GameInfo_GetDeveloperURL)},
        {"GameInfo::get_Genre", API_FN_PAIR(GameInfo_GetGenre)},
        {"GameInfo::get_ReleaseDate", API_FN_PAIR(GameInfo_GetReleaseDate)},
        {"GameInfo::get_Version", API_FN_PAIR(GameInfo_GetVersion)},
        {"Game::get_FaceDirectionRatio", API_FN_PAIR(Game_GetFaceDirectionRatio)},
        {"Game::set_FaceDirectionRatio", API_FN_PAIR(Game_SetFaceDirectionRatio)},
        {"Game::get_RoomCount", API_FN_PAIR(Game_GetRoomCount)},
        {"Game::geti_RoomNumbers", API_FN_PAIR(Game_GetRoomNumber)},
        {"Game::geti_RoomNames", API_FN_PAIR(Game_GetRoomName)},
    };

    ccAddExternalFunctions(game_api);
}

void RegisterStaticObjects()
{
    ccAddExternalScriptObject("game", &play, &GameStaticManager);
    ccAddExternalScriptObject("mouse", &scmouse, &scmouse);
    ccAddExternalScriptObject("palette", &palette[0], &GlobalStaticManager);
}
