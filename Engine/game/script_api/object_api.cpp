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
// Script bindings registered by RegisterObjectAPI().
//
//=============================================================================
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/world/object.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterObjectAPI()
{
    ScFnRegister object_api[] = {
        ScStatic<&Object_GetAtRoomXY2>("Object::GetAtRoomXY^2"),
        ScStatic<&Object_GetAtScreenXY2>("Object::GetAtScreenXY^2"),
        ScStatic<&Object_GetAtRoomXY>("Object::GetAtRoomXY^3"),
        ScStatic<&Object_GetAtScreenXY>("Object::GetAtScreenXY^3"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&Object_GetByName>("Object::GetByName"),
        ScStatic<&Object_GetByName>("Object::GetByScriptName"),

        ScMethod<&Object_Animate5>("Object::Animate^5"),
        ScMethod<&Object_Animate6>("Object::Animate^6"),
        ScMethod<&Object_Animate>("Object::Animate^7"),
        ScMethod<&Object_IsCollidingWithObject>("Object::IsCollidingWithObject^1"),
        ScMethod<&Object_GetGraphicPosition, ScManagedBy<globalDynamicArray>>("Object::GetGraphicPosition^0"),
        ScMethod<&Object_GetGraphicBoundBox, ScManagedBy<globalDynamicArray>>("Object::GetGraphicBoundBox^0"),
        ScMethod<&Object_GetProperty>("Object::GetProperty^1"),
        ScMethod<&Object_GetPropertyText>("Object::GetPropertyText^2"),
        ScMethod<&Object_GetTextProperty>("Object::GetTextProperty^1"),
        ScMethod<&Object_SetProperty>("Object::SetProperty^2"),
        ScMethod<&Object_SetTextProperty>("Object::SetTextProperty^2"),
        ScMethod<&Object_IsInteractionAvailable>("Object::IsInteractionAvailable^1"),
        ScMethod<&Object_Move>("Object::Move^5"),
        ScMethod<&Object_MovePath>("Object::MovePath^5"),
        ScMethod<&Object_RemoveTint>("Object::RemoveTint^0"),
        ScMethod<&Object_RunFrameEvent, ScVoidResult>("Object::RunFrameEvent^3"),
        ScMethod<&Object_RunInteraction>("Object::RunInteraction^1"),
        ScMethod<&Object_SetLightLevel>("Object::SetLightLevel^1"),
        ScMethod<&Object_SetPosition>("Object::SetPosition^2"),
        ScMethod<&Object_SetView>("Object::SetView^3"),
        ScMethod<&Object_StopAnimating>("Object::StopAnimating^0"),
        ScMethod<&Object_StopMoving>("Object::StopMoving^0"),
        ScMethod<&Object_Tint>("Object::Tint^5"),
        ScMethod<&Object_GetAnimating>("Object::get_Animating"),
        ScMethod<&Object_GetAudioVolume>("Object::get_AnimationVolume"),
        ScMethod<&Object_SetAudioVolume>("Object::set_AnimationVolume"),
        ScMethod<&Object_GetAudioPanning>("Object::get_AudioPanning"),
        ScMethod<&Object_SetAudioPanning>("Object::set_AudioPanning"),
        ScMethod<&Object_GetAudioSpeed>("Object::get_AudioSpeed"),
        ScMethod<&Object_SetAudioSpeed>("Object::set_AudioSpeed"),
        ScMethod<&Object_GetAudioVolume>("Object::get_AudioVolume"),
        ScMethod<&Object_SetAudioVolume>("Object::set_AudioVolume"),
        ScMethod<&Object_GetBaseline>("Object::get_Baseline"),
        ScMethod<&Object_SetBaseline>("Object::set_Baseline"),
        ScMethod<&Object_GetBlockingHeight>("Object::get_BlockingHeight"),
        ScMethod<&Object_SetBlockingHeight>("Object::set_BlockingHeight"),
        ScMethod<&Object_GetBlockingWidth>("Object::get_BlockingWidth"),
        ScMethod<&Object_SetBlockingWidth>("Object::set_BlockingWidth"),
        ScMethod<&Object_GetBlockingRectX>("Object::get_BlockingRectX"),
        ScMethod<&Object_SetBlockingRectX>("Object::set_BlockingRectX"),
        ScMethod<&Object_GetBlockingRectY>("Object::get_BlockingRectY"),
        ScMethod<&Object_SetBlockingRectY>("Object::set_BlockingRectY"),
        ScMethod<&Object_GetClickable>("Object::get_Clickable"),
        ScMethod<&Object_SetClickable>("Object::set_Clickable"),
        ScMethod<&Object_GetDestinationX>("Object::get_DestinationX"),
        ScMethod<&Object_GetDestinationY>("Object::get_DestinationY"),
        ScMethod<&Object_GetName_New>("Object::get_DisplayName"),
        ScMethod<&Object_SetName>("Object::set_DisplayName"),
        ScMethod<&Object_GetEnabled>("Object::get_Enabled"),
        ScMethod<&Object_SetEnabled>("Object::set_Enabled"),
        ScMethod<&Object_GetFrame>("Object::get_Frame"),
        ScMethod<&Object_GetGraphic>("Object::get_Graphic"),
        ScMethod<&Object_SetGraphic>("Object::set_Graphic"),
        ScMethod<&Object_GetID>("Object::get_ID"),
        ScMethod<&Object_GetLoop>("Object::get_Loop"),
        ScMethod<&Object_GetManualScaling>("Object::get_ManualScaling"),
        ScMethod<&Object_SetManualScaling>("Object::set_ManualScaling"),
        ScMethod<&Object_GetMoving>("Object::get_Moving"),
        // [OBSOLETE] Name => DisplayName
        ScMethod<&Object_GetName_New>("Object::get_Name"),
        ScMethod<&Object_SetName>("Object::set_Name"),
        ScMethod<&Object_GetScaling>("Object::get_Scaling"),
        ScMethod<&Object_SetScaling>("Object::set_Scaling"),
        ScMethod<&Object_GetScriptName>("Object::get_ScriptName"),
        ScMethod<&Object_GetSolid>("Object::get_Solid"),
        ScMethod<&Object_SetSolid>("Object::set_Solid"),
        ScMethod<&Object_GetOpacity>("Object::get_Opacity"),
        ScMethod<&Object_SetOpacity>("Object::set_Opacity"),
        // [OBSOLETE] Transparency => Opacity
        ScMethod<&Object_GetTransparency>("Object::get_Transparency"),
        ScMethod<&Object_SetTransparency>("Object::set_Transparency"),
        ScMethod<&Object_GetView>("Object::get_View"),
        ScMethod<&Object_GetVisible>("Object::get_Visible"),
        ScMethod<&Object_SetVisible>("Object::set_Visible"),
        ScMethod<&Object_GetX>("Object::get_X"),
        ScMethod<&Object_SetX>("Object::set_X"),
        ScMethod<&Object_GetY>("Object::get_Y"),
        ScMethod<&Object_SetY>("Object::set_Y"),
        ScMethod<&Object_HasExplicitLight>("Object::get_HasExplicitLight"),
        ScMethod<&Object_HasExplicitTint>("Object::get_HasExplicitTint"),
        ScMethod<&Object_GetLightLevel>("Object::get_LightLevel"),
        ScMethod<&Object_SetLightLevel>("Object::set_LightLevel"),
        ScMethod<&Object_GetTintBlue>("Object::get_TintBlue"),
        ScMethod<&Object_GetTintGreen>("Object::get_TintGreen"),
        ScMethod<&Object_GetTintRed>("Object::get_TintRed"),
        ScMethod<&Object_GetTintSaturation>("Object::get_TintSaturation"),
        ScMethod<&Object_GetTintLuminance>("Object::get_TintLuminance"),

        ScMethod<&Object_GetBlendMode>("Object::get_BlendMode"),
        ScMethod<&Object_SetBlendMode>("Object::set_BlendMode"),
        ScMethod<&Object_GetGraphicAnchorX>("Object::get_GraphicAnchorX"),
        ScMethod<&Object_SetGraphicAnchorX, ScUndefinedResult>("Object::set_GraphicAnchorX"),
        ScMethod<&Object_GetGraphicAnchorY>("Object::get_GraphicAnchorY"),
        ScMethod<&Object_SetGraphicAnchorY, ScUndefinedResult>("Object::set_GraphicAnchorY"),
        ScMethod<&Object_GetGraphicOffsetX>("Object::get_GraphicOffsetX"),
        ScMethod<&Object_SetGraphicOffsetX>("Object::set_GraphicOffsetX"),
        ScMethod<&Object_GetGraphicOffsetY>("Object::get_GraphicOffsetY"),
        ScMethod<&Object_SetGraphicOffsetY>("Object::set_GraphicOffsetY"),
        ScMethod<&Object_GetRotation>("Object::get_GraphicRotation"),
        ScMethod<&Object_SetRotation, ScUndefinedResult>("Object::set_GraphicRotation"),
        ScMethod<&Object_GetGraphicPivotX>("Object::get_GraphicPivotX"),
        ScMethod<&Object_SetGraphicPivotX, ScUndefinedResult>("Object::set_GraphicPivotX"),
        ScMethod<&Object_GetGraphicPivotY>("Object::get_GraphicPivotY"),
        ScMethod<&Object_SetGraphicPivotY, ScUndefinedResult>("Object::set_GraphicPivotY"),
        ScMethod<&Object_GetGraphicPivotOffsetX>("Object::get_GraphicPivotOffsetX"),
        ScMethod<&Object_SetGraphicPivotOffsetX>("Object::set_GraphicPivotOffsetX"),
        ScMethod<&Object_GetGraphicPivotOffsetY>("Object::get_GraphicPivotOffsetY"),
        ScMethod<&Object_SetGraphicPivotOffsetY>("Object::set_GraphicPivotOffsetY"),
        ScMethod<&Object_GetUseRegionTint>("Object::get_UseRegionTint"),
        ScMethod<&Object_SetUseRegionTint>("Object::set_UseRegionTint"),

        ScMethod<&Object_GetMotionPath>("Object::get_MotionPath"),

        ScMethod<&Object_GetShader>("Object::get_Shader"),
        ScMethod<&Object_SetShader>("Object::set_Shader"),
    };

    ccAddExternalFunctions(object_api);
}
