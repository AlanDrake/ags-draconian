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
#include "ac/drawingsurface.h"

#include "ac/common.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/dynamicsprite.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_translation.h"
#include "ac/room.h"
#include "ac/roomobject.h"
#include "ac/roomstatus.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/walkbehind.h"
#include "debug/debug_log.h"
#include "font/fonts.h"
#include "gfx/gfx_def.h"
#include "gfx/gfx_util.h"
#include "gui/guimain.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern RoomStatus *croom;
extern RoomObject *objs;
extern SpriteCache spriteset;

// ** SCRIPT DRAWINGSURFACE OBJECT

void DrawingSurface_Release(ScriptDrawingSurface *surface)
{
    if (surface->roomBackgroundNumber >= 0)
    {
        on_room_bg_surface_release(surface->roomBackgroundNumber, surface->modified);
        surface->roomBackgroundNumber = -1;
    }
    else if (surface->roomMaskType > kRoomAreaNone)
    {
        on_room_mask_surface_release(surface->roomMaskType, surface->modified);
        surface->roomMaskType = kRoomAreaNone;
    }
    else if (surface->dynamicSpriteNumber >= 0)
    {
        on_dynsprite_surface_release(surface->dynamicSpriteNumber, surface->modified);
        surface->dynamicSpriteNumber = -1;
    }
    else if (surface->dynamicSurfaceNumber >= 0)
    {
        dynamicallyCreatedSurfaces[surface->dynamicSurfaceNumber] = nullptr;
        surface->dynamicSurfaceNumber = -1;
    }
    surface->modified = false;
}

Bitmap *AssertBitmapSurface(Bitmap *bmp, const char *api_name)
{
    if (!bmp)
    {
        debug_script_warn(
            "%s: attempted to use surface after its source image was disposed or DrawingSurface.Release() was called.",
            api_name);
    }
    return bmp;
}

ScriptDrawingSurface *DrawingSurface_CreateCopy(ScriptDrawingSurface *surface)
{
    Bitmap *source_bitmap = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.CreateCopy");
    if (!source_bitmap)
    {
        return nullptr;
    }

    for (int i = 0; i < MAX_DYNAMIC_SURFACES; i++)
    {
        if (dynamicallyCreatedSurfaces[i] == nullptr)
        {
            dynamicallyCreatedSurfaces[i].reset(BitmapHelper::CreateBitmapCopy(source_bitmap));
            ScriptDrawingSurface *new_surface = new ScriptDrawingSurface();
            new_surface->dynamicSurfaceNumber = i;
            ccRegisterManagedObject(new_surface, new_surface);
            return new_surface;
        }
    }

    debug_script_warn("!DrawingSurface.CreateCopy: too many surface copies created (max is %d)", MAX_DYNAMIC_SURFACES);
    return nullptr;
}

