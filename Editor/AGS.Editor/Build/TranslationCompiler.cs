using System;
using System.Globalization;
using System.IO;
using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// Compiles a translation source (a .po file) into the engine's .tra file with po2tra, as
    /// agsbuild does when it builds the game (Tools/agsbuild/main.cpp, AddTranslationTasks): the
    /// game's name and unique id go into the .tra, so that the engine takes it only for this game.
    ///
    /// po2tra accepts any text as a .po file (what it cannot read as entries it leaves out, and it
    /// warns when no entry is translated); it fails when it cannot read the source or write the
    /// result. A failure is a message, not an exception.
    ///
    /// The class has no state: any number of compilations, also at the same time on several
    /// threads, as long as they write different files.
    /// </summary>
    public static class TranslationCompiler
    {
        /// <summary>The tool's file name, in the editor's folder.</summary>
        public const string ToolFileName = "po2tra.exe";

        /// <summary>
        /// Compiles one translation source with po2tra from toolsDirectory into traFile, creating the
        /// file's folder if needed. Blocks until the tool has exited.
        /// </summary>
        /// <param name="toolsDirectory">The folder of po2tra.exe.</param>
        /// <param name="poFile">The translation source; relative paths are taken from the current directory.</param>
        /// <param name="traFile">The compiled translation, replaced if it exists.</param>
        /// <param name="gameName">The game's name, written into the .tra; may be empty.</param>
        /// <param name="uniqueId">The game's unique id, written into the .tra.</param>
        /// <returns>
        /// The tool's errors, warnings and information. When it failed there is an error among them, and
        /// the first error names the translation source.
        /// </returns>
        /// <exception cref="ArgumentException">A path is null or empty.</exception>
        public static CompileMessages Compile(string toolsDirectory, string poFile, string traFile, string gameName,
            int uniqueId)
        {
            if (string.IsNullOrEmpty(toolsDirectory))
            {
                throw new ArgumentException("The tools' folder is empty.", nameof(toolsDirectory));
            }
            if (string.IsNullOrEmpty(poFile))
            {
                throw new ArgumentException("The translation source's path is empty.", nameof(poFile));
            }
            if (string.IsNullOrEmpty(traFile))
            {
                throw new ArgumentException("The compiled translation's path is empty.", nameof(traFile));
            }

            string poPath = Path.GetFullPath(poFile);
            string traPath = Path.GetFullPath(traFile);
            string failure = CreateFolderOf(traPath);
            if (failure != null)
            {
                return new CompileMessages(new CompileError(FailedText(poPath) + failure));
            }

            string arguments = string.Join(" ", new[]
            {
                BuildToolOptions.QuoteArgument(poPath),
                BuildToolOptions.QuoteArgument(traPath),
                "--gamename", BuildToolOptions.QuoteArgument(gameName ?? string.Empty),
                "--uniqueid", uniqueId.ToString(CultureInfo.InvariantCulture)
            });
            ToolProcessResult result = ToolProcess.Run(Path.Combine(toolsDirectory, ToolFileName), arguments,
                Path.GetDirectoryName(poPath));
            if (result.Succeeded)
            {
                return result.Messages;
            }
            return NameTheSource(result, poPath);
        }

        private static string FailedText(string poPath)
        {
            return "Translation '" + poPath + "' failed to compile: ";
        }

        // Creates the folder of the file; returns why that failed, or null
        private static string CreateFolderOf(string path)
        {
            string folder = Path.GetDirectoryName(path);
            if (string.IsNullOrEmpty(folder))
            {
                return null;
            }
            try
            {
                Directory.CreateDirectory(folder);
                return null;
            }
            catch (IOException ex)
            {
                return "cannot create the folder " + folder + ": " + ex.Message;
            }
            catch (UnauthorizedAccessException ex)
            {
                return "cannot create the folder " + folder + ": " + ex.Message;
            }
        }

        // The messages of a failed run, the first error naming the source: po2tra's errors do not
        // name it ("Error: failed to open source PO for reading.")
        private static CompileMessages NameTheSource(ToolProcessResult result, string poPath)
        {
            var messages = new CompileMessages();
            bool named = false;
            foreach (CompileMessage message in result.Messages)
            {
                if (!named && message is CompileError)
                {
                    messages.Add(new CompileError(FailedText(poPath) + message.Message, message.ScriptName,
                        message.LineNumber));
                    named = true;
                    continue;
                }
                messages.Add(message);
            }
            if (!named)
            {
                // ToolProcess gives an error for every failure: this is for one it may give in the future
                string text = FailedText(poPath) + ToolFileName + " failed";
                if (result.ExitCode.HasValue)
                {
                    text += " with exit code " + result.ExitCode.Value.ToString(CultureInfo.InvariantCulture);
                }
                text += ".";
                string output = result.Output.Trim();
                if (output.Length > 0)
                {
                    text += " Its output:" + Environment.NewLine + output;
                }
                messages.Add(new CompileError(text));
            }
            return messages;
        }
    }
}
