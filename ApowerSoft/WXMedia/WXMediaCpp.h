/*
内部编译使用的头文件
*/
#ifndef  _WXMEDIA_CPP_H_
#define  _WXMEDIA_CPP_H_

#include <WXBase.h>
#include <WXMedia.h>
#include <FfmpegIncludes.h>      // ffmpeg 头文件
#include <libyuv.h>
#include <WXLog.h>

#define DBG_GET_REFCOUNT(spUnk)   (spUnk ? (spUnk.p->AddRef(), spUnk.p->Release() - 1) : 0)


#define MAIN_WORK_THREAD 0
#define OTHER_WORK_THREAD 1
WXMEDIA_API void WXTaskPost(int index, WXTask task);

#define AUDIO_SAMPLE_RATE  48000 //音频采集混音使用的采样频率
#define AUDIO_CHANNELS     2     //音频采样频率
#define AUDIO_FRAME_SIZE   1920  //对应10ms  S16 数据量
#define AUDIO_AAC_SAMPLE   1024

#ifdef _WIN32



//DXGI头文件
#include <dxgi.h>
#include <dxgi1_2.h>

//D3D头文件
#include <d3d9.h>
#include <mfidl.h>

//WASAPI头文件
#include <Avrt.h>
#include <MMSystem.h>
#include <initguid.h>
#include <MMDeviceapi.h>
#include <AudioClient.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <EndpointVolume.h> 

#include "LibInst.hpp"

//GDI+头文件
#include <gdiplus.h>

static std::wstring HrToString(HRESULT hr) {
	WCHAR buffer[1024];
	memset(buffer, 0, 1024 * sizeof(WCHAR));
	::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buffer, 1024, nullptr);
	std::wstring wstr = buffer;
	return buffer;
}

inline bool __fastcall PropVariantIsEmpty(const PROPVARIANT& pv)
{
	return ((unsigned short)pv.vt <= VT_NULL);	// VT_EMPTY or VT_NULL
}

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)

#define WXMEDIA_WNDCLASS L"WXWndClass"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)  if(p){p->Release();p=NULL;}
#endif

#ifndef HR_RETURN_NULL
#define HR_RETURN_NULL(hr)  {if(FAILED(hr))return NULL;}
#endif

#ifndef HR_RETURN_0
#define HR_RETURN_0(hr)  {if(FAILED(hr))return 0;}
#endif

#define FAILED_RETURN2(hr, p, str)    if(FAILED(hr) || NULL == p){ WXLogW(L"\t[%ws] %ws=%08x",__FUNCTIONW__, str, hr);return;}

#define FAILED_CONTINUE1(hr, str)     if(FAILED(hr)){ WXLogW(L"\t[%ws] %ws=%08x",__FUNCTIONW__, str, hr);return;}
#define FAILED_CONTINUE2(hr, p, str)  if(FAILED(hr) || NULL == p){WXLogW(L"\t[%ws] %ws=%08x",__FUNCTIONW__, str, hr);return;}

EXTERN_C const IID   IID_ISampleGrabber;
EXTERN_C const IID   IID_ISampleGrabberCB;
EXTERN_C const CLSID CLSID_NullRenderer;
EXTERN_C const CLSID CLSID_SampleGrabber;

struct ISampleGrabberCB;
MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown{
public:
	virtual HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime,IMediaSample *pSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime,BYTE *pBuffer,long BufferLen) = 0;
};

struct ISampleGrabber;
MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(BOOL OneShot) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetMediaType(const AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(AM_MEDIA_TYPE *pType) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(BOOL BufferThem) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(long *pBufferSize, long *pBuffer) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(IMediaSample **ppSample) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetCallback(ISampleGrabberCB *pCallback,long WhichMethodToCallback) = 0;
};

#define MOUSE_TIME 300
//游戏录制
void      WXGameStartRecord(int nFps);//改变fps
void      WXGameStopRecord();

extern WXLocker s_lockGlobal;



#endif  //_WIN32


class RgbaData {
public:
	static void CopyRect(const AVFrame* srcFrame, const int PosX, const int PosY, AVFrame* dstFrame);

	//RGB32鼠标绘制
	static void DrawMouseRGBA(AVFrame* MixFrame,
		const int PosX,
		const int PosY,
		const AVFrame* AlphaFrame);


	//区域叠加，一般用于，鼠标叠加
	static void DrawMouseMonoMask(AVFrame* MixFrame,
		const int PosX,
		const int PosY,
		const uint8_t* pAndMask,
		const uint8_t* pXorMask,
		int nW,int nH);

