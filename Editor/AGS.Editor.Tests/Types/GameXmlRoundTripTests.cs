using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml;

namespace AGS.Types
{
    /// <summary>
    /// Contract C5 (docs/refactor/PLAN.md): the project format of Game.agf. A project that was saved
    /// by an editor of this format must load and save again as the same XML, or the editor would
    /// change (or lose) the data of existing projects.
    ///
    /// The sample Tools/test/data/Game.agf is a small project written by the editor. This test
    /// loads it with Game.FromXml and writes it with Game.ToXml, and compares the two &lt;Game&gt;
    /// elements. What it leaves out: the attributes of the root element, and the sections that the
    /// editor's components write after the &lt;Game&gt; element (&lt;Component&gt;), which are not part
    /// of the Game class; and the value of the setting BuildTargets, which the editor limits to the
    /// build targets that are registered in it (there are none in a test).
    ///
    /// If the editor writes the project differently on purpose (a new property, a new format
    /// version), regenerate the sample: run this test with AGS_UPDATE_GAME_SAMPLE=1 in the
    /// environment, and review the diff. The C++ test AGFReaderSample (Tools/test) then has to
    /// read the new sample without any unknown element.
    /// </summary>
    [TestFixture]
    public class GameXmlRoundTripTests
    {
        private const string SampleFile = @"Tools\test\data\Game.agf";
        private const string UpdateVariable = "AGS_UPDATE_GAME_SAMPLE";

        [TearDown]
        public void TearDown()
        {
            // the schema is shared by all the games of the process
            InteractionSchema.Instance.Events = new InteractionEvent[0];
        }

        private static string FindSampleFile()
        {
            string dir = TestContext.CurrentContext.TestDirectory;
            while (!string.IsNullOrEmpty(dir))
            {
                if (Directory.Exists(Path.Combine(dir, @"Tools\test")))
                    return Path.Combine(dir, SampleFile);
                dir = Path.GetDirectoryName(dir);
            }
            Assert.Fail("Cannot find " + SampleFile + " above " + TestContext.CurrentContext.TestDirectory);
            return null;
        }

        private static string WriteIndented(Action<XmlTextWriter> write)
        {
            var text = new StringWriter();
            var writer = new XmlTextWriter(text);
            writer.Formatting = Formatting.Indented;
            write(writer);
            writer.Flush();
            return text.ToString().Replace("\r\n", "\n");
        }

        // Loads the sample into a Game, as the editor does when it opens a project
        private static Game Load(XmlDocument doc)
        {
            XmlElement root = doc.DocumentElement;
            var version = new System.Version(root.GetAttribute("Version"));
            var game = new Game();
            game.SavedXmlVersion = version;
            game.SavedXmlVersionIndex = 0;
            game.SavedXmlEditorVersion = new System.Version(root.GetAttribute("EditorVersion"));
            game.FromXml(root);
            // The editor makes the interaction events from the cursors after it loads the game; the
            // events of characters and inventory items are written with the help of that schema
            AGS.Editor.Tasks.SyncCursorsWithInteractionSchema(game.Cursors, InteractionSchema.Instance);
            return game;
        }

        private static XmlDocument LoadDocument(string path)
        {
            var doc = new XmlDocument();
            doc.Load(path);
            return doc;
        }

        private static void IgnoreBuildTargets(XmlElement game)
        {
            XmlElement targets = game["Settings"]["BuildTargets"];
            Assert.IsNotNull(targets, "<BuildTargets> is not in the settings");
            targets.InnerText = string.Empty;
        }

        private static string FirstDifferences(string expected, string actual)
        {
            string[] a = expected.Split('\n');
            string[] b = actual.Split('\n');
            var report = new StringBuilder();
            int shown = 0;
            for (int i = 0; i < Math.Max(a.Length, b.Length) && shown < 8; ++i)
            {
                string left = i < a.Length ? a[i] : "<end>";
                string right = i < b.Length ? b[i] : "<end>";
                if (left != right)
                {
                    report.AppendFormat("line {0}:\n  sample: {1}\n  saved:  {2}\n", i + 1, left.Trim(), right.Trim());
                    ++shown;
                }
            }
            return report.ToString();
        }

