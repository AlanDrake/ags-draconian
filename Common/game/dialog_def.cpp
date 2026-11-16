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
#include "game/dialog_def.h"

#include <algorithm>

#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;

void DialogDef::ReadFromFile_v321(Stream *in)
{
    std::vector<DialogOptionDef> options;
    options.resize(LEGACY_MAXTOPICOPTIONS);
    char name[150];
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        in->Read(name, 150);
        options[i].OriginalText.SetString(name, 150);
        options[i].Text = options[i].OriginalText;
    }
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        options[i].Flags = in->ReadInt32();
    }
    in->ReadInt32(); // unused, was optionscripts 32-bit pointer
    for (size_t i = 0u; i < LEGACY_MAXTOPICOPTIONS; ++i)
    {
        options[i].EntryPoint = in->ReadInt16();
    }
    StartEntryPoint = in->ReadInt16();
    CodeSize = in->ReadInt16();
    uint32_t option_count = in->ReadInt32();
    Flags = in->ReadInt32();

    option_count = std::min<uint32_t>(option_count, options.size());
    Options.resize(option_count);
    std::copy_n(options.begin(), option_count, Options.begin());
}

void DialogDef::ReadFromFile_v363(Stream *in)
{
    // Dialog topic settings
    ScriptName = StrUtil::ReadString(in);
    Flags = in->ReadInt32();
    in->ReadInt32(); // reserved
    in->ReadInt32();
    in->ReadInt32();

    // Options
    uint32_t option_count = in->ReadInt32();
    Options.resize(option_count);
    for (auto &opt : Options)
    {
        opt.OriginalText = StrUtil::ReadString(in);
        opt.Text = opt.OriginalText;
        opt.Flags = in->ReadInt32();
        in->ReadInt32(); // reserved
        in->ReadInt32();
        in->ReadInt32();
    }
}