	//区域叠加，一般用于，鼠标叠加
	static void DrawMouseMask(AVFrame* MixFrame,
		const int PosX,
		const int PosY,
		const int* pMask,
		int nW, int nH);

	//区域叠加，一般用于，鼠标叠加
	//Alpha 0-100
	static void MixRect(AVFrame* MixFrame,
		const int PosX,
		const int PosY,
		const int nAlpha,
		const AVFrame* AlphaFrame);

	static void DrawPicture(AVFrame* MixFrame, const AVFrame* AlphaFrame);
};

class IWXVideRenderImpl {
public:
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
public:
	virtual  int    Open(HWND hwnd, int width, int height) = 0;
	virtual  int    Draw(AVFrame *frame, int bFixed, int nRotateFlip, float fBrightness, float fSaturation, float fContrast) = 0;
	virtual  void   Close() = 0;
};




WXCTSTR WXCaptureGetWM();
int     WXCaptureGetWMPosX();
int     WXCaptureGetWMPosY();


class   IWasapiDevice {
public:
	virtual int      Open(int bSystem, int bComm) = 0;
	virtual void     Close() = 0;
	virtual void    ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) = 0;//默认设备改变！
};
void WXWasapiNotifyInit();
void WXWasapiNotifyAddDevice(IWasapiDevice* device);
void WXWasapiNotifyRemoveDevice(IWasapiDevice* device);

extern int g_bWXCaptureStopFlag /*= FALSE*/;

class CMMNotificationClient : public IMMNotificationClient
{
public:
	WXLocker m_mutex;

	CComPtr<IMMDeviceEnumerator>m_pEnum = NULL;
	std::map<uintptr_t, IWasapiDevice*>m_mapDevice;

	static CMMNotificationClient& GetInst() {
		static CMMNotificationClient g_WasapiNotify;
		return g_WasapiNotify;
	}
	static IMMDeviceEnumerator* GetEnum() {
		return GetInst().m_pEnum.p;
	}

	void AddDevice(IWasapiDevice* device) {
		WXAutoLock al(m_mutex);
		Register();
		uintptr_t uid = (uintptr_t)(void*)device;
		m_mapDevice[uid] = device;
	}

	void RemoveDevice(IWasapiDevice* device) {
		WXAutoLock al(m_mutex);
		Register();
		uintptr_t uid = (uintptr_t)(void*)device;
		m_mapDevice.erase(uid);
	}

	void Register() {
		WXAutoLock al(m_mutex);
		if (m_pEnum == nullptr) {
			HRESULT hr = m_pEnum.CoCreateInstance(__uuidof(MMDeviceEnumerator));
			if (FAILED(hr))
			{
				WXLogW(L"m_pEnum.CoCreateInstance(__uuidof(MMDeviceEnumerator)) Error");
				m_pEnum = NULL;
				return;
			}

			int vv1 = DBG_GET_REFCOUNT(m_pEnum);


			//注册事件  
			hr = m_pEnum->RegisterEndpointNotificationCallback((IMMNotificationClient*)this);
			if (FAILED(hr))
			{
				WXLogW(L"m_pEnum.RegisterEndpointNotificationCallback(this)) Error");
				m_pEnum = NULL;
				return;
			}
		}
	}

	virtual ~CMMNotificationClient()
	{
		m_pEnum = nullptr;
	}
private:

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) {
		return S_OK;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void) {
		return S_OK;
	}

	virtual ULONG STDMETHODCALLTYPE Release(void) {
		return S_OK;
	}

	//默认设备改变
	HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(
		EDataFlow flow, ERole role,
		LPCWSTR pwstrDeviceId)
	{
		//WXLogW(L"OnDefaultDeviceChanged flow=%d role=%d  [pwstrDeviceId=%ws]",(int)flow, (int)role, pwstrDeviceId);
		//默认设备改变
		WXAutoLock al(m_mutex);
		for (auto obj : m_mapDevice) {
			if (obj.second) {
				obj.second->ChangeDefaultDevice(flow, role, pwstrDeviceId);//默认设备改变
			}
		}
		//回调或者通知。。
		return S_OK;
	}
	//添加设备
	HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId)
	{
		return S_OK;
	};
	//移除设备
	HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(
		LPCWSTR pwstrDeviceId,
		DWORD dwNewState)
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(
		LPCWSTR pwstrDeviceId,
		const PROPERTYKEY key)
	{
		return S_OK;
	}
};

#endif
