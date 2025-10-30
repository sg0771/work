/*
基于GDI的窗口采集类
在Win10下采集基于libcef的应用程序会有问题

2020.11.4  Win10 采集CEF的应用正常了，
但是Win7 下面录制CEF程序可能闪黑
*/
#ifdef _WIN32
#include "WindowCapture.h"
#include "WXCapture.h"

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT    0x00000002
#endif

int WindowCapture::Init() {
	SetCapture(); 
	if (m_video.m_hwndCapture == NULL)
		return WX_ERROR_VIDEO_DEVICE_OPEN;//桌面DC

	m_bWin7 = WXGetSystemVersion() == 7;
	m_flags = m_bWin7 ? 0 : PW_RENDERFULLCONTENT;

	m_hWnd = m_video.m_hwndCapture;
	m_strName = m_video.m_wszDevName;

	DWORD  _processID = 0;
	DWORD  _threadID = ::GetWindowThreadProcessId(m_hWnd, &_processID);
	TCHAR exe_name[MAX_PATH];
	WXGetProcessName(_processID, exe_name);

	if (wcsicmp(exe_name, L"notepad") == 0) {
		m_flags = 0;//不让记事本闪烁
	}

	int bQQ = (wcsicmp(exe_name, L"QQ") == 0);
	LogW(L"WindowCapture EXE  is[%ws]", exe_name);
	RECT rc; //获取录制区域大小
	::GetWindowRect(m_hWnd, &rc);

	m_iOrgW = rc.right - rc.left;
	m_iOrgH = rc.bottom - rc.top;
	LogW(L"WindowRect[%d,%d,%d,%d] Size=[%dx%d]", (int)rc.left, (int)rc.top, (int)rc.right, (int)rc.bottom, (int)m_iOrgW, (int)m_iOrgH);
	
	RECT rcC; //获取录制区域大小
	::GetClientRect(m_hWnd, &rcC);
	m_hDC = ::GetWindowDC(m_hWnd);

	/*auto g_DPIScaleX = GetDeviceCaps(m_hDC, LOGPIXELSX) / 96.0f;
	auto g_DPIScaleY = GetDeviceCaps(m_hDC, LOGPIXELSY) / 96.0f;
	m_iOrgW = m_iOrgW * g_DPIScaleX;
	m_iOrgH = m_iOrgH * g_DPIScaleY;*/
	if (bQQ) {
		m_dy = 8;//
		m_dx = 8;
		m_iWidth = (m_iOrgW - 2 * m_dx) / 4 * 4;
		m_iHeight = (m_iOrgH - 2 * m_dy) / 4 * 4;
	}else {
		m_dx = (m_iOrgW - rcC.right) / 2;//一般是8像素
		m_dy = 0;
		m_iWidth = rcC.right / 4 * 4;
		m_iHeight = rcC.bottom  / 4 * 4;
		if (m_iOrgW > rcC.right) {
			m_iHeight = (m_iOrgH-8) / 4 * 4;
		}
	}

	LogW(L"ClientRectSize=[%dx%d]",  (int)rcC.right, (int)rcC.bottom);

	if (m_iOrgW <= 100 || m_iOrgH <= 100)
		return WX_ERROR_VIDEO_DEVICE_OPEN;//
	/*auto g_DPIScaleX = GetDeviceCaps(m_hDC, LOGPIXELSX) / 96.0f;
	auto g_DPIScaleY = GetDeviceCaps(m_hDC, LOGPIXELSY) / 96.0f;
	m_iOrgW = (m_iOrgW * g_DPIScaleX) ;
	m_iOrgW = m_iOrgW / 4 * 4;
	m_iOrgH = m_iOrgH * g_DPIScaleY ;
	m_iWidth = (m_iWidth * g_DPIScaleX);
	m_iWidth = m_iWidth / 4 * 4;
	m_iHeight = m_iHeight * g_DPIScaleY ;*/



	m_captureFrame.Init(AV_PIX_FMT_RGB32, m_iOrgW, m_iOrgH);//原始数据
	m_hBitmap = ::CreateCompatibleBitmap(m_hDC, m_iOrgW, m_iOrgH);
	m_hMemDC  = ::CreateCompatibleDC(m_hDC);//内存DC , 使用m_hDC 会导致画不出鼠标

	m_dstFmt = AV_PIX_FMT_RGB32;
	m_InputVideoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//采集图像

	m_nPool = m_iMachLevel == LEVEL_BEST ? MAX_POOL : 1;
	m_aData = new WXVideoFrame[m_nPool];
	for (int i = 0; i < m_nPool; i++) {  //初始化
		m_aData[i].Init(m_dstFmt, m_iWidth, m_iHeight);
		m_queuePool.Push(&m_aData[i]);
	}
	wcscpy(m_wszFileName, L"ApowerSoft Window Capture");
	return WX_ERROR_SUCCESS;
}

