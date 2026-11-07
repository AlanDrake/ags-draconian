using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// The result of one run of a tool by <see cref="ToolProcess.Run"/>.
    /// </summary>
    public sealed class ToolProcessResult
    {
        public ToolProcessResult(int? exitCode, CompileMessages messages, string output)
        {
            ExitCode = exitCode;
            Messages = messages ?? new CompileMessages();
            Output = output ?? string.Empty;
        }

        /// <summary>The tool's exit code; null if it did not start.</summary>
        public int? ExitCode { get; }

        /// <summary>Whether the tool started and exited with code 0.</summary>
        public bool Succeeded
        {
            get { return ExitCode == 0; }
        }

        /// <summary>
        /// The tool's errors, warnings and information (<see cref="ToolProcess.ParseDiagnostics"/>): those
        /// from its standard output, then those from its standard error; and the runner's own error when
        /// the tool did not start, or failed without saying why.
        /// </summary>
        public CompileMessages Messages { get; }

        /// <summary>Everything the tool wrote: its standard output, then its standard error.</summary>
        public string Output { get; }
    }
}
