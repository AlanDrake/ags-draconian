using System;
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
    /// Runs one of the build's tools that write human-readable text, not the JSON lines of agsbuild
    /// (po2tra, agspak, crmbuild), as a hidden process, and turns what it writes into a
    /// <see cref="ToolProcessResult"/>.
    ///
    /// A tool reports problems as lines that begin with "Error:", "Warning:" or "Info:"
    /// (<see cref="ParseDiagnostics"/>), and its exit code says whether it succeeded. A failed run,
    /// a missing tool or one that crashes is a result, not an exception; Run throws only for wrong
    /// arguments.
    ///
    /// Run blocks until the tool has exited. Its input is closed at the start, so a tool that reads
    /// it gets the end of the file. The tool and every process it starts are in a Windows job object
    /// (<see cref="ProcessJob"/>): what it leaves running when it exits is killed, and so is the
    /// tool if the editor ends first. Where Windows refuses a job (Windows 7 when the editor is in a
    /// job already), what the tool leaves running stays.
    ///
    /// The class has no state: any number of runs, also at the same time on several threads.
    /// </summary>
    public static class ToolProcess
    {
        // How long the output may stay open after the tool has exited (a process out of reach keeps it open)
        private const int OUTPUT_TIMEOUT_MS = 5000;
        private const uint KILLED_EXIT_CODE = 1;
        // The lines of output quoted in the runner's own error messages
        private const int QUOTED_LINES = 10;
        // The message of a diagnostic that has none, as agsbuild gives it (Tools/agsbuild/diagnostics.cpp)
        private const string NO_MESSAGE = "(no message)";

        /// <summary>
        /// Runs a tool and waits for it to exit.
        /// </summary>
        /// <param name="toolPath">The executable (or batch file); it is not looked for in PATH.</param>
        /// <param name="arguments">The command line after the program's name, quoted already
        /// (<see cref="BuildToolOptions.QuoteArgument"/>); may be null.</param>
        /// <param name="workingDirectory">The tool's current directory; the editor's if null or empty.</param>
        /// <exception cref="ArgumentException">toolPath is null or empty.</exception>
        public static ToolProcessResult Run(string toolPath, string arguments, string workingDirectory)
        {
            if (string.IsNullOrEmpty(toolPath))
            {
                throw new ArgumentException("The tool's path is empty.", nameof(toolPath));
            }
            if (!File.Exists(toolPath))
            {
                return NotStarted("Cannot find the tool " + toolPath + ".");
            }

            var startInfo = new ProcessStartInfo(toolPath, arguments ?? string.Empty)
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
            if (!string.IsNullOrEmpty(workingDirectory))
            {
                startInfo.WorkingDirectory = workingDirectory;
            }

            // The job is ready before the start, so that the process joins it at once: a process it started
            // before joining would not be in the job
            ProcessJob job = ProcessJob.Create();
            try
            {
                var process = new Process { StartInfo = startInfo };
                try
                {
                    process.Start();
                }
                catch (Win32Exception ex)
                {
                    process.Dispose();
                    return NotStarted("Cannot start the tool " + toolPath + ": " + ex.Message);
                }

                using (process)
                {
                    if (job != null && !job.Add(process))
                    {
                        job.Dispose();
                        job = null;
                    }
                    return Execute(process, job, Path.GetFileName(toolPath));
                }
            }
            finally
            {
                if (job != null)
                {
                    job.Dispose(); // kills whatever is still in the job
                }
            }
        }

        /// <summary>
        /// The errors, warnings and information in a tool's text output, in their order, as agsbuild finds
        /// them in the output of its tools (Tools/agsbuild/diagnostics.cpp, ExtractDiagnostics).
        ///
        /// A diagnostic is a line that begins with "Error:", "Warning:" or "Info:", in any case, after
        /// spaces or tabs. When its message has the form "&lt;what&gt; at &lt;file&gt;, line &lt;N&gt; : &lt;message&gt;",
        /// where &lt;what&gt; is a few plain words or nothing, the file and line are taken from it. A
        /// message that is empty or ends with ':' ("Error: failed to read X:") is completed by the next
        /// line, the reason, unless that line is blank, begins with a space or tab, or is a diagnostic
        /// itself. A diagnostic left without a message reads "(no message)". Other lines are ignored.
        /// </summary>
        /// <param name="text">The output; null is no output.</param>
        public static CompileMessages ParseDiagnostics(string text)
        {
            var messages = new CompileMessages();
            if (string.IsNullOrEmpty(text))
            {
                return messages;
            }

            var found = new List<Diagnostic>();
            bool awaitsReason = false;
            foreach (string line in text.Split('\n'))
            {
                Diagnostic diagnostic = ParseLine(line);
                if (diagnostic != null)
                {
                    found.Add(diagnostic);
                    awaitsReason = diagnostic.Message.Length == 0 || diagnostic.Message[diagnostic.Message.Length - 1] == ':';
                    continue;
                }
                // "Error: failed to read X:" and then the reason, as the tools print an error they got
                string reason = line.TrimEnd(' ', '\t', '\r');
                if (awaitsReason && reason.Length > 0 && line[0] != ' ' && line[0] != '\t')
                {
                    Diagnostic last = found[found.Count - 1];
                    last.Message += (last.Message.Length == 0 ? "" : " ") + reason;
                }
                awaitsReason = false;
            }

            foreach (Diagnostic diagnostic in found)
            {
                messages.Add(diagnostic.ToMessage());
            }
            return messages;
        }

        private static ToolProcessResult NotStarted(string message)
        {
            return new ToolProcessResult(null, new CompileMessages(new CompileError(message)), null);
        }

        // Reads the started tool's output until it has exited, and makes the result
        private static ToolProcessResult Execute(Process process, ProcessJob job, string toolName)
        {
            string pipeFailure = CloseInput(process);
            var stdout = new OutputReader(process.StandardOutput, toolName + " stdout");
            var stderr = new OutputReader(process.StandardError, toolName + " stderr");

            process.WaitForExit();
            int exitCode = process.ExitCode;
            if (job != null)
            {
                // Whatever the tool left running is not needed, and could keep the output open
                job.Terminate(KILLED_EXIT_CODE);
            }
            // Both are waited for: the second may have finished while the first was waited for
            bool stdoutRead = stdout.Finish(OUTPUT_TIMEOUT_MS);
            bool stderrRead = stderr.Finish(OUTPUT_TIMEOUT_MS);
            string stdoutText = stdout.Text;
            string stderrText = stderr.Text;
            pipeFailure = stdout.Failure ?? stderr.Failure ?? pipeFailure;

            CompileMessages messages = ParseDiagnostics(stdoutText);
            messages.AddRange(ParseDiagnostics(stderrText));
            if (exitCode != 0 && !messages.HasErrors)
            {
                string text = toolName + " failed with exit code " + FormatExitCode(exitCode) + ".";
                if (pipeFailure != null)
                {
                    text += " Its pipes failed: " + pipeFailure;
                }
                else if (!stdoutRead || !stderrRead)
                {
                    text += " Its output was not read to the end.";
                }
                messages.Add(new CompileError(WithLastLines(text, LastLines(stdoutText, stderrText))));
            }
            return new ToolProcessResult(exitCode, messages, JoinOutput(stdoutText, stderrText));
        }

        // Closes the tool's input, so that a tool that reads it does not wait; returns why that failed, or null
        private static string CloseInput(Process process)
        {
            try
            {
                process.StandardInput.Close();
                return null;
            }
            catch (IOException ex)
            {
                // The tool has gone already: its exit code tells the rest
                return ex.Message;
            }
        }

        private static string JoinOutput(string stdout, string stderr)
        {
            if (stdout.Length == 0 || stderr.Length == 0 || stdout.EndsWith("\n", StringComparison.Ordinal))
            {
                return stdout + stderr;
            }
            return stdout + Environment.NewLine + stderr;
        }

        private static string FormatExitCode(int exitCode)
        {
            // Crashes are NTSTATUS codes, which are read in hexadecimal
            return exitCode >= 0 && exitCode <= 0xFFFF
                ? exitCode.ToString(CultureInfo.InvariantCulture)
                : string.Format(CultureInfo.InvariantCulture, "{0} = 0x{0:X8}", exitCode);
        }

        // The last lines that are not blank: of the standard output, then of the standard error
        private static List<string> LastLines(string stdout, string stderr)
        {
            List<string> lines = stdout.Split('\n').Concat(stderr.Split('\n'))
                .Select(l => l.TrimEnd('\r'))
                .Where(l => l.Trim().Length > 0)
                .ToList();
            return lines.Skip(Math.Max(0, lines.Count - QUOTED_LINES)).ToList();
        }

        private static string WithLastLines(string text, List<string> lines)
        {
            if (lines.Count == 0)
            {
                return text;
            }
            return text + " The last lines of its output:" + Environment.NewLine + string.Join(Environment.NewLine, lines);
        }

        // One line of output as a diagnostic, or null if it is not one (diagnostics.cpp, ParseDiagnosticLine)
        private static Diagnostic ParseLine(string line)
        {
            int pos = 0;
            while (pos < line.Length && (line[pos] == ' ' || line[pos] == '\t'))
            {
                pos++;
            }
            BuildToolSeverity severity;
            int prefixLength;
            if (StartsWithNoCase(line, pos, "error:"))
            {
                severity = BuildToolSeverity.Error;
                prefixLength = 6;
            }
            else if (StartsWithNoCase(line, pos, "warning:"))
            {
                severity = BuildToolSeverity.Warning;
                prefixLength = 8;
            }
            else if (StartsWithNoCase(line, pos, "info:"))
            {
                severity = BuildToolSeverity.Info;
                prefixLength = 5;
            }
            else
            {
                return null;
            }

            string rest = line.Substring(pos + prefixLength).TrimStart(' ', '\t').TrimEnd(' ', '\t', '\r', '\n');
            var diagnostic = new Diagnostic(severity, rest);

            // "<what> at <file>, line <N> : <message>", where <what> is a few plain words, or nothing
            int atPos;
            int filePos;
            if (rest.StartsWith("at ", StringComparison.Ordinal))
            {
                atPos = 0;
                filePos = 3;
            }
            else
            {
                atPos = rest.IndexOf(" at ", StringComparison.Ordinal);
                filePos = atPos + 4;
            }
            if (atPos < 0 || !rest.Take(atPos).All(IsLetterOrSpace))
            {
                return diagnostic;
            }
            int linePos = rest.IndexOf(", line ", filePos, StringComparison.Ordinal);
            if (linePos < 0)
            {
                return diagnostic;
            }
            int digits = linePos + 7;
            int digitsEnd = digits;
            while (digitsEnd < rest.Length && rest[digitsEnd] >= '0' && rest[digitsEnd] <= '9')
            {
                digitsEnd++;
            }
            if (digitsEnd > digits && digitsEnd - digits < 9 && digitsEnd + 3 <= rest.Length &&
                string.CompareOrdinal(rest, digitsEnd, " : ", 0, 3) == 0)
            {
                diagnostic.File = rest.Substring(filePos, linePos - filePos);
                diagnostic.Line = int.Parse(rest.Substring(digits, digitsEnd - digits), NumberStyles.None,
                    CultureInfo.InvariantCulture);
                diagnostic.Message = rest.Substring(digitsEnd + 3);
            }
            return diagnostic;
        }

        // Only ASCII letters are folded, as in agsbuild
        private static bool StartsWithNoCase(string s, int pos, string lowerPrefix)
        {
            if (s.Length < pos + lowerPrefix.Length)
            {
                return false;
            }
            for (int i = 0; i < lowerPrefix.Length; i++)
            {
                char c = s[pos + i];
                if (c >= 'A' && c <= 'Z')
                {
                    c = (char)(c - 'A' + 'a');
                }
                if (c != lowerPrefix[i])
                {
                    return false;
                }
            }
            return true;
        }

        private static bool IsLetterOrSpace(char c)
        {
            return c == ' ' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }

        /// <summary>
        /// A diagnostic while its message may still grow by the next line.
        /// </summary>
        private sealed class Diagnostic
        {
            public Diagnostic(BuildToolSeverity severity, string message)
            {
                Severity = severity;
                Message = message;
            }

            public BuildToolSeverity Severity { get; }
            public string Message { get; set; }
            public string File { get; set; }
            public int Line { get; set; }

            public CompileMessage ToMessage()
            {
                string message = Message.Length > 0 ? Message : NO_MESSAGE;
                // agsbuild leaves out an empty file name, and the line with it
                bool located = !string.IsNullOrEmpty(File);
                switch (Severity)
                {
                    case BuildToolSeverity.Error:
                        return located ? new CompileError(message, File, Line) : new CompileError(message);
                    case BuildToolSeverity.Warning:
                        return located ? new CompileWarning(message, File, Line) : new CompileWarning(message);
                    default:
                        return located ? new CompileInformation(message, File, Line) : new CompileInformation(message);
                }
            }
        }

        /// <summary>
        /// Reads all of one of the tool's output streams on a thread of its own, so that a full pipe
        /// cannot hold the tool up.
        /// </summary>
        private sealed class OutputReader
        {
            private readonly StreamReader _reader;
            private readonly StringBuilder _text = new StringBuilder();
            private readonly Thread _thread;
            private volatile string _failure;

            public OutputReader(StreamReader reader, string name)
            {
                _reader = reader;
                _thread = new Thread(Read) { IsBackground = true, Name = name };
                _thread.Start();
            }

            /// <summary>What has been read so far.</summary>
            public string Text
            {
                get
                {
                    lock (_text)
                    {
                        return _text.ToString();
                    }
                }
            }

            /// <summary>Why reading failed, or null.</summary>
            public string Failure
            {
                get { return _failure; }
            }

            /// <summary>
            /// Waits for the end of the stream, and closes it. Returns false if the stream is still open
            /// after the timeout: a process out of reach keeps it open, and the thread keeps the stream,
            /// leaving it to the garbage collector.
            /// </summary>
            public bool Finish(int timeoutMs)
            {
                if (!_thread.Join(timeoutMs))
                {
                    return false;
                }
                _reader.Dispose();
                return true;
            }

            private void Read()
            {
                var buffer = new char[8192];
                try
                {
                    int count;
                    while ((count = _reader.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        lock (_text)
                        {
                            _text.Append(buffer, 0, count);
                        }
                    }
                }
                catch (IOException ex)
                {
                    _failure = ex.Message;
                }
            }
        }
    }
}
