using System.IO;
using System.Text;
using System.Xml;
using NUnit.Framework;

namespace AGS.Types
{
    /// <summary>
    /// The WorkspaceState section of Game.agf.user. The deserializer throws on an element without a
    /// property, so a property that stops being used has to stay readable: a user file written by an
    /// older editor must still load, and the next save must drop the element.
    /// </summary>
    [TestFixture]
    public class WorkspaceStateTests
    {
        private const string UserDataWithRequiredRebuildTime =
            "<AGSEditorUserData Version=\"4.0.0.33\">" +
            "<WorkspaceState>" +
            "<LastBuildConfiguration>Release</LastBuildConfiguration>" +
            "<LastBuildGameFileName>Windows=game,</LastBuildGameFileName>" +
            "<RequiredRebuildTime>2026-11-07</RequiredRebuildTime>" +
            "<RequiresRebuild>True</RequiresRebuild>" +
            "</WorkspaceState>" +
            "</AGSEditorUserData>";

        private static XmlNode Parse(string xml)
        {
            XmlDocument doc = new XmlDocument();
            doc.LoadXml(xml);
            return doc.DocumentElement;
        }

        private static string Serialize(WorkspaceState state)
        {
            StringBuilder sb = new StringBuilder();
            using (XmlTextWriter writer = new XmlTextWriter(new StringWriter(sb)))
            {
                state.ToXml(writer);
            }
            return sb.ToString();
        }

        [Test]
        public void FromXml_ReadsUserDataWithRequiredRebuildTime()
        {
            WorkspaceState state = new WorkspaceState();

            state.FromXml(Parse(UserDataWithRequiredRebuildTime));

            Assert.AreEqual(BuildConfiguration.Release, state.LastBuildConfiguration);
            Assert.AreEqual("Windows=game,", state.LastBuildGameFileName);
            Assert.IsTrue(state.RequiresRebuild);
        }

        [Test]
        public void ToXml_DoesNotWriteRequiredRebuildTime()
        {
            WorkspaceState state = new WorkspaceState();
            state.FromXml(Parse(UserDataWithRequiredRebuildTime));

            string xml = Serialize(state);

            Assert.That(xml, Does.Contain("<WorkspaceState>"));
            Assert.That(xml, Does.Contain("<RequiresRebuild>True</RequiresRebuild>"));
            Assert.That(xml, Does.Not.Contain("RequiredRebuildTime"));
        }

        [Test]
        public void FromXml_WithoutWorkspaceStateSection_KeepsDefaults()
        {
            WorkspaceState state = new WorkspaceState();

            state.FromXml(Parse("<AGSEditorUserData Version=\"4.0.0.33\" />"));

            Assert.AreEqual(BuildConfiguration.Unknown, state.LastBuildConfiguration);
            Assert.IsFalse(state.RequiresRebuild);
        }
    }
}
