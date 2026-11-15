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
#include "game/system.h"

#include <SDL.h>

#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "game/audio/audio.h"
#include "game/dynobj/script_system.h"
#include "game/game.h"
#include "game/game_setup_struct.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "game/translation.h"
#include "game/ui/mouse.h"
#include "gfx/gfxfilter.h"
#include "gfx/graphicsdriver.h"
#include "gfx/sprite_cache.h"
#include "main/engine.h"
#include "main/game_run.h"
#include "main/graphics_mode.h"
#include "media/audio/audio_core.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "platform/base/sys_main.h"
#include "util/common.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::Engine;

bool System_GetHasInputFocus()
{
    return !EngineServices::Current().SwitchedAway;
}

int System_GetColorDepth()
{
    return scsystem.coldepth;
}

int System_GetFPS()
{
    float fps = get_real_fps();
    return std::isnan(fps) ? -1 : static_cast<int>(std::round(fps));
}

int System_GetOS()
{
    return scsystem.os;
}

const char *System_GetVersion()
{
    return CreateNewScriptString(EngineServices::Current().Version.LongString);
}

int System_GetHardwareAcceleration()
{
    return EngineServices::Current().GfxDriver->HasAcceleratedTransform() ? 1 : 0;
}

const char *System_GetLocaleName()
{
    return CreateNewScriptString(play.GetTextLocaleName());
}

int System_GetNumLock()
{
    SDL_Keymod mod_state = SDL_GetModState();
    return (mod_state & KMOD_NUM) ? 1 : 0;
}

int System_GetCapsLock()
{
    SDL_Keymod mod_state = SDL_GetModState();
    return (mod_state & KMOD_CAPS) ? 1 : 0;
}

int System_GetScrollLock()
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    return (state[SDL_SCANCODE_SCROLLLOCK]) ? 1 : 0;
}

int System_GetVsync()
{
    return scsystem.vsync;
}

void System_SetVsync(int newValue)
{
    if (EngineServices::Current().GfxDriver->DoesSupportVsyncToggle())
    {
        System_SetVSyncInternal(newValue != 0);
    }
}

void System_SetVSyncInternal(bool vsync)
{
    scsystem.vsync = vsync;
    EngineServices::Current().Config.Display.VSync = vsync;
}

int System_GetWindowed()
{
    return scsystem.windowed;
}

void System_SetWindowed(int windowed)
{
    if (windowed != scsystem.windowed)
        engine_try_switch_windowed_gfxmode();
}

int System_GetSupportsGammaControl()
{
    return EngineServices::Current().GfxDriver->SupportsGammaControl();
}

int System_GetGamma()
{
    return play.gamma_adjustment;
}

void System_SetGamma(int newValue)
{
    EngineServices &services = EngineServices::Current();
    if ((newValue < 0) || (newValue > 200))
        quitprintf("!System.Gamma: value must be between 0-200 (not %d)", newValue);

    if (play.gamma_adjustment != newValue)
    {
        debug_script_log("Gamma control set to %d", newValue);
        play.gamma_adjustment = newValue;

        if (services.GfxDriver->SupportsGammaControl())
            services.GfxDriver->SetGamma(newValue);
    }
}

int System_GetAudioChannelCount()
{
    return game.numGameChannels;
}

ScriptAudioChannel *System_GetAudioChannels(int index)
{
    if ((index < 0) || (index >= game.numGameChannels))
        quitprintf("!System.AudioChannels: invalid sound channel index %d, supported %d - %d", index, 0,
                   game.numGameChannels - 1);

    return &scrAudioChannel[index];
}

int System_GetVolume()
{
    return play.audio_master_volume;
}

void System_SetVolume(int newvol)
{
    if ((newvol < 0) || (newvol > 100))
        quit("!System.Volume: invalid volume - must be from 0-100");

    if (newvol == play.audio_master_volume)
        return;

    play.audio_master_volume = newvol;
    auto newvol_f = static_cast<float>(newvol) / 100.0;
    audio_core_set_master_volume(newvol_f);
}

const char *System_GetRuntimeInfo()
{
    String runtimeInfo = GetRuntimeInfo();

    return CreateNewScriptString(runtimeInfo.GetCStr());
}

int System_GetRenderAtScreenResolution()
{
    return EngineServices::Current().Config.RenderAtScreenRes;
}

void System_SetRenderAtScreenResolution(int enable)
{
    EngineServices::Current().Config.RenderAtScreenRes = enable != 0;
}

int System_GetEngineInteger(int value_id, int index)
{
    int value = 0;
    if (!GetEngineInteger(value, static_cast<EngineValueID>(value_id), index))
        debug_script_error("System.GetEngineInteger: undefined engine value %d (#%d), or not an integer value",
                           value_id, index);
    return value;
}

