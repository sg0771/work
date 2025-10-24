/*
	窗口录制
	by Tam.Xie 2018.12.10
*/
#ifndef _WX_WINDOW_CAPTURE_H_
#define _WX_WINDOW_CAPTURE_H_

#include <WXMediaCpp.h>

#include "VideoSource.h"

class WindowCapture :public VideoSource{
	int m_iOrgW = 0;
	int m_iOrgH = 0;//原始窗口分辨率

	int m_dx = 0;
	int m_dy = 0;
	
	WXVideoFrame m_captureFrame;
	UINT m_flags = 0;
	WXCHAR m_wszFileName[MAX_PATH];
	bool m_bWin7 = FALSE;
public:
	//采集线程
	void ThreadProcess() {
		WXVideoFrame* VideoFrame = m_queuePool.Pop();
		if (VideoFrame == nullptr) { //队列已经满了
			return;
		}
		m_ptsVideo = WXGetTimeMs();
		int ret = GrabFrameImpl(VideoFrame);
		if (ret > 0) { //采集成功
			//m_nVideoFrame++;
			m_ptsLast = m_ptsVideo;
			VideoFrame->GetFrame()->pts = m_ptsVideo;//采集时间戳
			this->PushVideoData(VideoFrame);
		}
		else { //采集失败
			m_queuePool.Push(VideoFrame);
			//WXLogW(L"GrabFrameImpl Error!!!");
			if (m_iMachLevel != LEVEL_BEST) {
				SLEEPMS(m_iTime);
			}
			else {
				SLEEPMS(1);
			}
			return;
		}

		int64_t ptsVideo2 = WXGetTimeMs() - m_ptsVideo;
		int64_t ptsSleep = m_iTime - ptsVideo2;
		if (ptsSleep > 0) {
			SLEEPMS(ptsSleep);
		}
	}
	virtual  int      Init();//
	virtual  void     Start();//启动
	virtual  void     Stop();     //结束
	virtual  int      GrabFrameImpl(WXVideoFrame* avframe); //获取数据,返回1成功，返回0失败
	virtual  WXCTSTR  Type();  //类型
};

#endif //_WX_WINDOW_CAPTURE_H_

