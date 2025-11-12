/*
使用 D3DX  D3D  GDI  显示 AVFrame 或者 RGB32 数据
*/

#ifdef _WIN32

#include "WXMediaCpp.h"

//通过D3DX处理后将Surface回调给WPF
#include <d3dx9/d3dx9.h>

class D3dxRender {
	typedef struct _VERTEX {
		FLOAT x, y, z, rhw, u, v;
	} VERTEX;

	CComPtr<IDirect3DPixelShader9>	m_pShader;
	CComPtr<IDirect3D9Ex>		m_pD3D = nullptr;
	CComPtr<IDirect3DDevice9Ex>	m_pDev = nullptr;
	CComPtr<IDirect3DTexture9>  m_pTexture = nullptr;
	CComPtr<IDirect3DSurface9>  m_pSurface = nullptr;

	enum {
		INDEX_Y = 0,
		INDEX_U = 1,
		INDEX_V = 2,
		INDEX_NUM = 3
	};
	CComPtr<IDirect3DTexture9>	m_pTex[INDEX_NUM] = { nullptr,nullptr,nullptr };
	CComPtr<ID3DXConstantTable> m_pCTable = 0;//于shader的数据交换
	D3DXHANDLE m_pHBrightness = 0; //亮度
	D3DXHANDLE m_pHSaturation = 0; //饱和度
	D3DXHANDLE m_pHContrast = 0;  //对比度
	float  m_fBrightness = 1.0f;//亮度
	float  m_fSaturation = 1.0f;//饱和度
	float  m_fContrast = 1.0f;//对比度

	std::atomic<uint64_t> m_playID = 0;
	std::atomic_bool m_bOpen = false;

	WXLocker m_mutex;
	WXVideoFrame m_frame;
	HWND  m_hWnd = nullptr;

	D3DPRESENT_PARAMETERS m_d3dpp;

	HANDLE m_hSharedHandle = nullptr;
	void CreateSurface() {
		m_pTexture = nullptr;
		m_hSharedHandle = nullptr;
		m_pDev->CreateTexture(
			m_dstWidth, m_dstHeight, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pTexture, &m_hSharedHandle);
		if (m_pTexture) {
			m_pSurface = nullptr;
			m_pTexture->GetSurfaceLevel(0, &m_pSurface);
		}
	}
	//------------------------------d3dx9  shader-------------------------------------------
	bool  CreateD3D(int index) {
		HRESULT hr = E_FAIL;
		//显示器参数
		D3DDISPLAYMODE d3ddm;
		ZeroMemory(&d3ddm, sizeof(d3ddm));
		hr = m_pD3D->GetAdapterDisplayMode(index, &d3ddm);
		if (FAILED(hr)) {
			WXLogW(L"GetAdapterDisplayMode error");
			return false;
		}
		//if (d3ddm.Format != D3DFMT_X8R8G8B8) {
		//	WXLogW(L"GetAdapterDisplayMode Format error");
		//	return false;
		//}

		//窗口大小
		m_dstWidth = m_txtWidth = m_iWidth = d3ddm.Width;
		m_dstHeight = m_txtHeight = m_iHeight = d3ddm.Height;
		D3DCAPS9					caps;
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
			WXLogW(L"m_pD3D->CreateDeviceEx error");
			return false;
		}

		m_pShader = nullptr;
		if (!CompilePShader()) {
			return false;
		}

		CreateSurface();

		// Create textures.
		m_pDev->CreateTexture(m_iWidth, m_iHeight, 1,
			D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_Y], nullptr);
		m_pDev->CreateTexture(m_iWidth / 2, m_iHeight / 2, 1,
			D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_U], nullptr);
		m_pDev->CreateTexture(m_iWidth / 2, m_iHeight / 2, 1,
			D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_V], nullptr);

		m_frame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
		return true;
	}

	bool  CompilePShader()
	{
		const char* strShader_yuv2rgb =
			"sampler s0 : register(s0);\n\
		sampler s1 : register(s1);\n\
		sampler s2 : register(s2);\n\
		float Brightness = 1.0f; \n\
		float Saturation = 1.0f; \n\
		float Contrast = 1.0f; \n\
		float  f1 = 1.164f; \n\
		float  f2 = 0.0627f;\n\
		float  r1 = 1.596f;\n\
		float  g1 = 0.392f;\n\
		float  g2 = 0.813f;\n\
		float  b1 = 2.017f;\n\
		float4 yuv2rgb(float y, float u, float v) {\n\
			float y1 = f1 * (y - f2);\n\
			float u1 = u - 0.502f; \n\
			float v1 = v - 0.502f; \n\
			float r  = (y1 + r1 * v1);\n\
			float g  = (y1 - g1 * u1 - g2 * v1);\n\
			float b  = (y1 + b1 * u1);\n\
			return float4(Brightness * r,Brightness * g, Brightness * b, 1);\n\
		}\n\
		float4 LerpFunc(float4 s0, float4 s1, float weight) {\n\
			float r = s0.r * (1.0f - weight) + s1.r * weight;\n\
			float g = s0.g * (1.0f - weight) + s1.g * weight;\n\
			float b = s0.b * (1.0f - weight) + s1.b * weight;\n\
			return float4(r,g,b, 1);\n\
		}\n\
		float4 frag(float4 s0, float gray) {\n\
			float4 grayColor = float4(gray, gray, gray,1);\n\
			float4 resSaturation = LerpFunc(grayColor, s0, Saturation); \n\
			float4 avgColor = float4(0.5f, 0.5f, 0.5f,1); \n\
			return LerpFunc(avgColor, resSaturation, Contrast); \n\
		}\n\
		float4 Main(float2 tex :TEXCOORD0) :COLOR0{\n\
			float  y = tex2D(s0, tex).r;\n\
			float  u = tex2D(s1, tex).r;\n\
			float  v = tex2D(s2, tex).r;\n\
			float4 res  = yuv2rgb(y,u,v);\n\
			return frag(res, y); \n\
		}";

		m_pShader = nullptr;
		HRESULT hr = S_OK;
		CComPtr<ID3DXBuffer>pShader = nullptr;
		CComPtr<ID3DXBuffer>pErrorBuffer = nullptr;
		CComPtr<ID3DXConstantTable> pCtTable = nullptr;
		m_pCTable = nullptr;
		hr = LibInst::GetInst().mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_3_0",
			D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
			&pShader, &pErrorBuffer, &m_pCTable);

		if (FAILED(hr)) {
			pShader = nullptr;
			pErrorBuffer = nullptr;
			m_pCTable = nullptr;
			hr = LibInst::GetInst().mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_2_0",
				D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
				&pShader, &pErrorBuffer, &m_pCTable);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &m_pShader);
		}
		if (SUCCEEDED(hr)) {
			m_pHBrightness = m_pCTable->GetConstantByName(0, "Brightness");//亮度
			m_pHSaturation = m_pCTable->GetConstantByName(0, "Saturation");//饱和度
			m_pHContrast = m_pCTable->GetConstantByName(0, "Contrast");//对比度
			m_pCTable->SetDefaults(m_pDev);
		}
		else {
			return false;
		}
		return true;
	}

	int   m_iWidth = 0;
	int   m_iHeight = 0;

	int   m_dstWidth = 0;
	int   m_dstHeight = 0;

	int   m_txtWidth = 0;
	int   m_txtHeight = 0;

	std::atomic_bool  m_bNewImage = false;
public:
	//设置亮度、饱和度、对比度
	void  SetLut(int nBrightness, int nSaturation, int nContrast) {
		WXAutoLock al(m_mutex);
		m_fBrightness = nBrightness / 100.0f;
		m_fSaturation = nSaturation / 100.0f;
		m_fContrast = nContrast / 100.0f;
	}

#define CHECK_HR(hr)  if (FAILED(hr)) {m_pDev->EndScene();return nullptr;}

	//获取D3DSurface
	void* GetSurface(int* width, int* height) { //渲染到D3DSurface
		WXAutoLock al(m_mutex);
		if (m_dstWidth == 0 || m_dstHeight == 0) {
			return nullptr;
		}

		if (width) {
			*width = 0;
		}
		if (height) {
			*height = 0;
		}
		if (m_bOpen.load()) {
			if (m_dstWidth != m_txtWidth || m_dstHeight != m_txtHeight) {
				CreateSurface();
				m_txtWidth = m_dstWidth;
				m_txtHeight = m_dstHeight;
				WXLogW(L"VideoSurfaceBack Change Size To %dx%d", m_txtWidth, m_txtHeight);
			}

			if (width) {
				*width = m_txtWidth;
			}
			if (height) {
				*height = m_txtHeight;
			}

			HRESULT hr = S_OK;
			hr = m_pDev->BeginScene();
			CHECK_HR(hr)
				CComPtr<IDirect3DSurface9> pBackBuffer = nullptr;
			hr = m_pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
			CHECK_HR(hr)

				hr = m_pDev->SetRenderTarget(0, pBackBuffer);
			CHECK_HR(hr)

				hr = m_pDev->Clear(0, nullptr, D3DCLEAR_TARGET, 0/*m_bkColor*/, 1.0f, 0);
			CHECK_HR(hr)

				if (m_bNewImage.load()) { //新画面内容加载到YUV Texture
					m_bNewImage.store(false);
					D3DLOCKED_RECT d3d_rect[INDEX_NUM];
					hr = m_pTex[INDEX_Y]->LockRect(0, &d3d_rect[INDEX_Y], 0, 0);
					CHECK_HR(hr)
						hr = m_pTex[INDEX_U]->LockRect(0, &d3d_rect[INDEX_U], 0, 0);
					CHECK_HR(hr)
						hr = m_pTex[INDEX_V]->LockRect(0, &d3d_rect[INDEX_V], 0, 0);
					CHECK_HR(hr)
						libyuv::I420Copy(
							m_frame.GetFrame()->data[0], m_frame.GetFrame()->linesize[0],
							m_frame.GetFrame()->data[1], m_frame.GetFrame()->linesize[1],
							m_frame.GetFrame()->data[2], m_frame.GetFrame()->linesize[2],
							(uint8_t*)d3d_rect[INDEX_Y].pBits, d3d_rect[INDEX_Y].Pitch,
							(uint8_t*)d3d_rect[INDEX_U].pBits, d3d_rect[INDEX_U].Pitch,
							(uint8_t*)d3d_rect[INDEX_V].pBits, d3d_rect[INDEX_V].Pitch,
							m_txtWidth, m_txtHeight
						);

					hr = m_pTex[INDEX_Y]->UnlockRect(0);
					CHECK_HR(hr)
						hr = m_pTex[INDEX_U]->UnlockRect(0);
					CHECK_HR(hr)
						hr = m_pTex[INDEX_V]->UnlockRect(0);
					CHECK_HR(hr)
				}

			hr = m_pCTable->SetFloat(m_pDev, m_pHBrightness, m_fBrightness);
			CHECK_HR(hr)
				hr = m_pCTable->SetFloat(m_pDev, m_pHSaturation, m_fSaturation);
			CHECK_HR(hr)
				hr = m_pCTable->SetFloat(m_pDev, m_pHContrast, m_fContrast);
			CHECK_HR(hr)
				hr = m_pDev->SetPixelShader(m_pShader);
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_Y, m_pTex[INDEX_Y]);
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_U, m_pTex[INDEX_U]);
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_V, m_pTex[INDEX_V]);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_LIGHTING, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_ZENABLE, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
			CHECK_HR(hr)
				hr = m_pDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)

				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
			CHECK_HR(hr)
				hr = m_pDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX3);
			CHECK_HR(hr)
				float fw = (float)m_dstWidth / (float)m_iWidth;
			float fh = (float)m_dstHeight / (float)m_iHeight;
			float dw = (float)m_dstWidth - 0.5f;
			float dh = (float)m_dstHeight - 0.5f;
			VERTEX pVex[] = {
				{  -0.5f, -0.5f, 0.5f, 1.0f, 0,  0 },
				{  dw,        0, 0.5f, 1.0f, fw, 0 },
				{  dw,       dh, 0.5f, 1.0f, fw, fh },
				{  -0.5f,    dh, 0.5f, 1.0f, 0,  fh },
			};
			hr = m_pDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, pVex, sizeof(pVex[0]));//D3DX Shader
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_Y, nullptr);
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_U, nullptr);
			CHECK_HR(hr)
				hr = m_pDev->SetTexture(INDEX_V, nullptr);
			CHECK_HR(hr)
				hr = m_pDev->SetPixelShader(nullptr);
			CHECK_HR(hr)
				RECT rect{
					0, 0, m_txtWidth, m_txtHeight
			};
			hr = m_pDev->StretchRect(pBackBuffer, &rect, m_pSurface, &rect, D3DTEXF_LINEAR);//将D3DX Shader处理结果保存到输出Surface
			CHECK_HR(hr)
				hr = m_pDev->EndScene();
			CHECK_HR(hr)
				return m_pSurface;
		}
		return nullptr;
	}

	void SetID(uint64_t uid) {
		WXAutoLock al(m_mutex);
		m_playID.store(uid);
	}

	//填充YUV420数据
	//比如快速切换视频时，原视频线程没有结束，但是ID已经改变，这个时候就无法进行绘制了
	int Draw(uint64_t uid, AVFrame* frame) { //内存拷贝
		WXAutoLock al(m_mutex);
		if (m_playID.load() != uid)
			return 0; //ID 被改变了

		if (m_bOpen.load()) {

			//某些图像可能宽度或者高度超出BufferSize
			//需要缩放处理
			if (frame->width <= m_iWidth && frame->height <= m_iHeight) {
				//直接copy
				libyuv::I420Copy(
					frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					m_frame.GetFrame()->data[0], m_frame.GetFrame()->linesize[0],
					m_frame.GetFrame()->data[1], m_frame.GetFrame()->linesize[1],
					m_frame.GetFrame()->data[2], m_frame.GetFrame()->linesize[2],
					frame->width, frame->height
				);
				m_dstWidth = frame->width;
				m_dstHeight = frame->height;
			}
			else {
				//Scale
				int dw = (m_iHeight * frame->width / frame->height) / 2 * 2;//缩放到当前高度
				int dh = (m_iWidth * frame->height / frame->width) / 2 * 2;//缩放到当前宽度
				if (dw > m_iWidth) { //超出当前宽度
					m_dstWidth = m_iWidth;
					m_dstHeight = dh;
				}
				else {
					m_dstWidth = dw;
					m_dstHeight = m_iHeight;
				}

				libyuv::I420Scale(
					frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					frame->width, frame->height,
					m_frame.GetFrame()->data[0], m_frame.GetFrame()->linesize[0],
					m_frame.GetFrame()->data[1], m_frame.GetFrame()->linesize[1],
					m_frame.GetFrame()->data[2], m_frame.GetFrame()->linesize[2],
					m_dstWidth, m_dstHeight,
					libyuv::FilterMode::kFilterBilinear
				);
			}
			m_bNewImage.store(true);
			return 1;
		}

		return 0;
	}

	//创建播放对象
	bool  Open(HWND hwnd) {
		WXAutoLock al(m_mutex);
		if (LibInst::GetInst().m_libD3DX9 == nullptr) {
			return FALSE;
		}
		m_pD3D = nullptr;
		LibInst::GetInst().mDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
		if (!m_pD3D) {
			WXLogW(L"Direct3DCreate9Ex error");
			return FALSE;
		}
		m_hWnd = hwnd;// ? hwnd : GetDesktopWindow();
		int nCount = m_pD3D->GetAdapterCount();
		for (int index = 0; index < nCount; index++) { //多显卡处理
			if (CreateD3D(index)) {//d3dx9 
				if (index != 0) //多显卡特殊设备
					WXLogA("%s index=%d", __FUNCTION__, index);
				m_bOpen.store(true);
				break;
			}
		}
		return m_bOpen.load();
	}

	//关闭
	void  Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen.load()) {
			m_pD3D = nullptr;
			m_pDev = nullptr;
			m_pTexture = nullptr;
			m_pSurface = nullptr;
			m_pTex[INDEX_Y] = nullptr;
			m_pTex[INDEX_U] = nullptr;
			m_pTex[INDEX_V] = nullptr;
			m_pShader = nullptr;
			m_bOpen.store(false);
		}
	}

	//渲染到句柄，可以用于MFC测试
	void  Render() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			RECT rect{ 0, 0, m_txtWidth, m_txtHeight };
			RECT rcDst;
			::GetClientRect(m_hWnd, &rcDst);
			HRESULT hr = m_pDev->Present(&rect, &rcDst, nullptr, nullptr);//渲染到桌面

			if (hr == D3DERR_DEVICELOST) {
				hr = m_pDev->TestCooperativeLevel();
				if (SUCCEEDED(hr) || hr == D3DERR_DEVICENOTRESET) {
					//Reset
					m_pDev->Reset(&m_d3dpp);
				}
			}
		}
	}

};

