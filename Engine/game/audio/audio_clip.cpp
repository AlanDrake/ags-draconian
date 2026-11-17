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
#include "game/audio_clip_type_def.h"
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
#include "util/field_table.h"

using namespace AGS::Common;

namespace
{

// FromDefinition: a save does not hold the values set during play, and the clip has the definition's values again
// whenever a game is started or restored (a restore clears them, DoBeforeRestore)
constexpr FieldRow<AudioClipState> kAudioClipStateFields[] = {
    Field<&AudioClipState::PropertyOverrides>("PropertyOverrides", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kAudioClipStateFields), "AudioClipState: a field has no row in kAudioClipStateFields");

} // namespace

FieldTable<AudioClipState> AudioClipState::Fields()
{
    return kAudioClipStateFields;
}

int AudioClip_GetID(AudioClip *clip)
{
    return clip->ID;
}

const char *AudioClip_GetScriptName(AudioClip *clip)
{
    return CreateNewScriptString(clip->ScriptName);
}

int AudioClip_GetDefaultPriority(AudioClip *clip)
{
    return clip->DefaultPriority;
}

int AudioClip_GetDefaultRepeat(AudioClip *clip)
{
    return clip->IsRepeatingByDefault;
}

int AudioClip_GetDefaultVolume(AudioClip *clip)
{
    return clip->DefaultVolume;
}

int AudioClip_GetFileType(AudioClip *clip)
{
    return clip->FileType;
}

int AudioClip_GetType(AudioClip *clip)
{
    return clip->Type;
}

int AudioClip_GetIsAvailable(AudioClip *clip)
{
    return EngineServices::Current().Assets->DoesAssetExist(
               get_audio_clip_assetpath(clip->BundlingType, clip->FileName))
               ? 1
               : 0;
}

void AudioClip_Stop(AudioClip *clip)
{
    Game &game = Game::Current();
    for (int i = NUM_SPEECH_CHANS; i < game.GameChannelCount; i++)
    {
        auto *playback = AudioChans::GetChannelIfPlaying(i);
        if ((playback != nullptr) && (playback->sourceClipID == clip->ID))
        {
            AudioChannel_Stop(&game.ScAudioChannels[i]);
        }
    }
}

ScriptAudioChannel *AudioClip_Play(AudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, false);
}

ScriptAudioChannel *AudioClip_PlayFrom(AudioClip *clip, int position, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, position, false);
}

ScriptAudioChannel *AudioClip_PlayQueued(AudioClip *clip, int priority, int repeat)
{
    return play_audio_clip(clip, priority, repeat, 0, true);
}

ScriptAudioChannel *AudioClip_PlayOnChannel(AudioClip *clip, int chan, int priority, int repeat)
{
    Game &game = Game::Current();
    if (chan < NUM_SPEECH_CHANS || chan >= game.GameChannelCount)
        quitprintf("!AudioClip.PlayOnChannel: invalid channel %d, the range is %d - %d", chan, NUM_SPEECH_CHANS,
                   game.GameChannelCount - 1);
    if (priority == SCR_NO_VALUE)
        priority = clip->DefaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->IsRepeatingByDefault != 0;
    return play_audio_clip_on_channel(clip, chan, priority, repeat);
}

ScriptAudioChannel *AudioClip_PlayAsType(AudioClip *clip, int type, int chan, int priority, int repeat)
{
    Game &game = Game::Current();
    if (type <= AUDIOTYPE_SPEECH || static_cast<uint32_t>(type) >= game.AudioClipTypes.size())
        quitprintf("!AudioClip.PlayAsType: invalid audio type %d, the range is %u - %u", type, AUDIOTYPE_SPEECH + 1,
                   static_cast<uint32_t>(game.AudioClipTypes.size() - 1));
    if (chan != SCR_NO_VALUE && (chan < NUM_SPEECH_CHANS || chan >= game.GameChannelCount))
        quitprintf("!AudioClip.PlayAsType: invalid channel %d, the range is %d - %d", chan, NUM_SPEECH_CHANS,
                   game.GameChannelCount - 1);
    if (chan == SCR_NO_VALUE)
        chan = -1;
    if (priority == SCR_NO_VALUE)
        priority = clip->DefaultPriority;
    if (repeat == SCR_NO_VALUE)
        repeat = clip->IsRepeatingByDefault != 0;
    return play_audio_clip(AudioPlayback(clip, type), chan, priority, repeat);
}

int AudioClip_GetProperty(AudioClip *clip, const char *property)
{
    return get_int_property(Game::Current().AudioClipProperties[clip->ID], clip->PropertyOverrides, property);
}

const char *AudioClip_GetTextProperty(AudioClip *clip, const char *property)
{
    return get_text_property_dynamic_string(Game::Current().AudioClipProperties[clip->ID], clip->PropertyOverrides,
                                            property);
}

bool AudioClip_SetProperty(AudioClip *clip, const char *property, int value)
{
    return set_int_property(clip->PropertyOverrides, property, value);
}

bool AudioClip_SetTextProperty(AudioClip *clip, const char *property, const char *value)
{
    return set_text_property(clip->PropertyOverrides, property, value);
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "script/script_runtime.h"

AudioClip *AudioClip_GetByName(const char *name)
{
    return static_cast<AudioClip *>(ccGetScriptObjectAddress(name, ccDynamicAudioClip.GetType()));
}
