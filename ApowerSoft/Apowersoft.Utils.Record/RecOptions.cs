using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using System.IO;
using System.Text;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {
    public partial class Rec {
        public class MouseOption {

            public MouseOption() {
                LeftClickAnimationColor = Color.Blue;
                RightClickAnimationColor = Color.Red;
                MouseHotSpotColor = Color.Yellow;
            }

            /// <summary>
            /// record mouse cursor in the video. True by default.
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public bool Enable { get; set; } = true;

            /// <summary>
            /// display a small animation to indicate which mouse button has been clicked by user. False by default
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public bool AnimateMouseButtons { get; set; } = false;

            /// <summary>
            /// display a small animation (animated circles) when user clicks using the mouse in the output video. True by default
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public bool AnimateMouseClicks { get; set; } = true;

            /// <summary>
            /// defines the duration of the animation for mouse button clicks. Default duration is 1000 ms (1 sec).
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public int AnimationDuration { get; set; } = 1000;

            public Color LeftClickAnimationColor { get; set; } = Color.Blue;

            public Color RightClickAnimationColor { get; set; } = Color.Red;

            public float ClickAnimationOpacity { get; set; } = 1.0f;

            /// <summary>
            /// show/hide the semitrasnparent hotspot around the mouse cursor (to make it easier to follow the mouse on the output video). True by default.
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public bool ShowMouseHotSpot { get; set; } = true;

            /// <summary>
            /// color of the hotspot around the mouse. Default is Yellow.
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public Color MouseHotSpotColor { get; set; } = Color.Yellow;

            public float MouseHotSpotOpacity { get; set; } = 1.0f;

            /// <summary>
            /// sets radius size of hot spot around mouse cursor. 
            /// </summary>
            /// <value></value>
            /// <returns></returns>
            /// <remarks></remarks>
            public int MouseSpotRadius { get; set; } = 15;

            public int MouseAnimationRadius { get; set; } = 20;
        }

        public class Option : ICloneable {
            public Option() {
                // VBConversions Note: Non-static class variable initialization is below.  Class variables cannot be initially assigned non-static values in C#.
                RecordType = enumRecType.FullScreen;
                RecordRect = Rectangle.Empty;
                AudioInput = enumAudioInput.System;
                AudioQuality = enumAudioQuality.Standard;
                VideoCodec = RecVideo.DefaultOptions.VideoCodec;
                VideoBitrate = Convert.ToInt32(RecVideo.DefaultOptions.Bitrate);
                VideoFramerate = Convert.ToSingle(RecVideo.DefaultOptions.FrameRate);
                WindowHandleToRecord = IntPtr.Zero;
                StopType = enumRecStopType.Manually;
                Priority = enumPriority.Balance;
                MouseOptions = new MouseOption();
            }

            public bool IsRunning { get; set; } = false;
            public enumRecType RecordType { get; set; } = enumRecType.FullScreen;
            public Rectangle RecordRect { get; set; } = Rectangle.Empty;
            public string OutputFile { get; set; } = "";

            #region 水印设置

            public string WatermarkText { get; set; } = "";
            public Image WatermarkImage { get; set; } = null;
            public Rectangle WatermarkImageBounds { get; set; } = Rectangle.Empty;
            public Font WatermarkTextFont { get; set; } = null;
            public Point WatermarkTextPosition { get; set; } = new Point(10, 10);
            public Color WatermarkTextColor { get; set; } = Color.Red;
            public int WatermarkStartTime { get; set; } = 0;
            public int WatermarkStyle { get; set; } = 0;
            public int WatermarkFontSize { get; set; } = 28;
            public bool WatermarkHasBackground { get; set; } = true;

            #endregion

            #region 音频设置
            public int AudioFrequency { get; set; } = 44100;
            public string AudioFormat { get; set; } = "MP3";
            public string AudioCodec { get; set; } = "AAC";
            public enumAudioInput AudioInput { get; set; } = enumAudioInput.System;
            public enumAudioQuality AudioQuality { get; set; } = enumAudioQuality.Standard;
            public int AudioBitrate { get; set; } = 128000;
            public AudioDevice AudioSystemDevice { get; set; } = null;
            public AudioDevice AudioMicrophoneDevice { get; set; } = null;
            public int AudioSystemLevel { get; set; } = 100;
            public int AudioMicrophoneLevel { get; set; } = 100;

            public bool AudioEnableGAC { get; set; } = false;
            
            public bool AudioEnableSkipSilence { get; set; } = false;
            public int AudioDeleteSamllFileSeconds { get; set; } = 0;

            public bool AudioEnableDeleteSmallFile { get; set; } = false;
            public bool AudioEnableAutoSplitByInterval { get; set; } = false;
            public bool AudioEnableAutoSplitBySilence { get; set; } = false;
            /// <summary>
            /// 单位为秒
            /// </summary>
            public int AudioAutoSplitInterval { get; set; } = 0;
            /// <summary>
            /// 单位为毫秒
            /// </summary>
            public int AudioAutoSplitSilence { get; set; } = 0;

            public bool AudioMicrophoneBoost { get; set; } = false;

            public bool AudioMicrophoneDenoise { get; set; } = false;

            public enumAudioFormat SystemAudioFormat { get; set; } = enumAudioFormat.Default;

            public enumAudioFormat MicrophoneAudioFormat { get; set; } = enumAudioFormat.Default;

            #endregion

            #region 视频设置

            internal string VideoFormat { get; set; } = "mp4";
            public string VideoCodec { get; set; } = RecVideo.DefaultOptions.VideoCodec;
            public int VideoBitrate { get; set; } = RecVideo.DefaultOptions.Bitrate;
            public float VideoFramerate { get; set; } = RecVideo.DefaultOptions.FrameRate;
            public IntPtr WindowHandleToRecord { get; set; } = IntPtr.Zero;
            public bool CaptureBlt { get; set; } = false;
            public bool ForceHDC { get; set; } = false;
            public string FullScreenDevice { get; set; }
            public bool UseHardwareCoding { get; set; }
            public bool UseDxgi { get; set; }
            public bool DisableDxgi { get; set; }
            public bool ForceFps { get; set; }
            public bool AntiAliasing { get; set; }
            /// <summary>
            /// set if creat temp file
            /// </summary>
            public bool SetTemp { get; set; } = true;

            public bool FullscreenUseRect { get; set; } = false;

            #endregion

            #region 鼠标设置

            public MouseOption MouseOptions { get; private set; }

            #endregion

            #region 摄像头设置

            public WebCamDevice WebcamDevice { get; set; } = null;

            #endregion

            #region 其他设置
            
            public enumRecStopType StopType { get; set; } = enumRecStopType.Manually;
            /// <summary>
            /// 最小3秒钟（3000毫秒）
            /// </summary>
            public long StopForDuration { get; set; } = 0;
            public DateTime StopAtTime { get; set; } = DateTime.MaxValue;

            public bool HideDesktop { get; set; } = false;
            public bool HideTaskBar { get; set; } = false;
            public bool HideScreenSaver { get; set; } = false;
            public bool PlaySoundTip { get; set; } = false;

            /// <summary>
            /// 新版文件需要加密
            /// </summary>
            public bool IsNeedTsEncrypt { get; set; } = false;

            #endregion


         public enumPriority Priority { get; set; } = enumPriority.Balance;

            public int GifFrameRate { get; set; } = 5;
            
            public int GameProcessID { get; set; } = 0;

            public bool GameLostAutoStop { get; set; } = true;

            public dynamic Clone() {
                return MemberwiseClone();
            }

            internal void SetToWXCaptureParam(ref TWXCaptureConfig param) {
                param.FileName = OutputFile;
                param.Mode = Priority == enumPriority.Quality ? WXCaptureMode.MODE_BEST
                    : (Priority == enumPriority.Performance ? WXCaptureMode.MODE_FAST : WXCaptureMode.MODE_NORMAL);

                // Audio
                if (AudioInput != enumAudioInput.None) {
                    param.AudioParam.HasAudio = 1;
                    if (AudioInput == enumAudioInput.System || AudioInput == enumAudioInput.Both) {
                        param.AudioParam.SystemDeviceGUID = AudioSystemDevice?.GUID ?? RecAudio.DefaultPlaybackDevice?.GUID ?? "";
                    }
                    if (AudioInput == enumAudioInput.Microphone || AudioInput == enumAudioInput.Both) {
                        param.AudioParam.MicDeviceGUID = AudioMicrophoneDevice?.GUID ?? RecAudio.DefaultRecordingDevice?.GUID ?? "";
                    }
                    //param.AudioParam.Codec = AudioCodec;
                    param.AudioParam.AGC = AudioMicrophoneBoost ? 1 : 0;
                    param.AudioParam.NS = AudioMicrophoneDenoise ? 1 : 0;
                    param.AudioParam.Bitarte = AudioBitrate;
                    param.AudioParam.SampleRate = AudioFrequency;

                    bool systemAudio = Options.AudioInput == enumAudioInput.Both || Options.AudioInput == enumAudioInput.System;
                    param.AudioParam.SystemLevel = systemAudio ? AudioSystemLevel : 0;
                    bool microphone = Options.AudioInput == enumAudioInput.Both || Options.AudioInput == enumAudioInput.Microphone;
                    param.AudioParam.MicLevel = microphone ? AudioMicrophoneLevel : 0;
                }
                else {
                    param.AudioParam.HasAudio = 1;
                    param.AudioParam.SystemDeviceGUID = "";
                    param.AudioParam.MicDeviceGUID = "";
                    param.AudioParam.SystemLevel = 0;
                    param.AudioParam.MicLevel = 0;
                }
                
                // Video 
                param.VideoParam.FPS = VideoFramerate > 0 ? (int)VideoFramerate : 24;
                param.VideoParam.UseVideo = RecordType != enumRecType.Audio ? 1 : 0;
                if (RecordType != enumRecType.Audio) {
                    param.VideoParam.UseHardCode = Rec.SupportHarewareCoding && UseHardwareCoding ? 1 : 0;
                    param.VideoParam.DXGI = UseDxgi ? 1 : 0;
                    param.VideoParam.Codec = !string.IsNullOrEmpty(VideoCodec) ? VideoCodec : GetDefaultVideoCodec();
                    if (FullScreenDevice == "all" || FullScreenDevice == "FullScreen" || (FullScreenDevice != null && FullScreenDevice.Contains(";"))) {
                        param.VideoParam.DeviceName = FullScreenDevice;
                    }
                    else {
                        if (RecordType == enumRecType.FullScreen) {
                            param.VideoParam.DeviceName = !string.IsNullOrEmpty(FullScreenDevice) ? FullScreenDevice : Screen.PrimaryScreen.DeviceName;
                        }
                        else if (RecordType == enumRecType.Region || RecordType == enumRecType.AroundMouse) {
                            param.VideoParam.DeviceName = Screen.PrimaryScreen.DeviceName;
                        }
                    }
                    Logger.Log.i("SetToWXCaptureParam:" + param.VideoParam.DeviceName + " Type: " + RecordType);
                    param.VideoParam.Bitrate = VideoBitrate * 1000;

                    if (RecordType == enumRecType.Webcam) {
                        param.VideoParam.IsCamera = 1;
                        if (WebcamDevice != null) {
                            param.VideoParam.DeviceName = WebcamDevice.GUID;
                            param.VideoParam.VideoPreviewWindow = WebcamDevice.PreviewWindowHandle;
                            param.VideoParam.VideoCallback = WebcamDevice.PreviewDataCallbackHandler;
                            if (WebcamDevice.SelectedProperty != null) {
                                param.VideoParam.FPS = WebcamDevice.SelectedProperty.AverageFPS;
                                param.VideoParam.CameraWidth = WebcamDevice.SelectedProperty.FrameSize.Width;
                                param.VideoParam.CameraHeight = WebcamDevice.SelectedProperty.FrameSize.Height;
                            }
                        }
                    }
                    else if (RecordType == enumRecType.AirPlay || RecordType == enumRecType.Stream) {
                        param.VideoParam.IsCamera = 1;
                        param.VideoParam.DeviceName = "WX_AIRPLAY";
                        param.VideoParam.CameraWidth = RecordRect.Width;
                        param.VideoParam.CameraHeight = RecordRect.Height;
                    }
                    else if (RecordType == enumRecType.Game) {
                        param.VideoParam.IsCamera = 1;
                        param.VideoParam.DeviceName = "Game";
                        param.VideoParam.GameProcessID = GameProcessID;
                        param.VideoParam.CameraWidth = RecordRect.Width;
                        param.VideoParam.CameraHeight = RecordRect.Height;
                    }
                    else if (RecordType == enumRecType.Window) {
                        param.VideoParam.IsCamera = 2;
                        param.VideoParam.HwndCapture = WindowHandleToRecord;
                    }

                    if(param.VideoParam.DeviceName != "FullScreen") {
                        if (RecordType == enumRecType.Region || RecordType == enumRecType.AroundMouse) {
                            param.VideoParam.ForceHDC = ForceHDC ? 1 : 0;
                            param.VideoParam.UseRect = 1;
                        }
                        else if (RecordType == enumRecType.FullScreen) {
                            param.VideoParam.UseRect = FullscreenUseRect ? 1 : 0;
                        }
                        else {
                            param.VideoParam.UseRect = 0;
                        }

                        if (param.VideoParam.UseRect == 1) {
                            param.VideoParam.ScreenRect = new RECT() { Left = RecordRect.Left, Top = RecordRect.Top, Right = RecordRect.Right, Bottom = RecordRect.Bottom };
                        }
                    }
                    else {
                        param.VideoParam.UseRect = 0;
                    }

                    param.VideoParam.FollowMouse = RecordType == enumRecType.AroundMouse ? 1 : 0;
                    param.VideoParam.UseCaptureBlt = CaptureBlt ? 1 : 0;
                    param.VideoParam.ForceFps = ForceFps ? 1 : 0;
                    param.VideoParam.AntiAliasing = AntiAliasing ? 1 : 0;

                    // Text watermark
                    param.TextWaterMark.UseTextWaterMark = string.IsNullOrEmpty(WatermarkText) ? 0 : 1;
                    if (param.TextWaterMark.UseTextWaterMark == 1) {
                        param.TextWaterMark.WaterMark = WatermarkText;
                        param.TextWaterMark.TextColor = Utils.ColorToDWord(WatermarkTextColor);
                        param.TextWaterMark.PosX = WatermarkTextPosition.X;
                        param.TextWaterMark.PosY = WatermarkTextPosition.Y;

                        if (WatermarkTextFont == null) {
                            param.TextWaterMark.FontName = SystemFonts.CaptionFont.FontFamily.Name;
                            param.TextWaterMark.FontSize = (int)SystemFonts.CaptionFont.Size;
                        }
                        else {
                            param.TextWaterMark.FontName = WatermarkTextFont.FontFamily.Name;
                            param.TextWaterMark.FontSize = WatermarkFontSize;
                            Logger.Log.i("    " + param.TextWaterMark.FontSize.ToString());
                        }
                        param.TextWaterMark.Style = WatermarkStyle;
                    }

                    // Image watermark
                    param.ImageWaterMark.ImageWaterMark = WatermarkImage != null ? 1 : 0;
                    if (param.ImageWaterMark.ImageWaterMark == 1) {
                        if (!TrySetImageWaterMark(ref param.ImageWaterMark)) {
                            param.ImageWaterMark.ImageWaterMark = 0;
                        }
                    }
                    
                    WXCaptureLib.WXDisableDXGI(DisableDxgi ? 1 : 0);

                    // Mouse
                        if (RecordType == enumRecType.Region || RecordType == enumRecType.FullScreen || RecordType == enumRecType.AroundMouse) {
                        param.MouseParam.Used = MouseOptions.Enable ? 1 : 0;
                        if (MouseOptions.Enable) {
                            param.MouseParam.MouseHotdot = MouseOptions.ShowMouseHotSpot ? 1 : 0;
                            param.MouseParam.HotdotRadius = MouseOptions.MouseSpotRadius;
                            param.MouseParam.ColorMouse = Utils.ColorToDWord(MouseOptions.MouseHotSpotColor);
                            param.MouseParam.AlphaHotdot = MouseOptions.MouseHotSpotOpacity;
                            param.MouseParam.MouseAnimation = MouseOptions.AnimateMouseClicks ? 1 : 0;
                            param.MouseParam.AnimationRadius = MouseOptions.MouseAnimationRadius;
                            param.MouseParam.ColorLeft = Utils.ColorToDWord(MouseOptions.LeftClickAnimationColor);
                            param.MouseParam.ColorRight = Utils.ColorToDWord(MouseOptions.RightClickAnimationColor);
                            param.MouseParam.AlphaAnimation = MouseOptions.ClickAnimationOpacity;
                        }
                    }
                }
            }

            private bool TrySetImageWaterMark(ref ImageWaterMarkParam param) {
                Image originImage = WatermarkImage;
                if (originImage == null) {
                    return false;
                }

                try {
                    param.FileName = Path.GetTempFileName();
                    param.Alpha = 1.0f;
                    param.PosXImage = WatermarkImageBounds.Left;
                    param.PosYImage = WatermarkImageBounds.Top;
                    if (WatermarkImageBounds.Size == originImage.Size) {
                        originImage.Save(param.FileName);
                    }
                    else {
                        // 水印图片需要保证尺寸是4的倍数，避免发生裁剪
                        int widthFix = WatermarkImageBounds.Width ;
                        if (widthFix % 4 != 0) {
                            widthFix = (widthFix / 4 + 1) * 4;
                        }
                        int heightFix = WatermarkImageBounds.Height;
                        if (heightFix % 4 != 0) {
                            heightFix = (heightFix / 4 + 1) * 4;
                        }
                        using (Bitmap bmp = new Bitmap(widthFix, heightFix)) {
                            using (Graphics graphics = Graphics.FromImage(bmp)) {
                                graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
                                graphics.DrawImage(originImage, 0, 0, bmp.Width, bmp.Height);
                            }
                            bmp.Save(param.FileName);
                        }
                    }
                    return true;
                }
                catch (Exception ex) {
                    Logger.Log.e(string.Format("Error while setting image watermark, {0}.", ex.Message));
                }
                return false;
            }

            private string GetDefaultVideoCodec() {
                string ext = Path.GetExtension(OutputFile);
                if (!string.IsNullOrEmpty(ext)) {
                    string[] codecs = RecVideo.GetVideoCodecs(ext.Trim('.').ToLower());
                    if (codecs.Length > 0) {
                        return codecs[0];
                    }
                }
                return "";
            }

            public override string ToString() {
                StringBuilder sb = new StringBuilder();
                sb.Append("OutputFile: ");
                sb.Append(OutputFile.ToString());
                sb.Append("\nRecordType: ");
                sb.Append(RecordType.ToString());
                if (RecordType == enumRecType.FullScreen) {
                    sb.Append("\nScreenDevice: ");
                    sb.Append(FullScreenDevice ?? "Empty");
                }
                sb.Append("\nRecordRect: ");
                sb.Append(RecordRect.ToString());
                sb.Append("\nAudioInput: ");
                sb.Append(AudioInput.ToString());
                sb.Append("\nAudioQuality: ");
                sb.Append(AudioQuality.ToString());
                sb.Append("\nAudioMicrophoneBoost: ");
                sb.Append(AudioMicrophoneBoost.ToString());
                sb.Append("\nAudioMicrophoneDenoise: ");
                sb.Append(AudioMicrophoneDenoise.ToString());
                sb.Append("\nVideoCodec: ");
                sb.Append(VideoCodec.ToString());
                sb.Append("\nVideoBitrate: ");
                sb.Append(VideoBitrate.ToString());
                sb.Append("\nVideoFramerate: ");
                sb.Append(VideoFramerate.ToString());
                sb.Append("\nStopType: ");
                sb.Append(StopType.ToString());
                sb.Append("\nStopForDuration: ");
                sb.Append(StopForDuration.ToString());
                sb.Append("\nPriority: ");
                sb.Append(Priority.ToString());
                sb.Append("\nForceHDC: ");
                sb.Append(ForceHDC.ToString());
                sb.Append("\nForceFPS: ");
                sb.Append(ForceFps.ToString());
                sb.Append("\nCaptureBlt: ");
                sb.Append(CaptureBlt.ToString()); 
                sb.Append("\nUseHardwareCoding: ");
                sb.Append(UseHardwareCoding.ToString());
                sb.Append("\nUseDxgi: ");
                sb.Append(UseDxgi.ToString());
                return sb.ToString();
            }

        }
    }
}