void WindowCapture::Start() {
	m_bStart = TRUE;
	GrabFrameImpl(nullptr);
	GrabFrameImpl(nullptr);
	ThreadSetName(L"WindowCapture");
	ThreadStart(true);
}

//关闭设备
void WindowCapture::Stop() {
	if (m_bStart) {
		m_bStart = FALSE;
		ThreadStop();
	}
}



//中途可能会因为拖动窗口导致大小的变化，这个时候发给消息回去，然后输出全黑的数据
int WindowCapture::GrabFrameImpl(WXVideoFrame* avframe) {
	if (!IsWindow(m_hWnd)) {
		//通知上层分辨率改变了，可能是Resize或者最小化
		if (m_cbEvent) {
			void* pEventSink = m_pEventSink;//回调对象，可能是C++对象
			wxCallBack cbEvent = m_cbEvent;//事件回调
			WXString strName = m_wszFileName;
			WXTask task = [cbEvent, pEventSink, strName] {
				cbEvent(pEventSink, WX_EVENT_ID_WINDOWCAPTURE_NO_DATA, (void*)(strName.str()));
			};
			WXTaskPost(OTHER_WORK_THREAD,task);
		}
		memset(avframe->GetFrame()->data[0], 0, avframe->GetFrame()->linesize[0] * avframe->GetFrame()->height);//刷黑，填充黑帧
		g_bWXCaptureStopFlag = TRUE;//返回黑帧
		return 1;
	}

	RECT rc;
	::GetWindowRect(m_hWnd, &rc);
	int sw = rc.right - rc.left;
	int sh = rc.bottom - rc.top;
	//auto g_DPIScaleX = GetDeviceCaps(m_hDC, LOGPIXELSX) / 96.0f;
	/*auto g_DPIScaleY = GetDeviceCaps(m_hDC, LOGPIXELSY) / 96.0f;
	sw = sw * g_DPIScaleX;
	sw = sw / 4 * 4;
	sh *= g_DPIScaleY;*/
	if (sw != m_iOrgW || sh != m_iOrgH) {
		//通知上层分辨率改变了，可能是Resize或者最小化
		if (m_cbEvent) {
			void* pEventSink = m_pEventSink;//回调对象，可能是C++对象
			wxCallBack cbEvent = m_cbEvent;//事件回调
			WXString strName = m_wszFileName;
			WXTask task = [cbEvent, pEventSink, strName] {
				cbEvent(pEventSink, WX_EVENT_ID_WINDOWCAPTURE_NO_DATA, (void*)(strName.str()));
			};
			WXTaskPost(OTHER_WORK_THREAD,task);
		}
		memset(avframe->GetFrame()->data[0], 0, avframe->GetFrame()->linesize[0] * avframe->GetFrame()->height);//刷黑，填充黑帧
		g_bWXCaptureStopFlag = TRUE;//返回黑帧
		return 1;
	}

	if (nullptr == m_hDC) {
		m_hDC = ::GetWindowDC(m_hWnd);
		m_hBitmap = ::CreateCompatibleBitmap(m_hDC, m_iOrgW, m_iOrgH);
		m_hMemDC = ::CreateCompatibleDC(m_hDC);//内存DC , 使用m_hDC 会导致画不出鼠标
	}

	

	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);

	if (!m_bWin7 || !m_bCaptureBlt) {
		BOOL bPrintWindow  = ::PrintWindow(m_hWnd, m_hMemDC, m_flags);//解决Chrome 无法录制的问题

		if (!bPrintWindow)
		{
			bPrintWindow = ::BitBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight, m_hDC, 0, 0, SRCCOPY);//基于区域录制避免Win7 CEF的黑屏
		}

		if (!bPrintWindow) { //截取指定窗口图像失败

			m_nError++;
			LogA("%s BitBlt Error!!  AAAA m_flags=%08x m_hWnd: %d", __FUNCTION__, m_flags, m_hWnd);
			m_hBitmap = (HBITMAP)SelectObject(m_hMemDC, hOldBitmap);
			SAFE_RELEASE_DC(m_hWnd, m_hDC)
			SAFE_DELETE_OBJECT(m_hBitmap)
			SAFE_DELETE_DC(m_hMemDC)
			return 0;
		}
	}else {
		BOOL bPrintWindow = ::BitBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight, m_hDC, 0, 0, SRCCOPY);//基于区域录制避免Win7 CEF的黑屏
		if (!bPrintWindow) { //截取指定窗口图像失败		
			m_nError++;
			LogA("%s BitBlt Error!!  BBBB", __FUNCTION__);
			m_hBitmap = (HBITMAP)SelectObject(m_hMemDC, hOldBitmap);
			SAFE_RELEASE_DC(m_hWnd, m_hDC)
			SAFE_DELETE_OBJECT(m_hBitmap)
			SAFE_DELETE_DC(m_hMemDC)
			return 0;
		}
	}

	m_bMouseVisable = FALSE;
	if (m_mouse.m_iUsed) {
		// 获取当前光标记起位置
		if (::GetCursorPos(&m_ptCursor)) {
			CURSORINFO cursorInfo;
			cursorInfo.cbSize = sizeof(CURSORINFO);
			::GetCursorInfo(&cursorInfo);

			// 获取光标的图像数据
			if (::GetIconInfo(cursorInfo.hCursor, &m_iconInfo)) {
				m_hCursor = cursorInfo.hCursor;
				if (m_iconInfo.hbmMask != nullptr) {
					DeleteObject(m_iconInfo.hbmMask);
				}
				if (m_iconInfo.hbmColor != nullptr) {
					DeleteObject(m_iconInfo.hbmColor);
				}
				m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
				m_ptCursor.y -= ((int)m_iconInfo.yHotspot);
				m_ptCursor.x -= rc.left;
				m_ptCursor.y -= rc.top;
				m_bMouseVisable = true;
			}
		}
	}

	if (m_bMouseVisable)
		DrawEx(m_ptsVideo, m_ptCursor.x, m_ptCursor.y, m_dx, m_dy);
	else
		DrawEx(m_ptsVideo,0,0, m_dx, m_dy);

	//获取 m_hMemDC RGBA 数据
	m_hBitmap = (HBITMAP)SelectObject(m_hMemDC, hOldBitmap);
	int nGetDIBits =::GetDIBits(m_hMemDC, m_hBitmap, 0, m_iOrgH, 
		(LPSTR)m_captureFrame.GetFrame()->data[0], (BITMAPINFO*)m_captureFrame.GetBIH(), DIB_RGB_COLORS);

	if (0 == nGetDIBits) {
		m_nError++;
		LogA("%s GetDIBits Error!!", __FUNCTION__);
		SAFE_RELEASE_DC(m_hWnd, m_hDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
		SAFE_DELETE_DC(m_hMemDC)
		return 0;
	}else if(avframe) {
		libyuv::ARGBCopy(m_captureFrame.GetFrame()->data[0] + m_dx * 4 + m_dy * m_captureFrame.GetFrame()->linesize[0], m_captureFrame.GetFrame()->linesize[0],
			avframe->GetFrame()->data[0], avframe->GetFrame()->linesize[0],
			m_iWidth, m_iHeight
		);
	}
	return 1;
}

WXCTSTR WindowCapture::Type() {
	return L"Window";
}


#endif
