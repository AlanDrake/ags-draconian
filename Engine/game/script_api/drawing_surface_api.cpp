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
#include "ac/drawingsurface.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/gamestate.h"
#include "ac/translation.h"
#include "gfx/gfx_def.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// void (ScriptDrawingSurface *surface, int colour)
RuntimeScriptValue Sc_DrawingSurface_Clear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_Clear);
}

// ScriptDrawingSurface* (ScriptDrawingSurface *surface)
RuntimeScriptValue Sc_DrawingSurface_CreateCopy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDrawingSurface, ScriptDrawingSurface, DrawingSurface_CreateCopy);
}

RuntimeScriptValue Sc_DrawingSurface_DrawArc(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawArc);
}

RuntimeScriptValue Sc_DrawingSurface_DrawCircle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDrawingSurface, DrawingSurface_DrawCircle);
}

RuntimeScriptValue Sc_DrawingSurface_DrawCircleOutline(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDrawingSurface, DrawingSurface_DrawCircleOutline);
}

RuntimeScriptValue Sc_DrawingSurface_DrawEllipse(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawEllipse);
}

RuntimeScriptValue Sc_DrawingSurface_DrawEllipseOutline(void *self, const RuntimeScriptValue *params,
                                                        int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawEllipseOutline);
}

RuntimeScriptValue Sc_DrawingSurface_DrawImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawImage((ScriptDrawingSurface *)self, params[0].IValue, params[1].IValue, params[2].IValue,
                             params[3].IValue, params[4].IValue, params[5].IValue, params[6].IValue, params[7].IValue,
                             params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_BlendImage(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 11);
    DrawingSurface_BlendImage((ScriptDrawingSurface *)self, params[0].IValue, params[1].IValue, params[2].IValue,
                              (BlendMode)params[3].IValue, params[4].IValue, params[5].IValue, params[6].IValue,
                              params[7].IValue, params[8].IValue, params[9].IValue, params[10].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptDrawingSurface *surface, int fromx, int fromy, int tox, int toy, int thickness)
RuntimeScriptValue Sc_DrawingSurface_DrawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawLine);
}

RuntimeScriptValue Sc_DrawingSurface_DrawPie(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawPie);
}

RuntimeScriptValue Sc_DrawingSurface_DrawPieOutline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDrawingSurface, DrawingSurface_DrawPieOutline);
}

// void (ScriptDrawingSurface *surface, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_DrawPixel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDrawingSurface, DrawingSurface_DrawPixel);
}

RuntimeScriptValue Sc_DrawingSurface_DrawRectangle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawRectangle);
}

RuntimeScriptValue Sc_DrawingSurface_DrawRectangleOutline(void *self, const RuntimeScriptValue *params,
                                                          int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDrawingSurface, DrawingSurface_DrawRectangleOutline);
}

// void (ScriptDrawingSurface *surface, int x, int y, int font, const char* text, ...)
RuntimeScriptValue Sc_DrawingSurface_DrawString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(DrawingSurface_DrawString, 4);
    DrawingSurface_DrawString((ScriptDrawingSurface *)self, params[0].IValue, params[1].IValue, params[2].IValue,
                              scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_DrawStringWrapped(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(DrawingSurface_DrawString, 6);
    DrawingSurface_DrawStringWrapped((ScriptDrawingSurface *)self, params[0].IValue, params[1].IValue, params[2].IValue,
                                     params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_DrawSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_DrawSurface((ScriptDrawingSurface *)self, (ScriptDrawingSurface *)params[0].Ptr, params[1].IValue,
                               params[2].IValue, params[3].IValue, params[4].IValue, params[5].IValue, params[6].IValue,
                               params[7].IValue, params[8].IValue, params[9].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_BlendSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(METHOD, 10);
    DrawingSurface_BlendSurface((ScriptDrawingSurface *)self, (ScriptDrawingSurface *)params[0].Ptr,
                                (BlendMode)params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue,
                                params[5].IValue, params[6].IValue, params[7].IValue, params[8].IValue,
                                params[9].IValue, params[10].IValue);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_DrawingSurface_DrawTriangle(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawTriangle);
}

RuntimeScriptValue Sc_DrawingSurface_DrawTriangleOutline(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptDrawingSurface, DrawingSurface_DrawTriangleOutline);
}

RuntimeScriptValue Sc_DrawingSurface_FloodFill(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDrawingSurface, DrawingSurface_FloodFill);
}

// int (ScriptDrawingSurface *surface, int x, int y)
RuntimeScriptValue Sc_DrawingSurface_GetPixel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(ScriptDrawingSurface, DrawingSurface_GetPixel);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDrawingSurface, DrawingSurface_SetPixel);
}

RuntimeScriptValue Sc_DrawingSurface_GetPixelsCopy(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptDrawingSurface, void, globalDynamicArray, DrawingSurface_GetPixelsCopy);
}

RuntimeScriptValue Sc_DrawingSurface_GetPixelsCopy32(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT4(ScriptDrawingSurface, void, globalDynamicArray, DrawingSurface_GetPixelsCopy32);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixels(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptDrawingSurface, DrawingSurface_SetPixels, void);
}

RuntimeScriptValue Sc_DrawingSurface_SetPixels32(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptDrawingSurface, DrawingSurface_SetPixels32, void);
}

// void (ScriptDrawingSurface* surface)
RuntimeScriptValue Sc_DrawingSurface_Release(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDrawingSurface, DrawingSurface_Release);
}

// int (ScriptDrawingSurface *surface)
RuntimeScriptValue Sc_DrawingSurface_GetDrawingColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetDrawingColor);
}

