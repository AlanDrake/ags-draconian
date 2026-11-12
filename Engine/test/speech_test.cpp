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
// Speech.SkipStyle: the script value converted to the internal skip flags and
// back must give the same value.
//
//=============================================================================
#include "gtest/gtest.h"
#include "ac/runtime_defines.h"
#include "ac/speech.h"

TEST(Speech, SkipStyleRoundTrip)
{
    const SkipSpeechStyle styles[] = {kSkipSpeechNone, kSkipSpeechKeyMouseTime, kSkipSpeechKeyTime,
                                      kSkipSpeechTime, kSkipSpeechKeyMouse,     kSkipSpeechMouseTime,
                                      kSkipSpeechKey,  kSkipSpeechMouse};
    for (SkipSpeechStyle style : styles)
    {
        EXPECT_EQ(style, internal_skip_speech_to_user(user_to_internal_skip_speech(style))) << "style " << style;
    }
}

TEST(Speech, SkipStyleReadBackIgnoresGamepadAndTouch)
{
    // The gamepad and touch flags are part of "key" and "mouse"; with or without them,
    // the style read back is the same
    EXPECT_EQ(kSkipSpeechKey, internal_skip_speech_to_user(SKIP_KEYPRESS));
    EXPECT_EQ(kSkipSpeechKey, internal_skip_speech_to_user(SKIP_KEYPRESS | SKIP_GAMEPAD));
    EXPECT_EQ(kSkipSpeechMouse, internal_skip_speech_to_user(SKIP_MOUSECLICK));
    EXPECT_EQ(kSkipSpeechMouse, internal_skip_speech_to_user(SKIP_MOUSECLICK | SKIP_TOUCH));
    EXPECT_EQ(kSkipSpeechKeyMouse, internal_skip_speech_to_user(SKIP_KEYPRESS | SKIP_MOUSECLICK));
    EXPECT_EQ(kSkipSpeechKeyMouseTime, internal_skip_speech_to_user(SKIP_AUTOTIMER | SKIP_KEYPRESS | SKIP_MOUSECLICK));
}
