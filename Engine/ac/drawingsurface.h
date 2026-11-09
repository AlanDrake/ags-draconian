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
//
//
//=============================================================================
#pragma once

#include "ac/dynobj/scriptdrawingsurface.h"

void DrawingSurface_Release(ScriptDrawingSurface *surface);
// convert actual co-ordinate back to what the script is expecting
ScriptDrawingSurface *DrawingSurface_CreateCopy(ScriptDrawingSurface *surface);
void DrawingSurface_DrawSurface(ScriptDrawingSurface *target, ScriptDrawingSurface *source, int alpha, int dst_x,
                                int dst_y, int dst_width, int dst_height, int src_x, int src_y, int src_width,
                                int src_height);
void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *surface, int new_colour);
int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *surface);
int DrawingSurface_GetHeight(ScriptDrawingSurface *surface);
int DrawingSurface_GetWidth(ScriptDrawingSurface *surface);
void DrawingSurface_Clear(ScriptDrawingSurface *surface, int colour);
void DrawingSurface_DrawCircle(ScriptDrawingSurface *surface, int x, int y, int radius);
void DrawingSurface_DrawRectangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2);
void DrawingSurface_DrawTriangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2, int x3, int y3);
void DrawingSurface_DrawString(ScriptDrawingSurface *surface, int x, int y, int font, const char *text);
void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *surface, int x, int y, int wid, int font, int alignment,
                                      const char *msg);
void DrawingSurface_DrawLine(ScriptDrawingSurface *surface, int fromx, int fromy, int tox, int toy, int thickness);
void DrawingSurface_DrawPixel(ScriptDrawingSurface *surface, int x, int y);
int DrawingSurface_GetPixel(ScriptDrawingSurface *surface, int x, int y);
