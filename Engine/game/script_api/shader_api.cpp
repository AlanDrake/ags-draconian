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
#include "ac/dynobj/scriptshader.h"
#include "game/render/shader_script.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_ShaderProgram_CreateFromFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptShaderProgram, ShaderProgram_CreateFromFile, const char);
}

RuntimeScriptValue Sc_ShaderProgram_CreateInstance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderProgram, ScriptShaderInstance, ShaderProgram_CreateInstance);
}

RuntimeScriptValue Sc_ShaderProgram_GetDefault(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderProgram, ScriptShaderInstance, ShaderProgram_GetDefault);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT(ScriptShaderInstance, ShaderInstance_SetConstantF, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF2(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT2(ScriptShaderInstance, ShaderInstance_SetConstantF2, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF3(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT3(ScriptShaderInstance, ShaderInstance_SetConstantF3, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetConstantF4(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ_PFLOAT4(ScriptShaderInstance, ShaderInstance_SetConstantF4, const char);
}

RuntimeScriptValue Sc_ShaderInstance_SetTexture(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptShaderInstance, ShaderInstance_SetTexture);
}

RuntimeScriptValue Sc_ShaderInstance_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptShaderInstance, ScriptShaderProgram, ShaderInstance_GetShader);
}

void RegisterShaderAPI()
{
    ScFnRegister shader_api[] = {
        {"ShaderProgram::CreateFromFile", API_FN_PAIR(ShaderProgram_CreateFromFile)},
        {"ShaderProgram::CreateInstance", API_FN_PAIR(ShaderProgram_CreateInstance)},
        {"ShaderProgram::get_Default", API_FN_PAIR(ShaderProgram_GetDefault)},
        {"ShaderInstance::SetConstantF", API_FN_PAIR(ShaderInstance_SetConstantF)},
        {"ShaderInstance::SetConstantF2", API_FN_PAIR(ShaderInstance_SetConstantF2)},
        {"ShaderInstance::SetConstantF3", API_FN_PAIR(ShaderInstance_SetConstantF3)},
        {"ShaderInstance::SetConstantF4", API_FN_PAIR(ShaderInstance_SetConstantF4)},
        {"ShaderInstance::SetTexture", API_FN_PAIR(ShaderInstance_SetTexture)},
        {"ShaderInstance::get_Shader", API_FN_PAIR(ShaderInstance_GetShader)},
    };

    ccAddExternalFunctions(shader_api);
}
