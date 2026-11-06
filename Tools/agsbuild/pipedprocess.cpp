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
#include "pipedprocess.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <string>
#if (AGS_PLATFORM_OS_WINDOWS)
#else
#include <errno.h>
#include <signal.h>

#include <fcntl.h>
#include <sys/wait.h>
#endif
#include "util/memorystream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGSBuild
{

String QuoteWindowsArg(const String &arg)
{
    const char *const text = arg.GetCStr();
    if (!arg.IsEmpty() && strpbrk(text, " \t\n\v\"") == nullptr)
    {
        return arg;
    }

    std::string quoted = "\"";
    for (const char *ch = text;; ++ch)
    {
        size_t backslashes = 0;
        for (; *ch == '\\'; ++ch)
        {
            ++backslashes;
        }
        if (*ch == 0)
        {
            // Before the closing quote every backslash is doubled, or the last one would escape it
            quoted.append(backslashes * 2, '\\');
            break;
        }
        // Doubled before a quote too, and one more to escape the quote; literal anywhere else
        quoted.append(*ch == '"' ? backslashes * 2 + 1 : backslashes, '\\');
        quoted += *ch;
    }
    quoted += '"';
    return quoted.c_str();
}

String QuotePosixShellArg(const String &arg)
{
    const char *const text = arg.GetCStr();
    const char *const plain_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-+=.,:/@%";
    if (!arg.IsEmpty() && text[strspn(text, plain_chars)] == 0)
    {
        return arg;
    }

    std::string quoted = "'";
    for (const char *ch = text; *ch; ++ch)
    {
        if (*ch == '\'')
        {
            quoted += "'\\''"; // there is no escape inside single quotes: close them, escape, reopen
        }
        else
        {
            quoted += *ch;
        }
    }
    quoted += '\'';
    return quoted.c_str();
}

String QuoteProcessArg(const String &arg)
{
#if (AGS_PLATFORM_OS_WINDOWS)
    return QuoteWindowsArg(arg);
#else
    return QuotePosixShellArg(arg); // the command is run by /bin/sh
#endif
}

ProcessArgs::ProcessArgs(std::initializer_list<String> args)
{
    for (const auto &arg : args)
    {
        Add(arg);
    }
}

ProcessArgs &ProcessArgs::Add(const String &arg)
{
    if (!_params.IsEmpty())
    {
        _params.AppendChar(' ');
    }
    _params.Append(QuoteProcessArg(arg));
    return *this;
}

ProcessArgs &ProcessArgs::Add(const ProcessArgs &args)
{
    if (!_params.IsEmpty() && !args._params.IsEmpty())
    {
        _params.AppendChar(' ');
    }
    _params.Append(args._params);
    return *this;
}

PipedProcess::PipedProcess(const String &exe_path, const String &params, TextStreamWriter *log_writer)
    : _exePath(exe_path),
      _params(params),
      _logWriter(log_writer)
{
    _command = String::FromFormat("%s %s", QuoteProcessArg(exe_path).GetCStr(), params.GetCStr());
#if (AGS_PLATFORM_OS_WINDOWS)
    memset(&_pi, 0, sizeof(_pi));
#endif
}

HError PipedProcess::RunSync(std::vector<uint8_t> *stdout_data)
{
    HError err = CreateProcess();
    if (!err)
        return err;

    if (IsPipeCreated())
    {
        err = RunProcessSync(stdout_data);
    }
    else
    {
        if (stdout_data)
            err = new Error("Failed to create pipe(s), won't be able to read out process output");
        else
            _logWriter->WriteString("Failed to create pipe(s), won't be able to read out process output\n");
        WaitForProcessSync();
    }

    CloseProcess();
    return err;
}

#if (AGS_PLATFORM_OS_WINDOWS)
// Processes are started from several task threads at once. Pipe creation, process creation and
// closing our copies of the pipe's write ends happen under one lock: no other child can then
// get a copy of a pipe's write end, and a pipe reports end-of-file as soon as its process exits.
static std::mutex CreateProcessMutex;

// The handles a child process inherits: only those in the list, not every inheritable handle
// agsbuild has (such as its own standard handles, which are the editor's pipes when the editor
// runs it: a tool that outlived agsbuild would keep them open)
class InheritedHandles
{
public:
    explicit InheritedHandles(const std::vector<HANDLE> &handles) : _handles(handles)
    {
        if (_handles.empty())
        {
            return;
        }
        SIZE_T size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
        _buffer.resize(size);
        auto *list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(_buffer.data());
        if (size == 0 || !InitializeProcThreadAttributeList(list, 1, 0, &size))
        {
            return;
        }
        _list = list;
        if (!UpdateProcThreadAttribute(_list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                       static_cast<void *>(_handles.data()), _handles.size() * sizeof(HANDLE), nullptr,
                                       nullptr))
        {
            DeleteProcThreadAttributeList(_list);
            _list = nullptr;
        }
    }
    ~InheritedHandles()
    {
        if (_list)
        {
            DeleteProcThreadAttributeList(_list);
        }
    }
    InheritedHandles(const InheritedHandles &) = delete;
    InheritedHandles &operator=(const InheritedHandles &) = delete;

    // The attribute list for STARTUPINFOEX, or nullptr if there are no handles, or the list could
    // not be made (then the child inherits every inheritable handle, as it would without a list)
    LPPROC_THREAD_ATTRIBUTE_LIST GetList() const { return _list; }
    bool HasHandles() const { return !_handles.empty(); }

private:
    std::vector<HANDLE> _handles;
    std::vector<uint8_t> _buffer;
    LPPROC_THREAD_ATTRIBUTE_LIST _list = nullptr;
};
#endif

HError PipedProcess::CreateProcess()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    std::lock_guard<std::mutex> lk(CreateProcessMutex);
    STARTUPINFOEXW six = {};
    STARTUPINFOW &si = six.StartupInfo;
    si.cb = sizeof(six);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    const bool stdout_pipe = CreatePipe(&_hStdOutPipeRead, &_hStdOutPipeWrite, &sa, 1024u) == TRUE;
    const bool stderr_pipe = CreatePipe(&_hStdErrPipeRead, &_hStdErrPipeWrite, &sa, 1024u) == TRUE;
    // Only the write ends are for the child; our read ends must not be inherited by anyone
    if (stdout_pipe)
        SetHandleInformation(_hStdOutPipeRead, HANDLE_FLAG_INHERIT, 0);
    if (stderr_pipe)
        SetHandleInformation(_hStdErrPipeRead, HANDLE_FLAG_INHERIT, 0);
    // NOTE: we only check the pipe result to know if we should dispose of it.
    // if pipe failed, we still can run the process, although we won't see it's output
    if (stdout_pipe || stderr_pipe)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = INVALID_HANDLE_VALUE;
        si.hStdOutput = _hStdOutPipeWrite;
        si.hStdError = _hStdErrPipeWrite;
    }

    std::vector<HANDLE> inherit;
    if (stdout_pipe)
    {
        inherit.push_back(_hStdOutPipeWrite);
    }
    if (stderr_pipe)
    {
        inherit.push_back(_hStdErrPipeWrite);
    }
    const InheritedHandles inherited(inherit);
    six.lpAttributeList = inherited.GetList();
    // The extended structure only with its flag, as CreateProcess documents it
    si.cb = six.lpAttributeList ? sizeof(six) : sizeof(si);
    const DWORD flags = CREATE_NO_WINDOW | (six.lpAttributeList ? EXTENDED_STARTUPINFO_PRESENT : 0);

    std::vector<wchar_t> cmd(_command.GetLength() + 1);
    StrUtil::ConvertUtf8ToWstr(_command.GetCStr(), cmd.data(), cmd.size());
    const bool created = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, inherited.HasHandles() ? TRUE : FALSE,
                                        flags, nullptr, nullptr, &si, &_pi) == TRUE;
    const DWORD create_error = created ? 0 : GetLastError();
    // The child has its own copies of the write ends now. Keeping ours open would keep the pipes
    // from ever reporting end-of-file, and would leak them into the next process we start.
    if (_hStdOutPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdOutPipeWrite);
    if (_hStdErrPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdErrPipeWrite);
    _hStdOutPipeWrite = INVALID_HANDLE_VALUE;
    _hStdErrPipeWrite = INVALID_HANDLE_VALUE;
    if (!created)
        return new Error("Process failed with err code: %u\n", create_error);
