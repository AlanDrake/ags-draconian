using AGS.Types;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class BuildTargetDebug : BuildTargetBase
    {
        public const string DEBUG_DIRECTORY = AGSEditor.DEBUG_OUTPUT_DIRECTORY;
        public const string DEBUG_TARGET_NAME = "_Debug";

        public override IDictionary<string, string> GetRequiredLibraryPaths()
        {
            Dictionary<string, string> paths = new Dictionary<string, string>();
            paths.Add(AGSEditor.ENGINE_EXE_FILE_NAME, Factory.AGSEditor.EditorDirectory);
            return paths;
        }

        public override string[] GetPlatformStandardSubfolders()
        {
            return new string[] { GetCompiledPath() };
        }

        public override string GetCompiledPath(params string[] parts)
        {
            return base.GetCompiledPath(parts).Replace(AGSEditor.OUTPUT_DIRECTORY, DEBUG_DIRECTORY);
        }

        /// <summary>
        /// Deletes the game package of the run layout (agsbuild --data-only) and the game EXE that
        /// editors before agsbuild made in its place.
        /// </summary>
        public override void DeleteMainGameData(string name, CompileMessages errors)
        {
            string dir = Path.Combine(OutputDirectoryFullPath, DEBUG_DIRECTORY);
            string filename = Path.Combine(dir, name + ".exe");
            Utilities.ExecuteOrError(() => { Utilities.TryDeleteFile(filename); }, $"Failed to delete an old file {filename}.", errors);
            DeleteCommonGameFiles(dir, name, errors);
        }

        public override bool IsNormalBuildTarget
        {
            get
            {
                return false;
            }
        }

        public override string Name
        {
            get
            {
                return DEBUG_TARGET_NAME;
            }
        }

        public override string OutputDirectory
        {
            get
            {
                return "";
            }
        }

        public override string OutputDirectoryFullPath
        {
            get
            {
                return Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, OutputDirectory);
            }
        }
    }
}
