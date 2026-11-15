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
// Definitions that the host and the engine's configuration share with the game logic.
//
//=============================================================================
#pragma once

enum eScriptSystemOSID
{
    eOS_Unknown = 0,
    eOS_DOS,
    eOS_Win,
    eOS_Linux,
    eOS_Mac,
    eOS_Android,
    eOS_iOS,
    eOS_PSP,
    eOS_Web,
    eOS_FreeBSD,
    eNumOS
};

// Runtime behavior switches.
// The purpose is differentiating between modern and backwards-compatible behavior
// on a per-operation basis.
enum RuntimeBehaviorSwitch
{
    // Is here to have at least one valid constant
    kRBS_Dummy = 0,
    // CLNUP: remove in AGS 4 (all of the below relate to running pre-3.6.3 versions)
    // Enables smooth transition between two consecutive walk orders
    kRBO_SmoothWalkTransition,
    // Apply text direction for GUI controls (except Labels, where it's always applied)
    kRBO_ApplyGUITextDirection,
    // Apply text alignment for dialog options (and auto-switch when translation changes)
    kRBO_ApplyDialogOptionTextAlignment,
    // Do not auto-translate values of text properties; only translate displayed text
    kRBO_NoTextPropertyAutoTranslate,
    kNum_RBS
};

enum SkipSpeechStyle
{
    kSkipSpeechNone = -1,
    kSkipSpeechKeyMouseTime = 0,
    kSkipSpeechKeyTime = 1,
    kSkipSpeechTime = 2,
    kSkipSpeechKeyMouse = 3,
    kSkipSpeechMouseTime = 4,
    kSkipSpeechKey = 5,
    kSkipSpeechMouse = 6,

    // Aliases for the most useful styles (used in accessibility options)
    kSkipSpeech_AnyInput = kSkipSpeechKeyMouse,
    kSkipSpeech_AnyInputOrTime = kSkipSpeechKeyMouseTime,

    kSkipSpeechFirst = kSkipSpeechNone,
    kSkipSpeechLast = kSkipSpeechMouse,
    kNumSpeechSkipOptions = kSkipSpeechLast - kSkipSpeechFirst
};

enum SpeechMode
{
    kSpeech_None = -1,
    kSpeech_TextOnly = 0,
    kSpeech_VoiceText = 1,
    kSpeech_VoiceOnly = 2,

    kSpeech_First = kSpeech_TextOnly,
    kSpeech_Last = kSpeech_VoiceOnly
};
