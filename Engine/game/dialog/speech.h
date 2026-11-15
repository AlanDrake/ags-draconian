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
#pragma once

#include "platform/base/engine_defines.h"
#include "util/string.h"

// Converts a script-facing skip style to a combination of SKIP_* flags (game/runtime_defines.h).
int user_to_internal_skip_speech(SkipSpeechStyle userval);
// Converts SKIP_* flags to a skip style: the style that has exactly the timer, key and mouse flags
// of the input. Any other flag (gamepad, touch) leaves the input unmatched, and the result is then
// kSkipSpeechTime if the timer flag is set, otherwise kSkipSpeechNone.
SkipSpeechStyle internal_skip_speech_to_user(int internal_val);
// Locates and initializes a voice pack of the given *name*, tells if successful;
// pass empty string for default voice pack.
bool init_voicepak(const AGS::Common::String &name = "");
// Gets voice pack's ID name, that is a filename without "sp_" prefix and no extension.
AGS::Common::String get_voicepak_name();
// Gets an asset's parent path for voice-over clips and data files
AGS::Common::String get_voice_assetpath();

struct CharacterInfo;
struct ScriptOverlay;

// Script API: Speech::get_TextOverlay
ScriptOverlay *Speech_GetTextOverlay();

// Script API: Speech::get_PortraitOverlay
ScriptOverlay *Speech_GetPortraitOverlay();

// Script API: Speech::get_SpeakingCharacter
CharacterInfo *Speech_GetSpeakingCharacter();

// Script API: Speech::get_OverlayZOrder
int Speech_GetOverlayZOrder();

// Script API: Speech::set_OverlayZOrder
void Speech_SetOverlayZOrder(int zorder);

// Script API: Speech::get_AlwaysWaitForText
bool Speech_GetAlwaysWaitForText();

// Script API: Speech::set_AlwaysWaitForText
void Speech_SetAlwaysWaitForText(bool on);

// Script API: Speech::get_AnimationStopTimeMargin
int Speech_GetAnimationStopTimeMargin();

// Script API: Speech::set_AnimationStopTimeMargin
void Speech_SetAnimationStopTimeMargin(int time);

// Script API: Speech::get_CustomPortraitPlacement
int Speech_GetCustomPortraitPlacement();

// Script API: Speech::set_CustomPortraitPlacement
void Speech_SetCustomPortraitPlacement(int placement);

// Script API: Speech::get_DisplayPostTimeMs
int Speech_GetDisplayPostTimeMs();

// Script API: Speech::set_DisplayPostTimeMs
void Speech_SetDisplayPostTimeMs(int time_ms);

// Script API: Speech::get_GlobalSpeechAnimationDelay
int Speech_GetGlobalSpeechAnimationDelay();

// Script API: Speech::set_GlobalSpeechAnimationDelay
void Speech_SetGlobalSpeechAnimationDelay(int delay);

// Script API: Speech::get_PortraitXOffset
int Speech_GetPortraitXOffset();

// Script API: Speech::set_PortraitXOffset
void Speech_SetPortraitXOffset(int x);

// Script API: Speech::get_PortraitY
int Speech_GetPortraitY();

// Script API: Speech::set_PortraitY
void Speech_SetPortraitY(int y);

// Script API: Speech::get_Style
int Speech_GetStyle();

// Script API: Speech::set_Style
void Speech_SetStyle(int newstyle);

// Script API: Speech::get_SkipStyle
SkipSpeechStyle Speech_GetSkipStyle();

// Script API: Speech::set_SkipStyle
void Speech_SetSkipStyle(SkipSpeechStyle new_value);

// Script API: Speech::get_SkipKey
int Speech_GetSkipKey();

// Script API: Speech::set_SkipKey
void Speech_SetSkipKey(int key);

// Script API: Speech::get_TextAlignment
int Speech_GetTextAlignment();

// Script API: Speech::set_TextAlignment
void Speech_SetTextAlignment(int alignment);

// Script API: Speech::get_UseGlobalSpeechAnimationDelay
int Speech_GetUseGlobalSpeechAnimationDelay();

// Script API: Speech::set_UseGlobalSpeechAnimationDelay
void Speech_SetUseGlobalSpeechAnimationDelay(int delay);

// Script API: Speech::set_VoiceMode
void Speech_SetVoiceMode(int newmod);

// Script API: Speech::get_VoiceMode
int Speech_GetVoiceMode();

// Script function: sets the voice-over volume (0-255), also for the voice clip that is playing now
void SetSpeechVolume(int volume);
// Script function: tells whether the game has voice-over available (1 or 0)
int IsSpeechVoxAvailable();

struct ScriptAudioChannel;

//=============================================================================
// Play voice-over for the active blocking speech and initialize relevant data (lip-sync line, speech style);
// char_id is a character index, -1 for the narrator; returns false if no voice was played
bool play_voice_speech(int char_id, int cue_id);
// Play voice-over clip in non-blocking manner; if as_speech, it is marked as the voice of the active speech;
// char_id is a character index, -1 for the narrator; returns false if no voice was played
bool play_voice_nonblocking(int char_id, int cue_id, bool as_speech);
// Play voice-over clip in non-blocking manner, as a given audio type, optionally selecting an explicit channel
// (chan is -1 to pick any); returns the channel played on, or null if no voice was played
const ScriptAudioChannel *play_voice_clip_as_type(int char_id, int cue_id, int type, int chan, int priority,
                                                  int repeat);
// Stop voice-over for the active speech and reset relevant data
void stop_voice_speech();
// Stop non-blocking voice-over and revert audio volumes if necessary;
// does nothing if no voice is marked as speech (a voice played with as_speech false is not stopped)
void stop_voice_nonblocking();
