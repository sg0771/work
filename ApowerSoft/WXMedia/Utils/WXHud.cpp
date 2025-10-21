/*
通过HUD来绘制边框
*/
#include "../WXMediaCpp.h"
#ifdef _WIN32
#define DELTA 5
class HudDraw {
	WXLocker m_mutex;

	WXDataBuffer m_data;
	int m_iRectX = 0;
	int m_iRectY = 0;
	int m_iRectW = 0;
	int m_iRectH = 0;

	int m_iWidth = 0;
	int m_iHeight = 0;

	int time = 0;
	HANDLE hMutex=NULL;   //定义互斥对象
	bool m_bStop = true;
	std::thread *m_thread = NULL;
private:
	CComPtr<IDirectDraw>        m_pDD = NULL;
	CComPtr<IDirectDrawSurface> m_pDDSPrimary = NULL;
	CComPtr<IDirectDrawSurface> m_pDDSOverlay = NULL;
	DDOVERLAYFX          m_OverlayFX;
	DWORD                m_dwOverlayFlags = 0;
	DDCAPS               m_ddcaps;
public:

	void DrawRect(int left, int top, int width, int height) {
		WXAutoLock al(m_mutex);
		if (m_data.m_iBufSize == 0)return;
		if (m_iRectX != left || m_iRectY != top || m_iRectW != width || m_iRectH != height) {
			time = 0;
			m_iRectX = left;
			m_iRectY = top;
			m_iRectW = width;
			m_iRectH = height;
			memset(m_data.GetBuffer(), 0, m_iWidth * m_iHeight * 4);
			for (int h = 0; h < m_iHeight; h++) {
				for (int w = 0; w < m_iWidth; w++) {
					if (w == m_iRectX || w == m_iRectX + m_iRectW ||
						h == m_iRectY || h == m_iRectY + m_iRectH) {
						if (w <= m_iRectX + m_iRectW && w >= m_iRectX &&
							h >= m_iRectY && h <= m_iRectY + m_iRectH) {
							int pos = w * 4 + h * m_iWidth * 4;
							m_data.GetBuffer()[pos + 0] = 0x00;
							m_data.GetBuffer()[pos + 1] = 0x00;
							m_data.GetBuffer()[pos + 2] = 0xFF;
							m_data.GetBuffer()[pos + 3] = 0x00;
						}
					}
				}
			}
			SetEvent(hMutex);//设置为有信号
		}else {
			time++;
			if(time < 2)
				SetEvent(hMutex);//设置为有信号
		}
	}

	void Draw(uint8_t *buf) {
		WXAutoLock al(m_mutex);
		if (m_data.m_iBufSize == 0)return;
		memcpy(m_data.GetBuffer(), buf, m_iWidth * m_iHeight * 4);
		SetEvent(hMutex);//设置为有信号
	}

	void Reset() {
		WXAutoLock al(m_mutex);
		memset(m_data.GetBuffer(), 0, m_iWidth * m_iHeight * 4);
		SetEvent(hMutex);//设置为有信号
	}

