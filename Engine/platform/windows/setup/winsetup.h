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
// Built-in setup dialog for Windows version
//
//=============================================================================

#pragma once

#include "platform/base/agsplatformdriver.h"
#include "util/ini_util.h"

namespace AGS
{
namespace Engine
{


SetupReturnValue WinSetup(const Common::ConfigTree &cfg_in, const Common::ConfigTree &def_cfg_in,
                          Common::ConfigTree &cfg_out, const Common::String &game_data_dir,
                          const Common::String &version_str);

} // namespace Engine
} // namespace AGS