//在WXDXFilterGetSurface后在初始化的HWND上渲染出来
WXMEDIA_API void WXDXFilterRender(void* obj) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		p->Render();
	}
}

WXMEDIA_API void* WXDXFilterCreate(HWND hwnd) {
	D3dxRender* obj = new D3dxRender();
	if (obj->Open(hwnd)) {
		return obj;
	}
	return nullptr;
}

WXMEDIA_API void WXDXFilterDestroy(void* obj) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		p->Close();
		delete p;
	}
}

WXMEDIA_API void WXDXFilterSetLut(void* obj, int nBrightness, int nSaturation, int nContrast) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		return p->SetLut(nBrightness, nSaturation, nContrast);
	}
}

WXMEDIA_API void WXDXFilterSetID(void* obj, uint64_t uid) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		return p->SetID(uid);
	}
}

WXMEDIA_API int WXDXFilterDraw(void* obj, uint64_t uid, AVFrame* frame) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		return p->Draw(uid, frame);
	}
	return 0;
}

WXMEDIA_API void* WXDXFilterGetSurface(void* obj, int* width, int* height) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		return p->GetSurface(width, height);
	}
	return 0;
}

WXMEDIA_API void* WXDXFilterGetSurfaceEx(void* obj) {
	if (obj) {
		D3dxRender* p = (D3dxRender*)obj;
		return p->GetSurface(NULL, NULL);
	}
	return 0;
}

//WXDXFilterGetSurface 后调用Surface绘制之后应该执行WXDXFilterReleaseSurface
//连续两次GetSurface，第二次返回nullptr
WXMEDIA_API void WXDXFilterReleaseSurface(void* obj) {
	//if (obj) {
	//	D3dxRender* p = (D3dxRender*)obj;
	//	p->Release();
	//}
}

static WXLocker s_lockRender;//渲染设备锁
//对比度、亮度、饱和度按调节
static int s_nLut = 0;
static int s_nGray = 0;
static int s_nChrome = 0;
WXMEDIA_API void WXVideoRenderSetLut(int type, int value) {
	int v2 = value;
	if (v2 > 50)
		v2 = 50;
	if (v2 < -50)
		v2 = -50;

	WXAutoLock al(s_lockRender);
	WXLogA("%s type=%d value=%d", __FUNCTION__, type, v2);
	if (type == TYPE_LUT) {
		s_nLut = v2;
	}
	else if (type == TYPE_GRAY) {
		s_nGray = v2;
	}
	else  if (type == TYPE_CHROME) {
		s_nChrome = v2;
	}
}

//正常
static int  WXGetRotate90(int nRotateFilp) {
	switch (nRotateFilp)
	{
	case RENDER_ROTATE_NONE:
		return RENDER_ROTATE_90;
	case RENDER_ROTATE_90:
		return RENDER_ROTATE_180;
	case RENDER_ROTATE_180:
		return RENDER_ROTATE_270;
	case RENDER_ROTATE_270:
		return RENDER_ROTATE_NONE;

	case RENDER_ROTATE_NONE_FLIPX:
		return RENDER_ROTATE_270_FLIPX;
	case RENDER_ROTATE_90_FLIPX:
		return RENDER_ROTATE_NONE_FLIPX;
	case RENDER_ROTATE_180_FLIPX:
		return RENDER_ROTATE_90_FLIPX;
	case RENDER_ROTATE_270_FLIPX:
		return RENDER_ROTATE_180_FLIPX;
	default:
		return 0;
	}
}

//正常
static int  WXGetRotate180(int nRotateFilp) {
	switch (nRotateFilp)
	{
	case RENDER_ROTATE_NONE:
		return RENDER_ROTATE_180;
	case RENDER_ROTATE_90:
		return RENDER_ROTATE_270;
	case RENDER_ROTATE_180:
		return RENDER_ROTATE_NONE;
	case RENDER_ROTATE_270:
		return RENDER_ROTATE_90;

	case RENDER_ROTATE_NONE_FLIPX:
		return RENDER_ROTATE_180_FLIPX;
	case RENDER_ROTATE_90_FLIPX:
		return RENDER_ROTATE_270_FLIPX;
	case RENDER_ROTATE_180_FLIPX:
		return RENDER_ROTATE_NONE_FLIPX;
	case RENDER_ROTATE_270_FLIPX:
		return RENDER_ROTATE_90_FLIPX;
	default:
		return 0;
	}
}

//正常
static int  WXGetRotate270(int nRotateFilp) {
	switch (nRotateFilp)
	{
	case RENDER_ROTATE_NONE:
		return RENDER_ROTATE_270;
	case RENDER_ROTATE_90:
		return RENDER_ROTATE_NONE;
	case RENDER_ROTATE_180:
		return RENDER_ROTATE_90;
	case RENDER_ROTATE_270:
		return RENDER_ROTATE_180;

	case RENDER_ROTATE_NONE_FLIPX:
		return RENDER_ROTATE_90_FLIPX;
	case RENDER_ROTATE_90_FLIPX:
		return RENDER_ROTATE_180_FLIPX;
	case RENDER_ROTATE_180_FLIPX:
		return RENDER_ROTATE_270_FLIPX;
	case RENDER_ROTATE_270_FLIPX:
		return RENDER_ROTATE_NONE_FLIPX;
	default:
		return 0;
	}
}

//正常
static int  WXGetFlipX(int nRotateFilp) {
	switch (nRotateFilp)
	{
	case RENDER_ROTATE_NONE:
		return RENDER_ROTATE_NONE_FLIPX;
	case RENDER_ROTATE_90:
		return RENDER_ROTATE_90_FLIPX;
	case RENDER_ROTATE_180:
		return RENDER_ROTATE_180_FLIPX;
	case RENDER_ROTATE_270:
		return RENDER_ROTATE_270_FLIPX;

	case RENDER_ROTATE_NONE_FLIPX:
		return RENDER_ROTATE_NONE;
	case RENDER_ROTATE_90_FLIPX:
		return RENDER_ROTATE_90;
	case RENDER_ROTATE_180_FLIPX:
		return RENDER_ROTATE_180;
	case RENDER_ROTATE_270_FLIPX:
		return RENDER_ROTATE_270;
	default:
		return 0;
	}
}

//正常
static int  WXGetFlipY(int nRotateFilp) {
	switch (nRotateFilp)
	{
	case RENDER_ROTATE_NONE:
		return RENDER_ROTATE_NONE_FLIPY;
	case RENDER_ROTATE_90:
		return RENDER_ROTATE_90_FLIPY;
	case RENDER_ROTATE_180:
		return RENDER_ROTATE_180_FLIPY;
	case RENDER_ROTATE_270:
		return RENDER_ROTATE_270_FLIPY;

	case RENDER_ROTATE_NONE_FLIPX:
		return RENDER_ROTATE_NONE_FLIPXY;
	case RENDER_ROTATE_90_FLIPX:
		return RENDER_ROTATE_90_FLIPXY;
	case RENDER_ROTATE_180_FLIPX:
		return RENDER_ROTATE_180_FLIPXY;
	case RENDER_ROTATE_270_FLIPX:
		return RENDER_ROTATE_270_FLIPXY;
	default:
		return 0;
	}
}

WXMEDIA_API int  WXGetRotateFilp(int nRotateFilp, int type) {
	if (type == ROTATEFLIP_TYPE_RESET)
		return RENDER_ROTATE_NONE;
	switch (type)
	{
	case ROTATEFLIP_TYPE_ROTATE90:
		return WXGetRotate90(nRotateFilp);
	case ROTATEFLIP_TYPE_ROTATE180:
		return WXGetRotate180(nRotateFilp);
	case ROTATEFLIP_TYPE_ROTATE270:
		return WXGetRotate270(nRotateFilp);
	case ROTATEFLIP_TYPE_FLIPX:
		return WXGetFlipX(nRotateFilp);
	case ROTATEFLIP_TYPE_FLIPY:
		return WXGetFlipY(nRotateFilp);
	default:
		WXLogW(L"WXGetRotateFilp type=%d Error", type);
		return nRotateFilp;
	}
}



class WindowsRender {
	D3DPRESENT_PARAMETERS m_d3dpp;

