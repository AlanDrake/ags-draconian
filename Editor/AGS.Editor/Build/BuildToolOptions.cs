using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace AGS.Editor.Build
{
    /// <summary>
    /// What a run of agsbuild needs, and its command line.
    /// </summary>
    public sealed class BuildToolOptions
    {
        /// <summary>The project's Game.agf. Required.</summary>
        public string ProjectFile { get; set; }

        /// <summary>The folder that receives the game's files; it must exist. Required.</summary>
        public string OutputDirectory { get; set; }

        /// <summary>
        /// The folder of agsbuild and the tools it runs (--sys-dir). If null, agsbuild looks for
        /// the tools in its own folder.
        /// </summary>
        public string ToolsDirectory { get; set; }

        /// <summary>
        /// The folder for intermediate files (--temp-dir). If null, agsbuild makes one in the
        /// output folder.
        /// </summary>
        public string TempDirectory { get; set; }

        /// <summary>
        /// The script API header (--api-ash). If null, agsbuild reads agsdefns.sh in the tools folder.
        /// </summary>
        public string ApiHeader { get; set; }

        /// <summary>
        /// Script headers to compile every script with (--header, once for each, in this order).
        /// agsbuild does not have the option yet: today it takes each file for one more positional
        /// argument and ignores it.
        /// </summary>
        public List<string> ScriptHeaders { get; } = new List<string>();

        /// <summary>
        /// Builds the game data only (--data-only). agsbuild does not have the option yet, and ignores it.
        /// </summary>
        public bool DataOnly { get; set; }

        /// <summary>Fails the build if the project has something the tools cannot read (--strict).</summary>
        public bool Strict { get; set; }

        /// <summary>The number of tasks run at the same time (--threads), 1 or more. If null, agsbuild decides.</summary>
        public int? Threads { get; set; }

        /// <summary>
        /// The command line for agsbuild, without the program's name. Paths are written in full
        /// (<see cref="Path.GetFullPath(string)"/>), so that the command means the same in any
        /// working directory, and never starts with '-', which agsbuild would take for an option.
        /// Throws InvalidOperationException if a required value is missing or a value is out of range,
        /// and what <see cref="Path.GetFullPath(string)"/> throws for a malformed path.
        /// </summary>
        public string ToArguments()
        {
            if (string.IsNullOrEmpty(ProjectFile))
            {
                throw new InvalidOperationException($"{nameof(ProjectFile)} is not set.");
            }
            if (string.IsNullOrEmpty(OutputDirectory))
            {
                throw new InvalidOperationException($"{nameof(OutputDirectory)} is not set.");
            }
            if (Threads.HasValue && Threads.Value < 1)
            {
                throw new InvalidOperationException($"{nameof(Threads)} must be 1 or more.");
            }

            List<string> arguments = new List<string>();
            arguments.Add(QuotePath(ProjectFile));
            arguments.Add(QuotePath(OutputDirectory));
            arguments.Add("--json");
            AddPathOption(arguments, "--sys-dir", ToolsDirectory);
            AddPathOption(arguments, "--temp-dir", TempDirectory);
            AddPathOption(arguments, "--api-ash", ApiHeader);
            foreach (string header in ScriptHeaders)
            {
                if (string.IsNullOrEmpty(header))
                {
                    throw new InvalidOperationException($"{nameof(ScriptHeaders)} has an empty entry.");
                }
                AddPathOption(arguments, "--header", header);
            }
            if (DataOnly)
            {
                arguments.Add("--data-only");
            }
            if (Strict)
            {
                arguments.Add("--strict");
            }
            if (Threads.HasValue)
            {
                arguments.Add("--threads");
                arguments.Add(Threads.Value.ToString(CultureInfo.InvariantCulture));
            }
            return string.Join(" ", arguments);
        }

        /// <summary>
        /// Writes one argument of a Windows command line, so that the program reads back exactly
        /// <paramref name="value"/> (the rules of CommandLineToArgvW and of the C runtime):
        /// quotes around a value that is empty or has a space, a tab, a line break or a quote;
        /// a backslash before each quote, and each backslash doubled if a quote follows the run.
        /// The two parsers differ only on "" inside quotes and on the program's name, which this
        /// never writes. Not for cmd.exe, which has rules of its own (^, &amp;, %).
        /// </summary>
        public static string QuoteArgument(string value)
        {
            if (value == null)
            {
                throw new ArgumentNullException(nameof(value));
            }
            if (value.Length > 0 && value.IndexOfAny(new[] { ' ', '\t', '\n', '\r', '\v', '\f', '"' }) < 0)
            {
                return value;
            }

            StringBuilder quoted = new StringBuilder(value.Length + 2);
            quoted.Append('"');
            int backslashes = 0;
            foreach (char c in value)
            {
                if (c == '\\')
                {
                    backslashes++;
                    continue;
                }
                if (c == '"')
                {
                    // The backslashes before a quote are doubled, and one more escapes the quote
                    quoted.Append('\\', backslashes * 2 + 1);
                }
                else
                {
                    quoted.Append('\\', backslashes);
                }
                quoted.Append(c);
                backslashes = 0;
            }
            // The backslashes at the end come before the closing quote
            quoted.Append('\\', backslashes * 2);
            quoted.Append('"');
            return quoted.ToString();
        }

        private static string QuotePath(string path)
        {
            return QuoteArgument(Path.GetFullPath(path));
        }

        private static void AddPathOption(List<string> arguments, string option, string path)
        {
            if (!string.IsNullOrEmpty(path))
            {
                arguments.Add(option);
                arguments.Add(QuotePath(path));
            }
        }
    }
}
