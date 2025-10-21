using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Drawing;
using System.Threading;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {

    public static class RecWebcam {

        internal enum Oper { Openhwnd, OpenSink, Close, Setting };

        public delegate void onWebcamDeviceStateChangedEventHandler();

        public static event onWebcamDeviceStateChangedEventHandler onWebcamDeviceStateChanged = delegate { };
        
        private static bool monitoring = false;
        private static Task deviceMonitorTask;

        private static ConcurrentQueue<Tuple<Oper, object>> opers = new ConcurrentQueue<Tuple<Oper, object>>();

        private static WebCamDevice[] webCamDevice;
        public static WebCamDevice[] WebCamDevices {
            get { return webCamDevice; }
        }

        /// <summary>
        /// 图片流的类型，可选rgb32，默认yuv
        /// </summary>
        public static string ImageDataType { get; set; }

        /// <summary>
        /// 虚拟背景及美颜参数
        /// </summary>
        public static WebcamPara WebcamPara { get; set; } = new WebcamPara();


        public static void Refresh() {
            webCamDevice = GetDevices();
        }

        internal static void Init() {
            try {
                WXCaptureLib.WXCameraInit();

                webCamDevice = GetDevices();

                monitoring = true;
                if (deviceMonitorTask == null) {
                    deviceMonitorTask = Task.Factory.StartNew(DeviceMonitorProc);
                }
            }
            catch (Exception ex) {
                Logger.Log.e(ex);
            }
        }

        internal static void UnInit() {
            monitoring = false;
        }

        internal static void AddOper(Tuple<Oper, object> op) {
            if (op != null) {
                opers.Enqueue(op);
            }
        }

        private static void DeviceMonitorProc() {
            try {
                int count = 0;
                while (monitoring) {
                    Thread.Sleep(250);
                    ++count;

                    if (monitoring && !opers.IsEmpty) {
                        Tuple<Oper, object> op;
                        if (opers.TryDequeue(out op)) {
                            ExecOpen(op.Item1, op.Item2);
                        }
                    }

                    if (monitoring && count % 4 == 0) {
                        RefreshState();
                    }

                    if (count >=80) {
                        GC.Collect();
                        count = 0;
                    }
                }
            }
            catch (Exception ex) {
                Logger.Log.e(ex);
            }
        }

        private static void ExecOpen(Oper op, object param) {
            if (op == Oper.Openhwnd) {
                WebCamDevice device = param as WebCamDevice;
                if (device != null && !string.IsNullOrEmpty(device.GUID)) {
                    WebCamDevice.DeviceProperty prop = device.SelectedProperty;
                    if (prop != null) {
                        IntPtr ptr;
                        if (device.IsHorizontalFlip) {
                            ptr = WXCaptureLib.WXCameraOpenWithHwndExt(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewWindowHandle, 0, 1);
                        }
                        else {
                            ptr = WXCaptureLib.WXCameraOpenWithHwnd(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewWindowHandle, 0);
                        }
                        device.SetPtr(ptr);
                        if (ptr != IntPtr.Zero) {
                            Logger.Log.i(string.Format("Started camera device preview.: {0}.", device.GUID));
                        }
                    }
                }
                return;
            }

            if (op == Oper.OpenSink) {
                WebCamDevice device = param as WebCamDevice;
                if (device != null && !string.IsNullOrEmpty(device.GUID)) {
                    WebCamDevice.DeviceProperty prop = device.SelectedProperty;
                    if (prop != null) {
                        IntPtr ptr;
                        if(!string.IsNullOrEmpty(ImageDataType) && ImageDataType.ToLower() == "rgb32") {
                            if (device.IsHorizontalFlip) {
                                ptr = WXCaptureLib.WXCameraOpenWithSinkExt2(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewDataCallbackHandler, 1);
                            }
                            else {
                                ptr = WXCaptureLib.WXCameraOpenWithSinkExt2(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewDataCallbackHandler, 0);
                            }
                        }
                        else {
                            if (device.IsHorizontalFlip) {
                                ptr = WXCaptureLib.WXCameraOpenWithSinkExt(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewDataCallbackHandler, 1);
                            }
                            else {
                                ptr = WXCaptureLib.WXCameraOpenWithSink(device.GUID, prop.FrameSize.Width, prop.FrameSize.Height, prop.AverageFPS, device.PreviewDataCallbackHandler);
                            }
                        }
                        device.SetPtr(ptr);
                        if (ptr != IntPtr.Zero) {
                            Logger.Log.i(string.Format("Started camera device preview: {0}.", device.GUID));
                        }
                    }
                }
                return;
            }

            if (op == Oper.Close) {
                WebCamDevice device = param as WebCamDevice;
                if (device != null && device.PtrOpen != IntPtr.Zero) {
                    if (device.PtrOpen == WXCaptureLib.WXCameraGetCurrDevice()) {
                        WXCaptureLib.WXCameraClose(device.PtrOpen);
                        Logger.Log.i("Has Stopped camera device");
                    }
                    device.SetPtr(IntPtr.Zero);
                    Logger.Log.i(string.Format("Stopped camera device: {0}.", device.GUID));
                }
                return;
            }

            if (op == Oper.Setting) {
                Tuple<object, IntPtr> t = param as Tuple<object, IntPtr>;
                if (t != null) {
                    IntPtr ptr = WXCaptureLib.WXCameraGetCurrDevice();
                    if (ptr != IntPtr.Zero) {
                        WXCaptureLib.WXCameraSetting(ptr, t.Item2);
                    }
                }
                return;
            }
        }

        private static void RefreshState() {
            WebCamDevice[] oldDevices = WebCamDevices;
            List<string> oldDevicesNames = new List<string>();
            if (oldDevices != null) {
                oldDevicesNames.AddRange(oldDevices.Select(item => item.Name));
            }

            WebCamDevice[] newDevices = GetDevices();
            List<string> newDevicesNames = new List<string>(newDevices.Select(item => item.Name));

            if (!Utils.CompareLists(oldDevicesNames, newDevicesNames)) {
                webCamDevice = newDevices;
                Logger.Log.i("Webcam device state changed");
                onWebcamDeviceStateChanged();
            }
        }

        private static WebCamDevice[] GetDevices() {
            List<WebCamDevice> webcamList = new List<WebCamDevice>();
            int count = WXCaptureLib.WXCameraGetCount();
            for (int i = 0; i < count; ++i) {
                WebCamDevice webcam = GetCameraDeviceInfo(i);
                if (webcam != null) {
                    webcam.SetDefaultProperty();
                    webcamList.Add(webcam);
                }
            }
            return webcamList.ToArray();
        }

        private static WebCamDevice GetCameraDeviceInfo(int index) {
            IntPtr ptr = WXCaptureLib.WXCameraGetInfo(index);
            if (ptr == IntPtr.Zero) {
                return null;
            }
            
            try {
                CameraInfo cameraInfo = (CameraInfo)Marshal.PtrToStructure(ptr, typeof(CameraInfo));
                List<WebCamDevice.DeviceProperty> deviceProperties = new List<WebCamDevice.DeviceProperty>();
                for (int i = 0; i < cameraInfo.SizeFmt; ++i) {
                    var dp = ConvertDeviceProperty(cameraInfo.ArrayFmt[i]);
                    if (dp != null) {
                        deviceProperties.Add(dp);
                    }
                }
                return new WebCamDevice() { Name = cameraInfo.Name, GUID = cameraInfo.GUID, DeviceProperties = deviceProperties.ToArray() };
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Get camera device info error: {0}, {1}.", index, ex.Message));
            }
            return null;
        }

        private static WebCamDevice.DeviceProperty ConvertDeviceProperty(CameraDataFormat cdf) {
            var dp = new WebCamDevice.DeviceProperty();
            dp.FrameSize = new Size(cdf.Width, cdf.Height);
            dp.AverageFPS = cdf.FPS;
            dp.Index = cdf.Index;
            return dp;
        }

    }

    public class WebcamPara {
        public string BackgroundImagePath { get; set; } = "";
        public float Whiten { get; set; }
        public float Blur { get; set; }
        public float Brightness { get; set; }
    }
}
