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
// Script bindings registered by RegisterFileAPI().
//
//=============================================================================
#include "ac/file.h"
#include "game/dynobj/script_string.h"
#include "platform/base/agsplatformdriver.h"
#include "script/dynobj/cc_dynamic_array.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

using namespace AGS::Common;
using namespace AGS::Engine;

RuntimeScriptValue Sc_File_Copy(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(File_Copy, const char, const char);
}

// int (const char *filename)
RuntimeScriptValue Sc_File_Delete(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(File_Delete, const char);
}

// int (const char *filename)
RuntimeScriptValue Sc_File_Exists(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(File_Exists, const char);
}

RuntimeScriptValue Sc_File_GetFileTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ(ScriptDateTime, File_GetFileTime, const char);
}

RuntimeScriptValue Sc_File_Rename(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(File_Rename, const char, const char);
}

RuntimeScriptValue Sc_File_GetFiles(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ_PINT2(void, globalDynamicArray, File_GetFiles, const char);
}

// void *(const char *filename, int mode)
RuntimeScriptValue Sc_sc_OpenFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_POBJ_PINT(sc_File, sc_OpenFile, const char);
}

RuntimeScriptValue Sc_File_ResolvePath(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(const char, myScriptStringImpl, File_ResolvePath, const char);
}

// void (sc_File *file)
RuntimeScriptValue Sc_File_Close(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(sc_File, File_Close);
}

// int (sc_File *file)
RuntimeScriptValue Sc_File_ReadInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadInt);
}

RuntimeScriptValue Sc_File_ReadFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(sc_File, File_ReadFloat);
}

// int (sc_File *file)
RuntimeScriptValue Sc_File_ReadRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadRawChar);
}

RuntimeScriptValue Sc_File_ReadRawFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(sc_File, File_ReadRawFloat);
}

// int (sc_File *file)
RuntimeScriptValue Sc_File_ReadRawInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_ReadRawInt);
}

// void (sc_File *file, char* buffer)
RuntimeScriptValue Sc_File_ReadRawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_ReadRawLine, char);
}

// const char* (sc_File *file)
RuntimeScriptValue Sc_File_ReadRawLineBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_ReadRawLineBack);
}

// void (sc_File *file, char *toread)
RuntimeScriptValue Sc_File_ReadString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_ReadString, char);
}

// const char* (sc_File *file)
RuntimeScriptValue Sc_File_ReadStringBack(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_ReadStringBack);
}

// void (sc_File *file, int towrite)
RuntimeScriptValue Sc_File_WriteInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteInt);
}

RuntimeScriptValue Sc_File_WriteFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(sc_File, File_WriteFloat);
}

// void (sc_File *file, int towrite)
RuntimeScriptValue Sc_File_WriteRawChar(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteRawChar);
}

RuntimeScriptValue Sc_File_WriteRawFloat(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(sc_File, File_WriteRawFloat);
}

RuntimeScriptValue Sc_File_WriteRawInt(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(sc_File, File_WriteRawInt);
}

// void (sc_File *file, const char *towrite)
RuntimeScriptValue Sc_File_WriteRawLine(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_WriteRawLine, const char);
}

// void (sc_File *file, const char *towrite)
RuntimeScriptValue Sc_File_WriteString(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(sc_File, File_WriteString, const char);
}

RuntimeScriptValue Sc_File_ReadRawBytes(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PINT2(sc_File, File_ReadRawBytes, void);
}

RuntimeScriptValue Sc_File_WriteRawBytes(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_POBJ_PINT2(sc_File, File_WriteRawBytes, void);
}

RuntimeScriptValue Sc_File_Seek(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT_PINT2(sc_File, File_Seek);
}

// int (sc_File *file)
RuntimeScriptValue Sc_File_GetEOF(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetEOF);
}

// int (sc_File *file)
RuntimeScriptValue Sc_File_GetError(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetError);
}

RuntimeScriptValue Sc_File_GetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(sc_File, File_GetPosition);
}

RuntimeScriptValue Sc_File_GetPath(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJ(sc_File, const char, myScriptStringImpl, File_GetPath);
}


void RegisterFileAPI()
{
    ScFnRegister file_api[] = {
        {"File::Copy^2", API_FN_PAIR(File_Copy)},
        {"File::Delete^1", API_FN_PAIR(File_Delete)},
        {"File::Exists^1", API_FN_PAIR(File_Exists)},
        {"File::GetFileTime^1", API_FN_PAIR(File_GetFileTime)},
        {"File::GetFiles^3", API_FN_PAIR(File_GetFiles)},
        {"File::Rename^2", API_FN_PAIR(File_Rename)},
        {"File::Open^2", API_FN_PAIR(sc_OpenFile)},
        {"File::ResolvePath^1", API_FN_PAIR(File_ResolvePath)},

        {"File::Close^0", API_FN_PAIR(File_Close)},
        {"File::ReadInt^0", API_FN_PAIR(File_ReadInt)},
        {"File::ReadFloat^0", API_FN_PAIR(File_ReadFloat)},
        {"File::ReadRawChar^0", API_FN_PAIR(File_ReadRawChar)},
        {"File::ReadRawInt^0", API_FN_PAIR(File_ReadRawInt)},
        {"File::ReadRawFloat^0", API_FN_PAIR(File_ReadRawFloat)},
        {"File::ReadRawLine^1", API_FN_PAIR(File_ReadRawLine)},
        {"File::ReadRawLineBack^0", API_FN_PAIR(File_ReadRawLineBack)},
        {"File::ReadString^1", API_FN_PAIR(File_ReadString)},
        {"File::ReadStringBack^0", API_FN_PAIR(File_ReadStringBack)},
        {"File::WriteInt^1", API_FN_PAIR(File_WriteInt)},
        {"File::WriteFloat^1", API_FN_PAIR(File_WriteFloat)},
        {"File::WriteRawChar^1", API_FN_PAIR(File_WriteRawChar)},
        {"File::WriteRawFloat^1", API_FN_PAIR(File_WriteRawFloat)},
        {"File::WriteRawInt^1", API_FN_PAIR(File_WriteRawInt)},
        {"File::WriteRawLine^1", API_FN_PAIR(File_WriteRawLine)},
        {"File::WriteString^1", API_FN_PAIR(File_WriteString)},
        {"File::ReadRawBytes^3", API_FN_PAIR(File_ReadRawBytes)},
        {"File::WriteRawBytes^3", API_FN_PAIR(File_WriteRawBytes)},
        {"File::Seek^2", API_FN_PAIR(File_Seek)},
        {"File::get_EOF", API_FN_PAIR(File_GetEOF)},
        {"File::get_Error", API_FN_PAIR(File_GetError)},
        {"File::get_Position", API_FN_PAIR(File_GetPosition)},
        {"File::get_Path", API_FN_PAIR(File_GetPath)},
    };

    ccAddExternalFunctions(file_api);
}
