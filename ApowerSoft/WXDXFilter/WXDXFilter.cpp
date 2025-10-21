#include <d3dx9/d3dx9.h>
#include <d3d9.h>
#include <atlbase.h>
#include <mutex>
#include <libyuv.h>

//#pragma comment(lib,"libyuv.lib")

#define WXLocker     std::recursive_mutex
#define WXAutoLock   std::lock_guard<WXLocker>

EXTERN_C void  WXLogA(const char* format, ...) {
	static char    szMsg[4096];
	memset(szMsg, 0, 4096);
	va_list marker = nullptr;
	va_start(marker, format);
	vsprintf(szMsg, format, marker);
	va_end(marker);
#ifdef _DEBUG
	FILE* fout = fopen("D:\\my.log.txt", "ab+");
	if (fout) {
		fwrite(szMsg, strlen(szMsg), 1, fout);
		fclose(fout);
	}
#else
	::MessageBoxA(NULL, szMsg, "WXDXFilter.dll", MB_OK);
#endif

}

class WXVideoFrame
{
public:
	uint8_t* data[4] = { nullptr };
	int linesize[4] = { 0 };
	int width = 0;
	int height = 0;
public:
	WXVideoFrame() {
	}
	virtual ~WXVideoFrame() {
		for (size_t i = 0; i < 4; i++)
		{
			if (this->data[i]) {
				free(this->data[i]);
				this->data[i] = nullptr;
			}
			this->linesize[i] = 0;
		}
	}
	void Reset(int _width, int _height) {
		if (_width == width && _height == height)
			return;

		for (size_t i = 0; i < 4; i++)
		{
			if (this->data[i]) {
				free(this->data[i]);
				this->data[i] = nullptr;
			}
			this->linesize[i] = 0;
		}
		this->width = _width;
		this->height = _height;
		int pitch = (width + 31) / 32 * 32;
		this->linesize[0] = pitch;
		this->data[0] = (uint8_t*)malloc(this->linesize[0] * this->height);

		this->linesize[1] = pitch / 2;
		this->data[1] = (uint8_t*)malloc(this->linesize[1] * this->height / 2);

		this->linesize[2] = pitch / 2;
		this->data[2] = (uint8_t*)malloc(this->linesize[2] * this->height / 2);
	}
};


