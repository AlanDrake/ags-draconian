using System;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// Decides from the version attributes of a Game.agf whether this editor opens the project.
    /// </summary>
    public static class ProjectVersion
    {
        /// <summary>
        /// The oldest project format this editor opens: AGS 4.0's open-format rooms (3.99.99.01).
        /// An older project keeps its rooms as room*.crm files of a format that the room reader refuses
        /// (3.x, or version 40 of the 3.99 alphas).
        /// </summary>
        public const int LOWEST_SUPPORTED_VERSION_INDEX = AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_OPEN_ROOMS;

        /// <summary>
        /// The first editor that wrote its own version into Game.agf (version index 25).
        /// </summary>
        private const string FIRST_EDITOR_VERSION_WRITTEN = "3.5.0.22";

        /// <summary>
        /// Converts a version to the XXYYZZWW encoded index that the 3.6.0 to 4.0.0.19 editors wrote.
        /// </summary>
        public static int ToVersionIndex(System.Version version)
        {
            return Math.Min(version.Major, 99) * 1000000
                + Math.Min(version.Minor, 99) * 10000
                + Math.Min(version.Build, 99) * 100
                + Math.Min(version.Revision, 99);
        }

        /// <summary>
        /// Checks the version attributes of a Game.agf: the format version, the version index
        /// (written by the 3.0.3.2 to 4.0.0.19 editors, null if absent) and the editor's version
        /// (null if absent). Returns the project's version index: the one written, or else the one
        /// derived from the format version. Throws AGSEditorException for a project newer than this
        /// editor, or older than AGS 4.0.
        /// </summary>
        public static int Check(System.Version primaryVersion, int? versionIndex, string savedWithEditorVersion)
        {
            bool usesIndex = primaryVersion >= new System.Version(AGSEditor.FIRST_XML_VERSION_USING_INDEX)
                && primaryVersion < new System.Version(AGSEditor.FIRST_XML_VERSION_WITHOUT_INDEX);
            if (primaryVersion > new System.Version(AGSEditor.LATEST_XML_VERSION)
                || (usesIndex && versionIndex != null && (versionIndex < 1 || versionIndex > AGSEditor.LAST_XML_VERSION_INDEX)))
            {
                throw new AGSEditorException("Game data file is from " +
                    ((savedWithEditorVersion == null) ? "a newer version" : ("version " + savedWithEditorVersion))
                    + " of AGS or an unsupported beta version. Please check the AGS website for a newer version of the editor.");
            }

            int index = versionIndex ?? ToVersionIndex(primaryVersion);
            bool olderThan400 = primaryVersion < new System.Version(AGSEditor.FIRST_XML_VERSION_USING_INDEX)
                || (usesIndex && index < LOWEST_SUPPORTED_VERSION_INDEX);
            if (olderThan400)
            {
                string savedBy = string.IsNullOrEmpty(savedWithEditorVersion)
                    ? $"an AGS editor older than {FIRST_EDITOR_VERSION_WRITTEN}"
                    : $"AGS {savedWithEditorVersion}";
                throw new AGSEditorException($"This project was saved by {savedBy}. AGS Draconian opens projects " +
                    "saved by AGS 4.0 or later; open and save it with the AGS 4.0 editor first.");
            }
            return index;
        }
    }
}