	const float vf1 = 1.0f;
	const float vf2 = 0.0f;
	const float vr1 = 1.5748f;
	const float vg1 = 0.18732f;
	const float vg2 = 0.46812f;
	const float vb1 = 1.8556f;

	const char* strShader_yuv2rgb =
		"sampler s0 : register(s0);\n\
		 sampler s1 : register(s1);\n\
		 sampler s2 : register(s2);\n\
		 float Brightness = 1.0f; \n\
		 float Saturation = 1.0f; \n\
		 float Contrast = 1.0f; \n\
		 float  f1 = 1.402f; \n\
		 float  f2 = 0.0f;\n\
		 float  r1 = 1.596f;\n\
		 float  g1 = 0.34414f;\n\
		 float  g2 = 0.71414f;\n\
		 float  b1 = 1.772f;\n\
		float4 yuv2rgb(float y, float u, float v) {\n\
			float y1 = f1 * (y - f2);\n\
			float u1 = u - 0.50f; \n\
			float v1 = v - 0.50f; \n\
			float r  = (y1 + r1 * v1);\n\
			float g  = (y1 - g1 * u1 - g2 * v1);\n\
			float b  = (y1 + b1 * u1);\n\
			return float4(Brightness * r,Brightness * g, Brightness * b, 1);\n\
		}\n\
		float4 LerpFunc(float4 s0, float4 s1, float weight) {\n\
			float r = s0.r * (1.0f - weight) + s1.r * weight;\n\
			float g = s0.g * (1.0f - weight) + s1.g * weight;\n\
			float b = s0.b * (1.0f - weight) + s1.b * weight;\n\
			return float4(r,g,b, 1);\n\
		}\n\
		float4 frag(float4 s0, float gray) {\n\
			float4 grayColor = float4(gray, gray, gray,1);\n\
			float4 resSaturation = LerpFunc(grayColor, s0, Saturation); \n\
			float4 avgColor = float4(0.5f, 0.5f, 0.5f,1); \n\
			return LerpFunc(avgColor, resSaturation, Contrast); \n\
		}\n\
		float4 Main(float2 tex :TEXCOORD0) :COLOR0{\n\
			float  y = tex2D(s0, tex).r;\n\
			float  u = tex2D(s1, tex).r;\n\
			float  v = tex2D(s2, tex).r;\n\
			float4 res  = yuv2rgb(y,u,v);\n\
			return frag(res, y); \n\
		}";

	//D3DX9 shader
	const char* strResizeX =
				"#define A -1.0f   \n\
		sampler s0 : register(s0);  \n\
		float2 dxdy : register(c0);    \n\
		static float4x4 tco = {       \n\
			0,  A,  -2 * A,    A,     \n\
			1,  0,  -A - 3,  A + 2,   \n\
			0, -A, 2 * A + 3, -A - 2, \n\
			0,  0,     A,   -A        \n\
		}; \n\
		float4 Main(float2 tex : TEXCOORD0) : COLOR\n\
		{\n\
			float t = frac(tex.x); \n\
			float2 pos = tex - float2(t, 0.); \n\
			float4 Q0 = tex2D(s0, (pos + float2(-.5, .5))*dxdy); \n\
			float4 Q1 = tex2D(s0, (pos + .5)*dxdy); \n\
			float4 Q2 = tex2D(s0, (pos + float2(1.5, .5))*dxdy); \n\
			float4 Q3 = tex2D(s0, (pos + float2(2.5, .5))*dxdy); \n\
			return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3)); \n\
		}";

	const char* strResizeY =
				"#define A  -1.0 \n\
		sampler s0 : register(s0); \n\
		float2 dxdy : register(c0); \n\
		static float4x4 tco = { \n\
			0,  A,  -2 * A,    A,\n\
			1,  0,  -A - 3,  A + 2,\n\
			0, -A, 2 * A + 3, -A - 2,\n\
			0,  0,     A,   -A\n\
		}; \n\
		float4 Main(float2 tex : TEXCOORD0) : COLOR\n\
		{ \n\
			float t = frac(tex.y); \n\
		float2 pos = tex - float2(0., t); \n\
		float4 Q0 = tex2D(s0, (pos + float2(.5, -.5))*dxdy); \n\
		float4 Q1 = tex2D(s0, (pos + .5)*dxdy); \n\
		float4 Q2 = tex2D(s0, (pos + float2(.5, 1.5))*dxdy); \n\
		float4 Q3 = tex2D(s0, (pos + float2(.5, 2.5))*dxdy); \n\
		return mul(mul(tco, float4(1., t, t*t, t*t*t)), float4x4(Q0, Q1, Q2, Q3)); \n\
		}";



	WXLocker m_mutex;
	int m_bOpen = FALSE;
	HWND m_hWnd = nullptr;
	int m_iWidth = 0;
	int m_iHeight = 0;
	WXVideoFrame m_I420Frame;
	WXVideoFrame m_I420MirrorFrame;
	WXVideoFrame m_RGB32Frame;
	WXVideoFrame m_RGB32MirrorFrame;
	WXVideoConvert m_conv;

	typedef struct _CUSTOMVERTEX {
		FLOAT       x, y, z, rhw, u, v;
	} CUSTOMVERTEX;

	enum {
		shader_resizex,
		shader_resizey,
		shader_yv12torgb32,//I420
		shader_count
	};
	enum {
		INDEX_Y,
		INDEX_U,
		INDEX_V,
		INDEX_NUM
	};

	CComPtr<IDirect3DSurface9> m_pBackBuffer = nullptr;

	//D3D9
	CComPtr<IDirect3D9>  m_pD3d9 = nullptr;
	CComPtr<IDirect3DDevice9> m_pD3dDevice = nullptr;
	BOOL m_bSupportYUV = FALSE;//D3D YV12 Support

	//D3DX9
	D3DCAPS9 m_Caps;
	CComPtr<IDirect3DPixelShader9>m_pShaders[shader_count] = { nullptr };
	CComPtr<IDirect3D9Ex>		m_pD3DEx = nullptr;
	CComPtr<IDirect3DDevice9Ex>	m_pD3DDevEx = nullptr;
	CComPtr<IDirect3DTexture9>  m_pTexture = nullptr;//Render Target
	CComPtr<IDirect3DSurface9>  m_pSurface = nullptr;

	CComPtr<IDirect3DTexture9>  m_pTexRGBA[2] = { nullptr,nullptr };//RGBTarget
	CComPtr<IDirect3DSurface9>  m_pSurfRGBA[2] = { nullptr,nullptr };//纯RGB绘制
	CComPtr<IDirect3DTexture9>  m_pTexYUV[2] = { nullptr,nullptr };//RGBTarget
	CComPtr<IDirect3DSurface9>  m_pSurfYUV[2] = { nullptr,nullptr };//纯RGB绘制

	CComPtr<IDirect3DTexture9>	m_pResizeTexture = nullptr; //Resize Texture
	CComPtr<IDirect3DTexture9>	m_pTex[INDEX_NUM] = { nullptr,nullptr,nullptr };//YUV内存表面

	CComPtr<ID3DXConstantTable> m_pCTable = 0;//于shader的数据交换
	D3DXHANDLE m_hBrightness = 0; //亮度
	D3DXHANDLE m_hSaturation = 0; //饱和度
	D3DXHANDLE m_hContrast = 0;  //对比度

	//转换矩阵系数
	D3DXHANDLE m_hF1 = 0;
	D3DXHANDLE m_hF2 = 0;
	D3DXHANDLE m_hR1 = 0;
	D3DXHANDLE m_hG1 = 0;
	D3DXHANDLE m_hG2 = 0;
	D3DXHANDLE m_hB1 = 0;

	float  m_fBrightness = 1.0f;//亮度
	float  m_fSaturation = 1.0f;//饱和度
	float  m_fContrast = 1.0f;//对比度

