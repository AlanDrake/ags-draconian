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
#include "game/dialog/speech.h"

#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "game/asset_helper.h"
#include "game/audio/audio.h"
#include "game/dialog/lipsync.h"
#include "game/dynobj/script_overlay.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_state.h"
#include "game/runtime_defines.h"
#include "game/world/character.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "util/common.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/path.h"

using namespace AGS::Common;
using namespace AGS::Engine;

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
    EngineServices &services = EngineServices::Current();
    if (!services.Config.UseVoicePack)
        return false; // voice-over disabled

    String speech_file = name.IsEmpty() ? "speech.vox" : String::FromFormat("sp_%s.vox", name.GetCStr());
    if (services.ResPaths.SpeechPak.Name.CompareNoCase(speech_file) == 0)
        return true; // same pak already assigned

    // First remove existing voice packs
    services.ResPaths.VoiceAvail = false;
    services.Assets->RemoveLibrary(services.ResPaths.SpeechPak.Path);
    services.Assets->RemoveLibrary(services.ResPaths.VoiceDirSub);

    // Now check for the new packs and add if they exist
    String speech_filepath = find_assetlib(speech_file);
    if (!speech_filepath.IsEmpty())
    {
        Debug::Printf(kDbgMsg_Info, "Voice pack found: %s", speech_file.GetCStr());
        services.ResPaths.VoiceAvail = true;
    }
    else
    {
        Debug::Printf(kDbgMsg_Info, "Was not able to init voice pack '%s': file not found or of unknown format.",
                      speech_file.GetCStr());
    }

    String speech_subdir = "";
    // Look up in the alternative locations that include "voice" filter
    for (const auto &opt_dir : services.ResPaths.OptDataDirs)
    {
        if (opt_dir.second.FindSection("voice", ',') != String::NoIndex)
        {
            // If we have custom voice directory set, we will enable voice-over even if speech.vox does not exist
            speech_subdir = Path::ConcatPaths(opt_dir.first, name);
            if (File::IsDirectory(speech_subdir) && Directory::HasAnyFiles(speech_subdir))
            {
                Debug::Printf(kDbgMsg_Info, "Optional voice directory is defined: %s", speech_subdir.GetCStr());
                services.ResPaths.VoiceAvail = true;
            }
        }
    }

    // Save new resource locations and register asset libraries
    VoicePakName = name;
    VoiceAssetPath = name;
    services.ResPaths.SpeechPak.Name = speech_file;
    services.ResPaths.SpeechPak.Path = speech_filepath;
    services.ResPaths.VoiceDirSub = speech_subdir;
    services.Assets->AddLibrary(services.ResPaths.VoiceDirSub, "voice");
    services.Assets->AddLibrary(services.ResPaths.SpeechPak.Path, "voice");
    // each voice pack has its own lip-sync data
    AGS::Engine::LoadLipsyncData();
    return services.ResPaths.VoiceAvail;
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

#include "debug/out.h"
#include "game/game_def.h"
#include "script/dynobj/dynobj_manager.h"

ScriptOverlay *Speech_GetTextOverlay()
{
    return static_cast<ScriptOverlay *>(ccGetObjectAddressFromHandle(Game::Current().Play.SpeechTextScriptHandle));
}

ScriptOverlay *Speech_GetPortraitOverlay()
{
    return static_cast<ScriptOverlay *>(ccGetObjectAddressFromHandle(Game::Current().Play.SpeechPortraitScriptHandle));
}

Character *Speech_GetSpeakingCharacter()
{
    Game &game = Game::Current();
    return game.Speech.CharSpeaking >= 0 ? &game.Characters[game.Speech.CharSpeaking] : nullptr;
}

int Speech_GetOverlayZOrder()
{
    return Game::Current().Play.SpeechOverlayZOrder;
}

void Speech_SetOverlayZOrder(int zorder)
{
    Game::Current().Play.SpeechOverlayZOrder = zorder;
}

bool Speech_GetAlwaysWaitForText()
{
    return Game::Current().Play.AlwaysWaitForSpeechText;
}

void Speech_SetAlwaysWaitForText(bool on)
{
    if (!EngineServices::Current().Config.Access.AlwaysWaitForText)
        Game::Current().Play.AlwaysWaitForSpeechText = on;
}

int Speech_GetAnimationStopTimeMargin()
{
    return Game::Current().Play.SpeechAnimationStopTimeMargin;
}

