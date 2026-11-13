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
#include "ac/character.h"
#include "ac/dynobj/cc_character.h"
#include "ac/dynobj/cc_dynamicarray.h"
#include "ac/dynobj/cc_inventory.h"
#include "ac/dynobj/scriptmotionpath.h"
#include "ac/dynobj/scriptshader.h"
#include "ac/dynobj/scriptstring.h"
#include "ac/game.h"
#include "ac/global_translation.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Character_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(CharacterInfo, ccDynamicCharacter, Character_GetByName, const char);
}

// void | CharacterInfo *ch, ScriptInvItem *invi, int addIndex
RuntimeScriptValue Sc_Character_AddInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_AddInventory, ScriptInvItem);
}

// void | CharacterInfo *ch, int x, int y
RuntimeScriptValue Sc_Character_AddWaypoint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_AddWaypoint);
}

// void | CharacterInfo *ch, int loop, int delay, int repeat, int blocking, int direction
RuntimeScriptValue Sc_Character_Animate5(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(CharacterInfo, Character_Animate5);
}

RuntimeScriptValue Sc_Character_Animate6(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(CharacterInfo, Character_Animate6);
}

RuntimeScriptValue Sc_Character_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(CharacterInfo, Character_Animate);
}

// void | CharacterInfo *ch, int room, int x, int y
RuntimeScriptValue Sc_Character_ChangeRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_ChangeRoom);
}

RuntimeScriptValue Sc_Character_ChangeRoomSetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_ChangeRoomSetLoop);
}

// void | CharacterInfo *ch, int room, int newPos
RuntimeScriptValue Sc_Character_ChangeRoomAutoPosition(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_ChangeRoomAutoPosition);
}

// void | CharacterInfo *ch, int view
RuntimeScriptValue Sc_Character_ChangeView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_ChangeView);
}

// void | CharacterInfo *ch, CharacterInfo *other, int blockingStyle
RuntimeScriptValue Sc_Character_FaceCharacter(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_FaceCharacter, CharacterInfo);
}

// void | CharacterInfo *ch, int direction, int blockingStyle
RuntimeScriptValue Sc_Character_FaceDirection(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_FaceDirection);
}

// void | CharacterInfo *ch, int x, int y, int blockingStyle
RuntimeScriptValue Sc_Character_FaceLocation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_FaceLocation);
}

// void | CharacterInfo *ch, ScriptObject *obj, int blockingStyle
RuntimeScriptValue Sc_Character_FaceObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT(CharacterInfo, Character_FaceObject, ScriptObject);
}

// void | CharacterInfo *ch, CharacterInfo *tofollow, int distaway, int eagerness
RuntimeScriptValue Sc_Character_FollowCharacter(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT2(CharacterInfo, Character_FollowCharacter, CharacterInfo);
}

// CharacterInfo * | CharacterInfo *ch
RuntimeScriptValue Sc_Character_GetFollowing(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, CharacterInfo, ccDynamicCharacter, Character_GetFollowing);
}

RuntimeScriptValue Sc_Character_GetGraphicPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, void, globalDynamicArray, Character_GetGraphicPosition);
}

RuntimeScriptValue Sc_Character_GetGraphicBoundBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, void, globalDynamicArray, Character_GetGraphicBoundBox);
}

// int (CharacterInfo *ch, const char *property)
RuntimeScriptValue Sc_Character_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_GetProperty, const char);
}

// void (CharacterInfo *ch, const char *property, char *bufer)
RuntimeScriptValue Sc_Character_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(CharacterInfo, Character_GetPropertyText, const char, char);
}

// const char* (CharacterInfo *ch, const char *property)
RuntimeScriptValue Sc_Character_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Character_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(CharacterInfo, Character_SetProperty, const char);
}

RuntimeScriptValue Sc_Character_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(CharacterInfo, Character_SetTextProperty, const char, const char);
}

// int (CharacterInfo *ch, ScriptInvItem *invi)
RuntimeScriptValue Sc_Character_HasInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_HasInventory, ScriptInvItem);
}

// int (CharacterInfo *ch, CharacterInfo *other)
RuntimeScriptValue Sc_Character_IsCollidingWithChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_IsCollidingWithChar, CharacterInfo);
}

// int (CharacterInfo *ch, ScriptObject *objid)
RuntimeScriptValue Sc_Character_IsCollidingWithObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(CharacterInfo, Character_IsCollidingWithObject, ScriptObject);
}

RuntimeScriptValue Sc_Character_IsInteractionAvailable(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_BOOL_PINT(CharacterInfo, Character_IsInteractionAvailable);
}

// void (CharacterInfo *ch, int view)
RuntimeScriptValue Sc_Character_LockView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_LockView);
}

// void (CharacterInfo *ch, int view, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_LockViewEx);
}

RuntimeScriptValue Sc_Character_LockViewAlignedEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewAlignedEx);
}

RuntimeScriptValue Sc_Character_LockViewAnchored(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_OBJ_PARAM_COUNT(Character_LockViewAnchored, 1);
    Character_LockViewAnchored((CharacterInfo *)self, params[0].IValue, params[1].FValue, params[2].FValue,
                               params[3].IValue, params[4].IValue, params[5].IValue);
    return RuntimeScriptValue((int32_t)0);
}

