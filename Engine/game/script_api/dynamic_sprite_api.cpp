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
// Script bindings registered by RegisterDynamicSpriteAPI().
//
//=============================================================================
#include "game/render/dynamic_sprite.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

// void (ScriptDynamicSprite *dynsprite, int width, int height, int x, int y)
RuntimeScriptValue Sc_DynamicSprite_ChangeCanvasSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_ChangeCanvasSize);
}

// void (ScriptDynamicSprite *dynsprite, int source_sprite)
RuntimeScriptValue Sc_DynamicSprite_CopyTransparencyMask(void *self, const RuntimeScriptValue *params,
                                                         int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_CopyTransparencyMask);
}

// void (ScriptDynamicSprite *dynsprite, int x, int y, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Crop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptDynamicSprite, DynamicSprite_Crop);
}

// void (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_Delete(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptDynamicSprite, DynamicSprite_Delete);
}

// void (ScriptDynamicSprite *dynsprite, int direction)
RuntimeScriptValue Sc_DynamicSprite_Flip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptDynamicSprite, DynamicSprite_Flip);
}

// ScriptDrawingSurface* (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetDrawingSurface(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptDynamicSprite, ScriptDrawingSurface, DynamicSprite_GetDrawingSurface);
}

// void (ScriptDynamicSprite *dynsprite, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Resize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptDynamicSprite, DynamicSprite_Resize);
}

// void (ScriptDynamicSprite *dynsprite, int angle, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_Rotate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT3(ScriptDynamicSprite, DynamicSprite_Rotate);
}

// int (ScriptDynamicSprite *dynsprite, const char* name)
RuntimeScriptValue Sc_DynamicSprite_SaveToFile(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ(ScriptDynamicSprite, DynamicSprite_SaveToFile, const char);
}

// void (ScriptDynamicSprite *dynsprite, int red, int green, int blue, int saturation, int luminance)
RuntimeScriptValue Sc_DynamicSprite_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptDynamicSprite, DynamicSprite_Tint);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetColorDepth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetColorDepth);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetGraphic);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetHeight);
}

// int (ScriptDynamicSprite *dynsprite)
RuntimeScriptValue Sc_DynamicSprite_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDynamicSprite, DynamicSprite_GetWidth);
}

RuntimeScriptValue Sc_DynamicSprite_Create(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_Create);
}

// ScriptDynamicSprite* (int frame, int x1, int y1, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromBackground);
}

// ScriptDynamicSprite* (ScriptDrawingSurface *surface, int x, int y, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromDrawingSurface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT5(ScriptDynamicSprite, DynamicSprite_CreateFromDrawingSurface, ScriptDrawingSurface);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromExistingSprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromExistingSprite);
}

// ScriptDynamicSprite* (const char *filename)
RuntimeScriptValue Sc_DynamicSprite_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT(ScriptDynamicSprite, DynamicSprite_CreateFromFile, const char);
}

// ScriptDynamicSprite* (int save_slot, int width, int height)
RuntimeScriptValue Sc_DynamicSprite_CreateFromSaveGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromSaveGame);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot);
}

RuntimeScriptValue Sc_DynamicSprite_CreateFromScreenShot2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT2(ScriptDynamicSprite, DynamicSprite_CreateFromScreenShot2);
}


void RegisterDynamicSpriteAPI()
{
    // WARNING: DynamicSprite.Create and CreateFromExistingSprite have 1 param REPLACED since ags4
    ScFnRegister dynsprite_api[] = {
        {"DynamicSprite::Create^3", API_FN_PAIR(DynamicSprite_Create)},
        {"DynamicSprite::CreateFromBackground", API_FN_PAIR(DynamicSprite_CreateFromBackground)},
        {"DynamicSprite::CreateFromDrawingSurface^6", API_FN_PAIR(DynamicSprite_CreateFromDrawingSurface)},
        {"DynamicSprite::CreateFromExistingSprite^2", API_FN_PAIR(DynamicSprite_CreateFromExistingSprite)},
        {"DynamicSprite::CreateFromFile^2", API_FN_PAIR(DynamicSprite_CreateFromFile)},
        {"DynamicSprite::CreateFromSaveGame", API_FN_PAIR(DynamicSprite_CreateFromSaveGame)},
        {"DynamicSprite::CreateFromScreenShot^2", API_FN_PAIR(DynamicSprite_CreateFromScreenShot2)},
        {"DynamicSprite::CreateFromScreenShot^3", API_FN_PAIR(DynamicSprite_CreateFromScreenShot)},
        {"DynamicSprite::ChangeCanvasSize^4", API_FN_PAIR(DynamicSprite_ChangeCanvasSize)},
        {"DynamicSprite::CopyTransparencyMask^1", API_FN_PAIR(DynamicSprite_CopyTransparencyMask)},
        {"DynamicSprite::Crop^4", API_FN_PAIR(DynamicSprite_Crop)},
        {"DynamicSprite::Delete", API_FN_PAIR(DynamicSprite_Delete)},
        {"DynamicSprite::Flip^1", API_FN_PAIR(DynamicSprite_Flip)},
        {"DynamicSprite::GetDrawingSurface^0", API_FN_PAIR(DynamicSprite_GetDrawingSurface)},
        {"DynamicSprite::Resize^2", API_FN_PAIR(DynamicSprite_Resize)},
        {"DynamicSprite::Rotate^3", API_FN_PAIR(DynamicSprite_Rotate)},
        {"DynamicSprite::SaveToFile^1", API_FN_PAIR(DynamicSprite_SaveToFile)},
        {"DynamicSprite::Tint^5", API_FN_PAIR(DynamicSprite_Tint)},
        {"DynamicSprite::get_ColorDepth", API_FN_PAIR(DynamicSprite_GetColorDepth)},
        {"DynamicSprite::get_Graphic", API_FN_PAIR(DynamicSprite_GetGraphic)},
        {"DynamicSprite::get_Height", API_FN_PAIR(DynamicSprite_GetHeight)},
        {"DynamicSprite::get_Width", API_FN_PAIR(DynamicSprite_GetWidth)},
    };

    ccAddExternalFunctions(dynsprite_api);
}
