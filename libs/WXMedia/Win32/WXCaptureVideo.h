/*
录屏时的视频处理部分
*/
#ifndef _WXCaptureVideo_H_
#define _WXCaptureVideo_H_

#include "WXCapture.h"
#include "VideoSource.h"
#include "FfmpegMuxer.h"



//视频处理部分
class  WXCaptureVideo: public WXThread{
public:
	int64_t  m_ptsLast = 0;

	FfmpegMuxer *m_pMuxer = nullptr;
	WXCapture *m_pCapture = nullptr;

	int m_bGameCapture = FALSE;//游戏录制！！

	VideoSource *m_pVideoSource = nullptr;

	int      m_iWidth  = 0;
	int      m_iHeight = 0;

	int64_t  m_ptsLastVideo = 0;//时间戳（包含暂停时间中的）
	int64_t  m_ptsDelay = 0;//时间戳
	BOOL     m_bStart = FALSE;

	int64_t m_ptsStart = 0;

	//截图操作
	WXLocker m_lockChange;//加锁

	bool m_bGetPic = false;
	WXString m_strJpeg;
	int m_nTypeJPG;
	int m_iQuality;

	int m_bFirst = FALSE;
public:
	int64_t GetVideoTimeOut();
	void GetPicture(int type, WXCTSTR wszName, int quality);
	void ThreadProcess();
public:
	void Start();
	void Stop();
	void ChangeRect(int x, int y, int w, int h);
};

#endif