// void (CharacterInfo *ch, int view, int loop, int frame, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewFrameEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewFrameEx);
}

// void (CharacterInfo *ch, int view, int xoffs, int yoffs, int stopMoving)
RuntimeScriptValue Sc_Character_LockViewOffsetEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_LockViewOffsetEx);
}

// void (CharacterInfo *ch, ScriptInvItem *invi)
RuntimeScriptValue Sc_Character_LoseInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_LoseInventory, ScriptInvItem);
}

// void (CharacterInfo *ch, int x, int y, int blocking, int direct)
RuntimeScriptValue Sc_Character_Move(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_Move);
}

RuntimeScriptValue Sc_Character_MoveStraight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_MoveStraight);
}

RuntimeScriptValue Sc_Character_MovePath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT3(CharacterInfo, Character_MovePath, void);
}

// void (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_PlaceOnWalkableArea(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_PlaceOnWalkableArea);
}

RuntimeScriptValue Sc_Character_LoseInventoryAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_LoseInventoryAt);
}

// void (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_RemoveTint);
}

RuntimeScriptValue Sc_Character_RunFrameEvent(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_RunFrameEvent);
}

// void (CharacterInfo *ch, int mood)
RuntimeScriptValue Sc_Character_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_RunInteraction);
}

// void (CharacterInfo *ch, const char *text, ...)
RuntimeScriptValue Sc_Character_Say(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_Say, 1);
    Character_Say((CharacterInfo *)self, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Character_SayAt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_SayAt, 4);
    Character_SayAt((CharacterInfo *)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

RuntimeScriptValue Sc_Character_SayBackground(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_SayBackground, 1);
    auto *ret_obj = Character_SayBackground((CharacterInfo *)self, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(ret_obj, ret_obj);
}

RuntimeScriptValue Sc_Character_SetAsPlayer(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_SetAsPlayer);
}

// void (CharacterInfo *ch, int iview, int itime)
RuntimeScriptValue Sc_Character_SetIdleView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetIdleView);
}

RuntimeScriptValue Sc_Character_GetHasExplicitLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetHasExplicitLight);
}

RuntimeScriptValue Sc_Character_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetLightLevel);
}

RuntimeScriptValue Sc_Character_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetLightLevel);
}

RuntimeScriptValue Sc_Character_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintBlue);
}

RuntimeScriptValue Sc_Character_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintGreen);
}

RuntimeScriptValue Sc_Character_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintRed);
}

RuntimeScriptValue Sc_Character_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintSaturation);
}

RuntimeScriptValue Sc_Character_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTintLuminance);
}

// void (CharacterInfo *ch, int xspeed, int yspeed)
RuntimeScriptValue Sc_Character_SetSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetSpeed);
}

// void (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_StopMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_StopMoving);
}

// void (CharacterInfo *ch, const char *text, ...)
RuntimeScriptValue Sc_Character_Think(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Character_Think, 1);
    Character_Think((CharacterInfo *)self, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

//void (CharacterInfo *ch, int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_Character_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(CharacterInfo, Character_Tint);
}

// void (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_UnlockView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(CharacterInfo, Character_UnlockView);
}

// void (CharacterInfo *ch, int stopMoving)
RuntimeScriptValue Sc_Character_UnlockViewEx(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_UnlockViewEx);
}

// void (CharacterInfo *ch, int x, int y, int blocking, int direct)
RuntimeScriptValue Sc_Character_Walk(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(CharacterInfo, Character_Walk);
}

// void (CharacterInfo *ch, int x, int y, int blocking)
RuntimeScriptValue Sc_Character_WalkStraight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(CharacterInfo, Character_WalkStraight);
}

RuntimeScriptValue Sc_Character_WalkPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT3(CharacterInfo, Character_WalkPath, void);
}

RuntimeScriptValue Sc_Character_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(CharacterInfo, ccDynamicCharacter, Character_GetAtRoomXY);
}

RuntimeScriptValue Sc_Character_GetAtRoomXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(CharacterInfo, ccDynamicCharacter, Character_GetAtRoomXY2);
}

RuntimeScriptValue Sc_Character_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(CharacterInfo, ccDynamicCharacter, Character_GetAtScreenXY);
}

RuntimeScriptValue Sc_Character_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(CharacterInfo, ccDynamicCharacter, Character_GetAtScreenXY2);
}

// ScriptInvItem* (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetActiveInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, ScriptInvItem, ccDynamicInv, Character_GetActiveInventory);
}

// void (CharacterInfo *ch, ScriptInvItem* item)
RuntimeScriptValue Sc_Character_SetActiveInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetActiveInventory, ScriptInvItem);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAnimating);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetAnimationSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAnimationSpeed);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetAnimationSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAnimationSpeed);
}

RuntimeScriptValue Sc_Character_GetAudioPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAudioPanning);
}

RuntimeScriptValue Sc_Character_SetAudioPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAudioPanning);
}

RuntimeScriptValue Sc_Character_GetAudioSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAudioSpeed);
}

RuntimeScriptValue Sc_Character_SetAudioSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAudioSpeed);
}

