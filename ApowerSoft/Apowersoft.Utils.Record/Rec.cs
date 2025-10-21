using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Drawing;
using System.Drawing.Imaging;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using Apowersoft.Utils.Record.WXCapture;
using System.Timers;

namespace Apowersoft.Utils.Record {
    
    public partial class Rec {
        private class RecFileInfo {
            public string FilePath;
            public bool Finished;
        }

        private class ScreenshotFileInfo {
            public string FilePath;
            public string Format;
            public TakeScreenshotCompleteCallback CompleteCallback;
        }

        public delegate void AudioDisplaySpectrumChangedEventHandler(string sessionID, short[] bandsTable);
        public delegate void CompletedEventHandler(string sessionID, MediaInfo mi);
        public delegate void DeletedEventHandler(string sessionID, string outputFile);
        public delegate void ErrorEventHandler(string sessionID, enumErrorCode erroCode, string message);
        public delegate void FPSChangedEventHandler(string sessionID, double realtimeFPS, double averageFPS);
        public delegate void FramesBufferChangedEventHandler(string sessionID, Queue<FFMPEGFrameInfo> buffer);
        public delegate void PausedEventHandler(string sessionID, long millisecond);
        public delegate void ProcessingEventHandler(string sessionID, int percent, string videoFile, string audioFile, string outputFile);
        public delegate void RecordingEventHandler(string sessionID, long millisecond, long dataSize);
        public delegate void ResumeEventHandler(string sessionID);
        public delegate void SilentDurationChangedEventHandler(string sessionID, long millisecond);
        public delegate void StartEventHandler(string sessionID);
        public delegate void StoppedEventHandler(string sessionID, enumRecStopType stopType, string outputFile);
        public delegate void StoppingEventHandler(string sessionID, enumRecStopType stopType, string outputFile, long millisecond);
        public delegate void RecordingGameLostEventHandler(string sessionID, long lostTime);
        public delegate void AudioDeviceErrorEventHandler(string deviceName, bool isMicrophone, bool recStopped);
        public delegate void CreatFileFailedEventHandler(enumCreatFileError errorCode);
        public delegate void AdminReatartEventHandler();
        public delegate void HarewareCodeEventHandler();
        public delegate void GameInitEventHandler();

        public delegate void TakeScreenshotCompleteCallback(string fileName);
        public delegate void RegionResetCallBackHandler();        
        public delegate void RegionDrawCallBackHandler(int left, int top, int width, int height);

        public static event AudioDisplaySpectrumChangedEventHandler onAudioDisplaySpectrumChanged = delegate { };
        public static event RecordingGameLostEventHandler RecordingGameLost = delegate { };
        public static event AudioDeviceErrorEventHandler AudioDeviceError = delegate { };

        /// <summary>
        /// 通知上层清除区域虚线
        /// </summary>
        public static event RegionResetCallBackHandler RegionResetCallBack;
        /// <summary>
        /// 通知上层绘制区域虚线
        /// </summary>
        public static event RegionDrawCallBackHandler RegionDrawCallBack;

        /// <summary>
        /// 仅debug模式下使用
        /// </summary>
        /// <remarks>realtimeFPS: 实时帧率 averageFPS: 平均帧率</remarks>
        public static event FPSChangedEventHandler onFPSChanged= delegate { };
        /// <summary>
        /// 仅debug模式下使用
        /// </summary>
        public static event FramesBufferChangedEventHandler onFramesBufferChanged= delegate { };

        public static event PausedEventHandler onPaused= delegate { };
        public static event ProcessingEventHandler onProcessing= delegate { };
        public static event RecordingEventHandler onRecording= delegate { };
        public static event ResumeEventHandler onResume= delegate { };
        public static event SilentDurationChangedEventHandler onSilentDurationChanged= delegate { };
        public static event StartEventHandler onStart= delegate { };
        public static event StoppedEventHandler onStopped= delegate { };
        public static event StoppingEventHandler onStopping= delegate { };
        public static event ErrorEventHandler onError = delegate { };
        public static event DeletedEventHandler onDeleted = delegate { };
        public static event CompletedEventHandler onCompleted = delegate { };
        public static event CreatFileFailedEventHandler onCreatFile = delegate { };
        public static event AdminReatartEventHandler onAdminRestart = delegate { };
        public static event HarewareCodeEventHandler onHarewareCode = delegate { };
        public static event GameInitEventHandler onGameInit = delegate { };

        public static bool SupportHarewareCoding { get; private set; }

        public static Option Options { get; private set; } = new Option();

        public static enumRecStatus Status { get; private set; }

        public static string SessionID { get; private set; }
        
        internal static IntPtr RecInstance { get; private set; }
        
        private static System.Timers.Timer recordingDurationTimer = null;
        private static System.Timers.Timer recordingFreeSpaceTimer = null;

        private static long recTimeMS = 0;
        private static long durationTickCount = 0;
        private static bool gameLostNotified = false;

        private static ConcurrentDictionary<string, RecFileInfo> sessionRecFiles = new ConcurrentDictionary<string, RecFileInfo>();
        private static ConcurrentDictionary<string, ScreenshotFileInfo> screenshotFiles = new ConcurrentDictionary<string, ScreenshotFileInfo>();

