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

#include "speech.h"

// Script function: sets the voice-over volume (0-255), also for the voice clip that is playing now
void SetSpeechVolume(int volume);
// Script function: tells whether the game has voice-over available (1 or 0)
int IsSpeechVoxAvailable();
// Script function: tells whether the game has a separate music library (1 or 0)
int IsMusicVoxAvailable();

struct CharacterInfo;
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