void Speech_SetAnimationStopTimeMargin(int time)
{
    Game::Current().Play.SpeechAnimationStopTimeMargin = time;
}

int Speech_GetCustomPortraitPlacement()
{
    return Game::Current().Play.IsSpeechCustomPortraitPlacement;
}

void Speech_SetCustomPortraitPlacement(int placement)
{
    Game::Current().Play.IsSpeechCustomPortraitPlacement = placement;
}

int Speech_GetDisplayPostTimeMs()
{
    return Game::Current().Play.SpeechDisplayPostTimeMs;
}

void Speech_SetDisplayPostTimeMs(int time_ms)
{
    Game::Current().Play.SpeechDisplayPostTimeMs = time_ms;
}

int Speech_GetGlobalSpeechAnimationDelay()
{
    return Game::Current().Play.GlobalSpeechAnimationDelay;
}

void Speech_SetGlobalSpeechAnimationDelay(int delay)
{
    Game &game = Game::Current();
    if (game.Options[OPT_GLOBALTALKANIMSPD] == 0)
    {
        debug_script_warn("Speech.GlobalSpeechAnimationDelay cannot be set when global speech animation speed is not "
                          "enabled; set Speech.UseGlobalSpeechAnimationDelay first!");
        return;
    }
    game.Play.GlobalSpeechAnimationDelay = delay;
}

int Speech_GetPortraitXOffset()
{
    return Game::Current().Play.SpeechPortraitOffsetX;
}

void Speech_SetPortraitXOffset(int x)
{
    Game::Current().Play.SpeechPortraitOffsetX = x;
}

int Speech_GetPortraitY()
{
    return Game::Current().Play.SpeechPortraitY;
}

void Speech_SetPortraitY(int y)
{
    Game::Current().Play.SpeechPortraitY = y;
}

int Speech_GetStyle()
{
    return Game::Current().Options[OPT_SPEECHTYPE];
}

void Speech_SetStyle(int newstyle)
{
    if ((newstyle < 0) || (newstyle > 3))
        quit("!SetSpeechStyle: must use a SPEECH_* constant as parameter");
    Game::Current().Options[OPT_SPEECHTYPE] = newstyle;
}

SkipSpeechStyle Speech_GetSkipStyle()
{
    return internal_skip_speech_to_user(Game::Current().Play.SpeechSkipStyle);
}

void Speech_SetSkipStyle(SkipSpeechStyle new_value)
{
    if ((new_value < kSkipSpeechFirst) || (new_value > kSkipSpeechLast))
        quit("!SetSkipSpeech: invalid skip mode specified");

    if (EngineServices::Current().Config.Access.SpeechSkipStyle == kSkipSpeechNone)
    {
        debug_script_log("SkipSpeech style set to %d", new_value);
        Game::Current().Play.SpeechSkipStyle = user_to_internal_skip_speech((SkipSpeechStyle)new_value);
    }
}

int Speech_GetSkipKey()
{
    return Game::Current().Play.SpeechSkipKey;
}

void Speech_SetSkipKey(int key)
{
    Game::Current().Play.SpeechSkipKey = key;
}

int Speech_GetTextAlignment()
{
    return Game::Current().Play.SpeechTextAlignment;
}

void Speech_SetTextAlignment(int alignment)
{
    Game::Current().Play.SpeechTextAlignment = (HorAlignment)alignment;
}

int Speech_GetUseGlobalSpeechAnimationDelay()
{
    return Game::Current().Options[OPT_GLOBALTALKANIMSPD];
}

void Speech_SetUseGlobalSpeechAnimationDelay(int delay)
{
    Game::Current().Options[OPT_GLOBALTALKANIMSPD] = delay;
}

void Speech_SetVoiceMode(int newmod)
{
    if ((newmod < kSpeech_First) | (newmod > kSpeech_Last))
        quitprintf("!SetVoiceMode: invalid mode number %d", newmod);
    if (EngineServices::Current().Config.Access.SpeechMode == kSpeech_None)
        Game::Current().Play.SpeechVoiceMode = (SpeechMode)newmod;
}

int Speech_GetVoiceMode()
{
    return (int)Game::Current().Play.SpeechVoiceMode;
}

//-----------------------------------------------------------------------------

