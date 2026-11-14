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
#include "ac/game.h"
#include "game/dynobj/cc_object.h"
#include "game/dynobj/script_motion_path.h"
#include "game/dynobj/script_shader.h"
#include "game/dynobj/script_string.h"
#include "game/world/object.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_Object_GetByName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(ScriptObject, ccDynamicObject, Object_GetByName, const char);
}

// void (ScriptObject *obj, int loop, int delay, int repeat, int blocking, int direction)
RuntimeScriptValue Sc_Object_Animate5(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Animate5);
}

RuntimeScriptValue Sc_Object_Animate6(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT6(ScriptObject, Object_Animate6);
}

RuntimeScriptValue Sc_Object_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT7(ScriptObject, Object_Animate);
}

// int (ScriptObject *obj, ScriptObject *other)
RuntimeScriptValue Sc_Object_IsCollidingWithObject(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptObject, Object_IsCollidingWithObject, ScriptObject);
}

RuntimeScriptValue Sc_Object_GetGraphicPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, void, globalDynamicArray, Object_GetGraphicPosition);
}

RuntimeScriptValue Sc_Object_GetGraphicBoundBox(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, void, globalDynamicArray, Object_GetGraphicBoundBox);
}

// int (ScriptObject *obj, const char *property)
RuntimeScriptValue Sc_Object_GetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptObject, Object_GetProperty, const char);
}

// void (ScriptObject *obj, const char *property, char *bufer)
RuntimeScriptValue Sc_Object_GetPropertyText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ2(ScriptObject, Object_GetPropertyText, const char, char);
}

//const char* (ScriptObject *obj, const char *property)
RuntimeScriptValue Sc_Object_GetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ_POBJ(ScriptObject, const char, myScriptStringImpl, Object_GetTextProperty, const char);
}

RuntimeScriptValue Sc_Object_SetProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ_PINT(ScriptObject, Object_SetProperty, const char);
}

RuntimeScriptValue Sc_Object_SetTextProperty(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_POBJ2(ScriptObject, Object_SetTextProperty, const char, const char);
}

RuntimeScriptValue Sc_Object_IsInteractionAvailable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL_PINT(ScriptObject, Object_IsInteractionAvailable);
}

// void (ScriptObject *obj, int x, int y, int speed, int blocking, int direct)
RuntimeScriptValue Sc_Object_Move(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Move);
}

RuntimeScriptValue Sc_Object_MovePath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PINT4(ScriptObject, Object_MovePath, void);
}

// void (ScriptObject *obj)
RuntimeScriptValue Sc_Object_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_RemoveTint);
}

RuntimeScriptValue Sc_Object_RunFrameEvent(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptObject, Object_RunFrameEvent);
}

// void (ScriptObject *obj, int mode)
RuntimeScriptValue Sc_Object_RunInteraction(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_RunInteraction);
}

RuntimeScriptValue Sc_Object_HasExplicitLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_HasExplicitLight);
}

RuntimeScriptValue Sc_Object_HasExplicitTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_HasExplicitTint);
}

RuntimeScriptValue Sc_Object_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetLightLevel);
}

RuntimeScriptValue Sc_Object_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetLightLevel);
}

RuntimeScriptValue Sc_Object_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintBlue);
}

RuntimeScriptValue Sc_Object_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintGreen);
}

RuntimeScriptValue Sc_Object_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintRed);
}

RuntimeScriptValue Sc_Object_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintSaturation);
}

RuntimeScriptValue Sc_Object_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTintLuminance);
}

// void (ScriptObject *obj, int x, int y)
RuntimeScriptValue Sc_Object_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptObject, Object_SetPosition);
}

// void (ScriptObject *obj, int view, int loop, int frame)
RuntimeScriptValue Sc_Object_SetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptObject, Object_SetView);
}

// void (ScriptObject *obj)
RuntimeScriptValue Sc_Object_StopAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_StopAnimating);
}

// void (ScriptObject *obj)
RuntimeScriptValue Sc_Object_StopMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptObject, Object_StopMoving);
}

// void (ScriptObject *obj, int red, int green, int blue, int saturation, int luminance)
RuntimeScriptValue Sc_Object_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptObject, Object_Tint);
}

RuntimeScriptValue Sc_Object_GetAtRoomXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptObject, ccDynamicObject, Object_GetAtRoomXY);
}

RuntimeScriptValue Sc_Object_GetAtRoomXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptObject, ccDynamicObject, Object_GetAtRoomXY2);
}

