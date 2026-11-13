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
#include "ac/speech.h"

#include "ac/asset_helper.h"
#include "ac/common.h"
#include "ac/dynobj/scriptoverlay.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamestate.h"
#include "ac/lipsync.h"
#include "ac/runtime_defines.h"
#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "main/engine.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;

// identifier (username) of the voice pak
static String VoicePakName;
// parent part to use when making voice asset names
static String VoiceAssetPath;


int user_to_internal_skip_speech(SkipSpeechStyle userval)
{
    // NOTE: After Gamepad and Touch controls were implemented,
    // we treat "Skip by key" as "by key or gamepad button"
    // and "Skip by mouse" as "by mouse or touch".
    switch (userval)
    {
    case kSkipSpeechNone: return SKIP_NONE;
    case kSkipSpeechKeyMouseTime:
        // TODO: remake this as "kSkipAny"?
        return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK | SKIP_GAMEPAD | SKIP_TOUCH;
    case kSkipSpeechKeyTime: return SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_GAMEPAD;
    case kSkipSpeechTime: return SKIP_AUTOTIMER;
    case kSkipSpeechKeyMouse:
        // TODO: remake this as "kSkipAnyInput"?
        return SKIP_KEYPRESS | SKIP_MOUSECLICK | SKIP_GAMEPAD | SKIP_TOUCH;
    case kSkipSpeechMouseTime: return SKIP_AUTOTIMER | SKIP_MOUSECLICK | SKIP_TOUCH;
    case kSkipSpeechKey: return SKIP_KEYPRESS | SKIP_GAMEPAD;
    case kSkipSpeechMouse: return SKIP_MOUSECLICK | SKIP_TOUCH;
    default: debug_script_warn("user_to_internal_skip_speech: unknown userval"); return SKIP_NONE;
    }
}

SkipSpeechStyle internal_skip_speech_to_user(int internal_val)
{
    // Gamepad buttons count as keys, touch as mouse (see user_to_internal_skip_speech)
    if (internal_val & SKIP_GAMEPAD)
        internal_val |= SKIP_KEYPRESS;
    if (internal_val & SKIP_TOUCH)
        internal_val |= SKIP_MOUSECLICK;
    internal_val &= ~(SKIP_GAMEPAD | SKIP_TOUCH);

    if (internal_val & SKIP_AUTOTIMER)
    {
        internal_val &= ~SKIP_AUTOTIMER;
        if (internal_val == (SKIP_KEYPRESS | SKIP_MOUSECLICK))
        {
            return kSkipSpeechKeyMouseTime;
        }
        else if (internal_val == SKIP_KEYPRESS)
        {
            return kSkipSpeechKeyTime;
        }
        else if (internal_val == SKIP_MOUSECLICK)
        {
            return kSkipSpeechMouseTime;
        }
        return kSkipSpeechTime;
    }
    else
    {
        if (internal_val == (SKIP_KEYPRESS | SKIP_MOUSECLICK))
        {
            return kSkipSpeechKeyMouse;
        }
        else if (internal_val == SKIP_KEYPRESS)
        {
            return kSkipSpeechKey;
        }
        else if (internal_val == SKIP_MOUSECLICK)
        {
            return kSkipSpeechMouse;
        }
    }
    return kSkipSpeechNone;
}

bool init_voicepak(const String &name)
{
    if (!usetup.UseVoicePack)
        return false; // voice-over disabled

    String speech_file = name.IsEmpty() ? "speech.vox" : String::FromFormat("sp_%s.vox", name.GetCStr());
    if (ResPaths.SpeechPak.Name.CompareNoCase(speech_file) == 0)
        return true; // same pak already assigned

    // First remove existing voice packs
    ResPaths.VoiceAvail = false;
    AssetMgr->RemoveLibrary(ResPaths.SpeechPak.Path);
    AssetMgr->RemoveLibrary(ResPaths.VoiceDirSub);

    // Now check for the new packs and add if they exist
    String speech_filepath = find_assetlib(speech_file);
    if (!speech_filepath.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Info, "Voice pack found: %s", speech_file.GetCStr());
        ResPaths.VoiceAvail = true;
    }
    else
    {
        Debug::Printf(kDbgMsg_Info, "Was not able to init voice pack '%s': file not found or of unknown format.",
                      speech_file.GetCStr());
    }

    String speech_subdir = "";
    // Look up in the alternative locations that include "voice" filter
    for (const auto &opt_dir : ResPaths.OptDataDirs)
    {
        if (opt_dir.second.FindSection("voice", ',') != String::NoIndex)
        {
            // If we have custom voice directory set, we will enable voice-over even if speech.vox does not exist
            speech_subdir = Path::ConcatPaths(opt_dir.first, name);
            if (File::IsDirectory(speech_subdir) && Directory::HasAnyFiles(speech_subdir))
            {
                Debug::Printf(kDbgMsg_Info, "Optional voice directory is defined: %s", speech_subdir.GetCStr());
                ResPaths.VoiceAvail = true;
            }
        }
    }

    // Save new resource locations and register asset libraries
    VoicePakName = name;
    VoiceAssetPath = name;
    ResPaths.SpeechPak.Name = speech_file;
    ResPaths.SpeechPak.Path = speech_filepath;
    ResPaths.VoiceDirSub = speech_subdir;
    AssetMgr->AddLibrary(ResPaths.VoiceDirSub, "voice");
    AssetMgr->AddLibrary(ResPaths.SpeechPak.Path, "voice");
    // each voice pack has its own lip-sync data
    AGS::Engine::LoadLipsyncData();
    return ResPaths.VoiceAvail;
}

