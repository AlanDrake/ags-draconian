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
#include <cassert>
#include <vector>

#include <SDL.h>

#include "debug/out.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "game/system.h"
#include "main/config.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// System.Log(type, format, ...) formats its text without translating the format, which the binder's format rows
// (ScStaticFormat) always do: so it is written out by hand
RuntimeScriptValue Sc_System_Log(const RuntimeScriptValue *params, int32_t param_count)
{
    assert((params != nullptr && param_count >= 2) && "Not enough parameters in call to API function");
    std::vector<char> buffer;
    const char *text = ScriptSprintf(buffer, params[1].CStr, params + 2, param_count - 2);
    Debug::Printf(kDbgGroup_Script, static_cast<MessageType>(params[0].IValue), "%s", text);
    return RuntimeScriptValue(int32_t{0});
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
        ScStatic<&System_GetAudioChannelCount>("System::get_AudioChannelCount"),
        ScStatic<&System_GetAudioChannels>("System::geti_AudioChannels"),
        ScStatic<&System_GetCapsLock>("System::get_CapsLock"),
        ScStatic<&System_GetColorDepth>("System::get_ColorDepth"),
        ScStatic<&System_GetFPS>("System::get_FPS"),
        ScStatic<&System_GetGamma>("System::get_Gamma"),
        ScStatic<&System_SetGamma>("System::set_Gamma"),
        ScStatic<&System_GetHardwareAcceleration>("System::get_HardwareAcceleration"),
        ScStatic<&System_GetHasInputFocus>("System::get_HasInputFocus"),
        ScStatic<&System_GetLocaleName>("System::get_LocaleName"),
        ScStatic<&System_GetNumLock>("System::get_NumLock"),
        ScStatic<&System_GetOS>("System::get_OperatingSystem"),
        ScStatic<&System_GetRenderAtScreenResolution>("System::get_RenderAtScreenResolution"),
        ScStatic<&System_SetRenderAtScreenResolution>("System::set_RenderAtScreenResolution"),
        ScStatic<&System_GetRuntimeInfo>("System::get_RuntimeInfo"),
        ScStatic<&System_GetScrollLock>("System::get_ScrollLock"),
        ScStatic<&System_GetSupportsGammaControl>("System::get_SupportsGammaControl"),
        ScStatic<&System_GetVersion>("System::get_Version"),
        ScStatic<&System_GetVersion>("SystemInfo::get_Version"),
        ScStatic<&System_GetVolume>("System::get_Volume"),
        ScStatic<&System_SetVolume>("System::set_Volume"),
        ScStatic<&System_GetVsync>("System::get_VSync"),
        ScStatic<&System_SetVsync>("System::set_VSync"),
        ScStatic<&System_GetWindowed>("System::get_Windowed"),
        ScStatic<&System_SetWindowed>("System::set_Windowed"),

        ScStatic<&save_runtime_config_file>("System::SaveConfigToFile"),
        {"System::Log^102", Sc_System_Log, ScPl_System_Log},
        ScStatic<&System_GetEngineInteger>("System::GetEngineInteger^2"),
        ScStatic<&System_GetEngineString>("System::GetEngineString^2"),
        ScStatic<&System_GetDisplayFPS>("System::get_DisplayFPS"),
        ScStatic<&System_SetDisplayFPS>("System::set_DisplayFPS"),
    };

    ccAddExternalFunctions(system_api);
}