#else
    const bool stdout_pipe = pipe(_stdOutPipe) >= 0;
    const bool stderr_pipe = pipe(_stdErrPipe) >= 0;

    _cpid = fork();
    if (_cpid < 0)
    {
        return new Error("Process fork failed with err code: %d\n", errno);
    }

    if (_cpid == 0)
    {
        // Setup child process to write into the pipe
        // Own process group, so Terminate kills whatever the shell started too, and the terminal's
        // Ctrl+C reaches only agsbuild, which then stops its children itself
        setpgid(0, 0);
        // Route write pipe's end to stdout and release the old handle
        dup2(_stdOutPipe[1], STDOUT_FILENO);
        // Route write pipe's end to stderr and release the old handle
        dup2(_stdErrPipe[1], STDERR_FILENO);
        // Close our pipe instances
        // TODO: smart pointer for the pipe handle
        close(_stdOutPipe[0]);
        close(_stdOutPipe[1]);
        close(_stdErrPipe[0]);
        close(_stdErrPipe[1]);
        _stdOutPipe[0] = _stdOutPipe[1] = 0;
        _stdErrPipe[0] = _stdErrPipe[1] = 0;
        // TODO: in order to call the program directly, instead of using /bin/sh method,
        // we need a params parser, which splits params string into separate elements,
        // but it must properly handle args in doublequotes!
        execl("/bin/sh", "/bin/sh", "-c", _command.GetCStr(), nullptr);
        _exit(EXIT_FAILURE); // exec* does not return on success, if we got here then it failed
    }
    else
    {
        // Setup parent process to read from the pipe
        setpgid(_cpid, _cpid); // as the child does, whichever runs first
        // Close unused write end
        close(_stdOutPipe[1]);
        close(_stdErrPipe[1]);
        _stdOutPipe[1] = 0;
        _stdErrPipe[1] = 0;
    }
