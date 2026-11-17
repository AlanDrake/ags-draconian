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
#include "game/dynobj/cc_audio_clip.h"

#include "game/audio/audio_clip.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "script/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

const char *CCAudioClip::GetType()
{
    return "AudioClip";
}

size_t CCAudioClip::CalcSerializeSize(const void * /*address*/)
{
    return sizeof(int32_t);
}

void CCAudioClip::Serialize(const void *address, Stream *out)
{
    const AudioClip *ach = static_cast<const AudioClip *>(address);
    out->WriteInt32(ach->ID);
}

void CCAudioClip::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    int id = in->ReadInt32();
    ccRegisterUnserializedPersistentObject(index, &Game::Current().AudioClips[id], this);
}
