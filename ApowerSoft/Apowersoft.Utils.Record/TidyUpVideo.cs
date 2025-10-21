using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Apowersoft.Utils.Record {
    
    public class TidyUpVideo {

        private static bool screenSaverActived = true;
        private static int screenSaverTimeOut = 60;

        public static void HideDesktop() {
            try {
                IntPtr hTray = WinAPI.FindWindow("Progman", "Program Manager");
                hTray = WinAPI.FindWindowEx(hTray, IntPtr.Zero, "SHELLDLL_DefView", null);
                hTray = WinAPI.FindWindowEx(hTray, IntPtr.Zero, "SysListView32", null);
                if (hTray == IntPtr.Zero) {
                    WinAPI.EnumWindows((wnd, param) => {
                        StringBuilder className = new StringBuilder(1024);
                        WinAPI.GetClassName(wnd, className, 1024);
                        if ("WorkerW".Equals(className.ToString(), StringComparison.OrdinalIgnoreCase)) {
                            IntPtr hWnd = WinAPI.FindWindowEx(wnd, IntPtr.Zero, "SHELLDLL_DefView", null);
                            hWnd = WinAPI.FindWindowEx(hWnd, IntPtr.Zero, "SysListView32", null);
                            if (hWnd != IntPtr.Zero) {
                                hTray = hWnd;
                                return false;
                            }
                        }
                        return true;
                    }, IntPtr.Zero);
                }
                if (hTray != IntPtr.Zero) {
                    WinAPI.ShowWindow(hTray, (int)WinAPI.ShowWindowCommands.Hide);
                }
            }
            catch { }
        }

        public static void ShowDesktop() {
            try {
                IntPtr hTray = WinAPI.FindWindow("Progman", "Program Manager");
                hTray = WinAPI.FindWindowEx(hTray, IntPtr.Zero, "SHELLDLL_DefView", null);
                hTray = WinAPI.FindWindowEx(hTray, IntPtr.Zero, "SysListView32", null);
                if (hTray == IntPtr.Zero) {
                    WinAPI.EnumWindows((wnd, param) => {
                        StringBuilder className = new StringBuilder(1024);
                        WinAPI.GetClassName(wnd, className, 1024);
                        if ("WorkerW".Equals(className.ToString(), StringComparison.OrdinalIgnoreCase)) {
                            IntPtr hWnd = WinAPI.FindWindowEx(wnd, IntPtr.Zero, "SHELLDLL_DefView", null);
                            hWnd = WinAPI.FindWindowEx(hWnd, IntPtr.Zero, "SysListView32", null);
                            if (hWnd != IntPtr.Zero) {
                                hTray = hWnd;
                                return false;
                            }
                        }
                        return true;
                    }, IntPtr.Zero);
                }
                if (hTray != IntPtr.Zero) {
                    WinAPI.ShowWindow(hTray, (int)WinAPI.ShowWindowCommands.Restore);
                }
            }
            catch { }
        }

        public static void HideTaskBar() {
            IntPtr hTray = WinAPI.FindWindow("Shell_traywnd", "");
            if (hTray != IntPtr.Zero) {
                WinAPI.ShowWindow(hTray, (int)WinAPI.ShowWindowCommands.Hide);
            }
        }

        public static void ShowTaskBar() {
            IntPtr hTray = WinAPI.FindWindow("Shell_traywnd", "");
            if (hTray != IntPtr.Zero) {
                WinAPI.ShowWindow(hTray, (int)WinAPI.ShowWindowCommands.Restore);
            }
        }

        public static void HideScreenSaver() {
            screenSaverActived = ScreenSaver.GetScreenSaverActive();
            screenSaverTimeOut = ScreenSaver.GetScreenSaverTimeout();
            if (screenSaverActived) {
                ScreenSaver.SetScreenSaverActive(System.Convert.ToInt32(false));
            }
        }

        public static void ShowScreenSaver() {
            if (screenSaverActived) {
                ScreenSaver.SetScreenSaverTimeout(screenSaverTimeOut);
                ScreenSaver.SetScreenSaverActive(System.Convert.ToInt32(true));
            }
        }
    }

    internal static class ScreenSaver {

        // Signatures for unmanaged calls
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern bool SystemParametersInfo(int uAction, int uParam, ref int lpvParam, int flags);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern bool SystemParametersInfo(int uAction, int uParam, ref bool lpvParam, int flags);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int PostMessage(IntPtr hWnd, int wMsg, int wParam, int lParam);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern IntPtr OpenDesktop(string hDesktop, int Flags, bool Inherit, uint DesiredAccess);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern bool CloseDesktop(IntPtr hDesktop);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern bool EnumDesktopWindows(IntPtr hDesktop, EnumDesktopWindowsProc callback, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern bool IsWindowVisible(IntPtr hWnd);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        public static extern IntPtr GetForegroundWindow();

        // Callbacks
        private delegate bool EnumDesktopWindowsProc(IntPtr hDesktop, IntPtr lParam);

        // Constants
        private const int SPI_GETSCREENSAVERACTIVE = 16;
        private const int SPI_SETSCREENSAVERACTIVE = 17;
        private const int SPI_GETSCREENSAVERTIMEOUT = 14;
        private const int SPI_SETSCREENSAVERTIMEOUT = 15;
        private const int SPI_GETSCREENSAVERRUNNING = 114;
        private const int SPIF_SENDWININICHANGE = 2;

        private const uint DESKTOP_WRITEOBJECTS = 0x80;
        private const uint DESKTOP_READOBJECTS = 0x1;
        private const int WM_CLOSE = 16;

        // Returns TRUE if the screen saver is active (enabled, but not necessarily running).
        public static bool GetScreenSaverActive() {
            bool isActive = false;

            SystemParametersInfo(SPI_GETSCREENSAVERACTIVE, 0, ref isActive, 0);
            return isActive;
        }

        // Pass in TRUE(1) to activate or FALSE(0) to deactivate the screen saver.
        public static void SetScreenSaverActive(int Active) {
            int nullVar = 0;

            SystemParametersInfo(SPI_SETSCREENSAVERACTIVE, Active, ref nullVar, SPIF_SENDWININICHANGE);
        }

        // Returns the screen saver timeout setting, in seconds
        public static int GetScreenSaverTimeout() {
            int value = 0;
            SystemParametersInfo(SPI_GETSCREENSAVERTIMEOUT, 0, ref value, 0);
            return value;
        }

        // Pass in the number of seconds to set the screen saver timeout value.
        public static void SetScreenSaverTimeout(int Value) {
            int nullVar = 0;
            SystemParametersInfo(SPI_SETSCREENSAVERTIMEOUT, Value, ref nullVar, SPIF_SENDWININICHANGE);
        }

        // Returns TRUE if the screen saver is actually running
        public static bool GetScreenSaverRunning() {
            bool isRunning = false;
            SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, ref isRunning, 0);
            return isRunning;
        }

        // From Microsoft's Knowledge Base article #140723: http://support.microsoft.com/kb/140723
        // "How to force a screen saver to close once started in Windows NT, Windows 2000, and Windows Server 2003"

        public static void KillScreenSaver() {
            IntPtr hDesktop = OpenDesktop("Screen-saver", 0, false, DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS);
            if (hDesktop != IntPtr.Zero) {
                EnumDesktopWindows(hDesktop, new EnumDesktopWindowsProc(KillScreenSaverFunc), IntPtr.Zero);
                CloseDesktop(hDesktop);
            }
            else {
                PostMessage(GetForegroundWindow(), WM_CLOSE, 0, 0);
            }
        }

        private static bool KillScreenSaverFunc(IntPtr hWnd, IntPtr lParam) {
            if (IsWindowVisible(hWnd)) {
                PostMessage(hWnd, WM_CLOSE, 0, 0);
            }
            return true;
        }
    }

}
