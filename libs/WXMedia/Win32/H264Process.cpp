/*
GDI 采集后 转化为指定大小图片并编码为H264
*/
#ifdef _WIN32
#include <WXMediaCpp.h>


class H264Process : public WXThread {

	WXLocker m_mutex;

	int m_nTime = 67;

	int m_iWidth = 0;
	int m_iHeight = 0;

	WXVideoFrame m_pVideoFrameRGB32;
	WXVideoFrame m_pVideoFrameYUV;

	int m_iPosX = 0;
	int m_iPosY = 0;
	int m_iRectW = 0;
	int m_iRectH = 0;

	bool m_bOpen = false;

	void *m_sink = nullptr;
	OnH264VideoData m_cb = nullptr;
	int m_iIndex = -2;

	HDC m_hDC = nullptr;
	HDC m_hMemDC = nullptr;// 屏幕和内存设备描述表
	HBITMAP m_hBitmap = nullptr; //直接抓图

	AVCodecContext *m_pVideoCtx = nullptr;//编码器
	int64_t m_ptsStart = 0;//开始时间
public:
	//-1 表示主屏幕，
	bool Open(int index = -1,
		int width = 1280,
		int height = 720,
		int fps = 15,
		int bitrate = 2000 * 1000,
		void *sink = nullptr,
		OnH264VideoData cb = nullptr) {
		WXAutoLock al(m_mutex);
		m_iWidth = width;
		m_iHeight = height;
		m_pVideoFrameRGB32.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
		m_pVideoFrameYUV.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
		m_cb = cb;
		m_sink = sink;
		m_nTime = 1000 / fps;

		AVCodec *codec = codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		m_pVideoCtx = avcodec_alloc_context3(codec);
		m_pVideoCtx->width = m_iWidth;
		m_pVideoCtx->height = height;
		m_pVideoCtx->time_base.den = 1;
		m_pVideoCtx->time_base.num = fps;
		m_pVideoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
		m_pVideoCtx->bit_rate = bitrate;

		m_pVideoCtx->rc_min_rate = 0;
		m_pVideoCtx->rc_max_rate = bitrate;
		m_pVideoCtx->rc_buffer_size = bitrate;
		m_pVideoCtx->gop_size = fps;// 1s 一个关键帧

		av_opt_set(m_pVideoCtx->priv_data, "profile", "Baseline", 0);
		av_opt_set(m_pVideoCtx->priv_data, "preset", "superfast", 0);
		av_opt_set(m_pVideoCtx->priv_data, "tune", "zerolatency", 0);
		m_pVideoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; //生成extradata
		int ret = avcodec_open2(m_pVideoCtx, codec, nullptr);
		if (ret < 0) {
			WXLogW(L"X264 Encoder Open Error");
			return false;
		}

		m_hDC = ::GetDC(nullptr);//桌面DC

		m_iIndex = index;

		MonitorInfo *info = WXScreenGetInfo(m_iIndex);
		m_iPosX = info->left;
		m_iPosY = info->top;

		if (m_hDC == nullptr) {
			m_hDC = ::GetDC(nullptr);//桌面DC
			m_hMemDC = ::CreateCompatibleDC(m_hDC);//内存DC , 使用m_hDC 会导致画不出鼠标
			m_hBitmap = ::CreateCompatibleBitmap(m_hDC, m_iWidth, m_iHeight);
		}

		m_iRectW = info->width;
		m_iRectH = info->height;


		m_bOpen = true;
		ThreadSetName(L"H264Process");
		ThreadStart();
		return true;
	}
	void Close() {
		if (m_bOpen) {
			ThreadStop();
		}
	}
	std::atomic<bool> m_bChangeIndex = false;
	void Switch(int index) { //切换屏幕
		if (index == m_iIndex)return;
		if (!m_bChangeIndex) {
			m_bChangeIndex = true;
			m_iIndex = index;
		}
	}

private:
	void GetXY(int srcWidth, int srcHeight, int dstWidth, int dstHeight, int& desX, int& desY) {
		desX = 0;
		desY = 0;
		int sw1 = dstHeight * srcWidth / srcHeight;
		int sh1 = dstWidth * srcHeight / srcWidth;
		if (sw1 <= dstWidth) {
			desX = (dstWidth - sw1) / 2;
		}else {
			desY = (dstHeight - sh1) / 2;
		}
	}
	void GrabFrame(int64_t pts) {
		WXAutoLock al(m_mutex);

		if (m_hDC == nullptr) {
			m_hDC = ::GetDC(nullptr);//桌面DC
			m_hMemDC = ::CreateCompatibleDC(m_hDC);//内存DC , 使用m_hDC 会导致画不出鼠标
			m_hBitmap = ::CreateCompatibleBitmap(m_hDC, m_iWidth, m_iHeight);
		}

		if (m_bChangeIndex) {
			MonitorInfo *info = WXScreenGetInfo(m_iIndex);
			m_iPosX = info->left;
			m_iPosY = info->top;
			m_iRectW = info->width;
			m_iRectH = info->height;
			m_bChangeIndex = false;
		}

		HBITMAP hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hBitmap);

