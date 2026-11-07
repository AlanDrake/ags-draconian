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
    /// TemplatePackage with the real agspak and crmbuild.
    ///
    /// The tools are taken from the build tree next to the tests' (Solutions/.build/x86/Release
    /// for Solutions/.test/x86/Release), or from the folder in AGS_TOOLS_DIR; without them the
    /// tests are ignored. Building a room file from a real project needs the folder (or Game.agf)
    /// in AGS_TEST_PROJECT, such as the auto-test game of the test games repository; the project
    /// is copied, so that it is not changed.
    /// </summary>
    [TestFixture]
    [Timeout(120000)]
    public class TemplatePackageTests
    {
        private const string ToolsVariable = "AGS_TOOLS_DIR";
        private const string ProjectVariable = "AGS_TEST_PROJECT";
        // An AGS room file begins with -1 as an old 16-bit version, then this (Common/game/room_file.cpp)
        private const string RoomFileSignature = "AGS Room File v2";

        private string _toolsDir;
        private string _dir;

        [SetUp]
        public void SetUp()
        {
            _toolsDir = FindTools();
            if (_toolsDir == null)
            {
                Assert.Ignore(TemplatePackage.PackToolFileName + " and " + TemplatePackage.RoomToolFileName +
                              " are neither in " + DefaultToolsDir() + " nor in " + ToolsVariable +
                              ": build Solutions/Tools.sln, or set " + ToolsVariable + ".");
            }
            // With a space and letters outside ASCII, so that the paths need quoting and Unicode
            _dir = Path.Combine(Path.GetTempPath(), "AGS.Editor.Tests", "template caf\u00e9 " + Guid.NewGuid().ToString("N"));
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
        public void Pack_Assets_StoresThemUnderTheirNames()
        {
            var assets = new Dictionary<string, string>
            {
                { "Game.agf", WriteSource("game.xml", Encoding.UTF8.GetBytes("<Game>caf\u00e9 \u65e5\u672c</Game>")) },
                { "Rooms/1/data.xml", WriteSource("room data.xml", Encoding.UTF8.GetBytes("<Room>\u00fc</Room>")) },
                { "sub/Dir/x.bin", WriteSource("x.bin", Enumerable.Range(0, 256).Select(i => (byte)i).ToArray()) },
                { "sub/Caf\u00e9 \u65e5\u672c.txt", WriteSource("other.txt", Encoding.UTF8.GetBytes("\u00e9\u00e8")) },
            };
            string package = Path.Combine(_dir, "My Template.agt");
            string staging = Path.Combine(_dir, "staging");

            CompileMessages messages = TemplatePackage.Pack(_toolsDir, package, assets, staging);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(File.Exists(package), Is.True);
            Assert.That(Directory.Exists(staging), Is.False, "the staging folder is removed");
            Assert.That(ListPackage(package), Is.EquivalentTo(assets.Keys));
            string extracted = Path.Combine(_dir, "extracted");
            Directory.CreateDirectory(extracted);
            ToolProcessResult export = ToolProcess.Run(PackTool(),
                "-e " + BuildToolOptions.QuoteArgument(package) + " " + BuildToolOptions.QuoteArgument(extracted), null);
            Assert.That(export.Succeeded, Is.True, export.Output);
            foreach (KeyValuePair<string, string> asset in assets)
            {
                string file = Path.Combine(extracted, asset.Key.Replace('/', '\\'));
                Assert.That(File.Exists(file), Is.True, asset.Key);
                Assert.That(File.ReadAllBytes(file), Is.EqualTo(File.ReadAllBytes(asset.Value)), asset.Key);
            }
        }

        [Test]
        public void Pack_IntoStagingFolderInUse_LeavesItsOtherFiles()
        {
            string staging = Path.Combine(_dir, "staging");
            Directory.CreateDirectory(staging);
            string other = Path.Combine(staging, "room1.crm");
            File.WriteAllText(other, "kept");
            var assets = new[] { new KeyValuePair<string, string>("room1.crm", other) };

            CompileMessages messages = TemplatePackage.Pack(_toolsDir, Path.Combine(_dir, "room.art"), assets, staging);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(Directory.GetFileSystemEntries(staging), Is.EqualTo(new[] { other }));
        }

        [Test]
        public void Pack_MissingSource_IsAnErrorAndLeavesNoPackage()
        {
            string package = Path.Combine(_dir, "old.agt");
            File.WriteAllText(package, "an old template");
            string missing = Path.Combine(_dir, "missing.txt");
            var assets = new Dictionary<string, string>
            {
                { "Game.agf", WriteSource("game.xml", Encoding.UTF8.GetBytes("<Game/>")) },
                { "missing.txt", missing },
            };
            string staging = Path.Combine(_dir, "staging");

            CompileMessages messages = TemplatePackage.Pack(_toolsDir, package, assets, staging);

            Assert.That(messages.HasErrors, Is.True);
            Assert.That(messages.FirstError.Message, Does.Contain(missing));
            Assert.That(File.Exists(package), Is.False);
            Assert.That(Directory.Exists(staging), Is.False);
        }

        [Test]
        public void Pack_DuplicateNameButForCase_Throws()
        {
            string source = WriteSource("a.txt", new byte[] { 1 });
            var assets = new[]
            {
                new KeyValuePair<string, string>("Sub/A.txt", source),
                new KeyValuePair<string, string>("sub/a.TXT", source),
            };

            Assert.Throws<ArgumentException>(() =>
                TemplatePackage.Pack(_toolsDir, Path.Combine(_dir, "t.agt"), assets, Path.Combine(_dir, "staging")));
            Assert.That(Directory.Exists(Path.Combine(_dir, "staging")), Is.False);
        }

        [TestCase("")]
        [TestCase("/abs.txt")]
        [TestCase("a//b.txt")]
        [TestCase("../up.txt")]
        [TestCase("a/./b.txt")]
        [TestCase("C:/x.txt")]
        [TestCase("a\\b.txt")]
        public void Pack_NameThatIsNotARelativePath_Throws(string name)
        {
            var assets = new[] { new KeyValuePair<string, string>(name, WriteSource("a.txt", new byte[] { 1 })) };

            Assert.Throws<ArgumentException>(() =>
                TemplatePackage.Pack(_toolsDir, Path.Combine(_dir, "t.agt"), assets, Path.Combine(_dir, "staging")));
        }

        [Test]
        public void Pack_EmptyArgument_Throws()
        {
            var assets = new KeyValuePair<string, string>[0];
            string package = Path.Combine(_dir, "t.agt");
            string staging = Path.Combine(_dir, "staging");

            Assert.Throws<ArgumentException>(() => TemplatePackage.Pack(null, package, assets, staging));
            Assert.Throws<ArgumentException>(() => TemplatePackage.Pack(_toolsDir, "", assets, staging));
            Assert.Throws<ArgumentException>(() => TemplatePackage.Pack(_toolsDir, package, null, staging));
            Assert.Throws<ArgumentException>(() => TemplatePackage.Pack(_toolsDir, package, assets, null));
            Assert.Throws<ArgumentException>(() => TemplatePackage.Pack(_toolsDir, package,
                new[] { new KeyValuePair<string, string>("a.txt", "") }, staging));
        }

        [Test]
        public void BuildRoomFile_Project_WritesTheRoomFile()
        {
            string sourceProject = FindProject();
            if (sourceProject == null)
            {
                Assert.Ignore("Set " + ProjectVariable + " to a game project that builds with the tools, such as " +
                              "the auto-test game of the test games repository.");
            }
            string projectDir = Path.Combine(_dir, "My Game");
            CopyDirectory(Path.GetDirectoryName(sourceProject), projectDir);
            string roomFile = Path.Combine(_dir, "out dir", "room1.crm");

            CompileMessages messages = TemplatePackage.BuildRoomFile(_toolsDir,
                Path.Combine(projectDir, Path.GetFileName(sourceProject)), 1, roomFile);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(File.Exists(roomFile), Is.True);
            byte[] bytes = File.ReadAllBytes(roomFile);
            Assert.That(bytes.Length, Is.GreaterThan(2 + RoomFileSignature.Length));
            Assert.That(bytes[0], Is.EqualTo(0xFF));
            Assert.That(bytes[1], Is.EqualTo(0xFF));
            Assert.That(Encoding.ASCII.GetString(bytes, 2, RoomFileSignature.Length), Is.EqualTo(RoomFileSignature));
        }

        [Test]
        public void BuildRoomFile_RoomWithoutSources_IsAnErrorAndLeavesNoFile()
        {
            string project = Path.Combine(_dir, "Game.agf");
            File.WriteAllText(project, "<AGSEditorDocument />");
            string roomFile = Path.Combine(_dir, "out", "room7.crm");
            Directory.CreateDirectory(Path.GetDirectoryName(roomFile));
            File.WriteAllText(roomFile, "an old room file");

            CompileMessages messages = TemplatePackage.BuildRoomFile(_toolsDir, project, 7, roomFile);

            Assert.That(messages.HasErrors, Is.True);
            Assert.That(File.Exists(roomFile), Is.False);
        }

        [Test]
        public void BuildRoomFile_WrongArgument_Throws()
        {
            string project = Path.Combine(_dir, "Game.agf");
            string roomFile = Path.Combine(_dir, "room1.crm");

            Assert.Throws<ArgumentException>(() => TemplatePackage.BuildRoomFile("", project, 1, roomFile));
            Assert.Throws<ArgumentException>(() => TemplatePackage.BuildRoomFile(_toolsDir, null, 1, roomFile));
            Assert.Throws<ArgumentException>(() => TemplatePackage.BuildRoomFile(_toolsDir, project, 1, ""));
            Assert.Throws<ArgumentOutOfRangeException>(() => TemplatePackage.BuildRoomFile(_toolsDir, project, -1, roomFile));
        }

        // The asset names that agspak lists in the package
        private List<string> ListPackage(string package)
        {
            ToolProcessResult list = ToolProcess.Run(PackTool(), "-l " + BuildToolOptions.QuoteArgument(package), null);
            Assert.That(list.Succeeded, Is.True, list.Output);
            // "* <name padded to 40>[<size>]"
            return list.Output.Split('\n')
                .Where(l => l.StartsWith("* ", StringComparison.Ordinal))
                .Select(l => l.Substring(2, l.LastIndexOf('[') - 2).TrimEnd(' '))
                .ToList();
        }

        private string PackTool()
        {
            return Path.Combine(_toolsDir, TemplatePackage.PackToolFileName);
        }

        private string WriteSource(string name, byte[] content)
        {
            string dir = Path.Combine(_dir, "sources");
            Directory.CreateDirectory(dir);
            string path = Path.Combine(dir, name);
            File.WriteAllBytes(path, content);
            return path;
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

        private static string Describe(CompileMessages messages)
        {
            return string.Join(Environment.NewLine, messages.Select(m => m.GetType().Name + " " + m.Message));
        }

        private static string DefaultToolsDir()
        {
            return Path.GetFullPath(Path.Combine(TestContext.CurrentContext.TestDirectory, @"..\..\..\.build\x86\Release"));
        }

        private static string FindTools()
        {
            foreach (string dir in new[] { DefaultToolsDir(), Environment.GetEnvironmentVariable(ToolsVariable) })
            {
                if (!string.IsNullOrEmpty(dir) && File.Exists(Path.Combine(dir, TemplatePackage.PackToolFileName)) &&
                    File.Exists(Path.Combine(dir, TemplatePackage.RoomToolFileName)))
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
    }
}