	int m_bFixed = FALSE;
	int m_nRotateFlip = RENDER_ROTATE_NONE;
	RECT m_rcSrc{ 0,0,0,0 };
	int m_iMaxSize = 0;

private:
	//------------------------------d3dx9  shader-------------------------------------------
	bool __D3DX_CreateD3DX(int index) {

		HRESULT hr = E_FAIL;
		D3DDISPLAYMODE d3ddm;
		ZeroMemory(&d3ddm, sizeof(d3ddm));

		//显示器参数
		hr = m_pD3DEx->GetAdapterDisplayMode(index, &d3ddm);
		if (FAILED(hr)) {
			WXLogW(L"AdapterModex error");
			return false;
		}

		int bufWidth = MAX(d3ddm.Width, m_iWidth);
		int bufHeight = MAX(d3ddm.Height, m_iHeight);
		m_iMaxSize = MAX(bufWidth, bufHeight);

		hr = m_pD3DEx->GetDeviceCaps(index, D3DDEVTYPE_HAL, &m_Caps);
		if (FAILED(hr)) {
			if ((m_Caps.Caps & D3DCAPS_READ_SCANLINE) == 0) {
				WXLogW(L"DeviceCaps error");
				return false;
			}
		}
		int vp = 0;
		if (m_Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
			vp = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
		else
			vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;

		ZeroMemory(&m_d3dpp, sizeof(D3DPRESENT_PARAMETERS));
		m_d3dpp.Windowed = TRUE;
		m_d3dpp.hDeviceWindow = m_hWnd;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		m_d3dpp.BackBufferCount = 1;
		m_d3dpp.BackBufferWidth = m_iMaxSize;
		m_d3dpp.BackBufferHeight = m_iMaxSize;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT; // D3DPRESENT_INTERVAL_IMMEDIATE;

		m_pD3DDevEx = nullptr;
		hr = m_pD3DEx->CreateDeviceEx(
			index, D3DDEVTYPE_HAL, m_hWnd, vp,
			&m_d3dpp, nullptr, &m_pD3DDevEx);
		if (FAILED(hr)) {
			WXLogW(L"m_pD3DEx->CreateDeviceEx error");
			return false;
		}

		if (!m_pShaders[shader_resizex]) {
			if (!__D3DX_CompilePixelShader(m_pD3DDevEx, &m_pShaders[shader_resizex], shader_resizex)) {
				return false;
			}
		}
		if (!m_pShaders[shader_resizey]) {
			if (!__D3DX_CompilePixelShader(m_pD3DDevEx, &m_pShaders[shader_resizey], shader_resizey)) {
				return false;
			}
		}

		if (!m_pShaders[shader_yv12torgb32]) {
			if (!__D3DX_CompilePixelShader(m_pD3DDevEx, &m_pShaders[shader_yv12torgb32], shader_yv12torgb32)) {
				return false;
			}
		}

		if (FAILED(m_pD3DDevEx->CreateTexture(
			m_iWidth, m_iHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTexture, nullptr))) {
			WXLogW(L"m_pTexture error");
			return false;
		}

		if (FAILED(m_pTexture->GetSurfaceLevel(0, &m_pSurface))) {
			WXLogW(L"VideoSurface error");
			return false;
		}

		// Create textures.
		m_pD3DDevEx->CreateTexture(m_iWidth, m_iHeight, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_Y], nullptr);
		m_pD3DDevEx->CreateTexture(m_iWidth / 2, m_iHeight / 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_U], nullptr);
		m_pD3DDevEx->CreateTexture(m_iWidth / 2, m_iHeight / 2, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &m_pTex[INDEX_V], nullptr);
		return true;
	}

	bool __D3DX_CompilePixelShader(IDirect3DDevice9* pD3DDev, IDirect3DPixelShader9** ppPixelShader, int iShader)
	{
		*ppPixelShader = nullptr;
		HRESULT hr = S_OK;
		CComPtr<ID3DXBuffer>pShader = nullptr;
		if (iShader == shader_resizex) {
			CComPtr<ID3DXConstantTable> pCtTable = nullptr;
			CComPtr<ID3DXBuffer>pErrorBuffer = nullptr;
			hr = LibInst::GetInst().mD3DXCompileShader(strResizeX, strlen(strResizeX), 0, 0, "Main", "ps_3_0",
				D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
				&pShader, &pErrorBuffer, &pCtTable);
			if (FAILED(hr)) {
				pShader = nullptr;
				pErrorBuffer = nullptr;
				pCtTable = nullptr;
				hr = LibInst::GetInst().mD3DXCompileShader(strResizeX, strlen(strResizeX), 0, 0, "Main", "ps_2_0",
					D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
					&pShader, &pErrorBuffer, &pCtTable);
			}
		}
		else if (iShader == shader_resizey) {
			CComPtr<ID3DXConstantTable> pCtTable = nullptr;
			CComPtr<ID3DXBuffer>pErrorBuffer = nullptr;
			hr = LibInst::GetInst().mD3DXCompileShader(strResizeY, strlen(strResizeY), 0, 0, "Main", "ps_3_0",
				D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
				&pShader, &pErrorBuffer, &pCtTable);
			if (FAILED(hr)) {
				pShader = nullptr;
				pErrorBuffer = nullptr;
				pCtTable = nullptr;
				hr = LibInst::GetInst().mD3DXCompileShader(strResizeY, strlen(strResizeY), 0, 0, "Main", "ps_2_0",
					D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
					&pShader, &pErrorBuffer, &pCtTable);
			}
		}
		else if (iShader == shader_yv12torgb32) {
			CComPtr<ID3DXBuffer>pErrorBuffer = nullptr;
			m_pCTable = nullptr;
			hr = LibInst::GetInst().mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_3_0", D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY, &pShader, &pErrorBuffer, &m_pCTable);
			if (FAILED(hr)) {
				pShader = nullptr;
				pErrorBuffer = nullptr;
				m_pCTable = nullptr;
				hr = LibInst::GetInst().mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_2_0", D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY, &pShader, &pErrorBuffer, &m_pCTable);
			}
		}

		if (SUCCEEDED(hr)) {
			hr = m_pD3DDevEx->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
		}

		if (SUCCEEDED(hr) && iShader == shader_yv12torgb32) {
			m_hBrightness = m_pCTable->GetConstantByName(0, "Brightness");//亮度
			m_hSaturation = m_pCTable->GetConstantByName(0, "Saturation");//饱和度
			m_hContrast = m_pCTable->GetConstantByName(0, "Contrast");//对比度
			m_hF1 = m_pCTable->GetConstantByName(0, "f1");//
			m_hF2 = m_pCTable->GetConstantByName(0, "f2");//
			m_hR1 = m_pCTable->GetConstantByName(0, "r1");//
			m_hG1 = m_pCTable->GetConstantByName(0, "g1");//
			m_hG2 = m_pCTable->GetConstantByName(0, "g2");//
			m_hB1 = m_pCTable->GetConstantByName(0, "b1");//
			m_pCTable->SetDefaults(m_pD3DDevEx);
		}
		if (FAILED(hr)) {
			WXLogW(L"CreatePixelShader error:" + iShader);
			return false;
		}
		return true;
	}

	void __D3DX_TextureBlt(IDirect3DDevice9* pD3DDev, D3DTEXTUREFILTERTYPE filter) {
		if (!pD3DDev) {
			return;
		}
		//HRESULT hr;
		pD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		pD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
		pD3DDev->SetRenderState(D3DRS_ZENABLE, FALSE);
		pD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		pD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		pD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		pD3DDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		pD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

		pD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, filter);
		pD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, filter);
		pD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		pD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		pD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
	}

	bool __D3DX_TextureResize(IDirect3DTexture9* pTexture, const RECT& srcRect, const RECT& destRect, D3DTEXTUREFILTERTYPE filter)
	{
		float dx = 1.0f / m_iWidth;
		float dy = 1.0f / m_iHeight;
		CUSTOMVERTEX v[4];
		switch (m_nRotateFlip)
		{
		case RENDER_ROTATE_NONE:
		case RENDER_ROTATE_NONE_FLIPX:
			v[0] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.left * dx, srcRect.top * dy };
			v[1] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.right * dx, srcRect.top * dy };
			v[2] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.right * dx, srcRect.bottom * dy };
			v[3] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.left * dx, srcRect.bottom * dy };
			break;
		case RENDER_ROTATE_90:
		case RENDER_ROTATE_270_FLIPX:
			v[0] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.left * dx, srcRect.top * dy };
			v[1] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.right * dx, srcRect.top * dy };
			v[2] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.right * dx, srcRect.bottom * dy };
			v[3] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.left * dx, srcRect.bottom * dy };
			break;
		case RENDER_ROTATE_180:
		case RENDER_ROTATE_180_FLIPX:
			v[0] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.left * dx, srcRect.top * dy };
			v[1] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.right * dx, srcRect.top * dy };
			v[2] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.right * dx, srcRect.bottom * dy };
			v[3] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.left * dx, srcRect.bottom * dy };
			break;
		case RENDER_ROTATE_270:
		case RENDER_ROTATE_90_FLIPX:
			v[0] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.left * dx, srcRect.top * dy };
			v[1] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.right * dx, srcRect.top * dy };
			v[2] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, srcRect.right * dx, srcRect.bottom * dy };
			v[3] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, srcRect.left * dx, srcRect.bottom * dy };
			break;
		default:
			break;
		}

		m_pD3DDevEx->SetTexture(0, pTexture);
		m_pD3DDevEx->SetPixelShader(nullptr);
		__D3DX_TextureBlt(m_pD3DDevEx, filter);
		if (FAILED(m_pD3DDevEx->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0])))) {
			WXLogW(L"resize draw error");
			return false;
		}
		m_pD3DDevEx->SetTexture(0, nullptr);
		return true;
	}

	bool __D3DX_TextureResizeShader(IDirect3DTexture9* pTexture, const RECT& srcRect, const RECT& destRect, int iShader)
	{
		HRESULT hr = S_OK;
		D3DSURFACE_DESC desc;
		if (!pTexture || FAILED(pTexture->GetLevelDesc(0, &desc))) {
			WXLogW(L"Desc error");
			return false;
		}

		const float dx = 1.0f / desc.Width;
		const float dy = 1.0f / desc.Height;

		const float rx = (float)(srcRect.right - srcRect.left) / (destRect.right - destRect.left);
		const float ry = (float)(srcRect.bottom - srcRect.top) / (destRect.bottom - destRect.top);

		const float tx0 = (float)srcRect.left - 0.5f;
		const float ty0 = (float)srcRect.top - 0.5f;
		const float tx1 = (float)srcRect.right - 0.5f;
		const float ty1 = (float)srcRect.bottom - 0.5f;

		CUSTOMVERTEX v[4];
		switch (m_nRotateFlip)
		{
		case RENDER_ROTATE_90:
		case RENDER_ROTATE_270_FLIPX:
			v[0] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, tx0, ty0 };
			v[1] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx1, ty0 };
			v[2] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx1, ty1 };
			v[3] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, tx0, ty1 };
			break;
		case RENDER_ROTATE_180:
		case RENDER_ROTATE_180_FLIPX:
			v[0] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx0, ty0 };
			v[1] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx1, ty0 };
			v[2] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, tx1, ty1 };
			v[3] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, tx0, ty1 };
			break;
		case RENDER_ROTATE_270:
		case RENDER_ROTATE_90_FLIPX:
			v[0] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx0, ty0 };
			v[1] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, tx1, ty0 };
			v[2] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, tx1, ty1 };
			v[3] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx0, ty1 };
			break;
		case RENDER_ROTATE_NONE:
		case RENDER_ROTATE_NONE_FLIPX:
			v[0] = { (float)destRect.left - 0.5f,  (float)destRect.top - 0.5f,    0.5f, 2.0f, tx0, ty0 };
			v[1] = { (float)destRect.right - 0.5f, (float)destRect.top - 0.5f,    0.5f, 2.0f, tx1, ty0 };
			v[2] = { (float)destRect.right - 0.5f, (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx1, ty1 };
			v[3] = { (float)destRect.left - 0.5f,  (float)destRect.bottom - 0.5f, 0.5f, 2.0f, tx0, ty1 };
			break;
		}

		float fConstData[][4] = { { dx, dy, 0, 0 },{ rx, ry, 0, 0 } };
		hr |= m_pD3DDevEx->SetPixelShaderConstantF(0, (float*)fConstData, _countof(fConstData));
		hr |= m_pD3DDevEx->SetPixelShader(m_pShaders[iShader]);
		if (FAILED(hr))
		{
			WXLogW(L"set shader error");
			return false;
		}

		hr |= m_pD3DDevEx->SetTexture(0, pTexture);
		__D3DX_TextureBlt(m_pD3DDevEx, D3DTEXF_LINEAR);
		hr |= m_pD3DDevEx->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, v, sizeof(v[0]));
		if (FAILED(hr))
		{
			WXLogW(L"resize shader draw error");
			return false;
		}
		m_pD3DDevEx->SetTexture(0, nullptr);
		m_pD3DDevEx->SetPixelShader(nullptr);

		return true;
	}

	bool __D3DX_TextureResizeShader2pass(IDirect3DTexture9* pTexture, const RECT& srcRect, const RECT& destRect, int iShader)
	{
		HRESULT hr = S_OK;

		int w1 = srcRect.right - srcRect.left;
		int h1 = srcRect.bottom - srcRect.top;
		int w2 = destRect.right - destRect.left;
		int h2 = destRect.bottom - destRect.top;

		if (w1 == w2 && h1 == h2)
			return true;

		if (w1 != w2 && h1 != h2) { // need two pass
			D3DSURFACE_DESC desc;

			UINT texWidth = ((DWORD)w2 < m_Caps.MaxTextureWidth) ? w2 : m_Caps.MaxTextureWidth;
			UINT texHeight = ((DWORD)m_iHeight < m_Caps.MaxTextureHeight) ? m_iHeight : m_Caps.MaxTextureHeight;

			if (m_pResizeTexture && m_pResizeTexture->GetLevelDesc(0, &desc) == D3D_OK) {
				if (texWidth != desc.Width || texHeight != desc.Height) {
					m_pResizeTexture = nullptr; // need new texture
				}
			}

			if (!m_pResizeTexture) {
				hr = m_pD3DDevEx->CreateTexture(
					texWidth, texHeight, 1, D3DUSAGE_RENDERTARGET,
					D3DFMT_A16B16G16R16F, // use only float textures here
					D3DPOOL_DEFAULT, &m_pResizeTexture, nullptr);
				if (FAILED(hr) || FAILED(m_pResizeTexture->GetLevelDesc(0, &desc))) {
					m_pResizeTexture = nullptr;
					return __D3DX_TextureResize(pTexture, srcRect, destRect, D3DTEXF_LINEAR);
				}
			}

			RECT resizeRect{ 0,0,(LONG)desc.Width ,(LONG)desc.Height };

			// remember current RenderTarget
			CComPtr<IDirect3DSurface9> pRenderTarget;
			hr = m_pD3DDevEx->GetRenderTarget(0, &pRenderTarget);
			// set temp RenderTarget
			CComPtr<IDirect3DSurface9> pResizeSurface;
			hr |= m_pResizeTexture->GetSurfaceLevel(0, &pResizeSurface);

			hr |= m_pD3DDevEx->SetRenderTarget(0, pResizeSurface);
			// resize width
			if (!__D3DX_TextureResizeShader(pTexture, srcRect, resizeRect, iShader)) {//(w1 > w2 * 2) ? shader_downscaling_x : 
				// restore current RenderTarget
				hr |= m_pD3DDevEx->SetRenderTarget(0, pRenderTarget);
				return false;
			}

			// restore current RenderTarget
			hr |= m_pD3DDevEx->SetRenderTarget(0, pRenderTarget);
			// resize height
			if (!__D3DX_TextureResizeShader(m_pResizeTexture, resizeRect, destRect, iShader + 1)) {// (h1 > h2 * 2) ?   shader_downscaling_y : 
				return false;
			}
			if (FAILED(hr)) {
				WXLogW(L"resize 2pass error");
				return false;
			}
		}
		else if (w1 != w2) {
			return __D3DX_TextureResizeShader(pTexture, srcRect, destRect, iShader);//(w1 > w2 * 2) ? shader_downscaling_x :
		}
		else { // if (h1 != h2)
			return __D3DX_TextureResizeShader(pTexture, srcRect, destRect, iShader + 1);//(h1 > h2 * 2) ? shader_downscaling_y : 
		}

		return true;
	}

	void* __D3DX_RenderYV12(AVFrame* frame) {

		WXAutoLock al(m_mutex);
		HRESULT hr = S_OK;
		hr |= m_pD3DDevEx->BeginScene();

		m_pBackBuffer = nullptr;
		hr |= m_pD3DDevEx->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
		hr |= m_pD3DDevEx->SetRenderTarget(0, m_pBackBuffer);
		hr |= m_pD3DDevEx->Clear(0, nullptr, D3DCLEAR_TARGET, 0/*m_bkColor*/, 1.0f, 0);

		D3DLOCKED_RECT d3d_rect[INDEX_NUM];
		hr = m_pTex[INDEX_Y]->LockRect(0, &d3d_rect[INDEX_Y], 0, 0);
		hr |= m_pTex[INDEX_U]->LockRect(0, &d3d_rect[INDEX_U], 0, 0);
		hr |= m_pTex[INDEX_V]->LockRect(0, &d3d_rect[INDEX_V], 0, 0);
		if (SUCCEEDED(hr)) {
			if (m_nRotateFlip < 4) {
				//I420Copy
				libyuv::I420Copy(
					frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					(uint8_t*)d3d_rect[INDEX_Y].pBits, d3d_rect[INDEX_Y].Pitch,
					(uint8_t*)d3d_rect[INDEX_U].pBits, d3d_rect[INDEX_U].Pitch,
					(uint8_t*)d3d_rect[INDEX_V].pBits, d3d_rect[INDEX_V].Pitch,
					m_iWidth, m_iHeight
				);
			}
			else {
				//I420Mirror
				libyuv::I420Mirror(
					frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					(uint8_t*)d3d_rect[INDEX_Y].pBits, d3d_rect[INDEX_Y].Pitch,
					(uint8_t*)d3d_rect[INDEX_U].pBits, d3d_rect[INDEX_U].Pitch,
					(uint8_t*)d3d_rect[INDEX_V].pBits, d3d_rect[INDEX_V].Pitch,
					m_iWidth, m_iHeight
				);
			}
			m_pTex[INDEX_Y]->UnlockRect(0);
			m_pTex[INDEX_U]->UnlockRect(0);
			m_pTex[INDEX_V]->UnlockRect(0);
		}
		else {
			return nullptr;
		}
		//------------------------------------------------------------------------------------------------
		CUSTOMVERTEX vpos[] = {
			{ -0.5f, -0.5f, 0.5f, 2.0f, 0, 0 },
			{ (float)m_iWidth - 0.5f, -0.5f, 0.5f, 2.0f, 1, 0 },
			{ (float)m_iWidth - 0.5f,(float)m_iHeight - 0.5f, 0.5f, 2.0f, 1, 1 },
			{ -0.5f,(float)m_iHeight - 0.5f, 0.5f, 2.0f, 0, 1 },
		};
		m_pCTable->SetFloat(m_pD3DDevEx, m_hBrightness, m_fBrightness);//
		m_pCTable->SetFloat(m_pD3DDevEx, m_hSaturation, m_fSaturation);//
		m_pCTable->SetFloat(m_pD3DDevEx, m_hContrast, m_fContrast);//
		if (frame->format == AV_PIX_FMT_YUVJ420P) {
			m_pCTable->SetFloat(m_pD3DDevEx, m_hF1, vf1);//
			m_pCTable->SetFloat(m_pD3DDevEx, m_hF2, vf2);//
			m_pCTable->SetFloat(m_pD3DDevEx, m_hR1, vr1);//
			m_pCTable->SetFloat(m_pD3DDevEx, m_hG1, vg1);//
			m_pCTable->SetFloat(m_pD3DDevEx, m_hG2, vg2);//
			m_pCTable->SetFloat(m_pD3DDevEx, m_hB1, vb1);//
		}
		hr = m_pD3DDevEx->SetPixelShader(m_pShaders[shader_yv12torgb32]);

		hr |= m_pD3DDevEx->SetTexture(INDEX_Y, m_pTex[INDEX_Y]);
		hr |= m_pD3DDevEx->SetTexture(INDEX_U, m_pTex[INDEX_U]);
		hr |= m_pD3DDevEx->SetTexture(INDEX_V, m_pTex[INDEX_V]);

		m_pD3DDevEx->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		m_pD3DDevEx->SetRenderState(D3DRS_LIGHTING, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_ZENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);

		m_pD3DDevEx->SetSamplerState(INDEX_Y, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_Y, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_Y, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_pD3DDevEx->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_pD3DDevEx->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		m_pD3DDevEx->SetSamplerState(INDEX_U, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_U, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_U, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_pD3DDevEx->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_pD3DDevEx->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		m_pD3DDevEx->SetSamplerState(INDEX_V, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_V, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_pD3DDevEx->SetSamplerState(INDEX_V, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_pD3DDevEx->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_pD3DDevEx->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		m_pD3DDevEx->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX3);

		hr |= m_pD3DDevEx->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, vpos, sizeof(vpos[0]));
		if (FAILED(hr)) {
			WXLogW(L"draw convert error");
			return nullptr;
		}
		m_pD3DDevEx->SetTexture(INDEX_Y, nullptr);
		m_pD3DDevEx->SetTexture(INDEX_U, nullptr);
		m_pD3DDevEx->SetTexture(INDEX_V, nullptr);
		m_pD3DDevEx->SetPixelShader(nullptr);

		hr |= m_pD3DDevEx->StretchRect(m_pBackBuffer, &m_rcSrc, m_pSurface, &m_rcSrc, D3DTEXF_LINEAR);
		hr |= m_pD3DDevEx->EndScene();
		if (SUCCEEDED(hr)) {
			return m_pSurface;
		}
		return nullptr;
	}

	void* __D3DX_RenderRGBA(uint8_t* buf, int pitch)
	{
		HRESULT hr = S_OK;
		if (m_pSurfRGBA[0] == nullptr) {
			m_pTexRGBA[0] = nullptr;
			hr = m_pD3DDevEx->CreateTexture(m_iWidth, m_iHeight, 1, D3DUSAGE_DYNAMIC,
				D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTexRGBA[0], nullptr);
			if (SUCCEEDED(hr)) {
				m_pSurfRGBA[0] = nullptr;
				hr = m_pTexRGBA[0]->GetSurfaceLevel(0, &m_pSurfRGBA[0]);
			}
			if (FAILED(hr)) {
				return nullptr;
			}
		}

		//拷贝rgb32数据至离屏表面
		D3DLOCKED_RECT d3d_rect;
		hr |= m_pSurfRGBA[0]->LockRect(&d3d_rect, 0, 0);
		if (SUCCEEDED(hr)) {
			//ARGBCopy
			if (m_nRotateFlip < 4)
				libyuv::ARGBCopy(buf, pitch,
					(byte*)d3d_rect.pBits, d3d_rect.Pitch,
					m_iWidth, m_iHeight);
			else
				libyuv::ARGBMirror(buf, pitch,
					(byte*)d3d_rect.pBits, d3d_rect.Pitch,
					m_iWidth, m_iHeight);

			hr = m_pSurfRGBA[0]->UnlockRect();
			hr = m_pD3DDevEx->BeginScene();
			hr = m_pD3DDevEx->StretchRect(m_pSurfRGBA[0], &m_rcSrc, m_pSurface, &m_rcSrc, D3DTEXF_LINEAR);
			m_pBackBuffer = nullptr;
			hr = m_pD3DDevEx->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
			hr = m_pD3DDevEx->EndScene();
			return m_pSurface;
		}
		return nullptr;
	}

	bool __D3DX_RenderTexture() {
		RECT rcDst;
		::GetClientRect(m_hWnd, &rcDst);//显示区域
		RECT windowRect;
		::GetClientRect(m_hWnd, &windowRect);//当前窗口大小
		if (m_bFixed) {
			int dx = 0;
			int dy = 0;
			if (m_nRotateFlip % 2)
				WXMeidaUtilsGetDisplayRect(windowRect.right, windowRect.bottom, m_iHeight, m_iWidth, &dx, &dy);
			else
				WXMeidaUtilsGetDisplayRect(windowRect.right, windowRect.bottom, m_iWidth, m_iHeight, &dx, &dy);
			rcDst.left += dx;
			rcDst.right -= dx;
			rcDst.top += dy;
			rcDst.bottom -= dy;
		}
		int w1 = rcDst.right - rcDst.left;
		int h1 = rcDst.bottom - rcDst.top;

		HRESULT hr = S_OK;
		hr |= m_pD3DDevEx->BeginScene();
		hr |= m_pD3DDevEx->SetRenderTarget(0, m_pBackBuffer);
		hr |= m_pD3DDevEx->Clear(0, nullptr, D3DCLEAR_TARGET, 0/*m_bkColor*/, 1.0f, 0);
		// resize
		bool bret = false;
		if (w1 <= m_iMaxSize && h1 < m_iMaxSize && (w1 != m_iWidth || h1 != m_iHeight)) {
			bret = __D3DX_TextureResizeShader2pass(m_pTexture, m_rcSrc, rcDst, shader_resizex);
		}
		if (!bret) {
			bret = __D3DX_TextureResize(m_pTexture, m_rcSrc, rcDst, D3DTEXF_LINEAR);
		}
		hr = m_pD3DDevEx->EndScene();
		hr = m_pD3DDevEx->PresentEx(&windowRect, &windowRect, nullptr, nullptr, 0);
		if (hr == D3DERR_DEVICELOST) {
			hr = m_pD3DDevEx->TestCooperativeLevel();
			if (SUCCEEDED(hr) || hr == D3DERR_DEVICENOTRESET) {
				//Reset
				m_pD3DDevEx->Reset(&m_d3dpp);
			}
			return false;
		}
		return true;
	}



	BOOL D3DX_Draw(AVFrame* frame, int bFixed, int nRotateFlip, float fBrightness, float fSaturation, float fContrast) {
		WXAutoLock al(m_mutex);
		if (!m_bOpen)return FALSE;
		m_fBrightness = fBrightness;
		m_fSaturation = fSaturation;
		m_fContrast = fContrast;

		m_bFixed = bFixed;
		m_nRotateFlip = nRotateFlip;

		if (frame) {
			if (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUVJ420P) {
				if (nullptr != __D3DX_RenderYV12(frame)) {
					return __D3DX_RenderTexture();
				}
			}
			else if (frame->format == AV_PIX_FMT_RGB32) {
				if (nullptr != __D3DX_RenderRGBA(frame->data[0], frame->linesize[0])) {
					return __D3DX_RenderTexture();
				}
			}
			else { //NULL
				AVFrame* dstFrame = m_I420Frame.GetFrame();
				m_conv.Convert(frame, dstFrame);
				if (nullptr != __D3DX_RenderYV12(frame)) {
					return __D3DX_RenderTexture();
				}
			}
		}
		return TRUE;
	}

	void D3DX_Close() {
		WXAutoLock al(m_mutex);
		//WXLogA("%s", __FUNCTION__);
		if (m_bOpen) {
			m_pD3DEx = nullptr;
			m_pD3DDevEx = nullptr;
			m_pTexture = nullptr;
			m_pSurface = nullptr;
			m_pResizeTexture = nullptr;

			m_pSurfRGBA[0] = nullptr;
			m_pSurfRGBA[1] = nullptr;
			m_pSurfYUV[0] = nullptr;
			m_pSurfYUV[1] = nullptr;
			m_pTexRGBA[0] = nullptr;
			m_pTexRGBA[1] = nullptr;
			m_pTexYUV[0] = nullptr;
			m_pTexYUV[1] = nullptr;

			m_pTex[INDEX_Y] = nullptr;
			m_pTex[INDEX_U] = nullptr;
			m_pTex[INDEX_V] = nullptr;
			for (unsigned i = 0; i < _countof(m_pShaders); i++) {
				m_pShaders[i] = nullptr;
			}
			m_bOpen = FALSE;
		}
	}

	BOOL D3DX_Open(HWND hwnd, int width, int height) {
		WXAutoLock al(m_mutex);
		//WXLogA("%s", __FUNCTION__);
		m_hWnd = hwnd;
		m_iWidth = width / 2 * 2;
		m_iHeight = height / 2 * 2;
		m_rcSrc.right = m_iWidth;
		m_rcSrc.bottom = m_iHeight;

		if (LibInst::GetInst().m_libD3DX9 == nullptr) {
			return false;
		}
		m_pD3DEx = nullptr;
		LibInst::GetInst().mDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);
		if (!m_pD3DEx) {
			//WXLogW(L"Direct3DCreate9Ex error");
			return false;
		}
		int nCount = m_pD3DEx->GetAdapterCount();
		for (int index = 0; index < nCount; index++) {
			if (__D3DX_CreateD3DX(index)) {//d3dx9
				m_I420Frame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
				m_bOpen = TRUE;
				//WXLogA("%s OK Size = %dx%d", __FUNCTION__, m_iWidth, m_iHeight);
				return TRUE;
			}
		}
		//WXLogA("%s Failed!!!", __FUNCTION__);
		return FALSE;
	}



	BOOL _GDI_Draw(AVFrame* dstFrame, int bFixed, int nRotateFlip) {

		RECT rcDst;
		GetClientRect(m_hWnd, &rcDst);
		if (bFixed) {
			int WndW = rcDst.right - rcDst.left;
			int WndH = rcDst.bottom - rcDst.top;
			int desX = 0;
			int desY = 0;
			if (nRotateFlip % 2)
				WXMeidaUtilsGetDisplayRect(WndW, WndH, m_iHeight, m_iWidth, &desX, &desY);
			else
				WXMeidaUtilsGetDisplayRect(WndW, WndH, m_iWidth, m_iHeight, &desX, &desY);
			rcDst.left += desX;
			rcDst.right -= desX;
			rcDst.top += desY;
			rcDst.bottom -= desY;
		}

		Gdiplus::Rect rc(rcDst.left, rcDst.top, rcDst.right - rcDst.left, rcDst.bottom - rcDst.top);

		Gdiplus::Bitmap bmp(m_iWidth, m_iHeight, dstFrame->linesize[0], PixelFormat32bppARGB, dstFrame->data[0]);
		bmp.RotateFlip((Gdiplus::RotateFlipType)nRotateFlip);//

		HDC hdc = ::GetDC(m_hWnd);
		Gdiplus::Graphics display(hdc);
		display.DrawImage(&bmp, rc, 0, 0, bmp.GetWidth(), bmp.GetHeight(), Gdiplus::UnitPixel);
		::ReleaseDC(m_hWnd, hdc);
		return TRUE;
	}

	virtual BOOL GDI_Open(HWND hwnd, int width, int height) {
		WXAutoLock al(m_mutex);
		if (!m_bOpen) {
			m_bOpen = TRUE;
			m_hWnd = hwnd;
			m_iWidth = width;
			m_iHeight = height;
			m_RGB32Frame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
			WXLogA("%s OK Size = %dx%d", __FUNCTION__, m_iWidth, m_iHeight);
		}
		return m_bOpen;
	}
	virtual BOOL GDI_Draw(AVFrame* frame, int bFixed, int rotate, float fBrightness, float fSaturation, float fContrast) {
		WXAutoLock al(m_mutex);
		if (!m_bOpen)return FALSE;
		AVFrame* dstFrame = m_RGB32Frame.GetFrame();
		if (frame) {
			if (frame->format == AV_PIX_FMT_YUV420P) {
				libyuv::I420ToARGB(frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					dstFrame->data[0], dstFrame->linesize[0],
					m_iWidth, m_iHeight);
				return _GDI_Draw(dstFrame, bFixed, rotate);
			}
			else if (frame->format == AV_PIX_FMT_YUVJ420P) {
				libyuv::J420ToARGB(frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					dstFrame->data[0], dstFrame->linesize[0],
					m_iWidth, m_iHeight);
				return _GDI_Draw(dstFrame, bFixed, rotate);
			}
			else if (frame->format == AV_PIX_FMT_RGB32) {
				return _GDI_Draw(frame, bFixed, rotate);
			}
			else {
				m_conv.Convert(frame, dstFrame);
				return _GDI_Draw(dstFrame, bFixed, rotate);
			}
		}
		return TRUE;
	}

	virtual void GDI_Close() {
		if (m_bOpen) {
			WXLogA("%s OK Size = %dx%d", __FUNCTION__, m_iWidth, m_iHeight);
			m_bOpen = FALSE;
		}
	}

	void InitD3DTexture() {
		m_pTexRGBA[0] = nullptr;
		m_pSurfRGBA[0] = nullptr;
		m_pTexRGBA[1] = nullptr;
		m_pSurfRGBA[1] = nullptr;

		m_pTexYUV[0] = nullptr;
		m_pSurfYUV[0] = nullptr;
		m_pTexYUV[1] = nullptr;
		m_pSurfYUV[1] = nullptr;

		HRESULT hr = m_pD3dDevice->CreateTexture(m_iWidth, m_iHeight, 1, D3DUSAGE_DYNAMIC,
			(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), D3DPOOL_DEFAULT, &m_pTexYUV[0], nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_pTexYUV[0]->GetSurfaceLevel(0, &m_pSurfYUV[0]);
		}
		hr = m_pD3dDevice->CreateTexture(m_iHeight, m_iWidth, 1, D3DUSAGE_DYNAMIC,
			(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), D3DPOOL_DEFAULT, &m_pTexYUV[1], nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_pTexYUV[1]->GetSurfaceLevel(0, &m_pSurfYUV[1]);
		}
		if (m_pSurfYUV[0] && m_pSurfYUV[1]) {
			m_bSupportYUV = TRUE;
		}
		else {
			m_bSupportYUV = FALSE;
			WXLogA("%s Not Support YV12 Mode, check Display Driver", __FUNCTION__);
		}
	}
	virtual BOOL  D3D_Open(HWND hwnd, int width, int height) {
		WXAutoLock al(m_mutex);
		if (LibInst::GetInst().m_libD3D9 == nullptr) {
			return FALSE;
		}
		m_pD3d9 = LibInst::GetInst().mDirect3DCreate9(D3D_SDK_VERSION);
		if (m_pD3d9 == nullptr) {
			return FALSE;
		}
		m_hWnd = hwnd;
		m_iWidth = width;
		m_iHeight = height;
		m_I420Frame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
		m_RGB32Frame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
		m_I420MirrorFrame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
		m_RGB32MirrorFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);

		memset(&m_d3dpp, 0, sizeof(m_d3dpp));
		m_d3dpp.BackBufferCount = 1;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		m_d3dpp.Windowed = TRUE;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		m_d3dpp.BackBufferWidth = std::max(m_iWidth, m_iHeight);
		m_d3dpp.BackBufferHeight = std::max(m_iWidth, m_iHeight);

		int nCount = m_pD3d9->GetAdapterCount();

		HRESULT hr = E_FAIL;
		for (int index = 0; index < nCount; index++) {
			D3DCAPS9	caps;
			hr = m_pD3d9->GetDeviceCaps(index, D3DDEVTYPE_HAL, &caps);
			int vp = 0;
			if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
				vp = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
			else
				vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;

			hr = m_pD3d9->CreateDevice(index, D3DDEVTYPE_HAL, m_hWnd, vp,
				&m_d3dpp, &m_pD3dDevice);
			if (FAILED(hr)) {
				return FALSE;
			}
			else {
				break;
			}
		}

		hr = m_pD3dDevice->CreateTexture(m_iWidth, m_iHeight, 1, D3DUSAGE_DYNAMIC,
			D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTexRGBA[0], nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_pTexRGBA[0]->GetSurfaceLevel(0, &m_pSurfRGBA[0]);
		}
		if (FAILED(hr)) {
			return FALSE;
		}

		hr = m_pD3dDevice->CreateTexture(m_iHeight, m_iWidth, 1, D3DUSAGE_DYNAMIC,
			D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_pTexRGBA[1], nullptr);
		if (SUCCEEDED(hr)) {
			hr = m_pTexRGBA[1]->GetSurfaceLevel(0, &m_pSurfRGBA[1]);
		}
		if (FAILED(hr)) {
			return FALSE;
		}

		InitD3DTexture();
		m_bOpen = true;
		WXLogA("%s OK Size = %dx%d", __FUNCTION__, m_iWidth, m_iHeight);
		return TRUE;
	}

	BOOL _D3D_DrawSurface(IDirect3DSurface9* surf, int bFixed, int rotate) {
		RECT rcDst;
		::GetClientRect(m_hWnd, &rcDst);
		if (bFixed) { //在窗体上的显示区域
			int WndW = rcDst.right - rcDst.left;
			int WndH = rcDst.bottom - rcDst.top;
			int desX = 0;
			int desY = 0;
			if (rotate % 2)
				WXMeidaUtilsGetDisplayRect(WndW, WndH, m_iHeight, m_iWidth, &desX, &desY);
			else
				WXMeidaUtilsGetDisplayRect(WndW, WndH, m_iWidth, m_iHeight, &desX, &desY);
			rcDst.left += desX;
			rcDst.right -= desX;
			rcDst.top += desY;
			rcDst.bottom -= desY;
		}

		HRESULT hr = m_pD3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
		if (FAILED(hr))return FALSE;
		hr = m_pD3dDevice->BeginScene();
		if (FAILED(hr))return FALSE;

		CComPtr<IDirect3DSurface9>pBackBuffer = nullptr;
		hr = m_pD3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);//主表面的buffer, 和窗体大小一样?
		if (FAILED(hr))return FALSE;

		RECT rcSrc{ 0,0, (rotate % 2) ? m_iHeight : m_iWidth ,(rotate % 2) ? m_iWidth : m_iHeight };
		hr = m_pD3dDevice->StretchRect(surf, &rcSrc, pBackBuffer, &rcSrc, D3DTEXF_LINEAR);
		if (FAILED(hr))return FALSE;

		hr = m_pD3dDevice->EndScene();
		if (FAILED(hr))return FALSE;

		hr = m_pD3dDevice->Present(&rcSrc, &rcDst, nullptr, nullptr);//缩放效果取决于系统
		if (hr == D3DERR_DEVICELOST) {
			hr = m_pD3dDevice->TestCooperativeLevel();
			if (SUCCEEDED(hr) || hr == D3DERR_DEVICENOTRESET) {
				//Reset
				m_pD3dDevice->Reset(&m_d3dpp);
			}
			return FALSE;
		}
		return TRUE;
	}

	BOOL _D3D_DrawRGBAImpl(AVFrame* frame, int bFixed, int nRotateFilp) { //输入为YUV420P 或者 RGBA
		AVFrame* dstFrame = m_RGB32Frame.GetFrame();//带旋转
		if (nRotateFilp >= 4)
			dstFrame = m_RGB32MirrorFrame.GetFrame();//带旋转
		if (frame) {
			if (frame->format == AV_PIX_FMT_YUV420P) { //先转格式
				libyuv::I420ToARGB(frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					dstFrame->data[0], dstFrame->linesize[0],
					m_iWidth, m_iHeight);
			}
			else if (frame->format == AV_PIX_FMT_YUVJ420P) { //先转格式
				libyuv::J420ToARGB(frame->data[0], frame->linesize[0],
					frame->data[1], frame->linesize[1],
					frame->data[2], frame->linesize[2],
					dstFrame->data[0], dstFrame->linesize[0],
					m_iWidth, m_iHeight);
			}
			else if (frame->format == AV_PIX_FMT_RGB32) {
				libyuv::ARGBCopy(frame->data[0], frame->linesize[0],
					dstFrame->data[0], dstFrame->linesize[0],
					m_iWidth, m_iHeight);
			}
			else {
				m_conv.Convert(frame, dstFrame);
			}

			if (nRotateFilp >= 4) {  //翻转
				libyuv::ARGBMirror(dstFrame->data[0], dstFrame->linesize[0],
					m_RGB32Frame.GetFrame()->data[0], m_RGB32Frame.GetFrame()->linesize[0],
					m_iWidth, m_iHeight);
				dstFrame = m_RGB32Frame.GetFrame();
			}
		}

		IDirect3DSurface9* surf = nRotateFilp % 2 ? m_pSurfRGBA[1] : m_pSurfRGBA[0];
		HRESULT hr = 0;
		D3DLOCKED_RECT d3d_rect;
		hr = surf->LockRect(&d3d_rect, 0, 0);
		if (SUCCEEDED(hr)) {
			if (nRotateFilp < 4) {
				libyuv::ARGBRotate(dstFrame->data[0], dstFrame->linesize[0],
					(uint8_t*)d3d_rect.pBits, d3d_rect.Pitch,
					m_iWidth, m_iHeight,
					(libyuv::RotationMode)((nRotateFilp % 4) * 90));
			}
			else {
				libyuv::ARGBRotate(dstFrame->data[0], dstFrame->linesize[0],
					(uint8_t*)d3d_rect.pBits, d3d_rect.Pitch,
					m_iWidth, m_iHeight,
					(libyuv::RotationMode)(((nRotateFilp + 2) % 4) * 90));
			}
			surf->UnlockRect();

			return _D3D_DrawSurface(surf, bFixed, nRotateFilp);
		}
		return FALSE;
	}

	BOOL _D3D_DrawYUVImpl(AVFrame* dstFrame, int bFixed, int nRotateFilp) {
		BOOL bRotate = nRotateFilp % 2;

		IDirect3DSurface9* surf = bRotate ? m_pSurfYUV[1] : m_pSurfYUV[0];
		if (surf == nullptr) {
			InitD3DTexture();
			return FALSE;
		}

		HRESULT hr = 0;
		D3DLOCKED_RECT d3d_rect;
		hr |= surf->LockRect(&d3d_rect, 0, 0);
		if (SUCCEEDED(hr)) {
			int DstHeight = bRotate ? m_iWidth : m_iHeight;
			uint8_t* pDstY = (uint8_t*)d3d_rect.pBits;
			uint8_t* pDstU = (uint8_t*)d3d_rect.pBits + d3d_rect.Pitch * DstHeight;
			uint8_t* pDstV = (uint8_t*)d3d_rect.pBits + d3d_rect.Pitch * DstHeight * 5 / 4;

			if (nRotateFilp < 4) {
				libyuv::I420Rotate(dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					pDstY, d3d_rect.Pitch,
					pDstV, d3d_rect.Pitch / 2,
					pDstU, d3d_rect.Pitch / 2,
					m_iWidth, m_iHeight,
					(libyuv::RotationMode)((nRotateFilp % 4) * 90));
			}
			else {
				libyuv::I420Rotate(dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					pDstY, d3d_rect.Pitch,
					pDstV, d3d_rect.Pitch / 2,
					pDstU, d3d_rect.Pitch / 2,
					m_iWidth, m_iHeight,
					(libyuv::RotationMode)(((nRotateFilp + 2) % 4) * 90));
			}
			surf->UnlockRect();
			return _D3D_DrawSurface(surf, bFixed, nRotateFilp);
		}
		InitD3DTexture();
		return FALSE;
	}

	BOOL _D3D_DrawI420Impl(AVFrame* frame, int bFixed, int nRotateFilp) { //输入为YUV420P 或者 RGBA

		AVFrame* dstFrame = m_I420Frame.GetFrame();
		if (nRotateFilp >= 4)
			dstFrame = m_I420MirrorFrame.GetFrame();//带旋转

		if (frame) {

			libyuv::I420Copy(frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2],
				dstFrame->data[0], dstFrame->linesize[0],
				dstFrame->data[1], dstFrame->linesize[1],
				dstFrame->data[2], dstFrame->linesize[2],
				m_iWidth, m_iHeight);

			if (nRotateFilp >= 4) {  //翻转
				libyuv::I420Mirror(dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					m_I420Frame.GetFrame()->data[0], m_I420Frame.GetFrame()->linesize[0],
					m_I420Frame.GetFrame()->data[1], m_I420Frame.GetFrame()->linesize[1],
					m_I420Frame.GetFrame()->data[2], m_I420Frame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight);
				dstFrame = m_I420Frame.GetFrame();
			}
		}

		return _D3D_DrawYUVImpl(dstFrame, bFixed, nRotateFilp);
	}

	BOOL _D3D_DrawJ420Impl(AVFrame* frame, int bFixed, int nRotateFilp) { //输入为YUV420P 或者 RGBA

		AVFrame* dstFrame = m_I420Frame.GetFrame();
		if (nRotateFilp >= 4)
			dstFrame = m_I420MirrorFrame.GetFrame();//带旋转

		if (frame) {
			m_conv.Convert(frame, dstFrame);
			//libyuv::I420Copy(frame->data[0], frame->linesize[0],
			//	frame->data[1], frame->linesize[1],
			//	frame->data[2], frame->linesize[2],
			//	dstFrame->data[0], dstFrame->linesize[0],
			//	dstFrame->data[1], dstFrame->linesize[1],
			//	dstFrame->data[2], dstFrame->linesize[2],
			//	m_iWidth, m_iHeight);

			if (nRotateFilp >= 4) {  //翻转
				libyuv::I420Mirror(dstFrame->data[0], dstFrame->linesize[0],
					dstFrame->data[1], dstFrame->linesize[1],
					dstFrame->data[2], dstFrame->linesize[2],
					m_I420Frame.GetFrame()->data[0], m_I420Frame.GetFrame()->linesize[0],
					m_I420Frame.GetFrame()->data[1], m_I420Frame.GetFrame()->linesize[1],
					m_I420Frame.GetFrame()->data[2], m_I420Frame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight);
				dstFrame = m_I420Frame.GetFrame();
			}
		}

		return _D3D_DrawYUVImpl(dstFrame, bFixed, nRotateFilp);
	}

	virtual BOOL D3D_Draw(AVFrame* frame, int bFixed, int nRotateFilp, float fBrightness, float fSaturation, float fContrast) {
		WXAutoLock al(m_mutex);
		if (!m_bOpen) {
			D3D_Open(m_hWnd, m_iWidth, m_iHeight);
		}
		if (m_bOpen && frame) {
			BOOL bRet = FALSE;
			if (m_bSupportYUV && frame->format == AV_PIX_FMT_YUV420P) {
				bRet = _D3D_DrawI420Impl(frame, bFixed, nRotateFilp);
			}
			else if (m_bSupportYUV && frame->format == AV_PIX_FMT_YUVJ420P) {
				bRet = _D3D_DrawJ420Impl(frame, bFixed, nRotateFilp);
			}
			else {
				bRet = _D3D_DrawRGBAImpl(frame, bFixed, nRotateFilp);
			}
			if (!bRet) {
				D3D_Close();
				D3D_Open(m_hWnd, m_iWidth, m_iHeight);
			}
			return bRet;
		}
		return TRUE;
	}

	virtual void D3D_Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			m_pD3d9 = nullptr;
			m_pD3dDevice = nullptr;
			m_pSurfYUV[0] = nullptr;
			m_pSurfYUV[1] = nullptr;
			m_pSurfRGBA[0] = nullptr;
			m_pSurfRGBA[1] = nullptr;
			m_pTexYUV[0] = nullptr;
			m_pTexYUV[1] = nullptr;
			m_pTexRGBA[0] = nullptr;
			m_pTexRGBA[1] = nullptr;

			WXLogA("%s", __FUNCTION__);
			m_bOpen = FALSE;
		}
	}

