/*
视频数据源
*/
#ifndef _IWXVIDEO_DEV_H
#define _IWXVIDEO_DEV_H

#include "WXMediaCpp.h"
#include "WXCapture.h"

//双缓冲

#define USE_OLD_VERSION  1

class  VideoSource : public WXThread {
public:
	int m_iMachLevel = LEVEL_BETTER;//默认性能机器

	WXCond m_condData;
	void   PushVideoData(WXVideoFrame* VideoFrame) {
		m_queueData.Push(VideoFrame);
		m_condData.notify_all();
	}


	WXVideoFrame* GetVideoFrame() {
		//CMutex mutex;
		//CLockMutex lock(mutex);//定义独占锁
		//m_condData.wait(lock, [this] {
		//	return !m_queueData.Empty();
		//}); //堵塞到有数据
		return m_queueData.Pop();
	}

	void   ThreadPrepare() {
		//LogA("++++++ VideoSource ThreadPrepare WXCond_Wait");
	}

	void   ThreadWait() {
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);//线程优先级
	}


public:
	virtual int      Init() = 0;//
	virtual void     Start() = 0;//启动
	virtual void     Stop() = 0;     //结束
	virtual WXCTSTR  Type() = 0;  //类型
	virtual int      GrabFrameImpl(WXVideoFrame* avframe) = 0;
	virtual  void    ChangeRect(int x, int y, int w, int h) {

		WXTask task = [this, x, y, w, h] {
#ifdef _WIN32
			// WXAutoLock al(m_mutex);
			int nNewPosX = m_iRectPosX;
			int nNewPosY = m_iRectPosY;
			int nNewRectW = m_iRectW;
			int nNewRectH = m_iRectH;

			if (nNewPosX != x || nNewPosY != y) {
				if (!m_video.m_bFollowMouse) {
					WXLogA("%s Change Postion [%d,%d]-->[%d,%d]", __FUNCTION__, m_iRectPosX, m_iRectPosY, x, y);
					nNewPosX = x;
					nNewPosY = y;
				}
			}

			if (w > 0 || h > 0) {
				int64_t new_w = w / 2 * 2;
				int64_t new_h = h / 2 * 2;
				if (new_w != nNewRectW || new_h != nNewRectH) {
					WXLogA("%s Change Size [%dx%d]-->[%dx%d]",
						__FUNCTION__, m_iRectW, m_iRectH, new_w, new_h);
					nNewRectW = new_w;
					nNewRectH = new_h;
				}
			}
			m_iRectPosX = nNewPosX;
			m_iRectPosY = nNewPosY;
			m_iRectW = nNewRectW;
			m_iRectH = nNewRectH;
#endif
		};
		RunTask(task);
	}
public:
	int64_t m_ptsLast = 0; //上一帧采集时间
	int64_t m_ptsVideo = 0;//视频采集时间

	WXVideoFrame  m_tempFrame;//RGBA
	WXVideoFrame  m_InputVideoFrame; //输入的图像


#define MAX_POOL 3

	//采集缓存
	int m_nPool = 1;

	//WXVideoFrame  m_aData[MAX_POOL];//数据区
	
	WXVideoFrame*  m_aData = nullptr;//数据区

	ThreadSafeQueue<WXVideoFrame*>m_queuePool;
	ThreadSafeQueue<WXVideoFrame*>m_queueData;
public:
	static VideoSource* Create(WXCTSTR wszType);
	static void Destroy(VideoSource* p);

