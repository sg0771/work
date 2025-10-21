using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

using COLORREF = System.UInt32;

namespace Apowersoft.Utils.Record.WXCapture {

    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct TWXAudioConfig {
        /// <summary>
        /// 是否录制音频
        /// </summary>
        public int HasAudio;
        /// <summary>
        /// 播放设备GUID，-1表示不录
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string SystemDeviceGUID;
        /// <summary>
        /// 麦克风设备GUID，-1表示不录
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string MicDeviceGUID;
        /// <summary>
        /// "aac" "mp3" 等编码格式
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string Codec;
        /// <summary>
        /// 麦克风声音增强
        /// </summary>
        public int AGC;
        /// <summary>
        /// 麦克风降噪
        /// </summary>
        public int NS;
        
        /// <summary>
        /// 人声检测功能
        /// </summary>
        public int VAD; 
        /// <summary>
        /// 扬声器采集Level，100
        /// </summary>
        public int SystemLevel;
        /// <summary>
        /// MIC采集Level,100
        /// </summary>
        public int MicLevel;
        /// <summary>
        /// 采样频率，默认44100
        /// </summary>
        public int SampleRate;
        /// <summary>
        /// 声道，默认为2
        /// </summary>
        public int Channel;
        /// <summary>
        /// 音频码率
        /// </summary>
        public int Bitarte;
        /// <summary>
        /// 扬声器变调参数,100不变
        /// </summary>
        public int SystemScale;
        /// <summary>
        /// MIC变调参数,100不变
        /// </summary>
        public int MicScale;
    }


    [StructLayout(LayoutKind.Sequential, Pack = 1, CharSet = CharSet.Unicode)]
    internal struct TWXCaptureConfig {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string FileName;
        public WXCaptureMode Mode;
        public TWXAudioConfig AudioParam;
        public VideoDeviceParam VideoParam;
        //public TextWaterMarkParam TextWaterMark = new TextWaterMarkParam { UseTextWaterMark = 1, WaterMark = "456456465465", PosX = 200 , PosY = 400, FontName = "宋体", Delay = 0 , FontSize = 72, TextColor = 16777215 };
        public TextWaterMarkParam TextWaterMark;
        public ImageWaterMarkParam ImageWaterMark;
        public MouseParam MouseParam;
#if  X64
        public int test;
#endif
        public IntPtr PtrSink;
        public WXCallBack CallBack;

        /// <summary>
        /// 是否使用FLV缓存文件
        /// </summary>
        public int bUseTemp;
        /// <summary>
        /// 缓存文件名
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string OtherFileName;

        /// <summary>
        /// 图像缩放参数
        /// </summary>
        public int VideoScale;
        
        /// <summary>
        /// 片头文件， JPEG/PNG/GIF等
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string HeadImage;
        /// <summary>
        /// 片头长度
        /// </summary>
        public int nHeadDuration;
        /// <summary>
        /// 片尾文件， JPEG/PNG/GIF等
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string TrailImage; 
        /// <summary>
        /// 片尾长度
        /// </summary>
        public int TrailDuration;
    }

    internal static class WXCaptureLibEx {

        private static TWXCaptureConfig defaultConfig = new TWXCaptureConfig();
        public static TWXCaptureConfig DefaultConfig {
            get { return defaultConfig; }
        }

        static WXCaptureLibEx() {
            InitDefaultConfig();
        }

        /// <summary>
        /// 启动录屏 使用新的录制结构体的录制参数
        /// </summary>
        /// <param name="param">指向录屏参数对象的指针</param>
        /// <param name="errorCode">错误码</param>
        /// <returns>录屏对象标识</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXCaptureStartExt(IntPtr ptrTWXCaptureConfig, ref int error);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXCaptureStartExt2(TWXCaptureConfig CaptureConfig, ref int error);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void TWXCaptureConfigDefault(IntPtr ptrTWXCaptureConfig);


        private static void InitDefaultConfig() {
    
            int size = Marshal.SizeOf(defaultConfig);
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.StructureToPtr(defaultConfig, ptr, true);
            try {
                TWXCaptureConfigDefault(ptr);
                defaultConfig = (TWXCaptureConfig)Marshal.PtrToStructure(ptr, typeof(TWXCaptureConfig));
            }
            finally {
                Marshal.FreeHGlobal(ptr);
            }
        }

    }
}
