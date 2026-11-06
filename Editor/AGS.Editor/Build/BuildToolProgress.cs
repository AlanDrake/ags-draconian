namespace AGS.Editor.Build
{
    /// <summary>
    /// The state of a running build, as the build tool last reported it.
    /// </summary>
    public sealed class BuildToolProgress
    {
        public BuildToolProgress(int done, int total, string task)
        {
            Done = done;
            Total = total;
            Task = task;
        }

        /// <summary>The number of finished tasks (succeeded, failed or cancelled).</summary>
        public int Done { get; }

        /// <summary>The number of tasks of the build; 0 before the tool has counted them.</summary>
        public int Total { get; }

        /// <summary>
        /// The name of the task that started last, such as "agscc:GlobalScript.asc"; null before
        /// the first one starts. It may have finished already.
        /// </summary>
        public string Task { get; }
    }
}
