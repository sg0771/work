/*
具有外部采集线程的录屏输入
1. 游戏录制
2. Airplay Chromecast等投屏
3. 摄像头
进来的数据的时间戳应该为当前系统时间(单位毫秒)
*/

#ifndef _WX_WINDOWS_AIRPLAY_CAPTURE_H_
#define _WX_WINDOWS_AIRPLAY_CAPTURE_H_

#include "VideoSource.h"

class DataSource :public VideoSource {
	int64_t m_nDrop = 0;//采集丢包
	int64_t m_nCapture = 0;
	int64_t m_nInsert = 0;

public:
	void    PushYUV2(AVFrame *srcFrame);
	void    PushYUV(AVFrame *srcFrame, int rotate);
	void    PushRGBA(AVFrame *srcFrame);
	void    PushI420Data(uint8_t*buf, int width, int height);
	void    PushRGB32Data(uint8_t*buf, int width, int height, int pitch);
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
	virtual int      Init();//
	virtual void     Start();//启动
	virtual void     Stop();     //结束
	virtual int      GrabFrameImpl(WXVideoFrame* avframe) { return 0; }
	virtual WXCTSTR  Type();  //类型
	virtual void ChangeRect(int x, int y, int w, int h) {}
};

#endif //_DESKTOP_CAPTURE_