        private static readonly WXCallBack wxCallback = OnWXCallBack;

        public static void Init(bool vidoe, bool audio, bool webcam) {
            Init(vidoe, audio, webcam, true);
        }

        public static void Init(bool vidoe, bool audio, bool webcam, bool autoDump) {
            try {
                if (!autoDump) {
                    WXCaptureLib.WXSetCrashDumpFlag(0);
                }
                WXCaptureLib.WXDeviceInit(Config.LogFileWXCapture);

                if (audio) {
                    RecAudio.Init();
                }
                if (vidoe) {
                    RecVideo.Init();
                }
                if (webcam) {
                    RecWebcam.Init();
                }
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Rec Init error: {0}.", ex.ToString()));
            }
        }

        public static void InitMirror() {
            InitMirror(true);
        }

        public static void InitMirror(bool autoDump) {
            try {
                if (!autoDump) {
                    WXCaptureLib.WXSetCrashDumpFlag(0);
                }
                WXCaptureLib.WXMediaSaveParam(0);
                WXCaptureLib.WXDeviceInitMirror(Config.LogFileWXCapture);
                WXCaptureLib.WXSetLogFile(Config.LogFileWXCapture);

                RecAudio.Init();
                RecVideo.Init();

                SupportHarewareCoding = WXCaptureLib.WXSupportHarewareCodec() != 0;
                Logger.Log.i(string.Format("SupportHarewareCoding: {0}.", SupportHarewareCoding));

            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Rec Init error: {0}.", ex.Message));
            }
        }

        public static void InitCamera() {
            RecWebcam.Init();
        }

        public static void InitAudio() {
            RecAudio.Init();
        }

        public static void SetCrashInfo(string exePath, string startParam) {
            WXCaptureLib.SetDumpCallBackExe(exePath, startParam);
        }

        public static void SaveStructFile(bool isSave) {
            WXCaptureLib.WXMediaSaveParam(isSave ? 1 : 0);
        }

        public static enumErrorCode Start() {
            return StartRec();
        }

        public static void InitGame() {
            Task.Factory.StartNew(() => {
                WXCaptureLib.WXGameInit();
                onGameInit();
            });
        }

        public static void InitHarewareCoding() {
            SupportHarewareCoding = WXCaptureLib.WXSupportHarewareCodec() != 0;
            Logger.Log.i(string.Format("SupportHarewareCoding: {0}.", SupportHarewareCoding));
            onHarewareCode();
        }

        public static enumErrorCode Start(Screen screenToRecord) {
            Options.FullScreenDevice = screenToRecord != null ? screenToRecord.DeviceName : "";
            return StartRec();
        }

        public static void Stop(int vip = 1) {
            if (RecInstance != IntPtr.Zero) {
                StopRec(enumRecStopType.Manually, vip);
            }
        }

        public static void Pause() {
            if (RecInstance == IntPtr.Zero) {
                return;
            }

            Logger.Log.i("Pause");
            if (Status != enumRecStatus.Recording) {
                Logger.Log.w("Recorder is not running, can't pause");
                return;
            }

            try {
                WXCaptureLib.WXCapturePause(RecInstance);
                Status = enumRecStatus.Paused;
                onPaused(SessionID, recTimeMS);
            }
            catch (Exception ex) {
                Logger.Log.e(ex);
            }
        }

        public static void Resume() {
            if (RecInstance == IntPtr.Zero) {
                return;
            }

            Logger.Log.i("Resume");
            try {
                WXCaptureLib.WXCaptureResume(RecInstance);
                Status = enumRecStatus.Recording;
                onResume(SessionID);
            }
            catch (Exception ex) {
                Logger.Log.e(ex);
            }
        }

        public static void Cancel() {
            if (RecInstance != IntPtr.Zero) {
                StopRec(enumRecStopType.Cancel);
            }
        }

        public static void Release() {
            RecWebcam.UnInit();
            WXCaptureLib.WXDeviceDeinit();
        }

        public static void ReleaseMirror() {
            WXCaptureLib.WXDeviceDeinitMirror();
        }
        
        public static void BeginWatermark() {
        }

        /// <summary>
        /// 动态调整录制位置和大小
        /// </summary>
        /// <param name="rect">新的矩形区域</param>
        /// <param name="keepAspect">是否保持宽高比</param>
        /// <param name="fillBackgroundColor">如果keepAspect为true的时候，需要将新的图片进行等比缩放，有黑边的话需要进行填充</param>
        public static void ChangeRegion(Rectangle rect, bool keepAspect, Color fillBackgroundColor) {
            if (RecInstance != IntPtr.Zero) {
                WXCaptureLib.WXCaptureChangeRect2(RecInstance, rect.X, rect.Y, rect.Width, rect.Height);
            }
        }

        public static void MoveRegionLocation(int x, int y) {
            if (RecInstance != IntPtr.Zero) {
                WXCaptureLib.WXCaptureChangeRect(RecInstance, x, y);
            }
        }