//d3d9.dll
typedef IDirect3D9* (WINAPI* cbDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT(WINAPI* cbDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex**);

typedef HRESULT(WINAPI* cbD3DXCompileShader)(
	LPCSTR                          pSrcData,
	UINT                            SrcDataLen,
	CONST D3DXMACRO* pDefines,
	LPD3DXINCLUDE                   pInclude,
	LPCSTR                          pFunctionName,
	LPCSTR                          pProfile,
	DWORD                           Flags,
	LPD3DXBUFFER* ppShader,
	LPD3DXBUFFER* ppErrorMsgs,
	LPD3DXCONSTANTTABLE* ppConstantTable);

typedef HRESULT(WINAPI* cbD3DXCreateFontIndirectW)(
	LPDIRECT3DDEVICE9       pDevice,
	CONST D3DXFONT_DESCW* pDesc,
	LPD3DXFONT* ppFont);

class D3dxRender {
	HMODULE m_libD3D9 = nullptr;
	cbDirect3DCreate9 mDirect3DCreate9 = nullptr;
	cbDirect3DCreate9Ex mDirect3DCreate9Ex = nullptr;

	HMODULE m_libD3DX9 = nullptr;
	cbD3DXCompileShader mD3DXCompileShader = nullptr;
	cbD3DXCreateFontIndirectW mD3DXCreateFontIndirectW = nullptr;
public:
	D3dxRender() {

		::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM ��ʼ��

		m_libD3D9 = LoadLibraryA("d3d9.dll");
		if (m_libD3D9) {
			mDirect3DCreate9 = (cbDirect3DCreate9)GetProcAddress(m_libD3D9, "Direct3DCreate9");
			mDirect3DCreate9Ex = (cbDirect3DCreate9Ex)GetProcAddress(m_libD3D9, "Direct3DCreate9Ex");

			//��Դ��WXDXFilter.dll  ���Կ����
			//��� Direct3DCreate9Ex �޷���������
			//Ӳ����Ӳ�����Լ�DXGI���ܿ��ܶ��������� 
			//��Ҫ�����Կ�����
			CComPtr<IDirect3D9Ex> pD3DEx = nullptr;
			HRESULT hr = mDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
			if (FAILED(hr)) {
				mDirect3DCreate9 = nullptr;
				mDirect3DCreate9Ex = nullptr;
				FreeLibrary(m_libD3D9);
				m_libD3D9 = nullptr;
			}
		}

		if (m_libD3D9) { //D3DX9 ������D3D9����
			m_libD3DX9 = LoadLibraryA("d3dx9_43.dll");
			if (m_libD3DX9) {
				mD3DXCompileShader = (cbD3DXCompileShader)GetProcAddress(m_libD3DX9, "D3DXCompileShader");
				mD3DXCreateFontIndirectW = (cbD3DXCreateFontIndirectW)GetProcAddress(m_libD3DX9, "D3DXCreateFontIndirectW");
			}
		}
	}
	virtual ~D3dxRender() {
		Close();
		if (m_libD3DX9) {
			FreeLibrary(m_libD3DX9);
			m_libD3DX9 = nullptr;
			mD3DXCompileShader = nullptr;
			mD3DXCreateFontIndirectW = nullptr;
		}		
		if (m_libD3D9) {
			FreeLibrary(m_libD3D9);
			m_libD3D9 = nullptr;
			mDirect3DCreate9 = nullptr;
			mDirect3DCreate9Ex = nullptr;
		}
		exit(-1);
	}
private:
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
	CComPtr<ID3DXConstantTable> m_pCTable = 0;//��shader�����ݽ���
	D3DXHANDLE m_pHBrightness = 0; //����
	D3DXHANDLE m_pHSaturation = 0; //���Ͷ�
	D3DXHANDLE m_pHContrast = 0;  //�Աȶ�
	float  m_fBrightness = 1.0f;//����
	float  m_fSaturation = 1.0f;//���Ͷ�
	float  m_fContrast = 1.0f;//�Աȶ�


	bool m_bOpen = false;

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
		//��ʾ������
		D3DDISPLAYMODE d3ddm;
		ZeroMemory(&d3ddm, sizeof(d3ddm));
		hr = m_pD3D->GetAdapterDisplayMode(index, &d3ddm);
		if (FAILED(hr)) {
			WXLogA("D3D  GetAdapterDisplayMode error");
			return false;
		}
		//if (d3ddm.Format != D3DFMT_X8R8G8B8) {
		//	WXLogW(L"GetAdapterDisplayMode Format error");
		//	return false;
		//}

		//���ڴ�С
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
			WXLogA("m_pD3D->CreateDeviceEx error");
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

		m_frame.Reset(m_iWidth, m_iHeight);
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
		hr = mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_3_0",
			D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
			&pShader, &pErrorBuffer, &m_pCTable);

		if (FAILED(hr)) {
			pShader = nullptr;
			pErrorBuffer = nullptr;
			m_pCTable = nullptr;
			hr = mD3DXCompileShader(strShader_yuv2rgb, strlen(strShader_yuv2rgb), 0, 0, "Main", "ps_2_0",
				D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY,
				&pShader, &pErrorBuffer, &m_pCTable);
		}
		if (SUCCEEDED(hr)) {
			hr = m_pDev->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), &m_pShader);
		}
		if (SUCCEEDED(hr)) {
			m_pHBrightness = m_pCTable->GetConstantByName(0, "Brightness");//����
			m_pHSaturation = m_pCTable->GetConstantByName(0, "Saturation");//���Ͷ�
			m_pHContrast = m_pCTable->GetConstantByName(0, "Contrast");//�Աȶ�
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
	//�������ȡ����Ͷȡ��Աȶ�
	void  SetLut(int nBrightness, int nSaturation, int nContrast) {
		WXAutoLock al(m_mutex);
		m_fBrightness = nBrightness / 100.0f;
		m_fSaturation = nSaturation / 100.0f;
		m_fContrast = nContrast / 100.0f;
	}

#define CHECK_HR(hr)  if (FAILED(hr)) {m_pDev->EndScene();return nullptr;}

	//��ȡD3DSurface
	void* GetSurface() { //��Ⱦ��D3DSurface
		WXAutoLock al(m_mutex);
		return m_pSurface;
	}


	void I420Copy(BYTE* pSrcY, int pitchSrcY,
		BYTE* pSrcU,   int pitchSrcU,
		BYTE* pSrcV,  int pitchSrcV,
		BYTE* pDstY, int pitchDstY,
		BYTE* pDstU, int pitchDstU,
		BYTE* pDstV, int pitchDstV,
		int width, int height) {

		for (size_t i = 0; i < height; i++)
		{
			memcpy(pDstY + i * pitchDstY, pSrcY+ i * pitchSrcY, width);
		}
		for (size_t i = 0; i < height/2; i++)
		{
			memcpy(pDstU + i * pitchDstU, pSrcU + i * pitchSrcU, width/2);
		}
		for (size_t i = 0; i < height/2; i++)
		{
			memcpy(pDstV + i * pitchDstV, pSrcV + i * pitchSrcV, width/2);
		}
	}

	int RenderTexture() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			if (m_dstWidth != m_txtWidth || m_dstHeight != m_txtHeight) {
				CreateSurface();
				m_txtWidth = m_dstWidth;
				m_txtHeight = m_dstHeight;
			}

			HRESULT hr = S_OK;
			hr = m_pDev->BeginScene();
			CComPtr<IDirect3DSurface9> pBackBuffer = nullptr;
			hr = m_pDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);

			hr = m_pDev->SetRenderTarget(0, pBackBuffer);
				hr = m_pDev->Clear(0, nullptr, D3DCLEAR_TARGET, 0/*m_bkColor*/, 1.0f, 0);


				if (m_bNewImage.load()) { //�»������ݼ��ص�YUV Texture
					m_bNewImage.store(false);
					D3DLOCKED_RECT d3d_rect[INDEX_NUM];
					hr = m_pTex[INDEX_Y]->LockRect(0, &d3d_rect[INDEX_Y], 0, 0);
					
						hr = m_pTex[INDEX_U]->LockRect(0, &d3d_rect[INDEX_U], 0, 0);
					
						hr = m_pTex[INDEX_V]->LockRect(0, &d3d_rect[INDEX_V], 0, 0);
					
						I420Copy(
							m_frame.data[0], m_frame.linesize[0],
							m_frame.data[1], m_frame.linesize[1],
							m_frame.data[2], m_frame.linesize[2],
							(uint8_t*)d3d_rect[INDEX_Y].pBits, d3d_rect[INDEX_Y].Pitch,
							(uint8_t*)d3d_rect[INDEX_U].pBits, d3d_rect[INDEX_U].Pitch,
							(uint8_t*)d3d_rect[INDEX_V].pBits, d3d_rect[INDEX_V].Pitch,
							m_txtWidth, m_txtHeight
						);

					hr = m_pTex[INDEX_Y]->UnlockRect(0);
						hr = m_pTex[INDEX_U]->UnlockRect(0);
						hr = m_pTex[INDEX_V]->UnlockRect(0);
				}

			hr = m_pCTable->SetFloat(m_pDev, m_pHBrightness, m_fBrightness);
				hr = m_pCTable->SetFloat(m_pDev, m_pHSaturation, m_fSaturation);
				hr = m_pCTable->SetFloat(m_pDev, m_pHContrast, m_fContrast);
				hr = m_pDev->SetPixelShader(m_pShader);
				hr = m_pDev->SetTexture(INDEX_Y, m_pTex[INDEX_Y]);
				hr = m_pDev->SetTexture(INDEX_U, m_pTex[INDEX_U]);
				hr = m_pDev->SetTexture(INDEX_V, m_pTex[INDEX_V]);
				hr = m_pDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				hr = m_pDev->SetRenderState(D3DRS_LIGHTING, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_ZENABLE, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
				hr = m_pDev->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				hr = m_pDev->SetSamplerState(INDEX_Y, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				hr = m_pDev->SetSamplerState(INDEX_U, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				hr = m_pDev->SetSamplerState(INDEX_V, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				hr = m_pDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX3);
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

				hr = m_pDev->SetTexture(INDEX_Y, nullptr);
				hr = m_pDev->SetTexture(INDEX_U, nullptr);
				hr = m_pDev->SetTexture(INDEX_V, nullptr);
				hr = m_pDev->SetPixelShader(nullptr);
				RECT rect{
					0, 0, m_txtWidth, m_txtHeight
			};
			hr = m_pDev->StretchRect(pBackBuffer, &rect, m_pSurface, &rect, D3DTEXF_LINEAR);//��D3DX Shader���������浽���Surface
			hr = m_pDev->EndScene();
			return S_OK;
		}
		return E_FAIL;
	}

	//���YUV420����
	//��������л���Ƶʱ��ԭ��Ƶ�߳�û�н���������ID�Ѿ��ı䣬���ʱ����޷����л�����
	int UploadTexture(BYTE* bufy, BYTE* bufu, BYTE* bufv, int pitch, int pitchuv, int width, int height) { //�ڴ濽��
		WXAutoLock al(m_mutex);
		//if (m_playID.load() != uid)
		//	return 0; //ID ���ı���

		if (m_bOpen) {

			//ĳЩͼ����ܿ�Ȼ��߸߶ȳ���BufferSize
			//��Ҫ���Ŵ���
			if (width <= m_iWidth && height <= m_iHeight) {
				//ֱ��copy
				I420Copy(
					bufy, pitch,
					bufu, pitchuv,
					bufv, pitchuv,
					m_frame.data[0], m_frame.linesize[0],
					m_frame.data[1], m_frame.linesize[1],
					m_frame.data[2], m_frame.linesize[2],
					width, height
				);
				m_dstWidth = width;
				m_dstHeight = height;
			}
			else {
				//Scale
				int dw = (m_iHeight * width / height) / 2 * 2;//���ŵ���ǰ�߶�
				int dh = (m_iWidth * height / width) / 2 * 2;//���ŵ���ǰ���
				if (dw > m_iWidth) { //������ǰ���
					m_dstWidth = m_iWidth;
					m_dstHeight = dh;
				}
				else {
					m_dstWidth = dw;
					m_dstHeight = m_iHeight;
				}

				libyuv::I420Scale(
					bufy, pitch,
					bufu, pitchuv,
					bufv, pitchuv,
					width, height,
					m_frame.data[0], m_frame.linesize[0],
					m_frame.data[1], m_frame.linesize[1],
					m_frame.data[2], m_frame.linesize[2],
					m_dstWidth, m_dstHeight,
					libyuv::FilterMode::kFilterBilinear
				);
			}
			m_bNewImage.store(true);
			return 1;
		}

		return 0;
	}

	//�������Ŷ���
	bool  Open(HWND hwnd) {
		WXAutoLock al(m_mutex);
		if (m_libD3D9 == nullptr || m_libD3DX9 == nullptr)
			return false;

		m_pD3D = nullptr;
		mDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
		if (!m_pD3D) {
			WXLogA("Direct3DCreate9Ex error");
			return false;
		}
		m_hWnd = hwnd;// ? hwnd : GetDesktopWindow();
		int nCount = m_pD3D->GetAdapterCount();
		for (int index = 0; index < nCount; index++) { //���Կ�����
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

	//�ر�
	void  Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			m_pTex[INDEX_Y] = nullptr;
			m_pTex[INDEX_U] = nullptr;
			m_pTex[INDEX_V] = nullptr;
			m_pTexture = nullptr;
			m_pSurface = nullptr;
			m_pShader = nullptr;
			m_pD3D = nullptr;
			//m_pDev = nullptr;
			m_bOpen = false;
		}
	}

	//��Ⱦ���������������MFC����
	void  Render() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			RECT rect{ 0, 0, m_txtWidth, m_txtHeight };
			RECT rcDst;
			::GetClientRect(m_hWnd, &rcDst);
			HRESULT hr = m_pDev->Present(&rect, &rcDst, nullptr, nullptr);//��Ⱦ������

			if (hr == D3DERR_DEVICELOST) {
				hr = m_pDev->TestCooperativeLevel();
				if (SUCCEEDED(hr) || hr == D3DERR_DEVICENOTRESET) {
					//Reset
					m_pDev->Reset(&m_d3dpp);
				}
			}
		}
	}
	static D3dxRender& GetInst() {
		static D3dxRender s_inst;
		return s_inst;
	}
};