static void DrawingSurface_DrawImageImpl(ScriptDrawingSurface *surface, Bitmap *src, int dst_x, int dst_y, int alpha,
                                         BlendMode mode, int dst_width, int dst_height, int src_x, int src_y,
                                         int src_width, int src_height, int sprite_id)
{
    Bitmap *ds = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.DrawImage");
    if (!ds)
        return;

    // Drawing a surface onto itself is allowed: the bitmap lib supports it, and it may be used for effects.
    if (src->GetColorDepth() != ds->GetColorDepth())
    {
        if (sprite_id >= 0)
            debug_script_warn("DrawImage: Sprite %d colour depth %d-bit not same as destination depth %d-bit",
                              sprite_id, src->GetColorDepth(), ds->GetColorDepth());
        else
            debug_script_warn("DrawImage: Source image colour depth %d-bit not same as destination depth %d-bit",
                              src->GetColorDepth(), ds->GetColorDepth());
    }
    if ((alpha < 0) || (alpha > 255))
        debug_script_warn("DrawingSurface.DrawImage: invalid alpha %d, range is %d - %d", alpha, 0, 255);
    alpha = Math::Clamp(alpha, 0, 255);

    if (alpha == 0)
        return; // fully transparent
    if (dst_width < 1 || dst_height < 1 || src_width < 1 || src_height < 1)
        return; // invalid src or dest rectangles

    // Setup uninitialized arguments
    if (dst_width == SCR_NO_VALUE)
    {
        dst_width = src->GetWidth();
    }
    if (dst_height == SCR_NO_VALUE)
    {
        dst_height = src->GetHeight();
    }

    if (src_x == SCR_NO_VALUE)
    {
        src_x = 0;
    }
    if (src_y == SCR_NO_VALUE)
    {
        src_y = 0;
    }
    if (src_width == SCR_NO_VALUE)
    {
        src_width = src->GetWidth();
    }
    if (src_height == SCR_NO_VALUE)
    {
        src_height = src->GetHeight();
    }

    if (dst_x >= ds->GetWidth() || dst_x + dst_width <= 0 || dst_y >= ds->GetHeight() || dst_y + dst_height <= 0 ||
        src_x >= src->GetWidth() || src_x + src_width <= 0 || src_y >= src->GetHeight() || src_y + src_height <= 0)
        return; // source or destination rects lie completely off surface
    // Clamp the source rect to the valid limits to prevent exceptions (ignore dest, bitmap drawing deals with that)
    Math::ClampLength(src_x, src_width, 0, src->GetWidth());
    Math::ClampLength(src_y, src_height, 0, src->GetHeight());

    // TODO: possibly optimize by not making a stretched intermediate bitmap
    // if simpler blit/draw_sprite could be called (no translucency with alpha channel).
    std::unique_ptr<Bitmap> conv_src;
    if (dst_width != src->GetWidth() || dst_height != src->GetHeight() || src_width != src->GetWidth() ||
        src_height != src->GetHeight())
    {
        // Resize and/or partial copy specified
        conv_src.reset(BitmapHelper::CreateBitmap(dst_width, dst_height, src->GetColorDepth()));
        conv_src->StretchBlt(src, RectWH(src_x, src_y, src_width, src_height), RectWH(0, 0, dst_width, dst_height));

        src = conv_src.get();
    }

    ds = surface->StartDrawing();

    draw_sprite_support_alpha(ds, dst_x, dst_y, src, mode, alpha);

    surface->FinishedDrawing();
}

void DrawingSurface_DrawImage(ScriptDrawingSurface *surface, int dst_x, int dst_y, int slot, int alpha, int dst_width,
                              int dst_height, int src_x, int src_y, int src_width, int src_height)
{
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("DrawingSurface.DrawImage: invalid sprite slot number specified: %d", slot);
        slot = 0;
    }
    DrawingSurface_DrawImageImpl(surface, spriteset[slot], dst_x, dst_y, alpha, kBlend_Normal, dst_width, dst_height,
                                 src_x, src_y, src_width, src_height, slot);
}

void DrawingSurface_BlendImage(ScriptDrawingSurface *surface, int dst_x, int dst_y, int slot, BlendMode mode, int alpha,
                               int dst_width, int dst_height, int src_x, int src_y, int src_width, int src_height)
{
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("DrawingSurface.BlendImage: invalid sprite slot number specified: %d", slot);
        slot = 0;
    }
    DrawingSurface_DrawImageImpl(surface, spriteset[slot], dst_x, dst_y, alpha, mode, dst_width, dst_height, src_x,
                                 src_y, src_width, src_height, slot);
}

void DrawingSurface_DrawSurface(ScriptDrawingSurface *target, ScriptDrawingSurface *source, int alpha, int dst_x,
                                int dst_y, int dst_width, int dst_height, int src_x, int src_y, int src_width,
                                int src_height)
{
    Bitmap *source_bmp = AssertBitmapSurface(source->GetBitmapSurface(), "DrawingSurface.DrawSurface");
    if (!source_bmp)
        return;

    DrawingSurface_DrawImageImpl(target, source_bmp, dst_x, dst_y, alpha, kBlend_Normal, dst_width, dst_height, src_x,
                                 src_y, src_width, src_height, -1);
}

