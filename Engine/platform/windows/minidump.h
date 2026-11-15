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
// The crash dump written by the Release builds' exception handler on Windows.
//
//=============================================================================
#pragma once

#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS && !AGS_PLATFORM_DEBUG
// EXCEPTION_RECORD and EXCEPTION_POINTERS of <windows.h>, named by their struct tags so that this header does not
// include it (minidump.cpp includes it with UNICODE defined, win_ex_handling.cpp through the engine's wrapper).
struct _EXCEPTION_RECORD;
struct _EXCEPTION_POINTERS;

// The exception that CustomExceptionHandler caught last.
extern struct _EXCEPTION_RECORD excinfo;
// The result of CreateMiniDump: 0 when the dump was written, else the step that failed (1 dbghelp.dll not loaded,
// 2 MiniDumpWriteDump not found, 3 the file not created, 4 the dump not written).
extern int miniDumpResultCode;

// Writes CrashInfo.<engine version>.dmp into the current directory; pep may be nullptr.
void CreateMiniDump(struct _EXCEPTION_POINTERS *pep);
// The filter of the engine's __except block: records the exception and the CPU state, writes the dump, and
// tells the system to run the handler.
int CustomExceptionHandler(struct _EXCEPTION_POINTERS *exinfo);
#endif
