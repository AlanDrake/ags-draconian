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
    /// BuildToolRunner with the real agsbuild, building a real project.
    ///
    /// The tools are taken from the build tree next to the tests' (Solutions/.build/x86/Release
    /// for Solutions/.test/x86/Release), or from the folder in AGS_TOOLS_DIR. The project is the
    /// folder (or Game.agf) in AGS_TEST_PROJECT: the auto-test game of the test games repository
    /// builds with the tools (Tools/test/data has only its Game.agf). Without either, the tests
    /// are ignored. The project is copied, so that it is not changed.
    /// </summary>
    [TestFixture]
    [Timeout(120000)]
    public class AgsbuildRunTests
    {
        private const string ToolsVariable = "AGS_TOOLS_DIR";
        private const string ProjectVariable = "AGS_TEST_PROJECT";
        private const string ApiHeader = @"Editor\AGS.Editor\Resources\agsdefns.sh";

        private string _toolsDir;
        private string _sourceProject;
        private string _apiHeader;
        private string _dir;

        [SetUp]
        public void SetUp()
        {
            _toolsDir = FindTools();
            if (_toolsDir == null)
            {
                Assert.Ignore("agsbuild.exe is neither in " + DefaultToolsDir() + " nor in " + ToolsVariable +
                              ": build Solutions/Tools.sln, or set " + ToolsVariable + ".");
            }
            _sourceProject = FindProject();
            if (_sourceProject == null)
            {
                Assert.Ignore("Set " + ProjectVariable + " to a game project that builds with the tools, such as " +
                              "the auto-test game of the test games repository.");
            }
            _apiHeader = Path.Combine(FindRepositoryRoot(), ApiHeader);
            // With a space, so that the paths need quoting
            _dir = Path.Combine(Path.GetTempPath(), "AGS.Editor.Tests", "agsbuild run " + Guid.NewGuid().ToString("N"));
            Directory.CreateDirectory(_dir);
        }

        [TearDown]
        public void TearDown()
        {
            if (_dir == null)
            {
                return;
            }
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
        public void Run_Project_BuildsThePackage()
        {
            BuildToolOptions options = CopyProject();
            var reports = new List<BuildToolProgress>();

            BuildToolResult result = new BuildToolRunner().Run(options, reports.Add, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Success), Summary(result));
            Assert.That(result.ExitCode, Is.EqualTo(0));
            Assert.That(result.Errors, Is.EqualTo(0));
            Assert.That(result.Messages.HasErrors, Is.False, Summary(result));
            Assert.That(Directory.GetFiles(options.OutputDirectory, "*.ags"), Is.Not.Empty, "the game package");
            Assert.That(reports, Is.Not.Empty);
            BuildToolProgress last = reports.Last();
            Assert.That(last.Total, Is.GreaterThan(0));
            Assert.That(last.Done, Is.EqualTo(last.Total));
            Assert.That(reports.Any(r => r.Task != null && r.Task.StartsWith("agscc:", StringComparison.Ordinal)), Is.True);
        }

        [Test]
        public void Run_ScriptWithSyntaxError_FailsWithErrorAtTheLine()
        {
            BuildToolOptions options = CopyProject();
            string script = Path.Combine(Path.GetDirectoryName(options.ProjectFile), "GlobalScript.asc");
            Assume.That(File.Exists(script), "the project has GlobalScript.asc");
            int line = AppendLine(script, "this is not valid;");

            BuildToolResult result = new BuildToolRunner().Run(options, null, CancellationToken.None);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Failed), Summary(result));
            Assert.That(result.ExitCode, Is.EqualTo(-1));
            Assert.That(result.Errors, Is.GreaterThan(0));
            CompileError error = result.Messages.Errors.FirstOrDefault(e => e.ScriptName == "GlobalScript.asc");
            Assert.That(error, Is.Not.Null, Summary(result));
            Assert.That(error.LineNumber, Is.EqualTo(line));
        }

        [Test]
        public void Run_CancelledAfterTheStart_EndsCancelledWithNoProcessLeft()
        {
            BuildToolOptions options = CopyProject();
            ProcessSnapshot before = ProcessSnapshot.Take();
            var cancel = new CancellationTokenSource();
            var sinceCancel = new Stopwatch();

            BuildToolResult result = new BuildToolRunner().Run(options, p =>
            {
                if (!cancel.IsCancellationRequested)
                {
                    sinceCancel.Start();
                    cancel.Cancel();
                }
            }, cancel.Token);

            Assert.That(result.Outcome, Is.EqualTo(BuildToolOutcome.Cancelled), Summary(result));
            Assert.That(result.ExitCode, Is.EqualTo(2), "agsbuild ended on its own, not killed");
            Assert.That(sinceCancel.Elapsed, Is.LessThan(BuildToolRunner.DefaultCancelGracePeriod));
            // Any process from the tools folder, also one whose parent has exited
            List<string> left = null;
            bool none = ProcessSnapshot.WaitFor(() =>
            {
                left = ProcessSnapshot.Take().NewSince(before)
                    .Where(p => IsInFolder(ProcessSnapshot.ImagePath(p.Id), _toolsDir))
                    .Select(p => p.ExeFile + " (" + p.Id + ")").ToList();
                return left.Count == 0;
            }, 3000);
            Assert.That(none, Is.True, "processes left running: " + string.Join(", ", left));
        }

        // A copy of the project, and the options to build it into a folder next to the copy
        private BuildToolOptions CopyProject()
        {
            string projectDir = Path.Combine(_dir, "My Game");
            CopyDirectory(Path.GetDirectoryName(_sourceProject), projectDir);
            string outputDir = Path.Combine(_dir, "Compiled Data");
            Directory.CreateDirectory(outputDir);
            return new BuildToolOptions
            {
                ProjectFile = Path.Combine(projectDir, Path.GetFileName(_sourceProject)),
                OutputDirectory = outputDir,
                ToolsDirectory = _toolsDir,
                ApiHeader = _apiHeader,
            };
        }

        // Appends a line to a text file, as bytes; returns its line number
        private static int AppendLine(string path, string text)
        {
            var bytes = new List<byte>(File.ReadAllBytes(path));
            if (bytes.Count > 0 && bytes[bytes.Count - 1] != '\n')
            {
                bytes.AddRange(Encoding.ASCII.GetBytes("\r\n"));
            }
            int line = bytes.Count(b => b == '\n') + 1;
            bytes.AddRange(Encoding.ASCII.GetBytes(text + "\r\n"));
            File.WriteAllBytes(path, bytes.ToArray());
            return line;
        }

        private static void CopyDirectory(string source, string target)
        {
            Directory.CreateDirectory(target);
            foreach (string file in Directory.GetFiles(source))
            {
                File.Copy(file, Path.Combine(target, Path.GetFileName(file)));
            }
            foreach (string dir in Directory.GetDirectories(source))
            {
                CopyDirectory(dir, Path.Combine(target, Path.GetFileName(dir)));
            }
        }

        private static bool IsInFolder(string path, string folder)
        {
            if (path == null)
            {
                return false;
            }
            string dir = Path.GetFullPath(Path.GetDirectoryName(path)).TrimEnd('\\');
            return string.Equals(dir, Path.GetFullPath(folder).TrimEnd('\\'), StringComparison.OrdinalIgnoreCase);
        }

        private static string Summary(BuildToolResult result)
        {
            IEnumerable<string> messages = result.Messages.Select(m =>
                m.GetType().Name + " " + m.ScriptName + ":" + m.LineNumber + " " + m.Message);
            string log = result.Log.Length > 4000 ? "..." + result.Log.Substring(result.Log.Length - 4000) : result.Log;
            return "exit code " + result.ExitCode + Environment.NewLine + string.Join(Environment.NewLine, messages) +
                   Environment.NewLine + log;
        }

        private static string DefaultToolsDir()
        {
            return Path.GetFullPath(Path.Combine(TestContext.CurrentContext.TestDirectory, @"..\..\..\.build\x86\Release"));
        }

        private static string FindTools()
        {
            foreach (string dir in new[] { DefaultToolsDir(), Environment.GetEnvironmentVariable(ToolsVariable) })
            {
                if (!string.IsNullOrEmpty(dir) && File.Exists(Path.Combine(dir, BuildToolRunner.ToolFileName)))
                {
                    return Path.GetFullPath(dir);
                }
            }
            return null;
        }

        // The project's Game.agf, or null
        private static string FindProject()
        {
            string path = Environment.GetEnvironmentVariable(ProjectVariable);
            if (string.IsNullOrEmpty(path))
            {
                return null;
            }
            if (Directory.Exists(path))
            {
                path = Path.Combine(path, "Game.agf");
            }
            if (!File.Exists(path))
            {
                Assert.Fail(ProjectVariable + " names no game project: " + path);
            }
            return Path.GetFullPath(path);
        }

        private static string FindRepositoryRoot()
        {
            string dir = TestContext.CurrentContext.TestDirectory;
            while (!string.IsNullOrEmpty(dir))
            {
                if (File.Exists(Path.Combine(dir, ApiHeader)))
                {
                    return dir;
                }
                dir = Path.GetDirectoryName(dir);
            }
            Assert.Fail("Cannot find " + ApiHeader + " above " + TestContext.CurrentContext.TestDirectory);
            return null;
        }
    }
}
