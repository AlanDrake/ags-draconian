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

#include "game/audio_clip_def.h"
#include "game/dynobj/script_view_frame.h"
#include "game/runtime_defines.h"
#include "game/view_def.h"
#include "gfx/bitmap.h"
#include "gfx/gfx_def.h"

// The engine's view, loop and frame; aliases of their definitions until they hold more (PLAN.md 5.1a step 3)
using View = ViewDef;
using ViewLoop = ViewLoopDef;
using ViewFrame = ViewFrameDef;
using AudioClip = AudioClipDef;

namespace AGS
{
namespace Common
{
struct InteractionEvents;
class ScriptEventsBase;
} // namespace Common
} // namespace AGS
struct ObjectEvent;

int ViewFrame_GetFlipped(ScriptViewFrame *svf);
int ViewFrame_GetGraphic(ScriptViewFrame *svf);
void ViewFrame_SetGraphic(ScriptViewFrame *svf, int newPic);
AudioClip *ViewFrame_GetLinkedAudio(ScriptViewFrame *svf);
void ViewFrame_SetLinkedAudio(ScriptViewFrame *svf, AudioClip *clip);
int ViewFrame_GetSpeed(ScriptViewFrame *svf);
int ViewFrame_GetView(ScriptViewFrame *svf);
int ViewFrame_GetLoop(ScriptViewFrame *svf);
int ViewFrame_GetFrame(ScriptViewFrame *svf);

// Calculate the frame sound volume from different factors;
// pass scale as 100 if volume scaling is disabled
// NOTE: historically scales only in 0-100 range :/
int CalcFrameSoundVolume(int obj_vol, int anim_vol, int scale = 100);
// Handle the new animation frame (play linked sounds, etc);
// sound_volume is an optional *relative* factor, 100 is default (unchanged)
void CheckViewFrame(int view, int loop, int frame, int sound_volume = 100, int sound_pan = 0, int sound_speed = 1000);
void CheckViewFrame(int view, int loop, int frame, int sound_volume, int sound_pan, int sound_speed,
                    const ObjectEvent &obj_evt, AGS::Common::ScriptEventsBase *handlers, int evnt);
// Plays a linked sound of the given frame; returns if the sound is linked and played successfully
bool PlayViewFrameSound(int view, int loop, int frame, int sound_volume, int sound_pan, int sound_speed);
// Triggers a object's frame event handler for the given frame;
// returns if the frame has associated custom event and object has a handler
bool RunViewFrameEvent(int view, int loop, int frame, const ObjectEvent &obj_evt,
                       AGS::Common::ScriptEventsBase *handlers, int evnt);
// draws a view frame, flipped if appropriate
void DrawViewFrame(AGS::Common::Bitmap *ds, const ViewFrame *vframe, int x, int y);
// Gets the flip a view frame is drawn with: none if the view, loop or frame does not exist, or if
// the sprite drawn is not the frame's (pic, as in draw.cpp; pass -1 to skip this check)
AGS::Common::GraphicFlip GetViewFrameFlip(int view, int loop, int frame, int pic = -1);

// Script API: ViewFrame::get_EventName
const char *ViewFrame_GetEventName(ScriptViewFrame *svf);

// Script API: ViewFrame::set_EventName
void ViewFrame_SetEventName(ScriptViewFrame *svf, const char *name);

// Script API: ViewFrame::set_Flipped
void ViewFrame_SetFlipped(ScriptViewFrame *svf, int flip);

// Script API: ViewFrame::set_Speed
void ViewFrame_SetSpeed(ScriptViewFrame *svf, int speed);

// Script API: ViewFrame::get_XOffset
int ViewFrame_GetXOffset(ScriptViewFrame *svf);

// Script API: ViewFrame::set_XOffset
void ViewFrame_SetXOffset(ScriptViewFrame *svf, int xoff);

// Script API: ViewFrame::get_YOffset
int ViewFrame_GetYOffset(ScriptViewFrame *svf);

// Script API: ViewFrame::set_YOffset
void ViewFrame_SetYOffset(ScriptViewFrame *svf, int yoff);
