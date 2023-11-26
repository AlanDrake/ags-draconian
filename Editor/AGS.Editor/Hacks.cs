using System;
using System.Collections.Generic;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text;

namespace AGS.Editor
{
    public static class Hacks
    {
        private const int WM_SETTEXT = 0xC;
        private const int WM_SETREDRAW = 0x0b;
        private const int WM_USER = 0x400;
        private const int WM_THEMECHANGED = 0x31A;
        // RichTextBox messages
        private const int EM_SETSEL = 0x00B1;
        private const int EM_REPLACESEL = 0x00C2;
        private const int EM_GETSCROLLPOS = WM_USER + 221;
        private const int EM_SETSCROLLPOS = WM_USER + 222;
        private const int EM_SETMARGINS = 0xD3;
        private const int EC_LEFTMARGIN = 0x1;
        private const int EC_RIGHTMARGIN = 0x2;
        // TreeView messages
        private const int TV_FIRST = 0x1100;
        private const int TVM_GETEDITCONTROL = (TV_FIRST + 15);

        [DllImport("user32", CharSet = CharSet.Auto)]
        static extern int SendMessage(IntPtr hwnd, int wMsg, int wParam, IntPtr lParam);

        /// <summary>
        /// Disable Control's redrawing itself.
        /// </summary>
        public static void SuspendDrawing(this Control control)
        {
            SendMessage(control.Handle, WM_SETREDRAW, 0, IntPtr.Zero);
        }

        /// <summary>
        /// Enable Controls' redrawing itself.
        /// </summary>
        public static void ResumeDrawing(this Control control)
        {
            SendMessage(control.Handle, WM_SETREDRAW, 1, IntPtr.Zero);
            control.Invalidate();
        }

        /// <summary>
        /// Gets current RichTextBox scroll position.
        /// </summary>
        public static Point GetScrollPos(this RichTextBox rtb)
        {
            IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf<Point>());
            SendMessage(rtb.Handle, EM_GETSCROLLPOS, 0, pnt);
            var pos = Marshal.PtrToStructure(pnt, typeof(Point));
            Marshal.FreeHGlobal(pnt);
            return (Point)pos;
        }

        /// <summary>
        /// Sets current RichTextBox scroll position.
        /// </summary>
        public static void SetScrollPos(this RichTextBox rtb, Point pos)
        {
            IntPtr pnt = Marshal.AllocHGlobal(Marshal.SizeOf(pos));
            Marshal.StructureToPtr(pos, pnt, false);
            SendMessage(rtb.Handle, EM_SETSCROLLPOS, 0, pnt);
            Marshal.FreeHGlobal(pnt);
        }

        /// <summary>
        /// Erases a portion of the RichTextBox's text, starting from 'at' pos
        /// and following 'length' characters.
        /// WARNING: changes current selection.
        /// </summary>
        public static void EraseSelectedText(this RichTextBox rtb, int at, int length)
        {
            SendMessage(rtb.Handle, EM_SETSEL, at, (IntPtr)at + length);
            SendMessage(rtb.Handle, EM_REPLACESEL, 0, IntPtr.Zero);
        }

        /// <summary>
        /// Replaces a portion of the RichTextBox's text, starting from 'at' pos
        /// and following 'length' characters. The 'newText' will be pasted instead.
        /// WARNING: changes current selection.
        /// </summary>
        public static void ReplaceSelectedText(this RichTextBox rtb, int at, int length, string newText)
        {
            IntPtr strPtr = Marshal.StringToHGlobalAuto(newText);
            SendMessage(rtb.Handle, EM_SETSEL, at, (IntPtr)at + length);
            SendMessage(rtb.Handle, EM_REPLACESEL, 0, strPtr);
            Marshal.FreeHGlobal(strPtr);
        }

        /// <summary>
        /// Sets RichTextBox's margins
        /// </summary>
        public static void SetRichTextBoxMargins(this RichTextBox rtb, int left, int right)
        {
            SendMessage(rtb.Handle, EM_SETMARGINS, (EC_LEFTMARGIN | EC_RIGHTMARGIN), (IntPtr)(right * 0x10000 + left));
        }

        // Hack to get around the fact that the BeforeLabelEdit event provides no way to
        // let you change the text they're about to edit
        public static void SetTreeViewEditText(TreeView tree, string myText)
        {
            IntPtr editHandle = (IntPtr)SendMessage(tree.Handle, TVM_GETEDITCONTROL, 0, IntPtr.Zero);
            IntPtr strPtr = Marshal.StringToHGlobalAuto(myText);
            SendMessage(editHandle, WM_SETTEXT, 0, strPtr);
            Marshal.FreeHGlobal(strPtr);
        }

        // The PropertyGrid doesn't provide a settable SelectedTab property. Well, it does now.
        public static void SetSelectedTabInPropertyGrid(PropertyGrid propGrid, int selectedTabIndex)
        {
            Type type = propGrid.GetType();

            // Compatibility for derived classes
            if (type != typeof(PropertyGrid))
            {
                type = type.BaseType;
            }

            if ((selectedTabIndex < 0) || (selectedTabIndex >= propGrid.PropertyTabs.Count))
            {
                throw new ArgumentException("Invalid tab index to select: " + selectedTabIndex);
            }

            FieldInfo buttonsField = type.GetField("viewTabButtons", BindingFlags.NonPublic | BindingFlags.Instance);
            ToolStripButton[] viewTabButtons = (ToolStripButton[])buttonsField.GetValue(propGrid);
            ToolStripButton viewTabButton = viewTabButtons[selectedTabIndex];

            type.InvokeMember("OnViewTabButtonClick", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.InvokeMethod, null, propGrid, new object[] { viewTabButton, EventArgs.Empty });
        }

