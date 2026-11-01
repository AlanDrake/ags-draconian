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
#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS
#include <stdio.h> // sprintf
#include "platform/windows/debug/namedpipesagsdebugger.h"
#include "util/stdio_compat.h"

void NamedPipesAGSDebugger::SendAcknowledgement()
{
    DWORD bytesWritten;
    WriteFile(_hPipeSending, "MSGACK", 6, &bytesWritten, nullptr);
}


NamedPipesAGSDebugger::NamedPipesAGSDebugger(const char *instanceToken)
{
    _hPipeSending = nullptr;
    _hPipeReading = nullptr;
    _instanceToken = instanceToken;
}

bool NamedPipesAGSDebugger::Initialize()
{
    // can't use a single duplex pipe as it was deadlocking
    char pipeNameBuffer[MAX_PATH_SZ];
    WCHAR wpipeNameBuf[MAX_PATH_SZ];
    snprintf(pipeNameBuffer, MAX_PATH_SZ, "\\\\.\\pipe\\AGSEditorDebuggerGameToEd%s", _instanceToken);
    MultiByteToWideChar(CP_UTF8, 0, pipeNameBuffer, -1, wpipeNameBuf, MAX_PATH_SZ);
    _hPipeSending = CreateFileW(wpipeNameBuf, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    snprintf(pipeNameBuffer, MAX_PATH_SZ, "\\\\.\\pipe\\AGSEditorDebuggerEdToGame%s", _instanceToken);
    MultiByteToWideChar(CP_UTF8, 0, pipeNameBuffer, -1, wpipeNameBuf, MAX_PATH_SZ);
    _hPipeReading = CreateFileW(wpipeNameBuf, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if ((_hPipeReading == INVALID_HANDLE_VALUE) || (_hPipeSending == INVALID_HANDLE_VALUE))
        return false;

    return true;
}

void NamedPipesAGSDebugger::Shutdown()
{
    if (_hPipeReading != nullptr)
    {
        CloseHandle(_hPipeReading);
        CloseHandle(_hPipeSending);
        _hPipeReading = nullptr;
    }
}

bool NamedPipesAGSDebugger::SendMessageToEditor(const char *message)
{
    DWORD bytesWritten;
    return (WriteFile(_hPipeSending, message, strlen(message), &bytesWritten, nullptr) != 0);
}

bool NamedPipesAGSDebugger::IsMessageAvailable()
{
    DWORD bytesAvailable = 0;
    PeekNamedPipe(_hPipeReading, nullptr, 0, nullptr, &bytesAvailable, nullptr);

    return (bytesAvailable > 0);
}

char *NamedPipesAGSDebugger::GetNextMessage()
{
    DWORD bytesAvailable = 0;
    PeekNamedPipe(_hPipeReading, nullptr, 0, nullptr, &bytesAvailable, nullptr);

    if (bytesAvailable > 0)
    {
        char *buffer = (char *)malloc(bytesAvailable + 1);
        DWORD bytesRead = 0;
        ReadFile(_hPipeReading, buffer, bytesAvailable, &bytesRead, nullptr);
        buffer[bytesRead] = 0;

        SendAcknowledgement();
        return buffer;
    }
    else
    {
        return nullptr;
    }
}

#endif // AGS_PLATFORM_OS_WINDOWS
