using System;
using System.Collections.Generic;
using System.Text;
using System.ComponentModel;
using AGS.Types;
using System.Xml;

namespace AGS.Types
{
	public class WorkspaceState
	{
		private BuildConfiguration _lastBuildConfiguration = AGS.Types.BuildConfiguration.Unknown;
        private string _lastBuildGameFileName = "";
        private bool _requiresRebuild = false;

        public WorkspaceState()
		{
		}

		[Browsable(false)]
		[DefaultValue(BuildConfiguration.Unknown)]
		public BuildConfiguration LastBuildConfiguration
		{
			get { return _lastBuildConfiguration; }
			set { _lastBuildConfiguration = value; }
		}

        [Browsable(false)]
        public string LastBuildGameFileName
        {
            get { return _lastBuildGameFileName; }
            set { _lastBuildGameFileName = value; }
        }

        [Browsable(false)]
        public bool RequiresRebuild
        {
            get { return _requiresRebuild; }
            set { _requiresRebuild = value; }
        }

        [Obsolete]
        [Browsable(false)]
        public DateTime RequiredRebuildTime
        {
            get; set; // not used: the build manifests replaced the time-stamp comparison (6.7d-12); kept so that user data written before it still loads
        }

        // TODO: generic method of serializing key-value list in a string (or other xml element)
        private static string[] StringListSeparators = new string[] { "," };
        private static string[] KeyValueSeparators = new string[] { "=" };

        public Dictionary<string, string> GetLastBuildGameFiles()
        {
            var dic = new Dictionary<string, string>();
            string[] files = _lastBuildGameFileName.Split(StringListSeparators, StringSplitOptions.RemoveEmptyEntries);
            foreach (string item in files)
            {
                string[] keyval = item.Split(KeyValueSeparators, StringSplitOptions.None);
                if (keyval.Length >= 2 && !string.IsNullOrWhiteSpace(keyval[0]))
                    dic[keyval[0]] = keyval[1];
            }
            return dic;
        }

        public void SetLastBuildGameFiles(IReadOnlyDictionary<string, string> dic)
        {
            StringBuilder sb = new StringBuilder();
            foreach (var item in dic)
                sb.AppendFormat("{0}={1},", item.Key, item.Value);
            _lastBuildGameFileName = sb.ToString();
        }

        public void ToXml(XmlTextWriter writer)
		{
			SerializeUtils.SerializeToXML(this, writer);
		}

		/// <summary>
		/// Reads the state from the user data's root node; a null node, or one without a WorkspaceState
		/// section (user data written before 3.4.0.1, or reset after a failed read), gives the defaults.
		/// </summary>
		public void FromXml(System.Xml.XmlNode node)
		{
			_lastBuildConfiguration = AGS.Types.BuildConfiguration.Unknown;
			_lastBuildGameFileName = "";
			_requiresRebuild = false;

			if (node != null && node.SelectSingleNode("WorkspaceState") != null)
				SerializeUtils.DeserializeFromXML(this, node);
		}
	}
}