RuntimeScriptValue Sc_Character_GetAudioVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetAudioVolume);
}

RuntimeScriptValue Sc_Character_SetAudioVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetAudioVolume);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBaseline);
}

// void (CharacterInfo *ch, int basel)
RuntimeScriptValue Sc_Character_SetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBaseline);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBlinkInterval(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkInterval);
}

// void (CharacterInfo *ch, int interval)
RuntimeScriptValue Sc_Character_SetBlinkInterval(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkInterval);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBlinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkView);
}

// void (CharacterInfo *ch, int view)
RuntimeScriptValue Sc_Character_SetBlinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkView);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBlinkWhileThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlinkWhileThinking);
}

// void (CharacterInfo *ch, int yesOrNo)
RuntimeScriptValue Sc_Character_SetBlinkWhileThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlinkWhileThinking);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingHeight);
}

// void (CharacterInfo *ch, int hit)
RuntimeScriptValue Sc_Character_SetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingHeight);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingWidth);
}

// void (CharacterInfo *ch, int wid)
RuntimeScriptValue Sc_Character_SetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingWidth);
}

RuntimeScriptValue Sc_Character_GetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingRectX);
}

RuntimeScriptValue Sc_Character_SetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingRectX);
}

RuntimeScriptValue Sc_Character_GetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlockingRectY);
}

RuntimeScriptValue Sc_Character_SetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlockingRectY);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetClickable);
}

// void (CharacterInfo *ch, int clik)
RuntimeScriptValue Sc_Character_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetClickable);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetDiagonalWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDiagonalWalking);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetDiagonalWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetDiagonalWalking);
}

RuntimeScriptValue Sc_Character_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetEnabled);
}

RuntimeScriptValue Sc_Character_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(CharacterInfo, Character_SetEnabled);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetFrame);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetFrame);
}

RuntimeScriptValue Sc_Character_GetHasExplicitTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetHasExplicitTint);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetID);
}

RuntimeScriptValue Sc_Character_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetScriptName);
}

RuntimeScriptValue Sc_Character_GetIdleDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleDelay);
}

RuntimeScriptValue Sc_Character_GetIdleTime(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleTime);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetIdleView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleView);
}

// int (CharacterInfo *ch, int index)
RuntimeScriptValue Sc_Character_GetIInventoryQuantity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT(CharacterInfo, Character_GetIInventoryQuantity);
}

// void (CharacterInfo *ch, int index, int quant)
RuntimeScriptValue Sc_Character_SetIInventoryQuantity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(CharacterInfo, Character_SetIInventoryQuantity);
}

// [DEPRECATED] int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetIgnoreLighting(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIgnoreLighting);
}

// [DEPRECATED] void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetIgnoreLighting(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetIgnoreLighting);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetInventoryCount(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetInventoryCount);
}

RuntimeScriptValue Sc_Character_GetInventory(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_PINT(CharacterInfo, ScriptInvItem, ccDynamicInv, Character_GetInventory);
}

RuntimeScriptValue Sc_Character_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetLoop);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetLoop);
}

RuntimeScriptValue Sc_Character_GetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetManualScaling);
}

RuntimeScriptValue Sc_Character_SetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetManualScaling);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetMovementLinkedToAnimation(void *self, const RuntimeScriptValue *params,
                                                             int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetMovementLinkedToAnimation);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetMovementLinkedToAnimation(void *self, const RuntimeScriptValue *params,
                                                             int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetMovementLinkedToAnimation);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetMoving);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetDestinationX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDestinationX);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetDestinationY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetDestinationY);
}

// const char* (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(CharacterInfo, const char, myScriptStringImpl, Character_GetName);
}

// void (CharacterInfo *ch, const char *newName)
RuntimeScriptValue Sc_Character_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetName, const char);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetNormalView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetNormalView);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetPreviousRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetPreviousRoom);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetRoom);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetScaleMoveSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaleMoveSpeed);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetScaleMoveSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaleMoveSpeed);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetScaleVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaleVolume);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetScaleVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaleVolume);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetScaling);
}

// void (CharacterInfo *ch, int zoomlevel)
RuntimeScriptValue Sc_Character_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetScaling);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSolid);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSolid);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSpeaking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeaking);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSpeakingFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeakingFrame);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSpeechAnimationDelay(void *self, const RuntimeScriptValue *params,
                                                        int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechAnimationDelay);
}

// void (CharacterInfo *ch, int newDelay)
RuntimeScriptValue Sc_Character_SetSpeechAnimationDelay(void *self, const RuntimeScriptValue *params,
                                                        int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechAnimationDelay);
}

RuntimeScriptValue Sc_Character_GetIdleAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetIdleAnimationDelay);
}

// void (CharacterInfo *ch, int newDelay)
RuntimeScriptValue Sc_Character_SetIdleAnimationDelay(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetIdleAnimationDelay);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSpeechColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechColor);
}

// void (CharacterInfo *ch, int ncol)
RuntimeScriptValue Sc_Character_SetSpeechColor(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechColor);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetSpeechView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetSpeechView);
}

// void (CharacterInfo *ch, int view)
RuntimeScriptValue Sc_Character_SetSpeechView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetSpeechView);
}

