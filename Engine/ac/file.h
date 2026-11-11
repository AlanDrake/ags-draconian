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

// Script paths given to the functions below may start with a location token (see ResolveScriptPath).

// Tells whether the file is found, in the file system or in the game assets (1 or 0).
int File_Exists(const char *filename);
// Deletes the file; returns 1 on success, 0 if the path is not allowed for writing, no file is found or
// the deletion failed.
int File_Delete(const char *filename);
// Opens the file; returns a handle for the script (> 0), or 0 on failure.
int32_t FileOpen(const char *filename, AGS::Common::FileOpenMode open_mode, AGS::Common::StreamMode work_mode);
// Closes the file by the handle; quits the script on an invalid handle.
void FileClose(int32_t handle);
// Creates a managed sc_File and opens the file in the mode scFileRead, scFileWrite or scFileAppend;
// quits the script on another mode, returns null if the file cannot be opened.
void *sc_OpenFile(const char *filename, int mode);
// Returns the absolute path that the script path points to as a script string; the file need not exist.
const char *File_ResolvePath(const char *filename);

// The File.* script API, one function per member of the sc_File object. The write and read functions quit the
// script if the file is closed (handle 0); File_GetEOF and File_GetError then return 1, File_GetPosition -1.
// File_Close closes the file, which can be done again with no effect.
void File_Close(sc_File *file);
// Writes a string with its length and null terminator; File_ReadString and File_ReadStringBack read it back.
void File_WriteString(sc_File *file, const char *towrite);
// Writes an int, marked as such: File_ReadInt only accepts what this wrote.
void File_WriteInt(sc_File *file, int towrite);
// Writes one byte; warns if the value is not in 0-255.
void File_WriteRawChar(sc_File *file, int towrite);
// Writes 4 bytes with no marker: File_ReadRawInt reads them.
void File_WriteRawInt(sc_File *file, int towrite);
// Writes the string and a CR LF line break.
void File_WriteRawLine(sc_File *file, const char *towrite);
// Reads a line (ended by LF, CR or CR LF) into the buffer, which is MAX_MAXSTRLEN long; a longer line is cut.
void File_ReadRawLine(sc_File *file, char *buffer);
// Reads a whole line of any length and returns it as a script string.
const char *File_ReadRawLineBack(sc_File *file);
// Reads a string written by File_WriteString into the buffer: only one shorter than 200 bytes with its
// terminator (otherwise warns and reads nothing); an empty string at the end of the file.
void File_ReadString(sc_File *file, char *toread);
// Reads a string written by File_WriteString; an empty string at the end of the file or if there is none.
const char *File_ReadStringBack(sc_File *file);
// Reads an int written by File_WriteInt; -1 at the end of the file or if it was not written so.
int File_ReadInt(sc_File *file);
// Reads one byte; -1 at the end of the file (so is the byte 255, see the .cpp).
int File_ReadRawChar(sc_File *file);
// Reads 4 bytes as an int; -1 at the end of the file.
int File_ReadRawInt(sc_File *file);
// Moves the read/write position; returns the new position.
int File_Seek(sc_File *file, int offset, int origin);
// 1 at the end of the file or after an error.
int File_GetEOF(sc_File *file);
// 1 after a stream error.
int File_GetError(sc_File *file);
// The current position in the file.
int File_GetPosition(sc_File *file);
// Fills a list of filenames found using given pattern; sorts the resulting list.
// The filenames *do not* contain any parent paths, strictly names of files.
// While this function supports parent path in pattern, but it does not support
// wildcards in parent path, only wildcards in the file name.
void FillDirList(std::vector<AGS::Common::String> &files, const AGS::Common::String &pattern,
                 ScriptFileSortStyle file_sort, ScriptSortDirection sort_dir);

//=============================================================================

// Managed file streams: for script and plugin use. A stream is known by its handle (> 0), and the
// operation_name of the functions below names the caller in the error message.
// Registers the stream and returns its new handle (operation_name is not used).
int32_t add_file_stream(std::unique_ptr<AGS::Common::Stream> &&stream, const char *operation_name);
// Destroys the stream; quits the script on an invalid handle.
void close_file_stream(int32_t fhandle, const char *operation_name);
// Returns the stream; quits the script on an invalid handle (and returns null if it comes back).
AGS::Common::Stream *get_file_stream(int32_t fhandle, const char *operation_name);
// The same, but returns the stream's underlying IStreamBase.
AGS::Common::IStreamBase *get_file_stream_iface(int32_t fhandle, const char *operation_name);
