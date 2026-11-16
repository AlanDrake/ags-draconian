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
// Script bindings registered by RegisterCharacterAPI().
//
//=============================================================================
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_shader.h"
#include "game/game.h"
#include "game/script.h"
#include "game/script_api/bindings.h"
#include "game/translation.h"
#include "game/world/character.h"
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

void ScPl_Character_Say(Character *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_Say(ch, scsf_buffer);
}

void ScPl_Character_SayAt(Character *ch, int x, int y, int width, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_SayAt(ch, x, y, width, scsf_buffer);
}

ScriptOverlay *ScPl_Character_SayBackground(Character *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Character_SayBackground(ch, scsf_buffer);
}

void ScPl_Character_Think(Character *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_Think(ch, scsf_buffer);
}

void RegisterCharacterAPI(ScriptAPIVersion /*base_api*/, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister character_api[] = {
        ScStatic<&Character_GetAtRoomXY2>("Character::GetAtRoomXY^2"),
        ScStatic<&Character_GetAtScreenXY2>("Character::GetAtScreenXY^2"),
        ScStatic<&Character_GetAtRoomXY>("Character::GetAtRoomXY^3"),
        ScStatic<&Character_GetAtScreenXY>("Character::GetAtScreenXY^3"),
        // [OBSOLETE] GetByName => GetByScriptName
        ScStatic<&Character_GetByName>("Character::GetByName"),
        ScStatic<&Character_GetByName>("Character::GetByScriptName"),

        ScMethod<&Character_AddInventory>("Character::AddInventory^2"),
        ScMethod<&Character_AddWaypoint>("Character::AddWaypoint^2"),
        ScMethod<&Character_Animate5>("Character::Animate^5"),
        ScMethod<&Character_Animate6>("Character::Animate^6"),
        ScMethod<&Character_Animate>("Character::Animate^7"),
        ScMethod<&Character_ChangeRoom>("Character::ChangeRoom^3"),
        ScMethod<&Character_ChangeRoomSetLoop>("Character::ChangeRoom^4"),
        ScMethod<&Character_ChangeRoomAutoPosition>("Character::ChangeRoomAutoPosition^2"),
        ScMethod<&Character_ChangeView>("Character::ChangeView^1"),
        ScMethod<&Character_FaceCharacter>("Character::FaceCharacter^2"),
        ScMethod<&Character_FaceDirection>("Character::FaceDirection^2"),
        ScMethod<&Character_FaceLocation>("Character::FaceLocation^3"),
        ScMethod<&Character_FaceObject>("Character::FaceObject^2"),
        ScMethod<&Character_FollowCharacter>("Character::FollowCharacter^3"),
        ScMethod<&Character_GetGraphicPosition, ScManagedBy<globalDynamicArray>>("Character::GetGraphicPosition^0"),
        ScMethod<&Character_GetGraphicBoundBox, ScManagedBy<globalDynamicArray>>("Character::GetGraphicBoundBox^0"),
        ScMethod<&Character_GetProperty>("Character::GetProperty^1"),
        ScMethod<&Character_GetPropertyText>("Character::GetPropertyText^2"),
        ScMethod<&Character_GetTextProperty>("Character::GetTextProperty^1"),
        ScMethod<&Character_SetProperty>("Character::SetProperty^2"),
        ScMethod<&Character_SetTextProperty>("Character::SetTextProperty^2"),
        ScMethod<&Character_HasInventory>("Character::HasInventory^1"),
        ScMethod<&Character_IsCollidingWithChar>("Character::IsCollidingWithChar^1"),
        ScMethod<&Character_IsCollidingWithObject>("Character::IsCollidingWithObject^1"),
        ScMethod<&Character_IsInteractionAvailable>("Character::IsInteractionAvailable^1"),
        ScMethod<&Character_LockView>("Character::LockView^1"),
        ScMethod<&Character_LockViewEx>("Character::LockView^2"),
        ScMethod<&Character_LockViewAlignedEx>("Character::LockViewAligned^4"),
        ScMethod<&Character_LockViewAnchored>("Character::LockViewAnchored^6"),
        ScMethod<&Character_LockViewFrameEx>("Character::LockViewFrame^4"),
        ScMethod<&Character_LockViewOffsetEx>("Character::LockViewOffset^4"),
        ScMethod<&Character_LoseInventory>("Character::LoseInventory^1"),
        ScMethod<&Character_LoseInventoryAt>("Character::LoseInventoryAt^1"),
        ScMethod<&Character_Move>("Character::Move^4"),
        ScMethod<&Character_MovePath>("Character::MovePath^4"),
        ScMethod<&Character_MoveStraight>("Character::MoveStraight^3"),
        ScMethod<&Character_PlaceOnWalkableArea>("Character::PlaceOnWalkableArea^0"),
        ScMethod<&Character_RemoveTint>("Character::RemoveTint^0"),
        ScMethod<&Character_RunFrameEvent, ScVoidResult>("Character::RunFrameEvent^3"),
        ScMethod<&Character_RunInteraction>("Character::RunInteraction^1"),
        ScMethodFormat<&Character_Say>("Character::Say^101", ScPl_Character_Say),
        // old non-variadic variants
        ScMethodFormat<&Character_SayAt>("Character::SayAt^4", Character_SayAt),
        ScMethodFormat<&Character_SayBackground>("Character::SayBackground^1", Character_SayBackground),
        // newer variadic variants
        ScMethodFormat<&Character_SayAt>("Character::SayAt^104", ScPl_Character_SayAt),
        ScMethodFormat<&Character_SayBackground>("Character::SayBackground^101", ScPl_Character_SayBackground),
        ScMethod<&Character_SetAsPlayer>("Character::SetAsPlayer^0"),
        ScMethod<&Character_SetIdleView>("Character::SetIdleView^2"),
        ScMethod<&Character_SetLightLevel>("Character::SetLightLevel^1"),
        ScMethod<&Character_SetSpeed>("Character::SetWalkSpeed^2"),
        ScMethod<&Character_StopMoving>("Character::StopMoving^0"),
        ScMethodFormat<&Character_Think>("Character::Think^101", ScPl_Character_Think),
        ScMethod<&Character_Tint>("Character::Tint^5"),
        ScMethod<&Character_UnlockView>("Character::UnlockView^0"),
        ScMethod<&Character_UnlockViewEx>("Character::UnlockView^1"),
        ScMethod<&Character_Walk>("Character::Walk^4"),
        ScMethod<&Character_WalkPath>("Character::WalkPath^4"),
        ScMethod<&Character_WalkStraight>("Character::WalkStraight^3"),

        ScMethod<&Character_GetActiveInventory>("Character::get_ActiveInventory"),
        ScMethod<&Character_SetActiveInventory>("Character::set_ActiveInventory"),
        ScMethod<&Character_GetAnimating>("Character::get_Animating"),
        ScMethod<&Character_GetAnimationSpeed>("Character::get_AnimationSpeed"),
        ScMethod<&Character_SetAnimationSpeed>("Character::set_AnimationSpeed"),
        ScMethod<&Character_GetAudioVolume>("Character::get_AnimationVolume"),
        ScMethod<&Character_SetAudioVolume>("Character::set_AnimationVolume"),
        ScMethod<&Character_GetAudioPanning>("Character::get_AudioPanning"),
        ScMethod<&Character_SetAudioPanning>("Character::set_AudioPanning"),
        ScMethod<&Character_GetAudioSpeed>("Character::get_AudioSpeed"),
        ScMethod<&Character_SetAudioSpeed>("Character::set_AudioSpeed"),
        ScMethod<&Character_GetAudioVolume>("Character::get_AudioVolume"),
        ScMethod<&Character_SetAudioVolume>("Character::set_AudioVolume"),
        ScMethod<&Character_GetBaseline>("Character::get_Baseline"),
        ScMethod<&Character_SetBaseline>("Character::set_Baseline"),
        ScMethod<&Character_GetBlinkInterval>("Character::get_BlinkInterval"),
        ScMethod<&Character_SetBlinkInterval>("Character::set_BlinkInterval"),
        ScMethod<&Character_GetBlinkView>("Character::get_BlinkView"),
        ScMethod<&Character_SetBlinkView>("Character::set_BlinkView"),
        ScMethod<&Character_GetBlinkWhileThinking>("Character::get_BlinkWhileThinking"),
        ScMethod<&Character_SetBlinkWhileThinking>("Character::set_BlinkWhileThinking"),
        ScMethod<&Character_GetBlockingHeight>("Character::get_BlockingHeight"),
        ScMethod<&Character_SetBlockingHeight>("Character::set_BlockingHeight"),
        ScMethod<&Character_GetBlockingWidth>("Character::get_BlockingWidth"),
        ScMethod<&Character_SetBlockingWidth>("Character::set_BlockingWidth"),
        ScMethod<&Character_GetBlockingRectX>("Character::get_BlockingRectX"),
        ScMethod<&Character_SetBlockingRectX>("Character::set_BlockingRectX"),
        ScMethod<&Character_GetBlockingRectY>("Character::get_BlockingRectY"),
        ScMethod<&Character_SetBlockingRectY>("Character::set_BlockingRectY"),
        ScMethod<&Character_GetClickable>("Character::get_Clickable"),
        ScMethod<&Character_SetClickable>("Character::set_Clickable"),
        ScMethod<&Character_GetDestinationX>("Character::get_DestinationX"),
        ScMethod<&Character_GetDestinationY>("Character::get_DestinationY"),
        ScMethod<&Character_GetDiagonalWalking>("Character::get_DiagonalLoops"),
        ScMethod<&Character_SetDiagonalWalking>("Character::set_DiagonalLoops"),
        ScMethod<&Character_GetName>("Character::get_DisplayName"),
        ScMethod<&Character_SetName>("Character::set_DisplayName"),
        ScMethod<&Character_GetEnabled>("Character::get_Enabled"),
        ScMethod<&Character_SetEnabled>("Character::set_Enabled"),
        ScMethod<&Character_GetFollowing>("Character::get_Following"),
        ScMethod<&Character_GetFrame>("Character::get_Frame"),
        ScMethod<&Character_SetFrame>("Character::set_Frame"),
        ScMethod<&Character_GetID>("Character::get_ID"),
        ScMethod<&Character_GetIdleDelay>("Character::get_IdleDelay"),
        ScMethod<&Character_GetIdleTime>("Character::get_IdleTime"),
        ScMethod<&Character_GetIdleView>("Character::get_IdleView"),
        ScMethod<&Character_GetIdleAnimationDelay>("Character::get_IdleAnimationDelay"),
        ScMethod<&Character_SetIdleAnimationDelay>("Character::set_IdleAnimationDelay"),
        ScMethod<&Character_GetIInventoryQuantity>("Character::geti_InventoryQuantity"),
        ScMethod<&Character_SetIInventoryQuantity>("Character::seti_InventoryQuantity"),
        ScMethod<&Character_GetIgnoreLighting>("Character::get_IgnoreLighting"),
        ScMethod<&Character_SetIgnoreLighting>("Character::set_IgnoreLighting"),
        ScMethod<&Character_GetInventoryCount>("Character::get_InventoryCount"),
        ScMethod<&Character_GetInventory>("Character::geti_Inventory"),
        ScMethod<&Character_GetLoop>("Character::get_Loop"),
        ScMethod<&Character_SetLoop>("Character::set_Loop"),
        ScMethod<&Character_GetManualScaling>("Character::get_ManualScaling"),
        ScMethod<&Character_SetManualScaling>("Character::set_ManualScaling"),
        ScMethod<&Character_GetMovementLinkedToAnimation>("Character::get_MovementLinkedToAnimation"),
        ScMethod<&Character_SetMovementLinkedToAnimation>("Character::set_MovementLinkedToAnimation"),
        ScMethod<&Character_GetMoving>("Character::get_Moving"),
        // [OBSOLETE] Name => DisplayName
        ScMethod<&Character_GetName>("Character::get_Name"),
        ScMethod<&Character_SetName>("Character::set_Name"),
        ScMethod<&Character_GetNormalView>("Character::get_NormalView"),
        ScMethod<&Character_GetPreviousRoom>("Character::get_PreviousRoom"),
        ScMethod<&Character_GetRoom>("Character::get_Room"),
        ScMethod<&Character_GetScaleMoveSpeed>("Character::get_ScaleMoveSpeed"),
        ScMethod<&Character_SetScaleMoveSpeed>("Character::set_ScaleMoveSpeed"),
        ScMethod<&Character_GetScaleVolume>("Character::get_ScaleVolume"),
        ScMethod<&Character_SetScaleVolume>("Character::set_ScaleVolume"),
        ScMethod<&Character_GetScaling>("Character::get_Scaling"),
        ScMethod<&Character_SetScaling>("Character::set_Scaling"),
        ScMethod<&Character_GetScriptName>("Character::get_ScriptName"),
        ScMethod<&Character_GetSolid>("Character::get_Solid"),
        ScMethod<&Character_SetSolid>("Character::set_Solid"),
        ScMethod<&Character_GetSpeaking>("Character::get_Speaking"),
        ScMethod<&Character_GetSpeakingFrame>("Character::get_SpeakingFrame"),
        ScMethod<&Character_GetSpeechAnimationDelay>("Character::get_SpeechAnimationDelay"),
        ScMethod<&Character_SetSpeechAnimationDelay>("Character::set_SpeechAnimationDelay"),
        ScMethod<&Character_GetSpeechColor>("Character::get_SpeechColor"),
        ScMethod<&Character_SetSpeechColor>("Character::set_SpeechColor"),
        ScMethod<&Character_GetSpeechView>("Character::get_SpeechView"),
        ScMethod<&Character_SetSpeechView>("Character::set_SpeechView"),
        ScMethod<&Character_GetThinking>("Character::get_Thinking"),
        ScMethod<&Character_GetThinkingFrame>("Character::get_ThinkingFrame"),
        ScMethod<&Character_GetThinkView>("Character::get_ThinkView"),
        ScMethod<&Character_SetThinkView>("Character::set_ThinkView"),
        ScMethod<&Character_GetOpacity>("Character::get_Opacity"),
        ScMethod<&Character_SetOpacity>("Character::set_Opacity"),
        // [OBSOLETE] Transparency => Opacity
        ScMethod<&Character_GetTransparency>("Character::get_Transparency"),
        ScMethod<&Character_SetTransparency>("Character::set_Transparency"),
        ScMethod<&Character_GetTurnBeforeWalking>("Character::get_TurnBeforeWalking"),
        ScMethod<&Character_SetTurnBeforeWalking>("Character::set_TurnBeforeWalking"),
        ScMethod<&Character_GetTurning>("Character::get_Turning"),
        ScMethod<&Character_GetTurnWhenFacing>("Character::get_TurnWhenFacing"),
        ScMethod<&Character_SetTurnWhenFacing>("Character::set_TurnWhenFacing"),
        ScMethod<&Character_GetView>("Character::get_View"),
        ScMethod<&Character_GetWalkSpeedX>("Character::get_WalkSpeedX"),
        ScMethod<&Character_GetWalkSpeedY>("Character::get_WalkSpeedY"),
        ScMethod<&Character_GetX>("Character::get_X"),
        ScMethod<&Character_SetX>("Character::set_X"),
        ScMethod<&Character_GetX>("Character::get_x"),
        ScMethod<&Character_SetX>("Character::set_x"),
        ScMethod<&Character_GetY>("Character::get_Y"),
        ScMethod<&Character_SetY>("Character::set_Y"),
        ScMethod<&Character_GetY>("Character::get_y"),
        ScMethod<&Character_SetY>("Character::set_y"),
        ScMethod<&Character_GetZ>("Character::get_Z"),
        ScMethod<&Character_SetZ>("Character::set_Z"),
        ScMethod<&Character_GetZ>("Character::get_z"),
        ScMethod<&Character_SetZ>("Character::set_z"),
        ScMethod<&Character_GetHasExplicitLight>("Character::get_HasExplicitLight"),
        ScMethod<&Character_GetHasExplicitTint>("Character::get_HasExplicitTint"),
        ScMethod<&Character_GetLightLevel>("Character::get_LightLevel"),
        ScMethod<&Character_GetTintBlue>("Character::get_TintBlue"),
        ScMethod<&Character_GetTintGreen>("Character::get_TintGreen"),
        ScMethod<&Character_GetTintRed>("Character::get_TintRed"),
        ScMethod<&Character_GetTintSaturation>("Character::get_TintSaturation"),
        ScMethod<&Character_GetTintLuminance>("Character::get_TintLuminance"),
        ScMethod<&Character_GetVisible>("Character::get_Visible"),
        ScMethod<&Character_SetVisible>("Character::set_Visible"),

        ScMethod<&Character_GetBlendMode>("Character::get_BlendMode"),
        ScMethod<&Character_SetBlendMode>("Character::set_BlendMode"),
        ScMethod<&Character_GetGraphicAnchorX>("Character::get_GraphicAnchorX"),
        ScMethod<&Character_SetGraphicAnchorX, ScUndefinedResult>("Character::set_GraphicAnchorX"),
        ScMethod<&Character_GetGraphicAnchorY>("Character::get_GraphicAnchorY"),
        ScMethod<&Character_SetGraphicAnchorY, ScUndefinedResult>("Character::set_GraphicAnchorY"),
        ScMethod<&Character_GetGraphicOffsetX>("Character::get_GraphicOffsetX"),
        ScMethod<&Character_SetGraphicOffsetX>("Character::set_GraphicOffsetX"),
        ScMethod<&Character_GetGraphicOffsetY>("Character::get_GraphicOffsetY"),
        ScMethod<&Character_SetGraphicOffsetY>("Character::set_GraphicOffsetY"),
        ScMethod<&Character_GetRotation>("Character::get_GraphicRotation"),
        ScMethod<&Character_SetRotation, ScUndefinedResult>("Character::set_GraphicRotation"),
        ScMethod<&Character_GetGraphicPivotX>("Character::get_GraphicPivotX"),
        ScMethod<&Character_SetGraphicPivotX, ScUndefinedResult>("Character::set_GraphicPivotX"),
        ScMethod<&Character_GetGraphicPivotY>("Character::get_GraphicPivotY"),
        ScMethod<&Character_SetGraphicPivotY, ScUndefinedResult>("Character::set_GraphicPivotY"),
        ScMethod<&Character_GetGraphicPivotOffsetX>("Character::get_GraphicPivotOffsetX"),
        ScMethod<&Character_SetGraphicPivotOffsetX>("Character::set_GraphicPivotOffsetX"),
        ScMethod<&Character_GetGraphicPivotOffsetY>("Character::get_GraphicPivotOffsetY"),
        ScMethod<&Character_SetGraphicPivotOffsetY>("Character::set_GraphicPivotOffsetY"),
        ScMethod<&Character_GetUseRegionTint>("Character::get_UseRegionTint"),
        ScMethod<&Character_SetUseRegionTint>("Character::set_UseRegionTint"),
        ScMethod<&Character_GetViewAnchorX>("Character::get_ViewAnchorX"),
        ScMethod<&Character_GetViewAnchorY>("Character::get_ViewAnchorY"),
        ScMethod<&Character_GetViewOffsetX>("Character::get_ViewOffsetX"),
        ScMethod<&Character_GetViewOffsetY>("Character::get_ViewOffsetY"),

        ScMethod<&Character_GetFaceDirectionRatio>("Character::get_FaceDirectionRatio"),
        ScMethod<&Character_SetFaceDirectionRatio, ScUndefinedResult>("Character::set_FaceDirectionRatio"),
        ScMethod<&Character_GetMotionPath>("Character::get_MotionPath"),

        ScMethod<&Character_GetShader>("Character::get_Shader"),
        ScMethod<&Character_SetShader>("Character::set_Shader"),
    };

    ccAddExternalFunctions(character_api);
}
