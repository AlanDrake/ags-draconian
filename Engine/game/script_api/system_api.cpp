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
// Script bindings registered by RegisterSystemAPI().
//
//=============================================================================
#include <SDL.h>

#include "ac/dynobj/cc_audiochannel.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "ac/system.h"
#include "debug/out.h"
#include "main/config.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// int ()
RuntimeScriptValue Sc_System_GetAudioChannelCount(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetAudioChannelCount);
}

// ScriptAudioChannel* (int index)
RuntimeScriptValue Sc_System_GetAudioChannels(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT(ScriptAudioChannel, ccDynamicAudio, System_GetAudioChannels);
}

// int ()
RuntimeScriptValue Sc_System_GetCapsLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetCapsLock);
}

// int ()
RuntimeScriptValue Sc_System_GetColorDepth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetColorDepth);
}

RuntimeScriptValue Sc_System_GetFPS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetFPS);
}

// int ()
RuntimeScriptValue Sc_System_GetGamma(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetGamma);
}

// void (int newValue)
RuntimeScriptValue Sc_System_SetGamma(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetGamma);
}

// int ()
RuntimeScriptValue Sc_System_GetHardwareAcceleration(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetHardwareAcceleration);
}

RuntimeScriptValue Sc_System_GetHasInputFocus(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(System_GetHasInputFocus);
}

RuntimeScriptValue Sc_System_GetLocaleName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, System_GetLocaleName);
}

// int ()
RuntimeScriptValue Sc_System_GetNumLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetNumLock);
}

// int ()
RuntimeScriptValue Sc_System_GetOS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetOS);
}

// int ()
RuntimeScriptValue Sc_System_GetScrollLock(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetScrollLock);
}

// int ()
RuntimeScriptValue Sc_System_GetSupportsGammaControl(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetSupportsGammaControl);
}

// const char *()
RuntimeScriptValue Sc_System_GetVersion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, System_GetVersion);
}

// int ()
RuntimeScriptValue Sc_System_GetVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetVolume);
}

// void (int newvol)
RuntimeScriptValue Sc_System_SetVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetVolume);
}

// int ()
RuntimeScriptValue Sc_System_GetVsync(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetVsync);
}

// void (int newValue)
RuntimeScriptValue Sc_System_SetVsync(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetVsync);
}

RuntimeScriptValue Sc_System_GetWindowed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetWindowed);
}

RuntimeScriptValue Sc_System_SetWindowed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetWindowed);
}

// const char *()
RuntimeScriptValue Sc_System_GetRuntimeInfo(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ(const char, myScriptStringImpl, System_GetRuntimeInfo);
}

RuntimeScriptValue Sc_System_GetRenderAtScreenResolution(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(System_GetRenderAtScreenResolution);
}

RuntimeScriptValue Sc_System_SetRenderAtScreenResolution(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(System_SetRenderAtScreenResolution);
}

RuntimeScriptValue Sc_System_SaveConfigToFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(save_runtime_config_file);
}

RuntimeScriptValue Sc_System_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF_PURE(Sc_System_Log, 2);
    Debug::Printf(kDbgGroup_Script, (MessageType)params[0].IValue, "%s", scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_System_GetEngineInteger(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(System_GetEngineInteger);
}

RuntimeScriptValue Sc_System_GetEngineString(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(const char, myScriptStringImpl, System_GetEngineString);
}

RuntimeScriptValue Sc_System_GetDisplayFPS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_BOOL(System_GetDisplayFPS);
}

RuntimeScriptValue Sc_System_SetDisplayFPS(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PBOOL(System_SetDisplayFPS);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_System_Log(int message_type, const char *format, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF_PURE(format);
    Debug::Printf(kDbgGroup_Script, (MessageType)message_type, "%s", scsf_buffer);
}


void RegisterSystemAPI()
{
    ScFnRegister system_api[] = {
        {"System::get_AudioChannelCount", API_FN_PAIR(System_GetAudioChannelCount)},
        {"System::geti_AudioChannels", API_FN_PAIR(System_GetAudioChannels)},
        {"System::get_CapsLock", API_FN_PAIR(System_GetCapsLock)},
        {"System::get_ColorDepth", API_FN_PAIR(System_GetColorDepth)},
        {"System::get_FPS", API_FN_PAIR(System_GetFPS)},
        {"System::get_Gamma", API_FN_PAIR(System_GetGamma)},
        {"System::set_Gamma", API_FN_PAIR(System_SetGamma)},
        {"System::get_HardwareAcceleration", API_FN_PAIR(System_GetHardwareAcceleration)},
        {"System::get_HasInputFocus", API_FN_PAIR(System_GetHasInputFocus)},
        {"System::get_LocaleName", API_FN_PAIR(System_GetLocaleName)},
        {"System::get_NumLock", API_FN_PAIR(System_GetNumLock)},
        {"System::get_OperatingSystem", API_FN_PAIR(System_GetOS)},
        {"System::get_RenderAtScreenResolution", API_FN_PAIR(System_GetRenderAtScreenResolution)},
        {"System::set_RenderAtScreenResolution", API_FN_PAIR(System_SetRenderAtScreenResolution)},
        {"System::get_RuntimeInfo", API_FN_PAIR(System_GetRuntimeInfo)},
        {"System::get_ScrollLock", API_FN_PAIR(System_GetScrollLock)},
        {"System::get_SupportsGammaControl", API_FN_PAIR(System_GetSupportsGammaControl)},
        {"System::get_Version", API_FN_PAIR(System_GetVersion)},
        {"SystemInfo::get_Version", API_FN_PAIR(System_GetVersion)},
        {"System::get_Volume", API_FN_PAIR(System_GetVolume)},
        {"System::set_Volume", API_FN_PAIR(System_SetVolume)},
        {"System::get_VSync", API_FN_PAIR(System_GetVsync)},
        {"System::set_VSync", API_FN_PAIR(System_SetVsync)},
        {"System::get_Windowed", API_FN_PAIR(System_GetWindowed)},
        {"System::set_Windowed", API_FN_PAIR(System_SetWindowed)},

        {"System::SaveConfigToFile", Sc_System_SaveConfigToFile, save_runtime_config_file},
        {"System::Log^102", Sc_System_Log, ScPl_System_Log},
        {"System::GetEngineInteger^2", API_FN_PAIR(System_GetEngineInteger)},
        {"System::GetEngineString^2", API_FN_PAIR(System_GetEngineString)},
        {"System::get_DisplayFPS", API_FN_PAIR(System_GetDisplayFPS)},
        {"System::set_DisplayFPS", API_FN_PAIR(System_SetDisplayFPS)},
    };

    ccAddExternalFunctions(system_api);
}
