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
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterDynamicSpriteAPI()
{
    // WARNING: DynamicSprite.Create and CreateFromExistingSprite have 1 param REPLACED since ags4
    ScFnRegister dynsprite_api[] = {
        ScStatic<&DynamicSprite_Create>("DynamicSprite::Create^3"),
        ScStatic<&DynamicSprite_CreateFromBackground>("DynamicSprite::CreateFromBackground"),
        ScStatic<&DynamicSprite_CreateFromDrawingSurface>("DynamicSprite::CreateFromDrawingSurface^6"),
        ScStatic<&DynamicSprite_CreateFromExistingSprite>("DynamicSprite::CreateFromExistingSprite^2"),
        ScStatic<&DynamicSprite_CreateFromFile>("DynamicSprite::CreateFromFile^2"),
        ScStatic<&DynamicSprite_CreateFromSaveGame>("DynamicSprite::CreateFromSaveGame"),
        ScStatic<&DynamicSprite_CreateFromScreenShot2>("DynamicSprite::CreateFromScreenShot^2"),
        ScStatic<&DynamicSprite_CreateFromScreenShot>("DynamicSprite::CreateFromScreenShot^3"),
        ScMethod<&DynamicSprite_ChangeCanvasSize>("DynamicSprite::ChangeCanvasSize^4"),
        ScMethod<&DynamicSprite_CopyTransparencyMask>("DynamicSprite::CopyTransparencyMask^1"),
        ScMethod<&DynamicSprite_Crop>("DynamicSprite::Crop^4"),
        ScMethod<&DynamicSprite_Delete>("DynamicSprite::Delete"),
        ScMethod<&DynamicSprite_Flip>("DynamicSprite::Flip^1"),
        ScMethod<&DynamicSprite_GetDrawingSurface>("DynamicSprite::GetDrawingSurface^0"),
        ScMethod<&DynamicSprite_Resize>("DynamicSprite::Resize^2"),
        ScMethod<&DynamicSprite_Rotate>("DynamicSprite::Rotate^3"),
        ScMethod<&DynamicSprite_SaveToFile>("DynamicSprite::SaveToFile^1"),
        ScMethod<&DynamicSprite_Tint>("DynamicSprite::Tint^5"),
        ScMethod<&DynamicSprite_GetColorDepth>("DynamicSprite::get_ColorDepth"),
        ScMethod<&DynamicSprite_GetGraphic>("DynamicSprite::get_Graphic"),
        ScMethod<&DynamicSprite_GetHeight>("DynamicSprite::get_Height"),
        ScMethod<&DynamicSprite_GetWidth>("DynamicSprite::get_Width"),
    };

    ccAddExternalFunctions(dynsprite_api);
}