RuntimeScriptValue Sc_Object_GetAtScreenXY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3(ScriptObject, ccDynamicObject, Object_GetAtScreenXY);
}

RuntimeScriptValue Sc_Object_GetAtScreenXY2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT2(ScriptObject, ccDynamicObject, Object_GetAtScreenXY2);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAnimating);
}

RuntimeScriptValue Sc_Object_GetAudioPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAudioPanning);
}

RuntimeScriptValue Sc_Object_SetAudioPanning(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetAudioPanning);
}

RuntimeScriptValue Sc_Object_GetAudioSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAudioSpeed);
}

RuntimeScriptValue Sc_Object_SetAudioSpeed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetAudioSpeed);
}

RuntimeScriptValue Sc_Object_GetAudioVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetAudioVolume);
}

RuntimeScriptValue Sc_Object_SetAudioVolume(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetAudioVolume);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBaseline);
}

// void (ScriptObject *obj, int basel)
RuntimeScriptValue Sc_Object_SetBaseline(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBaseline);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingHeight);
}

// void (ScriptObject *obj, int bhit)
RuntimeScriptValue Sc_Object_SetBlockingHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingHeight);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingWidth);
}

// void (ScriptObject *obj, int bwid)
RuntimeScriptValue Sc_Object_SetBlockingWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingWidth);
}

RuntimeScriptValue Sc_Object_GetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingRectX);
}

RuntimeScriptValue Sc_Object_SetBlockingRectX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingRectX);
}

RuntimeScriptValue Sc_Object_GetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlockingRectY);
}

RuntimeScriptValue Sc_Object_SetBlockingRectY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlockingRectY);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetClickable);
}

// void (ScriptObject *obj, int clik)
RuntimeScriptValue Sc_Object_SetClickable(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetClickable);
}

RuntimeScriptValue Sc_Object_GetDestinationX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetDestinationX);
}

RuntimeScriptValue Sc_Object_GetDestinationY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetDestinationY);
}

RuntimeScriptValue Sc_Object_GetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_GetEnabled);
}

RuntimeScriptValue Sc_Object_SetEnabled(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptObject, Object_SetEnabled);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetFrame);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphic);
}

// void (ScriptObject *obj, int slott)
RuntimeScriptValue Sc_Object_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphic);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetID(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetID);
}

RuntimeScriptValue Sc_Object_GetScriptName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, const char, myScriptStringImpl, Object_GetScriptName);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetLoop);
}

RuntimeScriptValue Sc_Object_GetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_GetManualScaling);
}

RuntimeScriptValue Sc_Object_SetManualScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptObject, Object_SetManualScaling);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetMoving(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetMoving);
}

// const char* (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetName_New(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(ScriptObject, const char, myScriptStringImpl, Object_GetName_New);
}

RuntimeScriptValue Sc_Object_SetName(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptObject, Object_SetName, const char);
}

RuntimeScriptValue Sc_Object_GetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetScaling);
}

RuntimeScriptValue Sc_Object_SetScaling(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetScaling);
}


// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetSolid);
}

// void (ScriptObject *obj, int solid)
RuntimeScriptValue Sc_Object_SetSolid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetSolid);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetOpacity);
}

// void (ScriptObject *obj, int alpha)
RuntimeScriptValue Sc_Object_SetOpacity(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetOpacity);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetTransparency);
}

// void (ScriptObject *obj, int trans)
RuntimeScriptValue Sc_Object_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetTransparency);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetView);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_GetVisible);
}

// void (ScriptObject *obj, int onoroff)
RuntimeScriptValue Sc_Object_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptObject, Object_SetVisible);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetX);
}

// void (ScriptObject *obj, int x)
RuntimeScriptValue Sc_Object_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetX);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetY);
}

// void (ScriptObject *obj, int y)
RuntimeScriptValue Sc_Object_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetY);
}

// int (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetBlendMode);
}

// void (ScriptObject *obj, int blendMode)
RuntimeScriptValue Sc_Object_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetBlendMode);
}

RuntimeScriptValue Sc_Object_GetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptObject, Object_GetGraphicAnchorX);
}

RuntimeScriptValue Sc_Object_SetGraphicAnchorX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptObject, Object_SetGraphicAnchorX);
}

RuntimeScriptValue Sc_Object_GetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptObject, Object_GetGraphicAnchorY);
}

