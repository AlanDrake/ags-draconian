using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// ToolProcess: the diagnostics in a tool's text, and runs of batch files that play the part of
    /// po2tra, agspak or crmbuild: they write text (with "echo", or with "type", which copies the
    /// bytes as they are) and exit with a chosen code.
    /// </summary>
    [TestFixture]
    [Timeout(30000)]
    public class ToolProcessTests
    {
        private static readonly Encoding Utf8 = new UTF8Encoding(false);
        private string _dir;

        [SetUp]
        public void SetUp()
        {
            _dir = Path.Combine(Path.GetTempPath(), "AGS.Editor.Tests", "ToolProcess-" + Guid.NewGuid().ToString("N"));
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
        public void ParseDiagnostics_ErrorEndingWithColon_TakesTheReasonFromTheNextLine()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(Lines(
                "Error: failed to read Rooms/1/room1.crm:",
                "file not found",
                "not a reason any more",
                "Error:",
                "the reason of an empty one"));

            Assert.That(Describe(messages), Is.EqualTo(new[]
            {
                "Error||0|failed to read Rooms/1/room1.crm: file not found",
                "Error||0|the reason of an empty one",
            }));
        }

        [Test]
        public void ParseDiagnostics_IndentedBlankOrDiagnosticLineAfterColon_IsNotTheReason()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(Lines(
                "Error: failed to read a.txt:",
                "  indented detail",
                "Error: failed to read b.txt:",
                "\ttabbed detail",
                "Error: failed to read c.txt:",
                "",
                "after a blank line",
                "Error: failed to read d.txt:",
                "Warning: a warning of its own",
                "Error:"));

            Assert.That(Describe(messages), Is.EqualTo(new[]
            {
                "Error||0|failed to read a.txt:",
                "Error||0|failed to read b.txt:",
                "Error||0|failed to read c.txt:",
                "Error||0|failed to read d.txt:",
                "Warning||0|a warning of its own",
                "Error||0|(no message)",
            }));
        }

        [Test]
        public void ParseDiagnostics_WarningAndInfo_AreWarningAndInformation()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(Lines(
                "Warning: entry 'Hello' has no translation",
                "Info: found pattern file 'Caf\u00e9 \u65e5\u672c.files'."));

            Assert.That(messages[0], Is.InstanceOf<CompileWarning>());
            Assert.That(messages[1], Is.InstanceOf<CompileInformation>());
            Assert.That(Describe(messages), Is.EqualTo(new[]
            {
                "Warning||0|entry 'Hello' has no translation",
                "Information||0|found pattern file 'Caf\u00e9 \u65e5\u672c.files'.",
            }));
        }

        [Test]
        public void ParseDiagnostics_AtFileLine_GivesFileAndLine()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(Lines(
                "Error: Script error at room1.asc, line 12 : Undefined token 'foo'",
                "Warning: at Rooms\\1\\room1.asc, line 3 : Unused variable 'x'",
                "Info: in the header at Global Header.ash, line 40 : a note",
                // Not plain words before " at ", too many digits, no " : ", no file: the whole text
                "Error: C:\\x.txt at a.asc, line 1 : not located",
                "Error: Script error at a.asc, line 123456789 : too long a number",
                "Error: Script error at a.asc, line 7: no space before the colon",
                "Error: at , line 5 : no file name"));

            Assert.That(Describe(messages), Is.EqualTo(new[]
            {
                "Error|room1.asc|12|Undefined token 'foo'",
                "Warning|Rooms\\1\\room1.asc|3|Unused variable 'x'",
                "Information|Global Header.ash|40|a note",
                "Error||0|C:\\x.txt at a.asc, line 1 : not located",
                "Error||0|Script error at a.asc, line 123456789 : too long a number",
                "Error||0|Script error at a.asc, line 7: no space before the colon",
                "Error||0|no file name",
            }));
        }

        [Test]
        public void ParseDiagnostics_Prefixes_AreCaseInsensitiveAfterWhitespace()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(
                "ERROR: upper\r\n  warning: lower, indented\r\n\tInFo:mixed, tabbed, no space   \r\neRrOr:  trailing spaces  ");

            Assert.That(Describe(messages), Is.EqualTo(new[]
            {
                "Error||0|upper",
                "Warning||0|lower, indented",
                "Information||0|mixed, tabbed, no space",
                "Error||0|trailing spaces",
            }));
        }

        [Test]
        public void ParseDiagnostics_PlainLines_AreIgnored()
        {
            CompileMessages messages = ToolProcess.ParseDiagnostics(Lines(
                "Compiling room 1",
                "Errors: 0",
                "No error: here",
                "Warnings: 2",
                "Information: not a prefix",
                "",
                "   "));

            Assert.That(messages, Is.Empty);
            Assert.That(ToolProcess.ParseDiagnostics(null), Is.Empty);
            Assert.That(ToolProcess.ParseDiagnostics(""), Is.Empty);
        }

        [Test]
        public void Run_ToolPrintingAnError_FailsWithThatError()
        {
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "echo Reading the file",
                "echo Error: something",
                "exit 1"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Succeeded, Is.False);
            Assert.That(result.ExitCode, Is.EqualTo(1));
            Assert.That(Describe(result.Messages), Is.EqualTo(new[] { "Error||0|something" }));
            Assert.That(result.Output, Is.EqualTo(Lines("Reading the file", "Error: something")));
        }

        [Test]
        public void Run_ToolSucceeding_HasNoMessagesAndKeepsItsOutput()
        {
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "echo Wrote 12 entries",
                "echo on the error stream 1>&2",
                "exit 0"));

            ToolProcessResult result = ToolProcess.Run(tool, "", null);

            Assert.That(result.Succeeded, Is.True);
            Assert.That(result.ExitCode, Is.EqualTo(0));
            Assert.That(result.Messages, Is.Empty);
            Assert.That(result.Output, Is.EqualTo(Lines("Wrote 12 entries", "on the error stream ")));
        }

        [Test]
        public void Run_DiagnosticsOnBothStreams_StandardOutputFirst()
        {
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "echo Warning: on stderr 1>&2",
                "echo Warning: on stdout",
                "exit 0"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Succeeded, Is.True);
            Assert.That(Describe(result.Messages), Is.EqualTo(new[] { "Warning||0|on stdout", "Warning||0|on stderr" }));
        }

        [Test]
        public void Run_ExitCodeWithoutError_AddsErrorWithExitCodeAndLastOutput()
        {
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "echo Warning: a warning first",
                "echo line one",
                "echo line two 1>&2",
                "exit 3"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Succeeded, Is.False);
            Assert.That(result.ExitCode, Is.EqualTo(3));
            Assert.That(result.Messages, Has.Count.EqualTo(2));
            Assert.That(result.Messages[0], Is.InstanceOf<CompileWarning>());
            Assert.That(result.Messages[1], Is.InstanceOf<CompileError>());
            string message = result.Messages[1].Message;
            Assert.That(message, Does.StartWith("tool.bat failed with exit code 3."));
            Assert.That(message, Does.EndWith("line one" + Environment.NewLine + "line two "));
        }

        [Test]
        public void Run_Crash_ShowsExitCodeInHexadecimal()
        {
            string tool = WriteBatch("tool.bat", Lines("@echo off", "exit -1073741819"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.ExitCode, Is.EqualTo(-1073741819));
            Assert.That(result.Messages.Single().Message, Is.EqualTo("tool.bat failed with exit code -1073741819 = 0xC0000005."));
        }

        [Test]
        public void Run_MissingExecutable_IsAnErrorWithoutExitCode()
        {
            string tool = Path.Combine(_dir, "missing.exe");

            ToolProcessResult result = ToolProcess.Run(tool, "--help", null);

            Assert.That(result.Succeeded, Is.False);
            Assert.That(result.ExitCode, Is.Null);
            Assert.That(result.Output, Is.Empty);
            Assert.That(result.Messages, Has.Count.EqualTo(1));
            Assert.That(result.Messages[0], Is.InstanceOf<CompileError>());
            Assert.That(result.Messages[0].Message, Is.EqualTo("Cannot find the tool " + tool + "."));
        }

        [Test]
        public void Run_ExecutableThatCannotStart_IsAnErrorWithoutExitCode()
        {
            string tool = Path.Combine(_dir, "broken.exe");
            File.WriteAllText(tool, "not a program", Encoding.ASCII);

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.ExitCode, Is.Null);
            Assert.That(result.Messages.Single(), Is.InstanceOf<CompileError>());
            Assert.That(result.Messages[0].Message, Does.StartWith("Cannot start the tool " + tool + ": "));
        }

        [Test]
        public void Run_EmptyToolPath_Throws()
        {
            Assert.Throws<ArgumentException>(() => ToolProcess.Run(null, null, null));
            Assert.Throws<ArgumentException>(() => ToolProcess.Run("", null, null));
        }

        [Test]
        public void Run_WorkingDirectoryAndArguments_ReachTheTool()
        {
            string work = Path.Combine(_dir, "work dir");
            Directory.CreateDirectory(work);
            string tool = WriteBatch("tool.bat", Lines("@echo off", "echo %~1", "cd", "exit 0"));

            ToolProcessResult result = ToolProcess.Run(tool, BuildToolOptions.QuoteArgument("an argument"), work);

            Assert.That(result.Output, Is.EqualTo(Lines("an argument", work)));
        }

        [Test]
        public void Run_ToolReadingItsInput_GetsEndOfFile()
        {
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "set /p LINE=",
                "echo read [%LINE%]",
                "exit 0"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Succeeded, Is.True);
            Assert.That(result.Output, Is.EqualTo(Lines("read []")));
        }

        [Test]
        public void Run_Utf8Output_ArrivesIntact()
        {
            string word = "Caf\u00e9 \u65e5\u672c";
            // cmd reads the rest of the batch file, and writes its echo, in the code page set by chcp
            string tool = Path.Combine(_dir, "tool.bat");
            File.WriteAllText(tool, Lines(
                "@echo off",
                "chcp 65001 >nul",
                "echo Error: " + word,
                "exit 1"), Utf8);

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Output, Is.EqualTo(Lines("Error: " + word)));
            Assert.That(Describe(result.Messages), Is.EqualTo(new[] { "Error||0|" + word }));
        }

        [Test]
        public void Run_MegabytesOnBothStreams_DoNotBlockTheTool()
        {
            var big = new StringBuilder();
            for (int i = 0; big.Length < 4 * 1024 * 1024; i++)
            {
                big.Append("Packed file ").Append(i).Append(".dat: a line of output that is long enough to fill pipes\r\n");
            }
            WriteText("big.txt", big.ToString());
            string tool = WriteBatch("tool.bat", Lines(
                "@echo off",
                "type \"%~dp0big.txt\"",
                "type \"%~dp0big.txt\" 1>&2",
                "type \"%~dp0big.txt\"",
                "exit 0"));

            ToolProcessResult result = ToolProcess.Run(tool, null, null);

            Assert.That(result.Succeeded, Is.True);
            Assert.That(result.Messages, Is.Empty);
            Assert.That(result.Output.Length, Is.EqualTo(big.Length * 3));
            Assert.That(result.Output, Is.EqualTo(big.ToString() + big + big));
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