        /// <summary>
        /// 动态调整录制区域后，再次调用以同步设置
        /// </summary>
        /// <param name="rect">新的矩形区域</param>
        /// <remarks>本方法从未调用过是不会影响改变区域后的同步的</remarks>
        public static void ChangeRegionSynchronized(Rectangle rect) {
            if (RecInstance != IntPtr.Zero) {
                WXCaptureLib.WXCaptureSetRegion(rect.Left, rect.Top, rect.Width, rect.Height);
            }
        }

        /// <summary>
        /// 录制的时候截图
        /// </summary>
        /// <param name="filePath">输出路径</param>
        /// <param name="imageFormat">图片类型：png,jpg,bmp,gif,tiff 5种</param>
        /// <param name="takeScreenshotCompleteDelegate">截图完成后的回调</param>
        public static void TakeScreenshot(string filePath, string imageFormat, bool hasWaterMark, TakeScreenshotCompleteCallback takeScreenshotCallback) {
            if (string.IsNullOrEmpty(filePath) || imageFormat == null) {
                return;
            }

            if (RecInstance == IntPtr.Zero) {
                Logger.Log.w("Trying to taking screen shot while not recording.");
                return;
            }

            try {
                string tempPath = Path.GetTempFileName() + "." + imageFormat;
                screenshotFiles.TryAdd(tempPath, new ScreenshotFileInfo { FilePath = filePath, Format = imageFormat, CompleteCallback = takeScreenshotCallback });
                if (hasWaterMark) {
                    WXCaptureLib.WXCaptureGetPicture2(RecInstance, tempPath, 100);
                }
                else {
                    WXCaptureLib.WXCaptureGetPicture1(RecInstance, tempPath, 100);
                }
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Error while taking screenshot: {0}, {1}.", filePath, ex.Message));
            }
        }

        private static enumErrorCode ConvertWXCaptureErrorCode(int wxErrorCode) {
            switch (wxErrorCode) {
                case WXErrorCode.WX_ERROR_SUCCESS:
                    return enumErrorCode.NO_ERROR;
                case WXErrorCode.WX_ERROR_OPEN_FILE:
                    return enumErrorCode.CREATE_FILE_FAILED;
                default:
                    return enumErrorCode.UN_KNOWN;
            }
        }

        private static enumErrorCode StartRec() {
            if (RecInstance != IntPtr.Zero) {
                return enumErrorCode.REC_RUNNING;
            }

            enumErrorCode checkDeviceCode = CheckDevices();
            if (checkDeviceCode != enumErrorCode.NO_ERROR) {
                return checkDeviceCode;
            }

            enumErrorCode createDirCode = CreatOutputDir();
            if (createDirCode != enumErrorCode.NO_ERROR) {
                return createDirCode;
            }

            recTimeMS = 0;
            durationTickCount = 0;
            gameLostNotified = false;
            SessionID = CommUtilities.Utils.GetUniqID();
            sessionRecFiles.TryAdd(SessionID, new RecFileInfo { FilePath = Options.OutputFile, Finished = false });

            Status = enumRecStatus.Recording;

            if (Options.SetTemp) {
                WXCaptureLib.WXMediaSetTemp(1);
                Logger.Log.i("set temp file option open ");
            }
            else {
                WXCaptureLib.WXMediaSetTemp(0);
                Logger.Log.i("set temp file option close ");
            }

            Logger.Log.i("############## Record session start -> " + SessionID);
            Logger.Log.i(Options.ToString());
            WinAPI.PreventComputerFromEnteringSleep();
            onStart(SessionID);

            Task.Factory.StartNew(InterStartRec);
            return enumErrorCode.NO_ERROR;
        }

