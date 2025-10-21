using System;
using System.IO;
using Apowersoft.Utils.Record.WXMediaInfo;

namespace Apowersoft.Utils.Record {
    public static class VideoUtils {

        public class VideoInfo {
            
            public string FilePath { get; set; } = string.Empty;

            public VideoInfo(string filePath) {
                this.FilePath = filePath;
            }

            public MediaInfo GetMediaInfo(long duration = 0) {
                MediaInfo m = new MediaInfo() { FileName = FilePath };
                try {
                    if (!System.IO.File.Exists(this.FilePath)) {
                        Logger.Log.e("VideoUtils.VideoInfo", "GetMediaInfo", string.Format("File doesn't exist ->{0}", FilePath));
                        return m;
                    }

                    int error = 0;
                    IntPtr ptr = WXMediaInfoLib.WXMediaInfoCreate(FilePath, ref error);
                    if (ptr == IntPtr.Zero) {
                        Logger.Log.e("VideoUtils.VideoInfo", "GetMediaInfo", string.Format("WXMediaInfoCreateW failed: {0}", FilePath, error));
                        return m;
                    }

                    m.SizeBytes = WXMediaInfoLib.WXMediaInfoGetFileSize(ptr);
                    if (m.SizeBytes <= 0) {
                        m.SizeBytes = TryGetFileSize(FilePath);
                    }
                    m.SizeString = Utils.FormatSizeFromByte(m.SizeBytes);

                    m.Duration = WXMediaInfoLib.WXMediaInfoGetFileDuration(ptr) / 1000;
                    if (m.Duration < 0) {
                        m.Duration = duration;
                    }
                    m.DurationString = CommUtilities.Utils.FormatTimeFromSecond2String((int)m.Duration);

                    m.AudioCount = WXMediaInfoLib.WXMediaInfoGetAudioChannelNumber(ptr);
                    m.VideoCount = WXMediaInfoLib.WXMediaInfoGetVideoChannelNumber(ptr);

                    WXMediaInfoLib.WXMediaInfoDestroy(ptr);
                }
                catch (Exception ex) {
                    Logger.Log.e("VideoUtils.VideoInfo", "GetMediaInfo", ex.ToString());
                }
                return m;
            }

            private static long TryGetFileSize(string filePath) {
                try {
                    if (File.Exists(filePath)) {
                        return new FileInfo(filePath).Length;
                    }
                }
                catch { }
                return 0;
            }
            
        }
    }
}
