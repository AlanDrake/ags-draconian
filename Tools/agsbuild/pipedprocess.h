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
#if (AGS_PLATFORM_OS_WINDOWS)
#include "platform/windows/windows.h"
#else
#include <unistd.h>
#endif
#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <mutex>
#include <vector>

#include "util/error.h"
#include "util/string.h"
#include "util/textstreamwriter.h"

namespace AGSBuild
{

using String = AGS::Common::String;
using HError = AGS::Common::HError;

// Writes an argument the way a command line needs it, so that the program receives it as it is
// given here, whatever characters it has: in quotes and escaped where necessary.
// For Windows, by the rules that the C runtime and CommandLineToArgvW share: a quote is escaped
// with a backslash, and backslashes are doubled only where a quote follows them.
String QuoteWindowsArg(const String &arg);
// For a POSIX shell: in single quotes, where only the single quote itself needs care.
String QuotePosixShellArg(const String &arg);
// For the command line that PipedProcess runs on this platform.
String QuoteProcessArg(const String &arg);

// The arguments of a command line for PipedProcess. Each one reaches the program as it is given
// here: a path with spaces or a text with quotes is still one argument.
class ProcessArgs
{
public:
    ProcessArgs() = default;
    ProcessArgs(std::initializer_list<String> args);

    // Adds one argument
    ProcessArgs &Add(const String &arg);
    // Adds all arguments of another list
    ProcessArgs &Add(const ProcessArgs &args);
    // The text of the command line after the program
    const String &GetParams() const { return _params; }

private:
    String _params;
};

class PipedProcess
{
public:
    // exe_path is the program's path or name as it is, params is the text of the command line
    // after the program (see ProcessArgs)
    PipedProcess(const String &exe_path, const String &params, AGS::Common::TextStreamWriter *log_writer = nullptr);

    // The command line that is run
    const String &GetCommand() const { return _command; }

    HError RunSync(std::vector<uint8_t> *stdout_data = nullptr);

    int GetExitCode() const;

    // Kills the process; safe to call from another thread, at any time, also before the process was
    // started (then it is killed as soon as it exists). RunSync returns after that as usual.
    void Terminate();
    bool WasTerminated() const { return _terminateRequested; }

private:
    void KillProcess();
    HError CreateProcess();
    HError RunProcessSync(std::vector<uint8_t> *stdout_data = nullptr);
    void WaitForProcessSync();
    void CloseProcess();

    bool IsPipeCreated() const;

#if (AGS_PLATFORM_OS_WINDOWS)
    // Reads what the pipe has right now without blocking; returns whether anything was read
    bool ReadFromPipe(HANDLE pipe, std::vector<uint8_t> *output_data, bool print_to_buffer);
#else
    void ReadFromPipe(int pipe_fd, std::vector<uint8_t> *output_data, bool print_to_buffer);
#endif

    String _exePath;
    String _params;
    String _command;
    AGS::Common::TextStreamWriter *_logWriter = nullptr;
    int _exitCode = -1;
    std::atomic<bool> _terminateRequested{false};
    std::atomic<bool> _started{false};
    std::mutex _killMutex; // protects the process handle against being closed while it is killed
    bool _closed = false;
#if (AGS_PLATFORM_OS_WINDOWS)
    HANDLE _hStdOutPipeRead = INVALID_HANDLE_VALUE;
    HANDLE _hStdOutPipeWrite = INVALID_HANDLE_VALUE;
    HANDLE _hStdErrPipeRead = INVALID_HANDLE_VALUE;
    HANDLE _hStdErrPipeWrite = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION _pi;
#else
    typedef int PipeFD[2];
    PipeFD _stdOutPipe = {0, 0};
    PipeFD _stdErrPipe = {0, 0};
    pid_t _cpid = -1;
#endif
};

} // namespace AGSBuild