        private static void InterStartRec() {
            try {
                if (Options.RecordType == enumRecType.Window && WinAPI.IsWindow(Options.WindowHandleToRecord) == 0) {
                    Logger.Log.e(string.Format("Not record window {0} for it's not exist.", Options.WindowHandleToRecord));
                    return;
                }

                if (!CheckDriveSpaceOnStart()) {
                    Status = enumRecStatus.Stopped;
                    return;
                }
                
                StopAudioDeviceMonitoringBeforeRecording();

                if (Options.PlaySoundTip) {
                    Utils.PlayStartRecordingSound();
                }

                if (Options.RecordType == enumRecType.Region
                    || Options.RecordType == enumRecType.FullScreen
                     || Options.RecordType == enumRecType.AroundMouse) {
                    ExecTidyUpVideo(true);
                }

                if(Options.RecordType == enumRecType.Stream) {
                    WXCaptureLib.WXSetStreamRecord(1);
                    Logger.Log.i("Miracast start.");
                }

                int wxErrorCode = 0;
                TWXCaptureConfig param = WXCaptureLibEx.DefaultConfig;
                Options.SetToWXCaptureParam(ref param);
                param.CallBack = wxCallback;
                RecAudio.SetRecAudioFormats(Options.AudioInput, (int)Options.SystemAudioFormat, (int)Options.MicrophoneAudioFormat);

                IntPtr ptr = Marshal.AllocHGlobal(Marshal.SizeOf(param));
                Marshal.StructureToPtr(param, ptr, true);
                try {
                    if (Options.IsNeedTsEncrypt)
                        RecInstance = WXCaptureLibEx.WXCaptureStartExt2(param, ref wxErrorCode);
                    else
                        RecInstance = WXCaptureLibEx.WXCaptureStartExt(ptr, ref wxErrorCode);
                }
                finally {
                    Marshal.FreeHGlobal(ptr);
                    if (param.TextWaterMark.HFont != IntPtr.Zero) {
                        WinAPI.DeleteObject(param.TextWaterMark.HFont);
                        param.TextWaterMark.HFont = IntPtr.Zero;
                    }
                    if (!string.IsNullOrEmpty(param.ImageWaterMark.FileName)) {
                        Utils.TryDeleteFile(param.ImageWaterMark.FileName);
                    }
                }

                if (RecInstance == IntPtr.Zero) {
                    Logger.Log.e("Rec", "StartRec", string.Format("Start recordign failed: {0}.", wxErrorCode));
                    Status = enumRecStatus.Stopped;
                    enumErrorCode errorCode = ConvertWXCaptureErrorCode(wxErrorCode);
                    onError(SessionID, errorCode, "");
                    StartAudioDeviceMonitoringAfterRecording();
                    onAdminRestart();
                }
                else {
                    StartRecordingDurationTimer();
                    StartRecordingFreeSpaceTimer();
                    
                    onRecording(SessionID, recTimeMS, 0);
                    Logger.Log.i("Rec", "StartRec", "** Start OK **");
                }
            }
            catch (Exception ex) {
                Logger.Log.e(ex);
                Status = enumRecStatus.Stopped;
                onError(SessionID, enumErrorCode.EXCEPTION, "InterStartRec:" + ex.Message);
                StartAudioDeviceMonitoringAfterRecording();
                onAdminRestart();
            }
        }
        
        private static void StopAudioDeviceMonitoringBeforeRecording() {
            if (Options.AudioInput == enumAudioInput.Both) {
                RecAudio.StopMonitorDeviceVolume(Options.AudioSystemDevice, false);
                RecAudio.StopMonitorDeviceVolume(Options.AudioMicrophoneDevice, false);
            }
            else if (Options.AudioInput == enumAudioInput.System) {
                RecAudio.StopMonitorDeviceVolume(Options.AudioSystemDevice, false);
            }
            else if (Options.AudioInput == enumAudioInput.Microphone) {
                RecAudio.StopMonitorDeviceVolume(Options.AudioMicrophoneDevice, false);
            }
        }

        private static void StartAudioDeviceMonitoringAfterRecording() {
            if (Options.AudioInput == enumAudioInput.Both) {
                RecAudio.StartMonitorDeviceVolume(Options.AudioSystemDevice);
                RecAudio.StartMonitorDeviceVolume(Options.AudioMicrophoneDevice);
            }
            else if (Options.AudioInput == enumAudioInput.System) {
                RecAudio.StartMonitorDeviceVolume(Options.AudioSystemDevice);
            }
            else if (Options.AudioInput == enumAudioInput.Microphone) {
                RecAudio.StartMonitorDeviceVolume(Options.AudioMicrophoneDevice);
            }
        }

        private static void SetStartingAudioDeiveLevel() {
            bool systemAudio = Options.AudioInput == enumAudioInput.Both || Options.AudioInput == enumAudioInput.System;
            bool microphone = Options.AudioInput == enumAudioInput.Both || Options.AudioInput == enumAudioInput.Microphone;
            if (!systemAudio && !microphone) {
                return;
            }
            
            int count = 1;
            // 如何实现淡入效果？
            for (int i = 1; i <= count; ++i) {
                if (systemAudio) {
                    RecAudio.VolumeSet(enumAudioDeviceType.Playback, Options.AudioSystemLevel * i / count);
                }
                if (microphone) {
                    RecAudio.VolumeSet(enumAudioDeviceType.Recording, Options.AudioMicrophoneLevel * i / count);
                }
            }
        }

        private static void OnRegionReset(IntPtr ctx) {
            if (ctx == RecInstance) {
                RegionResetCallBack?.Invoke();
            }
        }

        private static void OnRegionDraw(IntPtr ctx, int left, int top, int width, int height) {
            if (ctx == RecInstance) {
                RegionDrawCallBack?.Invoke(left, top, width, height);
            }
        }
        
