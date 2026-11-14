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
// Script bindings registered by RegisterDialogOptionsRenderingAPI().
//
//=============================================================================
#include "game/dialog/dialog_options_rendering.h"
#include "game/game.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

void RegisterDialogOptionsRenderingAPI()
{
    ScFnRegister dialogopt_api[] = {
        ScMethod<&DialogOptionsRendering_Update>("DialogOptionsRenderingInfo::Update^0"),
        ScMethod<&DialogOptionsRendering_RunActiveOption>("DialogOptionsRenderingInfo::RunActiveOption^0"),
        ScMethod<&DialogOptionsRendering_GetActiveOptionID>("DialogOptionsRenderingInfo::get_ActiveOptionID"),
        ScMethod<&DialogOptionsRendering_SetActiveOptionID>("DialogOptionsRenderingInfo::set_ActiveOptionID"),
        ScMethod<&DialogOptionsRendering_GetDialogToRender>("DialogOptionsRenderingInfo::get_DialogToRender"),
        ScMethod<&DialogOptionsRendering_GetHeight>("DialogOptionsRenderingInfo::get_Height"),
        ScMethod<&DialogOptionsRendering_SetHeight>("DialogOptionsRenderingInfo::set_Height"),
        ScMethod<&DialogOptionsRendering_GetParserTextboxX>("DialogOptionsRenderingInfo::get_ParserTextBoxX"),
        ScMethod<&DialogOptionsRendering_SetParserTextboxX>("DialogOptionsRenderingInfo::set_ParserTextBoxX"),
        ScMethod<&DialogOptionsRendering_GetParserTextboxY>("DialogOptionsRenderingInfo::get_ParserTextBoxY"),
        ScMethod<&DialogOptionsRendering_SetParserTextboxY>("DialogOptionsRenderingInfo::set_ParserTextBoxY"),
        ScMethod<&DialogOptionsRendering_GetParserTextboxWidth>("DialogOptionsRenderingInfo::get_ParserTextBoxWidth"),
        ScMethod<&DialogOptionsRendering_SetParserTextboxWidth>("DialogOptionsRenderingInfo::set_ParserTextBoxWidth"),
        ScMethod<&DialogOptionsRendering_GetSurface>("DialogOptionsRenderingInfo::get_Surface"),
        ScMethod<&DialogOptionsRendering_GetWidth>("DialogOptionsRenderingInfo::get_Width"),
        ScMethod<&DialogOptionsRendering_SetWidth>("DialogOptionsRenderingInfo::set_Width"),
        ScMethod<&DialogOptionsRendering_GetX>("DialogOptionsRenderingInfo::get_X"),
        ScMethod<&DialogOptionsRendering_SetX>("DialogOptionsRenderingInfo::set_X"),
        ScMethod<&DialogOptionsRendering_GetY>("DialogOptionsRenderingInfo::get_Y"),
        ScMethod<&DialogOptionsRendering_SetY>("DialogOptionsRenderingInfo::set_Y"),
    };

    ccAddExternalFunctions(dialogopt_api);
}