void DrawingSurface_BlendSurface(ScriptDrawingSurface *target, ScriptDrawingSurface *source, BlendMode mode, int alpha,
                                 int dst_x, int dst_y, int dst_width, int dst_height, int src_x, int src_y,
                                 int src_width, int src_height)
{
    Bitmap *source_bmp = AssertBitmapSurface(source->GetBitmapSurface(), "DrawingSurface.BlendSurface");
    if (!source_bmp)
        return;

    DrawingSurface_DrawImageImpl(target, source_bmp, dst_x, dst_y, alpha, mode, dst_width, dst_height, src_x, src_y,
                                 src_width, src_height, -1);
}

void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *surface, int new_colour)
{
    // ScriptDrawingSurface::SetDrawingColor converts the colour for the surface's bitmap, so it needs one
    if (!AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.DrawingColor"))
    {
        return;
    }
    surface->SetDrawingColor(new_colour);
}

int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *surface)
{
    return surface->GetScriptDrawingColor();
}

void DrawingSurface_SetBlendMode(ScriptDrawingSurface *surface, int blend_mode)
{
    surface->SetBlendMode(static_cast<BlendMode>(blend_mode));
}

int DrawingSurface_GetBlendMode(ScriptDrawingSurface *surface)
{
    return surface->GetBlendMode();
}

int DrawingSurface_GetHeight(ScriptDrawingSurface *surface)
{
    Bitmap *ds = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.Height");
    return ds ? ds->GetHeight() : 0;
}

bool DrawingSurface_GetValid(ScriptDrawingSurface *surface)
{
    return surface->IsValid();
}

int DrawingSurface_GetWidth(ScriptDrawingSurface *surface)
{
    Bitmap *ds = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.Width");
    return ds ? ds->GetWidth() : 0;
}

int DrawingSurface_GetColorDepth(ScriptDrawingSurface *surface)
{
    Bitmap *ds = surface->GetBitmapSurface();
    return ds ? ds->GetColorDepth() : 0;
}

void DrawingSurface_Clear(ScriptDrawingSurface *surface, int colour)
{
    // clear is always done in solid mode
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawing(), "DrawingSurface.Clear");
    if (!ds)
        return;
    int allegro_color;
    if (colour == SCR_COLOR_TRANSPARENT)
    {
        allegro_color = ds->GetMaskColor();
    }
    else
    {
        allegro_color = ds->GetCompatibleColor(colour);
    }
    ds->Fill(allegro_color);
    surface->FinishedDrawing();
}

