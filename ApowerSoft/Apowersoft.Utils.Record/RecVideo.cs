using System;
using System.Collections.Generic;
using System.Linq;
using System.Drawing;
using System.Windows.Forms;
using Apowersoft.Utils.Record.WXCapture;

namespace Apowersoft.Utils.Record {
    public static class RecVideo {

        private static int[] bitrates = new int[] { /*512, 1000, */1800, 2000, 3000, 4000, 5000, 8000, /* 10000, 12000, */15000, /*18000,*/ 20000 };
        public static int[] Bitrates {
            get { return bitrates; }
        }
        
        public static DefaultOption DefaultOptions {
            get { return DefaultOption.Instance; }
        }

        private static float[] frameRates = new float[] {1.0F, 5.0F, 10.0F, /*12.0F,*/ 15.0F, 20.0F, 24.0F, 25.0F, 30.0F, 50.0F, 60.0F };
        public static float[] FrameRates {
            get { return frameRates; }
        }

        private static float[] gifFrameRates = new float[] { 3.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F };
        public static float[] GifFrameRates {
            get { return gifFrameRates; }
        }

        private static string[] outputFormats = new string[] { "MP4", "WMV", "AVI", "MOV", "FLV", "MPEG", "VOB", "ASF", "TS", "GIF" };
        public static string[] OutputFormats {
            get { return outputFormats; }
        }

        private static string[] videoPresetRegions;
        public static string[] VideoPresetRegions {
            get {
                if (videoPresetRegions == null) {
                    Tuple<string, Size>[] regionStrs = {
                        new Tuple<string, Size>("144P 192x144 (4:3)", new Size(192, 144)),
                        new Tuple<string, Size>("224P 400x240 (16:9)", new Size(400, 240)),
                        new Tuple<string, Size>("240P 320x240 (4:3)", new Size(320, 240)),
                        new Tuple<string, Size>("270P 480x270 (16:9)", new Size(480, 270)),
                        new Tuple<string, Size>("360P 480x360 (4:3)", new Size(480, 360)),
                        new Tuple<string, Size>("360P 640x360 (16:9)", new Size(640, 360)),
                        new Tuple<string, Size>("480P 640x480 (4:3)", new Size(640, 480)),
                        new Tuple<string, Size>("480P 854x480 (16:9)", new Size(854, 480)),
                        new Tuple<string, Size>("720P 1280x720 (16:9)", new Size(1280, 720)),
                        new Tuple<string, Size>("1080P 1920x1080 (16:9)", new Size(1920, 1080)),
                        new Tuple<string, Size>("2160P 3840x2160 (16:9)", new Size(3840, 2160)),
                        new Tuple<string, Size>("4K 4096x2160 (19:10)", new Size(4096, 2160)),
                        new Tuple<string, Size>("iPhone4/4S 960x640 (3:2)", new Size(960, 640)),
                        new Tuple<string, Size>("iPhone5/5S 1136x640 (16:9)", new Size(1136, 640)),
                        new Tuple<string, Size>("iPhone6/7/8 1334x750 (16:9)", new Size(1334, 750)),
                        new Tuple<string, Size>("iPhone6/7/8 Plus 1920x1080 (16:9)", new Size(1920, 1080)),
                        new Tuple<string, Size>("iPhone X 2436x1125 (16:9)", new Size(2436, 1125)),
                        new Tuple<string, Size>("iPad/iPad2 1024x768 (4:3)", new Size(1024, 768)),
                        new Tuple<string, Size>( "iPad Retina 2048x1536 (4:3)", new Size(2048, 1536)) };

                    int screenWidth = Screen.PrimaryScreen.Bounds.Width;
                    int screenHeight = Screen.PrimaryScreen.Bounds.Height;
                    var query = from rs in regionStrs
                                where rs.Item2.Width <= screenWidth && rs.Item2.Height <= screenHeight
                                select rs.Item1;
                    videoPresetRegions = query.ToArray();
                }
                return videoPresetRegions;
            }
        }

        public static void Init() {
        }

        public static string[] GetVideoCodecs(string videoFormat) {
            switch (videoFormat.ToLower().Trim()) {
                case "mp4":
                case "ts":
                    return new[] { "H264", "MPEG4"};
                case "avi":
                    return new[] { "MSMPEG4v3", "MPEG4", "MSMPEG4v2" };
                case "flv":
                    return new[] { "FLV", "H264" };
                case "asf":
                    return new[] { "MPEG4" };
                case "mpeg":
                    return new[] { "MPEG2" };
                case "vob":
                    return new[] { "MPEG2" };
                case "wmv":
                    return new[] { "MSMPEG4v3", "WMV2", "WMV1" };
                case "mov":
                    return new[] { "H264" };
                case "gif":
                    return new[] { "" };
                default:
                    return new[] { "H264" };
            }
        }

        public class DefaultOption {

            internal static readonly DefaultOption Instance = new DefaultOption();

            public int Bitrate {
                get { return 4000; }
            }

            public float FrameRate {
                get { return 24f; }
            }

            public string VideoCodec {
                get { return "H264"; }
            }
        }
    }

}
