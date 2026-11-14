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
// Script bindings registered by RegisterOverlayAPI().
//
//=============================================================================
#include "ac/translation.h"
#include "game/dynobj/script_shader.h"
#include "game/render/overlay.h"
#include "game/world/character_extras.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Overlay_CreateGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptOverlay, Overlay_CreateGraphical);
}

RuntimeScriptValue Sc_Overlay_CreateGraphical5(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateGraphical5);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptOverlay, Overlay_CreateRoomGraphical);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical5(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateRoomGraphical5);
}

RuntimeScriptValue Sc_Overlay_CreateTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateTextual(params[0].IValue, params[1].IValue, params[2].IValue,
                                                   params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(overlay, overlay);
}

RuntimeScriptValue Sc_Overlay_CreateRoomTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateRoomTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateRoomTextual(params[0].IValue, params[1].IValue, params[2].IValue,
                                                       params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(overlay, overlay);
}

// void (ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...)
RuntimeScriptValue Sc_Overlay_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Overlay_SetText, 4);
    Overlay_SetText((ScriptOverlay *)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptOverlay *sco)
RuntimeScriptValue Sc_Overlay_Remove(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptOverlay, Overlay_Remove);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetValid);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetX);
}

// void (ScriptOverlay *scover, int newx)
RuntimeScriptValue Sc_Overlay_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetX);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetY);
}

// void (ScriptOverlay *scover, int newy)
RuntimeScriptValue Sc_Overlay_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetY);
}

RuntimeScriptValue Sc_Overlay_GetAutoSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetAutoSize);
}

RuntimeScriptValue Sc_Overlay_SetAutoSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetAutoSize);
}

RuntimeScriptValue Sc_Overlay_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphic);
}

RuntimeScriptValue Sc_Overlay_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphic);
}

RuntimeScriptValue Sc_Overlay_InRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_InRoom);
}

RuntimeScriptValue Sc_Overlay_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetWidth);
}

RuntimeScriptValue Sc_Overlay_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetWidth);
}

RuntimeScriptValue Sc_Overlay_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetHeight);
}

RuntimeScriptValue Sc_Overlay_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetHeight);
}

RuntimeScriptValue Sc_Overlay_GetGraphicWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicWidth);
}

RuntimeScriptValue Sc_Overlay_GetGraphicHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicHeight);
}

RuntimeScriptValue Sc_Overlay_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptOverlay, const char, myScriptStringImpl, Overlay_GetTextProperty);
}

RuntimeScriptValue Sc_Overlay_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptOverlay, Overlay_SetTextProperty, const char);
}

RuntimeScriptValue Sc_Overlay_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetBlendMode);
}

RuntimeScriptValue Sc_Overlay_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetBlendMode);
}

RuntimeScriptValue Sc_Overlay_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptOverlay, ScriptShaderInstance, Overlay_GetShader);
}

RuntimeScriptValue Sc_Overlay_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptOverlay, Overlay_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_Overlay_GetFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetFlip);
}

RuntimeScriptValue Sc_Overlay_SetFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetFlip);
}

RuntimeScriptValue Sc_Overlay_GetGraphicPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptOverlay, void, globalDynamicArray, Overlay_GetGraphicPosition);
}

RuntimeScriptValue Sc_Overlay_GetGraphicBoundBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptOverlay, void, globalDynamicArray, Overlay_GetGraphicBoundBox);
}

RuntimeScriptValue Sc_Overlay_GetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetGraphicAnchorX);
}

RuntimeScriptValue Sc_Overlay_SetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetGraphicAnchorX);
}

RuntimeScriptValue Sc_Overlay_GetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetGraphicAnchorY);
}

RuntimeScriptValue Sc_Overlay_SetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetGraphicAnchorY);
}

RuntimeScriptValue Sc_Overlay_GetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicOffsetX);
}

RuntimeScriptValue Sc_Overlay_SetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphicOffsetX);
}

RuntimeScriptValue Sc_Overlay_GetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicOffsetY);
}

RuntimeScriptValue Sc_Overlay_SetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphicOffsetY);
}

RuntimeScriptValue Sc_Overlay_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetRotation);
}

RuntimeScriptValue Sc_Overlay_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetRotation);
}

RuntimeScriptValue Sc_Overlay_GetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetGraphicPivotX);
}

RuntimeScriptValue Sc_Overlay_SetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetGraphicPivotX);
}

