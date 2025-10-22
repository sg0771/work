#include "WXMediaCpp.h"
#include "libyuv/libyuv.h"
//渲染 YUV420P 或者 RGB32 的  AVFrame 到HWND上面

class DXRender {
public:
	DXRender() {

		//::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM 初始化
	}
	virtual ~DXRender() {
		Close();
	}
	void GetXY(int srcWidth, int srcHeight, int dstWidth, int dstHeight, int& desX, int& desY) {
		desX = 0;
		desY = 0;
		int sw1 = dstHeight * srcWidth / srcHeight;
		int sh1 = dstWidth * srcHeight / srcWidth;
		if (sw1 <= dstWidth) {
			desX = (dstWidth - sw1) / 2;
		}
		else {
			desY = (dstHeight - sh1) / 2;
		}
	}
private:
	CComPtr<IDirect3D9Ex>		m_pD3D = nullptr;
	CComPtr<IDirect3DDevice9Ex>	m_pDev = nullptr;

	//渲染RGB32
	CComPtr<IDirect3DSurface9>  m_pSurfRGB = nullptr;
	CComPtr<IDirect3DSurface9>  m_pSurfYV12 = nullptr;

	bool m_bOpen = false;

	WXLocker m_mutex;
	HWND  m_hWnd = nullptr;
	int   m_iWidth = 0;
	int   m_iHeight = 0;
	int   m_dstW = 0;
	int   m_dstH = 0;

	RECT m_rcSrc;//在内存纹理上的实际区域

	D3DPRESENT_PARAMETERS m_d3dpp;

	//------------------------------d3dx9  shader-------------------------------------------
	bool  CreateD3D(int index) {
		HRESULT hr = E_FAIL;
		//显示器参数
		D3DDISPLAYMODE d3ddm;
		ZeroMemory(&d3ddm, sizeof(d3ddm));
		hr = m_pD3D->GetAdapterDisplayMode(index, &d3ddm);
		if (FAILED(hr)) {
			WXLogA("D3D  GetAdapterDisplayMode error");
			return false;
		}

		//屏幕大小
		m_iWidth = d3ddm.Width;
		m_iHeight = d3ddm.Height;
		D3DCAPS9 caps;
		hr = m_pD3D->GetDeviceCaps(index, D3DDEVTYPE_HAL, &caps);
		int vp = 0;
		if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
			vp = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
		else
			vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;


		ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
		m_d3dpp.Windowed = TRUE;
		m_d3dpp.hDeviceWindow = m_hWnd;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		m_d3dpp.BackBufferCount = 1;
		m_d3dpp.BackBufferWidth = m_iWidth;
		m_d3dpp.BackBufferHeight = m_iHeight;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

		m_pDev = nullptr;
		hr = m_pD3D->CreateDeviceEx(index, D3DDEVTYPE_HAL, m_hWnd, vp, &m_d3dpp, nullptr, &m_pDev);
		if (FAILED(hr)) {
			WXLogA("m_pD3D->CreateDeviceEx error");
			return false;
		}



		m_pSurfRGB = nullptr;
		hr |= m_pDev->CreateOffscreenPlainSurface(m_iWidth, m_iHeight,
			D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pSurfRGB, nullptr);
		if (FAILED(hr)) {
			WXLogA("m_pD3D->CreateOffscreenPlainSurface RGB32 error");
			return false;
		}


		// Create textures.
		m_pSurfYV12 = nullptr;
		hr = m_pDev->CreateOffscreenPlainSurface(m_iWidth, m_iHeight,
			(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), D3DPOOL_DEFAULT, &m_pSurfYV12, nullptr);
		if (FAILED(hr)) {
			WXLogA("m_pD3D->CreateOffscreenPlainSurface YV12 error");
			return false;
		}


		return SUCCEEDED(hr);
	}

public:
	//创建播放对象
	bool  Open(HWND hwnd) {
		WXAutoLock al(m_mutex);

		m_pD3D = nullptr;
		LibInst::GetInst().mDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
		if (!m_pD3D) {
			WXLogA("Direct3DCreate9Ex error");
			return false;
		}
		m_hWnd = hwnd;// ? hwnd : GetDesktopWindow();
		int nCount = m_pD3D->GetAdapterCount();
		for (int index = 0; index < nCount; index++) { //多显卡处理
			if (CreateD3D(index)) {//d3dx9 
				m_bOpen = true;
				break;
			}
		}
		if (!m_bOpen) {
			WXLogA("CreateD3D error");
		}
		return m_bOpen;
	}