const char *System_GetEngineString(int value_id, int index)
{
    String value;
    if (!GetEngineString(value, static_cast<EngineValueID>(value_id), index))
        debug_script_error("System.GetEngineString: undefined engine value %d (#%d)", value_id, index);
    return CreateNewScriptString(value.GetCStr());
}

bool GetEngineInteger(int &value, EngineValueID value_id, int index)
{
    switch (value_id)
    {
    case ENGINE_VALUE_I_SPRCACHE_MAXNORMAL: value = static_cast<int>(spriteset.GetMaxCacheSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_NORMAL: value = static_cast<int>(spriteset.GetCacheSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_LOCKED: value = static_cast<int>(spriteset.GetLockedSize() / 1024u); return true;
    case ENGINE_VALUE_I_SPRCACHE_EXTERNAL: value = static_cast<int>(spriteset.GetExternalSize() / 1024u); return true;
    case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: /* fall-through */
    case ENGINE_VALUE_I_TEXCACHE_NORMAL:
    {
        size_t max_txcached, total_txcached, total_txlocked, total_txext;
        texturecache_get_state(max_txcached, total_txcached, total_txlocked, total_txext);
        switch (value_id)
        {
        case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: value = static_cast<int>(max_txcached / 1024u); return true;
        case ENGINE_VALUE_I_TEXCACHE_NORMAL: value = static_cast<int>(total_txcached / 1024u); return true;
        default: return false; // should not happen...
        }
    }
    case ENGINE_VALUE_I_FPS_MAX: value = isTimerFpsMaxed() ? -1 : frames_per_second; return true;
    case ENGINE_VALUE_I_FPS:
    {
        float fps = get_real_fps();
        value = std::isnan(fps) ? -1 : static_cast<int>(std::round(fps));
        return true;
    }
    default: return false;
    }
}

bool GetEngineString(AGS::Common::String &value, EngineValueID value_id, int index)
{
    EngineServices &services = EngineServices::Current();
    switch (value_id)
    {
    case ENGINE_VALUE_SI_VALUENAME: value = GetEngineValueName(static_cast<EngineValueID>(index)); return true;
    case ENGINE_VALUE_S_ENGINE_NAME: value = get_engine_name(); return true;
    case ENGINE_VALUE_S_ENGINE_VERSION: value = services.Version.LongString; return true;
    case ENGINE_VALUE_S_ENGINE_VERSION_FULL: value = get_engine_version_and_build(); return true;
    case ENGINE_VALUE_S_DISPLAY_MODE_STR:
        value = "";
        if (services.GfxDriver)
        {
            DisplayMode mode = services.GfxDriver->GetDisplayMode();
            value = String::FromFormat("%d x %d %d-bit %s", mode.Width, mode.Height, mode.ColorDepth,
                                       (mode.IsWindowed() ? " W" : (mode.IsRealFullscreen() ? " F" : " FD")));
        }
        return true;
    case ENGINE_VALUE_S_GFXRENDERER:
        value = (services.GfxDriver ? services.GfxDriver->GetDriverName() : "");
        return true;
    case ENGINE_VALUE_S_GFXFILTER:
        value = "";
        if (services.GfxDriver)
        {
            auto filter = services.GfxDriver->GetGraphicsFilter();
            if (filter)
                value = filter->GetInfo().Name;
        }
        return true;
    default:
        // Attempt to retrieve an integer value and print that into string
        {
            int ival = 0;
            if (GetEngineInteger(ival, value_id, index))
            {
                value = String::FromFormat("%d", ival);
                return true;
            }
        }
        return false;
    }
}

String GetEngineValueName(EngineValueID value_id)
{
    switch (value_id)
    {
    case ENGINE_VALUE_SI_VALUENAME: return "Engine Value Name";
    case ENGINE_VALUE_S_ENGINE_NAME: return "Engine Name";
    case ENGINE_VALUE_S_ENGINE_VERSION: return "Engine Version String";
    case ENGINE_VALUE_S_ENGINE_VERSION_FULL: return "Engine Version String (Full)";
    case ENGINE_VALUE_S_DISPLAY_MODE_STR: return "Display Mode String";
    case ENGINE_VALUE_S_GFXRENDERER: return "Graphics Renderer Name";
    case ENGINE_VALUE_S_GFXFILTER: return "Graphics Filter Name";
    case ENGINE_VALUE_I_SPRCACHE_MAXNORMAL: return "Sprite cache: normal capacity (KB)";
    case ENGINE_VALUE_I_SPRCACHE_NORMAL: return "Sprite cache: normal size (KB)";
    case ENGINE_VALUE_I_SPRCACHE_LOCKED: return "Sprite cache: locked size (KB)";
    case ENGINE_VALUE_I_SPRCACHE_EXTERNAL: return "Sprite cache: ext sprites size (KB)";
    case ENGINE_VALUE_I_TEXCACHE_MAXNORMAL: return "Texture cache: normal capacity (KB)";
    case ENGINE_VALUE_I_TEXCACHE_NORMAL: return "Texture cache: normal size (KB)";
    case ENGINE_VALUE_I_FPS_MAX: return "FPS cap";
    case ENGINE_VALUE_I_FPS: return "FPS real";
    default: return "";
    }
}

bool System_GetDisplayFPS()
{
    return display_fps != kFPS_Hide;
}

void System_SetDisplayFPS(bool show_fps)
{
    if (display_fps != kFPS_Forced)
        display_fps = show_fps ? kFPS_Display : kFPS_Hide;
}

String GetRuntimeInfo()
{
    EngineServices &services = EngineServices::Current();
    DisplayMode mode = services.GfxDriver->GetDisplayMode();
    Rect render_frame = services.GfxDriver->GetRenderDestination();
    PGfxFilter filter = services.GfxDriver->GetGraphicsFilter();
    const size_t total_normspr = spriteset.GetCacheSize();
    const size_t total_lockspr = spriteset.GetLockedSize();
    const size_t total_extspr = spriteset.GetExternalSize();
    const size_t max_normspr = spriteset.GetMaxCacheSize();
    const unsigned norm_spr_filled = max_normspr > 0 ? (uint64_t)total_normspr * 100 / max_normspr : 0;
    size_t max_txcached, total_txcached, total_txlocked, total_txext;
    texturecache_get_state(max_txcached, total_txcached, total_txlocked, total_txext);
    const unsigned tx_filled = max_txcached > 0 ? (uint64_t)total_txcached * 100 / max_txcached : 0;
    String runtimeInfo = String::FromFormat(
        "%s\nEngine version %s\n"
        "Game resolution %d x %d (%d-bit)\n"
        "Running %d x %d at %d-bit%s\nGFX: %s; %s\nDraw frame %d x %d\n"
        "Sprite cache KB: %zu / %zu (%u%%), locked: %zu, ext: %zu\n"
        "Texture cache KB: %zu / %zu (%u%%)",
        get_engine_name(), get_engine_version_and_build().GetCStr(), game.GetGameRes().Width, game.GetGameRes().Height,
        game.GetColorDepth(), mode.Width, mode.Height, mode.ColorDepth,
        (mode.IsWindowed() ? " W" : (mode.IsRealFullscreen() ? " F" : " FD")), services.GfxDriver->GetDriverName(),
        filter->GetInfo().Name.GetCStr(), render_frame.GetWidth(), render_frame.GetHeight(), total_normspr / 1024,
        max_normspr / 1024, norm_spr_filled, total_lockspr / 1024, total_extspr / 1024, total_txcached / 1024,
        max_txcached / 1024, tx_filled);
    if (play.separate_music_lib)
        runtimeInfo.Append("\nAUDIO.VOX enabled");
    if (play.voice_avail)
        runtimeInfo.Append("\nSPEECH.VOX enabled");
    if (get_translation_tree().size() > 0)
    {
        runtimeInfo.Append("\nUsing translation ");
        runtimeInfo.Append(get_translation_name());
    }

    return runtimeInfo;
}

void SetMultitasking(int mode)
{
    EngineServices &services = EngineServices::Current();
    if ((mode < 0) || (mode > 1))
        quit("!SetMultitasking: invalid mode parameter");
    // Save requested setting
    services.Config.RunInBackground = mode != 0;

    // Account for the override config option (must be checked first!)
    if ((services.Config.Override.Multitasking >= 0) && (mode != services.Config.Override.Multitasking))
    {
        Debug::Printf("SetMultitasking: overridden by user config: %d -> %d", mode,
                      services.Config.Override.Multitasking);
        mode = services.Config.Override.Multitasking;
    }

    // Must run on background if debugger is connected
    if ((mode == 0) && (editor_debugging_initialized != 0))
    {
        Debug::Printf("SetMultitasking: overridden by the external debugger: %d -> 1", mode);
        mode = 1;
    }

    // Regardless, don't allow background running if exclusive full screen
    if ((mode == 1) && services.GfxDriver && services.GfxDriver->GetDisplayMode().IsRealFullscreen())
    {
        Debug::Printf("SetMultitasking: overridden by fullscreen: %d -> 0", mode);
        mode = 0;
    }

    // Install engine callbacks for switching in and out the window
    Debug::Printf(kDbgMsg_Info, "Multitasking mode set: %d", mode);
    if (mode == 0)
    {
        sys_set_background_mode(false);
        sys_evt_set_focus_callbacks(display_switch_in_resume, display_switch_out_suspend);
    }
    else
    {
        sys_set_background_mode(true);
        sys_evt_set_focus_callbacks(display_switch_in, display_switch_out);
    }
}