public:
	enum
	{
		TYPE_NONE = -1,
		TYPE_D3DX = 0,
		TYPE_D3D = 1,
		TYPE_GDI = 2,

	};
	int m_type = TYPE_NONE;


	virtual BOOL  Open(HWND hwnd, int width, int height) {
		m_type = TYPE_D3DX;
		BOOL bRet = D3DX_Open(hwnd, width, height);
		if (!bRet) {
			m_type = TYPE_D3D;
			bRet = D3D_Open(hwnd, width, height);
			if (!bRet) {
				m_type = TYPE_GDI;
				bRet = GDI_Open(hwnd, width, height);
				if (!bRet) {
					m_type = TYPE_NONE;
				}
			}
		}
		return bRet;
	}

	virtual BOOL Draw(AVFrame* frame, int bFixed, int nRotateFilp, float fBrightness, float fSaturation, float fContrast) {
		if (m_type == TYPE_D3DX) {
			return D3DX_Draw(frame, bFixed, nRotateFilp, fBrightness, fSaturation, fContrast);
		}
		else		if (m_type == TYPE_D3D) {
			return D3D_Draw(frame, bFixed, nRotateFilp, fBrightness, fSaturation, fContrast);
		}
		else	if (m_type == TYPE_GDI) {
			return GDI_Draw(frame, bFixed, nRotateFilp, fBrightness, fSaturation, fContrast);
		}
		return FALSE;
	}
	virtual void Close() {
		if (m_type == TYPE_D3DX) {
			D3DX_Close();
		}
		else if (m_type == TYPE_D3D) {
			D3D_Close();
		}
		else if (m_type == TYPE_GDI) {
			GDI_Close();
		}
	}
};

