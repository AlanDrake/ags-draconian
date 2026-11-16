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
#include "game/dynobj/script_shader.h"
#include "game/render/overlay.h"
#include "game/script_api/bindings.h"
#include "game/translation.h"
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
        ScStatic<&Overlay_CreateGraphical>("Overlay::CreateGraphical^4"),
        ScStatic<&Overlay_CreateGraphical5>("Overlay::CreateGraphical^5"),
        ScStaticFormat<&Overlay_CreateTextual>("Overlay::CreateTextual^106", ScPl_Overlay_CreateTextual),
        ScStatic<&Overlay_CreateRoomGraphical>("Overlay::CreateRoomGraphical^4"),
        ScStatic<&Overlay_CreateRoomGraphical5>("Overlay::CreateRoomGraphical^5"),
        ScStaticFormat<&Overlay_CreateRoomTextual>("Overlay::CreateRoomTextual^106", ScPl_Overlay_CreateRoomTextual),
        ScMethod<&Overlay_GetGraphicPosition, ScManagedBy<globalDynamicArray>>("Overlay::GetGraphicPosition^0"),
        ScMethod<&Overlay_GetGraphicBoundBox, ScManagedBy<globalDynamicArray>>("Overlay::GetGraphicBoundBox^0"),
        ScMethodFormat<static_cast<void (*)(ScriptOverlay *, int, int, int, const char *)>(&Overlay_SetText)>(
            "Overlay::SetText^104", ScPl_Overlay_SetText),
        ScMethod<&Overlay_Remove>("Overlay::Remove^0"),
        ScMethod<&Overlay_SetPosition>("Overlay::SetPosition^4"),
        ScMethod<&Overlay_SetSize>("Overlay::SetSize^2"),
        ScMethod<&Overlay_SetScale, ScUndefinedResult>("Overlay::SetScale^2"),
        ScMethod<&Overlay_Tint>("Overlay::Tint^5"),
        ScMethod<&Overlay_SetLightLevel>("Overlay::SetLightLevel^1"),
        ScMethod<&Overlay_RemoveTint>("Overlay::RemoveTint^0"),
        ScMethod<&Overlay_GetValid>("Overlay::get_Valid"),
        ScMethod<&Overlay_GetX>("Overlay::get_X"),
        ScMethod<&Overlay_SetX>("Overlay::set_X"),
        ScMethod<&Overlay_GetY>("Overlay::get_Y"),
        ScMethod<&Overlay_SetY>("Overlay::set_Y"),
        ScMethod<&Overlay_GetAutoSize>("Overlay::get_AutoSize"),
        ScMethod<&Overlay_SetAutoSize, ScIntArg<0>>("Overlay::set_AutoSize"),
        ScMethod<&Overlay_GetGraphic>("Overlay::get_Graphic"),
        ScMethod<&Overlay_SetGraphic>("Overlay::set_Graphic"),
        ScMethod<&Overlay_InRoom>("Overlay::get_InRoom"),
        ScMethod<&Overlay_GetWidth>("Overlay::get_Width"),
        ScMethod<&Overlay_SetWidth>("Overlay::set_Width"),
        ScMethod<&Overlay_GetHeight>("Overlay::get_Height"),
        ScMethod<&Overlay_SetHeight>("Overlay::set_Height"),
        ScMethod<&Overlay_GetGraphicWidth>("Overlay::get_GraphicWidth"),
        ScMethod<&Overlay_GetGraphicHeight>("Overlay::get_GraphicHeight"),
        ScMethod<&Overlay_GetTextProperty>("Overlay::get_Text"),
        ScMethod<&Overlay_SetTextProperty>("Overlay::set_Text"),
        ScMethod<&Overlay_GetOpacity>("Overlay::get_Opacity"),
        ScMethod<&Overlay_SetOpacity>("Overlay::set_Opacity"),
        // [OBSOLETE] Transparency => Opacity
        ScMethod<&Overlay_GetTransparency>("Overlay::get_Transparency"),
        ScMethod<&Overlay_SetTransparency>("Overlay::set_Transparency"),
        ScMethod<&Overlay_GetVisible>("Overlay::get_Visible"),
        ScMethod<&Overlay_SetVisible>("Overlay::set_Visible"),
        ScMethod<&Overlay_GetZOrder>("Overlay::get_ZOrder"),
        ScMethod<&Overlay_SetZOrder>("Overlay::set_ZOrder"),
        ScMethod<&Overlay_GetHasLight>("Overlay::get_HasLightLevel"),
        ScMethod<&Overlay_GetHasTint>("Overlay::get_HasTint"),
        ScMethod<&Overlay_GetLightLevel>("Overlay::get_LightLevel"),
        ScMethod<&Overlay_GetTintBlue>("Overlay::get_TintBlue"),
        ScMethod<&Overlay_GetTintGreen>("Overlay::get_TintGreen"),
        ScMethod<&Overlay_GetTintRed>("Overlay::get_TintRed"),
        ScMethod<&Overlay_GetTintSaturation>("Overlay::get_TintSaturation"),
        ScMethod<&Overlay_GetTintLuminance>("Overlay::get_TintLuminance"),

        ScMethod<&Overlay_GetBlendMode>("Overlay::get_BlendMode"),
        ScMethod<&Overlay_SetBlendMode>("Overlay::set_BlendMode"),
        ScMethod<&Overlay_GetFlip>("Overlay::get_Flip"),
        ScMethod<&Overlay_SetFlip>("Overlay::set_Flip"),
        ScMethod<&Overlay_GetGraphicAnchorX>("Overlay::get_GraphicAnchorX"),
        ScMethod<&Overlay_SetGraphicAnchorX, ScUndefinedResult>("Overlay::set_GraphicAnchorX"),
        ScMethod<&Overlay_GetGraphicAnchorY>("Overlay::get_GraphicAnchorY"),
        ScMethod<&Overlay_SetGraphicAnchorY, ScUndefinedResult>("Overlay::set_GraphicAnchorY"),
        ScMethod<&Overlay_GetGraphicOffsetX>("Overlay::get_GraphicOffsetX"),
        ScMethod<&Overlay_SetGraphicOffsetX>("Overlay::set_GraphicOffsetX"),
        ScMethod<&Overlay_GetGraphicOffsetY>("Overlay::get_GraphicOffsetY"),
        ScMethod<&Overlay_SetGraphicOffsetY>("Overlay::set_GraphicOffsetY"),
        ScMethod<&Overlay_GetRotation>("Overlay::get_Rotation"),
        ScMethod<&Overlay_SetRotation, ScUndefinedResult>("Overlay::set_Rotation"),
        ScMethod<&Overlay_GetGraphicPivotX>("Overlay::get_GraphicPivotX"),
        ScMethod<&Overlay_SetGraphicPivotX, ScUndefinedResult>("Overlay::set_GraphicPivotX"),
        ScMethod<&Overlay_GetGraphicPivotY>("Overlay::get_GraphicPivotY"),
        ScMethod<&Overlay_SetGraphicPivotY, ScUndefinedResult>("Overlay::set_GraphicPivotY"),
        ScMethod<&Overlay_GetGraphicPivotOffsetX>("Overlay::get_GraphicPivotOffsetX"),
        ScMethod<&Overlay_SetGraphicPivotOffsetX>("Overlay::set_GraphicPivotOffsetX"),
        ScMethod<&Overlay_GetGraphicPivotOffsetY>("Overlay::get_GraphicPivotOffsetY"),
        ScMethod<&Overlay_SetGraphicPivotOffsetY>("Overlay::set_GraphicPivotOffsetY"),
        ScMethod<&Overlay_GetScaleX>("Overlay::get_ScaleX"),
        ScMethod<&Overlay_SetScaleX, ScUndefinedResult>("Overlay::set_ScaleX"),
        ScMethod<&Overlay_GetScaleY>("Overlay::get_ScaleY"),
        ScMethod<&Overlay_SetScaleY, ScUndefinedResult>("Overlay::set_ScaleY"),

        ScMethod<&Overlay_GetShader>("Overlay::get_Shader"),
        ScMethod<&Overlay_SetShader>("Overlay::set_Shader"),
    };

    ccAddExternalFunctions(overlay_api);

    ScFnRegister animoverlay_api[] = {
        ScStatic<&AnimatedOverlay_CreateAnimated>("AnimatedOverlay::CreateAnimated"),
        ScStatic<&AnimatedOverlay_CreateRoomAnimated>("AnimatedOverlay::CreateRoomAnimated"),
        ScMethod<&AnimatedOverlay_Animate>("AnimatedOverlay::Animate"),
        ScMethod<&AnimatedOverlay_GetAnimating>("AnimatedOverlay::get_Animating"),
        ScMethod<&AnimatedOverlay_GetFrame>("AnimatedOverlay::get_Frame"),
        ScMethod<&AnimatedOverlay_GetLoop>("AnimatedOverlay::get_Loop"),
        ScMethod<&AnimatedOverlay_GetPauseWithGame>("AnimatedOverlay::get_PauseWithGame"),
        ScMethod<&AnimatedOverlay_GetView>("AnimatedOverlay::get_View"),
    };

    ccAddExternalFunctions(animoverlay_api);
}
