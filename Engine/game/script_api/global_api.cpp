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
// Script API Functions
//
//=============================================================================
#include "debug/out.h"
#include "game/audio/audio.h"
#include "game/dialog/dialog.h"
#include "game/dialog/display.h"
#include "game/dialog/parser.h"
#include "game/dialog/speech.h"
#include "game/event.h"
#include "game/game.h"
#include "game/game_struct_defines.h"
#include "game/input/keyboard.h"
#include "game/lib/math.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/render/dynamic_sprite.h"
#include "game/render/screen.h"
#include "game/script_api/bindings.h"
#include "game/system.h"
#include "game/translation.h"
#include "game/ui/gui.h"
#include "game/ui/mouse.h"
#include "game/video/video.h"
#include "game/video/video_script.h"
#include "game/world/character.h"
#include "game/world/object.h"
#include "game/world/region.h"
#include "game/world/room.h"
#include "game/world/walk_behind.h"
#include "game/world/walkable_area.h"
#include "media/audio/audio_system.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/dynobj/dynobj_manager.h"
#include "script/runtimescript.h"
#include "script/script_api.h"
#include "script/script_runtime.h"
#include "util/string_compat.h"

using namespace AGS::Engine;

const char *Anything_GetTypeName(void *obj)
{
    auto *mgr = ccGetObjectManager(obj);
    if (!mgr)
        return CreateNewScriptString("");

    const char *name = nullptr;
    uint32_t obj_tid = mgr->GetTypeID(obj);
    if (obj_tid != UINT32_MAX)
    {
        const auto &types = RuntimeScript::GetJointRTTI()->GetTypes();
        if (obj_tid < types.size())
            name = types[obj_tid].name;
    }
    if (!name)
    {
        name = mgr->GetType();
    }
    return CreateNewScriptString(name);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// void (char*format, ...)
void ScPl_sc_AbortGame(const char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    _sc_AbortGame(scsf_buffer);
}

void ScPl_Display(char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    DisplaySimple(scsf_buffer);
}

void ScPl_DisplayAt(int x, int y, int width, char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    DisplayAt(x, y, width, scsf_buffer);
}

void ScPl_DisplayAtY(int ypos, char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    DisplayAtY(ypos, scsf_buffer);
}

// void (int ypos, int text_color, int back_color, char *title, char*format, ...)
void ScPl_DisplayTopBar(int ypos, int text_color, int back_color, char *title, char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(format);
    DisplayTopBar(ypos, text_color, back_color, title, scsf_buffer);
}


void RegisterGlobalAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    // NOTE: historically, AGS script compiler did not add number of argument appendix
    // to the global functions, for unknown reasons. If we add such appendix now,
    // we still have to keep an old registry entry for old games to link properly.
    ScFnRegister global_api[] = {
        ScStaticFormat<&_sc_AbortGame>("AbortGame", ScPl_sc_AbortGame),
        ScStatic<&AreThingsOverlapping>("AreThingsOverlapping"),
        ScStatic<&CallRoomScript>("CallRoomScript"),
        ScStatic<&ClaimEvent>("ClaimEvent"),
        ScStatic<&CopySaveSlot>("CopySaveSlot"),
        ScStatic<&CyclePalette>("CyclePalette"),
        ScStatic<&script_debug>("Debug"),
        ScStatic<&DeleteSaveSlot>("DeleteSaveSlot"),
        ScStatic<&DisableGroundLevelAreas>("DisableGroundLevelAreas"),
        ScStatic<&DisableInterface>("DisableInterface"),
        ScStaticFormat<&DisplaySimple>("Display", ScPl_Display),
        ScStaticFormat<&DisplayAt>("DisplayAt", ScPl_DisplayAt),
        // This function was non-variadic prior to 3.6.1, and the script compiler gives non-member
        // functions no "name^argnum" symbol id, so one entry serves both: a call from an older
        // script passes the two fixed parameters, which its binding accepts (param_count >= 2).
        ScStaticFormat<&DisplayAtY>("DisplayAtY", ScPl_DisplayAtY),
        ScStaticFormat<&DisplayTopBar>("DisplayTopBar", ScPl_DisplayTopBar),
        ScStatic<&EnableGroundLevelAreas>("EnableGroundLevelAreas"),
        ScStatic<&EnableInterface>("EnableInterface"),
        ScStatic<&EndCutscene>("EndCutscene"),
        ScStatic<&FadeIn>("FadeIn"),
        ScStatic<&FadeOut>("FadeOut"),
        ScStatic<&FindGUIID>("FindGUIID"),
        ScStatic<&FlipScreen>("FlipScreen"),
        ScStatic<&FloatToInt>("FloatToInt"),
        ScStatic<&GetBackgroundFrame>("GetBackgroundFrame"),
        ScStatic<&GetGameOption>("GetGameOption"),
        ScStatic<&GetGameSpeed>("GetGameSpeed"),
        ScStatic<&GetLocationType>("GetLocationType^3"),
        ScStatic<&GetLocationType2>("GetLocationType^2"),
        ScStatic<&GetLocationType2>("GetLocationType"),
        ScStatic<&GetScalingAt>("GetScalingAt"),
        ScStatic<&GetTextHeight>("GetTextHeight"),
        ScStatic<&GetTextWidth>("GetTextWidth"),
        ScStatic<&GetFontHeight>("GetFontHeight"),
        ScStatic<&GetFontLineSpacing>("GetFontLineSpacing"),
        ScStatic<&GetPressedKeys, ScManagedBy<globalDynamicArray>>("GetPressedKeys"),
        ScStatic<&GetTimerPos>("GetTimerPos"),
        ScStatic<&get_translation>("GetTranslation"),
        ScStatic<&GetWalkableAreaAtRoom>("GetWalkableAreaAtRoom"),
        ScStatic<&GetWalkableAreaAtScreen>("GetWalkableAreaAt"),
        ScStatic<&GetWalkableAreaAtScreen>("GetWalkableAreaAtScreen"),
        ScStatic<&GetWalkBehindBase>("GetWalkBehindBase"),
        ScStatic<&GetDrawingSurfaceForWalkableArea>("GetDrawingSurfaceForWalkableArea"),
        ScStatic<&GetDrawingSurfaceForWalkbehind>("GetDrawingSurfaceForWalkbehind"),
        ScStatic<&HasPlayerBeenInRoom>("HasPlayerBeenInRoom"),
        ScStatic<&ShowInputBox>("InputBox"),
        ScStatic<&InterfaceOff>("InterfaceOff"),
        ScStatic<&InterfaceOn>("InterfaceOn"),
        ScStatic<&IntToFloat>("IntToFloat"),
        ScStatic<&IsGamePaused>("IsGamePaused"),
        ScStatic<&IsInteractionAvailable>("IsInteractionAvailable"),
        ScStatic<&IsInterfaceEnabled>("IsInterfaceEnabled"),
        ScStatic<&IsKeyPressed>("IsKeyPressed"),
        ScStatic<&IsAnyKeyPressed>("IsAnyKeyPressed"),
        ScStatic<&IsMusicVoxAvailable>("IsMusicVoxAvailable"),
        ScStatic<&IsSpeechVoxAvailable>("IsSpeechVoxAvailable"),
        ScStatic<&IsTimerExpired>("IsTimerExpired"),
        ScStatic<&IsTranslationAvailable>("IsTranslationAvailable"),
        ScStatic<&MoveSaveSlot>("MoveSaveSlot"),
        ScStatic<&PauseGame>("PauseGame"),
        ScStatic<&PlayFlic>("PlayFlic"),
        ScStatic<&PlayVideo>("PlayVideo"),
        ScStatic<&QuitGame>("QuitGame"),
        ScStatic<&Random>("Random"),
        ScStatic<&RemoveWalkableArea>("RemoveWalkableArea"),
        ScStatic<&ResetRoom>("ResetRoom"),
        ScStatic<&restart_game>("RestartGame"),
        ScStatic<&RestoreGameSlot>("RestoreGameSlot"),
        ScStatic<&RestoreWalkableArea>("RestoreWalkableArea"),
        ScStatic<&RunAGSGame>("RunAGSGame"),
        ScStatic<&SaveScreenShot1>("SaveScreenShot"),
        ScStatic<&SaveScreenShot1>("SaveScreenShot^1"),
        ScStatic<&SaveScreenShot4>("SaveScreenShot^4"),
        ScStatic<&run_on_event>("SendEvent"),
        ScStatic<&SetAmbientTint>("SetAmbientTint"),
        ScStatic<&SetAmbientLightLevel>("SetAmbientLightLevel"),
        ScStatic<&SetAreaScaling>("SetAreaScaling"),
        ScStatic<&SetBackgroundFrame>("SetBackgroundFrame"),
        ScStatic<&SetFadeColor>("SetFadeColor"),
        ScStatic<&SetGameOption>("SetGameOption"),
        ScStatic<&SetGameSpeed>("SetGameSpeed"),
        ScStatic<&SetMultitasking>("SetMultitaskingMode"),
        ScStatic<&SetNextScreenTransition>("SetNextScreenTransition"),
        ScStatic<&SetPalRGB>("SetPalRGB"),
        ScStatic<&SetRestartPoint>("SetRestartPoint"),
        ScStatic<&SetScreenTransition>("SetScreenTransition"),
        ScStatic<&SetSpeechVolume>("SetSpeechVolume"),
        ScStatic<&SetTextWindowGUI>("SetTextWindowGUI"),
        ScStatic<&SetTimer>("SetTimer"),
        ScStatic<&SetWalkBehindBase>("SetWalkBehindBase"),
        ScStatic<&ShakeScreen>("ShakeScreen"),
        ScStatic<&ShakeScreenBackground>("ShakeScreenBackground"),
        ScStatic<&SkipCutscene>("SkipCutscene"),
        ScStatic<&SkipUntilCharacterStops>("SkipUntilCharacterStops"),
        ScStatic<&StartCutscene>("StartCutscene"),
        ScStatic<&StopDialog>("StopDialog"),
        ScStatic<&TintScreen>("TintScreen"),
        ScStatic<&UnPauseGame>("UnPauseGame"),
        ScStatic<&UpdateInventory>("UpdateInventory"),
        ScStatic<&UpdatePalette>("UpdatePalette"),
        ScStatic<&scrWait>("Wait"),
        ScStatic<&WaitKey>("WaitKey"),
        ScStatic<&WaitMouse>("WaitMouse"),
        ScStatic<&WaitMouseKey>("WaitMouseKey"),
        ScStatic<&WaitInput>("WaitInput"),
        ScStatic<&SkipWait0>("SkipWait"),
        ScStatic<&SkipWait0>("SkipWait^0"),
        ScStatic<&SkipWait>("SkipWait^1"),
    };

    ccAddExternalFunctions(global_api);

    // Few functions have to be selected based on API level,
    // We have to do this because AGS compiler did not generate
    // "name^argnum" symbol id for non-member functions for some reason....
    if (base_api < kScriptAPI_v362)
    {
        ScFnRegister global_api_dlgs[] = {
            ScStatic<&SaveGameSlot2>("SaveGameSlot"),
            ScStatic<&restore_game_dialog>("RestoreGameDialog"),
            ScStatic<&save_game_dialog>("SaveGameDialog"),
        };
        ccAddExternalFunctions(global_api_dlgs);
    }
    else
    {
        ScFnRegister global_api_dlgs[] = {
            ScStatic<&SaveGameSlot>("SaveGameSlot"),
            ScStatic<&restore_game_dialog2>("RestoreGameDialog"),
            ScStatic<&save_game_dialog2>("SaveGameDialog"),
        };
        ccAddExternalFunctions(global_api_dlgs);
    }

    // Ultimate base managed class
    // TODO: it is registered here only to avoid a separate .cpp for this one function;
    // move it if a better place appears.
    ScFnRegister anything_api[] = {
        ScMethod<&Anything_GetTypeName>("Anything::get_TypeName"),
    };

    ccAddExternalFunctions(anything_api);
}
