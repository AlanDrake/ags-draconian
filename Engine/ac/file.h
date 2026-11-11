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
// Script File API implementation.
//
//=============================================================================
#pragma once

#include <memory>
#include <vector>

#include "ac/dynobj/scriptfile.h"
#include "ac/runtime_defines.h"
#include "util/stream.h"
#include "util/string.h"

int File_Exists(const char *filename);
int File_Delete(const char *filename);
int32_t FileOpen(const char *filename, AGS::Common::FileOpenMode open_mode, AGS::Common::StreamMode work_mode);
void FileClose(int32_t handle);
void *sc_OpenFile(const char *filename, int mode);
const char *File_ResolvePath(const char *filename);
void File_Close(sc_File *file);
void File_WriteString(sc_File *file, const char *towrite);
void File_WriteInt(sc_File *file, int towrite);
void File_WriteRawChar(sc_File *file, int towrite);
void File_WriteRawInt(sc_File *file, int towrite);
void File_WriteRawLine(sc_File *file, const char *towrite);
void File_ReadRawLine(sc_File *file, char *buffer);
const char *File_ReadRawLineBack(sc_File *file);
void File_ReadString(sc_File *file, char *toread);
const char *File_ReadStringBack(sc_File *file);
int File_ReadInt(sc_File *file);
int File_ReadRawChar(sc_File *file);
int File_ReadRawInt(sc_File *file);
int File_Seek(sc_File *file, int offset, int origin);
int File_GetEOF(sc_File *file);
int File_GetError(sc_File *file);
int File_GetPosition(sc_File *file);
// Fills a list of filenames found using given pattern; sorts the resulting list.
// The filenames *do not* contain any parent paths, strictly names of files.
// While this function supports parent path in pattern, but it does not support
// wildcards in parent path, only wildcards in the file name.
void FillDirList(std::vector<AGS::Common::String> &files, const AGS::Common::String &pattern,
                 ScriptFileSortStyle file_sort, ScriptSortDirection sort_dir);

//=============================================================================

// Managed file streams: for script and plugin use
int32_t add_file_stream(std::unique_ptr<AGS::Common::Stream> &&stream, const char *operation_name);
void close_file_stream(int32_t fhandle, const char *operation_name);
AGS::Common::Stream *get_file_stream(int32_t fhandle, const char *operation_name);
AGS::Common::IStreamBase *get_file_stream_iface(int32_t fhandle, const char *operation_name);
