/*
* 基于GDI的桌面视频采集类
* by Tam.Xie 2017.08.01
* 首先枚举显示器设备，获取每个显示器在虚拟屏幕上的位置RECT(x,y,w,h)
* 把主屏幕HDC对应区域内容拷贝到内存DC，然后选入到HBITMAP，从HBITMAP获取RGBA数据
*/
#ifdef _WIN32
#include "WindowsGdiCapture.h"

void WindowsGdiCapture::Start() {
	m_bStart = TRUE;
	GrabFrameImpl(nullptr);
	GrabFrameImpl(nullptr);
	ThreadStart(true);
	ThreadSetName(L"WindowsGdiCapture");
	//LogW(L"%ws OK", __FUNCTIONW__);
}

//初始化GDI采集
int WindowsGdiCapture::Init() {
	SetCapture();

	if (m_video.m_bRect) {
		LogW(L"%ws Desktop Capture Use area screenshots",__FUNCTIONW__);
		MonitorInfo* info = WXScreenGetDefaultInfo();
		if (info == nullptr) {
			WXScreenInit();
			info = WXScreenGetDefaultInfo();
		}
		m_iRectPosX = m_video.m_rcScreen.left;
		m_iRectPosY = m_video.m_rcScreen.top;
		m_iRectW = (m_video.m_rcScreen.right - m_video.m_rcScreen.left) / 2 * 2;
		m_iRectH = (m_video.m_rcScreen.bottom - m_video.m_rcScreen.top) / 2 * 2;

	}else {
		LogW(L"%ws Open Desktop Device [%ws]",  __FUNCTIONW__, m_video.m_wszDevName);
		MonitorInfo* info = WXScreenGetInfoByName(m_video.m_wszDevName);
		if (info == nullptr) {
			info = WXScreenGetDefaultInfo();
		}
		m_video.m_bFollowMouse = FALSE;
		m_iRectPosX = info->left;
		m_iRectPosY = info->top;
		m_iRectW = info->width / 2 * 2;
		m_iRectH = info->height / 2 * 2;
	}

	m_iWidth  = m_iRectW;
	m_iHeight = m_iRectH;
	m_InputVideoFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);//图像

	m_strName = m_video.m_wszDevName;
	m_hDC = ::GetDC(nullptr);
	m_hMemDC = ::CreateCompatibleDC(nullptr);
	m_hBitmap = ::CreateDIBSection(m_hMemDC, (const BITMAPINFO*)m_InputVideoFrame.GetBIH(),
		DIB_RGB_COLORS, (void**)&m_pBits, nullptr, 0);

	m_dstFmt = AV_PIX_FMT_RGB32;
	m_InputVideoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//采集图像

	m_nPool = m_iMachLevel == LEVEL_BEST ? MAX_POOL : 1;
	m_aData = new WXVideoFrame[m_nPool];
	for (int i = 0; i < m_nPool; i++) {  //初始化
		m_aData[i].Init(m_dstFmt, m_iWidth, m_iHeight);
		m_queuePool.Push(&m_aData[i]);
	}
	//LogW(L"%ws OK", __FUNCTIONW__);
	return WX_ERROR_SUCCESS;
}