        private static void OnWXCallBack(IntPtr ptrSink, uint cbID, IntPtr cbData) {
            switch (cbID) {
                case WXCaptureLib.WX_EVENT_ID_CLOSE_FILE:
                    OnWxEventCloseFile(Marshal.PtrToStringUni(cbData));
                    break;
                case WXCaptureLib.WX_EVENT_ID_WINDOWCAPTURE_NO_DATA:
                    OnWxEventWindowCaptureNoData(cbData);
                    break;
                case WXCaptureLib.WX_EVENT_ID_SCRRENSHOT1:
                case WXCaptureLib.WX_EVENT_ID_SCRRENSHOT2:
                    Task.Factory.StartNew(str => OnWxEventScreenshot(str as string), Marshal.PtrToStringUni(cbData));
                    break;
                case WXCaptureLib.WX_EVENT_WASAPI_SYSTEM_STOP_MOVED:
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_INIT_ERROR:
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_INIT_ERROR_FOR_EXCLUSIVE:
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_STOP_EXCLUSIVE:
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_STOP_MOVED:
                    OnWxEventWasapi(cbID, Marshal.PtrToStringUni(cbData));
                    break;
                case WXCaptureLib.WX_EVENT_CREATE_FILE:
                case WXCaptureLib.WX_EVENT_NO_DATA:
                case WXCaptureLib.WX_EVENT_INTI_FFMPEG_MUXER:
                case WXCaptureLib.WX_EVENT_INTI_FFMPEG_ENCODER:
                case WXCaptureLib.WX_EVENT_FFMPEG_AVIO:
                case WXCaptureLib.WX_ERROR_FFMPEG_WRITE_HEADER:
                    Logger.Log.e("Rec", "OnWXCallBack", string.Format("Start recording failed: {0}.", cbID));
                    onCreatFile((enumCreatFileError)cbID);
                    break;
            }
        }

        private static void OnWxEventCloseFile(string v) {
           
            Task.Factory.StartNew(() => {
                string id = SessionID;
                RecFileInfo fileStatus;
                if (!string.IsNullOrEmpty(v)) {
                    if ( sessionRecFiles[SessionID].FilePath != v) {
                        id = sessionRecFiles.Where(i => i.Value.FilePath == v).FirstOrDefault().Key;
                    }
                }
                Logger.Log.i("Rec", "OnWxEventCloseFile", "1");
                Logger.Log.i("Rec", "id", id);
                foreach(var item in sessionRecFiles) {
                    Logger.Log.i("Rec", "idsession", item.ToString());
                }
                if (sessionRecFiles.TryGetValue(id, out fileStatus) && fileStatus != null) {
                    Logger.Log.i("Rec", "OnWxEventCloseFile", "2");
                    while (!fileStatus.Finished) {
                        Thread.Sleep(50);
                    }
                    #region MyRegion

                    StartAudioDeviceMonitoringAfterRecording();
                    Logger.Log.i("Rec", "StopRec", "Try getting video info...");
                    MediaInfo mi = new VideoUtils.VideoInfo(fileStatus.FilePath).GetMediaInfo(recTimeMS / 1000);
                    Logger.Log.i("Rec", "StopRec", "############## " + mi.ToString());

                    if (Options.SetTemp && (Path.GetExtension(mi.FileName) != ".flv" && Path.GetExtension(mi.FileName) != ".aac")) {
                        try {
                            string tempFileNameFlv = mi.FileName.Remove(mi.FileName.LastIndexOf(".")) + ".temp.flv";//以前底层会生成*.temp.flv和*.temp.aac，现只生成*.temp.flv
                            File.Delete(tempFileNameFlv);
                            Logger.Log.i("Rec", "StopRec", "Delete temp file.");
                        }
                        catch (Exception ex) {
                            Logger.Log.i("Delete temp file error." + ex.ToString());
                        }
                    }
                    #endregion
                    Logger.Log.i("Rec", "OnWxEventCloseFile", "3");

                    onCompleted(id, mi);
                }
            });
            if (!string.IsNullOrEmpty(v)) {
                Logger.Log.i(v);
                RecFileInfo file = sessionRecFiles.Values.FirstOrDefault(fi => fi.FilePath == v);
                if (file != null) {
                    file.Finished = true;
                }
            }
        }

        private static void OnWxEventWindowCaptureNoData(IntPtr cbData) {
            StopRec(enumRecStopType.WindowLost);
        }

        private static void OnWxEventScreenshot(string tempPath) {
            if (string.IsNullOrEmpty(tempPath)) {
                return;
            }

            ScreenshotFileInfo fileInfo;
            if (!screenshotFiles.TryGetValue(tempPath, out fileInfo) || fileInfo == null) {
                return;
            }

            string filePath = fileInfo.FilePath;
            string imageFormat = fileInfo.Format;
            TakeScreenshotCompleteCallback takeScreenshotCallback = fileInfo.CompleteCallback;
            try {
                imageFormat = imageFormat.ToLower();
                if (imageFormat.ToLower() == "jpg") {
                    new FileInfo(tempPath).MoveTo(filePath);
                    takeScreenshotCallback?.Invoke(filePath);
                    return;
                }

                FileStream fs = new FileStream(tempPath, FileMode.Open, FileAccess.Read);
                Image img = Image.FromStream(fs);

                try {
                    switch (imageFormat) {
                        case "png":
                            img.Save(filePath, ImageFormat.Png);
                            break;
                        case "gif":
                            img.Save(filePath, ImageFormat.Gif);
                            break;
                        case "tiff":
                            img.Save(filePath, ImageFormat.Tiff);
                            break;
                        case "bmp":
                        default:
                            img.Save(filePath, ImageFormat.Bmp);
                            break;
                    }
                }
                catch (Exception ex) {
                    Logger.Log.e(string.Format("Error while saving screen shot file: {0}, {1}.", filePath, ex.Message));
                }

                fs.Close();
                File.Delete(tempPath);

                if (File.Exists(filePath)) {
                    takeScreenshotCallback?.Invoke(filePath);
                }
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Error while taking screenshot: {0}, {1}.", filePath, ex.Message));
            }
        }