//支持动态切换分辨率
class VideoRenderer {
	int64_t m_ptsLastDisplay = 0;//最后一帧显示时间

	MMRESULT timer_id = 0;

	WXLocker m_mutex;

	WXVideoFrame   m_I420Frame;
	WXVideoFrame   m_RGB32Frame;

	//窗口消息
	WNDPROC m_oldProc = nullptr;

	WindowsRender* m_render = nullptr;

	int m_iRef = 0;
	HWND m_hWnd = nullptr;//显示窗口
	int m_iWidth = 0;  //视频宽度
	int m_iHeight = 0; //视频高度
	int m_bFixed = FALSE; //保持比例输出
	int m_iRotateType = RENDER_ROTATE_NONE;//旋转角度
	int m_bOpen = FALSE;//是否打开成功
	int m_bkColor = 0;//背景色
	int m_iWndWidth = 0;
	int m_iWndHeight = 0;

	int  m_bSupportD3DX = 1;//是否支持D3DX
	int  m_bSupportD3D = 1;//是否支持D3D

	int  m_bNeedReinit = 0;//当输入图像的分辨率和初始化分辨率不一致的时候，默认需要重置渲染器

	float  m_fBrightness = 1.0f;//亮度
	float  m_fSaturation = 1.0f;//饱和度
	float  m_fContrast = 1.0f;//对比度

public:
	void Refresh() {
		WXAutoLock al(m_mutex);
		if (m_ptsLastDisplay) {//已经有数据了
			int64_t ts = WXGetTimeMs();
			int64_t delay = ts - m_ptsLastDisplay;
			if (delay >= 300) {//250ms 画面不刷新
				DrawYUV();
				m_ptsLastDisplay = ts;
			}
		}
	}

