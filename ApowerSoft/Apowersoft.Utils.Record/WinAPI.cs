using System;
using System.Text;
using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;
using System.Security.Permissions;

namespace Apowersoft.Utils.Record {

    internal static class WinAPI {

        public enum WindowStyleExs {
            WS_EX_TRANSPARENT = 0x20,
            WM_NCHITTEST = 0x84,
            HTTRANSPARENT = -1
        }
        
        public enum ShowWindowCommands {
            /// <summary>
            /// Hides the window and activates another window.
            /// </summary>
            Hide = 0,

            /// <summary>
            /// Activates and displays a window. If the window is minimized or
            /// maximized, the system restores it to its original size and position.
            /// An application should specify this flag when displaying the window
            /// for the first time.
            /// </summary>
            Normal = 1,

            /// <summary>
            /// Activates the window and displays it as a minimized window.
            /// </summary>
            ShowMinimized = 2,

            /// <summary>
            /// Maximizes the specified window.
            /// </summary>
            Maximize = 3,
            
            /// <summary>
            /// Activates the window and displays it as a maximized window.
            /// </summary>
            ShowMaximized = 3,
            ShowNoActivate = 4,

            /// <summary>
            /// Activates the window and displays it in its current size and position.
            /// </summary>
            Show = 5,

            /// <summary>
            /// Minimizes the specified window and activates the next top-level
            /// window in the Z order.
            /// </summary>
            Minimize = 6,

            ShowMinNoActive = 7,
            ShowNA = 8,

            /// <summary>
            /// Activates and displays the window. If the window is minimized or
            /// maximized, the system restores it to its original size and position.
            /// An application should specify this flag when restoring a minimized window.
            /// </summary>
            Restore = 9,

            /// <summary>
            /// Sets the show state based on the SW_* value specified in the
            /// STARTUPINFO structure passed to the CreateProcess function by the
            /// program that started the application.
            /// </summary>
            ShowDefault = 10,

            /// <summary>
            /// <b>Windows 2000/XP:</b> Minimizes a window, even if the thread
            /// that owns the window is not responding. This flag should only be
            /// used when minimizing windows from a different thread.
            /// </summary>
            ForceMinimize = 11
        }

        [UnmanagedFunctionPointer(CallingConvention.Winapi, CharSet = CharSet.Unicode)]
        public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll")]
        public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass, string lpszWindow);

        [DllImport("user32.dll")]
        public static extern IntPtr FindWindow(string strClassName, string strWindowName);

        [DllImport("user32.dll")]
        public static extern int ShowWindow(IntPtr hwnd, int nStyle);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

