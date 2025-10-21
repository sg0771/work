using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Drawing;
using System.Windows.Forms;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {

    public class WebCamDevice {
        
        public delegate void NewFrameHandler(object sender, IntPtr buff, int width, int height);
        public delegate void onDeviceReadyEventHandler(WebCamDevice webcamDevice);
        public delegate void onFPSChangedEventHandler(WebCamDevice webcamDevice, double realtimeFPS, double averageFPS);
        public delegate void onFramesBufferChangedEventHandler(WebCamDevice webcamDevice, Queue<FFMPEGFrameInfo> buffer);
        public delegate void onScreenshotEventHandler(string imagePath);

        public event NewFrameHandler NewFrame = delegate { };
        public event onDeviceReadyEventHandler onDeviceReady = delegate { };
        public event onFPSChangedEventHandler onFPSChanged = delegate { };
        public event onFramesBufferChangedEventHandler onFramesBufferChanged = delegate { };
        public event onScreenshotEventHandler onScreenshot = delegate { };

        public DeviceProperty[] DeviceProperties { get; set; }
        public Color DrawFontBackgroundColor { get; set; }
        public Color DrawFontColor { get; set; }
        public int DrawFontSize { get; set; }
        public bool EnableTimeStamp { get; set; }
        public int Index { get; set; }
        public string Moniker { get; set; }
        public string Name { get; set; }
        public string GUID { get; set; }
        public string OutputImage { get; set; }
        public string OutputVideo { get; set; }
        public Image PausedImage { get; set; }
        public enumRecStatus Status { get; set; }
        public int VideoBitrate { get; set; }
        public bool WatermarkHasBackground { get; set; }
        public int WatermarkStartTime { get; set; }
        public string WatermarkText { get; set; }
        public Color WatermarkTextColor { get; set; }
        public Font WatermarkTextFont { get; set; }
        public StringFormat WatermarkTextFormat { get; set; }
        public DeviceProperty SelectedProperty { get; set; }
        public bool IsHorizontalFlip { get; private set; }

        public bool IsRunning {
            get { return ptrOpen != IntPtr.Zero && ptrOpen == WXCaptureLib.WXCameraGetCurrDevice(); }
        }

        public VideoCallBack PreviewDataCallbackHandler { get; private set; }

        public IntPtr PreviewWindowHandle { get; private set; }

        public bool ResumeAfterRecording { get; set; }

        private IntPtr ptrOpen = IntPtr.Zero;
        public IntPtr PtrOpen {
            get { return ptrOpen; }
        }
        
        public WebCamDevice() {
            PreviewDataCallbackHandler = OnPreviewDataCallback;
        }

        public void SetPtr(IntPtr ptr) {
            ptrOpen = ptr;
        }

        public void SetDefaultProperty() {
            if (DeviceProperties.Length > 0) {
                int defaultIndex = 0;
                for (int i = 0; i < DeviceProperties.Length; ++i) {
                    if (Math.Abs(DeviceProperties[i].FrameSize.Width - 640) < Math.Abs(DeviceProperties[defaultIndex].FrameSize.Width - 640)) {
                        defaultIndex = i;
                    }
                }
                SelectedProperty = DeviceProperties[defaultIndex];
            }
        }

        public void ChangeResolution(int index) {
            DeviceProperty property = DeviceProperties.FirstOrDefault(dp => dp.Index == index);
            if (property == null || (SelectedProperty != null && SelectedProperty.Index == property.Index)) {
                return;
            }

            if (!IsRunning) {
                return;
            }

            SelectedProperty = property;
            Logger.Log.i(string.Format("Changing camera device preview with window: {0}, {1}.", Name, property.ToString()));
            RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.Close, this));
            if (PreviewWindowHandle != IntPtr.Zero) {
                RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.Openhwnd, this));
            }
            else {
                RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.OpenSink, this));
            }

            if (ptrOpen != IntPtr.Zero) {
                Logger.Log.i("Camera property changed.");
            }
        }

        public bool PreviewStart(IntPtr windowHandle) {
            if (IsRunning) {
                return true;
            }

            if (windowHandle == IntPtr.Zero) {
                Logger.Log.w("Need a window handle to display camera.");
                return false;
            }

            Logger.Log.i(string.Format("Starting camera device preview with window: {0}.", Name));
            if (SelectedProperty == null && DeviceProperties.Length > 0) {
                SelectedProperty = DeviceProperties[0];
            }
            var property = SelectedProperty;
            if (property != null) {
                onDeviceReady(this);
                PreviewWindowHandle = windowHandle;
                RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.Openhwnd, this));
                return true;
            }
            else {
                Logger.Log.e(string.Format("Can not get device property, will not start preview: {0}.", Name));
            }
            return false;
        }

        public bool PreviewStartWithSink() {
            PreviewWindowHandle = IntPtr.Zero;
            Logger.Log.i(string.Format("Starting camera device preview with sink: {0}.", Name));
            if (SelectedProperty == null && DeviceProperties.Length > 0) {
                SelectedProperty = DeviceProperties[0];
            }
            var property = SelectedProperty;
            if (property != null) {
                onDeviceReady(this);
                RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.OpenSink, this));
                return true;
            }
            else {
                Logger.Log.e(string.Format("Can not get device property, will not start preview: {0}.", Name));
            }
            return false;
        }

        public void PreviewStop() {
            if (IsRunning) {
                Logger.Log.i(string.Format("Stopping camera device preview, {0}.", Name));
                RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.Close, this));
            }
        }

        public void PreviewResume() {
            if (IsRunning) {
                Logger.Log.w(string.Format("Camera is running, will not need to resume: {0}.", Name));
                return;
            }

            if (PreviewWindowHandle == IntPtr.Zero) {
                PreviewStartWithSink();
            }
            else {
                PreviewStart(PreviewWindowHandle);
            }
        }

        public void SetHorizontalFlip(bool hFlip) {
            WXCaptureLib.WXCameraSetHFilp(PtrOpen, hFlip ? 1 : 0);
            IsHorizontalFlip = hFlip;
            Logger.Log.i(string.Format("Set camera device horizontl flip: {0}.", hFlip.ToString()));
        }

        public void Screenshot(string outImageFile) {
        }

        public void DisplayPropertyPage(IntPtr parentWindowHandle) {
            RecWebcam.AddOper(new Tuple<RecWebcam.Oper, object>(RecWebcam.Oper.Setting,
                new Tuple<object, IntPtr>(this, parentWindowHandle)));
        }

        public override string ToString() {
            return Name;
        }
        
        private void OnPreviewDataCallback(IntPtr buff, int width, int height) {
            NewFrame(this, buff, width, height);
        }

        public class DeviceProperty : IComparable {
            public int AverageFPS { get; set; }
            public Size FrameSize { get; set; }
            public int Index { get; set; }
            public int MaximumFPS { get; set; }

            public int CompareTo(object obj) {
                DeviceProperty item = obj as DeviceProperty;
                if (item != null) {
                    return item.AverageFPS != AverageFPS ? item.AverageFPS - AverageFPS
                        : (item.FrameSize.Width == 1280 ? 1 : item.FrameSize.Width - FrameSize.Width);
                }
                return 0;
            }

            public override string ToString() {
                return string.Format("{0} fps, {1}x{2}", AverageFPS, FrameSize.Width, FrameSize.Height);
            }
        }
    }

}
