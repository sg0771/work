using System;
using System.Runtime.InteropServices;

using COLORREF = System.UInt32;

namespace Apowersoft.Utils.Record.WXCapture {

    internal static class WXErrorCode {
        /// <summary>
        /// 常规成功
        /// </summary>
        public const int WX_ERROR_SUCCESS = 0;
        /// <summary>
        /// 常规失败
        /// </summary>
        public const int WX_ERROR_ERROR = (1 << 0);
        /// <summary>
        /// 严重错误,创建文件失败
        /// </summary>
        public const int WX_ERROR_OPEN_FILE = (1 << 1);
        /// <summary>
        /// 没有抓屏数据
        /// </summary>
        public const int WX_WARNING_NO_VIDEO_DISPLAY_DATA = (1 << 2);
        /// <summary>
        /// 没有摄像头数据
        /// </summary>
        public const int WX_WARNING_NO_VIDEO_CAREMA_DATA = (1 << 3);
        /// <summary>
        /// 没有录制视频数据
        /// </summary>
        public const int WX_WARNING_NO_VIDEO_DATA = (1 << 4);
        /// <summary>
        /// 没有图像水印
        /// </summary>
        public const int WX_WARNING_NO_IMAGE_WATERMARK_DATA = (1 << 5);
        /// <summary>
        /// 没有文字水印
        /// </summary>
        public const int WX_WARNING_NO_TEXT_WATERMARK_DATA = (1 << 6);
        /// <summary>
        /// 没有扬声器音频数据
        /// </summary>
        public const int WX_WARNING_NO_SOUND_SYSTEM_DATA = (1 << 7);
        /// <summary>
        /// 没有麦克风音频数据
        /// </summary>
        public const int WX_WARNING_NO_SOUND_MIC_DATA = (1 << 8);
        /// <summary>
        /// 没有录制音频数据
        /// </summary>
        public const int WX_WARNING_NO_SOUND_DATA = (1 << 9);
        /// <summary>
        /// 视频区域设置错误，抓取图像可能异常
        /// </summary>
        public const int WX_WARNING_VIDEO_RECT = (1 << 10);
        /// <summary>
        /// 指定设备名不存在
        /// </summary>
        public const int WX_ERROR_VIDEO_NO_DEVICE = (1 << 11);
        /// <summary>
        /// 设备不支持该格式参数 width/height/fps
        /// </summary>
        public const int WX_ERROR_VIDEO_NO_PARAM = (1 << 12);
        /// <summary>
        /// 视频设备打开失败,COM 操作失败
        /// </summary>
        public const int WX_ERROR_VIDEO_DEVICE_OPEN = (1 << 13);
        /// <summary>
        /// 扬声器录制打开失败
        /// </summary>
        public const int WX_ERROR_SOUND_SYSTEM_OPEN = (1 << 20);
        /// <summary>
        /// 扬声器录制打开失败
        /// </summary>
        public const int WX_ERROR_SOUND_NO_SYSTEM_DEVICE = (1 << 21);
        /// <summary>
        /// 麦克风录制打开失败
        /// </summary>
        public const int WX_ERROR_SOUND_MIC_OPEN = (1 << 22);
        /// <summary>
        /// 扬声器录制打开失败
        /// </summary>
        public const int WX_ERROR_SOUND_NO_MIC_DEVICE = (1 << 23);
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate void WXCallBack(IntPtr ptrSink, uint cbID, IntPtr cbData);

    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct AudioDeviceParam {
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
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    internal enum WXCaptureMode {
        /// <summary>
        /// 快捷模式, 不考虑固定时间戳，视频按照到达时间戳来编码
        /// </summary>
        MODE_FAST = 0,
        /// <summary>
        /// 均衡模式
        /// </summary>
        MODE_NORMAL = 1,
        /// <summary>
        /// 质量最好，插帧丢帧使生成文件的fps逼近设置值
        /// </summary>
        MODE_BEST = 2
    }

#if X86 //OK
    [StructLayout(LayoutKind.Explicit, CharSet = CharSet.Unicode)]
    internal struct VideoDeviceParam
    {
        /// <summary>
        /// 是否使用
        /// </summary>
        [FieldOffset(0)]
        public int UseVideo;

        /// <summary>
        /// 帧率
        /// </summary>
        [FieldOffset(4)]
        public int FPS;

        /// <summary>
        /// 是否硬编码
        /// </summary>
        [FieldOffset(8)]
        public int UseHardCode;


        /// <summary>
        /// "h264" "mpeg4" 等编码格式
        /// </summary>
        [FieldOffset(12), MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string Codec;

        /// <summary>
        /// bitrate or qp
        /// </summary>
        [FieldOffset(44)]
        public int Bitrate;

        /// <summary>
        /// 预览窗口句柄
        /// </summary>
        [FieldOffset(48)]
        public IntPtr VideoPreviewWindow;

        /// <summary>
        /// 数据回调
        /// </summary>
        [FieldOffset(52)]
        public VideoCallBack VideoCallback;

        /// <summary>
        /// 设备名字或者GUID
        /// </summary>
        /// <remarks>显示器使用枚举到的名字；摄像头使用枚举到的GUID值；Airplay使用 "WX_AIRPLAY"</remarks>
        [FieldOffset(56), MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string DeviceName;

        /// <summary>
        /// 0表示屏幕录制，1表示摄像头/AIRPLAY/游戏录制，2表示窗口录制
        /// </summary>
        [FieldOffset(576)]
        public int IsCamera;

        /// <summary>
        /// 摄像头尺寸
        /// </summary>
        [FieldOffset(580)]
        public int CameraWidth;



        /// <summary>
        /// 摄像头尺寸
        /// </summary>
        [FieldOffset(584)]
        public int CameraHeight;


        /// <summary>
        /// Win10 DXGI桌面采集
        /// </summary>
        [FieldOffset(588)]
        public int DXGI;

        /// <summary>
        /// 窗口录制中要录制的窗口的HWND
        /// </summary>
        [FieldOffset(588)]
        public IntPtr HwndCapture;

        /// <summary>
        /// 是否使用区域
        /// </summary>
        [FieldOffset(592)]
        public int UseRect;

        /// <summary>
        /// 屏幕区域 RECT对象
        /// </summary>
        [FieldOffset(596)]
        public RECT ScreenRect;

        /// <summary>
        /// 是否跟随鼠标
        /// </summary>
        [FieldOffset(612)]
        public int FollowMouse;



        /// <summary>
        /// 截图时是否使用CaptureBlt
        /// </summary>
        [FieldOffset(616)]
        public int UseCaptureBlt;

        /// <summary>
        /// 游戏录制线程ID
        /// </summary>
        [FieldOffset(616)]
        public int GameThreadID;

        /// <summary>
        /// 每次都重新GetDC或者 CreateDC
        /// </summary>
        [FieldOffset(620)]
        public int ForceHDC;

        /// <summary>
        /// 游戏录制进程ID
        /// </summary>
        [FieldOffset(620)]
        public int GameProcessID;

        /// <summary>
        /// 通过插值使得生成文件的帧率逼近预设值
        /// </summary>
        [FieldOffset(624)]
        public int ForceFps;

        /// <summary>
        /// 抗锯齿处理， H264 将使用 YUV444 编码
        /// </summary>
        [FieldOffset(628)]
        public int AntiAliasing;

    };


#else
    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct VideoDeviceParam
    {
        /// <summary>
        /// 是否使用
        /// </summary>
        public int UseVideo;

        /// <summary>
        /// 帧率
        /// </summary>
        public int FPS;

        /// <summary>
        /// 是否硬编码
        /// </summary>
        public int UseHardCode;

        //x64 填充
        public int test;

        /// <summary>
        /// "h264" "mpeg4" 等编码格式
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]
        public string Codec;

        /// <summary>
        /// bitrate or qp
        /// </summary>
        public int Bitrate;


        public int test2;


        /// <summary>
        /// 预览窗口句柄
        /// </summary>
        public IntPtr VideoPreviewWindow;

        /// <summary>
        /// 数据回调
        /// </summary>
        public VideoCallBack VideoCallback;

        /// <summary>
        /// 设备名字或者GUID
        /// </summary>
        /// <remarks>显示器使用枚举到的名字；摄像头使用枚举到的GUID值；Airplay使用 "WX_AIRPLAY"</remarks>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string DeviceName;

        /// <summary>
        /// 0表示屏幕录制，1表示摄像头/AIRPLAY/游戏录制，2表示窗口录制
        /// </summary>
        public int IsCamera;

        /// <summary>
        /// 摄像头尺寸
        /// </summary>
        public int CameraWidth;

        /// <summary>
        /// 摄像头尺寸
        /// </summary>
        public int CameraHeight;


        /// <summary>
        /// Win10 DXGI桌面采集
        /// </summary>
        public int DXGI;

        /// <summary>
        /// 窗口录制中要录制的窗口的HWND
        /// </summary>
        public IntPtr HwndCapture;

        /// <summary>
        /// 是否使用区域
        /// </summary>
        public int UseRect;

        public int test3;

        /// <summary>
        /// 屏幕区域 RECT对象
        /// </summary>
        public RECT ScreenRect;

        /// <summary>
        /// 是否跟随鼠标
        /// </summary>
        public int FollowMouse;


        /// <summary>
        /// 截图时是否使用CaptureBlt
        /// </summary>
        public int UseCaptureBlt;

        /// <summary>
        /// 游戏录制线程ID
        /// </summary>
        public int GameThreadID;

        /// <summary>
        /// 每次都重新GetDC或者 CreateDC
        /// </summary>
        public int ForceHDC;

        /// <summary>
        /// 游戏录制进程ID
        /// </summary>
        public int GameProcessID;

        /// <summary>
        /// 通过插值使得生成文件的帧率逼近预设值
        /// </summary>
        public int ForceFps;
        /// <summary>
        /// 抗锯齿处理， H264 将使用 YUV444 编码
        /// </summary>
        public int AntiAliasing;

        public int test4;
    };


#endif

    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    public struct TextWaterMarkParam {
        /// <summary>
        /// 是否使用文字水印
        /// </summary>
        public int UseTextWaterMark;
        /// <summary>
        /// 文字水印位置X
        /// </summary>
        public int PosX;
        /// <summary>
        /// 文字水印位置Y
        /// </summary>
        public int PosY;
        /// <summary>
        /// 延时使用水印,单位为秒
        /// </summary>
        public int Style;
        /// <summary>
        /// 文字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string WaterMark;
        /// <summary>
        /// 文字颜色
        /// </summary>
        public COLORREF TextColor;
        /// <summary>
        /// 背景颜色
        /// </summary>
        public COLORREF BackColor;
        /// <summary>
        /// 指定字体名
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string FontName;
        /// <summary>
        /// 指定字体大小
        /// </summary>
        public int FontSize;

#if  X64
        public int test;
#endif
        /// <summary>
        /// 通过HFont指针指定
        /// </summary>
        public IntPtr HFont;
    }


    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct ImageWaterMarkParam {
        /// <summary>
        /// 是否使用图像水印
        /// </summary>
        public int ImageWaterMark;
        /// <summary>
        /// 指定图像位置 x
        /// </summary>
        public int PosXImage;
        /// <summary>
        /// 指定图像位置 y
        /// </summary>
        public int PosYImage;
        /// <summary>
        /// 延时使用水印,单位为秒
        /// </summary>
        public int Delay;
        /// <summary>
        /// 不透明度，0.0-1.0越小越透明
        /// </summary>
        public float Alpha;
        /// <summary>
        /// 水印图像文件名
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string FileName;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct MouseParam {
        /// <summary>
        /// 是否录制鼠标，为0 时不录制鼠标
        /// </summary>
        public int Used;

        // 鼠标热点
        public int MouseHotdot;
        public int HotdotRadius;
        public COLORREF ColorMouse;
        /// <summary>
        /// 不透明度，0.0-1.0越小越透明
        /// </summary>
        public float AlphaHotdot;

        // 鼠标点击动画
        public int MouseAnimation;
        public int AnimationRadius;
        public COLORREF ColorLeft;
        public COLORREF ColorRight;

        /// <summary>
        /// 不透明度，0.0-1.0越小越透明
        /// </summary>
        public float AlphaAnimation;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LogParam {
        public int UseLog;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string LogFileName;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 4, CharSet = CharSet.Unicode)]
    internal struct WXCaptureParam {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string FileName;
        public WXCaptureMode Mode;
        public AudioDeviceParam AudioParam;
        public VideoDeviceParam VideoParam;
        public TextWaterMarkParam TextWaterMark;
        public ImageWaterMarkParam ImageWaterMark;
        public MouseParam MouseParam;
        public IntPtr PtrSink;
        public WXCallBack CallBack;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct SoundDeviceInfo {
        /// <summary>
        /// 设备名字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]

        public string Name;
        /// <summary>
        /// 端口名字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string GUID;

        /// <summary>
        /// 默认设备
        /// </summary>
        public int IsDefalut;

        /// <summary>
        /// 当前音量值
        /// </summary>
        public int CurLevel;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct CameraDataFormat {
        public int Width;
        public int Height;
        public int FPS;

        /// <summary>
        /// 采集格式
        /// </summary>
        public int mt;

        /// <summary>
        /// pmt?
        /// </summary>
        public int Index;

        public long AvgTimePerFrame;

        ///// <summary>
        ///// 采集格式名字
        ///// </summary>
        //[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 6)]
        //public string Fmt;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct MonitorInfo {
        /// <summary>
        /// 名字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string Name;

        /// <summary>
        /// 是否为主屏幕 
        /// </summary>
        public int IsPrimary;

        public int Left;
        public int Top;
        public int Width;
        public int Height;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    internal struct CameraInfo {
        /// <summary>
        /// 设备名字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string Name;

        /// <summary>
        /// 端口名字
        /// </summary>
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
        public string GUID;

        public int SizeFmt;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 260)]
        public CameraDataFormat[] ArrayFmt;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void VideoCallBack(IntPtr buff, int width, int height);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void WXRegionResetCallBack(IntPtr ctx);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void WXRegionDrawCallBack(IntPtr ctx, int left, int top, int width, int height);

    internal static class WXCaptureLib {
        /// <summary>
        /// 录制结束，后面参数是生成文件名
        /// </summary>
        public const uint WX_EVENT_ID_CLOSE_FILE = 0;
        /// <summary>
        /// 截图1成功，后面参数是生成文件名
        /// </summary>
        public const uint WX_EVENT_ID_SCRRENSHOT1 = 1;
        /// <summary>
        /// 截图2成功，后面参数是生成文件名
        /// </summary>
        public const uint WX_EVENT_ID_SCRRENSHOT2 = 2;
        /// <summary>
        /// 窗口录制模式没有录制到数据
        /// </summary>
        public const uint WX_EVENT_ID_WINDOWCAPTURE_NO_DATA = 5;

        /// <summary>
        /// HOOK成功
        /// </summary>
        public const uint WX_EVENT_HOOK_START = 10;
        /// <summary>
        /// HOOK结束
        /// </summary>
        public const uint WX_EVENT_HOOK_STOP = 11;
        /// <summary>
        /// HOOK EXE 名字,比如 War
        /// </summary>
        public const uint WX_EVENT_HOOK_EXE = 12;
        /// <summary>
        /// HOOK DLL 名字,比如 DX8  DX9 
        /// </summary>
        public const uint WX_EVENT_HOOK_DLL = 13;
        /// <summary>
        /// 截图宽度   void* cbData 是一个　int
        /// </summary>
        public const uint WX_EVENT_HOOK_WIDTH = 15;
        /// <summary>
        /// 截图高度   void* cbData 是一个　int
        /// </summary>
        public const uint WX_EVENT_HOOK_HEIGHT = 16;
        /// <summary>
        /// 判断创建录制文件路径是否成功   void* cbData 是一个　int
        /// </summary>
        public const uint WX_EVENT_CREATE_FILE = 17;

        //没有音视频数据输入，一般是参数错误
        public const uint WX_EVENT_NO_DATA = 18;

        //ffmpeg文件容器创建失败，可能是不支持该格式
        public const uint WX_EVENT_INTI_FFMPEG_MUXER = 19;

        //ffmpeg音视频编码器创建失败,有可能是分辨率过高不支持
        public const uint WX_EVENT_INTI_FFMPEG_ENCODER = 20;

        //ffmpeg文件IO失败，可能是目录或者权限等问题
        public const uint WX_EVENT_FFMPEG_AVIO = 21;

        //ffmpeg文件头写入失败，可能是文件容器不支持预设的音视频编码器
        //需要切换到适配比较好的MP4格式
        public const uint WX_ERROR_FFMPEG_WRITE_HEADER = 22;
      
        /// <summary>
        /// 扬声器事件 WASAPI设备被拔除而停止录制
        /// </summary>
        public const uint WX_EVENT_WASAPI_SYSTEM_STOP_MOVED = 23;

        /// <summary>
        /// MIC事件 WASAPI设备初始化失败,可能需要更新驱动
        /// </summary>
        public const uint WX_EVENT_WASAPI_MIC_INIT_ERROR = 30;
        /// <summary>
        /// MIC事件 WASAPI设备因为外部应用独占而初始化失败
        /// </summary>
        public const uint WX_EVENT_WASAPI_MIC_INIT_ERROR_FOR_EXCLUSIVE = 31;
        /// <summary>
        /// MIC事件 WASAPI设备被外部应用独占而停止录制
        /// </summary>
        public const uint WX_EVENT_WASAPI_MIC_STOP_EXCLUSIVE = 32;
        /// <summary>
        /// MIC事件 WASAPI设备被拔除而停止录制
        /// </summary>
        public const uint WX_EVENT_WASAPI_MIC_STOP_MOVED  =  33;


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXSetCrashDumpFlag(int b);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXDeviceInit(string key);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXDeviceDeinit();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXDeviceInitMirror(string key);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXDeviceDeinitMirror();

        /// <summary>
        /// 音频设备初始化
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXWasapiInit();
        /// <summary>
        /// 获取播放设备数量
        /// </summary>
        /// <returns>播放设备数量</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXWasapiGetRenderCount();

        /// <summary>
        /// 获取播放设备信息
        /// </summary>
        /// <param name="index">设备序号</param>
        /// <returns>设备信息 SoundDeviceInfo 对象指针</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXWasapiGetRenderInfo(int index);

        /// <summary>
        /// 获取录音设备数量
        /// </summary>
        /// <returns>录音设备数量</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXWasapiGetCaptureCount();

        /// <summary>
        /// 获取录音设备信息
        /// </summary>
        /// <param name="index">录音设备序号</param>
        /// <returns>设备信息 SoundDeviceInfo 对象指针</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXWasapiGetCaptureInfo(int index);

        /// <summary>
        /// 获取默认录屏参数
        /// </summary>
        /// <param name="param">WXCaptureParam对象指针用于保存获取的信息</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureDefaultParam(IntPtr wxCaptureParam);

        /// <summary>
        /// 启动录屏
        /// </summary>
        /// <param name="param">指向录屏参数对象的指针</param>
        /// <param name="errorCode">错误码</param>
        /// <returns>录屏对象标识</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXCaptureStart(IntPtr wxCaptureParam, ref int errorCode);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXCaptureStartWithLevel(IntPtr wxCaptureParam, ref int error, int systemLevel, int micLevel);

        /// <summary>
        /// 停止录屏
        /// </summary>
        /// <param name="ptr"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureStop(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureStopEx(IntPtr ptr, int bVip);

        /// <summary>
        /// 获取当前录制时间
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <returns>当前录制时间（毫秒）</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long WXCaptureGetTime(IntPtr ptr);


        /// <summary>
        /// 设置播放设备
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <param name="index">播放设备序号</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetSystemSound(IntPtr ptr, int index);

        /// <summary>
        /// 设置录音设备
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <param name="index">录音设备序号</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetMicSound(IntPtr ptr, int index);

        /// <summary>
        /// 获取播放设备音量
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <returns>音量值0-100</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXCaptureGetSystemLevel(IntPtr ptr);

        /// <summary>
        /// 获取录音设备音量
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <returns>音量值0-100</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXCaptureGetMicLevel(IntPtr ptr);

        /// <summary>
        /// 设置播放设备音量
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <param name="level">音量0-100</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetSystemLevel(IntPtr ptr, int level);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetSystemSilence(IntPtr ptr, int bSilence);

        /// <summary>
        /// 设置录音设备音量
        /// </summary>
        /// <param name="ptr">录屏对象</param>
        /// <param name="level">音量0-100</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetMicLevel(IntPtr ptr, int level);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetMicSilence(IntPtr ptr, int bSilence);

        /// <summary>
        /// 屏幕信息初始化
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXScreenInit();

        /// <summary>
        /// 获取屏幕个数
        /// </summary>
        /// <returns>屏幕个数</returns>
        /// <remarks>显示器设备，由于Win10 多显示器的不同分辨率问题，暂时处理两个设备</remarks>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXScreenGetCount();

        /// <summary>
        /// 获取屏幕信息
        /// </summary>
        /// <param name="index">屏幕序号</param>
        /// <returns>屏幕信息 MonitorInfo 对象指针</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXScreenGetInfo(int index);

        /// <summary>
        /// 暂停
        /// </summary>
        /// <param name="ptr">录屏对象标识</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCapturePause(IntPtr ptr);

        /// <summary>
        /// 恢复
        /// </summary>
        /// <param name="ptr">录屏对象标识</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureResume(IntPtr ptr);

        /// <summary>
        /// 改变录制区域
        /// </summary>
        /// <param name="ptr">录屏对象标识</param>
        /// <param name="x">X</param>
        /// <param name="y">Y</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureChangeRect(IntPtr ptr, int x, int y);

        /// <summary>
        /// UI把边框绘制好后，将边框RECT保存下来
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureSetRegion(int left, int top, int width, int height);

        /// <summary>
        /// 改变录制区域(加入改变区域大小)
        /// </summary>
        /// <param name="ptr">录屏对象标识</param>
        /// <param name="x">X</param>
        /// <param name="y">Y</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureChangeRect2(IntPtr ptr, int x, int y, int width , int height);

        /// <summary>
        /// 直接写入AVFrame数据
        /// </summary>
        /// <param name="ptr">录屏对象标识</param>
        /// <param name="avframe">AVFrame数据指针</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCaptureVirtualCameraWriteVideoData(IntPtr ptr, IntPtr avframePtr);

        /// <summary>
        /// 摄像头设备初始化
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCameraInit();

        /// <summary>
        /// 获取摄像头设备数目
        /// </summary>
        /// <returns>摄像头设备数目</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXCameraGetCount();

        /// <summary>
        /// 获取摄像头设备信息
        /// </summary>
        /// <param name="index">摄像头设备序号</param>
        /// <returns>摄像头设备信息 CameraInfo 对象指针</returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraGetInfo(int index);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithHwnd(string devGUID, int width, int height, int iFps, IntPtr hwnd, int Fixed);
        
        //扩展，增加水平翻转参数
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithHwndExt(string devGUID, int width, int height, int iFps, IntPtr hwnd, int Fixed, int bHFilp);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithSink(string devGUID, int width, int height, int iFps, VideoCallBack cb);

        //扩展，增加水平翻转参数
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithSinkExt(string devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp);
        //扩展2，回调rgb32数据
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithSinkExt2(string devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp);

        //** 创建一路摄像头采集并回调RGB32数据，可以进行数据处理
        //devGUID: 摄像头GUID值
        //width，height，fps: 摄像头采集参数 
        //bHFilp  :  0不变，1左右翻转，也可以在Open成功之后进行 SetHFilp操作
        //bOpenGL :  0默认输出，1可以进行OpenGL处理，设置1之后可以进行OpenGL参数调整
        //bRGB32  :  是否输出RGB32 数据，0输出YUV，1输出RGB32,运行过程中不可修改
        //cb      :  数据回调函数, 形式是 (uint8_t* buf,int width, int height),运行过程中不可修改
        //返回值  : 成功返回0， 是否通过 WXGetCameraErrorLast/WXGetCameraErrorLastString 获取失败信息
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXCameraOpenWithOpenGL(string devGUID, int width, int height, int iFps, int bHFilp, int bRGB32, VideoCallBack cb);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCameraSetting(IntPtr ptr, IntPtr hwnd);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCameraClose(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXCameraGetCurrDevice();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr WXGameInit();

        /// <summary>
        /// 在打开设备的情况下设置水平翻转
        /// </summary>
        /// <param name="ptr"></param>
        /// <param name="bHFilp"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXCameraSetHFilp(IntPtr ptr, int bHFilp);

        //设置OpenGL参数
        //设置参数，成功返回0，失败返回-1
        //参数说明
        // "blur" 表示 磨皮系数
        // "whiten" 美白系数
        // "brightness" 整体增加亮度
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXCameraSetParam(IntPtr ptr, string wszName, float value);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXCameraSetBgImage(IntPtr ptr, string wszName);

        /// <summary>
        /// 游戏的ProcessID
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXGameHookPID();

        /// <summary>
        /// 游戏的ThreadID
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXGameHookTID();

        /// <summary>
        /// 录屏中截图，不带水印
        /// </summary>
        /// <param name="ptr"></param>
        /// <param name="filePath"></param>
        /// <param name="quality"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXCaptureGetPicture1(IntPtr ptr, string filePath, int quality);

        /// <summary>
        /// 录屏中截图，带水印
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXCaptureGetPicture2(IntPtr ptr, string filePath, int quality);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXSetLogFile(string logFileName);


        /// <summary>
        /// 打开音频设备
        /// </summary>
        /// <param name="strGuid">GUID</param>
        /// <param name="isSystem">1 为扬声器；0 为麦克风</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void AudioDeviceOpen(string strGuid, int isSystem);

        /// <summary>
        /// 非录制过程中获取设备音量
        /// </summary>
        /// <param name="isSystem">1 为扬声器；0 为麦克风</param>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int AudioDeviceGetVolume(int isSystem);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void AudioDeviceClose(int isSystem);

        /// <summary>
        /// 获取录屏中视频数据的大小
        /// </summary>
        /// <param name="ptr">录屏实例</param>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long WXCaptureGetVideoSize(IntPtr ptr);

        /// <summary>
        /// 获取录屏中音频数据的大小
        /// </summary>
        /// <param name="ptr">录屏实例</param>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern long WXCaptureGetAudioSize(IntPtr ptr);

        /// <summary>
        /// 是否支持硬编码
        /// </summary>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXSupportHarewareCodec();

        /// <summary>
        /// 设置截取波形长度
        /// </summary>
        /// <param name="n"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void WXSetWaveLength(int len);

        /// <summary>
        /// 获取波形数据
        /// </summary>
        /// <param name="pData">int数组指针，取值范围是 0-32768</param>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int WXGetWaveData(IntPtr pData);

        /// <summary>
        /// 是否开启防闪退， 0关闭，1开启 默认1
        /// </summary>
        /// <param name="bTs"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXMediaSetTemp(int bTs);

        /// <summary>
        /// 是否保存录制参数小文件
        /// </summary>
        ///int g_bSaveParam = 1 开启;
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXMediaSaveParam(int bSave);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameStart();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameCreate(IntPtr ptrSink, WXCallBack cb);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGameGetWidth();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGameGetHeight();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameStop();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameSetPreview(IntPtr hwnd);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameDestory();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGameHookType();

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet =CharSet.Unicode)]
        public extern static void WXGameDrawString(string str, int x, int y); 

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static void WXGameDrawImage(string str, int x, int y);

        /// <summary>
        /// 设置输出文字字体颜色和背景颜色
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXGameSetDrawColor(int colorText, int colorBk);

        /// <summary>
        /// 设置输出文字字体
        /// </summary>
        /// <param name="strFont"></param>
        /// <param name="fontSize"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static void WXGameSetDrawFont(string strFont, int fontSize);
        /// <summary>
        /// 是否还hook着游戏，0为无游戏（用于检测游戏是否停止，石锤对大多数游戏无效）
        /// </summary>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGameHooked();

        /// <summary>
        /// 计算当前时间和上一帧视频编码的时间间隔，如果超过一定时间比如3s，可以认为外部数据源采集结束。
        /// </summary>
        /// <returns></returns>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXCaptureGetVideoTimeOut();
        
        /// <summary>
        /// 设置麦克风采集 VAD 检测功能
        /// </summary>
        /// <param name="b">1开启；0关闭</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXCaptureSetVAD(int b);

        /// <summary>
        /// 设置扬声器采集格式
        /// </summary>
        /// <param name="i">0 默认格式，1 PCM16格式，2 FLOAT32格式</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXSetSystemSoundType(int i); 

        /// <summary>
        /// 设置MIC采集格式
        /// </summary>
        /// <param name="i">0 默认格式，1 PCM16格式，2 FLOAT32格式</param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXSetMicSoundType(int i);

        /// <summary>
        /// 设置crashexport.exe的路径和启动参数
        /// </summary>
        /// <param name="strExe"></param>
        /// <param name="strParam"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet =CharSet.Unicode)]
        public extern static void SetDumpCallBackExe(string strExe, string strParam);

        /// <summary>
        /// 得到网络流的宽
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGetNetStreamWidth();

        /// <summary>
        /// 得到网络流的高
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXGetNetStreamHeight();

        /// <summary>
        /// type = 0 通知结束网络流; type = 1 通知开始网络流
        /// </summary>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXSetStreamRecord(int type);

        /// <summary>
        /// 禁用DXGI录屏功能，只能用GDI
        /// </summary>
        /// <param name="b"></param>
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXDisableDXGI(int b);

    }
}
