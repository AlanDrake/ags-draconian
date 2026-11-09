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

// **** DRAWINGSURFACE: FUNCTIONS ****
//
// A DrawingSurface_<Name> function implements the script function DrawingSurface.<Name> of the same name, which
// scripts reach through the Sc_DrawingSurface_<Name> wrapper registered in RegisterDrawingSurfaceAPI
// (drawingsurface.cpp); a pair DrawingSurface_Get<Name> and DrawingSurface_Set<Name> implements the script property
// DrawingSurface.<Name> (get_<Name> and set_<Name>). surface is the script surface the function is called on. A
// comment tells where the script name differs. The functions that draw, read pixels or report the size warn in the
// script log and do nothing (a getter returns 0 or nullptr) when the surface is not valid: its source image was
// disposed or Release() was called. Colours are script colour numbers. The outline variants of the Draw functions are
// not declared here.

// Gives the surface back to its source (room background or mask, dynamic sprite, or a copy made by CreateCopy, which
// is freed) and makes the surface invalid. A source that was drawn on is told about the change.
void DrawingSurface_Release(ScriptDrawingSurface *surface);
// Returns a new surface with a copy of the image, or nullptr if the surface is not valid or MAX_DYNAMIC_SURFACES copies
// exist. The copy is a managed object, independent of the source.
ScriptDrawingSurface *DrawingSurface_CreateCopy(ScriptDrawingSurface *surface);
// Draws the source rectangle of source onto target, at the destination rectangle, with normal blending. SCR_NO_VALUE
// for a size means the full size of the source image, for a source position 0; a source rectangle that differs from
// the destination size is scaled. alpha 0-255 is clamped with a warning, 0 draws nothing.
void DrawingSurface_DrawSurface(ScriptDrawingSurface *target, ScriptDrawingSurface *source, int alpha, int dst_x,
                                int dst_y, int dst_width, int dst_height, int src_x, int src_y, int src_width,
                                int src_height);
// DrawingSurface.DrawingColor: the colour number the script set, which the drawing functions use.
void DrawingSurface_SetDrawingColor(ScriptDrawingSurface *surface, int new_colour);
int DrawingSurface_GetDrawingColor(ScriptDrawingSurface *surface);
// DrawingSurface.Height and DrawingSurface.Width: the size of the image, 0 if the surface is not valid.
int DrawingSurface_GetHeight(ScriptDrawingSurface *surface);
int DrawingSurface_GetWidth(ScriptDrawingSurface *surface);
// Fills the whole surface with the colour, replacing the pixels whatever the blend mode; SCR_COLOR_TRANSPARENT fills
// with the transparent mask colour.
void DrawingSurface_Clear(ScriptDrawingSurface *surface, int colour);
// Filled shapes in the drawing colour and blend mode, in surface coordinates: a circle, a rectangle with the given
// corners, a triangle with the given corners.
void DrawingSurface_DrawCircle(ScriptDrawingSurface *surface, int x, int y, int radius);
void DrawingSurface_DrawRectangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2);
void DrawingSurface_DrawTriangle(ScriptDrawingSurface *surface, int x1, int y1, int x2, int y2, int x3, int y3);
// Draws the text at x, y in the drawing colour and blend mode, applying the text direction.
void DrawingSurface_DrawString(ScriptDrawingSurface *surface, int x, int y, int font, const char *text);
// Breaks msg into lines no wider than wid and draws them one under another from y, aligned inside the span from x to
// x + wid - 1; a voice-over token at the start of msg is skipped. The outline colour is the speech text shadow colour
// (play.speech_text_shadow).
void DrawingSurface_DrawStringWrapped(ScriptDrawingSurface *surface, int x, int y, int wid, int font, int alignment,
                                      const char *msg);
// Draws a line; a thickness above 1 is made of thickness * thickness one-pixel lines, each offset by whole pixels
// around the given line.
void DrawingSurface_DrawLine(ScriptDrawingSurface *surface, int fromx, int fromy, int tox, int toy, int thickness);
// Sets one pixel to the drawing colour, blended with the old one if the blend mode and colour call for alpha blending.
void DrawingSurface_DrawPixel(ScriptDrawingSurface *surface, int x, int y);
// Returns the pixel's colour as a script colour number, 0 if the surface is not valid.
int DrawingSurface_GetPixel(ScriptDrawingSurface *surface, int x, int y);
