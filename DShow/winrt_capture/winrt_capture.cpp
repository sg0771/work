#include "winrt_capture.h"

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

#define POOL_NUM 1
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

static BOOL CALLBACK enum_monitor(HMONITOR handle, HDC hdc, LPRECT rect, LPARAM param){
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
	inline ComPtr(T* p) : ptr(p){
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(const ComPtr<T>& c) : ptr(c.ptr){
		if (ptr)
			ptr->AddRef();
	}
	inline ComPtr(ComPtr<T>&& c) : ptr(c.ptr) { c.ptr = nullptr; }
	inline ~ComPtr() { Kill(); }
	inline void Clear(){
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}
	inline ComPtr<T>& operator=(T* p){
		Replace(p);
		return *this;
	}

	inline ComPtr<T>& operator=(const ComPtr<T>& c){
		Replace(c.ptr);
		return *this;
	}

	inline ComPtr<T>& operator=(ComPtr<T>&& c){
		if (&ptr != &c.ptr) {
			Kill();
			ptr = c.ptr;
			c.ptr = nullptr;
		}
		return *this;
	}

	inline T* Detach(){
		T* out = ptr;
		ptr = nullptr;
		return out;
	}

	inline void CopyTo(T** out){
		if (out) {
			if (ptr)
				ptr->AddRef();
			*out = ptr;
		}
	}

	inline ULONG Release(){
		ULONG ref;
		if (!ptr)
			return 0;
		ref = ptr->Release();
		ptr = nullptr;
		return ref;
	}

	inline T** Assign(){
		Clear();
		return &ptr;
	}
	inline void Set(T* p){
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

template<class T> class ComQIPtr: public ComPtr<T> {

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

class WgcCapture :public WXThread {

public:
	bool  m_bInit = false;
	int   m_nFps = 25;//帧率
	int   m_nTime = 40;//操作时间间隔
	DWORD m_tsStart = 0;
	bool m_bStart = false;
	ComPtr<IDXGIFactory1> m_factory;
	ComPtr<IDXGIAdapter1> m_adapter;
	ComPtr<ID3D11Device>  m_d3d11Device;
	ComPtr<ID3D11DeviceContext> m_context;
	uint32_t m_adpIdx = 0;
	ComPtr<ID3D11Texture2D>m_texture = nullptr;
	BOOL m_capture_cursor = TRUE;

	WGC_GCI    m_item{ nullptr };
	WGC_Device m_device = nullptr;
	WGC_POOL    m_frame_pool{ nullptr };
	WGC::GraphicsCaptureSession m_session{ nullptr };
	WGS  m_ImageSize{ 0,0 };
	WGC_GCI::Closed_revoker m_closed;
	WGC_POOL::FrameArrived_revoker m_frame_arrived;

	bool increase_maximum_frame_latency() {
		ComQIPtr<IDXGIDevice1> dxgiDevice(m_d3d11Device);
		if (!dxgiDevice) {
			return false;
		}

		const HRESULT hr = dxgiDevice->SetMaximumFrameLatency(16);
		if (FAILED(hr)) {
			return false;
		}
		return true;
	}

	bool InitDevice(uint32_t& adapterIdx) {
		HRESULT hr;
		IID factoryIID = dxgiFactory2;
		hr = CreateDXGIFactory1(factoryIID, (void**)m_factory.Assign());
		if (FAILED(hr)) {
			return false;
		}

		std::vector<uint32_t> adapterOrder;
		ComPtr<IDXGIAdapter> adapter;
		DXGI_ADAPTER_DESC desc;
		uint32_t iGPUIndex = 0;
		bool hasIGPU = false;
		bool hasDGPU = false;
		int idx = 0;

		while (SUCCEEDED(m_factory->EnumAdapters(idx, &adapter))) {
			if (SUCCEEDED(adapter->GetDesc(&desc))) {
				if (desc.VendorId == VENDOR_ID_INTEL) {
					if (desc.DedicatedVideoMemory <= IGPU_MEM) {
						hasIGPU = true;
						iGPUIndex = (uint32_t)idx;
					}
					else {
						hasDGPU = true;
					}
				}
			}

			adapterOrder.push_back((uint32_t)idx++);
		}

		/* Intel specific adapter check for Intel integrated and Intel
		 * dedicated. If both exist, then change adapter priority so that the
		 * integrated comes first for the sake of improving overall
		 * performance */
		if (hasIGPU && hasDGPU) {
			adapterOrder.erase(adapterOrder.begin() + iGPUIndex);
			adapterOrder.insert(adapterOrder.begin(), iGPUIndex);
			adapterIdx = adapterOrder[adapterIdx];
		}

		hr = m_factory->EnumAdapters1(adapterIdx, &m_adapter);
		if (FAILED(hr)) {
			return false;
		}

		D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_10_0;
		m_adpIdx = adapterIdx;
		uint32_t createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
		//createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		const static D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};

		hr = D3D11CreateDevice(m_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
			createFlags, featureLevels,
			sizeof(featureLevels) /
			sizeof(D3D_FEATURE_LEVEL),
			D3D11_SDK_VERSION, &m_d3d11Device, &levelUsed,
			&m_context);
		if (FAILED(hr)) {
			return false;
		}

		/* prevent stalls sometimes seen in Present calls */
		if (!increase_maximum_frame_latency()) {
			return false;
		}
		return true;
	}

	void on_closed(WGC_GCI const&,WF::IInspectable const&){}

	void CreateTexture(int width, int height) {
		D3D11_TEXTURE2D_DESC frameDescriptor;
		memset(&frameDescriptor, 0, sizeof(frameDescriptor));
		frameDescriptor.Width = width;
		frameDescriptor.Height = height;
		frameDescriptor.Usage = D3D11_USAGE_STAGING;
		frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frameDescriptor.BindFlags = 0;
		frameDescriptor.MiscFlags = 0;
		frameDescriptor.MipLevels = 1;
		frameDescriptor.ArraySize = 1;
		frameDescriptor.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		frameDescriptor.SampleDesc.Count = 1;
		HRESULT hr = m_d3d11Device->CreateTexture2D(&frameDescriptor, nullptr, &m_texture);//创建内存表面
		if (FAILED(hr)) {
			//printf("CreateTexture2D \r\n");
		}
	}

	//采集回调函数
	void on_frame_arrived(WGC_POOL const& sender,WF::IInspectable const&){
		const WGC::Direct3D11CaptureFrame frame = sender.TryGetNextFrame();
		const WGS content_size = frame.ContentSize();
		if (content_size.Width != m_ImageSize.Width ||
			content_size.Height != m_ImageSize.Height) {
			m_frame_pool.Recreate(m_device, WGC_FORMAT, POOL_NUM, content_size);
			CreateTexture(content_size.Width, content_size.Height);
			m_ImageSize = content_size;
		}
		else {
			//正常采集处理
			winrt::com_ptr<ID3D11Texture2D> frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(
				frame.Surface());
			if (m_texture) { //处理采集数据
				m_context->CopyResource(m_texture, frame_surface.get());

				//call back RGB32 Data
				int nPitch = 0;
				D3D11_MAPPED_SUBRESOURCE mappedRect = {};   //显存到内存
				HRESULT hr = m_context->Map(m_texture, 0, D3D11_MAP_READ, 0, &mappedRect);
				if (SUCCEEDED(hr)) {
					nPitch = mappedRect.RowPitch;
					int t1 = timeGetTime() - m_tsStart;
					printf("DDDD nPitch=%d  tt = %d\r\n", nPitch, t1);
					m_context->Unmap(m_texture, 0);
				}
			}
		}
	}
public:


	//线程循环函数,必须实现
	virtual  void ThreadProcess() {
		if (!m_bStart)
			return;
		//在该线程触发采集操作
		//GrabFrameImpl
		int t1 = timeGetTime() - m_tsStart;
		printf("AAAA t1 = %d\r\n",t1);

		int ta = timeGetTime();
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			t1 = timeGetTime() - m_tsStart;
			printf("BBBB1111 t1 = %d\r\n", t1);
			//可能在这里触发采集
			//on_frame 采集回调在同一个线程执行
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			t1 = timeGetTime() - m_tsStart;
			printf("BBBB222 t1 = %d\r\n", t1);
		}
		t1 = timeGetTime() - m_tsStart;
		printf("CCCC t1 = %d\r\n", t1);
		int tb = timeGetTime() - ta;
		int tt = m_nTime - tb;
		if (tt < 0)tt = 1;
		Sleep(tt);
	}

public:
	

	WgcCapture(int nFps) {
		m_nFps = nFps;
		m_nTime = 1000 / m_nFps;
	}

	int GetWidth() {
		return m_ImageSize.Width;
	}
	int GetHeight() {
		return m_ImageSize.Height;
	}

	int  Init() {
		int bRet = 0;
		int bFlag = 0;
		WXTask task = [this, &bRet, &bFlag]() {
			bRet = this->InitImpl();
			bFlag = 1;
		};
		RunTask(task, FALSE);
		while (true) {
			if (bFlag) {
				break;
			}
			Sleep(10);
		}
		return bRet;
	}

	int  Start() {
		int bRet = 0;
		int bFlag = 0;
		WXTask task = [this, &bRet, &bFlag]() {
			bRet = this->StartImpl();
			bFlag = 1;
		};
		RunTask(task, FALSE);
		while (true){
			if (bFlag) {
				break;
			}
			Sleep(10);
		}
		return bRet;
	}

	void Stop() {
		int bFlag = 0;
		WXTask task = [this, &bFlag]() {
			this->StopImpl();
			bFlag = 1;
		};
		RunTask(task, FALSE);
		while (true) {
			if (bFlag) {
				break;
			}
			Sleep(10);
		}
	}
public:
	int  InitImpl() { //初始化
		printf("Initializing D3D11...\r\n");
		uint32_t uid = 0;
		bool bInit = InitDevice(uid);
		if (!bInit) {
			return false;
		}
		struct wgc_monitor_info monitor = { 0 };
		monitor.desired_id = 0;
		EnumDisplayMonitors(NULL, NULL, enum_monitor, (LPARAM)&monitor);
		m_capture_cursor = TRUE;

		try {
			ComPtr<IDXGIDevice> dxgi_device;
			HRESULT hr = m_d3d11Device->QueryInterface(&dxgi_device);
			if (FAILED(hr)) {
				return false;
			}
			winrt::com_ptr<IInspectable> inspectable;
			hr = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.Get(),
				inspectable.put());
			if (FAILED(hr)) {
				return false;
			}

			auto activation_factory = winrt::get_activation_factory<WGC_GCI>();
			auto interop_factory =activation_factory.as<IGCII>();
			m_item = create_item(interop_factory.get(), monitor.handle);
			if (!m_item)
				return false;
			m_device = inspectable.as<WGC_Device>();
			m_ImageSize = m_item.Size();
			m_frame_pool = WGC_POOL::Create(m_device, WGC_FORMAT,
				POOL_NUM, m_ImageSize);
			m_session = m_frame_pool.CreateCaptureSession(m_item);
			const BOOL cursor_supported = cursor_toggle_supported();//系统是否允许录制鼠标
			m_capture_cursor = m_capture_cursor && cursor_supported;
			if (cursor_supported)
				m_session.IsCursorCaptureEnabled(m_capture_cursor);
			m_closed = m_item.Closed(winrt::auto_revoke, { this, &WgcCapture::on_closed });
			m_frame_arrived = m_frame_pool.FrameArrived(winrt::auto_revoke, { this, &WgcCapture::on_frame_arrived });
			CreateTexture(m_ImageSize.Width, m_ImageSize.Height);
			m_bInit = true;
			return true;
		}
		catch (...) {
			m_bInit = false;
		}
		return m_bInit;
	}

	int  StartImpl(){
		try {
			m_session.StartCapture();//StopCapture
			m_tsStart = timeGetTime();
			m_bStart = true;
			return 1;
		}catch (...) {
		}
		return 0;
	}

	void StopImpl() {
		if (m_bInit) {
			m_frame_arrived.revoke();
			m_closed.revoke();
			try {
				m_frame_pool.Close();
				m_session.Close();
			}catch (...) {
			}
			m_bInit = false;
		}
	}
};

int  wgc_capture_width(WgcCapture* capture) {
	if (capture) {
		return capture->GetWidth();
	}
	return 0;
}

int  wgc_capture_height(WgcCapture* capture) {
	if (capture) {
		return capture->GetHeight();
	}
	return 0;
}

WgcCapture* wgc_capture_create(int nFps) {
	WgcCapture* capture = new WgcCapture(nFps);
	capture->ThreadStart(TRUE);
	capture->Init();
	return capture;
}

int   wgc_capture_start(WgcCapture* capture) {
	return capture->Start();
}

void  wgc_capture_stop(WgcCapture* capture) {
 	capture->Stop();
}