RuntimeScriptValue Sc_Overlay_GetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetGraphicPivotY);
}

RuntimeScriptValue Sc_Overlay_SetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetGraphicPivotY);
}

RuntimeScriptValue Sc_Overlay_GetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Overlay_SetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Overlay_GetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Overlay_SetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Overlay_GetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetScaleX);
}

RuntimeScriptValue Sc_Overlay_SetScaleX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetScaleX);
}

RuntimeScriptValue Sc_Overlay_GetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetScaleY);
}

RuntimeScriptValue Sc_Overlay_SetScaleY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetScaleY);
}

RuntimeScriptValue Sc_Overlay_GetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetOpacity);
}

RuntimeScriptValue Sc_Overlay_SetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetOpacity);
}

RuntimeScriptValue Sc_Overlay_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTransparency);
}

RuntimeScriptValue Sc_Overlay_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetTransparency);
}

RuntimeScriptValue Sc_Overlay_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetVisible);
}

RuntimeScriptValue Sc_Overlay_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptOverlay, Overlay_SetVisible);
}

RuntimeScriptValue Sc_Overlay_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetZOrder);
}

RuntimeScriptValue Sc_Overlay_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetZOrder);
}

RuntimeScriptValue Sc_Overlay_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptOverlay, Overlay_SetPosition);
}

RuntimeScriptValue Sc_Overlay_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptOverlay, Overlay_SetSize);
}

RuntimeScriptValue Sc_Overlay_SetScale(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT2(ScriptOverlay, Overlay_SetScale);
}

RuntimeScriptValue Sc_Overlay_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptOverlay, Overlay_Tint);
}

RuntimeScriptValue Sc_Overlay_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetLightLevel);
}

RuntimeScriptValue Sc_Overlay_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptOverlay, Overlay_RemoveTint);
}

RuntimeScriptValue Sc_Overlay_GetHasLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetHasLight);
}

RuntimeScriptValue Sc_Overlay_GetHasTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetHasTint);
}

RuntimeScriptValue Sc_Overlay_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetLightLevel);
}

RuntimeScriptValue Sc_Overlay_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintBlue);
}

RuntimeScriptValue Sc_Overlay_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintGreen);
}

RuntimeScriptValue Sc_Overlay_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintRed);
}

RuntimeScriptValue Sc_Overlay_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintSaturation);
}

RuntimeScriptValue Sc_Overlay_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintLuminance);
}

RuntimeScriptValue Sc_AnimatedOverlay_CreateAnimated(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptAnimatedOverlay, AnimatedOverlay_CreateAnimated);
}

RuntimeScriptValue Sc_AnimatedOverlay_CreateRoomAnimated(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptAnimatedOverlay, AnimatedOverlay_CreateRoomAnimated);
}

RuntimeScriptValue Sc_AnimatedOverlay_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT8(ScriptAnimatedOverlay, AnimatedOverlay_Animate);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAnimatedOverlay, AnimatedOverlay_GetAnimating);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetView);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetLoop);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetFrame);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetPauseWithGame(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetPauseWithGame);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
ScriptOverlay *ScPl_Overlay_CreateTextual(int x, int y, int width, int font, int text_color, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Overlay_CreateTextual(x, y, width, font, text_color, scsf_buffer);
}

ScriptOverlay *ScPl_Overlay_CreateRoomTextual(int x, int y, int width, int font, int text_color, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Overlay_CreateRoomTextual(x, y, width, font, text_color, scsf_buffer);
}

// void (ScriptOverlay *script_over, int width, int font, int text_color, char *text, ...)
void ScPl_Overlay_SetText(ScriptOverlay *script_over, int width, int font, int text_color, char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Overlay_SetText(script_over, width, font, text_color, scsf_buffer);
}


