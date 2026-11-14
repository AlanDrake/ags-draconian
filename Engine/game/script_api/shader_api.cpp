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
// Script bindings registered by RegisterShaderAPI().
//
//=============================================================================
#include "game/dynobj/script_shader.h"
#include "game/render/shader_script.h"
#include "game/script_api/bindings.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

void RegisterShaderAPI()
{
    ScFnRegister shader_api[] = {
        ScStatic<&ShaderProgram_CreateFromFile>("ShaderProgram::CreateFromFile"),
        ScMethod<&ShaderProgram_CreateInstance>("ShaderProgram::CreateInstance"),
        ScMethod<&ShaderProgram_GetDefault>("ShaderProgram::get_Default"),
        ScMethod<&ShaderInstance_SetConstantF>("ShaderInstance::SetConstantF"),
        ScMethod<&ShaderInstance_SetConstantF2>("ShaderInstance::SetConstantF2"),
        ScMethod<&ShaderInstance_SetConstantF3>("ShaderInstance::SetConstantF3"),
        ScMethod<&ShaderInstance_SetConstantF4>("ShaderInstance::SetConstantF4"),
        ScMethod<&ShaderInstance_SetTexture>("ShaderInstance::SetTexture"),
        ScMethod<&ShaderInstance_GetShader>("ShaderInstance::get_Shader"),
    };

    ccAddExternalFunctions(shader_api);
}
