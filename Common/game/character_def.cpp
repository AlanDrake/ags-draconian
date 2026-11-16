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
#include "game/character_def.h"

#include <cstring>
#include <utility>
#include <vector>

#include "game/game_version.h"
#include "util/stream.h"

using namespace AGS::Common;

void CharacterDef::set_item_quantity(int item_id, int quant)
{
    if (quant <= 0)
    {
        if (InventoryQuantity.count(item_id) > 0)
            InventoryQuantity.erase(item_id);
    }
    else
    {
        InventoryQuantity[item_id] = quant;
    }
}

/* static */ ScriptEventSchema CharacterDef::_eventSchema = {{
    {"OnAnyClick", kCharacterEvent_AnyClick},
    {"OnFrameEvent", kCharacterEvent_OnFrameEvent},
}};

void CharacterDef::RemapOldInteractions()
{
    std::vector<ScriptEventHandler> old_interactions = Interactions.GetHandlers();
    std::vector<ScriptEventHandler> new_interactions;
    // this is just for safety, it's supposed to be that large
    old_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions.resize(NUM_STANDARD_VERBS);
    new_interactions[STD_MODE_WALK] = {};
    new_interactions[STD_MODE_LOOK] = old_interactions[0];
    new_interactions[STD_MODE_HAND] = old_interactions[1];
    new_interactions[STD_MODE_TALK] = old_interactions[2];
    new_interactions[STD_MODE_USE] = old_interactions[3];
    new_interactions[STD_MODE_PICKUP] = old_interactions[5];
    new_interactions[STD_MODE_CUSTOM1] = old_interactions[6];
    new_interactions[STD_MODE_CUSTOM2] = old_interactions[7];

    _events.SetHandler(kCharacterEvent_AnyClick, old_interactions[4].FunctionName);
    Interactions.SetScriptModule(_events.GetScriptModule());
    Interactions.SetHandlers(std::move(new_interactions));
}

void CharacterDef::ReadFromFile(Stream *in, GameDataVersion data_ver)
{
    // Base fields (original game format)
    // NOTE: some of the fields below are dummy slots,
    // that are result of a old engine dumping a full data struct into the stream.
    // Some of the corresponding fields are not supposed to be set at design time.
    // All of the [UNUSED] may in theory be reused for other purposes.

    NormalView = in->ReadInt32();
    SpeechView = in->ReadInt32();
    View = in->ReadInt32();
    Room = in->ReadInt32();
    in->ReadInt32(); // [UNUSED] (prevroom)
    X = in->ReadInt32();
    Y = in->ReadInt32();
    in->ReadInt32(); // [UNUSED] (wait)
    Flags = in->ReadInt32();
    in->ReadInt16(); // [UNUSED] (following)
    in->ReadInt16(); // [UNUSED] (follow_info)
    IdleView = in->ReadInt32();
    IdleDelay = in->ReadInt16();
    in->ReadInt16(); // [UNUSED] (idleleft)
    Opacity = in->ReadInt16();
    Baseline = in->ReadInt16();
    in->ReadInt32(); // [UNUSED] (activeinv)
    SpeechColor = in->ReadInt32();
    ThinkView = in->ReadInt32();
    BlinkView = in->ReadInt16();
    in->ReadInt16(); // [UNUSED] (blinkinterval)
    in->ReadInt16(); // [UNUSED] (blinktimer)
    in->ReadInt16(); // [UNUSED] (blinkframe)
    WalkSpeedY = in->ReadInt16();
    in->ReadInt16(); // [UNUSED] (pic_yoffs)
    in->ReadInt32(); // [RESERVED] (z)
    in->ReadInt32(); // [UNUSED] (walkwait)
    SpeechAnimationDelay = in->ReadInt16();
    IdleAnimationDelay = in->ReadInt16();
    BlockingWidth = in->ReadInt16();
    BlockingHeight = in->ReadInt16();
    ID = in->ReadInt32(); // e.q. Character.ID, reset by the engine, but reserved here
    BlockingX = in->ReadInt16();
    BlockingY = in->ReadInt16();
    in->ReadInt16(); // [UNUSED] (loop)
    in->ReadInt16(); // [UNUSED] (frame)
    in->ReadInt16(); // [UNUSED] (walking)
    in->ReadInt16(); // [UNUSED] (animating)
    WalkSpeedX = in->ReadInt16();
    AnimationSpeed = in->ReadInt16();
    if (data_ver < kGameVersion_400_33)
    {
        int16_t legacy_invq[LEGACY_MAX_INV];
        in->ReadArrayOfInt16(legacy_invq, LEGACY_MAX_INV);
        for (int i = 0; i < LEGACY_MAX_INV; ++i)
            if (legacy_invq[i] > 0)
                InventoryQuantity[i] = legacy_invq[i];
    }
    in->ReadInt16(); // [UNUSED] (actx)
    in->ReadInt16(); // [UNUSED] (acty)

    Name.ReadCount(in, LEGACY_MAX_CHAR_NAME_LEN);
    ScriptName.ReadCount(in, LEGACY_MAX_SCRIPT_NAME_LEN);
    uint8_t on = in->ReadInt8(); // old enabled + visible flag
    // Only treat "on" as a visible flag, otherwise it breaks game logic
    // when reading older games (i.e. to make character visible you must turn
    // both enabled and visible properties).
    Flags |= (CHF_VISIBLE)*on;
    in->ReadInt8(); // alignment padding to int32
}