void DrawingSurface_DrawArc(ScriptDrawingSurface *surface, int x, int y, int radius, int angle1, int angle2)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawArc");
    if (ds)
    {
        ds->DrawArc(x, y, radius, angle1, angle2, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawCircle(ScriptDrawingSurface *surface, int x, int y, int radius)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawCircle");
    if (ds)
    {
        ds->FillCircle(Circle(x, y, radius), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawCircleOutline(ScriptDrawingSurface *surface, int x, int y, int radius)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawCircleOutline");
    if (ds)
    {
        ds->DrawCircle(Circle(x, y, radius), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawEllipse(ScriptDrawingSurface *surface, int x, int y, int radiusx, int radiusy)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawEllipse");
    if (ds)
    {
        ds->FillEllipse(x, y, radiusx, radiusy, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawEllipseOutline(ScriptDrawingSurface *surface, int x, int y, int radiusx, int radiusy)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawEllipseOutline");
    if (ds)
    {
        ds->DrawEllipse(x, y, radiusx, radiusy, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawPie(ScriptDrawingSurface *surface, int x, int y, int radius, int angle1, int angle2)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawPie");
    if (ds)
    {
        ds->FillPie(x, y, radius, angle1, angle2, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawPieOutline(ScriptDrawingSurface *surface, int x, int y, int radius, int angle1, int angle2)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawPieOutline");
    if (ds)
    {
        ds->DrawPie(x, y, radius, angle1, angle2, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawRectangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawRectangle");
    if (ds)
    {
        ds->FillRect(Rect(x1, y1, x2, y2), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawRectangleOutline(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawRectangleOutline");
    if (ds)
    {
        ds->DrawRect(Rect(x1, y1, x2, y2), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawTriangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2, int x3, int y3)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawTriangle");
    if (ds)
    {
        ds->FillTriangle(Triangle(x1, y1, x2, y2, x3, y3), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawTriangleOutline(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2, int x3, int y3)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawTriangleOutline");
    if (ds)
    {
        ds->DrawTriangle(Triangle(x1, y1, x2, y2, x3, y3), surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

void DrawingSurface_DrawString(ScriptDrawingSurface *surface, int x, int y, int font, const char *text)
{
    // no need to use "brush", blending is done by font renderer
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawing(), "DrawingSurface.DrawString");
    if (!ds)
        return;
    color_t text_color = surface->GetRealDrawingColor();
    String res_str = GUI::ApplyTextDirection(text);
    wouttext_outline(ds, x, y, font, text_color, surface->GetBlendMode(), res_str.GetCStr());
    surface->FinishedDrawing();
}

void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *surface, int x, int y, int width, int font, int alignment,
                                      const char *msg)
{
    // no need to use "brush", blending is done by font renderer
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawing(), "DrawingSurface.DrawStringWrapped");
    if (!ds)
        return;

    int linespacing = get_font_linespacing(font);
    const char *draw_text = skip_voiceover_token(msg);
    if (break_up_text_into_lines(draw_text, Lines, width, font) == 0)
    {
        return;
    }

    color_t text_color = surface->GetRealDrawingColor();
    color_t outline_color = ds->GetCompatibleColor(play.speech_text_shadow);
    for (size_t i = 0; i < Lines.Count(); i++)
    {
        GUI::DrawTextAlignedHor(ds, Lines[i], font, text_color, outline_color, surface->GetBlendMode(), x,
                                x + width - 1, y + linespacing * static_cast<int>(i), (FrameAlignment)alignment);
    }

    surface->FinishedDrawing();
}

void DrawingSurface_DrawLine(ScriptDrawingSurface *surface, int fromx, int fromy, int tox, int toy, int thickness)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawLine");
    if (!ds)
        return;
    // draw several lines to simulate the thickness
    int ii, jj, x, y;
    color_t draw_color = surface->GetRealDrawingColor();
    for (ii = 0; ii < thickness; ii++)
    {
        x = (ii - (thickness / 2));
        for (jj = 0; jj < thickness; jj++)
        {
            y = (jj - (thickness / 2));
            ds->DrawLine(Line(fromx + x, fromy + y, tox + x, toy + y), draw_color);
        }
    }
    surface->FinishedDrawingWithBrush();
}

void DrawingSurface_DrawPixel(ScriptDrawingSurface *surface, int x, int y)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.DrawPixel");
    if (!ds)
        return;
    if (surface->IsAlphaBlending())
    {
        ds->BlendPixel(x, y, surface->GetRealDrawingColor());
    }
    else
    {
        ds->PutPixel(x, y, surface->GetRealDrawingColor());
    }
    surface->FinishedDrawingWithBrush();
}

void DrawingSurface_FloodFill(ScriptDrawingSurface *surface, int x, int y)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawingWithBrush(), "DrawingSurface.FloodFill");
    if (ds)
    {
        ds->FloodFill(x, y, surface->GetRealDrawingColor());
        surface->FinishedDrawingWithBrush();
    }
}

int DrawingSurface_GetPixel(ScriptDrawingSurface *surface, int x, int y)
{
    Bitmap *ds = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.GetPixel");
    if (ds)
        return BitmapHelper::BitmapColorToAGSColor(ds->GetPixel(x, y), ds->GetColorDepth());
    return 0;
}

void DrawingSurface_SetPixel(ScriptDrawingSurface *surface, int x, int y, int color)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawing(), "DrawingSurface.SetPixel");
    if (ds)
    {
        ds->PutPixel(x, y, ds->GetCompatibleColor(color));
        surface->FinishedDrawing();
    }
}

void *DrawingSurface_GetPixelsCopyImpl(ScriptDrawingSurface *surface, int x, int y, int width, int height,
                                       uint8_t dest_elem_size)
{
    Bitmap *ds = AssertBitmapSurface(surface->GetBitmapSurface(), "DrawingSurface.GetPixelsCopy");
    if (!ds)
        return nullptr;

    if (dest_elem_size != sizeof(uint8_t) && dest_elem_size != ds->GetBPP())
    {
        debug_script_error("DrawingSurface.GetPixelsCopy: invalid destination array type for %d-bit image",
                           ds->GetColorDepth());
        return nullptr;
    }
    if (x < 0 || y < 0 || x >= ds->GetWidth() || y >= ds->GetHeight())
    {
        debug_script_warn("DrawingSurface.GetPixelsCopy: invalid region %d,%d %dx%d", x, y, width, height);
        return nullptr;
    }

    if (width < 0)
        width = ds->GetWidth();
    if (height < 0)
        height = ds->GetHeight();
    width = std::min(width, ds->GetWidth() - x);
    height = std::min(height, ds->GetHeight() - y);
    auto arr = DynamicArrayHelpers::CreateArray(dest_elem_size, (width * height * ds->GetBPP()) / dest_elem_size);
    if (arr)
    {
        const int src_pitch = ds->GetWidth() * ds->GetBPP();
        const int dst_pitch = width * ds->GetBPP();
        PixelOp::CopyPixelsRegion(ds->GetData(), ds->GetBPP(), src_pitch, x, y, width, height,
                                  static_cast<uint8_t *>(arr.Obj()), dst_pitch, 0, 0);
    }
    return arr.Obj();
}

void *DrawingSurface_GetPixelsCopy(ScriptDrawingSurface *surface, int x, int y, int width, int height)
{
    return DrawingSurface_GetPixelsCopyImpl(surface, x, y, width, height, sizeof(uint8_t));
}

void *DrawingSurface_GetPixelsCopy32(ScriptDrawingSurface *surface, int x, int y, int width, int height)
{
    return DrawingSurface_GetPixelsCopyImpl(surface, x, y, width, height, sizeof(uint32_t));
}

void DrawingSurface_SetPixelsImpl(ScriptDrawingSurface *surface, void *arrobj, int x, int y, int width, int height,
                                  uint8_t dest_elem_size)
{
    Bitmap *ds = AssertBitmapSurface(surface->StartDrawing(), "DrawingSurface.SetPixels");
    if (!ds)
        return;
    if (arrobj == nullptr)
        return;

    if (dest_elem_size != sizeof(uint8_t) && dest_elem_size != ds->GetBPP())
    {
        debug_script_error("DrawingSurface.SetPixels: invalid source array type for %d-bit image", ds->GetColorDepth());
        return;
    }
    if (x < 0 || y < 0 || x >= ds->GetWidth() || y >= ds->GetHeight())
    {
        debug_script_warn("DrawingSurface.SetPixels: invalid region %d,%d", x, y);
        return;
    }

    if (width < 0)
        width = ds->GetWidth();
    if (height < 0)
        height = ds->GetHeight();
    const auto &arr_header = CCDynamicArray::GetHeader(arrobj);
    const uint8_t *arr_ptr = static_cast<uint8_t *>(arrobj);
    const int src_pitch = width * ds->GetBPP();
    const int dst_pitch = ds->GetWidth() * ds->GetBPP();
    const int copy_width = std::min(width, ds->GetWidth() - x);
    const int copy_height = std::min(height, std::min<int>(arr_header.TotalSize / (src_pitch), ds->GetHeight()));
    PixelOp::CopyPixelsRegion(arr_ptr, ds->GetBPP(), src_pitch, 0, 0, copy_width, copy_height, ds->GetDataForWriting(),
                              dst_pitch, x, y);
    surface->FinishedDrawing();
}

void DrawingSurface_SetPixels(ScriptDrawingSurface *surface, void *arrobj, int x, int y, int width, int height)
{
    DrawingSurface_SetPixelsImpl(surface, arrobj, x, y, width, height, sizeof(uint8_t));
}

void DrawingSurface_SetPixels32(ScriptDrawingSurface *surface, void *arrobj, int x, int y, int width, int height)
{
    DrawingSurface_SetPixelsImpl(surface, arrobj, x, y, width, height, sizeof(uint32_t));
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

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
