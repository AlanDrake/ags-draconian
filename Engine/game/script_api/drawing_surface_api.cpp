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
// Script bindings registered by RegisterDrawingSurfaceAPI().
//
//=============================================================================
#include "game/game_state.h"
#include "game/render/drawing_surface.h"
#include "game/script_api/bindings.h"
#include "game/translation.h"
#include "gfx/gfx_def.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_DrawingSurface_DrawString(ScriptDrawingSurface *surface, int x, int y, int font, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    DrawingSurface_DrawString(surface, x, y, font, scsf_buffer);
}

void ScPl_DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *surface, int x, int y, int width, int font,
                                           int alignment, const char *msg, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(msg);
    DrawingSurface_DrawStringWrapped(surface, x, y, width, font, alignment, scsf_buffer);
}

void RegisterDrawingSurfaceAPI(ScriptAPIVersion /*base_api*/, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister drawsurf_api[] = {
        ScMethod<&DrawingSurface_Clear>("DrawingSurface::Clear^1"),
        ScMethod<&DrawingSurface_CreateCopy>("DrawingSurface::CreateCopy^0"),
        ScMethod<&DrawingSurface_DrawArc>("DrawingSurface::DrawArc^5"),
        ScMethod<&DrawingSurface_DrawCircle>("DrawingSurface::DrawCircle^3"),
        ScMethod<&DrawingSurface_DrawCircleOutline>("DrawingSurface::DrawCircleOutline^3"),
        ScMethod<&DrawingSurface_DrawEllipse>("DrawingSurface::DrawEllipse^4"),
        ScMethod<&DrawingSurface_DrawEllipseOutline>("DrawingSurface::DrawEllipseOutline^4"),
        ScMethod<&DrawingSurface_DrawImage>("DrawingSurface::DrawImage^10"),
        ScMethod<&DrawingSurface_DrawLine>("DrawingSurface::DrawLine^5"),
        ScMethod<&DrawingSurface_DrawPie>("DrawingSurface::DrawPie^5"),
        ScMethod<&DrawingSurface_DrawPieOutline>("DrawingSurface::DrawPieOutline^5"),
        ScMethod<&DrawingSurface_DrawPixel>("DrawingSurface::DrawPixel^2"),
        ScMethod<&DrawingSurface_DrawRectangle>("DrawingSurface::DrawRectangle^4"),
        ScMethod<&DrawingSurface_DrawRectangleOutline>("DrawingSurface::DrawRectangleOutline^4"),
        ScMethodFormat<&DrawingSurface_DrawString>("DrawingSurface::DrawString^104", ScPl_DrawingSurface_DrawString),
        // old non-variadic and new variadic variants
        ScMethodFormat<&DrawingSurface_DrawStringWrapped>("DrawingSurface::DrawStringWrapped^6",
                                                          DrawingSurface_DrawStringWrapped),
        ScMethodFormat<&DrawingSurface_DrawStringWrapped>("DrawingSurface::DrawStringWrapped^106",
                                                          ScPl_DrawingSurface_DrawStringWrapped),
        ScMethod<&DrawingSurface_DrawSurface>("DrawingSurface::DrawSurface^10"),
        ScMethod<&DrawingSurface_DrawTriangle>("DrawingSurface::DrawTriangle^6"),
        ScMethod<&DrawingSurface_DrawTriangleOutline>("DrawingSurface::DrawTriangleOutline^6"),
        ScMethod<&DrawingSurface_FloodFill>("DrawingSurface::FloodFill^2"),
        ScMethod<&DrawingSurface_GetPixel>("DrawingSurface::GetPixel^2"),
        ScMethod<&DrawingSurface_GetPixelsCopy, ScManagedBy<globalDynamicArray>>("DrawingSurface::GetPixelsCopy^4"),
        ScMethod<&DrawingSurface_GetPixelsCopy32, ScManagedBy<globalDynamicArray>>("DrawingSurface::GetPixelsCopy32^4"),
        ScMethod<&DrawingSurface_SetPixel>("DrawingSurface::SetPixel^3"),
        ScMethod<&DrawingSurface_SetPixels>("DrawingSurface::SetPixels^5"),
        ScMethod<&DrawingSurface_SetPixels32>("DrawingSurface::SetPixels32^5"),
        ScMethod<&DrawingSurface_Release>("DrawingSurface::Release^0"),
        ScMethod<&DrawingSurface_GetDrawingColor>("DrawingSurface::get_DrawingColor"),
        ScMethod<&DrawingSurface_SetDrawingColor>("DrawingSurface::set_DrawingColor"),
        ScMethod<&DrawingSurface_GetBlendMode>("DrawingSurface::get_BlendMode"),
        ScMethod<&DrawingSurface_SetBlendMode>("DrawingSurface::set_BlendMode"),
        ScMethod<&DrawingSurface_GetColorDepth>("DrawingSurface::get_ColorDepth"),
        ScMethod<&DrawingSurface_GetHeight>("DrawingSurface::get_Height"),
        ScMethod<&DrawingSurface_GetValid>("DrawingSurface::get_Valid"),
        ScMethod<&DrawingSurface_GetWidth>("DrawingSurface::get_Width"),

        ScMethod<&DrawingSurface_BlendImage>("DrawingSurface::BlendImage^11"),
        ScMethod<&DrawingSurface_BlendSurface>("DrawingSurface::BlendSurface^11"),
    };

    ccAddExternalFunctions(drawsurf_api);
}
