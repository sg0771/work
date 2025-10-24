/*
* 基于WGC的桌面视频采集类
*/
#ifdef _WIN32
#include "WgcCapture.h"
#include <WXMediaCpp.h>




bool WgcCapture::increase_maximum_frame_latency() {
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

bool WgcCapture::InitDevice(uint32_t& adapterIdx) {
	HRESULT hr;
	IID factoryIID = dxgiFactory2;
	hr = LibInst::GetInst().mCreateDXGIFactory1(factoryIID, (void**)m_factory.Assign());
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

	hr = LibInst::GetInst().mD3D11CreateDevice(m_adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL,
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

void WgcCapture::on_closed(WGC_GCI const&, WF::IInspectable const&) {

	WXLogA("%s",__FUNCTION__);
}

void WgcCapture::CreateTexture(int width, int height) {
	m_texture = nullptr;
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
void WgcCapture::on_frame_arrived(WGC_POOL const& sender, WF::IInspectable const&) {

	WXVideoFrame* currFrame = m_queuePool.Pop();
	if (currFrame == NULL)
	{
		return;
	}

	m_ptsVideo = WXGetTimeMs();//当前时间

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
			D3D11_MAPPED_SUBRESOURCE mappedRect = {};   //显存到内存
			HRESULT hr = m_context->Map(m_texture, 0, D3D11_MAP_READ, 0, &mappedRect);
			if (SUCCEEDED(hr)) {
				if (content_size.Width == m_iWidth && content_size.Height == m_iHeight) {
					libyuv::ARGBCopy((uint8_t*)mappedRect.pData, mappedRect.RowPitch,
						currFrame->GetFrame()->data[0], currFrame->GetFrame()->linesize[0],
						m_iWidth, m_iHeight);
				}
				else {
					libyuv::ARGBScale((uint8_t*)mappedRect.pData, mappedRect.RowPitch,
						content_size.Width, content_size.Height,
						currFrame->GetFrame()->data[0], currFrame->GetFrame()->linesize[0],
						m_iWidth, m_iHeight, libyuv::FilterMode::kFilterLinear);
				}			
				currFrame->GetFrame()->pts = m_ptsVideo;//采集时间戳
				m_queueData.Push(currFrame);
				m_context->Unmap(m_texture, 0);
			}
		}
	}

	//int delay = m_iTime - ( WXGetTimeMs() - m_ptsVideo);
	//SLEEPMS(delay);
}

int  WgcCapture::InitImpl() { //初始化
	//printf("Initializing D3D11...\r\n");
	uint32_t uid = 0;

	winrt::init_apartment();
	bool bInit = InitDevice(uid);
	if (!bInit) {
		return 0;
	}
	struct wgc_monitor_info monitor = { 0 };
	monitor.desired_id = 0;
	EnumDisplayMonitors(NULL, NULL, enum_monitor, (LPARAM)&monitor);//采集默认显示器
	m_capture_cursor = TRUE;

	try {
		ComPtr<IDXGIDevice> dxgi_device;
		HRESULT hr = m_d3d11Device->QueryInterface(&dxgi_device);
		if (FAILED(hr)) {
			return 0;
		}
		winrt::com_ptr<IInspectable> inspectable;
		hr = LibInst::GetInst().mCreateDirect3D11DeviceFromDXGIDevice(dxgi_device.Get(),
			inspectable.put());
		if (FAILED(hr)) {
			return 0;
		}

		auto activation_factory = winrt::get_activation_factory<WGC_GCI>();

		auto interop_factory = activation_factory.as<IGCII>();

		m_item = create_item(interop_factory.get(), monitor.handle);

		if (!m_item)
			return 0;

		m_device = inspectable.as<WGC_Device>();

		m_ImageSize = m_item.Size();

		m_frame_pool = WGC_POOL::Create(m_device, WGC_FORMAT,
			POOL_NUM, m_ImageSize);
		
		m_session = m_frame_pool.CreateCaptureSession(m_item); //采集对象
		
		const BOOL cursor_supported = cursor_toggle_supported();//系统是否允许录制鼠标
		
		m_capture_cursor = m_capture_cursor && cursor_supported;
		
		if (cursor_supported)
			m_session.IsCursorCaptureEnabled(m_capture_cursor); //录制鼠标

		m_closed = m_item.Closed(winrt::auto_revoke, { this, &WgcCapture::on_closed }); //结束回调

		m_frame_arrived = m_frame_pool.FrameArrived(winrt::auto_revoke, { this, &WgcCapture::on_frame_arrived });//采集数据回调
		CreateTexture(m_ImageSize.Width, m_ImageSize.Height); //创建Texture
		m_bInit = true;

		m_iWidth = m_ImageSize.Width;
		m_iHeight = m_ImageSize.Height;
		m_InputVideoFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);//图像
		m_strName = m_video.m_wszDevName;

		m_nPool = 3;
		m_aData = new WXVideoFrame[m_nPool];
		for (int i = 0; i < m_nPool; i++) {  //初始化
			m_aData[i].Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
			m_queuePool.Push(&m_aData[i]);
		}

		return 1;
	}
	catch (...) {
		m_bInit = 0;
	}
	return m_bInit;
}



int WgcCapture::Init() {

	try {

		m_bInitWGC = this->InitImpl();


	}
	catch (...) {
		m_bStart = FALSE;
		m_bFlag = 0;
	}
	return m_bInit ? WX_ERROR_SUCCESS : WX_ERROR_ERROR;
}

void WgcCapture::Start() {
	if (m_bInit) {
		SetCapture();
		m_session.StartCapture();//StopCapture
		m_bStart = TRUE;
	}
}

//关闭设备
void WgcCapture::Stop() {

	if (m_bStart) {
		m_frame_arrived.revoke();
		m_closed.revoke();
		try {
			m_session.Close();
			m_frame_pool.Close();
		}
		catch (...) {
		}
		m_bStart = false;
	}
}

//实际采集操作
int WgcCapture::GrabFrameImpl(WXVideoFrame* avframe) {
	//m_currFrame = avframe;
	//MSG msg;
	//if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	//	TranslateMessage(&msg);
	//	DispatchMessage(&msg);
	//}
	//if (m_currFrame)
	//	return 0;//采集失败
	return 1; //采集成功
}


WXCTSTR WgcCapture::Type() {
	return L"WGC";
}
#endif



//WinRT API for WGC
EXTERN_C int32_t WINAPI WINRT_IMPL_RoOriginateLanguageException(int32_t error, void* message, void* languageException) noexcept
{
	if (LibInst::GetInst().m_RoOriginateLanguageException == nullptr)
		return TRUE;
	return LibInst::GetInst().m_RoOriginateLanguageException(error, message, languageException);
}

EXTERN_C int32_t WINAPI WINRT_IMPL_RoGetActivationFactory(void* classId, winrt::guid const& iid, void** factory) noexcept
{
	if (LibInst::GetInst().m_RoGetActivationFactory == nullptr) {
		*factory = nullptr;
		return winrt::impl::error_class_not_available;
	}
	return LibInst::GetInst().m_RoGetActivationFactory(classId, iid, factory);
}