void RegisterOverlayAPI()
{
    // WARNING: Overlay.CreateGraphical and CreateRoomGraphical have 1 param REMOVED since ags4
    ScFnRegister overlay_api[] = {
        {"Overlay::CreateGraphical^4", API_FN_PAIR(Overlay_CreateGraphical)},
        {"Overlay::CreateGraphical^5", API_FN_PAIR(Overlay_CreateGraphical5)},
        {"Overlay::CreateTextual^106", Sc_Overlay_CreateTextual, ScPl_Overlay_CreateTextual},
        {"Overlay::CreateRoomGraphical^4", API_FN_PAIR(Overlay_CreateRoomGraphical)},
        {"Overlay::CreateRoomGraphical^5", API_FN_PAIR(Overlay_CreateRoomGraphical5)},
        {"Overlay::CreateRoomTextual^106", Sc_Overlay_CreateRoomTextual, ScPl_Overlay_CreateRoomTextual},
        {"Overlay::GetGraphicPosition^0", API_FN_PAIR(Overlay_GetGraphicPosition)},
        {"Overlay::GetGraphicBoundBox^0", API_FN_PAIR(Overlay_GetGraphicBoundBox)},
        {"Overlay::SetText^104", Sc_Overlay_SetText, ScPl_Overlay_SetText},
        {"Overlay::Remove^0", API_FN_PAIR(Overlay_Remove)},
        {"Overlay::SetPosition^4", API_FN_PAIR(Overlay_SetPosition)},
        {"Overlay::SetSize^2", API_FN_PAIR(Overlay_SetSize)},
        {"Overlay::SetScale^2", API_FN_PAIR(Overlay_SetScale)},
        {"Overlay::Tint^5", API_FN_PAIR(Overlay_Tint)},
        {"Overlay::SetLightLevel^1", API_FN_PAIR(Overlay_SetLightLevel)},
        {"Overlay::RemoveTint^0", API_FN_PAIR(Overlay_RemoveTint)},
        {"Overlay::get_Valid", API_FN_PAIR(Overlay_GetValid)},
        {"Overlay::get_X", API_FN_PAIR(Overlay_GetX)},
        {"Overlay::set_X", API_FN_PAIR(Overlay_SetX)},
        {"Overlay::get_Y", API_FN_PAIR(Overlay_GetY)},
        {"Overlay::set_Y", API_FN_PAIR(Overlay_SetY)},
        {"Overlay::get_AutoSize", API_FN_PAIR(Overlay_GetAutoSize)},
        {"Overlay::set_AutoSize", API_FN_PAIR(Overlay_SetAutoSize)},
        {"Overlay::get_Graphic", API_FN_PAIR(Overlay_GetGraphic)},
        {"Overlay::set_Graphic", API_FN_PAIR(Overlay_SetGraphic)},
        {"Overlay::get_InRoom", API_FN_PAIR(Overlay_InRoom)},
        {"Overlay::get_Width", API_FN_PAIR(Overlay_GetWidth)},
        {"Overlay::set_Width", API_FN_PAIR(Overlay_SetWidth)},
        {"Overlay::get_Height", API_FN_PAIR(Overlay_GetHeight)},
        {"Overlay::set_Height", API_FN_PAIR(Overlay_SetHeight)},
        {"Overlay::get_GraphicWidth", API_FN_PAIR(Overlay_GetGraphicWidth)},
        {"Overlay::get_GraphicHeight", API_FN_PAIR(Overlay_GetGraphicHeight)},
        {"Overlay::get_Text", API_FN_PAIR(Overlay_GetTextProperty)},
        {"Overlay::set_Text", API_FN_PAIR(Overlay_SetTextProperty)},
        {"Overlay::get_Opacity", API_FN_PAIR(Overlay_GetOpacity)},
        {"Overlay::set_Opacity", API_FN_PAIR(Overlay_SetOpacity)},
        // [OBSOLETE] Transparency => Opacity
        {"Overlay::get_Transparency", API_FN_PAIR(Overlay_GetTransparency)},
        {"Overlay::set_Transparency", API_FN_PAIR(Overlay_SetTransparency)},
        {"Overlay::get_Visible", API_FN_PAIR(Overlay_GetVisible)},
        {"Overlay::set_Visible", API_FN_PAIR(Overlay_SetVisible)},
        {"Overlay::get_ZOrder", API_FN_PAIR(Overlay_GetZOrder)},
        {"Overlay::set_ZOrder", API_FN_PAIR(Overlay_SetZOrder)},
        {"Overlay::get_HasLightLevel", API_FN_PAIR(Overlay_GetHasLight)},
        {"Overlay::get_HasTint", API_FN_PAIR(Overlay_GetHasTint)},
        {"Overlay::get_LightLevel", API_FN_PAIR(Overlay_GetLightLevel)},
        {"Overlay::get_TintBlue", API_FN_PAIR(Overlay_GetTintBlue)},
        {"Overlay::get_TintGreen", API_FN_PAIR(Overlay_GetTintGreen)},
        {"Overlay::get_TintRed", API_FN_PAIR(Overlay_GetTintRed)},
        {"Overlay::get_TintSaturation", API_FN_PAIR(Overlay_GetTintSaturation)},
        {"Overlay::get_TintLuminance", API_FN_PAIR(Overlay_GetTintLuminance)},

        {"Overlay::get_BlendMode", API_FN_PAIR(Overlay_GetBlendMode)},
        {"Overlay::set_BlendMode", API_FN_PAIR(Overlay_SetBlendMode)},
        {"Overlay::get_Flip", API_FN_PAIR(Overlay_GetFlip)},
        {"Overlay::set_Flip", API_FN_PAIR(Overlay_SetFlip)},
        {"Overlay::get_GraphicAnchorX", API_FN_PAIR(Overlay_GetGraphicAnchorX)},
        {"Overlay::set_GraphicAnchorX", API_FN_PAIR(Overlay_SetGraphicAnchorX)},
        {"Overlay::get_GraphicAnchorY", API_FN_PAIR(Overlay_GetGraphicAnchorY)},
        {"Overlay::set_GraphicAnchorY", API_FN_PAIR(Overlay_SetGraphicAnchorY)},
        {"Overlay::get_GraphicOffsetX", API_FN_PAIR(Overlay_GetGraphicOffsetX)},
        {"Overlay::set_GraphicOffsetX", API_FN_PAIR(Overlay_SetGraphicOffsetX)},
        {"Overlay::get_GraphicOffsetY", API_FN_PAIR(Overlay_GetGraphicOffsetY)},
        {"Overlay::set_GraphicOffsetY", API_FN_PAIR(Overlay_SetGraphicOffsetY)},
        {"Overlay::get_Rotation", API_FN_PAIR(Overlay_GetRotation)},
        {"Overlay::set_Rotation", API_FN_PAIR(Overlay_SetRotation)},
        {"Overlay::get_GraphicPivotX", API_FN_PAIR(Overlay_GetGraphicPivotX)},
        {"Overlay::set_GraphicPivotX", API_FN_PAIR(Overlay_SetGraphicPivotX)},
        {"Overlay::get_GraphicPivotY", API_FN_PAIR(Overlay_GetGraphicPivotY)},
        {"Overlay::set_GraphicPivotY", API_FN_PAIR(Overlay_SetGraphicPivotY)},
        {"Overlay::get_GraphicPivotOffsetX", API_FN_PAIR(Overlay_GetGraphicPivotOffsetX)},
        {"Overlay::set_GraphicPivotOffsetX", API_FN_PAIR(Overlay_SetGraphicPivotOffsetX)},
        {"Overlay::get_GraphicPivotOffsetY", API_FN_PAIR(Overlay_GetGraphicPivotOffsetY)},
        {"Overlay::set_GraphicPivotOffsetY", API_FN_PAIR(Overlay_SetGraphicPivotOffsetY)},
        {"Overlay::get_ScaleX", API_FN_PAIR(Overlay_GetScaleX)},
        {"Overlay::set_ScaleX", API_FN_PAIR(Overlay_SetScaleX)},
        {"Overlay::get_ScaleY", API_FN_PAIR(Overlay_GetScaleY)},
        {"Overlay::set_ScaleY", API_FN_PAIR(Overlay_SetScaleY)},

        {"Overlay::get_Shader", API_FN_PAIR(Overlay_GetShader)},
        {"Overlay::set_Shader", API_FN_PAIR(Overlay_SetShader)},
    };

    ccAddExternalFunctions(overlay_api);

    ScFnRegister animoverlay_api[] = {
        {"AnimatedOverlay::CreateAnimated", API_FN_PAIR(AnimatedOverlay_CreateAnimated)},
        {"AnimatedOverlay::CreateRoomAnimated", API_FN_PAIR(AnimatedOverlay_CreateRoomAnimated)},
        {"AnimatedOverlay::Animate", API_FN_PAIR(AnimatedOverlay_Animate)},
        {"AnimatedOverlay::get_Animating", API_FN_PAIR(AnimatedOverlay_GetAnimating)},
        {"AnimatedOverlay::get_Frame", API_FN_PAIR(AnimatedOverlay_GetFrame)},
        {"AnimatedOverlay::get_Loop", API_FN_PAIR(AnimatedOverlay_GetLoop)},
        {"AnimatedOverlay::get_PauseWithGame", API_FN_PAIR(AnimatedOverlay_GetPauseWithGame)},
        {"AnimatedOverlay::get_View", API_FN_PAIR(AnimatedOverlay_GetView)},
    };

    ccAddExternalFunctions(animoverlay_api);
}