RuntimeScriptValue Sc_Character_GetThinking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetThinking);
}

RuntimeScriptValue Sc_Character_GetThinkingFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetThinkingFrame);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetThinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetThinkView);
}

// void (CharacterInfo *ch, int view)
RuntimeScriptValue Sc_Character_SetThinkView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetThinkView);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetOpacity);
}

// void (CharacterInfo *ch, int alpha)
RuntimeScriptValue Sc_Character_SetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetOpacity);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTransparency);
}

// void (CharacterInfo *ch, int trans)
RuntimeScriptValue Sc_Character_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTransparency);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetTurnBeforeWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTurnBeforeWalking);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetTurnBeforeWalking(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTurnBeforeWalking);
}

RuntimeScriptValue Sc_Character_GetTurning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetTurning);
}

RuntimeScriptValue Sc_Character_GetTurnWhenFacing(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetTurnWhenFacing);
}

RuntimeScriptValue Sc_Character_SetTurnWhenFacing(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetTurnWhenFacing);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetView);
}

RuntimeScriptValue Sc_Character_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetVisible);
}

RuntimeScriptValue Sc_Character_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(CharacterInfo, Character_SetVisible);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetWalkSpeedX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetWalkSpeedX);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetWalkSpeedY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetWalkSpeedY);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetX);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetX);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetY);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetY);
}

// int (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetZ(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetZ);
}

// void (CharacterInfo *ch, int newval)
RuntimeScriptValue Sc_Character_SetZ(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetZ);
}

RuntimeScriptValue Sc_Character_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetBlendMode);
}

RuntimeScriptValue Sc_Character_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetBlendMode);
}

RuntimeScriptValue Sc_Character_GetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicAnchorX);
}

RuntimeScriptValue Sc_Character_SetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicAnchorX);
}

RuntimeScriptValue Sc_Character_GetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicAnchorY);
}

RuntimeScriptValue Sc_Character_SetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicAnchorY);
}

RuntimeScriptValue Sc_Character_GetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicOffsetX);
}

RuntimeScriptValue Sc_Character_SetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicOffsetX);
}

RuntimeScriptValue Sc_Character_GetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicOffsetY);
}

RuntimeScriptValue Sc_Character_SetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicOffsetY);
}

RuntimeScriptValue Sc_Character_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(CharacterInfo, ScriptShaderInstance, Character_GetShader);
}

RuntimeScriptValue Sc_Character_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(CharacterInfo, Character_SetShader, ScriptShaderInstance);
}

// bool (CharacterInfo *ch)
RuntimeScriptValue Sc_Character_GetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(CharacterInfo, Character_GetUseRegionTint);
}

// void (CharacterInfo *ch, int yesorno)
RuntimeScriptValue Sc_Character_SetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetUseRegionTint);
}

RuntimeScriptValue Sc_Character_GetViewAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetViewAnchorX);
}

RuntimeScriptValue Sc_Character_GetViewAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetViewAnchorY);
}

RuntimeScriptValue Sc_Character_GetViewOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetViewOffsetX);
}

RuntimeScriptValue Sc_Character_GetViewOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetViewOffsetY);
}

RuntimeScriptValue Sc_Character_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetRotation);
}

RuntimeScriptValue Sc_Character_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetRotation);
}

RuntimeScriptValue Sc_Character_GetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicPivotX);
}

RuntimeScriptValue Sc_Character_SetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicPivotX);
}

RuntimeScriptValue Sc_Character_GetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetGraphicPivotY);
}

RuntimeScriptValue Sc_Character_SetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetGraphicPivotY);
}