RuntimeScriptValue Sc_Object_SetGraphicAnchorY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptObject, Object_SetGraphicAnchorY);
}

RuntimeScriptValue Sc_Object_GetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphicOffsetX);
}

RuntimeScriptValue Sc_Object_SetGraphicOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphicOffsetX);
}

RuntimeScriptValue Sc_Object_GetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphicOffsetY);
}

RuntimeScriptValue Sc_Object_SetGraphicOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphicOffsetY);
}

RuntimeScriptValue Sc_Object_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptObject, ScriptShaderInstance, Object_GetShader);
}

// void (ScriptObject *obj, int blendMode)
RuntimeScriptValue Sc_Object_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptObject, Object_SetShader, ScriptShaderInstance);
}

// bool (ScriptObject *obj)
RuntimeScriptValue Sc_Object_GetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptObject, Object_GetUseRegionTint);
}

// void (ScriptObject *obj, int yesorno)
RuntimeScriptValue Sc_Object_SetUseRegionTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetUseRegionTint);
}

RuntimeScriptValue Sc_Object_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptObject, Object_GetRotation);
}

RuntimeScriptValue Sc_Object_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptObject, Object_SetRotation);
}

RuntimeScriptValue Sc_Object_GetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptObject, Object_GetGraphicPivotX);
}

RuntimeScriptValue Sc_Object_SetGraphicPivotX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptObject, Object_SetGraphicPivotX);
}

RuntimeScriptValue Sc_Object_GetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptObject, Object_GetGraphicPivotY);
}

RuntimeScriptValue Sc_Object_SetGraphicPivotY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptObject, Object_SetGraphicPivotY);
}

RuntimeScriptValue Sc_Object_GetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Object_SetGraphicPivotOffsetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphicPivotOffsetX);
}

RuntimeScriptValue Sc_Object_GetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptObject, Object_GetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Object_SetGraphicPivotOffsetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptObject, Object_SetGraphicPivotOffsetY);
}

RuntimeScriptValue Sc_Object_GetMotionPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptObject, ScriptMotionPath, Object_GetMotionPath);
}