#endif
    _started = true;
    if (_terminateRequested)
        KillProcess();
    return HError::None();
}

void PipedProcess::Terminate()
{
    _terminateRequested = true;
    if (_started)
        KillProcess();
}

void PipedProcess::KillProcess()
{
    std::lock_guard<std::mutex> lk(_killMutex);
    if (_closed)
        return;
#if (AGS_PLATFORM_OS_WINDOWS)
    if (_pi.hProcess != nullptr)
        TerminateProcess(_pi.hProcess, 1);
#else
    if (_cpid > 0)
        kill(-_cpid, SIGKILL); // the whole process group
#endif
}

#if (AGS_PLATFORM_OS_WINDOWS)
bool PipedProcess::ReadFromPipe(HANDLE pipe, std::vector<uint8_t> *output_data, bool print_to_buffer)
{
    uint8_t buffer[1024];
    DWORD pipe_has_bytes = 0;
    bool any_read = false;
    if ((PeekNamedPipe(pipe, nullptr, 0, nullptr, &pipe_has_bytes, nullptr) == TRUE) && (pipe_has_bytes > 0))
    {
        // Never ask ReadFile for more than the pipe reported: a blocking ReadFile on an empty pipe
        // waits until more data arrives, which never happens once the process has exited.
        DWORD bytes_read = 0;
        while (pipe_has_bytes > 0 &&
               ReadFile(pipe, buffer, std::min<DWORD>(pipe_has_bytes, sizeof(buffer) - 1), &bytes_read, nullptr) ==
                   TRUE &&
               bytes_read > 0 && bytes_read <= pipe_has_bytes)
        {
            if (output_data)
            {
                std::copy(buffer, buffer + bytes_read, std::back_inserter(*output_data));
            }
            if (print_to_buffer)
            {
                buffer[bytes_read] = 0;
                _logWriter->WriteString(reinterpret_cast<const char *>(buffer));
            }
            pipe_has_bytes -= bytes_read;
            any_read = true;
        }
    }
    return any_read;
}
#else
void PipedProcess::ReadFromPipe(int pipe_fd, std::vector<uint8_t> *output_data, bool print_to_buffer)
{
    uint8_t buffer[1024];
    ssize_t bytes_read = 0;
    do
    {
        bytes_read = read(pipe_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            if (output_data)
            {
                std::copy(buffer, buffer + bytes_read, std::back_inserter(*output_data));
            }
            if (print_to_buffer)
            {
                buffer[bytes_read] = 0;
                _logWriter->WriteString(reinterpret_cast<const char *>(buffer));
            }
        }
    } while ((bytes_read > 0) || (bytes_read == EAGAIN || bytes_read == EWOULDBLOCK));
}
#endif

