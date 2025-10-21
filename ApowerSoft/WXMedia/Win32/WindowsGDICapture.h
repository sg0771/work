/*
* 基于GDI的桌面视频采集类
* 首先枚举显示器设备，获取每个显示器在虚拟屏幕上的位置RECT(x,y,w,h)
* 把主屏幕HDC对应区域内容拷贝到内存DC，然后选入到HBITMAP，从HBITMAP获取RGBA数据
* WIN7 专用！
*/

#ifndef _WX_WINDOWS_GDI_CAPTURE_H_
#define _WX_WINDOWS_GDI_CAPTURE_H_

#include <WXMediaCpp.h>
#include "VideoSource.h"

class WindowsGdiCapture: public VideoSource {
	void *m_pBits = nullptr;
public:	//采集线程
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
	virtual int      Init();//
	virtual void     Start();//启动
	virtual void     Stop();     //结束
	virtual int      GrabFrameImpl(WXVideoFrame* avframe); //获取数据,返回1成功，返回0失败
	virtual WXCTSTR  Type();  //类型
};

#endif //_WX_WINDOWS_GDI_CAPTURE_H_