RuntimeScriptValue Sc_Character_GetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Character_SetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Character_GetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_INT(CharacterInfo, Character_GetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Character_SetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params,
                                                       int32_t param_count)
{
    API_OBJCALL_VOID_PINT(CharacterInfo, Character_SetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Character_GetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(CharacterInfo, Character_GetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Character_SetFaceDirectionRatio(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(CharacterInfo, Character_SetFaceDirectionRatio);
}

RuntimeScriptValue Sc_Character_GetMotionPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(CharacterInfo, ScriptMotionPath, Character_GetMotionPath);
}

//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

void ScPl_Character_Say(CharacterInfo *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_Say(ch, scsf_buffer);
}

void ScPl_Character_SayAt(CharacterInfo *ch, int x, int y, int width, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_SayAt(ch, x, y, width, scsf_buffer);
}

ScriptOverlay *ScPl_Character_SayBackground(CharacterInfo *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Character_SayBackground(ch, scsf_buffer);
}

void ScPl_Character_Think(CharacterInfo *ch, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    Character_Think(ch, scsf_buffer);
}

void RegisterCharacterAPI(ScriptAPIVersion /*base_api*/, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister character_api[] = {
        {"Character::GetAtRoomXY^2", API_FN_PAIR(Character_GetAtRoomXY2)},
        {"Character::GetAtScreenXY^2", API_FN_PAIR(Character_GetAtScreenXY2)},
        {"Character::GetAtRoomXY^3", API_FN_PAIR(Character_GetAtRoomXY)},
        {"Character::GetAtScreenXY^3", API_FN_PAIR(Character_GetAtScreenXY)},
        // [OBSOLETE] GetByName => GetByScriptName
        {"Character::GetByName", API_FN_PAIR(Character_GetByName)},
        {"Character::GetByScriptName", API_FN_PAIR(Character_GetByName)},

        {"Character::AddInventory^2", API_FN_PAIR(Character_AddInventory)},
        {"Character::AddWaypoint^2", API_FN_PAIR(Character_AddWaypoint)},
        {"Character::Animate^5", API_FN_PAIR(Character_Animate5)},
        {"Character::Animate^6", API_FN_PAIR(Character_Animate6)},
        {"Character::Animate^7", API_FN_PAIR(Character_Animate)},
        {"Character::ChangeRoom^3", API_FN_PAIR(Character_ChangeRoom)},
        {"Character::ChangeRoom^4", API_FN_PAIR(Character_ChangeRoomSetLoop)},
        {"Character::ChangeRoomAutoPosition^2", API_FN_PAIR(Character_ChangeRoomAutoPosition)},
        {"Character::ChangeView^1", API_FN_PAIR(Character_ChangeView)},
        {"Character::FaceCharacter^2", API_FN_PAIR(Character_FaceCharacter)},
        {"Character::FaceDirection^2", API_FN_PAIR(Character_FaceDirection)},
        {"Character::FaceLocation^3", API_FN_PAIR(Character_FaceLocation)},
        {"Character::FaceObject^2", API_FN_PAIR(Character_FaceObject)},
        {"Character::FollowCharacter^3", API_FN_PAIR(Character_FollowCharacter)},
        {"Character::GetGraphicPosition^0", API_FN_PAIR(Character_GetGraphicPosition)},
        {"Character::GetGraphicBoundBox^0", API_FN_PAIR(Character_GetGraphicBoundBox)},
        {"Character::GetProperty^1", API_FN_PAIR(Character_GetProperty)},
        {"Character::GetPropertyText^2", API_FN_PAIR(Character_GetPropertyText)},
        {"Character::GetTextProperty^1", API_FN_PAIR(Character_GetTextProperty)},
        {"Character::SetProperty^2", API_FN_PAIR(Character_SetProperty)},
        {"Character::SetTextProperty^2", API_FN_PAIR(Character_SetTextProperty)},
        {"Character::HasInventory^1", API_FN_PAIR(Character_HasInventory)},
        {"Character::IsCollidingWithChar^1", API_FN_PAIR(Character_IsCollidingWithChar)},
        {"Character::IsCollidingWithObject^1", API_FN_PAIR(Character_IsCollidingWithObject)},
        {"Character::IsInteractionAvailable^1", API_FN_PAIR(Character_IsInteractionAvailable)},
        {"Character::LockView^1", API_FN_PAIR(Character_LockView)},
        {"Character::LockView^2", API_FN_PAIR(Character_LockViewEx)},
        {"Character::LockViewAligned^4", API_FN_PAIR(Character_LockViewAlignedEx)},
        {"Character::LockViewAnchored^6", API_FN_PAIR(Character_LockViewAnchored)},
        {"Character::LockViewFrame^4", API_FN_PAIR(Character_LockViewFrameEx)},
        {"Character::LockViewOffset^4", API_FN_PAIR(Character_LockViewOffsetEx)},
        {"Character::LoseInventory^1", API_FN_PAIR(Character_LoseInventory)},
        {"Character::LoseInventoryAt^1", API_FN_PAIR(Character_LoseInventoryAt)},
        {"Character::Move^4", API_FN_PAIR(Character_Move)},
        {"Character::MovePath^4", API_FN_PAIR(Character_MovePath)},
        {"Character::MoveStraight^3", API_FN_PAIR(Character_MoveStraight)},
        {"Character::PlaceOnWalkableArea^0", API_FN_PAIR(Character_PlaceOnWalkableArea)},
        {"Character::RemoveTint^0", API_FN_PAIR(Character_RemoveTint)},
        {"Character::RunFrameEvent^3", API_FN_PAIR(Character_RunFrameEvent)},
        {"Character::RunInteraction^1", API_FN_PAIR(Character_RunInteraction)},
        {"Character::Say^101", Sc_Character_Say, ScPl_Character_Say},
        // old non-variadic variants
        {"Character::SayAt^4", API_FN_PAIR(Character_SayAt)},
        {"Character::SayBackground^1", API_FN_PAIR(Character_SayBackground)},
        // newer variadic variants
        {"Character::SayAt^104", Sc_Character_SayAt, ScPl_Character_SayAt},
        {"Character::SayBackground^101", Sc_Character_SayBackground, ScPl_Character_SayBackground},
        {"Character::SetAsPlayer^0", API_FN_PAIR(Character_SetAsPlayer)},
        {"Character::SetIdleView^2", API_FN_PAIR(Character_SetIdleView)},
        {"Character::SetLightLevel^1", API_FN_PAIR(Character_SetLightLevel)},
        {"Character::SetWalkSpeed^2", API_FN_PAIR(Character_SetSpeed)},
        {"Character::StopMoving^0", API_FN_PAIR(Character_StopMoving)},
        {"Character::Think^101", Sc_Character_Think, ScPl_Character_Think},
        {"Character::Tint^5", API_FN_PAIR(Character_Tint)},
        {"Character::UnlockView^0", API_FN_PAIR(Character_UnlockView)},
        {"Character::UnlockView^1", API_FN_PAIR(Character_UnlockViewEx)},
        {"Character::Walk^4", API_FN_PAIR(Character_Walk)},
        {"Character::WalkPath^4", API_FN_PAIR(Character_WalkPath)},
        {"Character::WalkStraight^3", API_FN_PAIR(Character_WalkStraight)},

        {"Character::get_ActiveInventory", API_FN_PAIR(Character_GetActiveInventory)},
        {"Character::set_ActiveInventory", API_FN_PAIR(Character_SetActiveInventory)},
        {"Character::get_Animating", API_FN_PAIR(Character_GetAnimating)},
        {"Character::get_AnimationSpeed", API_FN_PAIR(Character_GetAnimationSpeed)},
        {"Character::set_AnimationSpeed", API_FN_PAIR(Character_SetAnimationSpeed)},
        {"Character::get_AnimationVolume", API_FN_PAIR(Character_GetAudioVolume)},
        {"Character::set_AnimationVolume", API_FN_PAIR(Character_SetAudioVolume)},
        {"Character::get_AudioPanning", API_FN_PAIR(Character_GetAudioPanning)},
        {"Character::set_AudioPanning", API_FN_PAIR(Character_SetAudioPanning)},
        {"Character::get_AudioSpeed", API_FN_PAIR(Character_GetAudioSpeed)},
        {"Character::set_AudioSpeed", API_FN_PAIR(Character_SetAudioSpeed)},
        {"Character::get_AudioVolume", API_FN_PAIR(Character_GetAudioVolume)},
        {"Character::set_AudioVolume", API_FN_PAIR(Character_SetAudioVolume)},
        {"Character::get_Baseline", API_FN_PAIR(Character_GetBaseline)},
        {"Character::set_Baseline", API_FN_PAIR(Character_SetBaseline)},
        {"Character::get_BlinkInterval", API_FN_PAIR(Character_GetBlinkInterval)},
        {"Character::set_BlinkInterval", API_FN_PAIR(Character_SetBlinkInterval)},
        {"Character::get_BlinkView", API_FN_PAIR(Character_GetBlinkView)},
        {"Character::set_BlinkView", API_FN_PAIR(Character_SetBlinkView)},
        {"Character::get_BlinkWhileThinking", API_FN_PAIR(Character_GetBlinkWhileThinking)},
        {"Character::set_BlinkWhileThinking", API_FN_PAIR(Character_SetBlinkWhileThinking)},
        {"Character::get_BlockingHeight", API_FN_PAIR(Character_GetBlockingHeight)},
        {"Character::set_BlockingHeight", API_FN_PAIR(Character_SetBlockingHeight)},
        {"Character::get_BlockingWidth", API_FN_PAIR(Character_GetBlockingWidth)},
        {"Character::set_BlockingWidth", API_FN_PAIR(Character_SetBlockingWidth)},
        {"Character::get_BlockingRectX", API_FN_PAIR(Character_GetBlockingRectX)},
        {"Character::set_BlockingRectX", API_FN_PAIR(Character_SetBlockingRectX)},
        {"Character::get_BlockingRectY", API_FN_PAIR(Character_GetBlockingRectY)},
        {"Character::set_BlockingRectY", API_FN_PAIR(Character_SetBlockingRectY)},
        {"Character::get_Clickable", API_FN_PAIR(Character_GetClickable)},
        {"Character::set_Clickable", API_FN_PAIR(Character_SetClickable)},
        {"Character::get_DestinationX", API_FN_PAIR(Character_GetDestinationX)},
        {"Character::get_DestinationY", API_FN_PAIR(Character_GetDestinationY)},
        {"Character::get_DiagonalLoops", API_FN_PAIR(Character_GetDiagonalWalking)},
        {"Character::set_DiagonalLoops", API_FN_PAIR(Character_SetDiagonalWalking)},
        {"Character::get_DisplayName", API_FN_PAIR(Character_GetName)},
        {"Character::set_DisplayName", API_FN_PAIR(Character_SetName)},
        {"Character::get_Enabled", API_FN_PAIR(Character_GetEnabled)},
        {"Character::set_Enabled", API_FN_PAIR(Character_SetEnabled)},
        {"Character::get_Following", API_FN_PAIR(Character_GetFollowing)},
        {"Character::get_Frame", API_FN_PAIR(Character_GetFrame)},
        {"Character::set_Frame", API_FN_PAIR(Character_SetFrame)},
        {"Character::get_ID", API_FN_PAIR(Character_GetID)},
        {"Character::get_IdleDelay", API_FN_PAIR(Character_GetIdleDelay)},
        {"Character::get_IdleTime", API_FN_PAIR(Character_GetIdleTime)},
        {"Character::get_IdleView", API_FN_PAIR(Character_GetIdleView)},
        {"Character::get_IdleAnimationDelay", API_FN_PAIR(Character_GetIdleAnimationDelay)},
        {"Character::set_IdleAnimationDelay", API_FN_PAIR(Character_SetIdleAnimationDelay)},
        {"Character::geti_InventoryQuantity", API_FN_PAIR(Character_GetIInventoryQuantity)},
        {"Character::seti_InventoryQuantity", API_FN_PAIR(Character_SetIInventoryQuantity)},
        {"Character::get_IgnoreLighting", API_FN_PAIR(Character_GetIgnoreLighting)},
        {"Character::set_IgnoreLighting", API_FN_PAIR(Character_SetIgnoreLighting)},
        {"Character::get_InventoryCount", API_FN_PAIR(Character_GetInventoryCount)},
        {"Character::geti_Inventory", API_FN_PAIR(Character_GetInventory)},
        {"Character::get_Loop", API_FN_PAIR(Character_GetLoop)},
        {"Character::set_Loop", API_FN_PAIR(Character_SetLoop)},
        {"Character::get_ManualScaling", API_FN_PAIR(Character_GetManualScaling)},
        {"Character::set_ManualScaling", API_FN_PAIR(Character_SetManualScaling)},
        {"Character::get_MovementLinkedToAnimation", API_FN_PAIR(Character_GetMovementLinkedToAnimation)},
        {"Character::set_MovementLinkedToAnimation", API_FN_PAIR(Character_SetMovementLinkedToAnimation)},
        {"Character::get_Moving", API_FN_PAIR(Character_GetMoving)},
        // [OBSOLETE] Name => DisplayName
        {"Character::get_Name", API_FN_PAIR(Character_GetName)},
        {"Character::set_Name", API_FN_PAIR(Character_SetName)},
        {"Character::get_NormalView", API_FN_PAIR(Character_GetNormalView)},
        {"Character::get_PreviousRoom", API_FN_PAIR(Character_GetPreviousRoom)},
        {"Character::get_Room", API_FN_PAIR(Character_GetRoom)},
        {"Character::get_ScaleMoveSpeed", API_FN_PAIR(Character_GetScaleMoveSpeed)},
        {"Character::set_ScaleMoveSpeed", API_FN_PAIR(Character_SetScaleMoveSpeed)},
        {"Character::get_ScaleVolume", API_FN_PAIR(Character_GetScaleVolume)},
        {"Character::set_ScaleVolume", API_FN_PAIR(Character_SetScaleVolume)},
        {"Character::get_Scaling", API_FN_PAIR(Character_GetScaling)},
        {"Character::set_Scaling", API_FN_PAIR(Character_SetScaling)},
        {"Character::get_ScriptName", API_FN_PAIR(Character_GetScriptName)},
        {"Character::get_Solid", API_FN_PAIR(Character_GetSolid)},
        {"Character::set_Solid", API_FN_PAIR(Character_SetSolid)},
        {"Character::get_Speaking", API_FN_PAIR(Character_GetSpeaking)},
        {"Character::get_SpeakingFrame", API_FN_PAIR(Character_GetSpeakingFrame)},
        {"Character::get_SpeechAnimationDelay", API_FN_PAIR(Character_GetSpeechAnimationDelay)},
        {"Character::set_SpeechAnimationDelay", API_FN_PAIR(Character_SetSpeechAnimationDelay)},
        {"Character::get_SpeechColor", API_FN_PAIR(Character_GetSpeechColor)},
        {"Character::set_SpeechColor", API_FN_PAIR(Character_SetSpeechColor)},
        {"Character::get_SpeechView", API_FN_PAIR(Character_GetSpeechView)},
        {"Character::set_SpeechView", API_FN_PAIR(Character_SetSpeechView)},
        {"Character::get_Thinking", API_FN_PAIR(Character_GetThinking)},
        {"Character::get_ThinkingFrame", API_FN_PAIR(Character_GetThinkingFrame)},
        {"Character::get_ThinkView", API_FN_PAIR(Character_GetThinkView)},
        {"Character::set_ThinkView", API_FN_PAIR(Character_SetThinkView)},
        {"Character::get_Opacity", API_FN_PAIR(Character_GetOpacity)},
        {"Character::set_Opacity", API_FN_PAIR(Character_SetOpacity)},
        // [OBSOLETE] Transparency => Opacity
        {"Character::get_Transparency", API_FN_PAIR(Character_GetTransparency)},
        {"Character::set_Transparency", API_FN_PAIR(Character_SetTransparency)},
        {"Character::get_TurnBeforeWalking", API_FN_PAIR(Character_GetTurnBeforeWalking)},
        {"Character::set_TurnBeforeWalking", API_FN_PAIR(Character_SetTurnBeforeWalking)},
        {"Character::get_Turning", API_FN_PAIR(Character_GetTurning)},
        {"Character::get_TurnWhenFacing", API_FN_PAIR(Character_GetTurnWhenFacing)},
        {"Character::set_TurnWhenFacing", API_FN_PAIR(Character_SetTurnWhenFacing)},
        {"Character::get_View", API_FN_PAIR(Character_GetView)},
        {"Character::get_WalkSpeedX", API_FN_PAIR(Character_GetWalkSpeedX)},
        {"Character::get_WalkSpeedY", API_FN_PAIR(Character_GetWalkSpeedY)},
        {"Character::get_X", API_FN_PAIR(Character_GetX)},
        {"Character::set_X", API_FN_PAIR(Character_SetX)},
        {"Character::get_x", API_FN_PAIR(Character_GetX)},
        {"Character::set_x", API_FN_PAIR(Character_SetX)},
        {"Character::get_Y", API_FN_PAIR(Character_GetY)},
        {"Character::set_Y", API_FN_PAIR(Character_SetY)},
        {"Character::get_y", API_FN_PAIR(Character_GetY)},
        {"Character::set_y", API_FN_PAIR(Character_SetY)},
        {"Character::get_Z", API_FN_PAIR(Character_GetZ)},
        {"Character::set_Z", API_FN_PAIR(Character_SetZ)},
        {"Character::get_z", API_FN_PAIR(Character_GetZ)},
        {"Character::set_z", API_FN_PAIR(Character_SetZ)},
        {"Character::get_HasExplicitLight", API_FN_PAIR(Character_GetHasExplicitLight)},
        {"Character::get_HasExplicitTint", API_FN_PAIR(Character_GetHasExplicitTint)},
        {"Character::get_LightLevel", API_FN_PAIR(Character_GetLightLevel)},
        {"Character::get_TintBlue", API_FN_PAIR(Character_GetTintBlue)},
        {"Character::get_TintGreen", API_FN_PAIR(Character_GetTintGreen)},
        {"Character::get_TintRed", API_FN_PAIR(Character_GetTintRed)},
        {"Character::get_TintSaturation", API_FN_PAIR(Character_GetTintSaturation)},
        {"Character::get_TintLuminance", API_FN_PAIR(Character_GetTintLuminance)},
        {"Character::get_Visible", API_FN_PAIR(Character_GetVisible)},
        {"Character::set_Visible", API_FN_PAIR(Character_SetVisible)},

        {"Character::get_BlendMode", API_FN_PAIR(Character_GetBlendMode)},
        {"Character::set_BlendMode", API_FN_PAIR(Character_SetBlendMode)},
        {"Character::get_GraphicAnchorX", API_FN_PAIR(Character_GetGraphicAnchorX)},
        {"Character::set_GraphicAnchorX", API_FN_PAIR(Character_SetGraphicAnchorX)},
        {"Character::get_GraphicAnchorY", API_FN_PAIR(Character_GetGraphicAnchorY)},
        {"Character::set_GraphicAnchorY", API_FN_PAIR(Character_SetGraphicAnchorY)},
        {"Character::get_GraphicOffsetX", API_FN_PAIR(Character_GetGraphicOffsetX)},
        {"Character::set_GraphicOffsetX", API_FN_PAIR(Character_SetGraphicOffsetX)},
        {"Character::get_GraphicOffsetY", API_FN_PAIR(Character_GetGraphicOffsetY)},
        {"Character::set_GraphicOffsetY", API_FN_PAIR(Character_SetGraphicOffsetY)},
        {"Character::get_GraphicRotation", API_FN_PAIR(Character_GetRotation)},
        {"Character::set_GraphicRotation", API_FN_PAIR(Character_SetRotation)},
        {"Character::get_GraphicPivotX", API_FN_PAIR(Character_GetGraphicPivotX)},
        {"Character::set_GraphicPivotX", API_FN_PAIR(Character_SetGraphicPivotX)},
        {"Character::get_GraphicPivotY", API_FN_PAIR(Character_GetGraphicPivotY)},
        {"Character::set_GraphicPivotY", API_FN_PAIR(Character_SetGraphicPivotY)},
        {"Character::get_GraphicPivotOffsetX", API_FN_PAIR(Character_GetGraphicPivotOffsetX)},
        {"Character::set_GraphicPivotOffsetX", API_FN_PAIR(Character_SetGraphicPivotOffsetX)},
        {"Character::get_GraphicPivotOffsetY", API_FN_PAIR(Character_GetGraphicPivotOffsetY)},
        {"Character::set_GraphicPivotOffsetY", API_FN_PAIR(Character_SetGraphicPivotOffsetY)},
        {"Character::get_UseRegionTint", API_FN_PAIR(Character_GetUseRegionTint)},
        {"Character::set_UseRegionTint", API_FN_PAIR(Character_SetUseRegionTint)},
        {"Character::get_ViewAnchorX", API_FN_PAIR(Character_GetViewAnchorX)},
        {"Character::get_ViewAnchorY", API_FN_PAIR(Character_GetViewAnchorY)},
        {"Character::get_ViewOffsetX", API_FN_PAIR(Character_GetViewOffsetX)},
        {"Character::get_ViewOffsetY", API_FN_PAIR(Character_GetViewOffsetY)},

        {"Character::get_FaceDirectionRatio", API_FN_PAIR(Character_GetFaceDirectionRatio)},
        {"Character::set_FaceDirectionRatio", API_FN_PAIR(Character_SetFaceDirectionRatio)},
        {"Character::get_MotionPath", API_FN_PAIR(Character_GetMotionPath)},

        {"Character::get_Shader", API_FN_PAIR(Character_GetShader)},
        {"Character::set_Shader", API_FN_PAIR(Character_SetShader)},
    };

    ccAddExternalFunctions(character_api);
}