public:

	VideoSource() {
		memset(&m_video, 0, sizeof(m_video));
		memset(&m_text, 0, sizeof(m_text));
		memset(&m_image1, 0, sizeof(m_image1));
		memset(&m_image2, 0, sizeof(m_image2));
		memset(&m_mouse, 0, sizeof(m_mouse));

		//全局水印
		WXCTSTR wszName = WXCaptureGetWM();
		if (wszName) {
			m_image2.m_bUsed = 1;
			m_image2.m_iPosX = WXCaptureGetWMPosX();
			m_image2.m_iPosY = WXCaptureGetWMPosY();
			wcscpy(m_image2.m_wszFileName, wszName);
		}

		m_iMachLevel = WXGetGlobalValue(L"MachLevel", LEVEL_BETTER);
	}
	virtual ~VideoSource() {
#ifdef _WIN32
		SAFE_DELETE(m_pTextBrush)
		SAFE_DELETE(m_pFontFamily)
		SAFE_DELETE(m_pFont)

		SAFE_DELETE(m_pImage1)
		SAFE_DELETE(m_pImage2)

		SAFE_RELEASE_DC(m_hWnd, m_hDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
		SAFE_DELETE_DC(m_hMemDC)
		SAFE_DELETE(m_pBrushMouse);
		SAFE_DELETE_ARRAY(m_aData)
#endif
	}
public:
	//鼠标
#ifdef _WIN32
	int m_bUseMouse = FALSE;//使用鼠标
	int m_bMouseVisable = FALSE;//鼠标是否可见
	POINT    m_ptCursor;
	HCURSOR  m_hCursor = nullptr;
	ICONINFO m_iconInfo;
	int m_iAlphaHotdot = 0;//热点透明度,0 为全透明，1为不透明
	int m_iAlphaAnimation = 0;//动画透明度
	Gdiplus::SolidBrush* m_pBrushMouse = nullptr;
	int m_bClickLeft = FALSE;
	int m_bClickRight = FALSE;
	int m_lastX = -1000;
	int m_lastY = -1000;
	int64_t m_ptsMouseLeftAction = -1; //鼠标左键点击时间
	int64_t m_ptsMouseRightAction = -1; //鼠标右键点击时间

	//文字水印
	int m_bText = FALSE;
	Gdiplus::SolidBrush* m_pTextBrush = nullptr;// (Gdiplus::Color(128, 0, 255, 0));
	Gdiplus::FontFamily* m_pFontFamily = nullptr;// (m_text.m_wszFontName);
	Gdiplus::Font* m_pFont = nullptr;// (&m_pFontFamily, m_text.m_iFontSize);
	Gdiplus::PointF m_ptText{ 0, 0 };//PointF类对点进行了封装，这里是指定写字的开始点

	//图像水印
	int m_bImage1 = FALSE;
	Gdiplus::Rect m_rcImage1;
	int m_nImageW1 = 0;
	int m_nImageH1 = 0;
	Gdiplus::Image* m_pImage1 = nullptr;//水印图片1

	int m_bImage2 = FALSE;
	Gdiplus::Rect m_rcImage2;
	int m_nImageW2 = 0;
	int m_nImageH2 = 0;
	Gdiplus::Image* m_pImage2 = nullptr;//水印图片2

	//GDI录制
	BOOL m_bCaptureBlt = FALSE;
	uint32_t m_uCaptureBlt = SRCCOPY;
	HDC m_hDC = nullptr;
	HDC m_hMemDC = nullptr;// 屏幕和内存设备描述表
	HBITMAP m_hBitmap = nullptr;

	HWND m_hWnd = nullptr;
#endif
	//录屏对象
	void SetCapture() {

		if (m_video.m_iFps == 0)
			m_video.m_iFps = 24;

		if (m_iMachLevel == LEVEL_BEST) {
			m_iTime = (int)(1000.0f / double(m_video.m_iFps)) - 2; //增加实际采集率
		}
		else {
			m_iTime = (int)(1000.0f / (double)(m_video.m_iFps) + 0.8);
			if (m_iWidth * m_iHeight > 1920 * 1080 && m_video.m_iFps >= 20)
				m_iTime += 3;
		}
		//WXLogW(L"VideoSource Time=%d m_nPool=%d", m_iTime, m_nPool);
	
#ifdef _WIN32
		m_uCaptureBlt = !!m_video.m_bCaptureBlt ? SRCCOPY | CAPTUREBLT : SRCCOPY;//CAPTUREBLT 鼠标有闪烁
		m_bCaptureBlt = !!m_video.m_bCaptureBlt;//

		if (m_image1.m_bUsed && WXStrlen(m_image1.m_wszFileName)) {
			m_pImage1 = new Gdiplus::Image(m_image1.m_wszFileName);
			m_nImageW1 = m_pImage1->GetWidth();
			m_nImageH1 = m_pImage1->GetHeight();
			if (m_nImageW1 == 0 || m_nImageH1 == 0) {
				SAFE_DELETE(m_pImage1);
			}
			else {
				m_bImage1 = TRUE;
				m_rcImage1.X = m_image1.m_iPosX;
				m_rcImage1.Y = m_image1.m_iPosY;
				m_rcImage1.Width = m_pImage1->GetWidth();
				m_rcImage1.Height = m_pImage1->GetHeight();
			}
		}

		if (m_image2.m_bUsed && WXStrlen(m_image2.m_wszFileName)) {
			m_pImage2 = new Gdiplus::Image(m_image2.m_wszFileName);
			m_nImageW2 = m_pImage2->GetWidth();
			m_nImageH2 = m_pImage2->GetHeight();
			if (m_nImageW2 == 0 || m_nImageH1 == 2) {
				SAFE_DELETE(m_pImage2);
			}
			else {
				m_bImage2 = TRUE;
				m_rcImage2.X = m_image2.m_iPosX;
				m_rcImage2.Y = m_image2.m_iPosY;
				m_rcImage2.Width = m_pImage2->GetWidth();
				m_rcImage2.Height = m_pImage2->GetHeight();
			}

		}
		m_bUseMouse = m_mouse.m_iUsed | m_video.m_bFollowMouse;//是否使用鼠标信息

		if (m_mouse.m_iUsed) { //鼠标事件
			//WXLogW(L"Desktop Capture Mouse ");
			m_iAlphaHotdot = m_mouse.m_fAlphaHotdot * 255;
			m_iAlphaAnimation = m_mouse.m_fAlphaAnimation * 255;
			m_pBrushMouse = new Gdiplus::SolidBrush(Gdiplus::Color(m_iAlphaHotdot,
				GetRValue(m_mouse.m_colorMouse),
				GetGValue(m_mouse.m_colorMouse),
				GetBValue(m_mouse.m_colorMouse)));
		}
#endif
	}

	//绘制鼠标、文字、图像等东西
	void DrawEx(int64_t ptsCapture, int posx, int posy, int dx = 0, int dy = 0) {
#ifdef _WIN32
		if (m_hMemDC == nullptr)return;
		if (m_bImage1 == FALSE &&
			m_bImage2 == FALSE &&
			m_bText == FALSE && m_bMouseVisable == FALSE)
			return;

		Gdiplus::Graphics display(m_hMemDC);
		display.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);//抗锯齿

		if (m_bImage1) {
			Gdiplus::Rect rcDst{ dx + m_image1.m_iPosX, dy + m_image1.m_iPosY,m_nImageW1,m_nImageH1 };
			display.DrawImage(m_pImage1, rcDst/*, 0, 0, m_nImageW1, m_nImageH1, Gdiplus::UnitPixel*/);
		}

		if (m_bImage2) {
			Gdiplus::Rect rcDst{ dx + m_image2.m_iPosX, dy + m_image2.m_iPosY,m_nImageW2,m_nImageH2 };
			display.DrawImage(m_pImage2, rcDst/*, 0, 0, m_nImageW2, m_nImageH2, Gdiplus::UnitPixel*/);
		}

		if (m_bText) { //文字水印
			Gdiplus::PointF ptText{ m_ptText.X + dx, m_ptText.Y + dy };
			display.DrawString(m_text.m_wszText, -1, m_pFont, ptText, m_pTextBrush);
		}

		if (m_bMouseVisable) { //鼠标绘制
			int DrawX = posx + dx;
			int DrawY = posy + dy;
			if (m_mouse.m_bMouseAnimation || m_mouse.m_bMouseHotdot) {

				BOOL bDrawHotDot = TRUE;

				//鼠标动画效果！！
				if (m_mouse.m_bMouseAnimation) {
					int RadiuBase = m_mouse.m_iHotdotRadius;

					if (KEY_DOWN(MOUSE_MOVED)) { //鼠标左键动画
						m_ptsMouseLeftAction = ptsCapture;//鼠标点击时间
						m_bClickLeft = TRUE;
						m_bClickRight = FALSE;
						m_lastX = posx;
						m_lastY = posy;
						bDrawHotDot = FALSE;
					}
					else if (KEY_DOWN(MOUSE_EVENT)) { //鼠标右键动画
						m_ptsMouseRightAction = ptsCapture;//鼠标点击时间
						m_bClickLeft = FALSE;
						m_bClickRight = TRUE;
						m_lastX = posx;
						m_lastY = posy;
						bDrawHotDot = FALSE;
					}

					if (m_bClickLeft) {
						int delay = ptsCapture - m_ptsMouseLeftAction;
						delay = delay / 5 * 5;
						if (delay <= MOUSE_TIME) {
							bDrawHotDot = FALSE;
							int DstW = RadiuBase * delay / MOUSE_TIME;
							int PosX = m_lastX + m_iconInfo.xHotspot - DstW;
							int PosY = m_lastY + m_iconInfo.yHotspot - DstW;

							Gdiplus::GraphicsPath path;
							path.AddEllipse(PosX, PosY, 2 * DstW, 2 * DstW);
							Gdiplus::PathGradientBrush PGBrush(&path);
							PGBrush.SetCenterColor(Gdiplus::Color(m_iAlphaAnimation, 255, 255, 255));
							Gdiplus::Color colors[] = { Gdiplus::Color(m_iAlphaAnimation,
								GetRValue(m_mouse.m_colorLeft),
								GetGValue(m_mouse.m_colorLeft),
								GetBValue(m_mouse.m_colorLeft)) };
							int count = 1;
							PGBrush.SetSurroundColors(colors, &count);
							PGBrush.SetCenterPoint(Gdiplus::Point(PosX + DstW, PosY + DstW));
							display.FillEllipse(&PGBrush, PosX, PosY, 2 * DstW, 2 * DstW);
							DrawX = m_lastX;
							DrawY = m_lastY;
						}
						else if (delay <= MOUSE_TIME + 100) {
							// 在兼容设备描述表上画出该光标
							bDrawHotDot = FALSE;
							DrawX = m_lastX;
							DrawY = m_lastY;
						}
						else {
							m_bClickLeft = FALSE;
						}
					}
					else if (m_bClickRight) {
						int delay = ptsCapture - m_ptsMouseRightAction;
						delay = delay / 5 * 5;
						if (delay <= MOUSE_TIME) {
							bDrawHotDot = FALSE;
							int DstW = RadiuBase * delay / MOUSE_TIME;
							int PosX = m_lastX + m_iconInfo.xHotspot - DstW;
							int PosY = m_lastY + m_iconInfo.yHotspot - DstW;

							Gdiplus::GraphicsPath path;
							path.AddEllipse(PosX, PosY, 2 * DstW, 2 * DstW);
							Gdiplus::PathGradientBrush PGBrush(&path);
							PGBrush.SetCenterColor(Gdiplus::Color(m_iAlphaAnimation, 255, 255, 255));
							Gdiplus::Color colors[] = { Gdiplus::Color(m_iAlphaAnimation,
								GetRValue(m_mouse.m_colorRight),
								GetGValue(m_mouse.m_colorRight),
								GetBValue(m_mouse.m_colorRight)) };
							int count = 1;
							PGBrush.SetSurroundColors(colors, &count);
							PGBrush.SetCenterPoint(Gdiplus::Point(PosX + DstW, PosY + DstW));
							display.FillEllipse(&PGBrush, PosX, PosY, 2 * DstW, 2 * DstW);
							DrawX = m_lastX;
							DrawY = m_lastY;
						}
						else if (delay <= MOUSE_TIME + 100) {
							bDrawHotDot = FALSE;
							DrawX = m_lastX;
							DrawY = m_lastY;
						}
						else {
							m_bClickRight = TRUE;
						}
					}
				}

				//鼠标热点
				if (bDrawHotDot && m_mouse.m_bMouseHotdot) {
					display.FillEllipse(m_pBrushMouse,
						posx - m_mouse.m_iHotdotRadius + m_iconInfo.xHotspot,
						posy - m_mouse.m_iHotdotRadius + m_iconInfo.yHotspot,
						2 * m_mouse.m_iHotdotRadius,
						2 * m_mouse.m_iHotdotRadius);
				}
			}
			//最后绘制鼠标！！
			::DrawIconEx(m_hMemDC, DrawX, DrawY, m_hCursor, 0, 0, 0, nullptr, DI_NORMAL | DI_COMPAT);//鼠标居中
		}
#endif
	}

