using System;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// The command line of agsbuild, read back the way Windows programs read theirs
    /// (CommandLineToArgvW, whose rules the C runtime's argv follows).
    /// </summary>
    [TestFixture]
    public class BuildToolOptionsTests
    {
        private static readonly string[] QuotingCases =
        {
            "",
            "a",
            "a b",
            " leading",
            "trailing ",
            "  ",
            "a\tb",
            "a\nb",
            "\"",
            "\"\"",
            "a\"b",
            "a\"\"b",
            "\"quoted\"",
            "\\",
            "\\\\",
            "a\\b",
            "a\\\\b",
            "a\\",
            "a\\\\",
            "a b\\",
            "a b\\\\",
            "\\\"",
            "\\\\\"",
            "a\\\"b",
            "a\\\\\"b",
            "a \\\" b",
            "C:\\Program Files\\AGS\\",
            "C:\\Program Files\\AGS\\Game.agf",
            "\\\\server\\share\\My Game\\",
            "-x",
            "--json",
            "&|<>^%!()",
            "Caf\u00e9 \u65e5\u672c\u8a9e \U0001F600",
        };

        [TestCaseSource(nameof(QuotingCases))]
        public void QuoteArgument_ReadsBackAsTheSameArgument(string value)
        {
            string[] argv = SplitCommandLine("program.exe " + BuildToolOptions.QuoteArgument(value));

            Assert.That(argv, Is.EqualTo(new[] { "program.exe", value }));
        }

        [Test]
        public void QuoteArgument_AllCasesInOneCommandLine_ReadBackInOrder()
        {
            string commandLine = "program.exe " + string.Join(" ", QuotingCases.Select(BuildToolOptions.QuoteArgument));

            string[] argv = SplitCommandLine(commandLine);

            Assert.That(argv.Skip(1), Is.EqualTo(QuotingCases));
        }

        [TestCase("abc", "abc")]
        [TestCase("C:\\out\\", "C:\\out\\")]
        [TestCase("", "\"\"")]
        [TestCase("a b", "\"a b\"")]
        [TestCase("C:\\my out\\", "\"C:\\my out\\\\\"")]
        [TestCase("say \"hi\"", "\"say \\\"hi\\\"\"")]
        [TestCase("a\\\"b", "\"a\\\\\\\"b\"")]
        public void QuoteArgument_QuotesOnlyWhenNeeded(string value, string expected)
        {
            Assert.That(BuildToolOptions.QuoteArgument(value), Is.EqualTo(expected));
        }

        [Test]
        public void QuoteArgument_Null_Throws()
        {
            Assert.Throws<ArgumentNullException>(() => BuildToolOptions.QuoteArgument(null));
        }

        [Test]
        public void ToArguments_Minimal_HasPositionalArgumentsAndJson()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = @"C:\Games\My Game\Game.agf",
                OutputDirectory = @"C:\Games\My Game\Compiled\Data\",
            };

            string arguments = options.ToArguments();

            Assert.That(arguments, Is.EqualTo(@"""C:\Games\My Game\Game.agf"" ""C:\Games\My Game\Compiled\Data\\"" --json"));
            Assert.That(SplitArguments(arguments), Is.EqualTo(new[]
            {
                @"C:\Games\My Game\Game.agf", @"C:\Games\My Game\Compiled\Data\", "--json"
            }));
        }

        [Test]
        public void ToArguments_Full_HasEveryOptionInOrder()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = @"C:\Games\Game\Game.agf",
                OutputDirectory = @"C:\Games\Game\Compiled",
                ToolsDirectory = @"C:\Program Files\AGS",
                TempDirectory = @"D:\temp\build",
                ApiHeader = @"C:\Program Files\AGS\agsdefns.sh",
                DataOnly = true,
                Rebuild = true,
                Strict = true,
                Threads = 4,
            };
            options.ScriptHeaders.Add(@"C:\Games\Game\First.ash");
            options.ScriptHeaders.Add(@"C:\Games\Game\Second header.ash");

            string[] argv = SplitArguments(options.ToArguments());

            Assert.That(argv, Is.EqualTo(new[]
            {
                @"C:\Games\Game\Game.agf",
                @"C:\Games\Game\Compiled",
                "--json",
                "--sys-dir", @"C:\Program Files\AGS",
                "--temp-dir", @"D:\temp\build",
                "--api-ash", @"C:\Program Files\AGS\agsdefns.sh",
                "--header", @"C:\Games\Game\First.ash",
                "--header", @"C:\Games\Game\Second header.ash",
                "--data-only",
                "--rebuild",
                "--strict",
                "--threads", "4",
            }));
        }

        [Test]
        public void ToArguments_NonAsciiPathsWithSpaces_ReadBack()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = "C:\\Giochi\\citt\u00e0 \u65e5\u672c\\Game.agf",
                OutputDirectory = "C:\\Giochi\\citt\u00e0 \u65e5\u672c\\Compiled\\",
                ToolsDirectory = "C:\\\u0410\u0413\u0421 tools",
            };

            string[] argv = SplitArguments(options.ToArguments());

            Assert.That(argv, Is.EqualTo(new[]
            {
                "C:\\Giochi\\citt\u00e0 \u65e5\u672c\\Game.agf",
                "C:\\Giochi\\citt\u00e0 \u65e5\u672c\\Compiled\\",
                "--json",
                "--sys-dir", "C:\\\u0410\u0413\u0421 tools",
            }));
        }

        [Test]
        public void ToArguments_RelativePaths_AreMadeFull()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = "Game.agf",
                OutputDirectory = Path.Combine("Compiled", "Data"),
                TempDirectory = ".",
            };

            string[] argv = SplitArguments(options.ToArguments());

            Assert.That(argv, Is.EqualTo(new[]
            {
                Path.GetFullPath("Game.agf"),
                Path.GetFullPath(Path.Combine("Compiled", "Data")),
                "--json",
                "--temp-dir", Path.GetFullPath("."),
            }));
        }

        [Test]
        public void ToArguments_UnsetOptions_AreLeftOut()
        {
            var options = new BuildToolOptions
            {
                ProjectFile = @"C:\g\Game.agf",
                OutputDirectory = @"C:\g\out",
                ToolsDirectory = "",
                TempDirectory = null,
                Threads = null,
            };

            Assert.That(options.ToArguments(), Is.EqualTo(@"C:\g\Game.agf C:\g\out --json"));
        }

        [Test]
        public void ToArguments_MissingRequiredValue_Throws()
        {
            Assert.Throws<InvalidOperationException>(() => new BuildToolOptions { OutputDirectory = @"C:\out" }.ToArguments());
            Assert.Throws<InvalidOperationException>(() => new BuildToolOptions { ProjectFile = @"C:\g\Game.agf" }.ToArguments());
        }

        [Test]
        public void ToArguments_InvalidValue_Throws()
        {
            var noThreads = new BuildToolOptions { ProjectFile = @"C:\g\Game.agf", OutputDirectory = @"C:\out", Threads = 0 };
            var emptyHeader = new BuildToolOptions { ProjectFile = @"C:\g\Game.agf", OutputDirectory = @"C:\out" };
            emptyHeader.ScriptHeaders.Add("");

            Assert.Throws<InvalidOperationException>(() => noThreads.ToArguments());
            Assert.Throws<InvalidOperationException>(() => emptyHeader.ToArguments());
        }

        // The arguments as a program reads them, without its own name
        private static string[] SplitArguments(string arguments)
        {
            return SplitCommandLine("program.exe " + arguments).Skip(1).ToArray();
        }

        private static string[] SplitCommandLine(string commandLine)
        {
            int count;
            IntPtr argv = CommandLineToArgvW(commandLine, out count);
            if (argv == IntPtr.Zero)
            {
                throw new Win32Exception();
            }
            try
            {
                var args = new string[count];
                for (int i = 0; i < count; i++)
                {
                    args[i] = Marshal.PtrToStringUni(Marshal.ReadIntPtr(argv, i * IntPtr.Size));
                }
                return args;
            }
            finally
            {
                LocalFree(argv);
            }
        }

        [DllImport("shell32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern IntPtr CommandLineToArgvW(string commandLine, out int count);

        [DllImport("kernel32.dll")]
        private static extern IntPtr LocalFree(IntPtr memory);
    }
}