        [DllImport("user32.dll")]
        public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport("gdi32", SetLastError = true)]
        public static extern bool DeleteObject(IntPtr hObject);

        [DllImport("gdi32", SetLastError = true)]
        public static extern bool DeleteDC(IntPtr hDC);

        [DllImport("dwmapi.dll", PreserveSig = false)]
        public static extern bool DwmIsCompositionEnabled();

        [DllImport("User32.dll")]
        public static extern IntPtr GetDC(IntPtr Hwnd);

        [DllImport("User32.dll")]
        public static extern int ReleaseDC(IntPtr hWnd, IntPtr hDC);

        [DllImport("gdi32.dll", SetLastError = true)]
        public static extern int GetDeviceCaps(IntPtr hdc, int nIndex);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern bool SetProcessDPIAware();

        [DllImport("user32", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetWindowInfo(IntPtr hwnd, ref WindowInfo pwi);

        [DllImport("user32", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GetWindowRect(IntPtr hWnd, ref RECT lpRect);

        [DllImport("user32.dll")]
        private static extern bool GetCursorInfo(ref CURSORINFO pci);

        [DllImport("user32.dll")]
        private static extern bool DrawIcon(IntPtr hDC, int X, int Y, IntPtr hIcon);

        [DllImport("user32", SetLastError = true)]
        public static extern SafeIconHandle CopyIcon(IntPtr hIcon);

        [DllImport("user32", SetLastError = true)]
        public static extern bool DestroyIcon(IntPtr hIcon);

        [DllImport("user32", SetLastError = true)]
        public static extern bool GetIconInfo(SafeIconHandle iconHandle, ref ICONINFO iconInfo);

        [DllImport("GDI32.dll")]
        public static extern bool BitBlt(int hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, int hdcSrc, int nXSrc, int nYSrc, int dwRop);

        [DllImport("GDI32.dll")]
        public static extern int CreateCompatibleBitmap(int hdc, int nWidth, int nHeight);

        [DllImport("GDI32.dll")]
        public static extern int CreateCompatibleDC(int hdc);

        [DllImport("GDI32.dll")]
        public static extern bool DeleteDC(int hdc);

        [DllImport("GDI32.dll")]
        public static extern bool DeleteObject(int hObject);

        [DllImport("gdi32.dll")]
        public
        static extern int CreateDC(string lpszDriver, string lpszDevice, string lpszOutput, IntPtr lpInitData);

        [DllImport("GDI32.dll")]
        public static extern int GetDeviceCaps(int hdc, int nIndex);

        [DllImport("GDI32.dll")]
        public static extern int SelectObject(int hdc, int hgdiobj);

        [DllImport("User32.dll")]
        public static extern int GetDesktopWindow();

        [DllImport("User32.dll")]
        public static extern int GetWindowDC(int hWnd);

        [DllImport("User32.dll")]
        public static extern int ReleaseDC(int hWnd, int hDC);

        [DllImport("User32.dll")]
        public static extern int IsWindow(IntPtr hWnd);

        public enum TernaryRasterOperations : uint {
            SRCCOPY = 0xCC0020,
            SRCPAINT = 0xEE0086,
            SRCAND = 0x8800C6,
            SRCINVERT = 0x660046,
            SRCERASE = 0x440328,
            NOTSRCCOPY = 0x330008,
            NOTSRCERASE = 0x1100A6,
            MERGECOPY = 0xC000CA,
            MERGEPAINT = 0xBB0226,
            PATCOPY = 0xF00021,
            PATPAINT = 0xFB0A09,
            PATINVERT = 0x5A0049,
            DSTINVERT = 0x550009,
            BLACKNESS = 0x42,
            WHITENESS = 0xFF0062,
            CAPTUREBLT = 0x40000000
            //only if WinVer >= 5.0.0 (see wingdi.h)
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct RECT {
            public int left;
            public int top;
            public int right;
            public int bottom;
        } //RECT

        [StructLayout(LayoutKind.Sequential), Serializable]
        public struct WindowInfo {
            public uint cbSize;
            public RECT rcWindow;
            public RECT rcClient;
            public uint dwStyle;
            public uint dwExStyle;
            public uint dwWindowStatus;
            public uint cxWindowBorders;
            public uint cyWindowBorders;
            public ushort atomWindowType;
            public ushort wCreatorVersion;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct LPRECT {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct CURSORINFO {
            public int cbSize;
            public int flags;
            public IntPtr hCursor;
            public POINTAPI ptScreenPos;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct POINTAPI {
            public int x;
            public int y;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct ICONINFO {
            public bool fIcon;
            // Specifies whether this structure defines an icon or a cursor. A value of TRUE specifies
            public int xHotspot;
            // Specifies the x-coordinate of a cursor's hot spot. If this structure defines an icon, the hot
            public int yHotspot;
            // Specifies the y-coordinate of the cursor's hot spot. If this structure defines an icon, the hot
            public IntPtr hbmMask;
            // (HBITMAP) Specifies the icon bitmask bitmap. If this structure defines a black and white icon,
            public IntPtr hbmColor;
            // (HBITMAP) Handle to the icon color bitmap. This member can be optional if this
        }

        /// <summary>
        /// 阻止电脑进入休眠
        /// </summary>
        public static void PreventComputerFromEnteringSleep() {
            try {
                SetThreadExecutionState(EXECUTION_STATE.ES_SYSTEM_REQUIRED | EXECUTION_STATE.ES_CONTINUOUS | EXECUTION_STATE.ES_DISPLAY_REQUIRED);
            }
            catch { }
        }

        public static void ResetComputerFromEnteringSleep() {
            try {
                SetThreadExecutionState(EXECUTION_STATE.ES_CONTINUOUS);
            }
            catch { }
        }
        
        /// <summary>
        /// Define the API execution states
        /// </summary>
        private enum EXECUTION_STATE : uint {
            ES_SYSTEM_REQUIRED = 0x1, // Stay in working state by resetting display idle timer
            ES_DISPLAY_REQUIRED = 0x2, // Force display on by resetting system idle timer
            ES_CONTINUOUS = 0x80000000 // Force this state until next ES_CONTINUOUS call and one of the other flags are cleared
        }
        
        /// <summary>
        /// API call to prevent sleep (until the application exits)
        /// </summary>
        /// <remarks>http://indocst.blogspot.com/2015/03/prevent-computer-from-automatically.html</remarks>
        [DllImport("kernel32", ExactSpelling = true, CharSet = CharSet.Ansi, SetLastError = true)]
        private static extern EXECUTION_STATE SetThreadExecutionState(EXECUTION_STATE esflags);
    }

    internal class SafeIconHandle : SafeHandleZeroOrMinusOneIsInvalid {
        private SafeIconHandle() : base(true) {
        }

        public SafeIconHandle(IntPtr hIcon) : base(true) {
            this.SetHandle(hIcon);
        }

        [SecurityPermission(SecurityAction.LinkDemand, UnmanagedCode = true)]
        protected override bool ReleaseHandle() {
            return WinAPI.DestroyIcon(this.handle);
        }
    }

}