	HudDraw() {
		WXAutoLock al(m_mutex);
		if (LibInst::GetInst().m_libDraw == nullptr)
			return;

		MonitorInfo*info = WXScreenGetDefaultInfo();
		m_iWidth  = info->width;
		m_iHeight = info->height;
		HRESULT hr = InitDirectDraw(m_iWidth, m_iHeight);
		if (FAILED(hr)) {
			return;
		}
		m_data.Init(NULL, m_iWidth*m_iHeight*4);
		hMutex = CreateEvent(NULL, FALSE, TRUE, NULL);//创建事件对象
		ResetEvent(hMutex);
		m_bStop = false;
		m_thread = new std::thread(&HudDraw::ThreadRun,this);
	}
	virtual ~HudDraw() {
		WXAutoLock al(m_mutex);
		if (m_thread) {
			m_bStop = true;
			SetEvent(hMutex);//设置为有信号,结束线程
			m_thread->join();
			delete m_thread;
			m_thread = NULL;
		}
		m_pDDSOverlay = NULL;
		m_pDDSPrimary = NULL;
		m_pDD = NULL;
	}
	void ThreadRun() {
		while(!m_bStop){
			WaitForSingleObject(hMutex, INFINITE);
			Display();
			ResetEvent(hMutex);
			SLEEPMS(10);
		}
	}
private:
	void Display() {

		HRESULT hr = S_OK;
		DDSURFACEDESC desc;
		ZeroMemory(&desc, sizeof(DDSURFACEDESC));
		desc.dwSize = sizeof(DDSURFACEDESC);

		do {
			hr = m_pDDSOverlay->Lock(nullptr, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr);
			if (DDERR_SURFACELOST == hr) {
				m_pDDSPrimary->Restore();
				m_pDDSOverlay->Restore();
				SLEEPMS(100);
			}
		} while (hr == DDERR_WASSTILLDRAWING || hr == DDERR_SURFACELOST);

		if (SUCCEEDED(hr)) {
			LPBYTE lpSurf = (LPBYTE)desc.lpSurface;
			for (int i = 0; i < m_iHeight; i++) {
				memcpy(lpSurf + i * desc.lPitch, m_data.GetBuffer() + i * m_iWidth * 4, m_iWidth * 4);
			}
			m_pDDSOverlay->Unlock(NULL);
			hr = m_pDDSOverlay->Flip(NULL, DDFLIP_WAIT);//加载到Overlay
			RECT m_rcSrc = { 0, 0, m_iWidth,m_iHeight };
			RECT m_rcDst = { 0, 0, m_iWidth,m_iHeight };
			hr = m_pDDSOverlay->UpdateOverlay(&m_rcSrc, m_pDDSPrimary, &m_rcDst, m_dwOverlayFlags, &m_OverlayFX);
		}
	}
	HRESULT InitDirectDraw(int width, int height) {
		WXAutoLock al(m_mutex);

		HRESULT	hr;
		if (FAILED(hr = LibInst::GetInst().mDirectDrawCreate(NULL, &m_pDD, NULL)))
			return hr;

		// Request normal cooperative level to put us in windowed mode
		if (FAILED(hr = m_pDD->SetCooperativeLevel(NULL, DDSCL_NORMAL)))
			return hr;

		// Get driver capabilities to determine Overlay support.
		ZeroMemory(&m_ddcaps, sizeof(m_ddcaps));
		m_ddcaps.dwSize = sizeof(m_ddcaps);
		if (FAILED(hr = m_pDD->GetCaps(&m_ddcaps, NULL)))return hr;

		DDSURFACEDESC ddsd_1;
		ZeroMemory(&ddsd_1, sizeof(ddsd_1));
		ddsd_1.dwSize = sizeof(DDSURFACEDESC);
		ddsd_1.dwFlags = DDSD_CAPS;
		ddsd_1.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if (FAILED(hr = m_pDD->CreateSurface(&ddsd_1, &m_pDDSPrimary, NULL)))
			return hr;

		//RGBA Mode,通用模式
		DDPIXELFORMAT  ddpfOverlayFormat;
		ZeroMemory(&ddpfOverlayFormat, sizeof(ddpfOverlayFormat));
		ddpfOverlayFormat.dwSize = sizeof(ddpfOverlayFormat);
		ddpfOverlayFormat.dwFlags = DDPF_RGB;
		ddpfOverlayFormat.dwRGBBitCount = 32;
		ddpfOverlayFormat.dwRBitMask = 0x00FF0000;
		ddpfOverlayFormat.dwGBitMask = 0x0000FF00;
		ddpfOverlayFormat.dwBBitMask = 0x000000FF;
		ddpfOverlayFormat.dwRGBAlphaBitMask = 0;// 0xFF000000;

		m_iWidth = width;
		m_iHeight = height;
		DDSURFACEDESC ddsd;
		ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));
		ddsd.dwSize = sizeof(DDSURFACEDESC);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_BACKBUFFERCOUNT | DDSD_PIXELFORMAT;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_VIDEOMEMORY;
		ddsd.dwBackBufferCount = 1;
		ddsd.dwWidth = m_iWidth;
		ddsd.dwHeight = m_iHeight;
		ddsd.ddpfPixelFormat = ddpfOverlayFormat;
		if (FAILED(hr = m_pDD->CreateSurface(&ddsd, &m_pDDSOverlay, NULL)))
			return hr;

		// Setup effects structure
		ZeroMemory(&m_OverlayFX, sizeof(m_OverlayFX));
		m_OverlayFX.dwSize = sizeof(m_OverlayFX);

		// Setup overlay flags.
		m_dwOverlayFlags = DDOVER_SHOW | DDOVER_DDFX;
		if (m_ddcaps.dwCKeyCaps & DDCKEYCAPS_DESTOVERLAY) {  //关键色,0,0,0
			m_dwOverlayFlags |= DDOVER_KEYSRCOVERRIDE;//设置源透明显示
		}

		DDCOLORKEY Color;
		Color.dwColorSpaceLowValue = RGB(0, 0, 0);
		Color.dwColorSpaceHighValue = RGB(0, 0, 0);
		hr = m_pDDSOverlay->SetColorKey(DDCKEY_SRCOVERLAY, &Color);
		return S_OK;
	}
};


WXMEDIA_API void *WXHudCreate() {
	HudDraw *hud = new HudDraw();
	return (void*)hud;
}

WXMEDIA_API void WXHudDraw(void * ptr, uint8_t *buf) {
	HudDraw *hud = (HudDraw*)ptr;
	if (hud)
		hud->Draw(buf);
}

WXMEDIA_API void WXHudReset(void * ptr) {
	HudDraw *hud = (HudDraw*)ptr;
	if (hud)hud->Reset();
}

WXMEDIA_API void WXHudDestroy(void *ptr) {
	HudDraw *hud = (HudDraw*)ptr;
	delete hud;
}


WXMEDIA_API void WXHudDrawRect(void *ptr, int left, int top, int width, int height) {
	HudDraw *hud = (HudDraw*)ptr;
	if (hud)hud->DrawRect(left,top,width,height);
}

#endif