        private static void OnWxEventWasapi(uint cbID, string deviceName) {
            deviceName = deviceName ?? "";
            switch (cbID) {
                //case WXCaptureLib.WX_EVENT_WASAPI_SYSTEM_INIT_ERROR:
                //    Logger.Log.w(string.Format("OnWxEventWasapi: {0}, {1}.", "SYSTEM_INIT_ERROR", deviceName));
                //    AudioDeviceError(deviceName, false, false);
                //    break;
                //case WXCaptureLib.WX_EVENT_WASAPI_SYSTEM_INIT_ERROR_FOR_EXCLUSIVE:
                //    Logger.Log.w(string.Format("OnWxEventWasapi: {0}, {1}.", "SYSTEM_INIT_ERROR_FOR_EXCLUSIVE", deviceName));
                //    AudioDeviceError(deviceName, false, false);
                //    break;
                //case WXCaptureLib.WX_EVENT_WASAPI_SYSTEM_STOP_EXCLUSIVE:
                //    Logger.Log.e(string.Format("OnWxEventWasapi: {0}, {1}.", "SYSTEM_STOP_EXCLUSIVE", deviceName));
                //    //StopRec(enumRecStopType.ErrorOccured);
                //    AudioDeviceError(deviceName, false, false);
                //    break;
                case WXCaptureLib.WX_EVENT_WASAPI_SYSTEM_STOP_MOVED:
                    Logger.Log.e(string.Format("OnWxEventWasapi: {0}, {1}.", "SYSTEM_STOP_MOVED", deviceName));
                    //StopRec(enumRecStopType.ErrorOccured);
                    AudioDeviceError(deviceName, false, false);
                    break;
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_INIT_ERROR:
                    Logger.Log.w(string.Format("OnWxEventWasapi: {0}, {1}.", "MIC_INIT_ERROR", deviceName));
                    AudioDeviceError(deviceName, true, false);
                    break;
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_INIT_ERROR_FOR_EXCLUSIVE:
                    Logger.Log.w(string.Format("OnWxEventWasapi: {0}, {1}.", "MIC_INIT_ERROR_FOR_EXCLUSIVE", deviceName));
                    AudioDeviceError(deviceName, true, false);
                    break; 
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_STOP_EXCLUSIVE:
                    Logger.Log.e(string.Format("OnWxEventWasapi: {0}, {1}.", "MIC_STOP_EXCLUSIVE", deviceName));
                    //StopRec(enumRecStopType.ErrorOccured);
                    AudioDeviceError(deviceName, true, false);
                    break;
                case WXCaptureLib.WX_EVENT_WASAPI_MIC_STOP_MOVED:
                    Logger.Log.e(string.Format("OnWxEventWasapi: {0}, {1}.", "MIC_STOP_MOVED", deviceName));
                    //StopRec(enumRecStopType.ErrorOccured);
                    AudioDeviceError(deviceName, true, false);
                    break;
                default:
                    break;
            }
        }

        private static void StartRecordingDurationTimer() {
            if (recordingDurationTimer == null) {
                recordingDurationTimer = new System.Timers.Timer { Interval = 500 };
                recordingDurationTimer.Elapsed += OnRecordingDurationTick;
            }
            recordingDurationTimer.Start();
        }
        
        private static void StartRecordingFreeSpaceTimer() {
            if (recordingFreeSpaceTimer == null) {
                recordingFreeSpaceTimer = new System.Timers.Timer { Interval = 30000 };
                recordingFreeSpaceTimer.Elapsed += OnRecordingFreeSpaceTick;
            }
            recordingFreeSpaceTimer.Start();
        }

        private static void OnRecordingDurationTick(object sender, System.Timers.ElapsedEventArgs e) {
            recordingDurationTimer.Enabled = false;

            ++durationTickCount;

            try {
                if (Status == enumRecStatus.Recording && RecInstance != IntPtr.Zero) {
                    recTimeMS = WXCaptureLib.WXCaptureGetTime(RecInstance);
                    if (Options.StopType == enumRecStopType.ForDuration) {
                        if (recTimeMS >= Options.StopForDuration && Options.StopForDuration > 0) {
                            StopRec(enumRecStopType.ForDuration);
                            return;
                        }
                    }
                    else if (Options.StopType == enumRecStopType.AtTime) {
                        if (DateTime.Now.Ticks >= Options.StopAtTime.Ticks) {
                            StopRec(enumRecStopType.AtTime);
                            return;
                        }
                    }

                    long videoSize = 0;
                    if (Options.RecordType != enumRecType.Audio) {
                        videoSize = WXCaptureLib.WXCaptureGetVideoSize(RecInstance);
                    }
                    long audioSize = 0;
                    if (Options.AudioInput != enumAudioInput.None) {
                        audioSize = WXCaptureLib.WXCaptureGetAudioSize(RecInstance);
                    }
                    onRecording(SessionID, recTimeMS, videoSize + audioSize);

                    if (Options.RecordType == enumRecType.Game) {
                        long time = WXCaptureLib.WXCaptureGetVideoTimeOut();
                        if (time > 3000) {
                            if (Options.GameLostAutoStop) {
                                StopRec(enumRecStopType.GameStop);
                            }
                            else if (!gameLostNotified) {
                                gameLostNotified = true;
                                RecordingGameLost(SessionID, time);
                            }
                        }
                    }
                }
            }
            catch (Exception ex) {
                string x = ex.ToString();
            }
            finally {
                if (Status == enumRecStatus.Recording || Status == enumRecStatus.Paused) {
                    recordingDurationTimer.Enabled = true;
                }
            }
        }
        