// Script function: sets the voice-over volume (0-255), applied to the playing voice clip too
void SetSpeechVolume(int volume)
{
    if ((volume < 0) || (volume > 255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    auto *playback = AudioChans::GetChannel(SCHAN_SPEECH);
    if (playback)
    {
        playback->set_volume255(volume);
    }
    Game::Current().Play.SpeechVolume = volume;
}

int IsSpeechVoxAvailable()
{
    return EngineServices::Current().VoiceAvail ? 1 : 0;
}

// Construct an asset name for the voice-over clip for the given character (-1 is the narrator) and cue id;
// the name is placed under the voice pack's path (get_voice_assetpath)
static String get_cue_filename(int char_id, int cue_id)
{
    // Clip name generation rule:
    // Cut a lower case 'c' prefix from the character's script name (whatever follows it),
    // because in AGS characters were traditionally named as 'cEgo'.
    // Naming format: the script name without the prefix,
    //            the cue id is separated by a dot:
    //            "CHARNAME.X"
    const char *char_name = (char_id >= 0) ? Game::Current().Characters[char_id].ScriptName.GetCStr() : "narrator";
    size_t from = (char_name[0] == 'c') ? 1 : 0u;
    String clip_char_name(char_name + from, SIZE_MAX);

    const char *fmt_str = "%s.%d";
    String asset_filename = String::FromFormat(fmt_str, clip_char_name.GetCStr(), cue_id);
    return Path::ConcatPaths(get_voice_assetpath(), asset_filename);
}

// Looks for the voice-over asset of the given bare clip name, trying the extensions in the order of the list below;
// returns an empty path (and logs a script warning) if none exists
static AssetPath find_voice_clip(const String &voice_name)
{
    EngineServices &services = EngineServices::Current();
    // TODO: perhaps a better algorithm, allow any extension / sound format?
    // e.g. make a hashmap matching a voice name to a asset name
    std::array<const char *, 4> exts = {{"mp3", "ogg", "wav", "flac"}};
    AssetPath apath = get_voice_over_assetpath(voice_name);
    bool found = false;
    for (auto *ext : exts)
    {
        apath.Name.Format("%s.%s", voice_name.GetCStr(), ext);
        found = services.Assets->DoesAssetExist(apath);
        if (found)
            break;
    }

    if (!found)
    {
        debug_script_warn("Speech file not found: '%s'", voice_name.GetCStr());
        return AssetPath();
    }

    return apath;
}

// Play voice-over clip on the speech channel, first stopping whatever plays there;
// voice_name should be bare clip name without extension;
// returns false if the clip is not found, cannot be loaded or cannot start playing
static bool play_voice_clip_on_channel(const String &voice_name)
{
    stop_and_destroy_channel(SCHAN_SPEECH);

    AssetPath apath = find_voice_clip(voice_name);
    if (!apath)
        return false;

    std::unique_ptr<SoundClip> voice_clip(load_sound_clip(apath, "", false));
    if (voice_clip != nullptr)
    {
        voice_clip->fileName = apath.Name;
        voice_clip->bundlingType = kAudioBundle_SpeechVox;
        voice_clip->set_volume255(Game::Current().Play.SpeechVolume);
        if (!voice_clip->play())
            voice_clip.reset();
    }

    if (!voice_clip)
    {
        debug_script_warn("Speech load failure: '%s'", voice_name.GetCStr());
        return false;
    }

    AudioChans::SetChannel(SCHAN_SPEECH, std::move(voice_clip));
    return true;
}

// Play voice-over clip; if as_speech, also mark the speech as voiced (blocking or not by is_blocking)
// and apply the volume drop to the other audio;
// voice_name should be bare clip name without extension
static bool play_voice_clip_impl(const String &voice_name, bool as_speech, bool is_blocking)
{
    Game &game = Game::Current();
    if (!play_voice_clip_on_channel(voice_name))
        return false;
    if (!as_speech)
        return true;

    game.Play.HasSpeechVoice = true;
    game.Play.IsSpeechVoiceBlocking = is_blocking;

    cancel_scheduled_music_update();
    apply_volume_drop_modifier(true);
    return true;
}

// Stop voice-over clip and schedule audio volume reset
static void stop_voice_clip_impl()
{
    // update the music in a bit (fixes two speeches follow each other
    // and music going up-then-down)
    schedule_music_update_at(Clock::now() + std::chrono::milliseconds(500));
    stop_and_destroy_channel(SCHAN_SPEECH);
}

bool play_voice_speech(int char_id, int cue_id)
{
    Game &game = Game::Current();
    // don't play speech if we're skipping a cutscene (fast-forward), in text-only speech mode,
    // or if the voice-over is not available
    if (!game.Play.ShouldPlayVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(char_id, cue_id);
    if (!play_voice_clip_impl(voice_file, true, true))
        return false;

    // See if we have voice lip sync for this line:
    // compare the voice file name to the lip-sync lines' file names
    game.LipSync.CurLine = -1;
    game.LipSync.CurLinePhoneme = -1;
    // (the sync data names the clips without the voice pack's folder)
    const String clip_name = Path::GetFilename(voice_file);
    int line_id;
    for (line_id = 0; line_id < game.LipSync.NumLines; line_id++)
    {
        if (clip_name.CompareNoCase(game.LipSync.Lines[line_id].filename) == 0)
        {
            game.LipSync.CurLine = line_id;
            break;
        }
    }
    // if the game has any voice lip-sync data, disable the text-related lipsync
    // (even if this line has no match)
    if (game.LipSync.NumLines > 0)
        game.Options[OPT_LIPSYNCTEXT] = 0;

    // change Sierra w/bgrnd to Sierra without background when the game asks for that
    // with voice speech (NoTextBackgroundWhenVoice; 2 marks the switch, stop_voice_speech reverts it)
    if ((game.Options[OPT_SPEECHTYPE] == kSpeechStyle_SierraBackground) && (game.Play.NoTextBackgroundWhenVoice > 0))
    {
        game.Options[OPT_SPEECHTYPE] = kSpeechStyle_SierraTransparent;
        game.Play.NoTextBackgroundWhenVoice = 2;
    }
    return true;
}

bool play_voice_nonblocking(int char_id, int cue_id, bool as_speech)
{
    Game &game = Game::Current();
    // don't play voice if we're skipping a cutscene (fast-forward) or the voice-over is not available
    if (!game.Play.ShouldPlayVoiceSpeechNonBlocking())
        return false;
    // don't play voice if there's a blocking speech with voice-over already
    if (as_speech && game.Play.IsBlockingVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(char_id, cue_id);
    return play_voice_clip_impl(voice_file, as_speech, false);
}

const ScriptAudioChannel *play_voice_clip_as_type(int char_id, int cue_id, int type, int chan, int priority, int repeat)
{
    // don't play voice if we're skipping a cutscene (fast-forward) or the voice-over is not available
    if (!Game::Current().Play.ShouldPlayVoiceSpeechNonBlocking())
        return nullptr;

    String voice_file = get_cue_filename(char_id, cue_id);
    AssetPath apath = find_voice_clip(voice_file);
    if (!apath)
        return nullptr;

    AudioClipDef sc_clip; // pass temp dummy clip (the playback reads only the definition)
    sc_clip.FileName = apath.Name;
    sc_clip.FileType = AudioClipDef::GetAudioFileTypeFromExt(Path::GetFileExtension(apath.Name).GetCStr());
    sc_clip.BundlingType = kAudioBundle_SpeechVox;
    return play_audio_clip(AudioPlayback(&sc_clip, type), chan, priority, repeat, 0, false);
}

void stop_voice_speech()
{
    Game &game = Game::Current();
    if (!game.Play.HasSpeechVoice)
        return;

    stop_voice_clip_impl();

    // Reset lipsync
    game.LipSync.CurLine = -1;
    // Set back to Sierra w/bgrnd, if play_voice_speech switched it
    if (game.Play.NoTextBackgroundWhenVoice == 2)
    {
        game.Play.NoTextBackgroundWhenVoice = 1;
        game.Options[OPT_SPEECHTYPE] = kSpeechStyle_SierraBackground;
    }
    game.Play.HasSpeechVoice = false;
    game.Play.IsSpeechVoiceBlocking = false;
}

void stop_voice_nonblocking()
{
    Game &game = Game::Current();
    if (!game.Play.HasSpeechVoice)
        return;
    stop_voice_clip_impl();
    // Only reset speech flags if we are truly playing a non-blocking voice;
    // otherwise we might be inside blocking speech function and should let
    // it keep these flags to be able to finalize properly.
    // This is an imperfection of current speech implementation.
    if (!game.Play.IsSpeechVoiceBlocking)
    {
        game.Play.HasSpeechVoice = false;
        game.Play.IsSpeechVoiceBlocking = false;
    }
}
