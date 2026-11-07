using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public class PreCompileGameEventArgs
	{
		private bool _forceRebuild;
		private DateTime? _forceRebuildTime;

		public PreCompileGameEventArgs(bool forceRebuild, DateTime? dt)
			: this(forceRebuild, dt, false)
		{
		}

		public PreCompileGameEventArgs(bool forceRebuild, DateTime? dt, bool buildsWithTools)
		{
			_forceRebuild = forceRebuild;
			_forceRebuildTime = dt;
			BuildsWithTools = buildsWithTools;
			AllowCompilation = true;
		}

		/// <summary>
		/// True when agsbuild builds the game after the event: it compiles the scripts, rooms and
		/// translations from the saved project, so a handler only prepares sources. False for the
		/// editor's own build, which the Run command still uses (PLAN.md 6.7d-10).
		/// </summary>
		public bool BuildsWithTools { get; }

		public bool ForceRebuild
		{
			get { return _forceRebuild; }
		}

		public DateTime? ForceRebuildTime
		{
			get { return _forceRebuildTime; }
		}

		public bool AllowCompilation;
		public CompileMessages Errors;
	}
}
