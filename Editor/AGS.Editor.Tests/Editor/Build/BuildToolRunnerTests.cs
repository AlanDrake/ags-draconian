using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// BuildToolRunner against batch files that play the part of agsbuild: they write canned
    /// output (with "type", which copies the bytes as they are), wait for the standard input, or
    /// sleep, and exit with a chosen code.
    /// </summary>
    [TestFixture]
    [Timeout(30000)]
    public class BuildToolRunnerTests
    {
        private static readonly Encoding Utf8 = new UTF8Encoding(false);
        private string _dir;

        [SetUp]
        public void SetUp()
        {
            _dir = Path.Combine(Path.GetTempPath(), "AGS.Editor.Tests", "BuildToolRunner-" + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_dir);
        }

        [TearDown]
        public void TearDown()
        {
            try
            {
                Directory.Delete(_dir, true);
            }
            catch (IOException ex)
            {
                TestContext.WriteLine("Cannot delete " + _dir + ": " + ex.Message);
            }
        }

        [Test]
        public void Constructor_NegativeGracePeriod_Throws()
        {
            Assert.Throws<ArgumentOutOfRangeException>(() => new BuildToolRunner(TimeSpan.FromSeconds(-1)));
            Assert.That(new BuildToolRunner().CancelGracePeriod, Is.EqualTo(TimeSpan.FromSeconds(5)));
        }

        [Test]
        public void Run_SuccessfulBuild_ReportsProgressMessagesAndLog()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":3}",
                @"{""event"":""task"",""task"":""autoash"",""state"":""running""}",
                "plain text that is not an event",
                @"{""event"":""task"",""task"":""agf2dta"",""state"":""running""}",
                @"{""event"":""task"",""task"":""autoash"",""state"":""success""}",
                @"{""event"":""progress"",""done"":1,""total"":3}",
                @"{""event"":""diagnostic"",""severity"":""warning"",""tool"":""agscc"",""task"":""agscc:__DialogScripts.asc""," +
                @"""file"":""Dialog 3"",""line"":23,""message"":""Code execution cannot reach this point""}",
                @"{""event"":""diagnostic"",""severity"":""info"",""tool"":""agspak"",""task"":""agspak:game.ags""," +
                "\"message\":\"found pattern file 'Caf\u00e9 \u65e5\u672c.files'.\"}",
                @"{""event"":""task"",""task"":""agf2dta"",""state"":""success""}",
                @"{""event"":""progress"",""done"":3,""total"":3}",
                @"{""event"":""end"",""result"":""success"",""errors"":0,""warnings"":1}"));
            string log = Lines("System dir: C:\\AGS", "Caf\u00e9 \u65e5\u672c done.");
            WriteText("stderr.txt", log);
            string tool = WriteBatch("tool.bat", CannedBody(0));
            var runner = new BuildToolRunner();

            // Twice: a runner can be used again
            for (int run = 0; run < 2; run++)
            {
                var reports = new List<string>();
                int callerThread = Thread.CurrentThread.ManagedThreadId;
                var threads = new HashSet<int>();

                BuildToolResult result = runner.Run(tool, null, p =>
                {
                    threads.Add(Thread.CurrentThread.ManagedThreadId);
                    reports.Add(p.Done + "/" + p.Total + " " + (p.Task ?? "-"));
                }, CancellationToken.None);

                Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success));
                Assert.That(result.ExitCode, Is.EqualTo(0));
                Assert.That(result.Errors, Is.EqualTo(0));
                Assert.That(result.Warnings, Is.EqualTo(1));
                Assert.That(result.Log, Is.EqualTo(log));
                Assert.That(reports, Is.EqualTo(new[]
                {
                    "0/3 -", "0/3 autoash", "0/3 agf2dta", "0/3 agf2dta", "1/3 agf2dta", "1/3 agf2dta", "3/3 agf2dta"
                }));
                Assert.That(threads, Is.EqualTo(new[] { callerThread }), "progress is reported on the caller's thread");
                Assert.That(Describe(result.Messages), Is.EqualTo(new[]
                {
                    "Warning|Dialog 3|23|Code execution cannot reach this point",
                    "Information||0|found pattern file 'Caf\u00e9 \u65e5\u672c.files'.",
                }));
            }
        }

        [Test]
        public void Run_FailedBuild_ConvertsDiagnosticsInOrder()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":2}",
                @"{""event"":""task"",""task"":""agscc:GlobalScript.asc"",""state"":""running""}",
                @"{""event"":""task"",""task"":""agscc:room1.asc"",""state"":""running""}",
                Diagnostic("warning", "agscc:GlobalScript.asc", "Header.ash", 4, "Unused variable 'x'"),
                Diagnostic("error", "agscc:GlobalScript.asc", "GlobalScript.asc", 12, "Undefined token 'foo'"),
                @"{""event"":""task"",""task"":""agscc:GlobalScript.asc"",""state"":""failed""}",
                // The same header's warning from another script: left out
                Diagnostic("warning", "agscc:room1.asc", "Header.ash", 4, "Unused variable 'x'"),
                // On another line, in another file, or with another text: kept
                Diagnostic("warning", "agscc:room1.asc", "Header.ash", 5, "Unused variable 'x'"),
                Diagnostic("warning", "agscc:room1.asc", "Other.ash", 4, "Unused variable 'x'"),
                Diagnostic("warning", "agscc:room1.asc", "Header.ash", 4, "Unused variable 'y'"),
                // A repeated error is kept
                Diagnostic("error", "agscc:room1.asc", "Rooms\\\\1\\\\room1.asc", 7, "Undefined token 'foo'"),
                Diagnostic("error", "agscc:room1.asc", "Rooms\\\\1\\\\room1.asc", 7, "Undefined token 'foo'"),
                @"{""event"":""diagnostic"",""severity"":""info"",""tool"":""agscc"",""task"":""agscc:room1.asc"",""message"":""a note""}",
                @"{""event"":""diagnostic"",""severity"":""warning"",""tool"":""agscc"",""task"":""t"",""message"":""no file""}",
                @"{""event"":""diagnostic"",""severity"":""warning"",""tool"":""agscc"",""task"":""t"",""message"":""no file""}",
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agsbuild"",""task"":""copy-rooms"",""message"":""""}",
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agsbuild"",""task"":"""",""message"":""""}",
                @"{""event"":""task"",""task"":""agscc:room1.asc"",""state"":""failed""}",
                @"{""event"":""progress"",""done"":2,""total"":2}",
                @"{""event"":""end"",""result"":""failed"",""errors"":6,""warnings"":7}"));
            WriteText("stderr.txt", "");
            string tool = WriteBatch("tool.bat", CannedBody(1));

            BuildToolResult result = new BuildToolRunner().Run(tool, "", null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Failed));
            Assert.That(result.ExitCode, Is.EqualTo(1));
            Assert.That(result.Errors, Is.EqualTo(6), "the counts are the tool's");
            Assert.That(result.Warnings, Is.EqualTo(7), "the counts are the tool's");
            Assert.That(Describe(result.Messages), Is.EqualTo(new[]
            {
                "Warning|Header.ash|4|Unused variable 'x'",
                "Error|GlobalScript.asc|12|Undefined token 'foo'",
                "Warning|Header.ash|5|Unused variable 'x'",
                "Warning|Other.ash|4|Unused variable 'x'",
                "Warning|Header.ash|4|Unused variable 'y'",
                "Error|Rooms\\1\\room1.asc|7|Undefined token 'foo'",
                "Error|Rooms\\1\\room1.asc|7|Undefined token 'foo'",
                "Information||0|a note",
                "Warning||0|no file",
                "Error||0|Task copy-rooms reported an error without a message.",
                "Error||0|agsbuild reported an error without a message.",
            }));
        }

        [Test]
        public void Run_FailedWithoutErrorMessage_AddsErrorFromLog()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":1}",
                @"{""event"":""end"",""result"":""failed"",""errors"":0,""warnings"":0}"));
            WriteText("stderr.txt", Lines("first line", "", "Something broke at the end"));
            string tool = WriteBatch("tool.bat", CannedBody(-1));

            BuildToolResult result = new BuildToolRunner().Run(tool, null, null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Failed));
            Assert.That(result.ExitCode, Is.EqualTo(-1));
            Assert.That(result.Messages, Has.Count.EqualTo(1));
            Assert.That(result.Messages[0], Is.InstanceOf<CompileError>());
            Assert.That(result.Messages[0].Message, Does.Contain("tool.bat reported that the build failed"));
            Assert.That(result.Messages[0].Message, Does.Contain("first line" + Environment.NewLine + "Something broke at the end"));
        }

        [Test]
        public void Run_ExitWithoutEnd_IsCrashedWithExitCodeAndLastOutput()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":1}",
                @"{""event"":""task"",""task"":""agf2dta"",""state"":""running""}",
                Diagnostic("warning", "agf2dta", "a.asc", 1, "a warning before the crash"),
                "plain text on stdout"));
            WriteText("stderr.txt", Lines("Assertion failed: x != nullptr"));
            string tool = WriteBatch("tool.bat", CannedBody(-1073741819));

            BuildToolResult result = new BuildToolRunner().Run(tool, null, null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Crashed));
            Assert.That(result.ExitCode, Is.EqualTo(-1073741819));
            Assert.That(result.Messages, Has.Count.EqualTo(2));
            Assert.That(result.Messages[0], Is.InstanceOf<CompileWarning>());
            Assert.That(result.Messages[1], Is.InstanceOf<CompileError>());
            string message = result.Messages[1].Message;
            Assert.That(message, Does.Contain("tool.bat stopped before the end of the build"));
            Assert.That(message, Does.Contain("-1073741819 = 0xC0000005"));
            Assert.That(message, Does.Contain("plain text on stdout" + Environment.NewLine + "Assertion failed: x != nullptr"));
            Assert.That(message, Does.Not.Contain("\"event\""));
        }

        [Test]
        public void Run_Cancelled_WritesCancelToInputAndReadsTheEnd()
        {
            WriteText("stdout.txt", Lines(@"{""event"":""start"",""tasks"":4}"));
            WriteText("cancelled.txt", Lines(
                @"{""event"":""task"",""task"":""agscc:a.asc"",""state"":""cancelled""}",
                @"{""event"":""end"",""result"":""cancelled"",""errors"":0,""warnings"":0}"));
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "type \"%~dp0stdout.txt\"",
                "set /p LINE=",
                ">\"%~dp0received.txt\" echo(%LINE%",
                "if \"%LINE%\"==\"cancel\" (",
                "  type \"%~dp0cancelled.txt\"",
                "  exit 2",
                ")",
                "exit 5"));
            var cancel = new CancellationTokenSource();
            var clock = Stopwatch.StartNew();

            BuildToolResult result = new BuildToolRunner(TimeSpan.FromSeconds(20)).Run(tool, null, p => cancel.Cancel(),
                cancel.Token);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Cancelled));
            Assert.That(result.ExitCode, Is.EqualTo(2), "the tool ended on its own");
            Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)), "not killed after the grace period");
            Assert.That(File.ReadAllText(Path.Combine(_dir, "received.txt")).TrimEnd(), Is.EqualTo("cancel"));
            Assert.That(result.Messages, Is.Empty);
        }

        [Test]
        public void Run_CancelledToolThatIgnoresIt_IsKilledWithItsChildren()
        {
            WriteText("stdout.txt", Lines(@"{""event"":""start"",""tasks"":1}"));
            string tool = WriteBatch("tool.bat", SleeperBody());
            ProcessSnapshot before = ProcessSnapshot.Take();
            var started = new List<KeyValuePair<ProcessSnapshot.Entry, Process>>();
            var cancel = new CancellationTokenSource();
            var clock = Stopwatch.StartNew();

            BuildToolResult result = new BuildToolRunner(TimeSpan.FromSeconds(1)).Run(tool, null, p =>
            {
                if (!cancel.IsCancellationRequested)
                {
                    started.AddRange(ProcessSnapshot.Open(ProcessSnapshot.Take().NewDescendantsOf(CurrentProcessId, before)));
                    cancel.Cancel();
                }
            }, cancel.Token);

            try
            {
                Assert.That(started.Select(p => p.Key.ExeFile.ToLowerInvariant()), Does.Contain("cmd.exe").And.Contain("ping.exe"),
                    "the batch file and the child it started were running");
                Assert.That(ProcessSnapshot.StillRunning(started, 5000), Is.Empty, "processes left running");
                Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Cancelled));
                Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)));
                Assert.That(result.Messages, Is.Empty);
            }
            finally
            {
                foreach (KeyValuePair<ProcessSnapshot.Entry, Process> process in started)
                {
                    process.Value.Dispose();
                }
            }
        }

        [Test]
        public void Run_ToolReadingItsInputAfterTheEnd_GetsEndOfFile()
        {
            // agsbuild exits only when its input closes (Tools/agsbuild/diagnostics.cpp, InstallCancelHandlers)
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":1}",
                @"{""event"":""end"",""result"":""success"",""errors"":0,""warnings"":0}"));
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "type \"%~dp0stdout.txt\"",
                "set /p LINE=",
                ">\"%~dp0received.txt\" echo(%LINE%",
                "exit 0"));
            var clock = Stopwatch.StartNew();

            BuildToolResult result = new BuildToolRunner(TimeSpan.FromSeconds(20)).Run(tool, null, null,
                CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success));
            Assert.That(result.ExitCode, Is.EqualTo(0), "the tool ended on its own");
            Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)));
            Assert.That(File.ReadAllText(Path.Combine(_dir, "received.txt")).Trim(), Is.Empty);
        }

        [Test]
        public void Run_AllTasksDone_ClosesTheInputBeforeTheEnd()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":2}",
                @"{""event"":""progress"",""done"":1,""total"":2}",
                @"{""event"":""progress"",""done"":2,""total"":2}"));
            WriteText("end.txt", Lines(@"{""event"":""end"",""result"":""success"",""errors"":0,""warnings"":0}"));
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "type \"%~dp0stdout.txt\"",
                "set /p LINE=",
                ">\"%~dp0received.txt\" echo(%LINE%",
                "type \"%~dp0end.txt\"",
                "exit 0"));
            var clock = Stopwatch.StartNew();

            BuildToolResult result = new BuildToolRunner(TimeSpan.FromSeconds(20)).Run(tool, null, null,
                CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success));
            Assert.That(result.ExitCode, Is.EqualTo(0), "the tool ended on its own");
            Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)));
            Assert.That(File.ReadAllText(Path.Combine(_dir, "received.txt")).Trim(), Is.Empty);
        }

        [Test]
        public void Run_ToolThatDoesNotExitAfterTheEnd_IsKilled()
        {
            WriteText("stdout.txt", Lines(
                @"{""event"":""start"",""tasks"":1}",
                @"{""event"":""end"",""result"":""success"",""errors"":0,""warnings"":2}"));
            string tool = WriteBatch("tool.bat", SleeperBody());
            var clock = Stopwatch.StartNew();

            // Not after the grace period: after a second
            BuildToolResult result = new BuildToolRunner(TimeSpan.FromSeconds(20)).Run(tool, null, null,
                CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success), "the end decides");
            Assert.That(result.Warnings, Is.EqualTo(2));
            Assert.That(result.ExitCode, Is.Not.EqualTo(0), "killed");
            Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)));
        }

        [Test]
        public void Run_ProgressCallbackThrows_KillsTheToolAndRethrows()
        {
            WriteText("stdout.txt", Lines(@"{""event"":""start"",""tasks"":1}"));
            string tool = WriteBatch("tool.bat", SleeperBody());
            ProcessSnapshot before = ProcessSnapshot.Take();
            var started = new List<KeyValuePair<ProcessSnapshot.Entry, Process>>();
            var clock = Stopwatch.StartNew();

            Assert.Throws<InvalidOperationException>(() => new BuildToolRunner().Run(tool, null, p =>
            {
                started.AddRange(ProcessSnapshot.Open(ProcessSnapshot.Take().NewDescendantsOf(CurrentProcessId, before)));
                throw new InvalidOperationException("from the callback");
            }, CancellationToken.None));

            try
            {
                Assert.That(clock.Elapsed, Is.LessThan(TimeSpan.FromSeconds(10)));
                Assert.That(started, Is.Not.Empty);
                Assert.That(ProcessSnapshot.StillRunning(started, 5000), Is.Empty, "processes left running");
            }
            finally
            {
                foreach (KeyValuePair<ProcessSnapshot.Entry, Process> process in started)
                {
                    process.Value.Dispose();
                }
            }
        }

        [Test]
        public void Run_MegabytesOnStandardError_DoNotBlockTheTool()
        {
            WriteText("head.txt", Lines(@"{""event"":""start"",""tasks"":2}"));
            WriteText("tail.txt", Lines(
                @"{""event"":""progress"",""done"":2,""total"":2}",
                @"{""event"":""end"",""result"":""success"",""errors"":0,""warnings"":0}"));
            var big = new StringBuilder();
            for (int i = 0; big.Length < 6 * 1024 * 1024; i++)
            {
                big.Append("Task agscc:script").Append(i).Append(".asc: a line of the log that is long enough to fill pipes\r\n");
            }
            WriteText("big.txt", big.ToString());
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "type \"%~dp0head.txt\"",
                "type \"%~dp0big.txt\" 1>&2",
                "type \"%~dp0tail.txt\"",
                "type \"%~dp0big.txt\" 1>&2",
                "exit 0"));
            var reports = new List<int>();

            BuildToolResult result = new BuildToolRunner().Run(tool, null, p => reports.Add(p.Done), CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success));
            Assert.That(reports, Is.EqualTo(new[] { 0, 2 }));
            Assert.That(result.Log.Length, Is.EqualTo(big.Length * 2));
            Assert.That(result.Log, Is.EqualTo(big.ToString() + big));
        }

        [Test]
        public void Run_MissingExecutable_IsToolNotFound()
        {
            string tool = Path.Combine(_dir, "missing.exe");

            BuildToolResult result = new BuildToolRunner().Run(tool, "--json", null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.ToolNotFound));
            Assert.That(result.ExitCode, Is.Null);
            Assert.That(result.Messages, Has.Count.EqualTo(1));
            Assert.That(result.Messages[0], Is.InstanceOf<CompileError>());
            Assert.That(result.Messages[0].Message, Does.Contain(tool));
        }

        [Test]
        public void Run_OptionsWithoutAgsbuild_IsToolNotFound()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = Path.Combine(_dir, "Game.agf"),
                OutputDirectory = _dir,
                ToolsDirectory = _dir,
            };

            BuildToolResult result = new BuildToolRunner().Run(options, null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.ToolNotFound));
            Assert.That(result.Messages.Single().Message, Does.Contain(Path.Combine(_dir, BuildToolRunner.ToolFileName)));
        }

        [Test]
        public void Run_CancelledBeforeTheStart_DoesNotStartTheTool()
        {
            string tool = WriteBatch("tool.bat", Lines("@echo off", ">\"%~dp0started.txt\" echo started", "exit 0"));
            var cancel = new CancellationTokenSource();
            cancel.Cancel();

            BuildToolResult result = new BuildToolRunner().Run(tool, null, null, cancel.Token);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Cancelled));
            Assert.That(result.ExitCode, Is.Null);
            Assert.That(result.Messages, Is.Empty);
            Assert.That(File.Exists(Path.Combine(_dir, "started.txt")), Is.False);
        }

        [Test]
        public void Run_EmptyToolPath_Throws()
        {
            Assert.Throws<ArgumentException>(() => new BuildToolRunner().Run((string)null, null, null, CancellationToken.None));
            Assert.Throws<ArgumentNullException>(() => new BuildToolRunner().Run((BuildToolOptions)null, null, CancellationToken.None));
        }

        private static int CurrentProcessId
        {
            get
            {
                using (Process current = Process.GetCurrentProcess())
                {
                    return current.Id;
                }
            }
        }

        // Writes stdout.txt and stderr.txt as they are, and exits
        private static string CannedBody(int exitCode)
        {
            return Lines(
                "@echo off",
                "type \"%~dp0stdout.txt\"",
                "type \"%~dp0stderr.txt\" 1>&2",
                "exit " + exitCode);
        }

        // Writes stdout.txt, and sleeps with a child process that sleeps too; does not read its input
        private static string SleeperBody()
        {
            return Lines(
                "@echo off",
                "start \"\" /b ping -n 30 127.0.0.1 >nul",
                "type \"%~dp0stdout.txt\"",
                "ping -n 30 127.0.0.1 >nul",
                "exit 0");
        }

        private static string Diagnostic(string severity, string task, string file, int line, string message)
        {
            return "{\"event\":\"diagnostic\",\"severity\":\"" + severity + "\",\"tool\":\"agscc\",\"task\":\"" + task +
                   "\",\"file\":\"" + file + "\",\"line\":" + line + ",\"message\":\"" + message + "\"}";
        }

        private static string Lines(params string[] lines)
        {
            return string.Concat(lines.Select(l => l + "\r\n"));
        }

        private void WriteText(string name, string text)
        {
            File.WriteAllText(Path.Combine(_dir, name), text, Utf8);
        }

        private string WriteBatch(string name, string text)
        {
            string path = Path.Combine(_dir, name);
            File.WriteAllText(path, text, Encoding.ASCII);
            return path;
        }

        private static IEnumerable<string> Describe(CompileMessages messages)
        {
            return messages.Select(m =>
                m.GetType().Name.Replace("Compile", "") + "|" + m.ScriptName + "|" + m.LineNumber + "|" + m.Message);
        }
    }
}
