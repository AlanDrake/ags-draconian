using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// Runs agsbuild, or another tool that writes the same JSON lines (Tools/agsbuild/diagnostics.h),
    /// as a hidden process, and turns what it reports into a <see cref="BuildToolResult"/>.
    ///
    /// A failed build, a missing tool or one that crashes is a result, not an exception; Run throws
    /// only for wrong arguments, and passes on what the progress callback throws (after killing the
    /// tool). A token cancelled before the start gives Cancelled without starting the tool.
    ///
    /// Run blocks until the tool has exited. The progress callback is called on the thread that
    /// called Run, one call at a time, in the order of the tool's reports. While it runs, the
    /// tool's output waits in a queue (nothing is lost, and the tool is not held up), so a UI can
    /// hand the values to its own thread and return; it should not wait for its thread there.
    ///
    /// Cancelling writes the line "cancel" to the tool's standard input, after which agsbuild stops
    /// its tools and reports the end. A tool that has not exited after the grace period is killed
    /// with every process it started: they are all in a Windows job object, or, where Windows
    /// refuses one (Windows 7 when the editor is in a job already), killed with taskkill /T.
    /// The input is closed when nothing is left to cancel: when all the tasks are done, or at the
    /// end. After its end report the tool has a second (at most the grace period) to exit, and is
    /// killed after that; whatever it leaves running when it exits is killed too.
    ///
    /// A runner has no state of its own: one instance serves any number of runs, also at the same
    /// time on several threads.
    /// </summary>
    public sealed class BuildToolRunner
    {
        /// <summary>The executable that <see cref="Run(BuildToolOptions, Action{BuildToolProgress}, CancellationToken)"/> runs.</summary>
        public const string ToolFileName = "agsbuild.exe";

        /// <summary>How long a cancelled tool has to exit on its own, unless the constructor says otherwise.</summary>
        public static readonly TimeSpan DefaultCancelGracePeriod = TimeSpan.FromSeconds(5);

        private readonly TimeSpan _cancelGracePeriod;

        public BuildToolRunner()
            : this(DefaultCancelGracePeriod)
        {
        }

        /// <param name="cancelGracePeriod">How long a cancelled tool has to exit before it is killed.</param>
        public BuildToolRunner(TimeSpan cancelGracePeriod)
        {
            if (cancelGracePeriod < TimeSpan.Zero)
            {
                throw new ArgumentOutOfRangeException(nameof(cancelGracePeriod), "The grace period cannot be negative.");
            }
            _cancelGracePeriod = cancelGracePeriod;
        }

        /// <summary>How long a cancelled tool has to exit on its own before it is killed.</summary>
        public TimeSpan CancelGracePeriod
        {
            get { return _cancelGracePeriod; }
        }

        /// <summary>
        /// Builds a game with agsbuild.exe from <see cref="BuildToolOptions.ToolsDirectory"/>, or from
        /// the editor's folder if that is null, in the project's folder.
        /// </summary>
        /// <param name="progress">Called on this thread when the tool reports progress; may be null.</param>
        /// <param name="cancel">Cancels the build: see the class.</param>
        /// <exception cref="ArgumentNullException">options is null.</exception>
        /// <exception cref="InvalidOperationException">options misses a required value.</exception>
        public BuildToolResult Run(BuildToolOptions options, Action<BuildToolProgress> progress, CancellationToken cancel)
        {
            if (options == null)
            {
                throw new ArgumentNullException(nameof(options));
            }
            string arguments = options.ToArguments();
            string toolsDirectory = string.IsNullOrEmpty(options.ToolsDirectory)
                ? AppDomain.CurrentDomain.BaseDirectory
                : options.ToolsDirectory;
            string toolPath = Path.Combine(Path.GetFullPath(toolsDirectory), ToolFileName);
            string projectDirectory = Path.GetDirectoryName(Path.GetFullPath(options.ProjectFile));
            return RunTool(toolPath, arguments, Directory.Exists(projectDirectory) ? projectDirectory : null, progress,
                cancel);
        }

        /// <summary>
        /// Runs a tool that writes the lines of "agsbuild --json", in the current directory.
        /// </summary>
        /// <param name="toolPath">The executable (or batch file); it is not looked for in PATH.</param>
        /// <param name="arguments">The command line after the program's name, quoted already
        /// (<see cref="BuildToolOptions.QuoteArgument"/>); may be null.</param>
        /// <param name="progress">Called on this thread when the tool reports progress; may be null.</param>
        /// <param name="cancel">Cancels the build: see the class.</param>
        /// <exception cref="ArgumentException">toolPath is null or empty.</exception>
        public BuildToolResult Run(string toolPath, string arguments, Action<BuildToolProgress> progress,
            CancellationToken cancel)
        {
            return RunTool(toolPath, arguments, null, progress, cancel);
        }

        private BuildToolResult RunTool(string toolPath, string arguments, string workingDirectory,
            Action<BuildToolProgress> progress, CancellationToken cancel)
        {
            if (string.IsNullOrEmpty(toolPath))
            {
                throw new ArgumentException("The tool's path is empty.", nameof(toolPath));
            }
            if (cancel.IsCancellationRequested)
            {
                return new BuildToolResult(BuildToolOutcome.Cancelled, null, null, null, 0, 0);
            }
            if (!File.Exists(toolPath))
            {
                return ToolRun.NotStarted("Cannot find the build tool " + toolPath + ".");
            }

            using (var run = new ToolRun(toolPath, arguments, workingDirectory, progress, cancel, _cancelGracePeriod))
            {
                return run.Execute();
            }
        }

        /// <summary>
        /// One run of a tool: the process and everything that is learnt from it.
        /// </summary>
        private sealed class ToolRun : IDisposable
        {
            // How long the queue is waited on when nothing else is due
            private const int POLL_MS = 100;
            // How long the output may stay open after the tool has exited, and how long it is waited for
            private const int OUTPUT_TIMEOUT_MS = 5000;
            // How long a tool may take to exit after its end report (at most the grace period)
            private const int EXIT_AFTER_END_MS = 1000;
            private const int TASKKILL_TIMEOUT_MS = 10000;
            private const uint KILLED_EXIT_CODE = 1;
            // The lines of output quoted in the runner's own error messages
            private const int QUOTED_LINES = 10;

            private readonly string _toolPath;
            private readonly string _arguments;
            private readonly string _workingDirectory;
            private readonly Action<BuildToolProgress> _progress;
            private readonly CancellationToken _cancel;
            private readonly TimeSpan _gracePeriod;
            private readonly Stopwatch _clock = new Stopwatch();
            private readonly BlockingCollection<string> _stdoutLines = new BlockingCollection<string>();
            private readonly StringBuilder _log = new StringBuilder();
            private readonly Queue<string> _plainStdout = new Queue<string>();
            private readonly CompileMessages _messages = new CompileMessages();
            private readonly HashSet<Tuple<string, int, string>> _warnings = new HashSet<Tuple<string, int, string>>();

            private Process _process;
            private ProcessJob _job;
            private StreamReader _stdout;
            private StreamReader _stderr;
            private Thread _stdoutReader;
            private Thread _stderrReader;
            private volatile string _pipeFailure;
            private BuildToolEvent _end;
            private int _done;
            private int _total;
            private string _task;
            private bool _cancelSent;
            private bool _inputClosed;
            private TimeSpan? _killAt;
            private bool _killed;
            private bool _exited;
            private TimeSpan _outputDeadline;

            public ToolRun(string toolPath, string arguments, string workingDirectory, Action<BuildToolProgress> progress,
                CancellationToken cancel, TimeSpan gracePeriod)
            {
                _toolPath = toolPath;
                _arguments = arguments ?? string.Empty;
                _workingDirectory = workingDirectory;
                _progress = progress;
                _cancel = cancel;
                _gracePeriod = gracePeriod;
            }

            private string ToolName
            {
                get { return Path.GetFileName(_toolPath); }
            }

            public static BuildToolResult NotStarted(string message)
            {
                return new BuildToolResult(BuildToolOutcome.ToolNotFound, null, new CompileMessages(new CompileError(message)),
                    null, 0, 0);
            }

            public BuildToolResult Execute()
            {
                string startFailure = Start();
                if (startFailure != null)
                {
                    return NotStarted(startFailure);
                }
                ReadOutput();
                return Finish();
            }

            public void Dispose()
            {
                if (_process == null)
                {
                    _stdoutLines.Dispose();
                    return;
                }
                if (!_process.HasExited)
                {
                    // The progress callback threw: the tool must not outlive the run
                    KillTree();
                }
                if (_job != null)
                {
                    _job.Dispose(); // kills whatever is still in the job
                }
                // Process.Dispose leaves the redirected streams open. The reader threads end when the output
                // closes, which it has unless a process out of reach keeps it open: then they keep their
                // streams, and leave them to the garbage collector.
                CloseInput();
                if (ReaderDone(_stdoutReader) && ReaderDone(_stderrReader))
                {
                    _stdout.Dispose();
                    _stderr.Dispose();
                    _stdoutLines.Dispose();
                }
                _process.Dispose();
            }

            private static bool ReaderDone(Thread reader)
            {
                return reader == null || reader.Join(OUTPUT_TIMEOUT_MS);
            }

            // Starts the process; returns why it could not, or null
            private string Start()
            {
                var startInfo = new ProcessStartInfo(_toolPath, _arguments)
                {
                    UseShellExecute = false,
                    CreateNoWindow = true,
                    WindowStyle = ProcessWindowStyle.Hidden,
                    RedirectStandardInput = true,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    StandardOutputEncoding = new UTF8Encoding(false),
                    StandardErrorEncoding = new UTF8Encoding(false)
                };
                if (!string.IsNullOrEmpty(_workingDirectory))
                {
                    startInfo.WorkingDirectory = _workingDirectory;
                }

                // The job is ready before the start, so that the process joins it at once: a process it started
                // before joining would not be in the job
                _job = ProcessJob.Create();
                var process = new Process { StartInfo = startInfo };
                try
                {
                    process.Start();
                }
                catch (Win32Exception ex)
                {
                    process.Dispose();
                    return "Cannot start the build tool " + _toolPath + ": " + ex.Message;
                }
                _process = process;
                _clock.Start();
                if (_job != null && !_job.Add(process))
                {
                    _job.Dispose();
                    _job = null;
                }

                _stdout = process.StandardOutput;
                _stderr = process.StandardError;
                _stdoutReader = StartThread(() => ReadLines(_stdout), ToolName + " stdout");
                _stderrReader = StartThread(() => ReadAll(_stderr), ToolName + " stderr");
                return null;
            }

            private static Thread StartThread(ThreadStart work, string name)
            {
                var thread = new Thread(work) { IsBackground = true, Name = name };
                thread.Start();
                return thread;
            }

            // On a reader thread: the lines of the standard output, into the queue
            private void ReadLines(StreamReader reader)
            {
                try
                {
                    string line;
                    while ((line = reader.ReadLine()) != null)
                    {
                        _stdoutLines.Add(line);
                    }
                }
                catch (IOException ex)
                {
                    _pipeFailure = ex.Message;
                }
                finally
                {
                    _stdoutLines.CompleteAdding();
                }
            }

            // On a reader thread: all of the standard error, into the log
            private void ReadAll(StreamReader reader)
            {
                var buffer = new char[8192];
                try
                {
                    int count;
                    while ((count = reader.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        lock (_log)
                        {
                            _log.Append(buffer, 0, count);
                        }
                    }
                }
                catch (IOException ex)
                {
                    _pipeFailure = ex.Message;
                }
            }

            // Handles the output, cancelling and killing until the tool has exited and its output is read
            private void ReadOutput()
            {
                while (true)
                {
                    TimeSpan now = _clock.Elapsed;
                    if (!_cancelSent && _cancel.IsCancellationRequested)
                    {
                        SendCancel(now);
                    }
                    if (KillPending() && now >= _killAt.Value)
                    {
                        KillTree();
                        _killed = true;
                    }
                    if (!_exited && _process.HasExited)
                    {
                        // Whatever the tool left running is not needed, and could keep the output open
                        _exited = true;
                        if (_job != null)
                        {
                            _job.Terminate(KILLED_EXIT_CODE);
                        }
                        _outputDeadline = now + TimeSpan.FromMilliseconds(OUTPUT_TIMEOUT_MS);
                    }

                    string line;
                    bool taken;
                    try
                    {
                        taken = _stdoutLines.TryTake(out line, WaitMs(now), _cancelSent ? CancellationToken.None : _cancel);
                    }
                    catch (OperationCanceledException)
                    {
                        continue; // the next round sends the cancel
                    }

                    if (taken)
                    {
                        HandleLine(line);
                    }
                    else if (_stdoutLines.IsCompleted)
                    {
                        if (_exited)
                        {
                            return;
                        }
                        // The tool closed its output and runs on: wait for it, and still cancel and kill
                        _process.WaitForExit(WaitMs(now));
                    }
                    else if (_exited && _clock.Elapsed >= _outputDeadline)
                    {
                        return; // a process out of reach keeps the output open
                    }
                }
            }

            private bool KillPending()
            {
                return _killAt.HasValue && !_killed && !_exited;
            }

            private int WaitMs(TimeSpan now)
            {
                if (!KillPending())
                {
                    return POLL_MS;
                }
                double untilKill = Math.Ceiling((_killAt.Value - now).TotalMilliseconds);
                return (int)Math.Max(0, Math.Min(POLL_MS, untilKill));
            }

            // Closes the tool's input once nothing is left to cancel. agsbuild reads its input for "cancel" on a
            // thread that is still reading when the build ends, and the C runtime's exit() then waits for that
            // thread's lock of stdin: with the input open forever, and mostly also when the input closes after
            // the end (Tools/agsbuild/diagnostics.cpp, InstallCancelHandlers). Closed while the tool is still at
            // work, when all its tasks are done, the thread ends first and the tool exits.
            private void CloseInput()
            {
                if (_inputClosed)
                {
                    return;
                }
                _inputClosed = true;
                try
                {
                    _process.StandardInput.Close();
                }
                catch (IOException ex)
                {
                    // A cancel that could not be written is still in the buffer: the tool has gone
                    _pipeFailure = ex.Message;
                }
            }

            private void OnEnd(BuildToolEvent ev)
            {
                if (_end != null)
                {
                    return;
                }
                _end = ev;
                CloseInput();
                // The tool has nothing left to do, but may not manage to exit (see CloseInput)
                if (!_killAt.HasValue)
                {
                    TimeSpan wait = TimeSpan.FromMilliseconds(EXIT_AFTER_END_MS);
                    _killAt = _clock.Elapsed + (wait < _gracePeriod ? wait : _gracePeriod);
                }
            }

            private void SendCancel(TimeSpan now)
            {
                _cancelSent = true;
                if (_end != null)
                {
                    return; // the tool has reported the end already, and its time to exit runs
                }
                _killAt = now + _gracePeriod;
                if (_inputClosed)
                {
                    return; // all its tasks are done: the end comes without a cancel, or the kill does
                }
                try
                {
                    // Not through the StreamWriter: its encoding comes from the console and could add a preamble
                    byte[] line = Encoding.ASCII.GetBytes("cancel\n");
                    Stream input = _process.StandardInput.BaseStream;
                    input.Write(line, 0, line.Length);
                    input.Flush();
                }
                catch (IOException ex)
                {
                    // The tool has closed its input or exited: it cannot hear the cancel, so it is not waited for
                    _pipeFailure = ex.Message;
                    _killAt = now;
                }
            }

            private void HandleLine(string line)
            {
                BuildToolEvent ev = BuildToolEvent.Parse(line);
                if (ev == null)
                {
                    if (line.Trim().Length > 0)
                    {
                        _plainStdout.Enqueue(line);
                        if (_plainStdout.Count > QUOTED_LINES)
                        {
                            _plainStdout.Dequeue();
                        }
                    }
                    return;
                }

                switch (ev.Kind)
                {
                    case BuildToolEventKind.Start:
                        _done = 0;
                        _total = ev.Tasks;
                        ReportProgress();
                        break;
                    case BuildToolEventKind.Task:
                        if (ev.State == BuildToolTaskState.Running)
                        {
                            _task = ev.Task;
                        }
                        ReportProgress();
                        break;
                    case BuildToolEventKind.Progress:
                        _done = ev.Done;
                        _total = ev.Total;
                        if (_total > 0 && _done >= _total)
                        {
                            CloseInput();
                        }
                        ReportProgress();
                        break;
                    case BuildToolEventKind.Diagnostic:
                        AddDiagnostic(ev);
                        break;
                    case BuildToolEventKind.End:
                        OnEnd(ev);
                        break;
                }
            }

            private void ReportProgress()
            {
                if (_progress != null)
                {
                    _progress(new BuildToolProgress(_done, _total, _task));
                }
            }

            private void AddDiagnostic(BuildToolEvent ev)
            {
                string message = MessageText(ev);
                switch (ev.Severity)
                {
                    case BuildToolSeverity.Error:
                        _messages.Add(ev.File != null ? new CompileError(message, ev.File, ev.Line) : new CompileError(message));
                        break;
                    case BuildToolSeverity.Warning:
                        // The same header compiled with every script repeats its warnings in every script's task
                        if (_warnings.Add(Tuple.Create(ev.File, ev.Line, message)))
                        {
                            _messages.Add(ev.File != null
                                ? new CompileWarning(message, ev.File, ev.Line)
                                : new CompileWarning(message));
                        }
                        break;
                    default:
                        _messages.Add(ev.File != null
                            ? new CompileInformation(message, ev.File, ev.Line)
                            : new CompileInformation(message));
                        break;
                }
            }

            // The diagnostic's text, or what it is about if it has none
            private string MessageText(BuildToolEvent ev)
            {
                if (ev.Message.Length > 0)
                {
                    return ev.Message;
                }
                string what = ev.Severity == BuildToolSeverity.Error
                    ? "an error"
                    : ev.Severity == BuildToolSeverity.Warning ? "a warning" : "a note";
                string who = ev.Task.Length > 0 ? "Task " + ev.Task : (ev.Tool.Length > 0 ? ev.Tool : ToolName);
                return who + " reported " + what + " without a message.";
            }

            // Kills the tool and every process it started
            private void KillTree()
            {
                if (_job != null && _job.Terminate(KILLED_EXIT_CODE))
                {
                    return;
                }
                if (_process.HasExited)
                {
                    return; // without the job, what it may have left running cannot be found
                }
                // taskkill finds the tool's processes through the tool, so it runs before the tool is killed
                string failure = RunTaskkill(_process.Id);
                if (!_process.HasExited)
                {
                    try
                    {
                        _process.Kill();
                    }
                    catch (InvalidOperationException)
                    {
                        Debug.Assert(_process.HasExited, "Process.Kill failed for another reason than an exit");
                    }
                    catch (Win32Exception ex)
                    {
                        failure = ex.Message;
                    }
                }
                if (failure != null)
                {
                    _messages.Add(new CompileWarning("Could not stop all the processes of " + ToolName + ": " + failure));
                }
            }

            // Returns why taskkill failed, or null
            private static string RunTaskkill(int processId)
            {
                // Its output goes to its own hidden console: reading it could wait for longer than the timeout
                var startInfo = new ProcessStartInfo(Path.Combine(Environment.SystemDirectory, "taskkill.exe"),
                    "/F /T /PID " + processId.ToString(CultureInfo.InvariantCulture))
                {
                    UseShellExecute = false,
                    CreateNoWindow = true
                };
                try
                {
                    using (Process taskkill = Process.Start(startInfo))
                    {
                        if (!taskkill.WaitForExit(TASKKILL_TIMEOUT_MS))
                        {
                            return "taskkill did not finish.";
                        }
                        return taskkill.ExitCode == 0
                            ? null
                            : "taskkill failed with exit code " + taskkill.ExitCode.ToString(CultureInfo.InvariantCulture) + ".";
                    }
                }
                catch (Win32Exception ex)
                {
                    return ex.Message;
                }
            }

            private BuildToolResult Finish()
            {
                int? exitCode = null;
                if (_process.WaitForExit(OUTPUT_TIMEOUT_MS))
                {
                    exitCode = _process.ExitCode;
                }
                _stderrReader.Join(OUTPUT_TIMEOUT_MS);
                string log;
                lock (_log)
                {
                    log = _log.ToString();
                }

                BuildToolOutcome outcome;
                if (_end != null)
                {
                    switch (_end.Result)
                    {
                        case BuildToolEndResult.Success:
                            outcome = BuildToolOutcome.Success;
                            break;
                        case BuildToolEndResult.Cancelled:
                            outcome = BuildToolOutcome.Cancelled;
                            break;
                        default:
                            outcome = BuildToolOutcome.Failed;
                            if (!_messages.HasErrors)
                            {
                                _messages.Add(new CompileError(WithLastLines(
                                    ToolName + " reported that the build failed, without an error message.",
                                    LastLines(Enumerable.Empty<string>(), log))));
                            }
                            break;
                    }
                }
                else if (_cancelSent)
                {
                    outcome = BuildToolOutcome.Cancelled;
                }
                else
                {
                    outcome = BuildToolOutcome.Crashed;
                    string text = ToolName + " stopped before the end of the build" +
                                  (exitCode.HasValue ? " (exit code " + FormatExitCode(exitCode.Value) + ")." : ".");
                    if (_pipeFailure != null)
                    {
                        text += " Its pipes failed: " + _pipeFailure;
                    }
                    _messages.Add(new CompileError(WithLastLines(text, LastLines(_plainStdout, log))));
                }

                return new BuildToolResult(outcome, exitCode, _messages, log, _end != null ? _end.Errors : 0,
                    _end != null ? _end.Warnings : 0);
            }

            private static string FormatExitCode(int exitCode)
            {
                // Crashes are NTSTATUS codes, which are read in hexadecimal
                return exitCode >= 0 && exitCode <= 0xFFFF
                    ? exitCode.ToString(CultureInfo.InvariantCulture)
                    : string.Format(CultureInfo.InvariantCulture, "{0} = 0x{0:X8}", exitCode);
            }

            // The last lines that are not blank: of the plain text on stdout, then of the log
            private static List<string> LastLines(IEnumerable<string> plainStdout, string log)
            {
                IEnumerable<string> logLines = log.Split('\n').Select(l => l.TrimEnd('\r'));
                List<string> lines = plainStdout.Concat(logLines).Where(l => l.Trim().Length > 0).ToList();
                return lines.Skip(Math.Max(0, lines.Count - QUOTED_LINES)).ToList();
            }

            private static string WithLastLines(string text, List<string> lines)
            {
                if (lines.Count == 0)
                {
                    return text;
                }
                return text + " The last lines of its output:" + Environment.NewLine +
                       string.Join(Environment.NewLine, lines);
            }
        }
    }
}