HError PipedProcess::RunProcessSync(std::vector<uint8_t> *stdout_data)
{
#if (AGS_PLATFORM_OS_WINDOWS)
    if (_pi.hProcess == nullptr)
        return new Error("Internal error: external process was not initialized");

    DWORD proc_result = 0;
    do
    {
        proc_result = WaitForSingleObject(_pi.hProcess, 1);
        if (_hStdOutPipeRead)
            ReadFromPipe(_hStdOutPipeRead, stdout_data, true);
        if (_hStdErrPipeRead)
            ReadFromPipe(_hStdErrPipeRead, nullptr, true);
    } while (proc_result == WAIT_TIMEOUT);
    // Collect whatever the process wrote just before it exited
    bool any_left = true;
    while (any_left)
    {
        any_left = false;
        if (_hStdOutPipeRead != INVALID_HANDLE_VALUE)
            any_left |= ReadFromPipe(_hStdOutPipeRead, stdout_data, true);
        if (_hStdErrPipeRead != INVALID_HANDLE_VALUE)
            any_left |= ReadFromPipe(_hStdErrPipeRead, nullptr, true);
    }
#else
    if (_cpid == 0)
        return new Error("Internal error: external process was not initialized");

    pid_t wait_pid = 0;
    int wstatus = 0;
    do
    {
        wait_pid = waitpid(_cpid, &wstatus, WNOHANG);
        //printf("waitpid -- wstatus = %d\n", wstatus);
        if (_stdOutPipe[0] != 0)
            ReadFromPipe(_stdOutPipe[0], stdout_data, true);
        if (_stdErrPipe[0] != 0)
            ReadFromPipe(_stdErrPipe[0], nullptr, true);
    } while ((wait_pid == 0) || ((wait_pid == _cpid) && !WIFEXITED(wstatus) && !WIFSIGNALED(wstatus)));

    if (WIFEXITED(wstatus))
        _exitCode = static_cast<int8_t>(WEXITSTATUS(wstatus));
    else if (WIFSIGNALED(wstatus))
        _exitCode = WTERMSIG(wstatus); // FIXME: separate code variable
#endif
    return HError::None();
}

void PipedProcess::WaitForProcessSync()
{
#if (AGS_PLATFORM_OS_WINDOWS)
    WaitForSingleObject(_pi.hProcess, INFINITE);
#else

    int wstatus = 0;
    waitpid(_cpid, &wstatus, 0);
    if (WIFEXITED(wstatus))
        _exitCode = static_cast<int8_t>(WEXITSTATUS(wstatus));
    else if (WIFSIGNALED(wstatus))
        _exitCode = WTERMSIG(wstatus); // FIXME: separate code variable
#endif
}

void PipedProcess::CloseProcess()
{
    {
        std::lock_guard<std::mutex> lk(_killMutex);
        _closed = true;
    }
#if (AGS_PLATFORM_OS_WINDOWS)
    // TODO: smart pointer for the pipe handle
    if (_hStdOutPipeRead != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdOutPipeRead);
    if (_hStdOutPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdOutPipeWrite);
    if (_hStdErrPipeRead != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdErrPipeRead);
    if (_hStdErrPipeWrite != INVALID_HANDLE_VALUE)
        CloseHandle(_hStdErrPipeWrite);

    _hStdOutPipeRead = INVALID_HANDLE_VALUE;
    _hStdOutPipeWrite = INVALID_HANDLE_VALUE;
    _hStdErrPipeRead = INVALID_HANDLE_VALUE;
    _hStdErrPipeWrite = INVALID_HANDLE_VALUE;

    DWORD proc_code;
    GetExitCodeProcess(_pi.hProcess, &proc_code);
    CloseHandle(_pi.hThread);
    CloseHandle(_pi.hProcess);
    _exitCode = static_cast<int>(proc_code);
#else
    if (_cpid == 0)
        return;

    // TODO: smart pointer for the pipe handle
    if (_stdOutPipe[0])
        close(_stdOutPipe[0]);
    if (_stdOutPipe[1])
        close(_stdOutPipe[1]);
    if (_stdErrPipe[0])
        close(_stdErrPipe[0]);
    if (_stdErrPipe[1])
        close(_stdErrPipe[1]);
    _stdOutPipe[0] = _stdOutPipe[1] = 0;
    _stdErrPipe[0] = _stdErrPipe[1] = 0;
#endif
    _logWriter->WriteFormat("\tExit code: %d\n\n", _exitCode);
}

bool PipedProcess::IsPipeCreated() const
{
#if (AGS_PLATFORM_OS_WINDOWS)
    // Our write ends are closed right after the process starts, so only the read end tells
    return _hStdOutPipeRead != nullptr && _hStdOutPipeRead != INVALID_HANDLE_VALUE;
#else
    return _stdOutPipe[0] >= 0;
#endif
}

int PipedProcess::GetExitCode() const
{
    return static_cast<int>(_exitCode);
}

} // namespace AGSBuild
