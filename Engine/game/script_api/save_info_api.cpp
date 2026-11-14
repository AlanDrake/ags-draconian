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
// Script bindings registered by RegisterSaveInfoAPI().
//
//=============================================================================
#include "game/dynobj/script_restored_save_info.h"
#include "game/save/savegame.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterSaveInfoAPI()
{
    ScFnRegister saveinfo_api[] = {
        ScMethod<&SaveInfo_GetCancel>("RestoredSaveInfo::get_Cancel"),
        ScMethod<&SaveInfo_SetCancel>("RestoredSaveInfo::set_Cancel"),
        ScMethod<&SaveInfo_GetRetryWithoutComponents>("RestoredSaveInfo::get_RetryWithoutComponents"),
        ScMethod<&SaveInfo_SetRetryWithoutComponents>("RestoredSaveInfo::set_RetryWithoutComponents"),
        ScMethod<&SaveInfo_IsPrescan>("RestoredSaveInfo::get_IsPrescan"),
        ScMethod<&SaveInfo_HasExtraData>("RestoredSaveInfo::get_HasExtraData"),
        ScMethod<&SaveInfo_HasMissingData>("RestoredSaveInfo::get_HasMissingData"),
        ScMethod<&SaveInfo_GetSlot>("RestoredSaveInfo::get_Slot"),
        ScMethod<&SaveInfo_GetDescription>("RestoredSaveInfo::get_Description"),
        ScMethod<&SaveInfo_GetEngineVersion>("RestoredSaveInfo::get_EngineVersion"),
        ScMethod<&SaveInfo_GetGameVersion>("RestoredSaveInfo::get_GameVersion"),
        ScMethod<&SaveInfo_GetAudioClipTypeCount>("RestoredSaveInfo::get_AudioClipTypeCount"),
        ScMethod<&SaveInfo_GetCharacterCount>("RestoredSaveInfo::get_CharacterCount"),
        ScMethod<&SaveInfo_GetDialogCount>("RestoredSaveInfo::get_DialogCount"),
        ScMethod<&SaveInfo_GetGUICount>("RestoredSaveInfo::get_GUICount"),
        ScMethod<&SaveInfo_GetGUIControlCount>("RestoredSaveInfo::geti_GUIControlCount"),
        ScMethod<&SaveInfo_GetInventoryItemCount>("RestoredSaveInfo::get_InventoryItemCount"),
        ScMethod<&SaveInfo_GetCursorCount>("RestoredSaveInfo::get_CursorCount"),
        ScMethod<&SaveInfo_GetViewCount>("RestoredSaveInfo::get_ViewCount"),
        ScMethod<&SaveInfo_GetViewLoopCount>("RestoredSaveInfo::geti_ViewLoopCount"),
        ScMethod<&SaveInfo_GetViewFrameCount>("RestoredSaveInfo::geti_ViewFrameCount"),
        ScMethod<&SaveInfo_GetGlobalScriptDataSize>("RestoredSaveInfo::get_GlobalScriptDataSize"),
        ScMethod<&SaveInfo_GetScriptModuleCount>("RestoredSaveInfo::get_ScriptModuleCount"),
        ScMethod<&SaveInfo_GetScriptModuleNames>("RestoredSaveInfo::geti_ScriptModuleNames"),
        ScMethod<&SaveInfo_GetScriptModuleDataSizes>("RestoredSaveInfo::geti_ScriptModuleDataSizes"),
        ScMethod<&SaveInfo_GetRoom>("RestoredSaveInfo::get_Room"),
    };

    ccAddExternalFunctions(saveinfo_api);
}
