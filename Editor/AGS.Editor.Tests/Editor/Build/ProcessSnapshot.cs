using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace AGS.Editor.Build
{
    /// <summary>
    /// The processes running at one moment (Toolhelp32), for tests that check which processes a
    /// run leaves behind.
    /// </summary>
    internal sealed class ProcessSnapshot
    {
        private readonly Dictionary<int, Entry> _entries;

        private ProcessSnapshot(Dictionary<int, Entry> entries)
        {
            _entries = entries;
        }

        public struct Entry
        {
            public int Id;
            public int ParentId;
            public string ExeFile;
        }

        public static ProcessSnapshot Take()
        {
            IntPtr snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snapshot == INVALID_HANDLE_VALUE)
            {
                throw new Win32Exception();
            }
            try
            {
                var entries = new Dictionary<int, Entry>();
                var entry = new PROCESSENTRY32W { dwSize = (uint)Marshal.SizeOf(typeof(PROCESSENTRY32W)) };
                for (bool ok = Process32FirstW(snapshot, ref entry); ok; ok = Process32NextW(snapshot, ref entry))
                {
                    entries[(int)entry.th32ProcessID] = new Entry
                    {
                        Id = (int)entry.th32ProcessID,
                        ParentId = (int)entry.th32ParentProcessID,
                        ExeFile = entry.szExeFile
                    };
                }
                return new ProcessSnapshot(entries);
            }
            finally
            {
                CloseHandle(snapshot);
            }
        }

        public IEnumerable<Entry> Entries
        {
            get { return _entries.Values; }
        }

        /// <summary>
        /// The processes started, directly or not, by the given one, that were not running at the
        /// time of <paramref name="before"/> (so that a reused process id does not count).
        /// </summary>
        public List<Entry> NewDescendantsOf(int processId, ProcessSnapshot before)
        {
            var found = new List<Entry>();
            var parents = new Queue<int>();
            parents.Enqueue(processId);
            while (parents.Count > 0)
            {
                int parent = parents.Dequeue();
                foreach (Entry child in _entries.Values.Where(e => e.ParentId == parent && e.Id != parent))
                {
                    if (!before._entries.ContainsKey(child.Id) && found.All(f => f.Id != child.Id))
                    {
                        found.Add(child);
                        parents.Enqueue(child.Id);
                    }
                }
            }
            return found;
        }

        /// <summary>The processes that were not running at the time of <paramref name="before"/>.</summary>
        public List<Entry> NewSince(ProcessSnapshot before)
        {
            return _entries.Values.Where(e => !before._entries.ContainsKey(e.Id)).ToList();
        }

        /// <summary>The full path of a process's executable, or null if it cannot be read (or it has exited).</summary>
        public static string ImagePath(int processId)
        {
            IntPtr process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, (uint)processId);
            if (process == IntPtr.Zero)
            {
                return null;
            }
            try
            {
                var path = new StringBuilder(1024);
                int size = path.Capacity;
                return QueryFullProcessImageNameW(process, 0, path, ref size) ? path.ToString() : null;
            }
            finally
            {
                CloseHandle(process);
            }
        }

        /// <summary>
        /// Opens the processes, so that they can be waited for without mistaking another process
        /// for them; those that have exited already are left out.
        /// </summary>
        public static List<KeyValuePair<Entry, Process>> Open(IEnumerable<Entry> entries)
        {
            var opened = new List<KeyValuePair<Entry, Process>>();
            foreach (Entry entry in entries)
            {
                try
                {
                    Process process = Process.GetProcessById(entry.Id);
                    // Opens a handle, which keeps the process object and its id from being reused
                    GC.KeepAlive(process.Handle);
                    opened.Add(new KeyValuePair<Entry, Process>(entry, process));
                }
                catch (Exception ex) when (ex is ArgumentException || ex is InvalidOperationException)
                {
                    // Exited before it could be opened: nothing to wait for
                    continue;
                }
                catch (Win32Exception)
                {
                    // Exited while it was being opened, or not ours to open (cannot be checked)
                    continue;
                }
            }
            return opened;
        }

        /// <summary>
        /// Waits until the processes have exited, or the timeout; returns the names of those still running.
        /// </summary>
        public static List<string> StillRunning(List<KeyValuePair<Entry, Process>> processes, int timeoutMs)
        {
            var clock = Stopwatch.StartNew();
            var running = new List<string>();
            foreach (KeyValuePair<Entry, Process> process in processes)
            {
                int left = Math.Max(0, timeoutMs - (int)clock.ElapsedMilliseconds);
                if (!process.Value.WaitForExit(left))
                {
                    running.Add(process.Key.ExeFile + " (" + process.Key.Id + ")");
                }
            }
            return running;
        }

        /// <summary>
        /// Waits until <paramref name="condition"/> is true, or the timeout; returns the last result.
        /// </summary>
        public static bool WaitFor(Func<bool> condition, int timeoutMs)
        {
            var clock = Stopwatch.StartNew();
            while (!condition())
            {
                if (clock.ElapsedMilliseconds > timeoutMs)
                {
                    return false;
                }
                Thread.Sleep(50);
            }
            return true;
        }

        private const uint TH32CS_SNAPPROCESS = 0x00000002;
        private const uint PROCESS_QUERY_LIMITED_INFORMATION = 0x1000;
        private static readonly IntPtr INVALID_HANDLE_VALUE = new IntPtr(-1);

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct PROCESSENTRY32W
        {
            public uint dwSize;
            public uint cntUsage;
            public uint th32ProcessID;
            public IntPtr th32DefaultHeapID;
            public uint th32ModuleID;
            public uint cntThreads;
            public uint th32ParentProcessID;
            public int pcPriClassBase;
            public uint dwFlags;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public string szExeFile;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateToolhelp32Snapshot(uint flags, uint processId);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool Process32FirstW(IntPtr snapshot, ref PROCESSENTRY32W entry);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool Process32NextW(IntPtr snapshot, ref PROCESSENTRY32W entry);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint access, bool inheritHandle, uint processId);

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool QueryFullProcessImageNameW(IntPtr process, uint flags, StringBuilder exeName, ref int size);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr handle);
    }
}
