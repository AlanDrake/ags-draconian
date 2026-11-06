using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

namespace AGS.Editor.Build
{
    /// <summary>
    /// A Windows job object for a process and every process it starts, so that they can be
    /// killed together. It is made with JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE: disposing of it, or
    /// the editor ending, kills whatever is still in it.
    /// </summary>
    internal sealed class ProcessJob : IDisposable
    {
        // JOBOBJECTINFOCLASS.JobObjectExtendedLimitInformation
        private const int EXTENDED_LIMIT_INFORMATION_CLASS = 9;
        private const uint JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x2000;

        private readonly SafeJobHandle _handle;

        private ProcessJob(SafeJobHandle handle)
        {
            _handle = handle;
        }

        /// <summary>
        /// Makes an empty job, or returns null if Windows refuses.
        /// </summary>
        public static ProcessJob Create()
        {
            SafeJobHandle handle = CreateJobObject(IntPtr.Zero, null);
            if (handle.IsInvalid)
            {
                handle.Dispose();
                return null;
            }
            var info = new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
            info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if (!SetInformationJobObject(handle, EXTENDED_LIMIT_INFORMATION_CLASS, ref info, Marshal.SizeOf(info)))
            {
                handle.Dispose();
                return null;
            }
            return new ProcessJob(handle);
        }

        /// <summary>
        /// Puts a running process into the job; the processes it starts from then on are in the
        /// job too. Returns false if Windows refuses: Windows 7 does not nest jobs, so there a
        /// process that is in a job already (the editor's, inherited) cannot join another one.
        /// </summary>
        public bool Add(Process process)
        {
            bool added = AssignProcessToJobObject(_handle, process.Handle);
            GC.KeepAlive(process);
            return added;
        }

        /// <summary>
        /// Kills every process in the job, with the given exit code. Returns false if Windows refuses.
        /// </summary>
        public bool Terminate(uint exitCode)
        {
            return TerminateJobObject(_handle, exitCode);
        }

        public void Dispose()
        {
            _handle.Dispose();
        }

        private sealed class SafeJobHandle : SafeHandleZeroOrMinusOneIsInvalid
        {
            public SafeJobHandle()
                : base(true)
            {
            }

            protected override bool ReleaseHandle()
            {
                return CloseHandle(handle);
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_BASIC_LIMIT_INFORMATION
        {
            public long PerProcessUserTimeLimit;
            public long PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize;
            public UIntPtr MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass;
            public uint SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IO_COUNTERS
        {
            public ulong ReadOperationCount;
            public ulong WriteOperationCount;
            public ulong OtherOperationCount;
            public ulong ReadTransferCount;
            public ulong WriteTransferCount;
            public ulong OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
        {
            public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
            public IO_COUNTERS IoInfo;
            public UIntPtr ProcessMemoryLimit;
            public UIntPtr JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed;
            public UIntPtr PeakJobMemoryUsed;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        private static extern SafeJobHandle CreateJobObject(IntPtr jobAttributes, string name);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetInformationJobObject(SafeJobHandle job, int infoClass,
            ref JOBOBJECT_EXTENDED_LIMIT_INFORMATION info, int infoLength);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool AssignProcessToJobObject(SafeJobHandle job, IntPtr process);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool TerminateJobObject(SafeJobHandle job, uint exitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool CloseHandle(IntPtr handle);
    }
}
