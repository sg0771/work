/*
	桌面视频采集类
	by Tam.Xie 2017.08.01
	* Start接口中  index=0表示默认显示器,rect为nullptr 表示全屏抓取
*/
#ifndef _WX_DESKTOP_CAPTURE_
#define _WX_DESKTOP_CAPTURE_

#include <windows.h>
#include <stdint.h>

class WindowsDesktopCapture{
private:
	int m_cx = 0;
	int m_cy = 0;
	int m_iWidth = 0;
	int m_iHeight = 0;
	HDC m_hDesktopDC = nullptr;
	HDC m_hMemDC = nullptr;// 屏幕和内存设备描述表
	HBITMAP m_hBitmap = nullptr, m_hOldBitmap = nullptr; // 位图句柄							 
	BITMAPINFOHEADER m_bi;//位图信息头结构
	uint64_t m_dwTimeStart = 0;

	void DrawIcon();

public:
	WindowsDesktopCapture();
	~WindowsDesktopCapture();
	inline int GetWidth()  { return m_iWidth; }
	inline int GetHeight() { return m_iHeight; }

	void  GetData(uint8_t *buf);
};

#endif //_DESKTOP_CAPTURE_

