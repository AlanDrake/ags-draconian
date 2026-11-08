using System;
using AGS.Types;
using NUnit.Framework;

namespace AGS.Editor
{
    // The Game.agf version attributes that the editor opens: AGS 4.0 projects (from the open-format
    // rooms of 3.99.99.01) up to this editor's own format
    [TestFixture]
    public class ProjectVersionTests
    {
        private const string TooOldMessage = "AGS Draconian opens projects saved by AGS 4.0 or later; " +
            "open and save it with the AGS 4.0 editor first.";

        // Version and index as the editors write them: 3.0.3.2 to 4.0.0.19 froze the version and wrote an index
        [TestCase("2.80.1", null, null)]
        [TestCase("3.0.3.2", 15, null)]                 // 3.4.1.2
        [TestCase("3.0.3.2", 27, "3.5.1.0")]
        [TestCase("3.0.3.2", 3060316, "3.6.3.16")]
        [TestCase("3.0.3.2", 3999900, "3.99.99.0")]     // the last one with room*.crm files
        [TestCase("3.0.3.2", null, null)]               // no index: derived from the version
        public void OlderThan400IsRefused(string version, int? versionIndex, string editorVersion)
        {
            var ex = Assert.Throws<AGSEditorException>(
                () => ProjectVersion.Check(new System.Version(version), versionIndex, editorVersion));
            StringAssert.Contains(TooOldMessage, ex.Message);
            string savedBy = editorVersion != null ? $"saved by AGS {editorVersion}." : "older than 3.5.0.22.";
            StringAssert.Contains(savedBy, ex.Message);
        }

        [TestCase("3.0.3.2", 3999901, "3.99.99.1", ExpectedResult = 3999901)] // open-format rooms
        [TestCase("3.0.3.2", 4000019, "4.0.0.19", ExpectedResult = 4000019)]
        [TestCase("4.0.0.20", null, "4.0.0.20", ExpectedResult = 4000020)]    // no index any more
        [TestCase(AGSEditor.LATEST_XML_VERSION, null, AGSEditor.LATEST_XML_VERSION, ExpectedResult = 4000034)]
        public int From400IsOpened(string version, int? versionIndex, string editorVersion)
        {
            return ProjectVersion.Check(new System.Version(version), versionIndex, editorVersion);
        }

        [TestCase("4.0.0.99", null, "4.0.0.99")]
        [TestCase("3.0.3.2", 4000020, "4.0.0.20")] // an index past the last one written
        public void NewerThanEditorIsRefused(string version, int? versionIndex, string editorVersion)
        {
            var ex = Assert.Throws<AGSEditorException>(
                () => ProjectVersion.Check(new System.Version(version), versionIndex, editorVersion));
            StringAssert.Contains("Please check the AGS website for a newer version of the editor.", ex.Message);
        }
    }
}
