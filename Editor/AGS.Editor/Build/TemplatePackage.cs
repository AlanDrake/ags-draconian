using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// Makes the files of game and room templates with the build's tools: the package (*.agt,
    /// *.art) with agspak, and a room template's room file with crmbuild.
    ///
    /// A template is a plain AGS asset package. The editor reads it back with the native code
    /// (AGS.Native/agsnative.cpp, load_template_file and extract_template_files): a game template
    /// must hold Game.agf, a room template rtemplate.dat.
    ///
    /// The class has no state; a failed tool run is a message, not an exception.
    /// </summary>
    public static class TemplatePackage
    {
        public const string PackToolFileName = "agspak.exe";
        public const string RoomToolFileName = "crmbuild.exe";

        /// <summary>
        /// Packs the assets into a new AGS package at packagePath with agspak: each asset is a stored
        /// name ('/' separates sub-folders) paired with the file to read. The files are first copied
        /// under their names into a new folder below stagingDirectory (created if needed, removed
        /// afterwards, also on failure), which agspak packs recursively. An existing package at
        /// packagePath is deleted first.
        /// </summary>
        /// <returns>The messages; an error among them when the package was not made (then no package
        /// is left behind).</returns>
        /// <exception cref="ArgumentException">An argument is null or empty, an asset name is not a
        /// relative path, or two names are the same but for case.</exception>
        public static CompileMessages Pack(string toolsDirectory, string packagePath,
            IEnumerable<KeyValuePair<string, string>> assets, string stagingDirectory)
        {
            RequireText(toolsDirectory, nameof(toolsDirectory));
            RequireText(packagePath, nameof(packagePath));
            RequireText(stagingDirectory, nameof(stagingDirectory));
            List<KeyValuePair<string, string>> assetList = CheckAssets(assets);

            string packageFile = Path.GetFullPath(packagePath);
            string stagingRoot = Path.GetFullPath(stagingDirectory);
            var messages = new CompileMessages();
            if (!TryDelete(packageFile, messages))
            {
                return messages;
            }
            foreach (KeyValuePair<string, string> asset in assetList.Where(a => !File.Exists(a.Value)))
            {
                messages.Add(new CompileError("Cannot find the file " + asset.Value + " to pack as " + asset.Key + "."));
            }
            if (messages.HasErrors)
            {
                return messages;
            }

            string folder;
            try
            {
                folder = CreateStagingFolder(stagingRoot);
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                messages.Add(new CompileError("Cannot create a folder in " + stagingRoot + ": " + ex.Message));
                return messages;
            }

            try
            {
                if (Stage(assetList, folder, messages))
                {
                    string arguments = "-c " + BuildToolOptions.QuoteArgument(packageFile) + " " +
                                       BuildToolOptions.QuoteArgument(folder) + " -r";
                    ToolProcessResult result = ToolProcess.Run(Path.Combine(toolsDirectory, PackToolFileName),
                        arguments, stagingRoot);
                    messages.AddRange(result.Messages);
                    if (result.Succeeded && !File.Exists(packageFile))
                    {
                        messages.Add(new CompileError(PackToolFileName + " did not write " + packageFile + "."));
                    }
                }
            }
            finally
            {
                RemoveStagingFolder(folder, stagingRoot, messages);
            }

            if (messages.HasErrors)
            {
                TryDelete(packageFile, messages);
            }
            return messages;
        }

        /// <summary>
        /// Builds the room file of a saved room with crmbuild from the project's sources
        /// (Rooms/&lt;roomNumber&gt;/ next to projectFile) into outputFile, whose folder is created.
        /// The room file has no compiled script. crmbuild reads the project from disk, so the
        /// project must be saved first.
        /// </summary>
        /// <returns>The messages; an error among them when the room file was not made (then none is
        /// left behind).</returns>
        /// <exception cref="ArgumentException">A path is null or empty, or roomNumber is negative.</exception>
        public static CompileMessages BuildRoomFile(string toolsDirectory, string projectFile, int roomNumber,
            string outputFile)
        {
            RequireText(toolsDirectory, nameof(toolsDirectory));
            RequireText(projectFile, nameof(projectFile));
            RequireText(outputFile, nameof(outputFile));
            if (roomNumber < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(roomNumber), roomNumber, "A room number is not negative.");
            }

            string project = Path.GetFullPath(projectFile);
            string output = Path.GetFullPath(outputFile);
            var messages = new CompileMessages();
            if (!TryDelete(output, messages))
            {
                return messages;
            }
            try
            {
                Directory.CreateDirectory(Path.GetDirectoryName(output));
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                messages.Add(new CompileError("Cannot create the folder of " + output + ": " + ex.Message));
                return messages;
            }

            string arguments = BuildToolOptions.QuoteArgument(project) + " " +
                               roomNumber.ToString(CultureInfo.InvariantCulture) + " -o " +
                               BuildToolOptions.QuoteArgument(output);
            ToolProcessResult result = ToolProcess.Run(Path.Combine(toolsDirectory, RoomToolFileName), arguments,
                Path.GetDirectoryName(project));
            messages.AddRange(result.Messages);
            if (result.Succeeded && !File.Exists(output))
            {
                messages.Add(new CompileError(RoomToolFileName + " did not write " + output + "."));
            }
            if (messages.HasErrors)
            {
                TryDelete(output, messages);
            }
            return messages;
        }

        private static void RequireText(string value, string name)
        {
            if (string.IsNullOrEmpty(value))
            {
                throw new ArgumentException("The argument is null or empty.", name);
            }
        }

        // The assets, with names checked; throws for a wrong name or a duplicate
        private static List<KeyValuePair<string, string>> CheckAssets(IEnumerable<KeyValuePair<string, string>> assets)
        {
            if (assets == null)
            {
                throw new ArgumentException("The list of assets is null.", nameof(assets));
            }
            var list = new List<KeyValuePair<string, string>>();
            var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            foreach (KeyValuePair<string, string> asset in assets)
            {
                CheckAssetName(asset.Key);
                if (string.IsNullOrEmpty(asset.Value))
                {
                    throw new ArgumentException("The asset " + asset.Key + " has no file.", nameof(assets));
                }
                if (!names.Add(asset.Key))
                {
                    throw new ArgumentException("Two assets have the name " + asset.Key + ".", nameof(assets));
                }
                list.Add(asset);
            }
            return list;
        }

        // A name is a relative path of plain file names separated by '/': it is a path below the staging folder
        private static void CheckAssetName(string name)
        {
            if (string.IsNullOrEmpty(name))
            {
                throw new ArgumentException("An asset has no name.", "assets");
            }
            char[] invalid = Path.GetInvalidFileNameChars();
            foreach (string part in name.Split('/'))
            {
                if (part.Length == 0 || part == "." || part == ".." || part.IndexOfAny(invalid) >= 0 ||
                    part.EndsWith(".", StringComparison.Ordinal) || part.EndsWith(" ", StringComparison.Ordinal))
                {
                    throw new ArgumentException("The asset name " + name + " is not a relative path of file names " +
                                                "separated by '/'.", "assets");
                }
            }
        }

        // A new, empty folder below stagingRoot, which is created if needed
        private static string CreateStagingFolder(string stagingRoot)
        {
            Directory.CreateDirectory(stagingRoot);
            while (true)
            {
                string folder = Path.Combine(stagingRoot, Path.GetFileNameWithoutExtension(Path.GetRandomFileName()));
                if (!Directory.Exists(folder) && !File.Exists(folder))
                {
                    Directory.CreateDirectory(folder);
                    return folder;
                }
            }
        }

        // Copies the assets under their names into the folder; false if one could not be copied
        private static bool Stage(List<KeyValuePair<string, string>> assets, string folder, CompileMessages messages)
        {
            foreach (KeyValuePair<string, string> asset in assets)
            {
                string target = Path.Combine(folder, asset.Key.Replace('/', Path.DirectorySeparatorChar));
                try
                {
                    Directory.CreateDirectory(Path.GetDirectoryName(target));
                    File.Copy(asset.Value, target);
                    // A read-only copy could not be deleted with the folder
                    File.SetAttributes(target, FileAttributes.Normal);
                }
                catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
                {
                    messages.Add(new CompileError("Cannot copy " + asset.Value + " to " + target + ": " + ex.Message));
                    return false;
                }
            }
            return true;
        }

        // Removes the staging folder, and stagingRoot if nothing else is in it
        private static void RemoveStagingFolder(string folder, string stagingRoot, CompileMessages messages)
        {
            try
            {
                Directory.Delete(folder, true);
                if (!Directory.EnumerateFileSystemEntries(stagingRoot).Any())
                {
                    Directory.Delete(stagingRoot);
                }
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                messages.Add(new CompileWarning("Cannot delete the temporary folder " + folder + ": " + ex.Message));
            }
        }

        // Deletes the file if it exists; false, with an error, if it cannot
        private static bool TryDelete(string file, CompileMessages messages)
        {
            try
            {
                if (File.Exists(file))
                {
                    File.Delete(file);
                }
                return true;
            }
            catch (Exception ex) when (ex is IOException || ex is UnauthorizedAccessException)
            {
                messages.Add(new CompileError("Cannot delete " + file + ": " + ex.Message));
                return false;
            }
        }
    }
}
