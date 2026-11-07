using System;
using System.IO;
using System.Linq;
using System.Text;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor.Build
{
    /// <summary>
    /// TranslationCompiler with the real po2tra, taken from the build tree next to the tests'
    /// (Solutions/.build/x86/Release for Solutions/.test/x86/Release), or from the folder in
    /// AGS_TOOLS_DIR. Without it, the tests are ignored.
    /// </summary>
    [TestFixture]
    [Timeout(30000)]
    public class TranslationCompilerTests
    {
        private const string ToolsVariable = "AGS_TOOLS_DIR";
        private const string Signature = "AGSTranslation\0";
        // The key of the .tra file's strings (Tools/data/tra_utils.cpp)
        private const string StringKey = "Avis Durgan";
        private const string SmallSource =
            "msgid \"\"\n" +
            "msgstr \"\"\n" +
            "\"Content-Type: text/plain; charset=UTF-8\\n\"\n" +
            "\n" +
            "msgid \"Hello\"\n" +
            "msgstr \"Ciao\"\n";

        private static readonly Encoding Utf8 = new UTF8Encoding(false);
        private string _toolsDir;
        private string _dir;

        [SetUp]
        public void SetUp()
        {
            _toolsDir = FindTools();
            if (_toolsDir == null)
            {
                Assert.Ignore(TranslationCompiler.ToolFileName + " is neither in " + DefaultToolsDir() + " nor in " +
                              ToolsVariable + ": build Solutions/Tools.sln, or set " + ToolsVariable + ".");
            }
            _dir = Path.Combine(Path.GetTempPath(), "AGS.Editor.Tests", "TranslationCompiler-" + Guid.NewGuid().ToString("N"));
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
        public void Compile_SmallSource_WritesTheTraFileInANewFolder()
        {
            string poFile = WriteSource("Italian.po", SmallSource);
            string traFile = Path.Combine(_dir, "Compiled", "Data", "Italian.tra");

            CompileMessages messages = TranslationCompiler.Compile(_toolsDir, poFile, traFile, "Test game", 1234);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(File.Exists(traFile), Is.True);
            byte[] tra = File.ReadAllBytes(traFile);
            Assert.That(tra.Length, Is.GreaterThan(Signature.Length));
            Assert.That(tra.Take(Signature.Length).ToArray(), Is.EqualTo(Encoding.ASCII.GetBytes(Signature)));
        }

        [Test]
        public void Compile_MissingSource_GivesAnErrorNamingItAndNoTraFile()
        {
            string poFile = Path.Combine(_dir, "Missing.po");
            string traFile = Path.Combine(_dir, "Missing.tra");

            CompileMessages messages = TranslationCompiler.Compile(_toolsDir, poFile, traFile, "Test game", 1);

            Assert.That(messages.HasErrors, Is.True);
            string error = messages.OfType<CompileError>().First().Message;
            Assert.That(error, Does.Contain("Translation '" + poFile + "' failed to compile"));
            Assert.That(error, Does.Contain("failed to open source PO"));
            Assert.That(File.Exists(traFile), Is.False);
        }

        [Test]
        public void Compile_UnterminatedString_CompilesWithAWarning()
        {
            // po2tra leaves out what it cannot read as entries, here every one
            string poFile = WriteSource("Broken.po", "msgid \"Hello\nmsgstr \"Ciao\"\n");
            string traFile = Path.Combine(_dir, "Broken.tra");

            CompileMessages messages = TranslationCompiler.Compile(_toolsDir, poFile, traFile, "Test game", 1);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(messages.OfType<CompileWarning>().Select(m => m.Message),
                Has.Some.Contains("did not appear to have any translated lines"));
            Assert.That(File.Exists(traFile), Is.True);
        }

        [Test]
        public void Compile_OutputCannotBeWritten_GivesAnErrorNamingTheSource()
        {
            string poFile = WriteSource("Italian.po", SmallSource);
            // A folder where the file should be
            string traFile = Path.Combine(_dir, "Italian.tra");
            Directory.CreateDirectory(traFile);

            CompileMessages messages = TranslationCompiler.Compile(_toolsDir, poFile, traFile, "Test game", 1);

            Assert.That(messages.HasErrors, Is.True);
            string error = messages.OfType<CompileError>().First().Message;
            Assert.That(error, Does.Contain("Translation '" + poFile + "' failed to compile"));
            Assert.That(error, Does.Contain("failed to open output TRA"));
        }

        [Test]
        public void Compile_QuotesInTheGameNameAndNonAsciiPaths_ReachTheTool()
        {
            string folder = Path.Combine(_dir, "tradu\u00e7\u00e3o \u65e5\u672c");
            Directory.CreateDirectory(folder);
            string poFile = Path.Combine(folder, "Portugu\u00eas.po");
            File.WriteAllText(poFile, SmallSource, Utf8);
            string traFile = Path.Combine(folder, "out \u00e8", "Portugu\u00eas.tra");
            const string gameName = "A \"quoted\" g\u00e9me \\ with spaces\\";

            CompileMessages messages = TranslationCompiler.Compile(_toolsDir, poFile, traFile, gameName, -42);

            Assert.That(messages.HasErrors, Is.False, Describe(messages));
            Assert.That(File.Exists(traFile), Is.True);
            int uniqueId;
            string name;
            ReadGameId(File.ReadAllBytes(traFile), out uniqueId, out name);
            Assert.That(uniqueId, Is.EqualTo(-42));
            Assert.That(name, Is.EqualTo(gameName));
        }

        private string WriteSource(string name, string text)
        {
            string path = Path.Combine(_dir, name);
            File.WriteAllText(path, text, Utf8);
            return path;
        }

        // The game id block, which po2tra writes first: id 2 and a 32-bit size, the unique id, and the
        // name as a length and the bytes, each added to a byte of the key
        private static void ReadGameId(byte[] tra, out int uniqueId, out string name)
        {
            int pos = Signature.Length;
            Assert.That(BitConverter.ToInt32(tra, pos), Is.EqualTo(2), "the game id block");
            pos += 8;
            uniqueId = BitConverter.ToInt32(tra, pos);
            pos += 4;
            int length = BitConverter.ToInt32(tra, pos);
            pos += 4;
            byte[] key = Encoding.ASCII.GetBytes(StringKey);
            var bytes = new byte[length];
            for (int i = 0; i < length; i++)
            {
                bytes[i] = (byte)(tra[pos + i] - key[i % key.Length]);
            }
            name = Utf8.GetString(bytes);
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
                if (!string.IsNullOrEmpty(dir) && File.Exists(Path.Combine(dir, TranslationCompiler.ToolFileName)))
                {
                    return Path.GetFullPath(dir);
                }
            }
            return null;
        }
    }
}
