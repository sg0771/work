using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using System.Runtime.InteropServices;
using Apowersoft.Utils.Record.WXCapture;
using System.Threading.Tasks;

namespace Apowersoft.Utils.Record {
    
    public static class RecGame {

        public static event Action HookedGameInfoUpdated = delegate { };

        public static event Action HookedGameChanged = delegate { };

        public static event Action HookedRecGameClosed = delegate { };

        public static Func<Image, string, string> CreateGameInnerImageFileHandler;

        public static string HookedGameFilePath { get; private set; }

        public static int HookedGameVisualWidth { get; private set; }

        public static int HookedGameVisualHeight { get; private set; }

        public static int HookedGameType { get; private set; } = -1;

        public static string InnerGameTextFontFamily { get; private set; } = SystemFonts.CaptionFont.FontFamily.Name;

        public static bool IsHookedGameInfoValid {
            get { return CheckGameInfoValid(); }
        }
        
        private static WXCallBack callbackHandler;

        private static List<string> tempInnerImageFiles = new List<string>();

        public static void Initialize() {
            callbackHandler = OnWxCallback;
            WXCaptureLib.WXGameCreate(IntPtr.Zero, callbackHandler);
            CreateGameInnerImageFileHandler = CreateGameInnerImageFileDefault;
        }

        public static void Uninitialize() {
            WXCaptureLib.WXGameDestory();
            tempInnerImageFiles.ForEach(s => Utils.TryDeleteFile(s));
        }

        public static void StartMonitorGame() {
            WXCaptureLib.WXGameStart();
        }

        public static void StopMonitorGame() {
            SetGameShowingImageText(null, "", 0, 0, "", 0, 0, 0);
            WXCaptureLib.WXGameStop();
        }
        
        public static void SetPreview(IntPtr previewWindow) {
            WXCaptureLib.WXGameSetPreview(previewWindow);
        }

        public static string HookTypeToDesc(int type) {
            switch (type) {
                case 0:
                    return "D3D8";
                case 1:
                    return "D3D9";
                case 2:
                    return "D3D10";
                case 3:
                    return "D3D11";
                case 4:
                    return "D3D11_1";
                case 5:
                    return "D3D12";
                case 6:
                    return "OPENGL";
                default:
                    return "UNKNOWN";
            }
        }

        public static bool CheckCurrentGameExist() {
            int pid = WXCaptureLib.WXGameHookPID();
            if (pid > 0) {
                Process p = null;
                try {
                    p = Process.GetProcessById(pid);
                }
                catch { }
                if (p != null) {
                    return true;
                }
                Logger.Log.i(string.Format("Process {0} not exist.", pid));
            }
            return false;
        } 

        /// <summary>
        /// 设置游戏内显示的图片和文字
        /// </summary>
        /// <param name="img">图片，如果为null则为纯文字</param>
        /// <param name="text">文字，如果为空则为纯图片</param>
        /// <param name="x">坐标，默认相对于游戏画面左上角</param>
        /// <param name="y">坐标，默认相对于游戏画面左上角</param>
        /// <remarks>img为null，且text为空，则清空；img不为null，且text不为空，则二者组合</remarks>
        public static void SetGameShowingImageText(Image img, string text, int x, int y, string textFontFamily, int textFontSize, int textArgbColor, int backArgbColor) {
            if (img == null && string.IsNullOrEmpty(text)) {
                WXCaptureLib.WXGameDrawImage("", 0, 0);
                WXCaptureLib.WXGameDrawString("", 0, 0);
                Logger.Log.i("Set null image and text in game.");
            }
            else if (img != null) {
                string tempFile = CreateGameInnerImageFileHandler?.Invoke(img, text);

                if (!string.IsNullOrEmpty(tempFile)) {
                    tempInnerImageFiles.Add(tempFile);
                    WXCaptureLib.WXGameDrawImage(tempFile, x, y);
                    //Logger.Log.i("Set image and text in game." + tempFile + x + "x" + y);
                }
            }
            else {
                WXCaptureLib.WXGameDrawImage("", 0, 0);
                WXCaptureLib.WXGameDrawString(text, x, y);
                if (textArgbColor != 0 && backArgbColor != 0) {
                    WXCaptureLib.WXGameSetDrawColor(textArgbColor, backArgbColor);
                }
                if (textFontFamily != null) {
                    WXCaptureLib.WXGameSetDrawFont(textFontFamily, textFontSize);
                }
                //Logger.Log.i("Set text in game." + text + x + "x" + y);
            }
        }