String get_voicepak_name()
{
    return VoicePakName;
}

String get_voice_assetpath()
{
    return VoiceAssetPath;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "ac/dynobj/dynobj_manager.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "debug/out.h"

extern GameSetupStruct game;
extern int char_speaking;

ScriptOverlay *Speech_GetTextOverlay()
{
    return (ScriptOverlay *)ccGetObjectAddressFromHandle(play.speech_text_schandle);
}

ScriptOverlay *Speech_GetPortraitOverlay()
{
    return (ScriptOverlay *)ccGetObjectAddressFromHandle(play.speech_face_schandle);
}

CharacterInfo *Speech_GetSpeakingCharacter()
{
    return char_speaking >= 0 ? &game.chars[char_speaking] : nullptr;
}

int Speech_GetOverlayZOrder()
{
    return play.speech_zorder;
}

void Speech_SetOverlayZOrder(int zorder)
{
    play.speech_zorder = zorder;
}

bool Speech_GetAlwaysWaitForText()
{
    return play.speech_always_wait_for_text;
}

void Speech_SetAlwaysWaitForText(bool on)
{
    if (!usetup.Access.AlwaysWaitForText)
        play.speech_always_wait_for_text = on;
}

int Speech_GetAnimationStopTimeMargin()
{
    return play.close_mouth_speech_time;
}

void Speech_SetAnimationStopTimeMargin(int time)
{
    play.close_mouth_speech_time = time;
}

int Speech_GetCustomPortraitPlacement()
{
    return play.speech_portrait_placement;
}

void Speech_SetCustomPortraitPlacement(int placement)
{
    play.speech_portrait_placement = placement;
}

int Speech_GetDisplayPostTimeMs()
{
    return play.speech_display_post_time_ms;
}

void Speech_SetDisplayPostTimeMs(int time_ms)
{
    play.speech_display_post_time_ms = time_ms;
}

int Speech_GetGlobalSpeechAnimationDelay()
{
    return play.talkanim_speed;
}

void Speech_SetGlobalSpeechAnimationDelay(int delay)
{
    if (game.options[OPT_GLOBALTALKANIMSPD] == 0)
    {
        debug_script_warn("Speech.GlobalSpeechAnimationDelay cannot be set when global speech animation speed is not "
                          "enabled; set Speech.UseGlobalSpeechAnimationDelay first!");
        return;
    }
    play.talkanim_speed = delay;
}

int Speech_GetPortraitXOffset()
{
    return play.speech_portrait_x;
}

void Speech_SetPortraitXOffset(int x)
{
    play.speech_portrait_x = x;
}

int Speech_GetPortraitY()
{
    return play.speech_portrait_y;
}

void Speech_SetPortraitY(int y)
{
    play.speech_portrait_y = y;
}

int Speech_GetStyle()
{
    return game.options[OPT_SPEECHTYPE];
}

void Speech_SetStyle(int newstyle)
{
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    game.options[OPT_SPEECHTYPE] = newstyle;
}

SkipSpeechStyle Speech_GetSkipStyle()
{
    return internal_skip_speech_to_user(play.speech_skip_style);
}

void Speech_SetSkipStyle(SkipSpeechStyle new_value)
{
    if ((new_value < kSkipSpeechFirst) || (new_value > kSkipSpeechLast))
        quit("!SetSkipSpeech: invalid skip mode specified");

    if (usetup.Access.SpeechSkipStyle == kSkipSpeechNone)
    {
        debug_script_log("SkipSpeech style set to %d", new_value);
        play.speech_skip_style = user_to_internal_skip_speech((SkipSpeechStyle)new_value);
    }
}

int Speech_GetSkipKey()
{
    return play.skip_speech_specific_key;
}

void Speech_SetSkipKey(int key)
{
    play.skip_speech_specific_key = key;
}

int Speech_GetTextAlignment()
{
    return play.speech_text_align;
}

void Speech_SetTextAlignment(int alignment)
{
    play.speech_text_align = (HorAlignment)alignment;
}

int Speech_GetUseGlobalSpeechAnimationDelay()
{
    return game.options[OPT_GLOBALTALKANIMSPD];
}

void Speech_SetUseGlobalSpeechAnimationDelay(int delay)
{
    game.options[OPT_GLOBALTALKANIMSPD] = delay;
}

void Speech_SetVoiceMode(int newmod)
{
    if ((newmod < kSpeech_First) | (newmod > kSpeech_Last))
        quitprintf("!SetVoiceMode: invalid mode number %d", newmod);
    if (usetup.Access.SpeechMode == kSpeech_None)
        play.speech_mode = (SpeechMode)newmod;
}

int Speech_GetVoiceMode()
{
    return (int)play.speech_mode;
}

//-----------------------------------------------------------------------------
