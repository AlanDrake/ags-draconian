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
#include "game/audio/audio_clip.h"

#include "data/assetmanager.h"
#include "game/audio/audio.h"
#include "game/audio/audio_channel.h"
#include "game/dynobj/cc_audio_clip.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/lib/string.h"
#include "game/world/properties.h"
#include "media/audio/audio_system.h"
#include "platform/base/engine_services.h"
#include "script/runtimescriptvalue.h"
#include "util/common.h" // quitprintf

using namespace AGS::Common;

int AudioClip_GetID(ScriptAudioClip *clip)
{
    return clip->id;
}

const char *AudioClip_GetScriptName(ScriptAudioClip *clip)
{
    return CreateNewScriptString(clip->scriptName);
}

int AudioClip_GetDefaultPriority(ScriptAudioClip *clip)
{
    return clip->defaultPriority;
}

int AudioClip_GetDefaultRepeat(ScriptAudioClip *clip)
{
    return clip->defaultRepeat;
}

int AudioClip_GetDefaultVolume(ScriptAudioClip *clip)
{
    return clip->defaultVolume;
}

int AudioClip_GetFileType(ScriptAudioClip *clip)
{
    return clip->fileType;
}

int AudioClip_GetType(ScriptAudioClip *clip)
{
    return clip->type;
}

int AudioClip_GetIsAvailable(ScriptAudioClip *clip)
{
    return EngineServices::Current().Assets->DoesAssetExist(
               get_audio_clip_assetpath(clip->bundlingType, clip->fileName))
               ? 1
               : 0;
}

void AudioClip_Stop(ScriptAudioClip *clip)
{
    Game &game = Game::Current();
    for (int i = NUM_SPEECH_CHANS; i < game.numGameChannels; i++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        if ((playback != nullptr) && (playback->sourceClipID == clip->id))
        {
            AudioChannel_Stop(&scrAudioChannel[i]);
        }
    }
}

ScriptAudioChannel *AudioClip_Play(ScriptAudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, false);
}

ScriptAudioChannel *AudioClip_PlayFrom(ScriptAudioClip *clip, int position, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, position, false);
}

ScriptAudioChannel *AudioClip_PlayQueued(ScriptAudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, true);
}

ScriptAudioChannel *AudioClip_PlayOnChannel(ScriptAudioClip *clip, int chan, int priority, int repeat)
{
    Game &game = Game::Current();
    if (chan < NUM_SPEECH_CHANS || chan >= game.numGameChannels)
        quitprintf("!AudioClip.PlayOnChannel: invalid channel %d, the range is %d - %d", chan, NUM_SPEECH_CHANS,
                   game.numGameChannels - 1);
    if (priority == SCR_NO_VALUE)
        priority = clip->defaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->defaultRepeat;
    return play_audio_clip_on_channel(clip, chan, priority, repeat);
}

ScriptAudioChannel *AudioClip_PlayAsType(ScriptAudioClip *clip, int type, int chan, int priority, int repeat)
{
    Game &game = Game::Current();
    if (type <= AUDIOTYPE_SPEECH || static_cast<uint32_t>(type) >= game.audioClipTypes.size())
        quitprintf("!AudioClip.PlayAsType: invalid audio type %d, the range is %u - %u", type, AUDIOTYPE_SPEECH + 1,
                   static_cast<uint32_t>(game.audioClipTypes.size() - 1));
    if (chan != SCR_NO_VALUE && (chan < NUM_SPEECH_CHANS || chan >= game.numGameChannels))
        quitprintf("!AudioClip.PlayAsType: invalid channel %d, the range is %d - %d", chan, NUM_SPEECH_CHANS,
                   game.numGameChannels - 1);
    if (chan == SCR_NO_VALUE)
        chan = -1;
    if (priority == SCR_NO_VALUE)
        priority = clip->defaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->defaultRepeat;
    return play_audio_clip(AudioPlayback(clip, type), chan, priority, repeat);
}

int AudioClip_GetProperty(ScriptAudioClip *clip, const char *property)
{
    Game &game = Game::Current();
    return get_int_property(game.audioclipProps[clip->id], game.Play.audioclipProps[clip->id], property);
}

const char *AudioClip_GetTextProperty(ScriptAudioClip *clip, const char *property)
{
    Game &game = Game::Current();
    return get_text_property_dynamic_string(game.audioclipProps[clip->id], game.Play.audioclipProps[clip->id],
                                            property);
}

bool AudioClip_SetProperty(ScriptAudioClip *clip, const char *property, int value)
{
    return set_int_property(Game::Current().Play.audioclipProps[clip->id], property, value);
}

bool AudioClip_SetTextProperty(ScriptAudioClip *clip, const char *property, const char *value)
{
    return set_text_property(Game::Current().Play.audioclipProps[clip->id], property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"

ScriptAudioClip *AudioClip_GetByName(const char *name)
{
    return static_cast<ScriptAudioClip *>(ccGetScriptObjectAddress(name, ccDynamicAudioClip.GetType()));
}