		BOOL bScale = FALSE;
		BOOL bCopyDC = FALSE;
		if (m_iRectW != m_iWidth || m_iRectH != m_iHeight) {
			bScale = TRUE;
			::SetStretchBltMode(m_hMemDC, HALFTONE);//硬件缩放
			bCopyDC = ::StretchBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight,
				m_hDC, m_iPosX, m_iPosY, m_iRectW, m_iRectH,
				SRCCOPY);//有拖影
		}
		else {
			bCopyDC = ::BitBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight,
				m_hDC, m_iPosX, m_iPosY, SRCCOPY);//有拖影
		}

		if (!bCopyDC) {
			WXLogA("%s bCopyDC Error", __FUNCTION__);
			m_hBitmap = (HBITMAP)SelectObject(m_hMemDC, hOldBitmap);
			SAFE_RELEASE_DC(nullptr, m_hDC)
				SAFE_DELETE_OBJECT(m_hBitmap)
				SAFE_DELETE_DC(m_hMemDC)
				return;
		}
		//画鼠标

		//获取 m_hMemDC RGBA 数据
		m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, hOldBitmap);
		int nGetDIBits = ::GetDIBits(m_hMemDC, m_hBitmap, 0, m_iHeight,
			(LPSTR)m_pVideoFrameRGB32.GetFrame()->data[0], (BITMAPINFO*)m_pVideoFrameRGB32.GetBIH(), DIB_RGB_COLORS);
		if (0 >= nGetDIBits) {
			WXLogA("%s GetDIBits Error", __FUNCTION__);
			SAFE_RELEASE_DC(nullptr, m_hDC)
				SAFE_DELETE_OBJECT(m_hBitmap)
				SAFE_DELETE_DC(m_hMemDC)
				return;
		}

		libyuv::ARGBToI420(m_pVideoFrameRGB32.GetFrame()->data[0], m_pVideoFrameRGB32.GetFrame()->linesize[0],
			m_pVideoFrameYUV.GetFrame()->data[0], m_pVideoFrameYUV.GetFrame()->linesize[0],
			m_pVideoFrameYUV.GetFrame()->data[1], m_pVideoFrameYUV.GetFrame()->linesize[1],
			m_pVideoFrameYUV.GetFrame()->data[2], m_pVideoFrameYUV.GetFrame()->linesize[2],
			m_iWidth, m_iHeight);

		if (m_cb) {
			int got_packet = 0;
			AVPacket pkt = { 0 };
			av_init_packet(&pkt);
			avcodec_encode_video2(m_pVideoCtx, &pkt, m_pVideoFrameYUV.GetFrame(), &got_packet);
			if (got_packet) {
				m_cb(m_sink, pkt.data, pkt.size, pts);//输出H264
			}
			av_packet_unref(&pkt);
		}
	}

public:
	virtual  void ThreadPrepare() {
		WXLogW(L"Begin");
		m_ptsStart = WXGetTimeMs();
		if (m_cb)
			m_cb(m_sink, m_pVideoCtx->extradata, m_pVideoCtx->extradata_size, -1);//发送extradata
	}

	virtual  void ThreadProcess() {
		int64_t pts1 = WXGetTimeMs() - m_ptsStart;

		//采集编码
		GrabFrame(pts1);

		int64_t pts2 = WXGetTimeMs() - pts1 - m_ptsStart;
		SLEEPMS(m_nTime - pts2);
	}

	virtual  void ThreadPost() {
		avcodec_close(m_pVideoCtx);
		avcodec_free_context(&m_pVideoCtx);
		m_pVideoCtx = nullptr;
		SAFE_DELETE_DC(m_hDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
		SAFE_DELETE_DC(m_hMemDC)
	}
};

WXLocker s_LockH264;
WXMEDIA_API void *H264ProcessCreate(int index/* = -1*/,
	int width/* = 1280*/,
	int height/* = 720*/,
	int fps/* = 15*/,
	int bitrate/* = 2000 * 1000*/,
	void * sink/* = nullptr*/,
	OnH264VideoData cb/* = NULL*/) {
	WXAutoLock al(s_LockH264);
	H264Process *p = new  H264Process();
	if (p->Open(index, width, height, fps, bitrate, sink, cb)) {
		return (void*)p;
	}
	else {
		delete p;
	}
	return nullptr;
}


WXMEDIA_API void H264ProcessDestroy(void *ptr) {
	WXAutoLock al(s_LockH264);
	if (ptr) {
		H264Process*p = (H264Process*)ptr;
		p->Close();
		delete p;
	}
}

WXMEDIA_API void H264ProcessSwitch(void *ptr, int index) {
	WXAutoLock al(s_LockH264);
	if (ptr) {
		H264Process*p = (H264Process*)ptr;
		p->Switch(index);
	}
}

#endif
