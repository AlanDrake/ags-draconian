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
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/scriptrestoredsaveinfo.h"
#include "game/save/savegame.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_SaveInfo_GetCancel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_GetCancel);
}

RuntimeScriptValue Sc_SaveInfo_SetCancel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptRestoredSaveInfo, SaveInfo_SetCancel);
}

RuntimeScriptValue Sc_SaveInfo_GetRetryWithoutComponents(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetRetryWithoutComponents);
}

RuntimeScriptValue Sc_SaveInfo_SetRetryWithoutComponents(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptRestoredSaveInfo, SaveInfo_SetRetryWithoutComponents);
}

RuntimeScriptValue Sc_SaveInfo_HasExtraData(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_HasExtraData);
}

RuntimeScriptValue Sc_SaveInfo_HasMissingData(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_HasMissingData);
}

RuntimeScriptValue Sc_SaveInfo_IsPrescan(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptRestoredSaveInfo, SaveInfo_IsPrescan);
}

RuntimeScriptValue Sc_SaveInfo_GetSlot(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetSlot);
}

RuntimeScriptValue Sc_SaveInfo_GetDescription(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetDescription);
}

RuntimeScriptValue Sc_SaveInfo_GetEngineVersion(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetEngineVersion);
}

RuntimeScriptValue Sc_SaveInfo_GetGameVersion(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetGameVersion);
}

RuntimeScriptValue Sc_SaveInfo_GetAudioClipTypeCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetAudioClipTypeCount);
}

RuntimeScriptValue Sc_SaveInfo_GetCharacterCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetCharacterCount);
}

RuntimeScriptValue Sc_SaveInfo_GetDialogCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetDialogCount);
}

RuntimeScriptValue Sc_SaveInfo_GetGUICount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetGUICount);
}

RuntimeScriptValue Sc_SaveInfo_GetGUIControlCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetGUIControlCount);
}

RuntimeScriptValue Sc_SaveInfo_GetInventoryItemCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetInventoryItemCount);
}

RuntimeScriptValue Sc_SaveInfo_GetCursorCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetCursorCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetViewCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewLoopCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetViewLoopCount);
}

RuntimeScriptValue Sc_SaveInfo_GetViewFrameCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetViewFrameCount);
}

RuntimeScriptValue Sc_SaveInfo_GetGlobalScriptDataSize(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetGlobalScriptDataSize);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetScriptModuleCount);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleNames(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(ScriptRestoredSaveInfo, const char, myScriptStringImpl, SaveInfo_GetScriptModuleNames);
}

RuntimeScriptValue Sc_SaveInfo_GetScriptModuleDataSizes(void *self, const RuntimeScriptValue *params,
                                                        int32_t param_count)
{
    API_OBJCALL_INT_PINT(ScriptRestoredSaveInfo, SaveInfo_GetScriptModuleDataSizes);
}

RuntimeScriptValue Sc_SaveInfo_GetRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptRestoredSaveInfo, SaveInfo_GetRoom);
}

void RegisterSaveInfoAPI()
{
    ScFnRegister saveinfo_api[] = {
        {"RestoredSaveInfo::get_Cancel", API_FN_PAIR(SaveInfo_GetCancel)},
        {"RestoredSaveInfo::set_Cancel", API_FN_PAIR(SaveInfo_SetCancel)},
        {"RestoredSaveInfo::get_RetryWithoutComponents", API_FN_PAIR(SaveInfo_GetRetryWithoutComponents)},
        {"RestoredSaveInfo::set_RetryWithoutComponents", API_FN_PAIR(SaveInfo_SetRetryWithoutComponents)},
        {"RestoredSaveInfo::get_IsPrescan", API_FN_PAIR(SaveInfo_IsPrescan)},
        {"RestoredSaveInfo::get_HasExtraData", API_FN_PAIR(SaveInfo_HasExtraData)},
        {"RestoredSaveInfo::get_HasMissingData", API_FN_PAIR(SaveInfo_HasMissingData)},
        {"RestoredSaveInfo::get_Slot", API_FN_PAIR(SaveInfo_GetSlot)},
        {"RestoredSaveInfo::get_Description", API_FN_PAIR(SaveInfo_GetDescription)},
        {"RestoredSaveInfo::get_EngineVersion", API_FN_PAIR(SaveInfo_GetEngineVersion)},
        {"RestoredSaveInfo::get_GameVersion", API_FN_PAIR(SaveInfo_GetGameVersion)},
        {"RestoredSaveInfo::get_AudioClipTypeCount", API_FN_PAIR(SaveInfo_GetAudioClipTypeCount)},
        {"RestoredSaveInfo::get_CharacterCount", API_FN_PAIR(SaveInfo_GetCharacterCount)},
        {"RestoredSaveInfo::get_DialogCount", API_FN_PAIR(SaveInfo_GetDialogCount)},
        {"RestoredSaveInfo::get_GUICount", API_FN_PAIR(SaveInfo_GetGUICount)},
        {"RestoredSaveInfo::geti_GUIControlCount", API_FN_PAIR(SaveInfo_GetGUIControlCount)},
        {"RestoredSaveInfo::get_InventoryItemCount", API_FN_PAIR(SaveInfo_GetInventoryItemCount)},
        {"RestoredSaveInfo::get_CursorCount", API_FN_PAIR(SaveInfo_GetCursorCount)},
        {"RestoredSaveInfo::get_ViewCount", API_FN_PAIR(SaveInfo_GetViewCount)},
        {"RestoredSaveInfo::geti_ViewLoopCount", API_FN_PAIR(SaveInfo_GetViewLoopCount)},
        {"RestoredSaveInfo::geti_ViewFrameCount", API_FN_PAIR(SaveInfo_GetViewFrameCount)},
        {"RestoredSaveInfo::get_GlobalScriptDataSize", API_FN_PAIR(SaveInfo_GetGlobalScriptDataSize)},
        {"RestoredSaveInfo::get_ScriptModuleCount", API_FN_PAIR(SaveInfo_GetScriptModuleCount)},
        {"RestoredSaveInfo::geti_ScriptModuleNames", API_FN_PAIR(SaveInfo_GetScriptModuleNames)},
        {"RestoredSaveInfo::geti_ScriptModuleDataSizes", API_FN_PAIR(SaveInfo_GetScriptModuleDataSizes)},
        {"RestoredSaveInfo::get_Room", API_FN_PAIR(SaveInfo_GetRoom)},
    };

    ccAddExternalFunctions(saveinfo_api);
}