        private static void OnRecordingFreeSpaceTick(object sender, ElapsedEventArgs e) {
            recordingFreeSpaceTimer.Enabled = false;

            try {
                long requiredFreeSpace = 100L * 1024 * 1024;
                long availableFreeSpace;
                string driveName;
                if (!CheckIfFreeSpace(requiredFreeSpace, out availableFreeSpace, out driveName)) {
                    string availableFreeSpaceFomatted = CommUtilities.Utils.FormatSizeFromByte(availableFreeSpace);
                    string needFreeSpaceFomatted = CommUtilities.Utils.FormatSizeFromByte(requiredFreeSpace);
                    Logger.Log.w(string.Format("There is not enough free space on partition {0}. A total of {1} of free disk space is required.", driveName, needFreeSpaceFomatted));
                    StopRec(enumRecStopType.ErrorOccured);
                    onError(SessionID, enumErrorCode.NO_ENOUGH_FREE_SPACE, string.Format("{0} ({1}/{2})", driveName, availableFreeSpaceFomatted, needFreeSpaceFomatted));
                }
            }
            finally {
                if (Status == enumRecStatus.Recording || Status == enumRecStatus.Paused) {
                    recordingFreeSpaceTimer.Enabled = true;
                }
            }
        }

        private static void StopRec(enumRecStopType stopType,int vip = 1) {
            if (Status == enumRecStatus.Stopping || Status == enumRecStatus.Stopped) {
                return;
            }

            Status = enumRecStatus.Stopping;
            
            recordingDurationTimer?.Stop();
            recordingFreeSpaceTimer?.Stop();

            string stoppingSession = SessionID;
            Logger.Log.i("Rec", "StopRec", string.Format("Stopping {0}, {1}.", stoppingSession, stopType.ToString()));


            long time = WXCaptureLib.WXCaptureGetTime(RecInstance);
            onStopping(stoppingSession, stopType, Options.OutputFile, time);

            Task.Factory.StartNew(() => {
                try {
                    if (vip == 0) {
                        WXCaptureLib.WXCaptureStopEx(RecInstance, vip);
                    }
                    else {
                        WXCaptureLib.WXCaptureStop(RecInstance);
                    }
                    if (Options.RecordType == enumRecType.Stream) {
                        WXCaptureLib.WXSetStreamRecord(0);
                        Logger.Log.i("MiracastStop.");
                    }
                }
                catch (Exception ex) {
                    Logger.Log.e("Rec", "StopRec", string.Format("Rec.Stop error: {0}.", ex.Message));
                    onError(stoppingSession, enumErrorCode.EXCEPTION, "StopRec:" + ex.Message);
                }
                RecInstance = IntPtr.Zero;
                Status = enumRecStatus.Stopped;
                if (Options.RecordType == enumRecType.Region
       || Options.RecordType == enumRecType.FullScreen
        || Options.RecordType == enumRecType.AroundMouse) {
                    ExecTidyUpVideo(false);
                }
                onStopped(stoppingSession, stopType, Options.OutputFile);
                SessionID = stoppingSession;

                //if (stopType != enumRecStopType.Cancel) {
                //    Task.Factory.StartNew(() => {
                //        RecFileInfo fileStatus;
                //        Thread.Sleep(30000);
                //        if (sessionRecFiles.TryGetValue(stoppingSession, out fileStatus) && fileStatus != null) {
                //            while (!fileStatus.Finished) {
                //                Thread.Sleep(50);
                //            }
                //            StartAudioDeviceMonitoringAfterRecording();

                //            Logger.Log.i("Rec", "StopRec", "Try getting video info...");
                //            MediaInfo mi = new VideoUtils.VideoInfo(fileStatus.FilePath).GetMediaInfo(recTimeMS / 1000);
                //            Logger.Log.i("Rec", "StopRec", "############## " + mi.ToString());

                //            if (Options.SetTemp && (Path.GetExtension(mi.FileName) != ".flv" && Path.GetExtension(mi.FileName) != ".aac")) {
                //                try {
                //                    string tempFileNameFlv = mi.FileName.Remove(mi.FileName.LastIndexOf(".")) + ".temp.flv";//以前底层会生成*.temp.flv和*.temp.aac，现只生成*.temp.flv
                //                    File.Delete(tempFileNameFlv);
                //                    Logger.Log.i("Rec", "StopRec", "Delete temp file.");
                //                }
                //                catch(Exception ex) {
                //                    Logger.Log.i("Delete temp file error." + ex.ToString());
                //                }
                //            }
                //            onCompleted(stoppingSession, mi);

                //        }
                //    });
                //}
            });
        }

