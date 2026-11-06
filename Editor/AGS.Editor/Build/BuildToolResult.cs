using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// How a run of the build tool ended (<see cref="BuildToolResult.Outcome"/>).
    /// </summary>
    public enum BuildToolOutcome
    {
        /// <summary>The tool reported that the build succeeded.</summary>
        Success,
        /// <summary>The tool reported that the build failed; the messages say why.</summary>
        Failed,
        /// <summary>The build was cancelled: the tool reported it, or it was stopped.</summary>
        Cancelled,
        /// <summary>The tool's executable is missing or could not be started.</summary>
        ToolNotFound,
        /// <summary>The tool exited without reporting the end of the build.</summary>
        Crashed
    }

    /// <summary>
    /// The result of one run of the build tool.
    /// </summary>
    public sealed class BuildToolResult
    {
        public BuildToolResult(BuildToolOutcome outcome, int? exitCode, CompileMessages messages, string log,
            int errors, int warnings)
        {
            Outcome = outcome;
            ExitCode = exitCode;
            Messages = messages ?? new CompileMessages();
            Log = log ?? string.Empty;
            Errors = errors;
            Warnings = warnings;
        }

        /// <summary>How the run ended.</summary>
        public BuildToolOutcome Outcome { get; }

        /// <summary>
        /// The tool's exit code (1 if the runner killed it, -1 if it could kill the tool alone); null if it
        /// did not start, or did not exit in time.
        /// </summary>
        public int? ExitCode { get; }

        /// <summary>
        /// The tool's errors, warnings and information, in the order they arrived, and the
        /// runner's own error when the run did not end with a report from the tool.
        /// </summary>
        public CompileMessages Messages { get; }

        /// <summary>Everything the tool wrote to its standard error: its human-readable log.</summary>
        public string Log { get; }

        /// <summary>The number of errors in the tool's "end" report; 0 without one.</summary>
        public int Errors { get; }

        /// <summary>The number of warnings in the tool's "end" report; 0 without one.</summary>
        public int Warnings { get; }
    }
}
