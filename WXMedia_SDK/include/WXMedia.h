/*
WXMedia 总接口
*/
#ifndef _WX_MEDIA_H_
#define _WX_MEDIA_H_

#include "WXMediaDefines.h"      //各种声明和定义

#ifdef __cplusplus
extern "C" {
#endif

	//摄像头数据叠加位置
#define POS_RIGHT_TOP 0
#define POS_RIGHT_BUTTOM 1
#define POS_LEFT_BUTTOM 2
#define POS_LEFT_TOP 3

	// 2022.09.15
	// Add By Tam


	/*
		音频测试功能
		Add By Tam
		2022.05.10
	*/

	//最多支持8路音频播放
	//功能:循环播放一个音频文件，支持MP3 AAC
	// wszFileName: 音频文件名
	//返回值: 成功返回非零指针，是否返回0
	// 可能是文件格式不支持，或者音频设备打开失败
	WXMEDIA_API void* WXAudioFilePlayerStart(WXCTSTR wszFileName);

	//功能:播放一个音频文件，音频文件播放完毕之后自动关闭
	//wszFileName: 音频文件名
	//成功返回1，是否返回0
	WXMEDIA_API int WXAudioFilePlayerStartAutoClose(WXCTSTR wszFileName);

	//功能:关闭音频播放
	//ptr: 操作句柄，WXAudioFilePlayerStart 的返回值
	WXMEDIA_API void WXAudioFilePlayerStop(void* ptr);

	//功能:启动音频监听功能，如果指定AAC文件名，则同时进行音频播放
	//wszDevName: 音频设备的GUID， 或者 "default" / "conf" 等
	// bSystem: 是否扬声器设备
	//返回值: 成功返回非零指针，是否返回0，可能是系统不存在音频设备，或者音频设备打开失败
	WXMEDIA_API void* WXAudioTestStart(WXCTSTR wszDevName, int bSystem);

	//功能:获取当前音频采集音量大小
	//ptr: 操作句柄，WXAudioTestStart返回值
	//返回值:当前采集音量大小， 范围0-32767
	WXMEDIA_API int   WXAudioTestGetVolume(void* ptr);

	//功能:关闭音频设备测试
	//ptr: 操作句柄，WXAudioTestStart返回值
	WXMEDIA_API void  WXAudioTestStop(void* ptr);


	//判断文件是否存在
	WXMEDIA_API int WXFileExist(WXCTSTR wszFile);

	//类似于AirplayPush的操作，用于录制通过WXMedia播放的音频数据，格式为48000 2ch
	WXMEDIA_API void AudioDataPush(int sample_rate, int channel, uint8_t* buf, int buf_size);

	//计算视频图像在某个窗体上以保持比例的方式显示的时候，X轴和Y轴的填充黑边距离
	WXMEDIA_API void WXMeidaUtilsGetDisplayRect(int rect_width, int rect_height, int scr_width, int scr_height, int* dX, int* dY);


	//设置生成Dump文件后执行的外部EXE及参数
	//2019.08.30 by Tam
	WXMEDIA_API void        SetDumpCallBackExe(WXCTSTR strExe, WXCTSTR strParam);

	//招商银行的触摸屏使用DXGI采集会黑屏，要禁用DXGI录屏功能，只能用GDI
	// DXGI 加透明水印效果不好， 非VIP用户禁用DXGI就可以了
	WXMEDIA_API void WXDisableDXGI(int b);




#define LEVEL_GOOD   0  //较差设备，双核或者内存小于6G，比较古老的Win7，画质还有再低一些
#define LEVEL_BETTER 1  //默认设备，一般是4核
#define LEVEL_BEST   2  //本机至少大于4核才能设置为Best

	//界面层通过cpu数量、速度和内存等信息设置机器性能
	//底层根据对应的level调整视频编码参数使得画质更好
	WXMEDIA_API void WXSetMachineLevel(int level);



	WXMEDIA_API WXCTSTR      WXFfmpegGetError(int code);//获取错误代码
	//------------------- windows 设备枚举 ----------------------
	WXMEDIA_API void         WXSetCrashDumpFlag(int b);//设置是否使用 Dump信息
	WXMEDIA_API void         WXSetCrashDumpFile(WXCTSTR strFile);//设置Dump信息文件

	WXMEDIA_API void         WXUtilsInit(WXCTSTR logfile);//WXUtils 初始化

	//-- 2020.08.18
	WXMEDIA_API WXCTSTR      WXGetLocalName();//
	WXMEDIA_API WXCTSTR      WXGetLocalUserName();//获取本机名字
	WXMEDIA_API int          WXNetworkGetIPCount(); //获取本机IP个数
	WXMEDIA_API WXCTSTR      WXNetworkGetIPAddr(int index);//获取本机IP地址


	//获取PID对应的EXE名字
	WXMEDIA_API int          WXGetProcessName(unsigned long processID, wchar_t* strName);
	WXMEDIA_API int          WXBlackListedExe(WXCTSTR exe, int bGame);// EXE 黑名单过滤

	//获取当前系统的版本号
	WXMEDIA_API int          WXGetSystemVersion();



	WXMEDIA_API int          WXSupportDXGI();//设备是否支持 DXGI采集1 为可以，0 不可以

	//无损合并QSV的FLV方法
	WXMEDIA_API int          WXFfmpegMergerFiles(WXCTSTR outFilename, int inCount, WXCTSTR* arrInput);
	WXMEDIA_API int          WXFfmpegMergerGetState();
	WXMEDIA_API int64_t      WXFfmpegMergerGetCurr();
	WXMEDIA_API int64_t      WXFfmpegMergerGetTotal();



	//获取多媒体文件音频抽样值
	WXMEDIA_API int          WXGetAudioVolumeData(WXCTSTR filename, int* pData, int Num, int bFullWave);


	WXMEDIA_API void         WXSetLogFile(WXCTSTR wszFileName); //设置日志文件
	WXMEDIA_API void         WXLogWriteNew(const char* format, ...);//

	//2表示无时间输出
	WXMEDIA_API void		 WXLogWriteNewW2(const wchar_t* format, ...);//已经无用
	WXMEDIA_API void		 WXLogWriteNewW(const wchar_t* format, ...);//用于某些UNICODE字符的日志

	WXMEDIA_API int64_t      WXGetTimeMs();
	WXMEDIA_API void         WXSleepMs(int ms);
	WXMEDIA_API  int         WXStrlen(WXCTSTR str);
	WXMEDIA_API  WXCTSTR     WXStrdup(WXCTSTR str);
	WXMEDIA_API  WXTSTR      WXStrcpy(WXTSTR str1, WXCTSTR str2);
	WXMEDIA_API  int         WXStrcmp(WXCTSTR str1, WXCTSTR str2);

	//TCP-UDP
	//--------------------- TCP 发送 ---------------
	WXMEDIA_API void* WXTcpSenderCreate(WXCTSTR wszIP, int nPort);
	WXMEDIA_API int   WXTcpSenderSend(void* ptr, uint8_t* buf, int buf_size);
	WXMEDIA_API void  WXTcpSenderDestroy(void* ptr);

	//--------------------- UDP发送 --------------------
	WXMEDIA_API void* WXUdpSenderCreate(WXCTSTR wszIP, int nPort);
	WXMEDIA_API int   WXUdpSenderSend(void* ptr, uint8_t* buf, int buf_size);
	WXMEDIA_API void  WXUdpSenderDestroy(void* ptr);

	//----------------------- TCP接收 --------------------
	WXMEDIA_API void* WXTcpRecvCreate(int nPort, void* sink, OnTcpData cb);
	WXMEDIA_API void  WXTcpRecvDestroy(void* ptr);

	//-----------------------  UDP接收 --------------------
	WXMEDIA_API void* WXUdpRecvCreate(int nPort, void* sink, OnData cb);
	WXMEDIA_API void  WXUdpRecvDestroy(void* ptr);


	// MPEG-TS  数据处理
	WXMEDIA_API void* TSDemuxCreate();
	WXMEDIA_API void   TSDemuxDestroy(void* ptr);
	// 返回1 表示可以获得视频数据
	// 返回2 表示可以获得音频数据
#define TS_TYPE_AUDIO 2
#define TS_TYPE_VIDEO 1
	WXMEDIA_API int    TSDemuxWriteData(void* ptr, const uint8_t* buf, int buf_size);
	WXMEDIA_API void   TSDemuxGetExtraData(void* ptr, uint8_t** buf, int* buf_size);
	WXMEDIA_API void   TSDemuxGetVideoData(void* ptr, uint8_t** buf, int* buf_size);
	WXMEDIA_API void   TSDemuxGetAudioData(void* ptr, uint8_t** buf, int* buf_size);

	WXMEDIA_API void* TSMuxerCreate();
	WXMEDIA_API void  TSMuxerDestroy(void* ptr);
	WXMEDIA_API void  TSMuxerHandleVideo(void* ptr, uint8_t* encBuf, uint32_t bufsize, int64_t tic, uint8_t** pOut, int* nOutSize);
	WXMEDIA_API void  TSMuxerHandleAudio(void* ptr, uint8_t* encBuf, uint32_t bufsize, int64_t tic, uint8_t** pOut, int* nOutSize);

	// 用于实时流数据的播放，和WXSoundPlayerWriteData配套使用
	WXMEDIA_API void* WXSoundPlayerCreate(int inSampleRate, int inChannel);
	//回调音频数据
	WXMEDIA_API void* WXSoundPlayerCreateEx(int inSampleRate, int inChannel, void* pSink, OnData cb);
	WXMEDIA_API void  WXSoundPlayerWriteData(void* ptr, uint8_t* buf, int buf_size);
	WXMEDIA_API void  WXSoundPlayerVolume(void* ptr, int volume);//设置音量
	WXMEDIA_API void  WXSoundPlayerDestroy(void* ptr);

	//基于FDK-AAC的编码器
	WXMEDIA_API void* WXAACEncoderCreate(int inSampleRate, int inChannel);

	//返回1 表示有编码输出
	WXMEDIA_API int   WXAACEncoderEncodeFrame(void* ptr, uint8_t* buf, int buf_in, uint8_t** buf_out, int* out_len);
	WXMEDIA_API void  WXAACEncoderDestroy(void* ptr);


	//基于X264 的视频编码
	WXMEDIA_API void* WXX264EncoderCreate(int nWidth, int nHeight, uint8_t** pExtra, int* nExtraSize);
	//返回1 表示编码成功
	WXMEDIA_API int   WXX264EncoderEncodeFrame(void* ptr, struct AVFrame* frame, uint8_t** pOut, int* outlen);
	WXMEDIA_API void  WXX264EncoderDestroy(void* ptr);


	struct AVFrame;

	//判断数据是否纯黑图像
	WXMEDIA_API  int    WXMediaAVFrameIsBlackRGB32(struct AVFrame* frame);

	WXMEDIA_API  void   WXMediaUtilsRGB32Scale(struct AVFrame* dst, struct AVFrame* src, int fixed);

	WXMEDIA_API  struct AVFrame* WXMediaUtilsCopyVideoFrame(struct AVFrame* frame);

	//返回值0 表示 失败
	WXMEDIA_API int     WXMediaUtilsSaveAsPicture(struct AVFrame* frame, WXCTSTR wszName, int iQuatily);
	WXMEDIA_API int		WXMediaUtilsSaveAsPictureSize(struct AVFrame* frame, wchar_t* output, int outwidth, int outheight);

	//Debug验证数据
	WXMEDIA_API  void   WXAddDataToFile(WXCTSTR wszName, const void* data, int data_size);

	WXMEDIA_API  struct AVFrame* WXMediaUtilsFromPicture(WXCTSTR wszName);

	WXMEDIA_API  struct AVFrame* WXMediaUtilsYUVFrameFromPicture(WXCTSTR wszName);
	WXMEDIA_API  struct AVFrame* WXMediaUtilsRGB32FrameFromPicture(WXCTSTR wszName);

	WXMEDIA_API  struct AVFrame* WXMediaUtilsAllocVideoFrame(enum AVPixelFormat pix_fmt, int width, int height);

	WXMEDIA_API  struct AVFrame* WXMediaUtilsAllocAudioFrame(enum AVSampleFormat format, int sample_rate, int channel, int nb_samples);

	WXMEDIA_API  struct SwrContext* WXMediaUtilsAllocSwrCtx(int inSampleRate, int inCh, enum AVSampleFormat inSampleFmt, int outSampleRate, int outCh, enum AVSampleFormat outSampleFmt);

	//ffmpeg 去水印
	WXMEDIA_API  void WXMediaDelogo(uint8_t* dst, int dst_linesize,
		uint8_t* src, int src_linesize,
		int w, int h,
		int logo_x, int logo_y,
		int logo_w, int logo_h);


	WXMEDIA_API  void WXMediaCopyFrame(struct AVFrame* dst, const struct AVFrame* src);

	WXMEDIA_API  void WXMediaFreeFrame(struct AVFrame* dst);

	//从extra数据解析H264 的分辨率
	WXMEDIA_API  void H264GetSize(const uint8_t* pData, int dataSize, int* _width, int* _height, int* _profile);
	//从extra数据解析H265 的分辨率
	WXMEDIA_API  void H265GetSize(const uint8_t* pData, int dataSize, int* _width, int* _height, int* _profile);

	//音频格式转换
	struct WXAudioConvert;
	WXMEDIA_API void  WXAudioConvertDeinit(struct WXAudioConvert* ctx);
	WXMEDIA_API struct WXAudioConvert* WXAudioConvertInit(int inSampleRate, int inCh, enum AVSampleFormat inSampleFmt,
		int outSampleRate, int outCh, enum AVSampleFormat outSampleFmt);
	WXMEDIA_API void  WXAudioConvertFrame(struct WXAudioConvert* ctx, struct AVFrame* dstFrame, const struct AVFrame* srcFrame);
	WXMEDIA_API void  WXAudioConvertData(struct WXAudioConvert* ctx, uint8_t** out, int out_count, const uint8_t** in, int in_count);

	struct SwsContext;
	WXMEDIA_API struct SwsContext* WXMediaGetSwsContext(int width, int height, enum AVPixelFormat srcFormat, enum AVPixelFormat dstFormat);
	WXMEDIA_API void WXMdiaFreeSwsContext(struct SwsContext* c);
	WXMEDIA_API void WXMdiaSwsScale(struct SwsContext* c, const struct  AVFrame* srcFrame, struct AVFrame* dstFrame);

	//显示器操作

//显示器设备属性
	typedef struct _MonitorInfo {
		WXCHAR wszName[MAX_PATH];//名字
		int isPrimary; //主屏幕
		int left;
		int top;
		int width;
		int height;
	}MonitorInfo;

	WXMEDIA_API void WXScreenInit();   //初始化显示器管理
	WXMEDIA_API void WXScreenDeinit(); //
	WXMEDIA_API int  WXScreenGetCount(); //返回显示器个数
	WXMEDIA_API MonitorInfo* WXScreenGetInfo(int index); //返回对应显示器属性
	WXMEDIA_API MonitorInfo* WXScreenGetInfoByName(WXCTSTR wszDevice);//获取指定名字显示器属性
	WXMEDIA_API MonitorInfo* WXScreenGetDefaultInfo();//获取默认显示器属性

	WXMEDIA_API int WXScreenGetRotate(WXCTSTR wszDevice); //返回对应显示器旋转角度0 90 180 270

	typedef struct _CameraDataFormat {
		int width;
		int height;
		int fps;
		int mt;//采集格式
		int index;//在COM格式设置中的序号
		int64_t AvgTimePerFrame;
	}CameraDataFormat;

	typedef struct _CameraInfo {
		WXCHAR m_strName[MAX_PATH];//设备名字
		WXCHAR m_strGuid[MAX_PATH];//端口名字
		int size_fmt;
		CameraDataFormat m_arrFmt[MAX_PATH];//视频格式
	}CameraInfo;

	//摄像头设备
	WXMEDIA_API void WXCameraInit(); //初始化
	WXMEDIA_API void WXCameraDeinit();
	WXMEDIA_API int  WXCameraGetCount();
	WXMEDIA_API CameraInfo* WXCameraGetInfo(int index);

	//打开指定摄像头
	//一般用于预览图像。。。
	//width height iFps 可以从 info 得到
	//因为要支持多摄像头，这个接口不用了
	WXMEDIA_API void* WXCameraGetCurrDevice();//获取当前摄像头

	WXMEDIA_API void* WXCameraOpenWithHwnd(WXCTSTR devGUID, int width, int height, int iFps, HWND hwnd, int Fixed);
	WXMEDIA_API void* WXCameraOpenWithSink(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb);//回调I420
	WXMEDIA_API void* WXCameraOpenWithSink2(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb);//回调RGB32 
	WXMEDIA_API void  WXCameraSetting(void* ptr, HWND hwnd);
	WXMEDIA_API void  WXCameraClose(void* ptr);


	//  WXMedia 1.0.0.517
	//2019.05.14  By tam.xie
	//增加水平翻转输出摄像头采集数据
	//bHFilp = 1 表示水平翻转
	WXMEDIA_API void  WXCameraSetHFilp(void* ptr, int bHFilp);//在打开设置的情况下设置水平翻转
	WXMEDIA_API void* WXCameraOpenWithHwndExt(WXCTSTR devGUID, int width, int height, int iFps, HWND hwnd, int Fixed, int bHFilp);
	WXMEDIA_API void* WXCameraOpenWithSinkExt(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp); //回调I420
	WXMEDIA_API void* WXCameraOpenWithSinkExt2(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp);//回调RGB32

	//** 摄像头多路输出
	//bHFilp = 1 表示水平翻转
	//bRGB32 = 1 表示输出RGB32数据
	WXMEDIA_API void* WXCameraOpenWithSinkNew(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack2 cb, int bHFilp, int bRGB32);//回调RGB32


	//WASAPI 音频操作
	//设备名字
	typedef struct _SoundDeviceInfo {
		WXCHAR m_strName[MAX_PATH];//设备名字
		WXCHAR m_strGuid[MAX_PATH];//端口名字
		int  isDefalut;//默认设备
		int  isDefalutComm;//默认通信设备
	}SoundDeviceInfo;

	//WASAPI 设备属性
	WXMEDIA_API void WXWasapiNotifyInit();

	WXMEDIA_API void WXWasapiInit();
	WXMEDIA_API void WXWasapiDeinit();
	WXMEDIA_API int  WXWasapiGetRenderCount();
	WXMEDIA_API SoundDeviceInfo* WXWasapiGetRenderInfo(int index);
	WXMEDIA_API int  WXWasapiGetCaptureCount();
	WXMEDIA_API SoundDeviceInfo* WXWasapiGetCaptureInfo(int index);

	//监听音量
#define AUDIO_DEVIDE_MIC   0      //0 麦克风 默认设备
#define AUDIO_DEVIDE_SYS   1      //1 扬声器 默认设备
#define AUDIO_DEVIDE_ALL   2      //2 扬声器+麦克风 默认设备

#define AUDIO_DEVIDE_MIC_CONF    3   //3 麦克风 默认设备+默认通信设备
#define AUDIO_DEVIDE_SYS_CONF    4   //4 扬声器 默认设备+默认通信设备
#define AUDIO_DEVIDE_CONF        5   //5 扬声器+麦克风 默认设备+默认通信设备 

//guid 已经无用，可以改成 NULL或者随意字符串
//type 表示监听设备的类型
	WXMEDIA_API  void  AudioDeviceOpen(WXCTSTR guid, int type);

	//关闭音频监听
	//type=1: 关闭扬声器监听
	//type=0: 关闭麦克风监听
	//type=2: 关闭所有音频设备监听
	WXMEDIA_API  void  AudioDeviceClose(int type);

	//获取音频监听值
	//type=1: 获取扬声器监听值(可能包括默认通信设备)
	//type=0: 获取麦克风监听值(可能包括默认通信设备)
	//type=2: 获取(扬声器+麦克风)监听值(可能包括默认通信设备)
	//返回值:音量大小(0-32767)
	WXMEDIA_API  int   AudioDeviceGetVolume(int type);


	//不再使用
	WXMEDIA_API  void    AudioDeviceResetDefault();//重新遍历设备
	WXMEDIA_API  void    AudioDeviceListenComm(int bComm);
	WXMEDIA_API  int     AudioDeviceGetVolume2(int bSystem, int bComm);


	WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultGuid(int bSystem);
	WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultName(int bSystem);
	WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultCommGuid(int bSystem);
	WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultCommName(int bSystem);


	//win7 hook 方式录制游戏
	WXMEDIA_API int64_t WXGameGetDuration();//游戏的超时时间
	WXMEDIA_API int   WXGameHooked();//判断是否Hook成功，1 成功，0 失败
	WXMEDIA_API int   WXGameHookType();

	WXMEDIA_API void  WXGameDrawImage(WXCTSTR str, int x, int y);
	WXMEDIA_API void  WXGameCreate(void* sink, wxCallBack cb);
	WXMEDIA_API void  WXGameCreateByWindow(HWND hwnd, void* sink, wxCallBack cb);
	WXMEDIA_API void  WXGameSetPreview(HWND hwnd); //设置游戏预览
	WXMEDIA_API void  WXGameStart();
	WXMEDIA_API void  WXGameStop();
	WXMEDIA_API int   WXGameGetWidth(); //获取hook之后的图像大小
	WXMEDIA_API int   WXGameGetHeight(); //获取hook之后的图像大小
	WXMEDIA_API void  WXGameDestory();


	//2019.11.19 Tam
	//type=0不显示FPS信息
	//type=1在左上角显示FPS信息
	//type=2在右上角显示FPS信息
	//type=3在左下角显示FPS信息
	//type=4在右下角显示FPS信息
	WXMEDIA_API void  WXGameShowFps(int type);


	//2019.11.19 Tam
	//刷新FPS的模式
	//MODE=0 表示按秒更新
	//MODE=1 表示每一帧更新
	WXMEDIA_API void  WXGameShowFpsMode(int mode);


	//2019.11.20 Tam
	//按比例缩放游戏录制画面
	//默认值100
	//游戏录制前设置有效
	WXMEDIA_API void WXGameSetScale(int scale);
	WXMEDIA_API int WXGameGetScale();

	//是对下面方法的调用，不建议外部直接使用
	WXMEDIA_API void  WXGameDrawString(WXCTSTR str, int x, int y); //坐标原点左上角
	WXMEDIA_API void  WXGameSetDrawColor(int colorText, int colorBk);//设置输出文字字体颜色和背景颜色
	WXMEDIA_API void  WXGameSetDrawFont(WXCTSTR strFont, int fontSize);//设置输出文字字体


	//录屏接口


//音频设备参数
	typedef struct _AudioDeviceParam {
		int has_audio;// = true;  //Version 1.0.0.5
		WXCHAR m_systemName[MAX_PATH];  //扬声器的名字，从SoundInfo 获得
		WXCHAR m_micName[MAX_PATH];   //麦克风的名字
		WXCHAR codec[16];//"aac" "mp3" 等编码格式

		//MIC 设置
		int bAGC;//设置声音增强
		int bNS;//设置降噪
	}AudioDeviceParam;

	//扩展版本录制参数
	typedef struct _TWXAudioConfig {
		int has_audio;// = true;  //Version 1.0.0.5
		WXCHAR m_systemName[MAX_PATH];
		//扬声器的名字，从SoundInfo 获得,  默认可以用 all或者default，不用就用"""
		WXCHAR m_micName[MAX_PATH];
		//麦克风的名字
		WXCHAR codec[16];//"aac" "mp3" 等编码格式

		//MIC 设置
		int bAGC;//设置声音增强
		int bNS;//设置降噪

		//音频录制扩展参数 ---------- 2019.03.20
		int bVAD; //人声检测功能
		int nSystemLevel;//扬声器采集Level，100
		int nMicLevel;//MIC采集Level,100
		int nSampleRate;//采样频率，默认48000
		int nChannel;//声道，默认为2
		int nBitarte;//音频码率
		int nSystemScale;//扬声器变调参数,100不变
		int nMicScale;//MIC变调参数,100不变
	}TWXAudioConfig;

	//视频设备参数
	typedef struct _VideoDeviceParam {
		int      m_bUse;// = 0;

		int      m_iFps;// = 24; //原始

		int      m_bUseHW;//H264 硬编码，包括 QSV  NVENC  VideoToolBox
#ifdef _M_X64
		int      test1;
#endif

		//-------------------------------
		WXCHAR   m_wszCodec[16];// "h264" "mpeg4" 等编码格式

		//-------------------------------
		int      m_iBitrate;// = 1000*1000;//默认视频码率

#ifdef _M_X64
		int      test2;
#endif

		//-------------------------------
		HWND     m_hwndPreview;// = nullptr; 预览窗口

		VideoCallBack onVideoData;// = nullptr  数据回调

		//-------------------------------
		// 显示器使用枚举到的名字， 
		// 摄像头使用枚举到的GUID值，多个相同的摄像头名字可能一样，但是GUID值不一样 
		// Airplay等数据源 当成一种特殊的摄像头，名字使用 "AIRPLAY"
		//当使用屏幕录制时， "FullScreen"表示将所有显示器当成一路视频源录制
		//使用"all"表示把所有的屏幕独立录制成一路视频源，输出多个视频文件(声音数据一样)
		//如果使用 "name1;name2;name3"的以";"分开的方式，就是将指定的屏幕录制成独立的视频文件
		WXCHAR m_wszDevName[MAX_PATH];
		//-------------------------------
		//摄像头参数
		int   m_bCamera;// 0表示屏幕录制，1表示摄像头，2表示窗口录制
		int   m_iCameraWidth;// = 640;
		int   m_iCameraHeight;// = 480;


#ifdef _M_IX86
		//桌面采集参数
		union {
			int   m_bDXGI;// Win10 DXGI桌面采集，目前只支持主屏幕， 鼠标绘制和区域采集的具体功能还没加上去
			HWND  m_hwndCapture;//窗口录制中要录制的窗口的HWND
		};
#else
		int   m_bDXGI;// Win10 DXGI桌面采集，目前只支持主屏幕， 鼠标绘制和区域采集的具体功能还没加上去
		HWND  m_hwndCapture;//窗口录制中要录制的窗口的HWND
#endif

		int   m_bRect;// = 0,使用区域截图

#ifdef _M_X64
		int      test3;
#endif
		RECT  m_rcScreen;// = {0,0,100,100};

		int   m_bFollowMouse;// = 0 使用鼠标位置来截图，以鼠标为中心, 需要先设置区域的RECT值

		int   m_bCaptureBlt;// GDI采集 有些机器需要每次采集桌面都CreateDC

#ifdef _M_X64
		int      GameThreadID;
#endif

		int   m_bForceHDC;// GDI采集 有些机器需要每次采集桌面都CreateDC  //GameProcessID 游戏录制ID

#ifdef _M_X64
		int      GameProcessID;
#endif

		int   m_iForceFps;//是否恒定帧率CFR

		int   m_iAntiAliasing;//抗锯齿处理， H264 将使用 YUV444 编码

#ifdef _M_X64
		int      test4;
#endif
	}VideoDeviceParam;


	//设置水印字体样式
	//	FontStyleRegular = 0,   //常规
	//	FontStyleBold = 1,      //加粗
	//	FontStyleItalic = 2,    //斜体
	//	FontStyleBoldItalic = 3, //加粗斜体
	//	FontStyleUnderline = 4, //下划线 bUnderline
	//	FontStyleStrikeout = 8  // 删除线 bStrikeout
	//文字水印参数
	typedef struct _TextWaterMarkParam {
		int      m_bUsed;// = false;
		int      m_iPosX;// = 0; //文字水印位置X
		int      m_iPosY;// = 0; //文字水印位置Y
		int      m_nStyle;// = 0  字体样式 0-15
		WXCHAR   m_wszText[MAX_PATH];// = _T("录屏王测试工程"); //文字
		uint32_t m_iColor;// = RGB(255, 0, 0); //文字颜色 RGB(r,g,b)
		uint32_t m_BkColor;// = RGB(255, 0, 0); //背景颜色 RGB(r,g,b)// Add by Tam, 2018-04-02
		WXCHAR   m_wszFontName[MAX_PATH];// = _T("宋体"); //或者指定名字和大小
		int      m_iFontSize;// = 100;//字体大小
#ifdef _M_X64
		int test;
#endif
		HFONT    m_hfont;// = NULL,
	}TextWaterMarkParam;

	//图像水印参数
	typedef struct _ImageWaterMarkParam {
		int     m_bUsed;// = false; //是否使用图像水印
		int     m_iPosX;// = 200; //指定图像位置 x
		int     m_iPosY;// = 200;  //指定图像位置 y
		int     m_iDelay;//== 0 延时使用水印,单位为秒
		float     m_fAlpha;//0.0-1.0越小越透明
		WXCHAR m_wszFileName[MAX_PATH];//水印图像文件名
	}ImageWaterMarkParam;

	//鼠标事件参数
	typedef struct _MouseParam {
		int m_iUsed;//为0 时 不录制鼠标

		//鼠标热点
		int m_bMouseHotdot;// = false;
		int m_iHotdotRadius;// = 10;
		uint32_t m_colorMouse;// = RGB(255, 255, 0);
		float     m_fAlphaHotdot;//0.0-1.0越小越透明

		//鼠标点击动画
		int m_bMouseAnimation;// = false;
		int m_iAnimationRadius;// = 20;
		uint32_t m_colorLeft;// = RGB(255, 0, 0);
		uint32_t m_colorRight;// = RGB(0, 0, 255);
		float     m_fAlphaAnimation;//0.0-1.0越小越透明
	}MouseParam;

	typedef struct WXCaptureParam {
		WXCHAR m_wszFileName[MAX_PATH]; //输出文件名

		WXCaptureMode m_mode;//录制模式

		AudioDeviceParam m_audio;//音频采集编码配置
		VideoDeviceParam m_video;//视频采集编码配置
		TextWaterMarkParam m_text;//文字水印配置
		ImageWaterMarkParam m_image;//图像水印配置

		MouseParam m_mouse;//鼠标配置

		void* m_sink;//回调对象，可能是C++对象
		wxCallBack m_cb;//结束回调，函数

	}WXCaptureParam;


	//新的录制结构体，兼容老接口，扩展新参数
	typedef struct TWXCaptureConfig {
		WXCHAR m_wszFileName[MAX_PATH]; //输出文件名
		WXCaptureMode m_mode;//录制模式
		TWXAudioConfig m_audio;//音频录制参数，扩展模式  //2019.03.20 修改
		VideoDeviceParam    m_video;//视频录制参数
		TextWaterMarkParam  m_text;//文字水印录制参数
		ImageWaterMarkParam m_image;//图像水印录制参数
		MouseParam m_mouse;//鼠标配置参数

#ifdef _M_X64
		int test1;
#endif
		void* m_sink;//回调对象，可能是C++对象
		wxCallBack m_cb;//结束回调，函数

		//第二路输出
		int bUseTemp;//是否使用FLV缓存文件，2019.03.20 新增
		WXCHAR wszOtherFileName[MAX_PATH]; //缓存文件名，2019.03.20 新增

		int nVideoScale; //图像缩放参数，默认100，禁用

		//片头片尾设置，2019.03.25 新增
		WXCHAR wszHeadImage[MAX_PATH]; //片头文件， JPEG/PNG/GIF等
		int nHeadDuration;//片头长度

		WXCHAR wszTrailImage[MAX_PATH]; //片尾文件， JPEG/PNG/GIF等
		int nTrailDuration;//片尾长度
	}TWXCaptureConfig;


	//已经不用的函数
	WXMEDIA_API void WXSetSystemSoundType(int i); //设置扬声器采集格式
	WXMEDIA_API void WXSetMicSoundType(int i); //设置MIC采集格式
	WXMEDIA_API void WXMediaSaveParam(int bSave);//弃用了
	WXMEDIA_API void WXMediaSetTemp(int bTs);//录屏时是否先使用TS/FLV作为临时文件，程序异常退出后TS文件还可以正常播放，默认使用
	WXMEDIA_API int  WXMediaGetTemp();//

	//WXCapture  初始化操作
	WXMEDIA_API void WXDeviceInit(WXCTSTR wsz);//初始化，路径应该为全局路径
	WXMEDIA_API void WXDeviceDeinit();//退出
	WXMEDIA_API void WXDeviceInitMirror(WXCTSTR wsz);//退出
	WXMEDIA_API void WXDeviceDeinitMirror();//退出
	WXMEDIA_API void WXGameInit();
	//桌面采集
	//配置默认参数
	//建议弃用
	WXMEDIA_API void  WXCaptureDefaultParam(WXCaptureParam* param);

	//新的录屏结构体
	WXMEDIA_API void  TWXCaptureConfigDefault(TWXCaptureConfig* param);

	//启动录屏录音
	WXMEDIA_API void* WXCaptureStart(WXCaptureParam* param, int* error); //老接口

	WXMEDIA_API void* WXCaptureStartExt(TWXCaptureConfig* param, int* error);//使用新的录制结构体的录制参数

	//全局水印设置
	WXMEDIA_API void    WXCaptureSetWM(WXCTSTR wszName, int nPosX, int nPosY);

	//设置去水印的区域
	//width ， height = 0 表示复位
	//type = 0 表示录制时去水印
	//type = 1 表示编辑时去水印
	WXMEDIA_API void    WXSetDelogo(int type, int nPosX, int nPosY, int nWidth, int nHeight);

	// 2021.04.01
	// Airplay、AMCast 投屏录制
	// DataCaptureStart启动， AirplayPush推送数据， DataCaptureStop 结束
	// DataCaptureStart 返回1 表示启动成功， 返回0 表示启动失败
	// wszName 输出文件路径
	// nWidth 输出宽度 
	// nHeight 输出高度 
	// nFps 视频帧率，默认30 
	// nSoundType 音频类型 0，无音频，1 仅扬声器， 2仅麦克风， 3 扬声器+麦克风 
	// bHw 是否硬编码   1硬编码，0软编码
	// nVideoMode 画质， 0 最差， 1 一般， 2最好
	// bAllowResize AirplayPush 是否允许输入不同分辨率的数据
	// 比如Airplay 中会因为横竖屏的切换导致数据分辨率不一致 

	WXMEDIA_API int     DataCaptureStart(WXCTSTR wszName, int nWidth, int nHeight, int nFps,
		int nSoundType, int bHw, int nVideoMode, int bAllowResize);

	WXMEDIA_API void    DataCaptureStop();

	//使用加密TS做临时文件的MP4 录制
	WXMEDIA_API void* WXCaptureStartExt2(TWXCaptureConfig* param, int* error);//使用新的录制结构体的录制参数


	// WXCaptureStartWithLevel(param,error,100,100)
	// 2019.03.01
	WXMEDIA_API void* WXCaptureStartWithLevel(WXCaptureParam* param, int* error, int systemLevel, int miclevel);

	//当前时间戳
	WXMEDIA_API int64_t WXCaptureGetTime(void* ptr);
	WXMEDIA_API int64_t WXCaptureGetVideoTime(void* ptr);
	WXMEDIA_API int64_t WXCaptureGetAudioTime(void* ptr);

	WXMEDIA_API int64_t WXCaptureGetVideoSize(void* ptr);
	WXMEDIA_API int64_t WXCaptureGetVideoFrame(void* ptr);

	WXMEDIA_API int64_t WXCaptureGetAudioSize(void* ptr);
	WXMEDIA_API int64_t WXCaptureGetAudioFrame(void* ptr);

	WXMEDIA_API int64_t WXCaptureGetFileSize(void* ptr);//文件总长度


	WXMEDIA_API int     WXCaptureGetStopFlag();//查询是否需要结束录制，比如 窗口录制中的 窗口销毁或者窗口大小改变

	//2019.02.23
	//UI把边框绘制好后，将边框RECT保存下来
	WXMEDIA_API void WXCaptureSetRegion(int left, int top, int width, int height);

	//在录屏过程中获取音频设备音量Level

	WXMEDIA_API int WXCaptureGetSystemLevel(void* ptr);
	WXMEDIA_API int WXCaptureGetMicLevel(void* ptr);

	//设置采集音量，老接口
	WXMEDIA_API void WXCaptureSetSystemLevel(void* ptr, int level);
	WXMEDIA_API void WXCaptureSetSystemSilence(void* ptr, int bSilence);

	WXMEDIA_API void WXCaptureSetMicLevel(void* ptr, int level);
	WXMEDIA_API void WXCaptureSetMicSilence(void* ptr, int bSilence);

	//2019.02.25
	//设置采集音量Level
	//system = 1 表示扬声器 0表示MIC
	//nullptr 表示在采集前设置，并且为默认音量
	WXMEDIA_API void WXCaptureSetAudioLevel(void* ptr, int system, int level);
	WXMEDIA_API int  WXCaptureGetAudioLevel(void* ptr, int system);

	WXMEDIA_API int WXCaptureGetAudioScale(void* ptr, int bSystem);
	WXMEDIA_API void WXCaptureSetAudioScale(void* ptr, int bSystem, int nScale);

	//设置麦克风采集 VAD 检测功能
	WXMEDIA_API void WXCaptureSetVAD(int b);

	//和上一帧视频编码的时间间隔
	WXMEDIA_API int64_t WXCaptureGetVideoTimeOut();

	//计算当前时间和开始录制时间的间隔
	WXMEDIA_API int64_t WXCaptureGetDuration();

	WXMEDIA_API void  WXCapturePause(void* ptr);
	WXMEDIA_API void  WXCaptureResume(void* ptr);

	WXMEDIA_API void  WXCaptureGetPicture1(void* ptr, WXCTSTR wsz, int quality);
	WXMEDIA_API void  WXCaptureGetPicture2(void* ptr, WXCTSTR wsz, int quality);

	//为游戏添加摄像头画中画接口
	//b 为1时表示启用，0表示关闭
	//设置在游戏画面中的位置和大小
	//位置从左上角算起
	//大小限制在640*480
	WXMEDIA_API void WXCameraSetDrawCamera(void* ptr, int b, int posx, int posy, int width, int height);

	//修改摄像头属性，亮度之类参数
	WXMEDIA_API void  WXCaptureCameraSetting(void* ptr, HWND hwnd);

	WXMEDIA_API void  WXCaptureChangeRect(void* ptr, int x, int y);//改变区域
	WXMEDIA_API void  WXCaptureChangeRect2(void* ptr, int x, int y, int w, int h);//改变区域

	//关闭录屏录音
	WXMEDIA_API void WXCaptureStop(void* ptr);

	WXMEDIA_API void WXCaptureStopEx(void* ptr, int bVip);

	//获取录屏过程中的音频波形
	//设置截取波形长度
	WXMEDIA_API void WXSetWaveLength(int n);
	//获取波形数据
	WXMEDIA_API int  WXGetWaveData(int* pData);

	//获取监听波形
	//2019.08.30 By Tam
	WXMEDIA_API void  WXSetListenWaveLength(int n);
	WXMEDIA_API int   WXGetSystemData(int* pData);
	WXMEDIA_API int   WXGetMicData(int* pData);

	//摄像头、Airplay填充数据给录屏器,YUV420P数据
	WXMEDIA_API void  WXAirplayPush(struct AVFrame* frame);

	//加旋转
	WXMEDIA_API void  WXAirplayPush2(struct AVFrame* frame, int rotate);

	//游戏录制数据推送
	WXMEDIA_API void  WXRgbaPush(struct AVFrame* frame);

	//来自VLC等播放器回调的 I420数据推送
	WXMEDIA_API void  WXPushI420Data(uint8_t* buf, int width, int height);


	//来自VLC等播放器回调的 RGB32数据推送
	WXMEDIA_API void  WXPushRGB32Data(uint8_t* buf, int width, int height, int pitch);


	//区域录制时的区域回调
	//2018.12.17 Add By Tam.Xie
	typedef void(*WXRegionResetCallBack)(void* ctx);//通知上层清除区域虚线
	typedef void(*WXRegionDrawCallBack)(void* ctx, int left, int top, int width, int height);//通知上层绘制区域虚线
	WXMEDIA_API void WXSetRegionCallBack(void* ctx, WXRegionResetCallBack cb1, WXRegionDrawCallBack cb2);


	// 音视频采集对外接口
	// 2020.05.21
	//音频设备采集
	//创建及配置各种参数
	WXMEDIA_API void* WXAudioCaptureCreate();//创建对象
	WXMEDIA_API void      WXAudioCaptureSetCond(void* ptr, void* cond, int* bFlag);//设置线程等待信号
	WXMEDIA_API void      WXAudioCaptureDestroy(void* ptr);//销毁对象

	WXMEDIA_API void      WXAudioCaptureSetSampleRate(void* ptr, int nSampleRate);//设置采集采样率 默认48000
	WXMEDIA_API void      WXAudioCaptureSetChannel(void* ptr, int nChannel);//设置采集声道，默认2
	WXMEDIA_API void      WXAudioCaptureSetSystemDevice(void* ptr, WXCTSTR strSystem, int nLevel, int nScale);//设置扬声器采集参数
	WXMEDIA_API void      WXAudioCaptureSetMicDevice(void* ptr, WXCTSTR strMic, int nLevel, int nScale, int bAGC, int bNS, int bVAD);//设置麦克采集参数
	WXMEDIA_API void      WXAudioCaptureSetSink(void* ptr, void* sink, OnAudioData cbData);//设置数据回调

	WXMEDIA_API int       WXAudioCaptureInit(void* ptr);//初始化音频设备，成功返回0，成功后才能Start
	WXMEDIA_API void      WXAudioCaptureSetFrameSize(void* ptr, int size);//指定每一帧输出的数据长度


	WXMEDIA_API void      WXAudioCaptureStart(void* ptr);//启动录音
	WXMEDIA_API void      WXAudioCaptureStop(void* ptr);//结束录音
	WXMEDIA_API int       WXAudioCaptureGetAudioLevel(void* ptr, int bSystem);
	WXMEDIA_API void      WXAudioCaptureSetAudioLevel(void* ptr, int bSystem, int level);//在录音过程中修改录制音量

	WXMEDIA_API int       WXAudioCaptureGetAudioScale(void* ptr, int bSystem);
	WXMEDIA_API void      WXAudioCaptureSetAudioScale(void* ptr, int bSystem, int level);//在录音过程中修改变调参数 50-200

	//视频设备采集
	WXMEDIA_API void* WXVideoCaptureCreate();//创建对象
	WXMEDIA_API void      WXVideoCaptureSetCond(void* ptr, void* cond, int* bFlag);//设置信号

	WXMEDIA_API void      WXVideoCaptureDestroy(void* ptr);//销毁对象
	WXMEDIA_API void      WXVideoCaptureSetDataSink(void* ptr, void* pDataSink, OnVideoData cbData);//设置数据回调
	WXMEDIA_API void      WXVideoCaptureSetEventSink(void* ptr, void* pEventSink, wxCallBack cbData);//设置消息回调

	WXMEDIA_API void      WXVideoCaptureSetScreen(void* ptr, WXCTSTR strDisplay, int nFps);//设置屏幕采集
	WXMEDIA_API void      WXVideoCaptureSetWindow(void* ptr, HWND hwnd, int nFps);//设置窗口采集

	WXMEDIA_API void      WXVideoCaptureSetVideoParam(void* ptr, VideoDeviceParam* param);//设置视频参数
	WXMEDIA_API void      WXVideoCaptureSetTextParam(void* ptr, TextWaterMarkParam* param);//设置文字水印参数
	WXMEDIA_API void      WXVideoCaptureSetImageParam(void* ptr, ImageWaterMarkParam* param);//设置图像水印参数
	WXMEDIA_API void      WXVideoCaptureSetMouseParam(void* ptr, MouseParam* param);//设置鼠标信息

	WXMEDIA_API int       WXVideoCaptureUpdate(void* ptr);//动态切换数据源，可以先Stop 在Set各种参数 然后 Update再Start，成功返回0才能Start
	WXMEDIA_API int       WXVideoCaptureGetWidth(void* ptr);//获取宽度
	WXMEDIA_API int       WXVideoCaptureGetHeight(void* ptr);//获取高度

	WXMEDIA_API void      WXVideoCaptureStart(void* ptr);//启动采集线程
	WXMEDIA_API void      WXVideoCaptureStop(void* ptr);//结束采集线程
	WXMEDIA_API void      WXVideoCaptureChangeRect(void* ptr, int x, int y, int w, int h);//区域录制时修改录制区域
	WXMEDIA_API void      WXVideoCaptureGetPicture(void* ptr, int type, WXCTSTR wszName, int quality);//录制时截图
	WXMEDIA_API int64_t   WXVideoCaptureGetVideoTimeOut(void* ptr);//计算超时时间
	WXMEDIA_API void      WXVideoCapturePause(void* ptr, int b);//暂停视频采集


	//视频图像显示



//检测本机是否支持D3DX D3D GDI。。。
//在指定窗口上以指定模式显示某张图片
//wszName 为NULL时使用自定义数据
	WXMEDIA_API void WXVideoRenderCheck(HWND hwnd, int mode, WXCTSTR wszName);

	//视频显示 C Styte Interface
	//2020.01.02
	//当输入图像分辨率和初始化分辨不一致的时候，底层不重新初始化，而是将图像进行缩放以适应初始化的分辨率
	WXMEDIA_API void* WXVideoRenderCreateEx(HWND hWnd, int width, int height);

	WXMEDIA_API void* WXVideoRenderCreate(HWND hWnd, int width, int height);

	//bBroadcast 表示是否直播模式
	WXMEDIA_API void* WXVideoRenderCreate2(HWND hWnd, int width, int height, int bBroadcast);

	WXMEDIA_API void  WXVideoRenderDestroy(void* ptr);

	//默认会按机器性能分别初始化 D3DX D3D GDI
	//如果机器异常，可以通过调用下面的接口来强制调用某个API

	WXMEDIA_API void WXVideoRenderChangeMode(void* ptr, int mode);//切换GDI/D3D/D3DX

	WXMEDIA_API void WXVideoRenderChangeModeForce(void* ptr, int mode);//切换GDI/D3D/D3DX

	//画质优化
	//WXMEDIA_API void WXVideoRenderSetUseLut(void* ptr, int bLut);//启用 禁用
	//WXMEDIA_API void WXVideoRenderSetLutLevel(void* ptr, int nLutLevel);//设置Lut强度
	//WXMEDIA_API void WXVideoRenderSetYScale(void* ptr, int nYScale);//设置亮度变化
	//WXMEDIA_API void WXVideoRenderSetUVScale(void* ptr, int nUVScale);//设置饱和度变化

#define ROTATEFLIP_TYPE_RESET     0  //复位为原图
#define ROTATEFLIP_TYPE_ROTATE90  1  //顺时针旋转90度
#define ROTATEFLIP_TYPE_ROTATE180 2  //顺时针旋转180度
#define ROTATEFLIP_TYPE_ROTATE270 3  //顺时针旋转270度
#define ROTATEFLIP_TYPE_FLIPX     4  //水平翻转
#define ROTATEFLIP_TYPE_FLIPY     5  //垂直翻转
//从当前的RotateFilp状态生成新的 RotateFilp值

//2020.11.04
	WXMEDIA_API int  WXGetRotateFilp(int nRotateFilp, int type);

	WXMEDIA_API void WXVideoRenderDisplay(void* ptr, struct AVFrame* frame, int bFixed, int nRotateFilp);

	//Add 2022.4.23
	//关联到播放ID
	WXMEDIA_API void WXVideoRenderSetID(void* ptr, void* playID);
	//确保绘制出来的是播放ID的图像
	WXMEDIA_API void WXVideoRenderDisplayEx(void* ptr, struct AVFrame* frame, int bFixed, int nRotateFilp, void* playID);

	//2021.04.26
	//当width,height 有数值的时候使用直播模式
	//当width,height 为零的时候取消直播模式
	WXMEDIA_API void WXVideoRenderSetBroadcastSize(void* ptr, int width, int height);

	//Add 2020.07.14
	WXMEDIA_API void WXVideoRenderDisplayI420(void* ptr, uint8_t* pY, int PitchY, uint8_t* pU, int PitchU, uint8_t* pV, int PitchV, int bFixed, int nRotate);

	WXMEDIA_API void WXVideoRenderDisplayI420Ex(void* ptr, uint8_t* pY, int PitchY, uint8_t* pU, int PitchU, uint8_t* pV, int PitchV, int bFixed, int nRotate, void* playID);

	//画质调节
	/*
	范围-50 -- 50
	*/
#define TYPE_LUT    0  //对比度
#define TYPE_GRAY   1  //亮度
#define TYPE_CHROME 2  //饱和度
	//Add 2021.05.07  Tam
	WXMEDIA_API void WXVideoRenderSetLut(int type, int value);



	//视频编码器

//创建解码器
	WXMEDIA_API void* WXVideoDecoderCreate(int bH264, int bHw, uint8_t* extradata, int extrasize);
	//销毁解码器
	WXMEDIA_API void  WXVideoDecoderDestroy(void* ptr);

	//建议用GetFrame 的AVFrame 的 分辨率
	//获得宽度
	WXMEDIA_API int   WXVideoDecoderGetWidth(void* ptr);
	//获得高度
	WXMEDIA_API int   WXVideoDecoderGetHeight(void* ptr);
	//解码成功返回1，失败返回0
	WXMEDIA_API int   WXVideoDecoderSendPacket(void* ptr, uint8_t* buf, int size, int64_t pts);

#define TYPE_CURR_FRAME  0 //解码帧
#define TYPE_LAST_FRAME  1 //最后一帧
#define TYPE_BLACK_FRAME 2 //填充黑屏

	//获取图像帧
	WXMEDIA_API struct AVFrame* WXVideoDecoderGetFrame(void* ptr, int type);

	//获取图像帧， 指定旋转校对
	WXMEDIA_API struct AVFrame* WXVideoDecoderGetFrame2(void* ptr, int type, int rotate);

	//直播模式，以指定分辨率获取解码图像！
	//如1920x1080
	WXMEDIA_API struct AVFrame* WXVideoDecoderFrameEx(void* ptr, int type, int rotate, int dstWdith, int dstHeight);

	//H264硬编码能力检测
#define WXSupportH264Codec WXSupportHarewareCodec
	WXMEDIA_API int          WXSupportHarewareCodec();

	//H265硬编码能力检测
	WXMEDIA_API int          WXSupportH265Codec();


	//视频编码
	WXMEDIA_API void* WXVideoEncoderCreate(int bH264, int width, int height, int fps, int bitrate);

	//视频编码
	WXMEDIA_API int   WXVideoEncoderSendFrame(void* ptr, struct AVFrame* frame);

	//获得编码数据
	WXMEDIA_API int   WXVideoEncoderGetPacket(void* ptr, uint8_t** pOut, int* nOutSize);

	//获得编码数据
	WXMEDIA_API int  WXVideoEncoderEncodeFrame(void* ptr, struct AVFrame* avframe, uint8_t** pOut, int* nOutSize);

	//销毁编码器
	WXMEDIA_API void  WXVideoEncoderDestroy(void* ptr);

	//视频转换和视频播放接口




//创建DXFilter
//绑定到WPF ImageSource 上， 同时做为 PlayID 的 SetSurfcaeCb的回调
	WXMEDIA_API void* WXDXFilterCreate(HWND hwnd);


	//销毁DXFilter对象
	WXMEDIA_API void  WXDXFilterDestroy(void* obj);

	//从DXFilter获取Surface及其大小
	//obj:DXFilter对象
	//width:返回Surface的宽度
	//heigth:返回surface的高度
	WXMEDIA_API void* WXDXFilterGetSurface(void* obj, int* width, int* height);


	//WXDXFilterGetSurface 后调用Surface绘制之后应该执行WXDXFilterReleaseSurface
	//连续两次GetSurface，第二次返回nullptr
	//避免切换文件时产生的画面冻结问题
	//弃用了
	WXMEDIA_API void WXDXFilterReleaseSurface(void* obj);

	//DXFilter设置亮度、饱和度、对比度
	//obj:obj对象
	//nBrightness:亮度，范围0-200，默认值100
	//nSaturation:饱和度，范围0-200，默认值100
	//nContrast:对比度，范围0-200，默认值100
	WXMEDIA_API void WXDXFilterSetLut(void* obj, int nBrightness, int nSaturation, int nContrast);

	//在WXDXFilterGetSurface后在初始化的HWND上渲染出来
	WXMEDIA_API void WXDXFilterRender(void* obj);

	//渲染AVFrame 到 DXFilter
	struct AVFrame;
	WXMEDIA_API void WXDXFilterSetID(void* obj, uint64_t uid);
	WXMEDIA_API int WXDXFilterDraw(void* obj, uint64_t uid, struct AVFrame* frame);//



//功能: 声道声音播放时切换声道模式
//参数:
//nMode: 参照宏SOUND_MODE_*
// 2023.02.22
//add by Tam
// WXSetGlobalString(L"SoundMode", SOUND_MODE_NONE); //原声输出
// WXSetGlobalString(L"SoundMode", SOUND_MODE_MAX); //取双声道最大值
// WXSetGlobalString(L"SoundMode", SOUND_MODE_AVG); //双声道平均
// WXSetGlobalString(L"SoundMode", SOUND_MODE_LEFT); //只输出左路
// WXSetGlobalString(L"SoundMode", SOUND_MODE_RIGHT); //只输出右路

//或者
// WXSetGlobalValue(L"SoundMode", L"None"); //原声输出
// WXSetGlobalValue(L"SoundMode", L"Max"); //取双声道最大值
// WXSetGlobalValue(L"SoundMode", L"Avg"); //双声道平均
// WXSetGlobalValue(L"SoundMode", L"Left"); //只输出左路
// WXSetGlobalValue(L"SoundMode", L"Right"); //只输出右路


// 程序初始化调用
// WXSetGlobalString(L"MediaPlayer", L"Lav"); // 强制LAV 播放器
// WXSetGlobalString(L"MediaPlayer", L"Ffplay"); //强制LAV 播放器
 //或者
// WXSetGlobalValue(L"MediaPlayer", MEDIAPLAYER_LAV); // 强制LAV 播放器
// WXSetGlobalValue(L"MediaPlayer", MEDIAPLAYER_FFPLAY); // 强制Ffplay 播放器



//基于LavFilters 的播放器
//2020.08.06
//创建播放器对象

//功能：创建播放器对象
//参数：
//wszType: 无意义，一般用NULL
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//返回值: 播放器对象，返回NULL表示失败
	WXMEDIA_API void* WXPlayerCreate(WXCTSTR wszType, WXCTSTR wszInput, int nSpeed, int64_t nSeek);

	//创建播放器对象
	//功能：创建播放器对象，并且异步进行文件打开操作，通过回调来判断该文件是否可以进行其它操作
	//参数：
	//wszType: 无意义，一般用nullptr
	//wszInput: 输入文件名
	//speed: 播放速率，默认100，范围50-200
	//seek: 开始播放位置，默认为0
	//cbEvent: 异步回调消息
	//返回值: 播放器对象，返回nullptr表示失败
	WXMEDIA_API void* WXPlayerCreateEx(WXCTSTR wszType,
		WXCTSTR wszInput, int speed, int64_t seek, WXFfmpegOnEvent cbEvent);


	//创建播放器对象,异步执行OpenFile操作，
	// 其它线程定时通过WXPlayerGetState()查询该对象状态，
	// 等到它返回 FFPLAY_STATE_INIT_OK 就可以执行 Start Run等操作进行播放
	//参数：
	//wszType: 无意义，一般用nullptr
	//wszInput: 输入文件名
	//speed: 播放速率，默认100，范围50-200
	//seek: 开始播放位置，默认为0
	//返回值: 播放器对象，返回nullptr表示失败
	WXMEDIA_API void* WXPlayerCreateAsync(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek);


	//功能:给当前播放对象增加附加音轨和附加音轨延迟时间
	//参数:
	//ptr:播放器对象
	//wszAudio:音频文件名，当wszAudio为nullptr时表示调节当前附加音轨的同步时间
	//delay:音频播放相对于视频播放的延迟
	//大于0表示附加音轨相对主轨道延后播放，小于0表示播放，默认0表示同步播放
	//返回值:添加成功返回1，失败返回0
	WXMEDIA_API int     WXPlayerAttachAudio(void* ptr, WXCTSTR strAudio, int64_t nDelay);

	//功能:设置循环播放
	//参数:
	//ptr:播放器对象
	//bLoop: 1表示循环播放，0表示不循环
	//返回值:无
	WXMEDIA_API void    WXPlayerLoop(void* ptr, int bLoop);

	//功能:获取当前播放速率
	//参数:
	//ptr:播放器对象
	//返回值: 当前播放速率，范围 0.5-2.0
	WXMEDIA_API double  WXPlayerGetSpeed(void* ptr);

	//功能:异步销毁播放器对象
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXPlayerDestroy(void* ptr);


	//功能:设置播放器视频显示窗口(使用底层渲染)
	//参数:
	//ptr:播放器对象
	//hwnd:显示窗口句柄
	//返回值:无
	WXMEDIA_API void     WXPlayerSetView(void* ptr, HWND hwnd);

	//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXPlayerSetVideoCB(void* ptr, WXFfmpegOnVideoData cb);


	//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXPlayerSetVideoTimeCB(void* ptr, WXFfmpegOnVideoData2 cb);


	//回调Surface给WPF
	//返回值1表示底层支持YUV处理
	//返回值0表示底层不支持YUV处理
	//参数:
	//ptr:播放器对象
	//filter: DXFilter对象，通过WXDXFilter创建
	//cb:回调函数
	//返回值:无
	WXMEDIA_API int  WXPlayerSetVideoSurfaceCB(void* ptr, void* filter, WXFfmpegOnVideoData3 cb);

	//功能:设置音量
	//参数:
	//ptr:播放器对象
	//volum:设置的音量值，范围0-100
	//返回值:无
	WXMEDIA_API void     WXPlayerSetVolume(void* p, int nVolume);

	//功能:主轨道静音
	//参数:
	//ptr:播放器对象
	//bMuted: 1 表示静音， 0 表示取消静音
	//返回值:无
	WXMEDIA_API void     WXPlayerMute(void* ptr, int bMuted);


	//功能:设置播放器数据回调函数，返回AVFrame数据类型
	//参数:
	//ptr:播放器对象
	//ctx:对调对象
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXPlayerSetAVFrameCB(void* ptr, void* pCtx, OnVideoData cb);

	//功能:设置播放器数据回调函数，返回AVFrame数据类型
	//参数:
	//ptr:播放器对象
	//ctx:对调对象
	// nID: 播放器ID
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXPlayerSetAVFrameCB2(void* ptr, void* pCtx, int nID, onAVFrame cb);


	//功能:创建播放器对象，回调AVFrame数据类型
	WXMEDIA_API void* WXPlayerCreate2(WXCTSTR wszType, WXCTSTR wszInput, void* pCtx, OnVideoData cb);

	//功能:设置播放器消息回调对象
	//参数:
	//ptr:播放器对象
	//owner:消息回调对象
	//返回值:无
	WXMEDIA_API void     WXPlayerSetEventOwner(void* ptr, void* pOwner);

	//功能:设置播放器消息回调函数
	//参数:
	//ptr:播放器对象
	//cb:消息回调函数
	//返回值:无
	WXMEDIA_API void     WXPlayerSetEventCb(void* ptr, WXFfmpegOnEvent cb);

	//功能:设置播放器大小回调函数，可以返回当前视频的分辨率
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:
	WXMEDIA_API void     WXPlayerSetSizeCb(void* ptr, ffplayOnSize cb);

	//功能:
	//参数:
	//ptr:播放器对象
	//返回值:
	WXMEDIA_API void     WXPlayerSetEventID(void* ptr, WXCTSTR strID);

	//功能:
	//参数:
	//返回值:
	WXMEDIA_API WXCTSTR  WXPlayerGetEventID(void* p);

	//用来区分本地文件和 URL 的Start行为
	//功能:开始播放
	//参数:
	//ptr:播放器对象
	//返回值:成功返回1，失败返回0
	WXMEDIA_API int      WXPlayerStart(void* ptr);


	//功能:开始播放
	//参数:
	//ptr:播放器对象
	//返回值:成功返回1，失败返回0
	WXMEDIA_API int      WXPlayerStartEx(void* ptr);


	//功能:结束播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXPlayerStop(void* ptr);


	//功能:获取附加音轨时的实际播放延迟
	//参数:
	//ptr:播放器对象
	//返回值:附加音轨时的实际播放延迟
	WXMEDIA_API int64_t  WXPlayerGetDelay(void* ptr);

	//功能:播放时截图
	//参数:
	//ptr:播放器对象
	//wszName:截图文件路径
	//quality:截图编码系数
	//返回值:无
	WXMEDIA_API void     WXPlayerShotPicture(void* ptr, WXCTSTR  strName, int nQuality);


	//功能:判断播放器是否正在运行
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API int  WXPlayerRunning(void* ptr);

	//功能:暂停播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXPlayerPause(void* ptr);

	//功能:恢复播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXPlayerResume(void* ptr);


	//功能:获取当前播放时间，单位毫秒
	//参数:
	//ptr:播放器对象
	//返回值:获取当前播放时间，单位毫秒
	WXMEDIA_API int64_t  WXPlayerGetCurrTime(void* ptr);

	//功能:获取文件播放总时长，单位毫秒
	//参数:
	//ptr:播放器对象
	//返回值:文件播放总时长，单位毫秒
	WXMEDIA_API int64_t  WXPlayerGetTotalTime(void* ptr);

	//获取当前播放音量，默认100，范围0-100
	//功能:
	//参数:
	//ptr:播放器对象
	//返回值:当前播放音量，默认100，范围0-100
	WXMEDIA_API int      WXPlayerGetVolume(void* ptr);

	//功能:设置播放跳转
	//参数:
	//ptr:播放器对象
	//pts: 跳转值，单位毫秒
	//返回值:
	WXMEDIA_API void     WXPlayerSeek(void* ptr, int64_t pts);

	//功能:设置播放器速率
	//参数:
	//ptr:播放器对象
	//nSpeed:播放器速率，范围50-200(对应返回速率的05.-2.0)
	//返回值:
	WXMEDIA_API void     WXPlayerSpeed(void* ptr, int nSpeed);

	//功能:获取播放器状态
	//参数:
	//ptr:播放器对象
	//返回值:播放器状态
	WXMEDIA_API int      WXPlayerGetState(void* ptr);


	//功能:重置播放器
	//参数:
	//ptr:播放器对象
	//返回值:
	WXMEDIA_API void     WXPlayerSetReset(void* ptr);


	//功能:刷新播放器,
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXPlayerRefresh(void* p);


	//功能:设置播放器字幕
	//参数:
	//ptr:播放器对象
	//strName:字幕文件名，支持srt、ass等
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitle(void* ptr, WXCTSTR  strName);


	//功能:设置播放器字幕延迟时间，单位为毫秒
	//参数:
	//ptr:播放器对象
	//szName:字幕文件名，支持srt、ass等
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitleDelay(void* ptr, int64_t nDelay);



	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitleFontName(void* ptr, WXCTSTR  strFontName);

	//功能:设置播放器字幕字体大小，默认14
	//参数:
	//ptr:播放器对象
	//nFontSize:字体大小，默认14
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitleFontSize(void* ptr, int nFontSize);

	//功能:设置播放器字幕字体颜色
	//参数:
	//ptr:播放器对象
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitleFontColor(void* ptr, int dwFontColor);

	//功能:设置播放器字幕字体颜色
	//参数:
	//ptr:播放器对象
	//bCutSide: 是否在底层切黑边
	//返回值:无
	WXMEDIA_API void     WXPlayerSetUsingCutSide(void* ptr, int bCutSide);


	//功能:设置播放器字幕隐藏
	//参数:
	//ptr:播放器对象
	//bHide: 隐藏字幕
	//返回值:无
	WXMEDIA_API void     WXPlayerSetSubtitleHide(void* ptr, int bHide);



	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//FontSize:字体大小，默认14
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//返回值:无
	WXMEDIA_API void   WXPlayerSetSubtitleFont(void* ptr, WXCTSTR  strFontName, int nFontSize, int dwFontColor);


	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//FontSize:字体大小，默认14
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//bBold: 是否粗体
	//bItalic: 是否斜体
	//bUnderLine: 是否有下划线
	//bStrikeOut: 是否有删除线
	//返回值:无
	WXMEDIA_API void   WXPlayerSetSubtitleFontEx(void* ptr,
		WXCTSTR  strFontName, int nFontSize, int dwFontColor,
		int bBold, int bItalic, int bUnderLine, int bStrikeOut);


	//功能:设置播放器字幕显示高度
	//参数:
	//ptr:播放器对象
	//postion: 显示位置。范围0-100, 0 为底部，100为顶部
	//返回值:无
	WXMEDIA_API void   WXPlayerSetSubtitlePostion(void* ptr, int nPos);


	//功能:播放器裁剪视频，一般是 MP4 GIF 
	//参数:
	//ptr:播放器对象
	//strName: 显示位置
	//tsStart: 开始时间，单位毫秒
	//tsStop:  结束时间，单位毫秒
	//nWidth,nHeight: 输出文件的分辨率，两个为0时表示原分辨率输出
	//返回值:无
	WXMEDIA_API void   WXPlayerCutVideo(void* ptr, WXCTSTR strName, int64_t tsStart, int64_t tsStop, int nWidth, int nHeight);

	//--------------------------- ffplay ---------------------
	//功能：创建播放器对象
	//参数：
	//wszType: 无意义，一般用NULL
	//wszInput: 输入文件名
	//speed: 播放速率，默认100，范围50-200
	//seek: 开始播放位置，默认为0
	//返回值: 播放器对象，返回NULL表示失败
	WXMEDIA_API void* WXFfplayCreate(WXCTSTR strType, WXCTSTR strInput, int nSpeed, int64_t nSeek);


	//创建播放器对象
	//功能：创建播放器对象，并且异步进行文件打开操作，通过回调来判断该文件是否可以进行其它操作
	//参数：
	//wszType: 无意义，一般用nullptr
	//wszInput: 输入文件名
	//speed: 播放速率，默认100，范围50-200
	//seek: 开始播放位置，默认为0
	//cbEvent: 异步回调消息
	//返回值: 播放器对象，返回nullptr表示失败
	WXMEDIA_API void* WXFfplayCreateEx(WXCTSTR wszType,
		WXCTSTR wszInput, int speed, int64_t seek, WXFfmpegOnEvent cbEvent);


	//创建播放器对象,异步执行OpenFile操作，
	// 其它线程定时通过WXPlayerGetState()查询该对象状态，
	// 等到它返回 FFPLAY_STATE_INIT_OK 就可以执行 Start Run等操作进行播放
	//功能：创建播放器对象，并且异步进行文件打开操作
	//参数：
	//wszType: 无意义，一般用nullptr
	//wszInput: 输入文件名
	//speed: 播放速率，默认100，范围50-200
	//seek: 开始播放位置，默认为0
	//返回值: 播放器对象，返回nullptr表示失败
	WXMEDIA_API void* FfplayCreateAsync(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek);

	//功能:给当前播放对象增加附加音轨和附加音轨延迟时间
	//参数:
	//ptr:播放器对象
	//wszAudio:音频文件名，当wszAudio为nullptr时表示调节当前附加音轨的同步时间
	//delay:音频播放相对于视频播放的延迟
	//大于0表示附加音轨相对主轨道延后播放，小于0表示播放，默认0表示同步播放
	//返回值:添加成功返回1，失败返回0
	WXMEDIA_API int     WXFfplayAttachAudio(void* ptr, WXCTSTR wszAudio, int64_t nDelay);

	//功能:设置循环播放
	//参数:
	//ptr:播放器对象
	//bLoop: 1表示循环播放，0表示不循环
	//返回值:无
	WXMEDIA_API void    WXFfplayLoop(void* ptr, int bLoop);

	//功能:获取当前播放速率
	//参数:
	//ptr:播放器对象
	//返回值: 当前播放速率，范围 0.5-2.0
	WXMEDIA_API double  WXFfplayGetSpeed(void* ptr);

	//功能:销毁播放器对象
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXFfplayDestroy(void* ptr);


	//功能:设置播放器视频显示窗口(使用底层渲染)
	//参数:
	//ptr:播放器对象
	//hwnd:显示窗口句柄
	//返回值:无
	WXMEDIA_API void     WXFfplaySetView(void* ptr, HWND hwnd);

	//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXFfplaySetVideoCB(void* ptr, WXFfmpegOnVideoData cb);


	//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:无
	WXMEDIA_API void     WXFfplaySetVideoTimeCB(void* ptr, WXFfmpegOnVideoData2 cb);


	//功能:设置音量
	//参数:
	//ptr:播放器对象
	//volum:设置的音量值，范围0-100
	//返回值:
	WXMEDIA_API void     WXFfplaySetVolume(void* p, int volume);

	//功能:主轨道静音
	//参数:
	//ptr:播放器对象
	//bMuted: 1 表示静音， 0 表示取消静音
	//返回值:无
	WXMEDIA_API void     WXFfplayMute(void* ptr, int bMuted);

	//功能:设置播放器数据回调函数，返回AVFrame数据类型
	//参数:
	//ptr:播放器对象
	//ctx:对调对象
	//cb:回调函数
	//返回值:
	WXMEDIA_API void     WXFfplaySetAVFrameCB(void* ptr, void* pCtx, OnVideoData cb);


	//功能:设置播放器数据回调函数，返回AVFrame数据类型
	//参数:
	//ptr:播放器对象
	//ctx:对调对象
	//nID: 播放器ID, 用来区分多个播放器
	//cb:回调函数
	//返回值:
	WXMEDIA_API void     WXFfplaySetAVFrameCB2(void* ptr, void* pCtx, int nID, onAVFrame cb);



	//功能:设置播放器消息回调对象
	//参数:
	//ptr:播放器对象
	//owner:消息回调对象
	//返回值:
	WXMEDIA_API void     WXFfplaySetEventOwner(void* ptr, void* pOwner);

	//功能:设置播放器消息回调函数
	//参数:
	//ptr:播放器对象
	//cb:消息回调函数
	//返回值:
	WXMEDIA_API void     WXFfplaySetEventCb(void* ptr, WXFfmpegOnEvent cb);

	//功能:设置播放器大小回调函数，可以返回当前视频的分辨率
	//参数:
	//ptr:播放器对象
	//cb:回调函数
	//返回值:
	WXMEDIA_API void     WXFfplaySetSizeCb(void* ptr, ffplayOnSize cb);

	//功能:
	//参数:
	//ptr:播放器对象
	//返回值:
	WXMEDIA_API void     WXFfplaySetEventID(void* ptr, WXCTSTR strID);

	//功能:
	//参数:
	//返回值:
	WXMEDIA_API WXCTSTR  WXFfplayGetEventID(void* ptr);

	//功能:开始播放
	//参数:
	//ptr:播放器对象
	//返回值:成功返回1，失败返回0
	WXMEDIA_API int      WXFfplayStart(void* ptr);

	//用来区分本地文件和 URL 的Start行为
	//功能:开始播放
	//参数:
	//ptr:播放器对象
	//返回值:成功返回1，失败返回0
	WXMEDIA_API int      WXFfplayStartEx(void* ptr);

	//功能:结束播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXFfplayStop(void* ptr);

	//功能:获取附加音轨时的实际播放延迟
	//参数:
	//ptr:播放器对象
	//返回值:附加音轨时的实际播放延迟
	WXMEDIA_API int64_t  WXFfplayGetDelay(void* ptr);

	//功能:播放时截图
	//参数:
	//ptr:播放器对象
	//wszName:截图文件路径
	//quality:截图编码系数
	//返回值:无
	WXMEDIA_API void     WXFfplayShotPicture(void* ptr, WXCTSTR  strName, int nQuality);


	//功能:判断播放器是否正在运行
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API int  WXFfplayRunning(void* ptr);

	//功能:暂停播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXFfplayPause(void* ptr);

	//功能:恢复播放
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXFfplayResume(void* ptr);


	//功能:获取当前播放时间，单位毫秒
	//参数:
	//ptr:播放器对象
	//返回值:获取当前播放时间，单位毫秒
	WXMEDIA_API int64_t  WXFfplayGetCurrTime(void* ptr);

	//功能:获取文件播放总时长，单位毫秒
	//参数:
	//ptr:播放器对象
	//返回值:文件播放总时长，单位毫秒
	WXMEDIA_API int64_t  WXFfplayGetTotalTime(void* ptr);

	//获取当前播放音量，默认100，范围0-100
	//功能:
	//参数:
	//ptr:播放器对象
	//返回值:当前播放音量，默认100，范围0-100
	WXMEDIA_API int      WXFfplayGetVolume(void* ptr);

	//功能:设置播放跳转
	//参数:
	//ptr:播放器对象
	//pts: 跳转值，单位毫秒
	//返回值:
	WXMEDIA_API void     WXFfplaySeek(void* ptr, int64_t pts);

	//功能:设置播放器速率
	//参数:
	//ptr:播放器对象
	// speed:播放器速率，范围50-200(对应返回速率的05.-2.0)
	//返回值:
	WXMEDIA_API void     WXFfplaySpeed(void* ptr, int nSpeed);

	//功能:获取播放器状态
	//参数:
	//ptr:播放器对象
	//返回值:播放器状态
	WXMEDIA_API int      WXFfplayGetState(void* ptr);


	//功能:重置播放器
	//参数:
	//ptr:播放器对象
	//返回值:
	WXMEDIA_API void     WXFfplaySetReset(void* ptr);

	//功能:刷新播放器
	//参数:
	//ptr:播放器对象
	//返回值:无
	WXMEDIA_API void     WXFfplayRefresh(void* p);


	//功能:设置播放器字幕
	//参数:
	//ptr:播放器对象
	//szName:字幕文件名，支持srt、ass等
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitle(void* ptr, WXCTSTR  strName);

	//功能:设置播放器字幕延迟时间，单位为毫秒
	//参数:
	//ptr:播放器对象
	//szName:字幕文件名，支持srt、ass等
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitleDelay(void* ptr, int64_t nDelay);


	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitleFontName(void* ptr, WXCTSTR  strFontName);

	//功能:设置播放器字幕字体大小，默认14
	//参数:
	//ptr:播放器对象
	//FontSize:字体大小，默认14
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitleFontSize(void* ptr, int nFontSize);

	//功能:设置播放器字幕字体颜色
	//参数:
	//ptr:播放器对象
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitleFontColor(void* ptr, int dwFontColor);

	//功能:设置播放器字幕隐藏
	//参数:
	//ptr:播放器对象
	//bHide: 隐藏字幕
	//返回值:无
	WXMEDIA_API void     WXFfplaySetSubtitleHide(void* ptr, int bHide);


	//功能:设置播放器字幕字体颜色
	//参数:
	//ptr:播放器对象
	//bCutSide: 是否在底层切黑边
	//返回值:无
	WXMEDIA_API void     WXFfplaySetUsingCutSide(void* ptr, int bCutSide);

	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//FontSize:字体大小，默认14
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//返回值:无
	WXMEDIA_API void   WXFfplaySetSubtitleFont(void* ptr, WXCTSTR  strFontName, int nFontSize, int dwFontColor);


	//功能:设置播放器字幕字体
	//参数:
	//ptr:播放器对象
	//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
	//FontSize:字体大小，默认14
	//dwFontColor: RGB值， 如 0xFF00FF 等
	//bBold: 是否粗体
	//bItalic: 是否斜体
	//bUnderLine: 是否有下划线
	//bStrikeOut: 是否有删除线
	//返回值:无
	WXMEDIA_API void   WXFfplaySetSubtitleFontEx(void* ptr,
		WXCTSTR  strFontName, int nFontSize, int dwFontColor,
		int bBold, int bItalic, int bUnderLine, int bStrikeOut);


	//功能:设置播放器字幕显示高度
	//参数:
	//ptr:播放器对象
	//nPos: 显示位置。范围0-100, 0 为底部，100为顶部
	//返回值:无
	WXMEDIA_API void   WXFfplaySetSubtitlePostion(void* ptr, int nPos);



	//功能:播放器裁剪视频
	//参数:
	//ptr:播放器对象
	//strName: 显示位置
	//tsStart: 开始时间，单位毫秒
	//tsStop:  结束时间，单位毫秒
	//nWidth, nHeight: 输出文件的分辨率，两个为0时表示原分辨率输出
	//返回值:无
	WXMEDIA_API void   WXFfplayCutVideo(void* ptr, WXCTSTR strName, int64_t tsStart, int64_t tsStop, int nWidth, int nHeight);


	//回调Surface给WPF
	//返回值1表示底层支持YUV处理
	//返回值0表示底层不支持YUV处理
	//参数:
	//ptr:播放器对象
	//filter: DXFilter对象，通过WXDXFilter创建
	//cb:回调函数
	//返回值:无
	WXMEDIA_API int  WXFfplaySetVideoSurfaceCB(void* ptr, void* filter, WXFfmpegOnVideoData3 cb);

	//-------------- 网络流录制相关 -----------------------
	WXMEDIA_API void     WXSetStreamRecord(int b);
	WXMEDIA_API int      WXGetStreamRecord();
	WXMEDIA_API int      WXGetNetStreamWidth();
	WXMEDIA_API int      WXGetNetStreamHeight();


	//视频转换接口,建议直接调用ffmpeg.exe

//ffmpeg.c 的封装操作
	WXMEDIA_API void* FfmpegExeCreate();
	WXMEDIA_API int   FfmpegExeProcess(void* exe, int argv, const wchar_t** argc);
	WXMEDIA_API void  FfmpegExeDestroy(void* exe);
	//当前处理时间
	WXMEDIA_API int64_t FfmpegExeGetCurrTime(void* exe);
	//要处理的文件的总时长
	WXMEDIA_API int64_t FfmpegExeGetTotalTime(void* exe);
	WXMEDIA_API int FfmpegExeGetState(void* exe);


	//  视频转换 API
	WXMEDIA_API void* WXFfmpegParamCreate(void);
	WXMEDIA_API void     WXFfmpegParamDestroy(void* p);
	WXMEDIA_API void     WXFfmpegParamSetEventOwner(void* p, void* ownerEvent);
	WXMEDIA_API void     WXFfmpegParamSetEventCb(void* p, WXFfmpegOnEvent cbEvent);
	WXMEDIA_API void     WXFfmpegParamSetEventID(void* p, WXCTSTR szEvent);

	// 水印图片设置
	// 不可以重复Add相同的文件名
	//最后的4个参数是基于左上角的坐标系
	WXMEDIA_API void     WXFfmpegParamAddWMImage(void* p, WXCTSTR szImage, int x, int y, int w, int h);

	//设置旋转角度
	WXMEDIA_API void     WXFfmpegParamSetRotate(void* p, int rotate);

	//设置垂直旋转
	WXMEDIA_API void     WXFfmpegParamSetVFlip(void* p, int b);

	//设置水平旋转
	WXMEDIA_API void     WXFfmpegParamSetHFlip(void* p, int b);

	//设置裁剪区域
	WXMEDIA_API void     WXFfmpegParamSetCrop(void* p, int x, int y, int w, int h);

	//输入参数50-200, 文件转码后的播放速度 0.5-2.0
	WXMEDIA_API void     WXFfmpegParamSetSpeed(void* p, int speed);

	//亮度(-100,100) 默认0,对比度 (-100,100)  默认 50,饱和度(0,300) 默认100
	WXMEDIA_API void     WXFfmpegParamSetPictureQuality(void* p, int brightness, int contrast, int saturation);

	//音量 0-1000  默认256
	WXMEDIA_API void     WXFfmpegParamSetVolume(void* p, int volume);

	//用于依次输入合并文件
	WXMEDIA_API void     WXFfmpegParamAddInput(void* p, WXCTSTR szInput);

	//字幕设置
	WXMEDIA_API void     WXFfmpegParamSetSubtitle(void* p, WXCTSTR sz);
	WXMEDIA_API void     WXFfmpegParamSetSubtitleFont(void* p, WXCTSTR sz, int FontSize, int FontColor);
	WXMEDIA_API void     WXFfmpegParamSetSubtitleAlpha(void* p, int alpha);
	WXMEDIA_API void     WXFfmpegParamSetSubtitlePostion(void* p, int postion);
	WXMEDIA_API void     WXFfmpegParamSetSubtitleAlignment(void* p, int alignment);// 0 bottom, 1, center, 2 top

	//视频转换参数
	//设置转换时间，单位ms
	WXMEDIA_API void     WXFfmpegParamSetConvertTime(void* p, int64_t ptsStart, int64_t ptsDuration);


	//设置转换过程中编码前的回调函数
	WXMEDIA_API void     WXFfmpegParamSetVideoCB(void* p, onFfmpegVideoData cb);

	//设置转换后的视频格式 yuv420p 。。
	WXMEDIA_API void     WXFfmpegParamSetVideoFmtStr(void* p, WXCTSTR sz);

	//设置视频编码器
	WXMEDIA_API void     WXFfmpegParamSetVideoCodecStr(void* p, WXCTSTR sz);

	//设置底层H264编码器编码模式 0 Faset 1 Normal 2 Best
	WXMEDIA_API void     WXFfmpegParamSetVideoMode(void* p, int mode);

	//设置转换后的帧率
	WXMEDIA_API void     WXFfmpegParamSetVideoFps(void* p, double fps);

	//设置转换后的视频分辨率
	WXMEDIA_API void     WXFfmpegParamSetVideoSize(void* p, int width, int height);

	//设置转换后的显示比例
	WXMEDIA_API void WXFfmpegParamSetVideoDar(void* p, int dar_width, int dar_height);

	//设置视频码率
	WXMEDIA_API void     WXFfmpegParamSetVideoBitrate(void* p, int bitrate);

	//设置音频编码器
	WXMEDIA_API void     WXFfmpegParamSetAudioCodecStr(void* p, WXCTSTR sz);

	//设置音频码率
	WXMEDIA_API void     WXFfmpegParamSetAudioBitrate(void* p, int bitrate);

	//设置音频采样频率
	WXMEDIA_API void     WXFfmpegParamSetAudioSampleRate(void* p, int sample_rate);

	//设置音频声道数
	WXMEDIA_API void     WXFfmpegParamSetAudioChannel(void* p, int channel);

	// ptsStart 开始时间 ms
	// ptsDuration 长度 ms ， 两者为0表示全部处理
	// Fast， 1 表示不重新编码(更换格式之类，建议先录制ts、flv(H264+AAC)之类流式文件避免录制失败难以恢复，再转码为MP4), 0 表示重新编码
	WXMEDIA_API int  WXFfmpegCutFile(void* p, WXCTSTR wszInput, WXCTSTR wszOutput,
		int64_t ptsStart, int64_t ptsDuration, int Fast, int async);

	//播放器截取视频
	WXMEDIA_API int  WXFfmpegCutFile2(void* p, WXCTSTR wszInput, WXCTSTR wszOutput,
		int64_t ptsStart, int64_t ptsDuration, int width, int height);

	WXMEDIA_API int      WXFfmpegConvertVideo(void* p, WXCTSTR wszInput, WXCTSTR wszOutput, int async);

	//用于TS转MP4之类的快速格式转换
	WXMEDIA_API int      WXFfmpegConvertVideoFast(WXCTSTR wszInput, WXCTSTR wszOutput);

	WXMEDIA_API int      WXFfmpegConvertAudio(void* p, WXCTSTR wszInput, WXCTSTR wszOutput, int async);

	//多个同样编码格式的文件文件按输入依次合并为一个文件，多次AddInput
	WXMEDIA_API int      WXFfmpegMergerFile(void* p, WXCTSTR wszOutput, WXCTSTR wszTemp, int fast, int async);

	//音频和视频文件合并为一个文件
	WXMEDIA_API int      WXFfmpegMixerAV(void* p, WXCTSTR wszVideo, WXCTSTR wszAudio, WXCTSTR wszMixer, int async);

	//替换视频文件中的音频数据
	WXMEDIA_API int      WXFfmpegReplaceAudio(void* p, WXCTSTR wszVideo, WXCTSTR wszAudio, WXCTSTR wszOutput, int copy, int async);

	//视频文件截图
	WXMEDIA_API int      WXFfmpegShotPicture(void* p, WXCTSTR wszInput, int64_t ts, WXCTSTR wszOutput);

	//视频转换状态，异步操作时使用它来得到转换的结果
	WXMEDIA_API int64_t  WXFfmpegGetCurrTime(void* p);//获取当前已处理的总时长，单位ms
	WXMEDIA_API int64_t  WXFfmpegGetTotalTime(void* p);//获取需要处理的总时长，单位ms
	WXMEDIA_API int      WXFfmpegGetState(void* p);//获取状态
	WXMEDIA_API void     WXFfmpegInterrupt(void* p);//中断操作


	//视频多媒体信息接口,可以考虑使用MediaInfo.dll

	//获取多媒体文件信息
#define WXMEDIAO_INFO_TYPE_AUDIO  0
#define WXMEDIAO_INFO_TYPE_VIDEO  1
#define WXMEDIAO_INFO_TYPE_ATTACH 2

//Fast,不返回真实的帧率值，码率值等
	WXMEDIA_API void* WXMediaInfoCreateFast(WXCTSTR szFileName, int* error);
	WXMEDIA_API void* WXMediaInfoCreate(WXCTSTR szFileName, int* error);
	WXMEDIA_API void     WXMediaInfoDestroy(void* p);

	WXMEDIA_API int  WXMediaInfoHasAudio(void* p); //是否有音频,有为1，没有为0
	WXMEDIA_API int  WXMediaInfoHasVideo(void* p); //是否有视频,有为1，没有为0
	WXMEDIA_API int  WXMediaInfoHasAttach(void* p); //是否有数据,有为1，没有为0

	WXMEDIA_API int64_t  WXMediaInfoGetFileSize(void* p); //文件大小
	WXMEDIA_API int64_t  WXMediaInfoGetFileDuration(void* p);//时间长度，单位ms
	WXMEDIA_API WXCTSTR  WXMediaInfoGetFormat(void* p);//文件格式

	//获取视频文件的缩略图											 
	WXMEDIA_API int   WXMediaInfoGetPicture(void* p, WXCTSTR wszFileName);
	WXMEDIA_API int   WXMediaInfoGetCusPicture(void* p, WXCTSTR wszFileName, int height);

	//attach MJPG , 获得MP3 专辑封面的 JPG内存数据
	//PNG 或者 JPG 数据格式
	WXMEDIA_API int      WXMediaInfoGetAttachSize(void* p);
	WXMEDIA_API uint8_t* WXMediaInfoGetAttachData(void* p);
	WXMEDIA_API int      WXMediaInfoGetAudioPitcutre(void* p, WXCTSTR wszName);


	//获取视频参数
	WXMEDIA_API WXCTSTR  WXMediaInfoGetVideoCodecName(void* p);//视频编码器
	WXMEDIA_API double   WXMediaInfoGetVideoAvgFps(void* p);//视频平均帧率
	WXMEDIA_API int64_t  WXMediaInfoGetVideoBitrate(void* p);//视频码率
	WXMEDIA_API int      WXMediaInfoGetVideoWidth(void* p);//视频宽度 
	WXMEDIA_API int      WXMediaInfoGetVideoHeight(void* p);//视频高度

	//获取视频显示比例 DAR
	WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioWidth(void* p);
	WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioHeight(void* p);

	//获取音频参数
	WXMEDIA_API WXCTSTR  WXMediaInfoGetAudioCodecName(void* p); //编码器名字
	WXMEDIA_API int      WXMediaInfoGetAudioBitrate(void* p);//音频码率
	WXMEDIA_API int      WXMediaInfoGetAudioSampleRate(void* p);//音频采样率
	WXMEDIA_API int      WXMediaInfoGetAudioChannels(void* p);//音频声道数

	//-----------------  老的 WXMediaInfo方法 --------------------------
	//获取多媒体文件信息
	WXMEDIA_API void* WXMediaInfoCreate(WXCTSTR szFileName, int* error);
	WXMEDIA_API void     WXMediaInfoDestroy(void* p);

	//WXMEDIA_API int      WXMediaInfoGetPicture(void *p, WXCTSTR szFileName, int height);
	WXMEDIA_API int      WXMediaInfoGetAudioChannelNumber(void* p);//Type=0
	WXMEDIA_API int      WXMediaInfoGetVideoChannelNumber(void* p);//Type=1
	WXMEDIA_API int      WXMediaInfoGetAttachChannelNumber(void* p);//Type=2 //add

	WXMEDIA_API int64_t  WXMediaInfoGetFileSize(void* p);
	WXMEDIA_API int64_t  WXMediaInfoGetFileDuration(void* p);
	WXMEDIA_API WXCTSTR  WXMediaInfoGetFormat(void* p);
	WXMEDIA_API int      WXMediaInfoGetChannelNumber(void* p);
	WXMEDIA_API int64_t  WXMediaInfoGetVideoBitrate(void* p);
	WXMEDIA_API double   WXMediaInfoGetVideoAvgFps(void* p);//获取帧率
	WXMEDIA_API int      WXMediaInfoGetVideoRotate(void* p);//获取旋转角度
	WXMEDIA_API int64_t  WXMediaInfoGetVideoCompress(void* p);//视频压缩比

	//获取视频分辨率，相当于PAR
	WXMEDIA_API int      WXMediaInfoGetVideoWidth(void* p);
	WXMEDIA_API int      WXMediaInfoGetVideoHeight(void* p);

	//获取视频显示比例 DAR
	WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioWidth(void* p);
	WXMEDIA_API int      WXMediaInfoGetVideoDisplayRatioHeight(void* p);

	//获取Channel属性
	WXMEDIA_API int      WXMediaInfoChannelGetType(void* p, int type);//0 Audio 1 Video 2 Attach //change
	WXMEDIA_API WXCTSTR  WXMediaInfoChannelCodec(void* p, int type);
	WXMEDIA_API int      WXMediaInfoChannelAudioBitrate(void* p, int type);
	WXMEDIA_API int      WXMediaInfoChannelAudioSampleRate(void* p, int type);
	WXMEDIA_API int      WXMediaInfoChannelAudioChannels(void* p, int type);

	//attach MJPG , 获得MP3 专辑封面的 JPG内存数据
	//PNG 或者 JPG 数据格式
	WXMEDIA_API int      WXMediaInfoChannelGetAttachSize(void* p, int type); //add
	WXMEDIA_API uint8_t* WXMediaInfoChannelGetAttachData(void* p, int type); //add
	WXMEDIA_API int      WXMediaInfoGetAudioPitcutre(void* p, WXCTSTR strName);



	//录屏后的视频编辑,基本不直接使用了

//2019.02.25
//视频压缩转换，需要设置输出帧率
	WXMEDIA_API void* WXMediaConvertCreate(WXCTSTR wszFileName); //初始化解析输入文件
	WXMEDIA_API int64_t WXMediaConvertGetTargetSize(void* p, int mode, int fps);//获取指定模式+指定帧率的 预计视频转换输出文件的大小，单位字节
	WXMEDIA_API int WXMediaConvertProcess(void* p, WXCTSTR wszOutFileName, int mode, int fps);//转换到输出文件，类似于ffmpeg
	WXMEDIA_API int WXMediaConvertGetProcess(void* p); //获取转换进度，和上面一个函数运行在不同的线程
	WXMEDIA_API void WXMediaConvertDestroy(void* p);//退出
	WXMEDIA_API void WXMediaConvertProcessBreak(void* p);//中断转换过程 2019.02.26
	WXMEDIA_API int64_t WXMediaConvertGetTargetSize2(void* p, int oldMode, int fps, int oldBitrate, int newMode);
	WXMEDIA_API int WXMediaConvertProcess2(void* p, WXCTSTR wszOutFileName, int oldMode, int fps, int oldBitrate, int newMode);


	//设置视频压缩限制时间，0表示无限制
	//Add 2020.12.07
	WXMEDIA_API void WXMediaConvertSetTime(void* p, int second);

	//添加片头、片尾、水印等视频编辑操作
	//为裁剪后的文件添加片头片尾
	WXMEDIA_API int WXMediaFileEdit(WXCTSTR wszInput,  //输入文件
		WXCTSTR wszOutput,  //输出文件
		int64_t ptsStart,  //裁剪开始时间，单位ms
		int64_t ptsStop,   //裁剪结束时间，单位ms
		WXCTSTR wszHead, int64_t ptsHead, //片头图片及长度， 单位ms
		WXCTSTR wszTrail, int64_t ptsTaril,//片头图片及长度， 单位ms
		WXCTSTR wszWaterMark); //水印图片

	WXMEDIA_API int WXMediaFileEdit2(WXCTSTR wszInput,  //输入文件
		WXCTSTR wszOutput,  //输出文件
		int64_t ptsStart,  //裁剪开始时间，单位ms
		int64_t ptsStop   //裁剪结束时间，单位ms
	);


	//去除黑屏的 MP4 转换
	WXMEDIA_API int WXMediaFileEdit3(WXCTSTR wszInput, WXCTSTR wszOutput);


	//单纯添加片头片尾，如果已经保存了参数文件，原来的视频数据不需要重新解码编码，这样如果原始文件比较大的话处理速度会比较快
	//如果不存在，将强制转换为 fps=0 bitrate=25的数据
	WXMEDIA_API int WXMediaAddFile(WXCTSTR wszInput,  //输入文件
		WXCTSTR wszOutput,
		WXCTSTR wszHead, int64_t ptsHead, //片头图片及长度， 单位ms
		WXCTSTR wszTrail, int64_t ptsTaril//片头图片及长度， 单位ms
	);

	//已经实现
	WXMEDIA_API int WXMediaCutFile(WXCTSTR wszInput,  //输入文件
		int fps, int bitrate, //设置的帧率和码率
		WXCTSTR wszOutput,  //输出文件
		int64_t ptsStart,  //裁剪开始时间，单位ms
		int64_t ptsStop   //裁剪结束时间，单位ms
	);
	WXMEDIA_API int  WXMediaFileEditGetRate();//返回WXMediaFileEdit执行百分比　０- 100
	WXMEDIA_API void WXMediaFileEditBreak();//中断运行

	WXMEDIA_API void WXMediaFileEditSetSpeed(int nSpeed);//设置转换后的速度， 默认值100， 大于100为加快，小于100变慢


	//快速转换ts/flv/xws/xwm/mp4 到 mp4/xws
	WXMEDIA_API int WXConvertFast(WXCTSTR wszInput, WXCTSTR wszOutput);

	//快速转换ts/flv/xws/xwm/mp4 到 mp4/xws
	// nTime为最大输出时间,0为不限制
	WXMEDIA_API int WXConvertFast2(WXCTSTR wszInput, WXCTSTR wszOutput, int nTime);


	//增加音视频延迟处理
	//wszInput : 输入文件
	//wszOutput : 输出文件
	//audio_delay :  音频延迟参数，单位毫秒，表示相比原文件，音频延迟播放的时间
	WXMEDIA_API int WXConvertDelay(WXCTSTR wszInput, WXCTSTR wszOutput, int audio_delay);
	//-----------------------------------------------------------
	//视频文件合并

	//参数列表初始化
	WXMEDIA_API void WXMediaMergerFileReset();

	//依次
	//添加视频文件
	WXMEDIA_API void WXMediaMergerFileAdd(WXCTSTR wszName);

	//转换
	//wszOutput 输出文件
	//bVip VIP模式，非VIP只能合并文件开始的30%
	//nVideoMode 编码质量 0最差，1一般， 2最好
	//nDstWidth nDstHeight 指定输出分辨率，0x0 表示使用第一个文件的分辨率(如果第一个文件不存在视频则直接返回)
	//nFps 指定输出文件帧率，0表示默认值24
	WXMEDIA_API int WXMediaMergerFileProcess(WXCTSTR wszOutput, int bVip, int nVideoMode, int nDstWidth, int nDstHeight, int nFps);



	//HUD操作，实验
	WXMEDIA_API void* WXHudCreate();
	WXMEDIA_API void WXHudDraw(void* hud, uint8_t* buf);//绘制RGBABuffer
	WXMEDIA_API void WXHudReset(void* ptr);
	WXMEDIA_API void WXHudDestroy(void* hud);
	WXMEDIA_API void WXHudDrawRect(void* ptr, int left, int top, int width, int height);//绘制矩形


	//H264屏幕推流，远古项目，可以忽略
	//H264 屏幕推流
	WXMEDIA_API void* H264ProcessCreate(int index/* = -1*/,
		int width/* = 1280*/,
		int height/* = 720*/,
		int fps/* = 15*/,
		int bitrate/* = 2000 * 1000*/,
		void* sink/* = nullptr*/,
		OnH264VideoData cb/* = NULL*/);
	WXMEDIA_API void H264ProcessDestroy(void* ptr);
	WXMEDIA_API void H264ProcessSwitch(void* ptr, int index);

	//全局参数设置和配置

	// 获取EXE所在的路径
	//WXGetGlobalString(L"ExePath")

	//设置DUMP文件路径
	//WXSetGlobalString(L"DumpFile", strFile);

	//设置DUMP回调程序名
	//WXSetGlobalString(L"DumpCallBackExe", strExeFile);
	//设置调用DUMP回调程序的参数
	//WXSetGlobalString(L"DumpCallBackParam", strExeParam);

	//

	// L"MachLevel" 根据机器性能设置录屏编码，提高性能， 默认为1，范围0-2,2最优但是开销最大， 设置-1 表示自动处理
	// L"AEC"  是否使用Webrtc 回声消除功能 
	// L"HighQualityGif" 录屏为GIF文件时，是否使用高质量模式(默认gif速度较快，但是有明显栅格化问题，高质量编码1080速度3-4fps)

	//L"Memory" 获取本机内存，单位GB
	//L"Cpu" 获取本机Cpu 数量
	//L"CpuSpeed" 获取cpu速率

	//L"DumpUse" 崩溃时是否输出dump文件
	//L"DumpCallBack" 崩溃输出Dump文件时是否调用相关回调程序

	//L"DisbaleDXGI" 是否禁用DXGI采集，某些硬件设备硬件采集有问题

	//摄像头叠加位置，0右上角，1右下角，2左下角，3，左上角
	//WXSetGlobalValue(L"Capture_CameraType", type);

	//摄像头显示窗口宽度，0 表示禁用
	//WXSetGlobalValue(L"Capture_CameraWindowWidth", nWndWidth);

#ifdef __cplusplus
}
#endif


#endif