        private static void ExecTidyUpVideo(bool isSet) {
            if (isSet) {
                if (Options.HideDesktop) {
                    TidyUpVideo.HideDesktop();
                }
                if (Options.HideTaskBar) {
                    TidyUpVideo.HideTaskBar();
                }
                if (Options.HideScreenSaver) {
                    TidyUpVideo.HideScreenSaver();
                }
            }
            else {
                if (Options.HideDesktop) {
                    TidyUpVideo.ShowDesktop();
                }
                if (Options.HideTaskBar) {
                    TidyUpVideo.ShowTaskBar();
                }
                if (Options.HideScreenSaver) {
                    TidyUpVideo.ShowScreenSaver();
                }
            }
        }

        private static enumErrorCode CheckDevices() {
            if (Options.RecordType == enumRecType.Webcam && Options.WebcamDevice == null) {
                Logger.Log.e("No webcam device selected to record webcam.");
                return enumErrorCode.NO_WEBCAM;
            }

            if (Options.AudioInput == enumAudioInput.System && RecAudio.PlaybackDeviceCount == 0) {
                Logger.Log.w("The playback audio device is not ready!");
                Options.AudioInput = enumAudioInput.None;
                if (Options.RecordType == enumRecType.Audio) {
                    Logger.Log.e("Can not record audio from system device.");
                    return enumErrorCode.AUDIO_RECORDER_NO_DEVICE;
                }
            }
            else if (Options.AudioInput == enumAudioInput.Microphone && RecAudio.RecordingDeviceCount == 0) {
                Logger.Log.w("The microphone audio device is not ready!");
                Options.AudioInput = enumAudioInput.None;
                if (Options.RecordType == enumRecType.Audio) {
                    Logger.Log.e("Can not record audio from microphone device.");
                    return enumErrorCode.AUDIO_RECORDER_NO_DEVICE;
                }
            }
            else if (Options.AudioInput == enumAudioInput.Both) {
                if (RecAudio.AudioDevices.Length == 0) {
                    Logger.Log.w("The audio device is not ready!");
                    Options.AudioInput = enumAudioInput.None;
                    if (Options.RecordType == enumRecType.Audio) {
                        Logger.Log.e("Can not record audio from system or microphone device.");
                        return enumErrorCode.AUDIO_RECORDER_NO_DEVICE;
                    }
                }
                else if (RecAudio.PlaybackDeviceCount == 0) {
                    Logger.Log.w("The playback device is not ready!");
                    Options.AudioInput = enumAudioInput.Microphone;
                }
                else if (RecAudio.RecordingDeviceCount == 0) {
                    Logger.Log.w("The microphone device is not ready!");
                    Options.AudioInput = enumAudioInput.System;
                }
            }

            return enumErrorCode.NO_ERROR;
        }             

        private static enumErrorCode CreatOutputDir() {
            try {
                if (!string.IsNullOrEmpty(Options.OutputFile)) {
                    string dir = Path.GetDirectoryName(Options.OutputFile);
                    if (!Directory.Exists(dir)) {
                        Directory.CreateDirectory(dir);
                    }
                }
            }
            catch (Exception ex) {
                Logger.Log.e(string.Format("Failed to create dir for: {0}, {1}.", Options.OutputFile, ex.Message));
                onError(SessionID, enumErrorCode.EXCEPTION, "CreatOutputDir:" + ex.Message);
                return enumErrorCode.EXCEPTION;
            }

            return enumErrorCode.NO_ERROR;
        }

        private static bool CheckDriveSpaceOnStart() {
            long requiredFreeSpace = 10L * 1024 * 1024;
            long availableFreeSpace;
            string driveName;
            if (CheckIfFreeSpace(requiredFreeSpace, out availableFreeSpace, out driveName)) {
                return true;
            }

            string availableFreeSpaceFomatted = CommUtilities.Utils.FormatSizeFromByte(availableFreeSpace);
            string needFreeSpaceFomatted = CommUtilities.Utils.FormatSizeFromByte(requiredFreeSpace);
            Logger.Log.w(string.Format("There is not enough free space on partition {0}. A total of {1} of free disk space is required.", driveName, needFreeSpaceFomatted));
            onError(SessionID, enumErrorCode.NO_ENOUGH_FREE_SPACE, string.Format("{0} ({1}/{2})", driveName, availableFreeSpaceFomatted, needFreeSpaceFomatted));
            return false;
        }
        
        private static bool CheckIfFreeSpace(long requiredFreeSpace, out long availableFreeSpace, out string driveName) {
            availableFreeSpace = 0;
            driveName = "";
            if (requiredFreeSpace <= 0) {
                return true;
            }

            string outputDriveLetter = Path.GetPathRoot(Options.OutputFile).ToLower();
            foreach (DriveInfo drive in DriveInfo.GetDrives()) {
                if (drive.Name.ToLower() == outputDriveLetter) {
                    availableFreeSpace = drive.AvailableFreeSpace;
                    driveName = drive.Name;
                    return availableFreeSpace > requiredFreeSpace;
                }
            }

            return true;
        }

    }
}