EXTERN_C int SetPreviewSize(int width, int height)
{
	return S_OK;
}

EXTERN_C __declspec(dllexport) int InitFilter(HWND hwnd)
{
	bool bRet = D3dxRender::GetInst().Open(hwnd);
	if (bRet) {
		return S_OK;
	}
	return -1;
}

EXTERN_C __declspec(dllexport) int InitDevice(HWND hwnd, void** _Device)
{
	return S_OK;
}

EXTERN_C __declspec(dllexport) void* GetFrontSurface()
{
	return D3dxRender::GetInst().GetSurface();
}

EXTERN_C __declspec(dllexport) int ResetFilterParamenter()
{
	return S_OK;
}


EXTERN_C __declspec(dllexport) int UploadTexture(BYTE* bufy, BYTE* bufu, BYTE* bufv, const char* name, int pitch, int pitchuv, int width, int height, float posx, float posy)
{
	return D3dxRender::GetInst().UploadTexture(bufy, bufu, bufv, pitch, pitchuv, width, height);
}

EXTERN_C __declspec(dllexport) int RenderTexture(int width, int height) {
	return D3dxRender::GetInst().RenderTexture();
}

//��֧��YUV����D3DImage ���Ƶ�Bitmap�ڴ�
static WXVideoFrame s_yuvframe;
EXTERN_C int RenderFrameToBitmap(UINT8* yuv_buffer, int framewidth, int frameheight, UINT8* bitmap, int bitmapwidth, int bitmapheight, int pixelformat)
{
	const uint8_t* srcY = yuv_buffer;
	const uint8_t* srcU = yuv_buffer + framewidth * frameheight;
	const uint8_t* srcV = yuv_buffer + framewidth * frameheight + framewidth / 2 * frameheight / 2;

	if (framewidth == bitmapwidth && frameheight == bitmapheight)
	{
		libyuv::I420ToARGB(srcY, framewidth,
			srcU, framewidth / 2,
			srcV, framewidth / 2,
			bitmap, framewidth * 4,
			framewidth, frameheight);
	}
	else
	{

		s_yuvframe.Reset(bitmapwidth, bitmapheight);

		libyuv::I420Scale(srcY, framewidth,
			srcU, framewidth / 2,
			srcV, framewidth / 2,
			framewidth, frameheight,
			(uint8_t*)s_yuvframe.data[0], s_yuvframe.linesize[0],
			(uint8_t*)s_yuvframe.data[1], s_yuvframe.linesize[1],
			(uint8_t*)s_yuvframe.data[2], s_yuvframe.linesize[2],
			bitmapwidth, bitmapheight, libyuv::FilterMode::kFilterLinear
		);

		libyuv::I420ToARGB(
			(uint8_t*)s_yuvframe.data[0], s_yuvframe.linesize[0],
			(uint8_t*)s_yuvframe.data[1], s_yuvframe.linesize[1],
			(uint8_t*)s_yuvframe.data[2], s_yuvframe.linesize[2],
			bitmap, bitmapwidth * 4,
			bitmapwidth, bitmapheight);
	}

	return S_OK;
}

