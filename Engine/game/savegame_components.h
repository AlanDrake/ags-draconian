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

#include "game/savegame.h"
#include "util/stream.h"

namespace AGS
{

namespace Common
{
struct Interaction;
}

namespace Engine
{

using Common::Stream;
using Common::Interaction;

struct PreservedParams;
struct RestoredData;

namespace SavegameComponents
{
// Reads the components of the stream that select_cmp selects into the game state and r_data;
// the others are skipped. pp holds the state kept from before the restore. Stops at the first
// component that fails, and returns its error with the component name, version and offset.
HSaveError ReadAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                   RestoredData &r_data);
// Prescans the selected components that have a prescan handler (the others are skipped), gathering
// data counts into r_data and asserting that they match the game; does *not* keep any actual game data.
HSaveError PrescanAll(Stream *in, SavegameVersion svg_version, SaveCmpSelection select_cmp, const PreservedParams &pp,
                      RestoredData &r_data);
// Writes the common components that select_cmp selects to the stream, each one deflated if compress is set;
// the stream also gets the component list tags. Stops at the first component that fails.
HSaveError WriteAllCommon(Stream *out, SaveCmpSelection select_cmp, bool compress);
} // namespace SavegameComponents

} // namespace Engine
} // namespace AGS