        // Using explorer through the process interface opens a new window, this allows to keep an existing window and highlight a file in it
        // Taken from SO, with slight adjustments, from here https://stackoverflow.com/a/72200643
        [DllImport("shell32.dll", SetLastError = true)]
        public static extern int SHOpenFolderAndSelectItems(IntPtr pidlFolder, uint cidl, [In, MarshalAs(UnmanagedType.LPArray)] IntPtr[] apidl, uint dwFlags);

        [DllImport("shell32.dll", SetLastError = true)]
        public static extern void SHParseDisplayName([MarshalAs(UnmanagedType.LPWStr)] string name, IntPtr bindingContext, [Out] out IntPtr pidl, uint sfgaoIn, [Out] out uint psfgaoOut);

        public static void ShowInExplorer(string filePath)
        {
            ShowInExplorer(System.IO.Path.GetDirectoryName(filePath), System.IO.Path.GetFileName(filePath));
        }
        public static void ShowInExplorer(string folderPath, string file)
        {
            IntPtr nativeFolder;
            uint psfgaoOut;
            SHParseDisplayName(folderPath, IntPtr.Zero, out nativeFolder, 0, out psfgaoOut);

            if (nativeFolder == IntPtr.Zero)
            {
                // can't find folder
                return;
            }

            IntPtr nativeFile = IntPtr.Zero;
            if (!string.IsNullOrEmpty(file))
            {
                SHParseDisplayName(System.IO.Path.Combine(folderPath, file), IntPtr.Zero, out nativeFile, 0, out psfgaoOut);
            }

            IntPtr[] fileArray;
            if (nativeFile == IntPtr.Zero)
            {
                // Open the folder without the file selected if we can't find the file
                fileArray = new IntPtr[] { nativeFolder };
            }
            else
            {
                fileArray = new IntPtr[] { nativeFile };
            }

            SHOpenFolderAndSelectItems(nativeFolder, (uint)fileArray.Length, fileArray, 0);

            Marshal.FreeCoTaskMem(nativeFolder);
            if (nativeFile != IntPtr.Zero)
            {
                Marshal.FreeCoTaskMem(nativeFile);
            }
        }

        // Dark theme support
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 = 19;
        private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;

        [DllImport("dwmapi")]
        private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int attrValue, int attrSize);


        [DllImport("uxtheme.dll", ExactSpelling = true, CharSet = CharSet.Unicode)]
        private static extern int SetWindowTheme(IntPtr hwnd, string pszSubAppName, string pszSubIdList);

        [DllImport("uxtheme.dll", EntryPoint = "#133", SetLastError = true, ExactSpelling = true, CharSet = CharSet.Unicode)]
        private static extern bool AllowDarkModeForWindow(IntPtr hWnd, bool allow);

        // before 1903
        [DllImport("uxtheme.dll", EntryPoint = "#135", SetLastError = true, ExactSpelling = true, CharSet = CharSet.Unicode)]
        private static extern bool AllowDarkModeForApp(bool allow);

        // after 1903
        [DllImport("uxtheme.dll", EntryPoint = "#135", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern int SetPreferredAppMode(int preferredAppMode);

        [DllImport("uxtheme.dll", EntryPoint = "#136", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern void FlushMenuThemes();

        public static void AllowDarkModeForApp()
        {
            if (IsWindows10OrGreater(18985))
                SetPreferredAppMode(2);
            else if (IsWindows10OrGreater(17763))
                AllowDarkModeForApp(true);

        }


        public static void DarkTheme(IntPtr hwnd)
        {
            AllowDarkModeForWindow(hwnd, true);
        }

        public static void DarkThemeControl(IntPtr hwnd)
        {
            SetWindowTheme(hwnd, "DarkMode_Explorer", null);
            SendMessage(hwnd, WM_THEMECHANGED, 0, IntPtr.Zero);
        }

        public static void DarkThemeControl_CFD(IntPtr hwnd)
        {
            SetWindowTheme(hwnd, "DarkMode_CFD", null);
            SendMessage(hwnd, WM_THEMECHANGED, 0, IntPtr.Zero);
        }

        // Hack to be able to dark the title bar of the main window
        public static bool UseImmersiveDarkMode(IntPtr handle, bool enabled)
        {
            AllowDarkModeForWindow(handle, enabled);

            if (IsWindows10OrGreater(17763))
            {
                var attribute = DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1;
                if (IsWindows10OrGreater(18985))
                {
                    attribute = DWMWA_USE_IMMERSIVE_DARK_MODE;
                }

                int useImmersiveDarkMode = enabled ? 1 : 0;
                return DwmSetWindowAttribute(handle, (int)attribute, ref useImmersiveDarkMode, sizeof(int)) == 0;
            }

            return false;
        }

        private static bool IsWindows10OrGreater(int build = -1)
        {
            return Environment.OSVersion.Version.Major >= 10 && Environment.OSVersion.Version.Build >= build;
        }

        public static ScrollBar GetPropertyGridScrollBar(PropertyGrid pg)
        {
            // Hack to get scrollbars from propertygrid
            // https://stackoverflow.com/questions/1596100/how-can-i-catch-scroll-events-in-windows-forms-propertygrid
            Control m_pgv = null;

            // Loop through sub-controls and find PropertyGridView
            foreach (Control c in pg.Controls)
            {
                if (c.Text == "PropertyGridView")
                {
                    m_pgv = (Control)c;
                    break;
                }
            }

            // Reflection trickery to get a private field,
            // scrollBar in this case
            Type t = m_pgv.GetType();
            FieldInfo f = t.GetField("scrollBar", BindingFlags.Instance | BindingFlags.NonPublic);
            ScrollBar sb = (ScrollBar)f.GetValue(m_pgv);
            return sb;
        }
    }
}