        [Test]
        public void SampleIsInTheCurrentFormat()
        {
            XmlDocument doc = LoadDocument(FindSampleFile());
            Assert.AreEqual("AGSEditorDocument", doc.DocumentElement.Name);
            Assert.AreEqual(AGS.Editor.AGSEditor.LATEST_XML_VERSION, doc.DocumentElement.GetAttribute("Version"),
                "The project format version changed. Regenerate the sample (see the summary of this class).");
        }

        [Test]
        public void SampleLoadsAndSavesAsTheSameXml()
        {
            string path = FindSampleFile();
            XmlDocument doc = LoadDocument(path);
            Game game = Load(doc);
            var savedDoc = new XmlDocument();
            savedDoc.LoadXml(WriteIndented(w => game.ToXml(w)));
            // The editor keeps only the build targets that are registered in it, and none is in a test
            IgnoreBuildTargets(doc.DocumentElement["Game"]);
            IgnoreBuildTargets(savedDoc.DocumentElement);
            string saved = WriteIndented(w => savedDoc.DocumentElement.WriteTo(w));

            if (Environment.GetEnvironmentVariable(UpdateVariable) == "1")
            {
                // keeps the root element and the components as they are in the file
                XmlElement oldGame = doc.DocumentElement["Game"];
                var newGame = new XmlDocument();
                newGame.LoadXml(saved);
                doc.DocumentElement.ReplaceChild(doc.ImportNode(newGame.DocumentElement, true), oldGame);
                doc.DocumentElement.SetAttribute("Version", AGS.Editor.AGSEditor.LATEST_XML_VERSION);
                var settings = new XmlWriterSettings { Indent = true, Encoding = new UTF8Encoding(false) };
                using (XmlWriter w = XmlWriter.Create(path, settings))
                    doc.Save(w);
                Assert.Inconclusive(SampleFile + " was rewritten.");
            }

            string expected = WriteIndented(w => doc.DocumentElement["Game"].WriteTo(w));
            if (expected != saved)
            {
                Assert.Fail("The editor does not save the sample as it loaded it. If the new XML is intended, "
                    + "regenerate the sample: run this test with " + UpdateVariable + "=1.\n"
                    + FirstDifferences(expected, saved));
            }
        }

        [Test]
        public void SampleKeepsWhatItHolds()
        {
            // The round trip would also pass for a sample that lost its contents on load and save
            // alike; so check that the data of every kind of entity is in the loaded game.
            Game game = Load(LoadDocument(FindSampleFile()));
            Assert.Greater(game.Characters.Count, 0, "characters");
            Assert.Greater(game.InventoryItems.Count, 0, "inventory items");
            Assert.Greater(game.Cursors.Count, 0, "cursors");
            Assert.Greater(game.Fonts.Count, 0, "fonts");
            Assert.Greater(game.GUIs.Count, 0, "GUIs");
            Assert.Greater(game.ViewFlatList.Count, 0, "views");
            Assert.Greater(game.GlobalVariables.ToList().Count, 0, "global variables");
            Assert.Greater(game.PropertySchema.PropertyDefinitions.Count, 0, "custom properties");
            Assert.Greater(game.AudioClips.Count, 0, "audio clips");
            Assert.Greater(game.AudioClipTypes.Count, 0, "audio clip types");
            Assert.Greater(game.Dialogs.Count, 0, "dialogs");
            Assert.Greater(game.Rooms.Count, 0, "rooms");
            Assert.Greater(game.RootScriptFolder.AllItemsFlat.Count(), 0, "scripts");
            Assert.Greater(game.RootSpriteFolder.GetAllSpritesFromAllSubFolders().Count, 0, "sprites");
        }
    }
}