public:
	WXString m_strName;
public:

	AVPixelFormat m_dstFmt = AV_PIX_FMT_RGB32;//数据格式
	int m_bStart = FALSE;

	//WXLocker m_mutex;
	int m_iRectPosX = 0;
	int m_iRectPosY = 0;//带RECT区域时候的起始位置

	int m_iRectW = 0;
	int m_iRectH = 0;

	int m_nScale = 100;
	int m_iWidth = 0;
	int m_iHeight = 0;
	int m_iTime = 40;//40ms  25fps

	VideoDeviceParam    m_video;//视频录制参数
	TextWaterMarkParam  m_text;//文字水印录制参数
	ImageWaterMarkParam m_image1;//图像水印录制参数,参数携带
	ImageWaterMarkParam m_image2;//图像水印录制参数,全局设置

	MouseParam m_mouse;//鼠标配置参数

	void* m_pEventSink = nullptr;//回调对象，可能是C++对象
	wxCallBack m_cbEvent = nullptr;//事件回调

	void SetEventSink(void* sink, wxCallBack cbData) {
		m_pEventSink = sink;
		m_cbEvent = cbData;
	}

	void SetVideoParam(VideoDeviceParam* param) {
		if (param && param->m_bUse) {
			memcpy(&m_video, param, sizeof(m_video));
		}
	}
	void SetTextParam(TextWaterMarkParam* param) {
		if (param && param->m_bUsed) {
			memcpy(&m_text, param, sizeof(m_text));

			if (m_text.m_bUsed) {
				//文字水印
				int m_nLengthText = WXStrlen(m_text.m_wszText);
				if (m_nLengthText) {
					m_bText = TRUE;

					m_ptText.X = m_text.m_iPosX;
					m_ptText.Y = m_text.m_iPosY;

					m_pTextBrush = new Gdiplus::SolidBrush(Gdiplus::Color(255,
						GetRValue(m_text.m_iColor),
						GetGValue(m_text.m_iColor),
						GetBValue(m_text.m_iColor)));
					m_pFontFamily = new  Gdiplus::FontFamily(m_text.m_wszFontName);
					m_pFont = new Gdiplus::Font(m_pFontFamily, m_text.m_iFontSize, m_text.m_nStyle);
				}
			}
		}
	}
	void SetImageParam(ImageWaterMarkParam* param) {
		if (param && param->m_bUsed) {
			memcpy(&m_image1, param, sizeof(m_image1));
		}
	}
	void SetMouseParam(MouseParam* param) {
		if (param && param->m_iUsed) {
			memcpy(&m_mouse, param, sizeof(m_mouse));
		}
	}

public:
	inline int GetWidth() {
		return m_iWidth;
	}
	inline int GetHeight() {
		return m_iHeight;
	}
};

#endif