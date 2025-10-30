/*
* 基于WGC的桌面视频采集类
*/

#ifndef _WGC_CAPTURE_H_
#define _WGC_CAPTURE_H_

#include <WXMediaCpp.h>
#include "VideoSource.h"

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <cinttypes>
#include <dxgi.h>
#include <d3d11.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <Windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.System.h>
#include <wrl/client.h>

#include "WXBase.h"

#define IGCII     IGraphicsCaptureItemInterop
#define WF         winrt::Windows::Foundation
#define WG         winrt::Windows::Graphics 
#define WGC        WG::Capture
#define WGD        WG::DirectX
#define WGS        WG::SizeInt32
#define WGC_POOL   WGC::Direct3D11CaptureFramePool
#define WGC_GCI    WGC::GraphicsCaptureItem
#define WGC_Device WGD::Direct3D11::IDirect3DDevice
#define WGC_FORMAT WGD::DirectXPixelFormat::B8G8R8A8UIntNormalized

#define POOL_NUM 2
#define RESET_INTERVAL_SEC 3.0f
#define VENDOR_ID_INTEL 0x8086
#define IGPU_MEM (512 * 1024 * 1024)
static const IID dxgiFactory2 = { 0x50c83a1c, 0xe072, 0x4c48,{0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36, 0xa6, 0xd0} };

struct wgc_monitor_info {
	int cur_id;
	int desired_id;
	int id;
	RECT rect;
	HMONITOR handle;
};

static BOOL CALLBACK enum_monitor(HMONITOR handle, HDC hdc, LPRECT rect, LPARAM param) {
	struct wgc_monitor_info* monitor = (struct wgc_monitor_info*)param;
	if (monitor->cur_id == 0 || monitor->desired_id == monitor->cur_id) {
		monitor->id = monitor->cur_id;
		monitor->rect = *rect;
		monitor->handle = handle;
	}
	return (monitor->desired_id > monitor->cur_id++);
}

struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
	IDirect3DDxgiInterfaceAccess : IUnknown {
	virtual HRESULT __stdcall GetInterface(GUID const& id,
		void** object) = 0;
};

template<class T> class ComPtr {
protected:
	T* ptr;
	inline void Kill()
	{
		if (ptr)
			ptr->Release();
	}

	inline void Replace(T* p)
	{
		if (ptr != p) {
			if (p)
				p->AddRef();
			if (ptr)
				ptr->Release();
			ptr = p;
		}
	}

public:
	inline ComPtr() : ptr(nullptr) {}
	inline ComPtr(T* p) : ptr(p) {
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(const ComPtr<T>& c) : ptr(c.ptr) {
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(ComPtr<T>&& c) : ptr(c.ptr) { c.ptr = nullptr; }
	inline ~ComPtr() { Kill(); }
	inline void Clear() {
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}
	inline ComPtr<T>& operator=(T* p) {
		Replace(p);
		return *this;
	}

	inline ComPtr<T>& operator=(const ComPtr<T>& c) {
		Replace(c.ptr);
		return *this;
	}

	inline ComPtr<T>& operator=(ComPtr<T>&& c) {
		if (&ptr != &c.ptr) {
			Kill();
			ptr = c.ptr;
			c.ptr = nullptr;
		}
		return *this;
	}

	inline T* Detach() {
		T* out = ptr;
		ptr = nullptr;
		return out;
	}

	inline void CopyTo(T** out) {
		if (out) {
			if (ptr)
				ptr->AddRef();
			*out = ptr;
		}
	}

	inline ULONG Release() {
		ULONG ref;
		if (!ptr)
			return 0;
		ref = ptr->Release();
		ptr = nullptr;
		return ref;
	}

	inline T** Assign() {
		Clear();
		return &ptr;
	}
	inline void Set(T* p) {
		Kill();
		ptr = p;
	}

	inline T* Get() const { return ptr; }

	inline T** operator&() { return Assign(); }

	inline operator T* () const { return ptr; }
	inline T* operator->() const { return ptr; }

	inline bool operator==(T* p) const { return ptr == p; }
	inline bool operator!=(T* p) const { return ptr != p; }

	inline bool operator!() const { return !ptr; }
};

template<class T> class ComQIPtr : public ComPtr<T> {

public:
	inline ComQIPtr(IUnknown* unk)
	{
		this->ptr = nullptr;
		unk->QueryInterface(__uuidof(T), (void**)&this->ptr);
	}

	inline ComPtr<T>& operator=(IUnknown* unk)
	{
		ComPtr<T>::Clear();
		unk->QueryInterface(__uuidof(T), (void**)&this->ptr);
		return *this;
	}
};

template<typename T>
static winrt::com_ptr<T> GetDXGIInterfaceFromObject(
	WF::IInspectable const& object)
{
	auto access = object.as<IDirect3DDxgiInterfaceAccess>();
	winrt::com_ptr<T> result;
	winrt::check_hresult(
		access->GetInterface(winrt::guid_of<T>(), result.put_void()));
	return result;
}

static WGC_GCI create_item(IGCII* const interop_factory, HMONITOR monitor) {
	WGC_GCI item = { nullptr };
	try {
		const HRESULT hr = interop_factory->CreateForMonitor(
			monitor,
			winrt::guid_of<winrt::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
			reinterpret_cast<void**>(
				winrt::put_abi(item)));
	}
	catch (...) {}
	return item;
}

static bool cursor_toggle_supported() {
	try {
		return WF::Metadata::ApiInformation::
			IsPropertyPresent(
				L"Windows.Graphics.Capture.GraphicsCaptureSession",
				L"IsCursorCaptureEnabled");
	}
	catch (...) {
		return false;
	}
}

class WgcCapture: public VideoSource{
public:

	bool  m_bInit = false;
	ComPtr<IDXGIFactory1> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;
	ComPtr<ID3D11Device>  m_d3d11Device;
	ComPtr<ID3D11DeviceContext> m_context;
	uint32_t m_adpIdx = 0;
	ComPtr<ID3D11Texture2D>m_texture = nullptr;
	BOOL m_capture_cursor = TRUE;

	WGC_GCI     m_item{ nullptr };
	WGC_Device  m_device = nullptr;
	WGC_POOL    m_frame_pool{ nullptr };
	WGC::GraphicsCaptureSession m_session{ nullptr };
	WGS  m_ImageSize{ 0,0 };
	WGC_GCI::Closed_revoker m_closed;
	WGC_POOL::FrameArrived_revoker m_frame_arrived;

	bool increase_maximum_frame_latency();

	bool InitDevice(uint32_t& adapterIdx);

	void on_closed(WGC_GCI const&, WF::IInspectable const&);

	void CreateTexture(int width, int height);

	//采集回调函数
	void on_frame_arrived(WGC_POOL const& sender, WF::IInspectable const&);

	int  InitImpl();


	int m_bInitWGC = -1;
	int m_bFlag = FALSE;
public:

	virtual void ThreadProcess(void) {

	}
	virtual int      Init();//
	virtual void     Start();//启动
	virtual void     Stop();     //结束
	virtual int      GrabFrameImpl(WXVideoFrame* avframe); //获取数据,返回1成功，返回0失败
	virtual WXCTSTR  Type();  //类型
};

#endif //_WX_WINDOWS_GDI_CAPTURE_H_