// void (ScriptDrawingSurface *surface, int new_colour)
RuntimeScriptValue Sc_DrawingSurface_SetDrawingColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetDrawingColor);
}

RuntimeScriptValue Sc_DrawingSurface_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetBlendMode);
}

RuntimeScriptValue Sc_DrawingSurface_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDrawingSurface, DrawingSurface_SetBlendMode);
}

// int (ScriptDrawingSurface *surface)
RuntimeScriptValue Sc_DrawingSurface_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetHeight);
}

RuntimeScriptValue Sc_DrawingSurface_GetValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptDrawingSurface, DrawingSurface_GetValid);
}

// int (ScriptDrawingSurface *surface)
RuntimeScriptValue Sc_DrawingSurface_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetWidth);
}

RuntimeScriptValue Sc_DrawingSurface_GetColorDepth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDrawingSurface, DrawingSurface_GetColorDepth);
}

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
        {"DrawingSurface::Clear^1", API_FN_PAIR(DrawingSurface_Clear)},
        {"DrawingSurface::CreateCopy^0", API_FN_PAIR(DrawingSurface_CreateCopy)},
        {"DrawingSurface::DrawArc^5", API_FN_PAIR(DrawingSurface_DrawArc)},
        {"DrawingSurface::DrawCircle^3", API_FN_PAIR(DrawingSurface_DrawCircle)},
        {"DrawingSurface::DrawCircleOutline^3", API_FN_PAIR(DrawingSurface_DrawCircleOutline)},
        {"DrawingSurface::DrawEllipse^4", API_FN_PAIR(DrawingSurface_DrawEllipse)},
        {"DrawingSurface::DrawEllipseOutline^4", API_FN_PAIR(DrawingSurface_DrawEllipseOutline)},
        {"DrawingSurface::DrawImage^10", API_FN_PAIR(DrawingSurface_DrawImage)},
        {"DrawingSurface::DrawLine^5", API_FN_PAIR(DrawingSurface_DrawLine)},
        {"DrawingSurface::DrawPie^5", API_FN_PAIR(DrawingSurface_DrawPie)},
        {"DrawingSurface::DrawPieOutline^5", API_FN_PAIR(DrawingSurface_DrawPieOutline)},
        {"DrawingSurface::DrawPixel^2", API_FN_PAIR(DrawingSurface_DrawPixel)},
        {"DrawingSurface::DrawRectangle^4", API_FN_PAIR(DrawingSurface_DrawRectangle)},
        {"DrawingSurface::DrawRectangleOutline^4", API_FN_PAIR(DrawingSurface_DrawRectangleOutline)},
        {"DrawingSurface::DrawString^104", Sc_DrawingSurface_DrawString, ScPl_DrawingSurface_DrawString},
        // old non-variadic and new variadic variants
        {"DrawingSurface::DrawStringWrapped^6", API_FN_PAIR(DrawingSurface_DrawStringWrapped)},
        {"DrawingSurface::DrawStringWrapped^106", Sc_DrawingSurface_DrawStringWrapped,
         ScPl_DrawingSurface_DrawStringWrapped},
        {"DrawingSurface::DrawSurface^10", API_FN_PAIR(DrawingSurface_DrawSurface)},
        {"DrawingSurface::DrawTriangle^6", API_FN_PAIR(DrawingSurface_DrawTriangle)},
        {"DrawingSurface::DrawTriangleOutline^6", API_FN_PAIR(DrawingSurface_DrawTriangleOutline)},
        {"DrawingSurface::FloodFill^2", API_FN_PAIR(DrawingSurface_FloodFill)},
        {"DrawingSurface::GetPixel^2", API_FN_PAIR(DrawingSurface_GetPixel)},
        {"DrawingSurface::GetPixelsCopy^4", API_FN_PAIR(DrawingSurface_GetPixelsCopy)},
        {"DrawingSurface::GetPixelsCopy32^4", API_FN_PAIR(DrawingSurface_GetPixelsCopy32)},
        {"DrawingSurface::SetPixel^3", API_FN_PAIR(DrawingSurface_SetPixel)},
        {"DrawingSurface::SetPixels^5", API_FN_PAIR(DrawingSurface_SetPixels)},
        {"DrawingSurface::SetPixels32^5", API_FN_PAIR(DrawingSurface_SetPixels32)},
        {"DrawingSurface::Release^0", API_FN_PAIR(DrawingSurface_Release)},
        {"DrawingSurface::get_DrawingColor", API_FN_PAIR(DrawingSurface_GetDrawingColor)},
        {"DrawingSurface::set_DrawingColor", API_FN_PAIR(DrawingSurface_SetDrawingColor)},
        {"DrawingSurface::get_BlendMode", API_FN_PAIR(DrawingSurface_GetBlendMode)},
        {"DrawingSurface::set_BlendMode", API_FN_PAIR(DrawingSurface_SetBlendMode)},
        {"DrawingSurface::get_ColorDepth", API_FN_PAIR(DrawingSurface_GetColorDepth)},
        {"DrawingSurface::get_Height", API_FN_PAIR(DrawingSurface_GetHeight)},
        {"DrawingSurface::get_Valid", API_FN_PAIR(DrawingSurface_GetValid)},
        {"DrawingSurface::get_Width", API_FN_PAIR(DrawingSurface_GetWidth)},

        {"DrawingSurface::BlendImage^11", API_FN_PAIR(DrawingSurface_BlendImage)},
        {"DrawingSurface::BlendSurface^11", API_FN_PAIR(DrawingSurface_BlendSurface)},
    };

    ccAddExternalFunctions(drawsurf_api);
}
