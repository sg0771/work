/*
* 混合了DXGI/GDI的录屏类，在主显示器区域录制的时候优先使用DXGI采集
*/

#ifndef _MIXER_CAPTURE_H_
#define _MIXER_CAPTURE_H_

#ifdef _WIN32

#include <WXMediaCpp.h>
#include "VideoSource.h"
#include <d3d11.h>
#include "WXCursorCapture.h"
#include "WXImageWaterMark.h"

//视频帧
class MixVideoFrame :public WXVideoFrame {
public:

	//在内存数据上画圆
	void DrawCircle(uint32_t color) {
		if (m_pixfmt != AV_PIX_FMT_RGB32 || m_pFrame == nullptr || m_iWidth != m_iHeight)return;
		memset(m_pFrame->data[0], 0, m_iWidth * m_pFrame->linesize[0]);
		HDC hDC = ::GetDC(nullptr);
		HDC hMemDC = ::CreateCompatibleDC(hDC);//初始化
		Gdiplus::SolidBrush Brush(Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color)));
		HBITMAP hBitmap = ::CreateBitmap(m_iWidth, m_iWidth, 1, 32, m_pFrame->data[0]);
		HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
		::SetBkColor(hMemDC, RGB(0, 0, 0));
		//::SetBkMode(hMemDC, TRANSPARENT);
		Gdiplus::Graphics graphics(hMemDC);
		graphics.FillEllipse(&Brush, 0, 0, m_iWidth, m_iWidth);//画圆
		hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
		::GetDIBits(hMemDC, hBitmap, 0, m_iWidth, (LPSTR)m_pFrame->data[0], (BITMAPINFO*)&m_bih, DIB_RGB_COLORS);//取数据为RGBA
		::DeleteObject(hOldBitmap);
		::DeleteObject(hBitmap);
		::DeleteDC(hMemDC);
		::ReleaseDC(nullptr, hDC);
	}

	//在内存数据上圆环
	//绘制动态渐变圆环
	void DrawCircle2(uint32_t color, int delay) {
		if (m_pixfmt != AV_PIX_FMT_RGB32 || m_pFrame == nullptr || m_iWidth != m_iHeight)return;
		memset(m_pFrame->data[0], 0, m_iWidth * m_pFrame->linesize[0]);
		HDC hDC = ::GetDC(nullptr);
		HDC hMemDC = ::CreateCompatibleDC(hDC);//初始化

		HBITMAP hBitmap = ::CreateBitmap(m_iWidth, m_iWidth, 1, 32, m_pFrame->data[0]);
		HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
		::SetBkColor(hMemDC, RGB(0, 0, 0));
		//::SetBkMode(hMemDC, TRANSPARENT);
		Gdiplus::Graphics graphics(hMemDC);

		delay = delay / 5 * 5;
		int DstW = m_iWidth / 2 * delay / MOUSE_TIME;
		int PosX = m_iWidth / 2 - DstW;

		Gdiplus::GraphicsPath path;
		path.AddEllipse(PosX, PosX, 2 * DstW, 2 * DstW);
		Gdiplus::PathGradientBrush PGBrush(&path);
		PGBrush.SetCenterColor(Gdiplus::Color(255, 255, 255));
		Gdiplus::Color colors[] = { Gdiplus::Color(GetRValue(color), GetGValue(color), GetBValue(color)) };
		int count = 1;
		PGBrush.SetSurroundColors(colors, &count);
		PGBrush.SetCenterPoint(Gdiplus::Point(m_iWidth / 2, m_iWidth / 2));
		graphics.FillEllipse(&PGBrush, PosX, PosX, 2 * DstW, 2 * DstW);

		hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);
		::GetDIBits(hMemDC, hBitmap, 0, m_iWidth, (LPSTR)m_pFrame->data[0], (BITMAPINFO*)&m_bih, DIB_RGB_COLORS);//取数据为RGBA
		::DeleteObject(hOldBitmap);
		::DeleteObject(hBitmap);
		::DeleteDC(hMemDC);
		::ReleaseDC(nullptr, hDC);
	}
};

class MixerCapture :public VideoSource {
	void *m_pBits = nullptr;
	int  m_nTimeOutIndex = 0;//连续超时次数
	int64_t m_nVideoFrame = 0;
	int64_t m_nVideoFrameTimeOut = 0;

	//采集时的分辨率，全屏游戏的时候可能会改变当前显示器的分辨率
	int m_tmpWidth = 0;
	int m_tmpHeight = 0;

	//初始化状态时的分辨率
	int m_iScreenWidth = 0;
	int m_iScreenHeight = 0;

	WXVideoFrame m_pScreenFrame;//全屏数据，叠加鼠标、热点和水印

	WXVideoFrame m_pScreenBase;//全屏数据，无鼠标

	WXVideoFrame m_RectFrame;//全屏数据，无鼠标

	CComPtr<ID3D11Device>m_pDev = nullptr;//D3D11 设备
	CComPtr<ID3D11DeviceContext>m_pContext = nullptr;//D3D11 上下文
	CComPtr<IDXGIOutputDuplication>m_pDesktopDevice = nullptr;//D3D11 桌面设备
	CComPtr<ID3D11Texture2D>m_pTexture = nullptr;//内存表面

	WXCursorCapture m_cursor;//鼠标信息
	MixVideoFrame m_cursorHotdotFrame;//鼠标热点
	MixVideoFrame m_cursorLeftFrame;  //鼠标左键
	MixVideoFrame m_cursorRightFrame; //鼠标右键

	WXImageWaterMark *m_pImageMark1 = nullptr;//DXGI 水印
	WXImageWaterMark *m_pImageMark2 = nullptr;//DXGI 水印

	WXTextWaterMark *m_pTextMark = nullptr;

	int  OpenDevice(WXCTSTR wszText);
	BOOL DxgiGrab();

	void DrawMouseInfo(int64_t ptsCapture);//绘制鼠标
	void DrawDxgiInfo(int posx,int posy);//文字、图像

										  //主显示器区域
	int m_iScreenLeft = 0;
	int m_iScreenTop = 0;
	int m_iScreenRight = 0;
	int m_iScreenBottom = 0;

	int GDIGrabFrame(WXVideoFrame* frame); //获取数据
	int DXGIGrabFrame(WXVideoFrame* frame); //获取数据
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
	virtual WXCTSTR  Type();  //类型
	virtual int GrabFrameImpl(WXVideoFrame* frame); //获取数据
};

#endif //WIN32

#endif //_WX_WINDOWS_GDI_CAPTURE_H_

