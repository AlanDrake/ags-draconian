using System.Text;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// BuildToolEvent.Parse on the lines of "agsbuild --json" (Tools/agsbuild/diagnostics.h).
    /// The well-formed lines were written by agsbuild building the auto-test game, with the
    /// paths in messages shortened.
    /// </summary>
    [TestFixture]
    public class BuildToolEventTests
    {
        [Test]
        public void Parse_Start_ReadsTaskCount()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(@"{""event"":""start"",""tasks"":62}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Kind, Is.EqualTo(BuildToolEventKind.Start));
            Assert.That(ev.Tasks, Is.EqualTo(62));
        }

        [TestCase("running", BuildToolTaskState.Running)]
        [TestCase("success", BuildToolTaskState.Success)]
        [TestCase("failed", BuildToolTaskState.Failed)]
        [TestCase("cancelled", BuildToolTaskState.Cancelled)]
        public void Parse_Task_ReadsNameAndState(string state, BuildToolTaskState expected)
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""task"",""task"":""crmpak:room1.crm:cut-old-scripts"",""state"":""" + state + @"""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Kind, Is.EqualTo(BuildToolEventKind.Task));
            Assert.That(ev.Task, Is.EqualTo("crmpak:room1.crm:cut-old-scripts"));
            Assert.That(ev.State, Is.EqualTo(expected));
        }

        [Test]
        public void Parse_Progress_ReadsDoneAndTotal()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(@"{""event"":""progress"",""done"":14,""total"":25}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Kind, Is.EqualTo(BuildToolEventKind.Progress));
            Assert.That(ev.Done, Is.EqualTo(14));
            Assert.That(ev.Total, Is.EqualTo(25));
        }

        [TestCase("success", BuildToolEndResult.Success)]
        [TestCase("failed", BuildToolEndResult.Failed)]
        [TestCase("cancelled", BuildToolEndResult.Cancelled)]
        public void Parse_End_ReadsResultAndCounts(string result, BuildToolEndResult expected)
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""end"",""result"":""" + result + @""",""errors"":10,""warnings"":3}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Kind, Is.EqualTo(BuildToolEventKind.End));
            Assert.That(ev.Result, Is.EqualTo(expected));
            Assert.That(ev.Errors, Is.EqualTo(10));
            Assert.That(ev.Warnings, Is.EqualTo(3));
        }

        [Test]
        public void Parse_DiagnosticWithFileAndLine_ReadsAllFields()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""warning"",""tool"":""agscc"",""task"":""agscc:__DialogScripts.asc""," +
                @"""file"":""Dialog 3"",""line"":23,""message"":""Code execution cannot reach this point""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Kind, Is.EqualTo(BuildToolEventKind.Diagnostic));
            Assert.That(ev.Severity, Is.EqualTo(BuildToolSeverity.Warning));
            Assert.That(ev.Tool, Is.EqualTo("agscc"));
            Assert.That(ev.Task, Is.EqualTo("agscc:__DialogScripts.asc"));
            Assert.That(ev.File, Is.EqualTo("Dialog 3"));
            Assert.That(ev.Line, Is.EqualTo(23));
            Assert.That(ev.Message, Is.EqualTo("Code execution cannot reach this point"));
        }

        [Test]
        public void Parse_DiagnosticWithEscapedBackslashes_ReadsFileName()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agscc"",""task"":""agscc:room1.asc""," +
                @"""file"":""Rooms\\1\\room1.asc"",""line"":188,""message"":""Cannot start a definition or declaration with 'this'""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Severity, Is.EqualTo(BuildToolSeverity.Error));
            Assert.That(ev.File, Is.EqualTo(@"Rooms\1\room1.asc"));
            Assert.That(ev.Line, Is.EqualTo(188));
            Assert.That(ev.Message, Is.EqualTo("Cannot start a definition or declaration with 'this'"));
        }

        [Test]
        public void Parse_DiagnosticWithoutFile_HasNullFileAndLineZero()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""info"",""tool"":""agspak"",""task"":""agspak:game.ags""," +
                @"""message"":""found pattern file 'c:/game/Compiled/_temp/agspak-include.files'.""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Severity, Is.EqualTo(BuildToolSeverity.Info));
            Assert.That(ev.Tool, Is.EqualTo("agspak"));
            Assert.That(ev.File, Is.Null);
            Assert.That(ev.Line, Is.EqualTo(0));
            Assert.That(ev.Message, Is.EqualTo("found pattern file 'c:/game/Compiled/_temp/agspak-include.files'."));
        }

        [Test]
        public void Parse_DiagnosticOutsideTasks_HasEmptyTask()
        {
            // What agsbuild reports for a failure before the build (main.cpp, Fail)
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agsbuild"",""task"":""""," +
                @"""message"":""input project file 'c:/game/Game.agf' not found or not a valid file""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Task, Is.Empty);
            Assert.That(ev.File, Is.Null);
            Assert.That(ev.Message, Is.EqualTo("input project file 'c:/game/Game.agf' not found or not a valid file"));
        }

        [Test]
        public void Parse_DiagnosticWithEmptyMessage_HasEmptyMessage()
        {
            // agsbuild writes this for a task that failed without saying why (taskmanager.cpp, ReportFinishedTask)
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agsbuild"",""task"":""copy-rooms"",""message"":""""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Task, Is.EqualTo("copy-rooms"));
            Assert.That(ev.Message, Is.Empty);
        }

        [Test]
        public void Parse_DiagnosticWithMissingFields_HasDefaults()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(@"{""event"":""diagnostic"",""severity"":""error""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Tool, Is.Empty);
            Assert.That(ev.Task, Is.Empty);
            Assert.That(ev.File, Is.Null);
            Assert.That(ev.Line, Is.EqualTo(0));
            Assert.That(ev.Message, Is.Empty);
        }

        [TestCase(@"""line"":-4", 0)]
        [TestCase(@"""line"":""12""", 0)]
        [TestCase(@"""line"":12.5", 0)]
        [TestCase(@"""line"":99999999999", 0)]
        [TestCase(@"""line"":null", 0)]
        [TestCase(@"""line"":2147483647", int.MaxValue)]
        public void Parse_DiagnosticLineOutOfRange_IsZero(string lineField, int expected)
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""error"",""file"":""a.asc""," + lineField + @",""message"":""m""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Line, Is.EqualTo(expected));
        }

        [Test]
        public void Parse_EscapedQuotesBackslashesAndControls_AreDecoded()
        {
            // The escapes that JsonString in Tools/agsbuild/diagnostics.cpp writes
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""error"",""tool"":""agscc"",""task"":""agscc:a.asc""," +
                @"""file"":""C:\\My \""Games\""\\a.asc"",""line"":3," +
                @"""message"":""expected \""(\"" after 'if'\ttab\nnew line\r\u0001\\""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.File, Is.EqualTo(@"C:\My ""Games""\a.asc"));
            Assert.That(ev.Message, Is.EqualTo("expected \"(\" after 'if'\ttab\nnew line\r\u0001\\"));
        }

        [Test]
        public void Parse_NonAsciiText_IsKept()
        {
            // agsbuild writes UTF-8 as it is; the runner decodes it before Parse
            string text = "Caf\u00e9 \u65e5\u672c\u8a9e \u2713 \U0001F600";
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""warning"",""tool"":""agscc"",""task"":""t""," +
                @"""file"":""" + text + @".asc"",""line"":1,""message"":""" + text + @"""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.File, Is.EqualTo(text + ".asc"));
            Assert.That(ev.Message, Is.EqualTo(text));
        }

        [Test]
        public void Parse_EscapedNonAscii_IsDecoded()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""info"",""message"":""Caf\u00e9 \ud83d\ude00""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Message, Is.EqualTo("Caf\u00e9 \U0001F600"));
        }

        [Test]
        public void Parse_MessageLikeADate_StaysText()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""diagnostic"",""severity"":""info"",""message"":""2026-11-06T10:00:00Z""}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Message, Is.EqualTo("2026-11-06T10:00:00Z"));
        }

        [Test]
        public void Parse_SurroundingWhitespace_IsAllowed()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(" \t{\"event\":\"start\",\"tasks\":1}\r");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Tasks, Is.EqualTo(1));
        }

        [Test]
        public void Parse_UnknownFields_AreIgnored()
        {
            BuildToolEvent ev = BuildToolEvent.Parse(
                @"{""event"":""progress"",""done"":1,""total"":2,""eta"":{""seconds"":[1,2]},""note"":null}");

            Assert.That(ev, Is.Not.Null);
            Assert.That(ev.Done, Is.EqualTo(1));
            Assert.That(ev.Total, Is.EqualTo(2));
        }

        [TestCase(null)]
        [TestCase("")]
        [TestCase("   ")]
        [TestCase("System dir: C:\\AGS")]
        [TestCase("Error: not enough arguments")]
        [TestCase("{")]
        [TestCase("}")]
        [TestCase("{\"event\":\"start\",\"tasks\":1")]
        [TestCase("{\"event\":\"start\",\"tasks\":1} trailing")]
        [TestCase("{\"event\":\"start\",\"tasks\":1}{\"event\":\"start\",\"tasks\":1}")]
        [TestCase("{\"event\":\"start\",\"tasks\":}")]
        [TestCase("{\"event\":\"diagnostic\",\"severity\":\"error\",\"message\":\"unterminated}")]
        [TestCase("{\"event\":\"diagnostic\",\"severity\":\"error\",\"message\":\"bad escape \\x\"}")]
        [TestCase("[{\"event\":\"start\",\"tasks\":1}]")]
        [TestCase("\"start\"")]
        [TestCase("42")]
        [TestCase("null")]
        [TestCase("{}")]
        [TestCase("{\"tasks\":1}")]
        [TestCase("{\"event\":null}")]
        [TestCase("{\"event\":5}")]
        [TestCase("{\"event\":\"START\",\"tasks\":1}")]
        [TestCase("{\"event\":\"bogus\"}")]
        [TestCase("{\"event\":\"task\",\"task\":\"a\",\"state\":\"paused\"}")]
        [TestCase("{\"event\":\"task\",\"task\":\"a\"}")]
        [TestCase("{\"event\":\"diagnostic\",\"severity\":\"fatal\",\"message\":\"m\"}")]
        [TestCase("{\"event\":\"diagnostic\",\"message\":\"m\"}")]
        [TestCase("{\"event\":\"end\",\"result\":\"maybe\"}")]
        [TestCase("{\"event\":\"end\"}")]
        public void Parse_NotAnEvent_ReturnsNull(string line)
        {
            Assert.That(BuildToolEvent.Parse(line), Is.Null);
        }

        [Test]
        public void Parse_DeeplyNested_ReturnsNull()
        {
            var line = new StringBuilder("{\"event\":\"start\",\"tasks\":1,\"x\":");
            line.Append('[', 10000);
            line.Append(']', 10000);
            line.Append('}');

            Assert.That(BuildToolEvent.Parse(line.ToString()), Is.Null);
        }

        [Test]
        public void Parse_BinaryGarbage_ReturnsNull()
        {
            var line = new StringBuilder("{");
            for (int i = 0; i < 2000; i++)
            {
                line.Append((char)((i * 7919) % 0xD7FF + 1));
            }

            Assert.That(BuildToolEvent.Parse(line.ToString()), Is.Null);
        }
    }
}