void CharacterDef::WriteToFile(Stream *out) const
{
    // Base fields (original game format)
    // NOTE: some of the fields below are dummy slots,
    // that are result of a old engine dumping a full data struct into the stream.
    // Some of the corresponding fields are not supposed to be set at design time.
    // All of the [UNUSED] may in theory be reused for other purposes.

    out->WriteInt32(NormalView);
    out->WriteInt32(SpeechView);
    out->WriteInt32(View);
    out->WriteInt32(Room);
    out->WriteInt32(0); // [UNUSED] (prevroom)
    out->WriteInt32(X);
    out->WriteInt32(Y);
    out->WriteInt32(0); // [UNUSED] (wait)
    out->WriteInt32(Flags);
    out->WriteInt16(0); // [UNUSED] (following)
    out->WriteInt16(0); // [UNUSED] (follow_info)
    out->WriteInt32(IdleView);
    out->WriteInt16(IdleDelay);
    out->WriteInt16(0); // [UNUSED] (idleleft)
    out->WriteInt16(Opacity);
    out->WriteInt16(Baseline);
    out->WriteInt32(0); // [UNUSED] (activeinv)
    out->WriteInt32(SpeechColor);
    out->WriteInt32(ThinkView);
    out->WriteInt16(BlinkView);
    out->WriteInt16(0); // [UNUSED] blinkinterval
    out->WriteInt16(0); // [UNUSED] blinktimer
    out->WriteInt16(0); // [UNUSED] blinkframe
    out->WriteInt16(WalkSpeedY);
    out->WriteInt16(0); // [UNUSED] pic_yoffs
    out->WriteInt32(0); // [RESERVED] z
    out->WriteInt32(0); // [UNUSED] walkwait
    out->WriteInt16(SpeechAnimationDelay);
    out->WriteInt16(IdleAnimationDelay);
    out->WriteInt16(BlockingWidth);
    out->WriteInt16(BlockingHeight);
    out->WriteInt32(ID); // Character.ID
    out->WriteInt16(BlockingX);
    out->WriteInt16(BlockingY);
    out->WriteInt16(0); // [UNUSED] loop
    out->WriteInt16(0); // [UNUSED] frame
    out->WriteInt16(0); // [UNUSED] walking
    out->WriteInt16(0); // [UNUSED] animating
    out->WriteInt16(WalkSpeedX);
    out->WriteInt16(AnimationSpeed);
    // skip LEGACY_MAX_INV int16s since kGameVersion_400_33
    out->WriteInt16(0); // [UNUSED] actx
    out->WriteInt16(0); // [UNUSED] acty

    Name.WriteCount(out, LEGACY_MAX_CHAR_NAME_LEN);
    ScriptName.WriteCount(out, LEGACY_MAX_SCRIPT_NAME_LEN);
    out->WriteInt8(0); // [OBSOLETE], old enabled + visible flag
    out->WriteInt8(0); // alignment padding to int32
}