	static void CALLBACK OnTimer(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
		VideoRenderer* pThis = (VideoRenderer*)dwUser;
		if (pThis) {
			pThis->Refresh();
		}
	}

	////拦截窗口消息
	static LRESULT CALLBACK NewWndProcTask(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		VideoRenderer* pThis = (VideoRenderer*)::GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (pThis) {
			return pThis->WndProc(hwnd, message, wParam, lParam);
		}
		return 0;
	}

	LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == WM_PAINT || message == WM_ERASEBKGND || message == WM_SIZE) { //强制重绘
			::ValidateRect(hwnd, NULL);
		}
		if (message == WM_SIZE) {
			Refresh();
		}
		return m_oldProc(hwnd, message, wParam, lParam);
	}

	BOOL m_bStill = FALSE;//非VIP禁止使用音视频功能

	VideoRenderer() {

		m_bStill = WXGetGlobalValue(L"Still");

		m_bSupportD3DX = WXGetIniValue(_T("WXMedia"), L"SupportD3DX", 0);
		m_bSupportD3D = WXGetIniValue(_T("WXMedia"), L"SupportD3D", 0);

		//WXLogA("!!! %s m_bSupportD3DX=%d ", __FUNCTION__, m_bSupportD3DX);
		//WXLogA("!!! %s m_bSupportD3D =%d", __FUNCTION__, m_bSupportD3D);

		//WXLogA("%s m_nLutLevel=%d m_nYScale=%d m_nUVScale=%d", __FUNCTION__, s_nLut, s_nGray, s_nChrome);

		m_fContrast = (s_nLut + 50.0f) / 50.0f;
		m_fBrightness = (s_nGray + 50.0f) / 50.0f;
		m_fSaturation = (s_nChrome + 50.0f) / 50.0f;
	}

	virtual~VideoRenderer() {
		WXAutoLock al(m_mutex);
		Close();
	}
