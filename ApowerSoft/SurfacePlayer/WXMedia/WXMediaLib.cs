using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Text;
using System.Threading.Tasks;

namespace SurfacePlayer.WXMedia {
    public static class WXMediaLib {
        public enum AVSpeed {
            /// <summary>
            /// 0.5 speed
            /// </summary>
            Slow = 50,
            /// <summary>
            /// 1.0 speed
            /// </summary>
            Normal = 100,
            /// <summary>
            /// 1.5 speed
            /// </summary>
            Speed = 150,
            /// <summary>
            /// 2.0 speed
            /// </summary>
            Fast = 200
        }
        public enum FFPlayState {
            /// <summary>
            /// 不可用， 刚创建的对象
            /// </summary>
            Unavailable = 0,
            /// <summary>
            /// 已经创建或者进行了Stop操作，等待Start或者Destroy
            /// </summary>
            Waiting = 1,
            /// <summary>
            /// 正在播放状态
            /// </summary>
            Playing = 2,
            /// <summary>
            /// 已经开始播放，但是处于暂停状态
            /// </summary>
            Pause = 3,
            /// <summary>
            /// 数据播放完毕
            /// </summary>
            PlayingEnd = 4

        }
        internal enum FFMPEGError {
            OK = 0,
            NOFILE = 1,
            EMPTYFILE = 2,
            INIT = 3,
            READFILE = 4,
            PARSE = 5,
            BREADK = 6,
            NO_MEIDADATA = 7,
            PROCESS = 8,
            NO_OUTPUT_FILE = 9,
            TRANSCODE = 10,
            DECODE_ERROR_STAT = 11,
            ASSERT_AVOPTIONS = 12,
            ABORT_CODEC_EXPERIMENTAL = 13,
            DO_AUDIO_OUT = 14,
            DO_SUBTITLE_OUT = 15,
            DO_VIDEO_OUT = 16,
            DO_VIDEO_STAT = 17,
            READ_FILTERS = 18,
            FLUSH_ENCODERS = 19,
            ON_FILTERS = 20,
            ON_OPTS = 21,
            LIBAVUTIL = 22,
            EXIT_ON_ERROR = 23,

