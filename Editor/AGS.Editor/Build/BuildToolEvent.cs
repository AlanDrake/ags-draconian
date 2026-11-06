using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace AGS.Editor.Build
{
    /// <summary>
    /// The kinds of lines that "agsbuild --json" writes to its standard output
    /// (Tools/agsbuild/diagnostics.h).
    /// </summary>
    public enum BuildToolEventKind
    {
        Start,
        Task,
        Diagnostic,
        Progress,
        End
    }

    /// <summary>
    /// The state of a build task in a <see cref="BuildToolEventKind.Task"/> event.
    /// </summary>
    public enum BuildToolTaskState
    {
        Running,
        Success,
        Failed,
        Cancelled
    }

    /// <summary>
    /// The severity of a <see cref="BuildToolEventKind.Diagnostic"/> event.
    /// </summary>
    public enum BuildToolSeverity
    {
        Info,
        Warning,
        Error
    }

    /// <summary>
    /// The result of the build in the <see cref="BuildToolEventKind.End"/> event.
    /// </summary>
    public enum BuildToolEndResult
    {
        Success,
        Failed,
        Cancelled
    }

    /// <summary>
    /// One line of the output of "agsbuild --json". A property has a meaning only for the kinds
    /// of event named in its description, and its default value for the others.
    /// </summary>
    public sealed class BuildToolEvent
    {
        private BuildToolEvent(BuildToolEventKind kind)
        {
            Kind = kind;
        }

        public BuildToolEventKind Kind { get; }

        /// <summary>Start: the number of tasks of the build.</summary>
        public int Tasks { get; private set; }

        /// <summary>
        /// Task, Diagnostic: the name of the task, such as "agscc:GlobalScript.asc". Empty in
        /// a diagnostic that does not belong to a task.
        /// </summary>
        public string Task { get; private set; }

        /// <summary>Task: the new state of the task.</summary>
        public BuildToolTaskState State { get; private set; }

        /// <summary>Diagnostic: error, warning or information.</summary>
        public BuildToolSeverity Severity { get; private set; }

        /// <summary>Diagnostic: the tool that reported it, such as "agscc".</summary>
        public string Tool { get; private set; }

        /// <summary>Diagnostic: the file it is about, or null if the tool did not name one.</summary>
        public string File { get; private set; }

        /// <summary>Diagnostic: the line in <see cref="File"/>, or 0 if the tool did not name one.</summary>
        public int Line { get; private set; }

        /// <summary>Diagnostic: the text.</summary>
        public string Message { get; private set; }

        /// <summary>Progress: the number of finished tasks.</summary>
        public int Done { get; private set; }

        /// <summary>Progress: the number of tasks of the build.</summary>
        public int Total { get; private set; }

        /// <summary>End: how the build ended.</summary>
        public BuildToolEndResult Result { get; private set; }

        /// <summary>End: the number of errors reported during the build.</summary>
        public int Errors { get; private set; }

        /// <summary>End: the number of warnings reported during the build.</summary>
        public int Warnings { get; private set; }

        /// <summary>
        /// Reads one line of the tool's standard output. Returns null if the line is not an
        /// event: an empty line, plain text, malformed JSON, a JSON value that is not an object,
        /// an unknown "event", or an unknown state, severity or result. Never throws.
        /// </summary>
        public static BuildToolEvent Parse(string line)
        {
            JObject json = ReadObject(line);
            if (json == null)
            {
                return null;
            }

            switch (GetString(json, "event"))
            {
                case "start":
                    return new BuildToolEvent(BuildToolEventKind.Start) { Tasks = GetInt(json, "tasks") };
                case "task":
                    return ParseTask(json);
                case "diagnostic":
                    return ParseDiagnostic(json);
                case "progress":
                    return new BuildToolEvent(BuildToolEventKind.Progress)
                    {
                        Done = GetInt(json, "done"),
                        Total = GetInt(json, "total")
                    };
                case "end":
                    return ParseEnd(json);
                default:
                    return null;
            }
        }

        private static BuildToolEvent ParseTask(JObject json)
        {
            BuildToolTaskState state;
            switch (GetString(json, "state"))
            {
                case "running": state = BuildToolTaskState.Running; break;
                case "success": state = BuildToolTaskState.Success; break;
                case "failed": state = BuildToolTaskState.Failed; break;
                case "cancelled": state = BuildToolTaskState.Cancelled; break;
                default: return null;
            }
            return new BuildToolEvent(BuildToolEventKind.Task)
            {
                Task = GetString(json, "task") ?? string.Empty,
                State = state
            };
        }

        private static BuildToolEvent ParseDiagnostic(JObject json)
        {
            BuildToolSeverity severity;
            switch (GetString(json, "severity"))
            {
                case "error": severity = BuildToolSeverity.Error; break;
                case "warning": severity = BuildToolSeverity.Warning; break;
                case "info": severity = BuildToolSeverity.Info; break;
                default: return null;
            }
            string file = GetString(json, "file");
            int lineNumber = GetInt(json, "line");
            return new BuildToolEvent(BuildToolEventKind.Diagnostic)
            {
                Severity = severity,
                Tool = GetString(json, "tool") ?? string.Empty,
                Task = GetString(json, "task") ?? string.Empty,
                File = string.IsNullOrEmpty(file) ? null : file,
                Line = lineNumber > 0 ? lineNumber : 0,
                Message = GetString(json, "message") ?? string.Empty
            };
        }

        private static BuildToolEvent ParseEnd(JObject json)
        {
            BuildToolEndResult result;
            switch (GetString(json, "result"))
            {
                case "success": result = BuildToolEndResult.Success; break;
                case "failed": result = BuildToolEndResult.Failed; break;
                case "cancelled": result = BuildToolEndResult.Cancelled; break;
                default: return null;
            }
            return new BuildToolEvent(BuildToolEventKind.End)
            {
                Result = result,
                Errors = GetInt(json, "errors"),
                Warnings = GetInt(json, "warnings")
            };
        }

        // The line as a JSON object, or null if it is not exactly one
        private static JObject ReadObject(string line)
        {
            if (line == null)
            {
                return null;
            }
            // Most lines that are not events are plain text: tell without the cost of an exception
            int start = 0;
            while (start < line.Length && char.IsWhiteSpace(line[start]))
            {
                start++;
            }
            if (start == line.Length || line[start] != '{')
            {
                return null;
            }

            try
            {
                using (JsonTextReader reader = new JsonTextReader(new StringReader(line)))
                {
                    // A message that looks like a date stays the text it is
                    reader.DateParseHandling = DateParseHandling.None;
                    JObject json = JObject.Load(reader);
                    // Text after the object makes the reader throw
                    return reader.Read() ? null : json;
                }
            }
            catch (JsonException)
            {
                return null;
            }
        }

        // The value of a string property, or null if there is none
        private static string GetString(JObject json, string name)
        {
            JValue value = json[name] as JValue;
            return value != null && value.Type == JTokenType.String ? (string)value.Value : null;
        }

        // The value of an integer property, or 0 if there is none or it does not fit an int
        private static int GetInt(JObject json, string name)
        {
            JValue value = json[name] as JValue;
            if (value == null || value.Type != JTokenType.Integer || !(value.Value is long))
            {
                return 0;
            }
            long number = (long)value.Value;
            return number >= int.MinValue && number <= int.MaxValue ? (int)number : 0;
        }
    }
}