public:

	virtual void   Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			::timeKillEvent(timer_id);        //释放定时器
			timer_id = 0;
			if (m_oldProc != nullptr) {
				::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)nullptr);
				::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_oldProc);
				m_oldProc = nullptr;
			}
			if (m_render) {
				m_render->Close();
				SAFE_DELETE(m_render)
					m_render = nullptr;
			}
			m_bOpen = FALSE;
		}
	}

	virtual void   SetRotate(int rotate) {
		m_iRotateType = std::min(std::max(0, rotate), 7);
	}
	void   SetWindow(HWND view) {
		m_hWnd = view;
	}
	HWND   GetWindow() {
		return m_hWnd;
	}

	virtual void DrawBlack() { //刷黑显示
		HDC hdc = ::GetDC(m_hWnd);
		Gdiplus::Graphics display(hdc);
		Gdiplus::SolidBrush Brush(Gdiplus::Color(0, 0, 0));//黑色
		RECT rect;
		::GetClientRect(m_hWnd, &rect);
		display.FillRectangle(&Brush, 0, 0, rect.right, rect.bottom);//画矩形
		::ReleaseDC(m_hWnd, hdc);
	}

	void DrawRGB() {
		BOOL bVisible = IsWindowVisible(m_hWnd);
		if (!bVisible)
			return;

		if (!m_bStill) { //正常绘制
			m_render->Draw(m_RGB32Frame.GetFrame(), m_bFixed, m_iRotateType, m_fBrightness, m_fContrast, m_fSaturation);//强制刷新
		}
		else { //强制刷黑
			this->DrawBlack();
		}
	}
	void DrawYUV() {
		BOOL bVisible = IsWindowVisible(m_hWnd);
		if (!bVisible)
			return;
		if (!m_bStill) { //正常绘制
			m_render->Draw(m_I420Frame.GetFrame(), m_bFixed, m_iRotateType, m_fContrast, m_fBrightness, m_fSaturation);
		}
		else { //强制刷黑
			this->DrawBlack();
		}
	}
	int Open(int bNeedReopen, int width, int height) {
		WXAutoLock al(m_mutex);

		//	WXLogW(L"%ws", __FUNCTIONW__);
		m_bNeedReinit = bNeedReopen;
		m_iWidth = width / 2 * 2;
		m_iHeight = height / 2 * 2;
		m_RGB32Frame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);//  直接渲染R GBA数据的时候用的
		m_I420Frame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);// 直接渲染YUV数据的时候用的

		if (m_render) {
			m_render->Close();
		}
		else {
			m_render = new WindowsRender;
		}
		m_render->Open(m_hWnd, m_iWidth, m_iHeight);

		if (!m_bOpen) { //ReOpen不执行
			m_bOpen = true;
			if (m_oldProc == nullptr) { //声明原来的WinProc
				m_oldProc = (WNDPROC)::GetWindowLongPtr(m_hWnd, GWLP_WNDPROC);//保存原有的消息处理函数
				::SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
				::SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)VideoRenderer::NewWndProcTask);
			}
			//WXLogW(L"%ws OK!!", __FUNCTIONW__);

			timer_id = timeSetEvent(200, 1, (LPTIMECALLBACK)&VideoRenderer::OnTimer, (DWORD_PTR)this, TIME_PERIODIC);
		}
		return m_bOpen;
	}

	//不支持切换分辨率
	virtual void   DisplayRGBA(uint8_t* pBuf, int Pitch) {
		if (!IsWindowVisible(m_hWnd) || m_render == nullptr)
			return;
		WXAutoLock al(m_mutex);
		m_ptsLastDisplay = WXGetTimeMs();
		libyuv::ARGBCopy(pBuf, Pitch, m_RGB32Frame.GetFrame()->data[0], m_RGB32Frame.GetFrame()->linesize[0], m_iWidth, m_iHeight);
		DrawRGB();
	}
	//不支持切换分辨率
	virtual void   DisplayI420(uint8_t* pY, int PitchY, uint8_t* pU, int PitchU, uint8_t* pV, int PitchV) {
		if (!IsWindowVisible(m_hWnd) || m_render == nullptr)
			return;
		WXAutoLock al(m_mutex);
		m_ptsLastDisplay = WXGetTimeMs();
		libyuv::I420Copy(pY, PitchY,
			pU, PitchU,
			pV, PitchV,
			m_I420Frame.GetFrame()->data[0], m_I420Frame.GetFrame()->linesize[0],
			m_I420Frame.GetFrame()->data[1], m_I420Frame.GetFrame()->linesize[1],
			m_I420Frame.GetFrame()->data[2], m_I420Frame.GetFrame()->linesize[2],
			m_iWidth, m_iHeight);

		DrawYUV();
	}

	virtual void   Display(AVFrame* videoFrame) {
		if (!IsWindowVisible(m_hWnd) || m_render == nullptr)
			return;
		WXAutoLock al(m_mutex);

		m_ptsLastDisplay = WXGetTimeMs();
		if (!videoFrame) {
			DrawYUV();//强制刷新
		}
		else {
			if (videoFrame->format == AV_PIX_FMT_YUV420P || videoFrame->format == AV_PIX_FMT_YUVJ420P) {
				//int nRotate   = m_iRotateType;
				m_fContrast = (s_nLut + 50.0f) / 50.0f;
				m_fBrightness = (s_nGray + 50.0f) / 50.0f;
				m_fSaturation = (s_nChrome + 50.0f) / 50.0f;
				if (m_iWidth == videoFrame->width && m_iHeight == videoFrame->height) { //正常绘制
					libyuv::I420Copy(videoFrame->data[0], videoFrame->linesize[0],
						videoFrame->data[1], videoFrame->linesize[1],
						videoFrame->data[2], videoFrame->linesize[2],
						m_I420Frame.GetFrame()->data[0], m_I420Frame.GetFrame()->linesize[0],
						m_I420Frame.GetFrame()->data[1], m_I420Frame.GetFrame()->linesize[1],
						m_I420Frame.GetFrame()->data[2], m_I420Frame.GetFrame()->linesize[2],
						m_iWidth, m_iHeight);
					m_I420Frame.GetFrame()->format = videoFrame->format;
					m_I420Frame.SetFormat((AVPixelFormat)videoFrame->format);
					DrawYUV();
				}
				else { //创新创建渲染
					if (m_bNeedReinit) { //重新配置分辨率
						WXLogA("--------  WXVideoRender ReOpen %dx%d ------", videoFrame->width, videoFrame->height);
						Open(TRUE, videoFrame->width, videoFrame->height);
						Display(videoFrame);
					}
					else { //直播模式，将数据拷贝到缓冲区处理

						int dx = 0;
						int dy = 0;
						GetXY(videoFrame->width, videoFrame->height, m_iWidth, m_iHeight, dx, dy);
						memset(m_I420Frame.GetFrame()->data[0], 0, m_I420Frame.GetFrame()->linesize[0] * m_iHeight);
						memset(m_I420Frame.GetFrame()->data[1], 128, m_I420Frame.GetFrame()->linesize[1] * m_iHeight / 2);
						memset(m_I420Frame.GetFrame()->data[2], 128, m_I420Frame.GetFrame()->linesize[2] * m_iHeight / 2);

						libyuv::I420Scale(videoFrame->data[0], videoFrame->linesize[0],
							videoFrame->data[1], videoFrame->linesize[1],
							videoFrame->data[2], videoFrame->linesize[2],
							videoFrame->width, videoFrame->height,
							m_I420Frame.GetFrame()->data[0] + dx + dy * m_I420Frame.GetFrame()->linesize[0],
							m_I420Frame.GetFrame()->linesize[0],
							m_I420Frame.GetFrame()->data[1] + dx / 2 + (dy / 2) * m_I420Frame.GetFrame()->linesize[1],
							m_I420Frame.GetFrame()->linesize[1],
							m_I420Frame.GetFrame()->data[2] + dx / 2 + (dy / 2) * m_I420Frame.GetFrame()->linesize[2],
							m_I420Frame.GetFrame()->linesize[2],
							m_iWidth - 2 * dx, m_iHeight - 2 * dy,
							libyuv::FilterMode::kFilterLinear);
						DrawYUV();
					}
				}
			}
			else if (videoFrame->format == AV_PIX_FMT_RGB32) {
				DisplayRGBA(videoFrame->data[0], videoFrame->linesize[0]);
			}

		}
	}


	void* m_playID = nullptr;
	void SetID(void* playID) {
		m_playID = playID;
	}

	void   SetFixed(int bFixed) {
		m_bFixed = !!bFixed;
	}

	//当width,height 有数值的时候使用直播模式
	//当width,height 为零的时候取消直播模式
	void SetBroadcastSize(int width, int height) {
		WXAutoLock al(m_mutex);
		if (width > 0 && height > 0) {
			Open(FALSE, width, height);
		}
		else {
			m_bNeedReinit = TRUE;
			Open(TRUE, m_iWidth, m_iHeight);	//撤销直播模式
		}
	}
public:
	int AddRef() {
		WXAutoLock al(m_mutex);
		m_iRef++;
		return m_iRef;
	}
	int ReleaseRef() {
		WXAutoLock al(m_mutex);
		m_iRef--;
		return m_iRef;
	}
};


static std::map<HWND, VideoRenderer*>s_mapRender;

//当width,height 有数值的时候使用直播模式
//当width,height 为零的时候取消直播模式
WXMEDIA_API void WXVideoRenderSetBroadcastSize(void* ptr, int width, int height) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		render->SetBroadcastSize(width, height);
	}
}

//bBroadcast 表示是否直播模式
WXMEDIA_API void* WXVideoRenderCreate2(HWND hWnd, int width, int height, int bBroadcast) {
	WXAutoLock al(s_lockRender);
	VideoRenderer* pVideoRender = nullptr;
	if (s_mapRender.count(hWnd) == 0) {
		pVideoRender = new VideoRenderer();
		pVideoRender->SetWindow(hWnd);
		//WXLogA("%s OK ---- new VideoRenderer", __FUNCTION__);
		s_mapRender[hWnd] = pVideoRender;
	}
	else {
		pVideoRender = s_mapRender[hWnd];
		//WXLogA("%s OK ---- in map", __FUNCTION__);
	}
	pVideoRender->AddRef();
	if (pVideoRender->Open(!bBroadcast, width, height)) {
		//WXLogA("%s OK Open", __FUNCTION__);
		return (void*)pVideoRender;
	}
	return nullptr;
}

//视频显示 C Styte Interface
//2019.12.12
WXMEDIA_API void* WXVideoRenderCreate(HWND hWnd, int width, int height) {
	return WXVideoRenderCreate2(hWnd, width, height, FALSE);
}

//2020.08.12
WXMEDIA_API void* WXVideoRenderCreateEx(HWND hWnd, int width, int height) {
	return WXVideoRenderCreate2(hWnd, width, height, TRUE);
}

WXMEDIA_API void WXVideoRenderDestroy(void* ptr) {
	WXAutoLock al(s_lockRender);
	//WXLogA("%s OK ---- AAAA ", __FUNCTION__);
	if (ptr) {
		VideoRenderer* pVideoRender = (VideoRenderer*)ptr;
		int ref = pVideoRender->ReleaseRef();
		if (ref == 0) {
			//WXLogA("%s OK ---- BBBB Remove VidoeRenderer", __FUNCTION__);
			HWND hWnd = pVideoRender->GetWindow();
			pVideoRender->Close();
			delete pVideoRender;
			s_mapRender.erase(hWnd);
		}
		else {
			///WXLogA("%s OK ---- CCCC Do not Remove VidoeRenderer  Ref=%d !!!!!!!!!!!!!!!!!", __FUNCTION__, ref);
		}
	}
}

//Add 2022.4.23
//管理到播放ID
WXMEDIA_API void WXVideoRenderSetID(void* ptr, void* playID) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		render->SetID(playID);
	}
}
//确保绘制出来的是播放ID的图像
WXMEDIA_API void WXVideoRenderDisplayEx(void* ptr, struct AVFrame* frame, int bFixed, int nRotateFilp, void* playID) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		if (playID == playID) {
			render->SetFixed(bFixed);
			render->SetRotate(nRotateFilp);
			render->Display(frame);
		}
	}
}

WXMEDIA_API void WXVideoRenderDisplay(void* ptr, struct AVFrame* frame, int bFixed, int nRotateFilp) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		render->SetFixed(bFixed);
		render->SetRotate(nRotateFilp);
		render->Display(frame);
	}
}

WXMEDIA_API void WXVideoRenderDisplayI420(void* ptr, uint8_t* pY, int PitchY, uint8_t* pU, int PitchU, uint8_t* pV, int PitchV, int bFixed, int nRotate) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		render->SetFixed(bFixed);
		render->SetRotate(nRotate);
		render->DisplayI420(pY, PitchY, pU, PitchU, pV, PitchV);
	}
}

WXMEDIA_API void WXVideoRenderDisplayI420Ex(void* ptr, uint8_t* pY, int PitchY, uint8_t* pU, int PitchU, uint8_t* pV, int PitchV, int bFixed, int nRotate, void* playID) {
	if (ptr) {
		VideoRenderer* render = (VideoRenderer*)ptr;
		if (playID == render->m_playID) {
			render->SetFixed(bFixed);
			render->SetRotate(nRotate);
			render->DisplayI420(pY, PitchY, pU, PitchU, pV, PitchV);
		}
	}
}


//默认会按机器性能分别初始化 D3DX D3D GDI
//如果机器异常，可以通过调用下面的接口来强制调用某个API
WXMEDIA_API void WXVideoRenderChangeMode(void* ptr, int mode) {

}

WXMEDIA_API void WXVideoRenderSetUseLut(void* ptr, int bLut) {

}

WXMEDIA_API void WXVideoRenderSetLutLevel(void* ptr, int level) {

}

WXMEDIA_API void WXVideoRenderSetYScale(void* ptr, int level) {

}

WXMEDIA_API void WXVideoRenderSetUVScale(void* ptr, int level) {

}

WXMEDIA_API void WXVideoRenderChangeModeForce(void* ptr, int mode) {

}

//Lut 渲染处理
WXMEDIA_API void WXMediaSetLut(int bLut) {

}

//设置Lut 数组
WXMEDIA_API void WXMediaSetLutTable(uint8_t* arrY) {

}

#endif