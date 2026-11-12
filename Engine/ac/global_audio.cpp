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
#include "ac/global_audio.h"

#include <cstdio>

#include "ac/common.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/lipsync.h"
#include "ac/path_helper.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "game/roomstruct.h"
#include "main/engine.h"
#include "media/audio/audio_core.h"
#include "media/audio/audio_system.h"
#include "util/string_compat.h"
#include "util/time_util.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStruct thisroom;
extern std::vector<SpeechLipSyncLine> splipsync;
extern int numLipLines, curLipLine, curLipLinePhoneme;


// Script function: sets the voice-over volume (0-255), applied to the playing voice clip too
void SetSpeechVolume(int volume)
{
    if ((volume < 0) | (volume > 255))
        quit("!SetSpeechVolume: invalid volume - must be from 0-255");

    auto *playback = AudioChans::GetChannel(SCHAN_SPEECH);
    if (playback)
    {
        playback->set_volume255(volume);
    }
    play.speech_volume = volume;
}

int IsSpeechVoxAvailable()
{
    return play.voice_avail ? 1 : 0;
}

int IsMusicVoxAvailable()
{
    return play.separate_music_lib ? 1 : 0;
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
    const char *char_name = (char_id >= 0) ? game.chars[char_id].scrname.GetCStr() : "narrator";
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
    // TODO: perhaps a better algorithm, allow any extension / sound format?
    // e.g. make a hashmap matching a voice name to a asset name
    std::array<const char *, 4> exts = {{"mp3", "ogg", "wav", "flac"}};
    AssetPath apath = get_voice_over_assetpath(voice_name);
    bool found = false;
    for (auto *ext : exts)
    {
        apath.Name.Format("%s.%s", voice_name.GetCStr(), ext);
        found = AssetMgr->DoesAssetExist(apath);
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
        voice_clip->set_volume255(play.speech_volume);
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
    if (!play_voice_clip_on_channel(voice_name))
        return false;
    if (!as_speech)
        return true;

    play.speech_has_voice = true;
    play.speech_voice_blocking = is_blocking;

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
    // don't play speech if we're skipping a cutscene (fast-forward), in text-only speech mode,
    // or if the voice-over is not available
    if (!play.ShouldPlayVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(char_id, cue_id);
    if (!play_voice_clip_impl(voice_file, true, true))
        return false;

    // See if we have voice lip sync for this line:
    // compare the voice file name to the lip-sync lines' file names
    curLipLine = -1;
    curLipLinePhoneme = -1;
    int line_id;
    for (line_id = 0; line_id < numLipLines; line_id++)
    {
        if (voice_file.CompareNoCase(splipsync[line_id].filename) == 0)
        {
            curLipLine = line_id;
            break;
        }
    }
    // if the game has any voice lip-sync data, disable the text-related lipsync
    // (even if this line has no match)
    if (numLipLines > 0)
        game.options[OPT_LIPSYNCTEXT] = 0;

    // change Sierra w/bgrnd to Sierra without background when the game asks for that
    // with voice speech (no_textbg_when_voice; 2 marks the switch, stop_voice_speech reverts it)
    if ((game.options[OPT_SPEECHTYPE] == kSpeechStyle_SierraBackground) && (play.no_textbg_when_voice > 0))
    {
        game.options[OPT_SPEECHTYPE] = kSpeechStyle_SierraTransparent;
        play.no_textbg_when_voice = 2;
    }
    return true;
}

bool play_voice_nonblocking(int char_id, int cue_id, bool as_speech)
{
    // don't play voice if we're skipping a cutscene (fast-forward) or the voice-over is not available
    if (!play.ShouldPlayVoiceSpeechNonBlocking())
        return false;
    // don't play voice if there's a blocking speech with voice-over already
    if (as_speech && play.IsBlockingVoiceSpeech())
        return false;

    String voice_file = get_cue_filename(char_id, cue_id);
    return play_voice_clip_impl(voice_file, as_speech, false);
}

const ScriptAudioChannel *play_voice_clip_as_type(int char_id, int cue_id, int type, int chan, int priority, int repeat)
{
    // don't play voice if we're skipping a cutscene (fast-forward) or the voice-over is not available
    if (!play.ShouldPlayVoiceSpeechNonBlocking())
        return nullptr;

    String voice_file = get_cue_filename(char_id, cue_id);
    AssetPath apath = find_voice_clip(voice_file);
    if (!apath)
        return nullptr;

    ScriptAudioClip sc_clip; // pass temp dummy clip
    sc_clip.fileName = apath.Name;
    sc_clip.fileType = ScriptAudioClip::GetAudioFileTypeFromExt(Path::GetFileExtension(apath.Name).GetCStr());
    sc_clip.bundlingType = kAudioBundle_SpeechVox;
    return play_audio_clip(AudioPlayback(&sc_clip, type), chan, priority, repeat, 0, false);
}

void stop_voice_speech()
{
    if (!play.speech_has_voice)
        return;

    stop_voice_clip_impl();

    // Reset lipsync
    curLipLine = -1;
    // Set back to Sierra w/bgrnd, if play_voice_speech switched it
    if (play.no_textbg_when_voice == 2)
    {
        play.no_textbg_when_voice = 1;
        game.options[OPT_SPEECHTYPE] = kSpeechStyle_SierraBackground;
    }
    play.speech_has_voice = false;
    play.speech_voice_blocking = false;
}

void stop_voice_nonblocking()
{
    if (!play.speech_has_voice)
        return;
    stop_voice_clip_impl();
    // Only reset speech flags if we are truly playing a non-blocking voice;
    // otherwise we might be inside blocking speech function and should let
    // it keep these flags to be able to finalize properly.
    // This is an imperfection of current speech implementation.
    if (!play.speech_voice_blocking)
    {
        play.speech_has_voice = false;
        play.speech_voice_blocking = false;
    }
}
