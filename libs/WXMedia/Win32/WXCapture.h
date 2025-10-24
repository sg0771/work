/*
PC 录屏录音
*/

#ifndef _WX_CAPTURE_H_
#define _WX_CAPTURE_H_

#include <WXMediaCpp.h>
#include "FfmpegMuxer.h"

#define CAPTURE_TOTAL_TIME  0
#define CAPTURE_VIDEO_TIME  1
#define CAPTURE_VIDEO_FRAME 2
#define CAPTURE_VIDEO_SIZE  3
#define CAPTURE_AUDIO_TIME  4
#define CAPTURE_AUDIO_FRAME 5
#define CAPTURE_AUDIO_SIZE  6
#define CAPTURE_TOTAL_SIZE  7

#define MAX_CAPTURE  9

class  WXCapture{
	int m_bStartFlag = 0;
	WXCond m_condStart;//唤醒所有线程

	TWXCaptureConfig m_config;
public:
	WXString m_strFileName = L"";//设置的输出文件名

	void* m_pEventSink = nullptr;//回调对象，可能是C++对象
	wxCallBack m_cbEvent = nullptr;//结束回调，函数

	//--------------- 多设备录制 ---------------
	int m_nVideoCount = 0;//视频设备数量
	void* m_pWXVideo[MAX_CAPTURE] = { nullptr };//视频线程

	int m_nMuxerCount = 0;//输出数量
	FfmpegMuxer* m_pMuxer[MAX_CAPTURE] = { nullptr };

	void*  m_pWXAudio = nullptr;//音频采集对象

	int   CheckVideoParam(VideoDeviceParam& _videoParm, WXCTSTR wszExt);
	int   AddVideoEndpoint(WXCaptureMode _mode,
		VideoDeviceParam* _videoParam,
		TextWaterMarkParam* textParam,
		ImageWaterMarkParam* _imageParam,
		MouseParam* _mouseParam,
		FfmpegMuxer* pMuxer);

	int    CheckAudioParam(TWXAudioConfig& _audioParam, WXCTSTR wszExt);
	int    AddAudioEndpoint(const TWXAudioConfig& _audioParam, 
		FfmpegMuxer* pMuxer);
public:
	int     GetAudioLevel(int bSystem);
	void    SetAudioLevel(int bSystem,int level);
	int     GetAudioScale(int bSystem);
	void    SetAudioScale(int bSystem, int nScale);

	void    GetPicture(int type, WXCTSTR wszName, int quality);// 0为 加水印前，1 为加水印后
public: //控制API
    int     Create(TWXCaptureConfig *param); //成功返回0， 失败返回错误码
	void    Start();
	void    Stop();
	void    Pause();
	void    Resume();
	void    ChangeRect(int x, int y, int w = 0, int h = 0);//截屏过程中改变区域
	
	int64_t GetVideoTimeOut();
	int64_t GetMuxerInfo(int type);
};

#endif

