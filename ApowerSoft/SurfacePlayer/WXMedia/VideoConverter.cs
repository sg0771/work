using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Timers;
using System.Windows;
using static SurfacePlayer.WXMedia.WXMediaLib;

namespace SurfacePlayer.WXMedia {
    public class VideoConverter {
        private readonly VideoInfo info;
        private readonly string outputDir;
        private WXMediaLib.OnFfmpegVideoConvertDataHandler cb;
        private IntPtr convertID = IntPtr.Zero;
        private Timer convertTimer;
        private bool updatingState = false;

        public event Action<VideoConverter, long, long> ConvertTick;
        public Func<Section[]> GetSectionDataCallback { get; set; }
        public enum ConvertState { None, Stopped, Coverting, Completed, Errored, Canceled };
        public VideoConverter(VideoInfo info, string outputDir) {
            this.info = info;
            this.outputDir = outputDir;
            cb = OnFfmpegVideoConvertDataCallback;
            convertTimer = new Timer() { Interval = 1000 };
            convertTimer.Elapsed += OnConvertTimerElapsed;
        }

        private void OnConvertTimerElapsed(object sender, ElapsedEventArgs e) {
            if (convertID == IntPtr.Zero || updatingState) {
                return;
            }

            updatingState = true;
            try {
                UpdateState();
            } catch { }

            updatingState = false;
        }
        private long totalDuration = 0;
        private void UpdateState() {
            int state = WXMediaLib.WXFfmpegGetState(convertID);
            if (state == (int)WXMediaLib.FFMPEGError.PROCESS) {
                if (totalDuration == 0) {
                    totalDuration = WXMediaLib.WXFfmpegGetTotalTime(convertID);
                }
                ConvertTick?.Invoke(this, WXMediaLib.WXFfmpegGetCurrTime(convertID), totalDuration);
            } else {
                convertTimer.Stop();
                if (state == (int)WXMediaLib.FFMPEGError.OK) {
                    ConvertTick?.Invoke(this, totalDuration, totalDuration);
                    State = ConvertState.Completed;
                } else {
                    LastError = GetInterStateCode(state);
                    State = ConvertState.Errored;
                }
            }
        }
        private static string GetInterStateCode(int state) {
            foreach (FFMPEGError v in Enum.GetValues(typeof(FFMPEGError))) {
                if (state == (int)v) {
                    return "ERROR_" + v.ToString();
                }
            }
            return "GENERAL_ERROR " + state.ToString();
        }

        public string LastError { get; private set; } = "NONE";

        private ConvertState state = ConvertState.None;
        public ConvertState State {
            get { return state; }
            set {
                if (state != value) {
                    state = value;
                    //ConvertStateChanged(this);
                }
            }
        }
        public void StartConvert() {
            if (convertID != IntPtr.Zero) {
                return;
            }
            State = ConvertState.Coverting;
            Task.Factory.StartNew(delegate {
                try {
                    DoConvert();
                } catch{

                }
            });
        }
        private void DoConvert() {
            convertID = WXMediaLib.WXFfmpegParamCreate();
            if (convertID == IntPtr.Zero) {
                LastError = "CREATE_ERROR";
                State = ConvertState.Errored;
                return;
            }

            WXMediaLib.WXFfmpegParamSetVideoCB(convertID, cb);
            // WXConvertLib.WXFfmpegParamSetConvertTime(convertID, StartTime, EndTime - StartTime);
            //WXConvertLib.WXFfmpegParamSetCPUS(convertID, AppConfig.ConvertConcurrent == 1 ? 2 : 1);

            WXMediaLib.WXFfmpegParamSetVideoCodecStr(convertID, "libx264");
            WXMediaLib.WXFfmpegParamSetVideoFmtStr(convertID, "yuv420p");
            if (info.Width > 0 && info.Height > 0) {
                WXMediaLib.WXFfmpegParamSetVideoSize(convertID, info.Width / 4 * 4, info.Height / 4 * 4);
            }

            State = ConvertState.Coverting;

            convertTimer.Start();

            WXMediaLib.WXFfmpegConvertVideo(convertID, info.FilePath, outputDir + info.FileName + ".mp4", 1);
        }
        private void OnFfmpegVideoConvertDataCallback(IntPtr ptrCtx, 
            int width, int height, 
            IntPtr pData, IntPtr lineSize, long pts) {
            const int WX_PIX_FMT_YUV420 = 1;

            Section[] sectionData = GetSectionDataCallback();
            if (sectionData != null && sectionData.Length > 0) {
                List<Rect> rects = new List<Rect>();
                // 挑出时间范围包含当前时间的框框
                foreach (var section in sectionData) {
                    if (pts >= section.StartTimeMs && pts <= section.EndTimeMs) {
                        rects.AddRange(section.Rects);
                    }
                }

                var rectsInConvert = new int[rects.Count, 4];
                for (int i = 0; i < rects.Count; ++i) {
                    // 传入时针对视频原数据缩放
                    rectsInConvert[i, 0] = (int)(rects[i].Left * width);
                    rectsInConvert[i, 1] = (int)(rects[i].Top * height);
                    rectsInConvert[i, 2] = (int)(rects[i].Width * width);
                    rectsInConvert[i, 3] = (int)(rects[i].Height * height);
                }

                WXDelogosLib.WXDelogos(pData, lineSize, pData, lineSize, width, height, WX_PIX_FMT_YUV420, rectsInConvert, rects.Count, 4, 0);
            }
        }
    }
}
