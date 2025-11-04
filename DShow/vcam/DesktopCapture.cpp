#include "DesktopCapture.h"

WindowsDesktopCapture::WindowsDesktopCapture(){
	//默认显示器属性
	DEVMODE curDevMode;
	memset(&curDevMode, 0, sizeof(curDevMode));
	curDevMode.dmSize = sizeof(DEVMODE);
	BOOL bEnumRet = ::EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &curDevMode);
	m_iWidth  = curDevMode.dmPelsWidth;
	m_iHeight = curDevMode.dmPelsHeight;
	memset(&m_bi, 0, 40);
	m_bi.biSize = 40;
	m_bi.biWidth = 0;
	m_bi.biHeight = 0;
	m_bi.biPlanes = 1;
	m_bi.biBitCount = 32;
	m_iWidth  = m_iWidth / 4 * 4;
	m_iHeight = m_iHeight / 4 * 4;
	m_bi.biWidth = m_iWidth;
	m_bi.biHeight = m_iHeight;
	m_hDesktopDC = ::GetDC(nullptr);
	m_hMemDC  = CreateCompatibleDC(m_hDesktopDC);
	m_hBitmap = CreateCompatibleBitmap(m_hDesktopDC, m_iWidth, m_iHeight);
}

WindowsDesktopCapture::~WindowsDesktopCapture(){

	if (m_hMemDC) {
		DeleteDC(m_hMemDC);
		m_hMemDC = nullptr;
	}
	if (m_hDesktopDC) {
		::ReleaseDC(nullptr, m_hDesktopDC);
		m_hDesktopDC = nullptr;
	}
	if (m_hOldBitmap) {
		DeleteObject(m_hOldBitmap);
		m_hOldBitmap = nullptr;
	}
	if (m_hBitmap) {
		DeleteObject(m_hBitmap);
		m_hBitmap = nullptr;
	}
}

void WindowsDesktopCapture::DrawIcon(){
	// 获取当前光标记起位置
	HCURSOR hCursor;
	CURSORINFO cursorInfo;
	cursorInfo.cbSize = sizeof(CURSORINFO);
	GetCursorInfo(&cursorInfo);
	hCursor = cursorInfo.hCursor;
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	// 获取光标的图像数据
	ICONINFO iconInfo;
	if (GetIconInfo(hCursor, &iconInfo)){
		ptCursor.x -= ((int)iconInfo.xHotspot);
		ptCursor.y -= ((int)iconInfo.yHotspot);
		if (iconInfo.hbmMask != nullptr){
			DeleteObject(iconInfo.hbmMask);
		}
		if (iconInfo.hbmColor != nullptr){
			DeleteObject(iconInfo.hbmColor);
		}
		// 在兼容设备描述表上画出该光标
		DrawIconEx(m_hMemDC,ptCursor.x, ptCursor.y,hCursor,0, 0,0,nullptr,DI_NORMAL | DI_COMPAT);
	}
}

void WindowsDesktopCapture::GetData(uint8_t *buf){
	m_hOldBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hBitmap);
	StretchBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight, m_hDesktopDC, m_cx, m_cy, m_iWidth, m_iHeight, SRCCOPY);
	DrawIcon();
	m_hBitmap = (HBITMAP)SelectObject(m_hMemDC, m_hOldBitmap);
	GetDIBits(m_hMemDC, m_hBitmap, 0, m_iHeight, (LPSTR)buf, (BITMAPINFO *)&m_bi, DIB_RGB_COLORS);
}