EXTERN_C __declspec(dllexport) int UploadFile2Texture(const char* filename, const char* texturename)
{
	//WXLogA("+++%s %s", __FUNCTION__, filename);

	return S_OK;
}

EXTERN_C __declspec(dllexport) int UploadRGBATexture(BYTE* buf, const char* name, int pitch, int width, int height, float posx, float posy)
{
	//WXLogA("+++%s [%s]", __FUNCTION__, name);
	return S_OK;
}

struct FilterFrame;
EXTERN_C __declspec(dllexport) int UploadRGBATextures(struct FilterFrame* frame, int length)
{
	return S_OK;
}

EXTERN_C __declspec(dllexport) int ForceRefresh() {

	WXLogA("%s", __FUNCTION__);

	return S_OK;
}

typedef struct ChromaKeyContext
{
	int color; //0x000000~0xffffff	�û�ѡ�����ɫֵ RGB��ʽ
	int darkColor;// �Ƚϰ��ı���ɫ
	float _Threshhold;// �ٳ��̶�
	float _BlurSize;// ��Եģ���̶�

}ChromaKeyContext;
EXTERN_C __declspec(dllexport) int SetChromaParamenter(ChromaKeyContext context)
{
	return S_OK;
}
//EXTERN_C int SetColorAdjustParamenter(ColorAdjustContext context)
//{
//	WXLogA("%s", __FUNCTION__);
//
//	return S_OK;
//}
//EXTERN_C int SetImageEnhanceParamenter(ImageEnhanceContext context)
//{
//	WXLogA("%s", __FUNCTION__);
//
//	return S_OK;
//}
EXTERN_C __declspec(dllexport) int SetParameter(float Saturation, float Hue, float Brightness, float Contrast, float Hightlights, float Shadows)
{
	return S_OK;
}
//EXTERN_C int SetPreviewParameter(PreviewContext context)
//{
//	WXLogA("%s", __FUNCTION__);
//
//	return S_OK;
//}