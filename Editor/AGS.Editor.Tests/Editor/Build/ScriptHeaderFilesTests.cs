using System;
using System.IO;
using System.Linq;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// The plugin headers the editor writes out for agsbuild --header.
    /// </summary>
    [TestFixture]
    public class ScriptHeaderFilesTests
    {
        [Test]
        public void Constructor_WritesEachHeaderUnderItsFileNameInOrder()
        {
            Script[] headers =
            {
                new Script("__Plugin_b.ash", "import int B();", true),
                new Script("__Plugin_a.ash", "import int A(); // caf\u00e9", true),
            };

            using (var files = new ScriptHeaderFiles(headers))
            {
                Assert.That(files.Paths.Select(Path.GetFileName), Is.EqualTo(new[] { "__Plugin_b.ash", "__Plugin_a.ash" }));
                Assert.That(File.ReadAllText(files.Paths[0]), Is.EqualTo("import int B();"));
                // UTF-8 without a byte order mark
                byte[] bytes = File.ReadAllBytes(files.Paths[1]);
                Assert.That(bytes.Take(3), Is.Not.EqualTo(new byte[] { 0xEF, 0xBB, 0xBF }));
                Assert.That(bytes.Skip(bytes.Length - 2), Is.EqualTo(new byte[] { 0xC3, 0xA9 }));
            }
        }

        [Test]
        public void Constructor_KeepsHeadersOfTheSameName()
        {
            Script[] headers = { new Script("same.ash", "one", true), new Script("same.ash", "two", true) };

            using (var files = new ScriptHeaderFiles(headers))
            {
                Assert.That(files.Paths.Count, Is.EqualTo(2));
                Assert.That(files.Paths[0], Is.Not.EqualTo(files.Paths[1]));
                Assert.That(files.Paths.Select(File.ReadAllText), Is.EqualTo(new[] { "one", "two" }));
            }
        }

        [Test]
        public void Constructor_UsesTheFileNameOfAPath()
        {
            using (var files = new ScriptHeaderFiles(new[] { new Script(Path.Combine("sub", "x.ash"), "", true) }))
            {
                Assert.That(Path.GetFileName(files.Paths[0]), Is.EqualTo("x.ash"));
            }
        }

        [Test]
        public void Constructor_LeavesNothingBehindWhenAHeaderHasNoName()
        {
            int before = CountHeaderFolders();

            Assert.Throws<ArgumentException>(() => new ScriptHeaderFiles(new[]
            {
                new Script("good.ash", "", true),
                new Script("", "", true),
            }));
            Assert.That(CountHeaderFolders(), Is.EqualTo(before));
        }

        [Test]
        public void Dispose_DeletesTheFiles()
        {
            string folder;
            using (var files = new ScriptHeaderFiles(new[] { new Script("x.ash", "", true) }))
            {
                folder = Path.GetDirectoryName(Path.GetDirectoryName(files.Paths[0]));
                Assert.That(Directory.Exists(folder), Is.True);
            }
            Assert.That(Directory.Exists(folder), Is.False);
        }

        [Test]
        public void Dispose_ReportsAFileThatCannotBeDeleted()
        {
            var files = new ScriptHeaderFiles(new[] { new Script("x.ash", "", true) });
            using (File.Open(files.Paths[0], FileMode.Open, FileAccess.Read, FileShare.None))
            {
                files.Dispose();
                Assert.That(files.DeleteError, Does.Contain("Could not delete"));
            }
            files.Dispose();
            Assert.That(Directory.Exists(Path.GetDirectoryName(files.Paths[0])), Is.False);
        }

        [Test]
        public void Paths_IsEmptyWithoutHeaders()
        {
            using (var files = new ScriptHeaderFiles(new Script[0]))
            {
                Assert.That(files.Paths, Is.Empty);
            }
        }

        private static int CountHeaderFolders()
        {
            return Directory.GetDirectories(Path.GetTempPath(), "AGSEditor-headers-*").Length;
        }
    }
}