            //ffplay 启动
            OK_START = 30,
            //ffplay 结束
            OK_STOP = 31,
            //ffplay 获取图片成功 
            OK_GET_PICTURE = 32,
            //ffplay 读取文件完毕
            OK_GET_EOF = 33,
            //ffplay 视频数据播放完毕
            OK_VIDEO_STOP = 34,
            //ffplay 音频数据播放完毕
            OK_AUDIO_STOP = 35,
            //ffplay播放完毕
            FFPLAY_ERROR_OK_FINISH = 36
        };

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static void WXDeviceInit(string szLog);//初始化

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXDeviceDeinit();//退出


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static int WXMediaInfoGetPicture(IntPtr ptr, string wszFileName);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static IntPtr WXMediaInfoCreate(string wszFileName, ref int error);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXMediaInfoDestroy(IntPtr ptr);
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXMediaInfoGetVideoWidth(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXMediaInfoGetVideoHeight(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXMediaInfoGetFileDuration(IntPtr ptr);
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXFfplayGetCurrTime(IntPtr ptr);

        //ctx 回调对象
        //width heigth data linesize pts 都是对应AVFrame成员
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void WXFfmpegOnVideoDataTime(IntPtr ctx, int width, int height, IntPtr data, IntPtr linesize, Int64 pts);

        //通知底层有新图像
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void onSurface(IntPtr dxfilter, int width, int height);
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXFfplaySetVideoSurfaceCB(IntPtr ptr, IntPtr hwnd, onSurface cbSurface);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern void WXSetGlobalValue([In()][MarshalAs(UnmanagedType.LPWStr)] string strType, int nValue);
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXFfplayCreate([In()][MarshalAs(UnmanagedType.LPWStr)] string strType, 
            [In()][MarshalAs(UnmanagedType.LPWStr)] string strInput, AVSpeed nSpeed, Int64 nSeek);

        //视频操作回调函数
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public delegate void WXFfmpegOnEvent(IntPtr ctx, [In()][MarshalAs(UnmanagedType.LPWStr)] string strID,
            UInt32 iEvent, [In()][MarshalAs(UnmanagedType.LPWStr)] string strMsg);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public static extern IntPtr WXFfplayCreateEx([In()][MarshalAs(UnmanagedType.LPWStr)] string strType,
    [In()][MarshalAs(UnmanagedType.LPWStr)] string strInput, AVSpeed nSpeed, Int64 nSeek, WXFfmpegOnEvent cbOpenFile);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplaySetVolume(IntPtr ptr, int volume);
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static IntPtr WXFfplaySetVideoTimeCB(IntPtr ptr, IntPtr NotUsePtr, WXFfmpegOnVideoDataTime cb);


        //创建DXFilter对象
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static IntPtr WXDXFilterCreate(IntPtr hwnd);


        //销毁DXFilter对象
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXDXFilterDestroy(IntPtr dxfilter);

        //获取底层处理后的surface
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXDXFilterGetSurface(IntPtr dxfilter, IntPtr refWidth, IntPtr refHeight);

        //获取底层处理后的surface
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXDXFilterGetSurfaceEx(IntPtr dxfilter);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static int WXDXFilterDrawYUV(IntPtr dxfilter, int width, int height, IntPtr pData, IntPtr linesize);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static int WXDXFilterDraw(IntPtr dxfilter, IntPtr avframe);

        //设置亮度、饱和度、对比度
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static IntPtr WXDXFilterSetLut(IntPtr dxfilter, int nBrightness, int nSaturation, int nContrast);


        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplaySeek(IntPtr ptr, long pts);
      
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static bool WXFfplayStart(IntPtr ptr);//启动
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplayStop(IntPtr ptr);//结束
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplayPause(IntPtr ptr);//暂停
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplayResume(IntPtr ptr);//恢复
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfplayDestroy(IntPtr ptr);
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXFfplayGetState(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXFfplayGetTotalTime(IntPtr ptr);


        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        public delegate void OnFfmpegVideoConvertDataHandler(IntPtr ptrCtx, int width, int height, IntPtr pData, IntPtr lineSize, long pts);
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]

        public extern static void WXFfmpegParamSetVideoCB(IntPtr ptr, OnFfmpegVideoConvertDataHandler cb);
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static IntPtr WXFfmpegParamCreate();
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]

        public extern static void WXFfmpegParamSetVideoCodecStr(IntPtr ptr, string codec);
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static void WXFfmpegParamSetVideoFmtStr(IntPtr ptr, string codec);
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFfmpegParamSetVideoSize(IntPtr ptr, int width, int height);
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
        public extern static int WXFfmpegConvertVideo(IntPtr ptr, string strInput, string strOutput, int async);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXFfmpegGetTotalTime(IntPtr ptr);
       
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static int WXFfmpegGetState(IntPtr ptr);
        
        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static long WXFfmpegGetCurrTime(IntPtr ptr);

        //创建YUV420P AVFrame并分配内存
        //WXMEDIA_API void* WXFrameCreate(int width, int height);
        //WXMEDIA_API uint8_t** WXFrameGetData(void* ptr);
        //WXMEDIA_API int* WXFrameGetLinesize(void* ptr);
        //WXMEDIA_API void WXFrameDestroy(void* ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXFrameCreate(int width, int height);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXFrameGetData(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static IntPtr WXFrameGetLinesize(IntPtr ptr);

        [DllImport("WXMedia.dll", CallingConvention = CallingConvention.Cdecl)]
        public extern static void WXFrameDestroy(IntPtr ptr);
    }
    public static class WXDelogosLib {
        [DllImport("LibDelogo.dll", CallingConvention = CallingConvention.Cdecl)]
        internal static extern int WXDelogos(IntPtr dst, IntPtr dst_linesize, 
            IntPtr src, IntPtr src_linesize, int w, int h, 
            int pix_fmt, int[,] logo_rectangles, 
            int nb_rects, int band, int show);

        [DllImport("LibDelogo.dll", CallingConvention = CallingConvention.Cdecl)]
        internal static extern int WXDelogosScaleYUV(IntPtr src, int in_width, int in_height,
            IntPtr dst, int out_width, int out_height, 
            int[,] logo_rectangles, int nb_rects);
    }

    public static class WXDXFilter
    {
        [DllImport("WXDXFilter.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int InitFilter(IntPtr hwnd);

        [DllImport("WXDXFilter.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int UploadTexture(IntPtr bufy, IntPtr bufu, IntPtr bufv, IntPtr name,
            int pitch, int pitchuv, int width, int height, float posx, float posy);
       
        
        [DllImport("WXDXFilter.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int RenderTexture(int width, int height);

        [DllImport("WXDXFilter.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern IntPtr GetFrontSurface();

        [DllImport("WXDXFilter.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int RenderFrameToBitmap(IntPtr Frame, int framewidth, int frameheight,
        IntPtr bitmap, int bitmapwidth, int bitmapheight, int pixelformat);
    }

}