	//关闭
	void  Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			m_pSurfRGB = nullptr;
			m_pSurfYV12 = nullptr;
			m_pD3D = nullptr;
			m_bOpen = false;
		}
	}

	HRESULT  UpdataDataRGB32(IDirect3DSurface9* pSurf, int width, int height, uint8_t* buf, int pitch) {
		D3DLOCKED_RECT d3d_rect;
		HRESULT hr = pSurf->LockRect(&d3d_rect, 0, 0);
		if (SUCCEEDED(hr)) {
			m_dstW = width;
			m_dstH = height;
			uint8_t* pDst = (uint8_t*)d3d_rect.pBits;
			int nPitch = d3d_rect.Pitch;
			if (width <= m_iWidth && height <= m_iHeight) {
				//ICopy
				libyuv::ARGBCopy(
					buf, pitch,
					pDst, nPitch,
					m_dstW, m_dstH
				);
			}
			else {
				//Scale
				int dw = (m_iHeight * width / height) / 2 * 2;//缩放到当前高度
				int dh = (m_iWidth * height / width) / 2 * 2;//缩放到当前宽度
				if (dw > m_iWidth) { //超出当前宽度
					m_dstW = m_iWidth;
					m_dstH = dh;
				}
				else {
					m_dstW = dw;
					m_dstH = m_iHeight;
				}

				libyuv::ARGBScale(
					buf, pitch,
					width, height,
					pDst, nPitch,
					m_dstW, m_dstH, libyuv::FilterMode::kFilterLinear
				);
			}
			hr = pSurf->UnlockRect();
		}
		return hr;
	}

	HRESULT  UpdataDataYV12(IDirect3DSurface9* pSurf, int width, int height,
		uint8_t* bufY, int pitchY,
		uint8_t* bufU, int pitchU,
		uint8_t* bufV, int pitchV) {
		D3DLOCKED_RECT d3d_rect;
		HRESULT hr = pSurf->LockRect(&d3d_rect, 0, 0);
		if (SUCCEEDED(hr)) {
			m_dstW = width;
			m_dstH = height;

			uint8_t* pDstY = (uint8_t*)d3d_rect.pBits;
			int nDstPitchY = d3d_rect.Pitch;
			uint8_t* pDstU = pDstY + nDstPitchY * m_iHeight;
			int nDstPitchU = nDstPitchY / 2;
			uint8_t* pDstV = pDstY + nDstPitchY * m_iHeight * 5 / 4;
			int nDstPitchV = nDstPitchY / 2;

			if (width <= m_iWidth && height <= m_iHeight) {
				//ICopy
				libyuv::I420Copy(
					bufY, pitchY,
					bufU, pitchU,
					bufV, pitchV,
					pDstY, nDstPitchY,
					pDstV, nDstPitchU,
					pDstU, nDstPitchV,
					m_dstW, m_dstH
				);
			}
			else {
				//Scale
				int dw = (m_iHeight * width / height) / 2 * 2;//缩放到当前高度
				int dh = (m_iWidth * height / width) / 2 * 2;//缩放到当前宽度
				if (dw > m_iWidth) { //超出当前宽度
					m_dstW = m_iWidth;
					m_dstH = dh;
				}
				else {
					m_dstW = dw;
					m_dstH = m_iHeight;
				}
				libyuv::I420Scale(
					bufY, pitchY,
					bufU, pitchU,
					bufV, pitchV,
					width, height,
					pDstY, nDstPitchY,
					pDstV, nDstPitchU,
					pDstU, nDstPitchV,
					m_dstW, m_dstH, libyuv::FilterMode::kFilterLinear
				);
			}
			hr = pSurf->UnlockRect();
			return hr;
		}
		return S_OK;
	}

	void  Render(IDirect3DSurface9* pSurf) {
		m_rcSrc.left = 0;
		m_rcSrc.right = m_dstW;
		m_rcSrc.top = 0;
		m_rcSrc.bottom = m_dstH;

		//需要计算一下居中尺寸

		m_pDev->BeginScene();
		CComPtr<IDirect3DSurface9> pBackBuffer = nullptr;
		HRESULT hr = m_pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
		hr |= m_pDev->Clear(0, nullptr, D3DCLEAR_TARGET, 0/*m_bkColor*/, 1.0f, 0);
		hr |= m_pDev->StretchRect(pSurf, &m_rcSrc, pBackBuffer, &m_rcSrc, D3DTEXF_LINEAR);
		m_pDev->EndScene();

		RECT rcDst;
		::GetClientRect(m_hWnd, &rcDst);

		int dx = 0;
		int dy = 0;
		GetXY(m_dstW, m_dstH, rcDst.right, rcDst.bottom, dx, dy);
		rcDst.left += dx;
		rcDst.right -= dx;
		rcDst.top += dy;
		rcDst.bottom -= dy;

		hr = m_pDev->Present(&m_rcSrc, &rcDst, nullptr, nullptr);//渲染到窗口

		if (hr == D3DERR_DEVICELOST) {
			hr = m_pDev->TestCooperativeLevel();
			if (SUCCEEDED(hr) || hr == D3DERR_DEVICENOTRESET) {
				//Reset
				m_pDev->Reset(&m_d3dpp);
			}
		}
	}

	//AVFrame 解码图像显示
	void  Display(AVFrame* avframe) {
		WXAutoLock al(m_mutex);
		if (m_bOpen && avframe) {
			IDirect3DSurface9* pSurf = nullptr;
			HRESULT hr = E_FAIL;
			if (avframe->format == AV_PIX_FMT_YUV420P) {
				pSurf = m_pSurfYV12;
				hr = UpdataDataYV12(pSurf, avframe->width, avframe->height,
					avframe->data[0], avframe->linesize[0],
					avframe->data[1], avframe->linesize[1],
					avframe->data[2], avframe->linesize[2]);
			}
			else 	if (avframe->format == AV_PIX_FMT_RGB32) {
				pSurf = m_pSurfRGB;
				hr = UpdataDataRGB32(pSurf, avframe->width, avframe->height,
					avframe->data[0], avframe->linesize[0]);
			}
			if (SUCCEEDED(hr)) {
				Render(pSurf);
			}
		}
	}


	//RGB32 YV12 紧凑数据显示
	void  Display(int bRGB32, int width, int height, uint8_t* buf, int pitch) {
		WXAutoLock al(m_mutex);
		HRESULT hr = E_FAIL;
		IDirect3DSurface9* pSurf = nullptr;
		if (bRGB32) {
			pSurf = m_pSurfRGB;
			hr = UpdataDataRGB32(pSurf, width, height, buf, pitch);
		}
		else {

			pSurf = m_pSurfYV12;
			int size = width * height;
			uint8_t* pY = buf;
			int nPitchY = pitch;
			uint8_t* pU = buf + size;
			int nPitchU = pitch / 2;
			uint8_t* pV = buf + size * 5 / 4;
			int nPitchV = pitch / 2;
			hr = UpdataDataYV12(pSurf, width, height,
				pY, nPitchY,
				pU, nPitchU,
				pV, nPitchV);
		}
		if (SUCCEEDED(hr)) {
			Render(pSurf);
		}
	}
};

WXMEDIA_API void* MLVRenderCreate(HWND hwnd) {
	DXRender* pObj = new DXRender;
	if (pObj->Open(hwnd)) {
		return pObj;
	}
	delete pObj;
	return nullptr;
}

WXMEDIA_API void MLVRenderDisplayFrame(void* p, struct AVFrame* frame) {
	DXRender* pObj = (DXRender*)p;
	if (pObj) {
		pObj->Display(frame);
	}
}


//渲染YV12/RGB32内存数据
WXMEDIA_API void MLVRenderDisplayData(void* p, int bRGB32, int width, int height, uint8_t* buf, int pitch) {
	DXRender* pObj = (DXRender*)p;
	if (pObj) {
		pObj->Display(bRGB32, width, height, buf, pitch);
	}
}

WXMEDIA_API void MLVRenderDestroy(void* p) {
	DXRender* pObj = (DXRender*)p;;
	if (pObj) {
		delete pObj;
	}
}