        /// <summary>
        /// 主动得到宽高
        /// </summary>
        /// <returns></returns>
        public static int GetImageWidth() {
            return WXCaptureLib.WXGameGetWidth();
        }

        public static int GetImageHeight() {
            return WXCaptureLib.WXGameGetHeight();
        }

        private static void OnWxCallback(IntPtr ptrSink, uint cbID, IntPtr cbData) {
            switch (cbID) {
                case WXCaptureLib.WX_EVENT_HOOK_START:
                    if (cbData != IntPtr.Zero) {
                        string path = Marshal.PtrToStringUni(cbData);
                        if (path != HookedGameFilePath) {
                            HookedGameFilePath = path;
                            HookedGameVisualWidth = HookedGameVisualHeight = 0;
                            HookedGameType = WXCaptureLib.WXGameHookType();
                            HookedGameInfoUpdated();
                            HookedGameChanged();
                        }
                    }
                    break;
                case WXCaptureLib.WX_EVENT_HOOK_STOP:
                    HookedGameFilePath = "";
                    HookedGameVisualWidth = HookedGameVisualHeight = 0;
                    HookedGameType = -1;
                    HookedGameInfoUpdated();
                    break;
                case WXCaptureLib.WX_EVENT_HOOK_WIDTH:
                    HookedGameType = WXCaptureLib.WXGameHookType();
                    if (cbData != IntPtr.Zero) {
                        HookedGameVisualWidth = Marshal.ReadInt32(cbData);
                        HookedGameInfoUpdated();
                    }
                    break;
                case WXCaptureLib.WX_EVENT_HOOK_HEIGHT:
                    HookedGameType = WXCaptureLib.WXGameHookType();
                    if (cbData != IntPtr.Zero) {
                        HookedGameVisualHeight = Marshal.ReadInt32(cbData);
                        HookedGameInfoUpdated();
                    }
                    break;
                default:
                    break;
            }
        }

        private static bool CheckGameInfoValid() {
            return !string.IsNullOrEmpty(HookedGameFilePath) && HookedGameVisualWidth > 0 && HookedGameVisualHeight > 0;
        }

        private static string CreateGameInnerImageFileDefault(Image img, string text) {
            try {
                return CreateGameInnerImageFile(img, text);
            }
            catch (Exception ex) {
                Logger.Log.e("Error in CreateGameInnerImageFile: " + ex.Message);
            }
            return "";
        }

        private static string CreateGameInnerImageFile(Image img, string text) {
            string filePath = Path.GetTempFileName() + ".bmp";

            int width = 30 * 2;
            int height = 4 * 2;
            if (img != null) {
                width += img.Width;
                height += img.Height;
            }

            Font textFont = null;
            if (!string.IsNullOrEmpty(text)) {
                textFont = new Font(SystemFonts.CaptionFont.FontFamily, 14f, FontStyle.Regular, GraphicsUnit.Pixel);
                Size size = Size.Empty;
                using (Bitmap bmp = new Bitmap(2, 2)) {
                    using (Graphics graphics = Graphics.FromImage(bmp)) {
                        size = graphics.MeasureString(text, textFont, 1000, StrFormat.BothCentered).ToSize();
                    }
                }
                width += 10;
                width += size.Width;
                if (height < size.Height - 8) {
                    height = size.Height - 8;
                }
            }

            using (Bitmap dbmp = new Bitmap(width, height)) {
                using (Graphics graphics = Graphics.FromImage(dbmp)) {
                    graphics.Clear(Color.FromArgb(178/**102**/, 0, 0, 0));
                    int left = 30;
                    if (img != null) {
                        graphics.DrawImageUnscaled(img, left, (height - img.Height) / 2);
                        left += img.Width;
                        left += 10;
                    }
                    if (!string.IsNullOrEmpty(text)) {
                        graphics.DrawString(text, textFont, Brushes.White, new RectangleF(left, 0, width - left, height), StrFormat.VCenteredHLeft);
                    }
                }
                dbmp.Save(filePath);
            }

            return filePath;
        }

    }
}
