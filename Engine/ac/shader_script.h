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
// Shader script API.
//
//=============================================================================
#pragma once

#include "ac/dynobj/scriptshader.h"

// Creates a new shader and registers a ScriptShaderProgram object
ScriptShaderProgram *CreateScriptShaderProgram(const char *filename);
// Creates a new shader instance and registers a ScriptShaderInstance object
ScriptShaderInstance *ShaderProgram_CreateInstance(ScriptShaderProgram *sc_shader);
// Restore shaders and shader instances after loading a save
void RestoreShaders();

// Script API: ShaderProgram::CreateFromFile
ScriptShaderProgram *ShaderProgram_CreateFromFile(const char *filename);

// Script API: ShaderProgram::get_Default
ScriptShaderInstance *ShaderProgram_GetDefault(ScriptShaderProgram *shader_prg);

// Script API: ShaderInstance::SetConstantF
void ShaderInstance_SetConstantF(ScriptShaderInstance *sc_shinst, const char *name, float value);

// Script API: ShaderInstance::SetConstantF2
void ShaderInstance_SetConstantF2(ScriptShaderInstance *sc_shinst, const char *name, float x, float y);

// Script API: ShaderInstance::SetConstantF3
void ShaderInstance_SetConstantF3(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z);

// Script API: ShaderInstance::SetConstantF4
void ShaderInstance_SetConstantF4(ScriptShaderInstance *sc_shinst, const char *name, float x, float y, float z,
                                  float w);

// Script API: ShaderInstance::SetTexture
void ShaderInstance_SetTexture(ScriptShaderInstance *sc_shinst, int index, int sprite);

// Script API: ShaderInstance::get_Shader
ScriptShaderProgram *ShaderInstance_GetShader(ScriptShaderInstance *sc_shinst);