void RegisterObjectAPI()
{
    ScFnRegister object_api[] = {
        {"Object::GetAtRoomXY^2", API_FN_PAIR(Object_GetAtRoomXY2)},
        {"Object::GetAtScreenXY^2", API_FN_PAIR(Object_GetAtScreenXY2)},
        {"Object::GetAtRoomXY^3", API_FN_PAIR(Object_GetAtRoomXY)},
        {"Object::GetAtScreenXY^3", API_FN_PAIR(Object_GetAtScreenXY)},
        // [OBSOLETE] GetByName => GetByScriptName
        {"Object::GetByName", API_FN_PAIR(Object_GetByName)},
        {"Object::GetByScriptName", API_FN_PAIR(Object_GetByName)},

        {"Object::Animate^5", API_FN_PAIR(Object_Animate5)},
        {"Object::Animate^6", API_FN_PAIR(Object_Animate6)},
        {"Object::Animate^7", API_FN_PAIR(Object_Animate)},
        {"Object::IsCollidingWithObject^1", API_FN_PAIR(Object_IsCollidingWithObject)},
        {"Object::GetGraphicPosition^0", API_FN_PAIR(Object_GetGraphicPosition)},
        {"Object::GetGraphicBoundBox^0", API_FN_PAIR(Object_GetGraphicBoundBox)},
        {"Object::GetProperty^1", API_FN_PAIR(Object_GetProperty)},
        {"Object::GetPropertyText^2", API_FN_PAIR(Object_GetPropertyText)},
        {"Object::GetTextProperty^1", API_FN_PAIR(Object_GetTextProperty)},
        {"Object::SetProperty^2", API_FN_PAIR(Object_SetProperty)},
        {"Object::SetTextProperty^2", API_FN_PAIR(Object_SetTextProperty)},
        {"Object::IsInteractionAvailable^1", API_FN_PAIR(Object_IsInteractionAvailable)},
        {"Object::Move^5", API_FN_PAIR(Object_Move)},
        {"Object::MovePath^5", API_FN_PAIR(Object_MovePath)},
        {"Object::RemoveTint^0", API_FN_PAIR(Object_RemoveTint)},
        {"Object::RunFrameEvent^3", API_FN_PAIR(Object_RunFrameEvent)},
        {"Object::RunInteraction^1", API_FN_PAIR(Object_RunInteraction)},
        {"Object::SetLightLevel^1", API_FN_PAIR(Object_SetLightLevel)},
        {"Object::SetPosition^2", API_FN_PAIR(Object_SetPosition)},
        {"Object::SetView^3", API_FN_PAIR(Object_SetView)},
        {"Object::StopAnimating^0", API_FN_PAIR(Object_StopAnimating)},
        {"Object::StopMoving^0", API_FN_PAIR(Object_StopMoving)},
        {"Object::Tint^5", API_FN_PAIR(Object_Tint)},
        {"Object::get_Animating", API_FN_PAIR(Object_GetAnimating)},
        {"Object::get_AnimationVolume", API_FN_PAIR(Object_GetAudioVolume)},
        {"Object::set_AnimationVolume", API_FN_PAIR(Object_SetAudioVolume)},
        {"Object::get_AudioPanning", API_FN_PAIR(Object_GetAudioPanning)},
        {"Object::set_AudioPanning", API_FN_PAIR(Object_SetAudioPanning)},
        {"Object::get_AudioSpeed", API_FN_PAIR(Object_GetAudioSpeed)},
        {"Object::set_AudioSpeed", API_FN_PAIR(Object_SetAudioSpeed)},
        {"Object::get_AudioVolume", API_FN_PAIR(Object_GetAudioVolume)},
        {"Object::set_AudioVolume", API_FN_PAIR(Object_SetAudioVolume)},
        {"Object::get_Baseline", API_FN_PAIR(Object_GetBaseline)},
        {"Object::set_Baseline", API_FN_PAIR(Object_SetBaseline)},
        {"Object::get_BlockingHeight", API_FN_PAIR(Object_GetBlockingHeight)},
        {"Object::set_BlockingHeight", API_FN_PAIR(Object_SetBlockingHeight)},
        {"Object::get_BlockingWidth", API_FN_PAIR(Object_GetBlockingWidth)},
        {"Object::set_BlockingWidth", API_FN_PAIR(Object_SetBlockingWidth)},
        {"Object::get_BlockingRectX", API_FN_PAIR(Object_GetBlockingRectX)},
        {"Object::set_BlockingRectX", API_FN_PAIR(Object_SetBlockingRectX)},
        {"Object::get_BlockingRectY", API_FN_PAIR(Object_GetBlockingRectY)},
        {"Object::set_BlockingRectY", API_FN_PAIR(Object_SetBlockingRectY)},
        {"Object::get_Clickable", API_FN_PAIR(Object_GetClickable)},
        {"Object::set_Clickable", API_FN_PAIR(Object_SetClickable)},
        {"Object::get_DestinationX", API_FN_PAIR(Object_GetDestinationX)},
        {"Object::get_DestinationY", API_FN_PAIR(Object_GetDestinationY)},
        {"Object::get_DisplayName", API_FN_PAIR(Object_GetName_New)},
        {"Object::set_DisplayName", API_FN_PAIR(Object_SetName)},
        {"Object::get_Enabled", API_FN_PAIR(Object_GetEnabled)},
        {"Object::set_Enabled", API_FN_PAIR(Object_SetEnabled)},
        {"Object::get_Frame", API_FN_PAIR(Object_GetFrame)},
        {"Object::get_Graphic", API_FN_PAIR(Object_GetGraphic)},
        {"Object::set_Graphic", API_FN_PAIR(Object_SetGraphic)},
        {"Object::get_ID", API_FN_PAIR(Object_GetID)},
        {"Object::get_Loop", API_FN_PAIR(Object_GetLoop)},
        {"Object::get_ManualScaling", API_FN_PAIR(Object_GetManualScaling)},
        {"Object::set_ManualScaling", API_FN_PAIR(Object_SetManualScaling)},
        {"Object::get_Moving", API_FN_PAIR(Object_GetMoving)},
        // [OBSOLETE] Name => DisplayName
        {"Object::get_Name", API_FN_PAIR(Object_GetName_New)},
        {"Object::set_Name", API_FN_PAIR(Object_SetName)},
        {"Object::get_Scaling", API_FN_PAIR(Object_GetScaling)},
        {"Object::set_Scaling", API_FN_PAIR(Object_SetScaling)},
        {"Object::get_ScriptName", API_FN_PAIR(Object_GetScriptName)},
        {"Object::get_Solid", API_FN_PAIR(Object_GetSolid)},
        {"Object::set_Solid", API_FN_PAIR(Object_SetSolid)},
        {"Object::get_Opacity", API_FN_PAIR(Object_GetOpacity)},
        {"Object::set_Opacity", API_FN_PAIR(Object_SetOpacity)},
        // [OBSOLETE] Transparency => Opacity
        {"Object::get_Transparency", API_FN_PAIR(Object_GetTransparency)},
        {"Object::set_Transparency", API_FN_PAIR(Object_SetTransparency)},
        {"Object::get_View", API_FN_PAIR(Object_GetView)},
        {"Object::get_Visible", API_FN_PAIR(Object_GetVisible)},
        {"Object::set_Visible", API_FN_PAIR(Object_SetVisible)},
        {"Object::get_X", API_FN_PAIR(Object_GetX)},
        {"Object::set_X", API_FN_PAIR(Object_SetX)},
        {"Object::get_Y", API_FN_PAIR(Object_GetY)},
        {"Object::set_Y", API_FN_PAIR(Object_SetY)},
        {"Object::get_HasExplicitLight", API_FN_PAIR(Object_HasExplicitLight)},
        {"Object::get_HasExplicitTint", API_FN_PAIR(Object_HasExplicitTint)},
        {"Object::get_LightLevel", API_FN_PAIR(Object_GetLightLevel)},
        {"Object::set_LightLevel", API_FN_PAIR(Object_SetLightLevel)},
        {"Object::get_TintBlue", API_FN_PAIR(Object_GetTintBlue)},
        {"Object::get_TintGreen", API_FN_PAIR(Object_GetTintGreen)},
        {"Object::get_TintRed", API_FN_PAIR(Object_GetTintRed)},
        {"Object::get_TintSaturation", API_FN_PAIR(Object_GetTintSaturation)},
        {"Object::get_TintLuminance", API_FN_PAIR(Object_GetTintLuminance)},

        {"Object::get_BlendMode", API_FN_PAIR(Object_GetBlendMode)},
        {"Object::set_BlendMode", API_FN_PAIR(Object_SetBlendMode)},
        {"Object::get_GraphicAnchorX", API_FN_PAIR(Object_GetGraphicAnchorX)},
        {"Object::set_GraphicAnchorX", API_FN_PAIR(Object_SetGraphicAnchorX)},
        {"Object::get_GraphicAnchorY", API_FN_PAIR(Object_GetGraphicAnchorY)},
        {"Object::set_GraphicAnchorY", API_FN_PAIR(Object_SetGraphicAnchorY)},
        {"Object::get_GraphicOffsetX", API_FN_PAIR(Object_GetGraphicOffsetX)},
        {"Object::set_GraphicOffsetX", API_FN_PAIR(Object_SetGraphicOffsetX)},
        {"Object::get_GraphicOffsetY", API_FN_PAIR(Object_GetGraphicOffsetY)},
        {"Object::set_GraphicOffsetY", API_FN_PAIR(Object_SetGraphicOffsetY)},
        {"Object::get_GraphicRotation", API_FN_PAIR(Object_GetRotation)},
        {"Object::set_GraphicRotation", API_FN_PAIR(Object_SetRotation)},
        {"Object::get_GraphicPivotX", API_FN_PAIR(Object_GetGraphicPivotX)},
        {"Object::set_GraphicPivotX", API_FN_PAIR(Object_SetGraphicPivotX)},
        {"Object::get_GraphicPivotY", API_FN_PAIR(Object_GetGraphicPivotY)},
        {"Object::set_GraphicPivotY", API_FN_PAIR(Object_SetGraphicPivotY)},
        {"Object::get_GraphicPivotOffsetX", API_FN_PAIR(Object_GetGraphicPivotOffsetX)},
        {"Object::set_GraphicPivotOffsetX", API_FN_PAIR(Object_SetGraphicPivotOffsetX)},
        {"Object::get_GraphicPivotOffsetY", API_FN_PAIR(Object_GetGraphicPivotOffsetY)},
        {"Object::set_GraphicPivotOffsetY", API_FN_PAIR(Object_SetGraphicPivotOffsetY)},
        {"Object::get_UseRegionTint", API_FN_PAIR(Object_GetUseRegionTint)},
        {"Object::set_UseRegionTint", API_FN_PAIR(Object_SetUseRegionTint)},

        {"Object::get_MotionPath", API_FN_PAIR(Object_GetMotionPath)},

        {"Object::get_Shader", API_FN_PAIR(Object_GetShader)},
        {"Object::set_Shader", API_FN_PAIR(Object_SetShader)},
    };

    ccAddExternalFunctions(object_api);
}