//关闭设备
void WindowsGdiCapture::Stop() {
	if (m_bStart) {
		ThreadStop();
		m_bStart = FALSE;
		SAFE_RELEASE_DC(nullptr, m_hDC)
		SAFE_DELETE_DC(m_hMemDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
	}
}


int WindowsGdiCapture::GrabFrameImpl(WXVideoFrame* avframe) {

	m_bMouseVisable = FALSE;
	//使用鼠标或者跟随鼠标
	if (m_mouse.m_iUsed || m_video.m_bFollowMouse) { //获取鼠标位置和信息

		// 获取当前光标记起位置
		::GetCursorPos(&m_ptCursor);//基于主屏幕的鼠标位置
		CURSORINFO cursorInfo;
		cursorInfo.cbSize = sizeof(CURSORINFO);
		if (::GetCursorInfo(&cursorInfo)) {
			// 获取光标的图像数据
			if (::GetIconInfo(cursorInfo.hCursor, &m_iconInfo)) {
				if (m_iconInfo.hbmMask != nullptr) {
					DeleteObject(m_iconInfo.hbmMask);
				}
				if (m_iconInfo.hbmColor != nullptr) {
					DeleteObject(m_iconInfo.hbmColor);
				}
				m_bMouseVisable = TRUE; //有鼠标图像
				m_hCursor = cursorInfo.hCursor;
			/*	m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
				m_ptCursor.y -= ((int)m_iconInfo.yHotspot);*/
			}
		}		
	}

	if (m_hDC && m_video.m_bFollowMouse) {
		SAFE_DELETE_OBJECT(m_hBitmap)
		m_pBits = nullptr;
		m_hBitmap = ::CreateDIBSection(m_hMemDC, (const BITMAPINFO*)m_InputVideoFrame.GetBIH(),
			DIB_RGB_COLORS, (void**)&m_pBits, NULL, 0);
	}

	if(nullptr == m_hDC){
		m_hDC = ::GetDC(nullptr);
		m_hMemDC = ::CreateCompatibleDC(NULL);
		m_pBits = nullptr;
		m_hBitmap = ::CreateDIBSection(m_hMemDC, (const BITMAPINFO*)m_InputVideoFrame.GetBIH(),
			DIB_RGB_COLORS, (void**)&m_pBits,NULL, 0);
	}

	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);

	BOOL bScale = FALSE;
	BOOL bCopyDC = FALSE;

	if (m_iRectW != m_iWidth || m_iRectH != m_iHeight) {
		bScale = TRUE;
		SetStretchBltMode(m_hMemDC, HALFTONE);//硬件缩放
		bCopyDC = ::StretchBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight, 
			m_hDC,
			m_video.m_bFollowMouse ? m_ptCursor.x - m_iWidth / 2 - ((int)m_iconInfo.xHotspot): m_iRectPosX,
			m_video.m_bFollowMouse ? m_ptCursor.y - m_iHeight / 2 - ((int)m_iconInfo.yHotspot) : m_iRectPosY,
			m_iRectW,m_iRectH,
			m_uCaptureBlt);//有拖影
	}else {
		bCopyDC = ::BitBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight,
			m_hDC,
			m_video.m_bFollowMouse ? m_ptCursor.x - m_iWidth / 2 - ((int)m_iconInfo.xHotspot) : m_iRectPosX,
			m_video.m_bFollowMouse ? m_ptCursor.y - m_iHeight / 2 - ((int)m_iconInfo.yHotspot) : m_iRectPosY,
			m_uCaptureBlt);
	}

	if (!bCopyDC) { //从主桌面获取数据失败
		m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, hOldBitmap);
		SAFE_RELEASE_DC(nullptr, m_hDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
		SAFE_DELETE_DC(m_hMemDC)
		return 0;
	}

	if (m_bMouseVisable) { //绘制鼠标
		if (m_video.m_bFollowMouse) { //鼠标强制居中
			DrawEx(m_ptsVideo, m_iWidth / 2, m_iHeight / 2);
		}else {
			//鼠标在当前屏幕的位置，虚拟坐标位置
			//暂时不考虑图像缩放后鼠标的缩放问题！
			m_ptCursor.x -= m_iRectPosX;
			m_ptCursor.y -= m_iRectPosY;
			if (bScale) {
				m_ptCursor.x = m_ptCursor.x * m_iWidth / m_iRectW;
				m_ptCursor.y = m_ptCursor.y * m_iHeight / m_iRectH;
			}

			m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
			m_ptCursor.y -= ((int)m_iconInfo.yHotspot);
			DrawEx(m_ptsVideo, m_ptCursor.x, m_ptCursor.y);
		}
	}else {
		DrawEx(m_ptsVideo,0,0);
	}
	//获取 m_hMemDC RGBA 数据
	m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, hOldBitmap);
	if(avframe)
		memcpy(avframe->GetFrame()->data[0], m_pBits, m_iWidth * m_iHeight * 4);//m_pBits已经和
	return 1;
}

WXCTSTR WindowsGdiCapture::Type() {
	return L"GDI";
}
#endif

