/*
Windows DirectShow 摄像头视频采集
*/
#ifdef _WIN32

#include "./Lav/BaseClasses/streams.h"
#include "./Lav/BaseClasses/renbase.h"

#include <WXMediaCpp.h>
#include "WXCapture.h"
#include <json.hpp>


//EXTERN_C const CLSID CLSID_NullRenderer;

//Impl on WXCaptureVideo.cpp
extern void CameraDataPush(AVFrame* frame);

extern void WXGameDrawCamera(AVFrame* frame, int x, int y, int w, int h);

static void WXDeleteMediaType(AM_MEDIA_TYPE* pmt) {
	if (pmt != nullptr) {
		if (pmt->cbFormat != 0) {
			CoTaskMemFree((PVOID)pmt->pbFormat);
			pmt->cbFormat = 0;
			pmt->pbFormat = nullptr;
		}
		if (pmt->pUnk) {
			pmt->pUnk->Release();
			pmt->pUnk = nullptr;
		}
		CoTaskMemFree((PVOID)pmt);
	}
}

//directshow API
//从设备名字得到一个BaseFilter指针
static IBaseFilter* WXCreateIBaseFilterByGUID(WXCTSTR wsz) {
	CComPtr<ICreateDevEnum> pDevEnum = nullptr;
	HRESULT hr = pDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
	HR_RETURN_NULL(hr);
	CComPtr<IEnumMoniker>pEnumMoniker = nullptr;
	hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumMoniker, 0);
	HR_RETURN_NULL(hr);
	hr = pEnumMoniker->Reset();
	HR_RETURN_NULL(hr);
	while (1) {
		ULONG cFetched = 0;
		CComPtr<IMoniker>pM = nullptr;
		hr = pEnumMoniker->Next(1, &pM, &cFetched);
		if (FAILED(hr) || nullptr == pM)break;
		WCHAR* wzGUID = nullptr;
		hr = pM->GetDisplayName(nullptr, nullptr, &wzGUID);
		if (FAILED(hr))continue;
		WXString strGUID = wzGUID;//设备GUID
		if (strGUID == wsz) {
			IBaseFilter* pSrcFilter = nullptr;
			hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrcFilter);
			HR_RETURN_NULL(hr);
			return pSrcFilter;
		}
	}
	return nullptr;//没有找到设备
}

static IPin* WXGetCapturePin(IBaseFilter* pFilter, PIN_DIRECTION PinDir, GUID guidDst) {
	CComPtr<IEnumPins>pEnum = nullptr;
	IPin* pPin = nullptr;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr)) {
		return nullptr;
	}
	while (pEnum->Next(1, &pPin, 0) == S_OK) {
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);
		if (PinDir == PinDirThis) {
			CComPtr<IKsPropertySet> pKs = nullptr;
			hr = pPin->QueryInterface(IID_IKsPropertySet, (void**)&pKs);
			GUID guid;
			DWORD cbBytes = 0;
			if (hr == S_OK) {
				hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, nullptr, 0, &guid,
					sizeof(GUID), &cbBytes);
				if (guid == guidDst) {
					break;
				}
			}
		}
		pPin->Release();
		pPin = nullptr;
	}
	PIN_INFO info;
	if (pPin) {
		hr = pPin->QueryPinInfo(&info);
	}
	return pPin;
}

//将Filter连接到Pin
HRESULT WXConnectFilter(IGraphBuilder* pGB, IPin* srcPin, IBaseFilter* pFilter) {
	if (pFilter == nullptr)
		return E_FAIL;
	CComPtr<IEnumPins>pinEnum = NULL;
	if (SUCCEEDED(pFilter->EnumPins(&pinEnum))) {
		pinEnum->Reset();
		CComPtr < IPin> dstPin = NULL;
		while (pinEnum->Next(1, &dstPin, nullptr) == S_OK) {
			if (dstPin) {
				PIN_INFO pinInfo;
				if (SUCCEEDED(dstPin->QueryPinInfo(&pinInfo))) {
					pinInfo.pFilter->Release();
					if (pinInfo.dir == PINDIR_INPUT) {
						HRESULT hr = pGB->Connect(srcPin, dstPin);
						if (SUCCEEDED(hr)) {
							return hr;
						}
					}
				}
			}
			dstPin = NULL;
		}
	}
	return E_FAIL;
}

static WXLocker s_LockCamera;//摄像头操作锁
static std::atomic_bool s_bInitCamera = FALSE;//是否已经初始化过摄像头

//Camera.json配置
WXString g_strJsonPath = L"WXCamera.json";//Camera.json

void SetCameraJsonPath(WXCTSTR path) {
	g_strJsonPath = path;
}


class DShowCapture {
public:


	//唯一摄像头设备
	static DShowCapture* Inst(DShowCapture* p, int mode = 1) {
		static DShowCapture* s_pCurrDevice = nullptr;
		if (p) {
			s_pCurrDevice = p;
		}
		else if (p == nullptr && mode == 0) {
			s_pCurrDevice = nullptr;//关闭
		}
		return s_pCurrDevice;
	}

	//--------------- 摄像头属性 ------------------
	static std::vector<CameraInfo>& GetList() {
		static std::vector<CameraInfo> s_arrCameraInfo;
		return s_arrCameraInfo;
	}



	static int ListCameraInfo() {

		//读取json文件中的摄像头信息
		static std::map<std::string, CameraInfo> mapCameraInfoFromFile;//存储json文件中的摄像头信息
		
		std::ifstream inputFile(g_strJsonPath.str());
		if (inputFile.is_open()) {
			nlohmann::json json_camera;
			inputFile >> json_camera; // 从文件中解析 JSON 数据
			inputFile.close();
			if (json_camera.is_array()) {
				for (size_t nCamera = 0; nCamera < json_camera.size(); nCamera++)
				{
					std::string strU8_guid = json_camera[nCamera]["guid"];
					std::wstring strU16_guid = WXBase::UTF8ToUTF16(strU8_guid.c_str());
					std::string strU8_name = json_camera[nCamera]["name"];
					std::wstring strU16_name = WXBase::UTF8ToUTF16(strU8_name.c_str());
					int fmt_size = json_camera[nCamera]["size_fmt"];

					CameraInfo pCamera;
					memset(&pCamera, 0, sizeof(CameraInfo));
					bool bSuppost = false;//格式支持
					wcscpy(pCamera.m_strName, strU16_name.c_str());
					wcscpy(pCamera.m_strGuid, strU16_guid.c_str());
					pCamera.size_fmt = fmt_size;
					nlohmann::json arrFmt = json_camera[nCamera]["arrFmt"];
					for (size_t i = 0; i < fmt_size; i++) {
						pCamera.m_arrFmt[i].width = arrFmt[i]["width"];
						pCamera.m_arrFmt[i].height = arrFmt[i]["height"];
						pCamera.m_arrFmt[i].fps = arrFmt[i]["fps"];
						pCamera.m_arrFmt[i].mt = arrFmt[i]["mt"];
						pCamera.m_arrFmt[i].index = arrFmt[i]["index"];
						pCamera.m_arrFmt[i].AvgTimePerFrame = arrFmt[i]["AvgTimePerFrame"];
					}
					mapCameraInfoFromFile[strU8_guid] = pCamera;
				}
			}
		}

		DShowCapture::GetList().clear();//清空队列

		CComPtr<ICreateDevEnum> pCreateDevEnum = nullptr;
		HRESULT hr = pCreateDevEnum.CoCreateInstance(CLSID_SystemDeviceEnum);
		HR_RETURN_0(hr)
		if (pCreateDevEnum == nullptr)
			return 0;

		CComPtr<IEnumMoniker> pEm = nullptr;
		hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
		HR_RETURN_0(hr)
		if (pEm == nullptr)
			return 0;

		pEm->Reset();
		while (TRUE) {
			ULONG cFetched = 0;
			CComPtr<IMoniker>pM = nullptr;
			hr = pEm->Next(1, &pM, &cFetched);
			if (FAILED(hr) || nullptr == pM)break;

			WCHAR* wzGUID = nullptr;
			hr = pM->GetDisplayName(nullptr, nullptr, &wzGUID);
			if (FAILED(hr))
				continue;

			WXString strGUID = wzGUID;//设备GUID
			std::string strU8_guid = WXBase::UTF16ToUTF8(strGUID.str());
			if (mapCameraInfoFromFile.count(strU8_guid)) {
				CameraInfo info = mapCameraInfoFromFile[strU8_guid];
				DShowCapture::GetList().push_back(info);
				continue;
			}

			CComPtr<IPropertyBag>pBag;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
			if (FAILED(hr) || pBag == nullptr)
				continue;

			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, nullptr);
			if (FAILED(hr))
				continue;

			WXString strName = var.bstrVal;
			SysFreeString(var.bstrVal);

			//风云录屏过滤
			//
			if (strName == L"screen-capture-recorder") {
				WXLogA("fengyun soft error!!!");
				continue;
			}

			CComPtr<IBaseFilter>pBF = nullptr;
			hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&pBF);
			if (FAILED(hr) || pBF == nullptr)
				continue;

			CComPtr<IPin>pinCapture = WXGetCapturePin(pBF, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE);
			if (pinCapture == nullptr)
				continue;

			CComPtr<IAMStreamConfig>pSC = nullptr;
			hr = pinCapture->QueryInterface(IID_IAMStreamConfig, (void**)&pSC);
			if (FAILED(hr) || pSC == nullptr)
				continue;

			int iCount = 0, iSize = 0;
			hr = pSC->GetNumberOfCapabilities(&iCount, &iSize);//查找设备采集能力
			if (SUCCEEDED(hr) && iCount > 0) {
				CameraInfo pCamera;
				memset(&pCamera, 0, sizeof(CameraInfo));
				bool bSuppost = false;//格式支持
				wcscpy(pCamera.m_strName, strName.str());
				wcscpy(pCamera.m_strGuid, strGUID.str());

				//枚举分辨率
				for (int index = 0; index < iCount; index++) {
					VIDEO_STREAM_CONFIG_CAPS scc;
					AM_MEDIA_TYPE* pmt = nullptr;
					//采集格式
					if (SUCCEEDED(pSC->GetStreamCaps(index, &pmt, (BYTE*)&scc))) {

						BITMAPINFOHEADER* bih = nullptr;
						int64_t AvgTimePerFrame = 0;//默认帧率
						if (pmt->formattype == FORMAT_VideoInfo && pmt->pbFormat) {
							VIDEOINFOHEADER* lpHeader = (VIDEOINFOHEADER*)pmt->pbFormat;
							bih = &(lpHeader->bmiHeader);
							AvgTimePerFrame = lpHeader->AvgTimePerFrame;
						}
						else if (pmt->formattype == FORMAT_VideoInfo2 && pmt->pbFormat) {
							VIDEOINFOHEADER2* lpHeader = (VIDEOINFOHEADER2*)pmt->pbFormat;
							bih = &(lpHeader->bmiHeader);
							AvgTimePerFrame = lpHeader->AvgTimePerFrame;
						}
						if (bih == nullptr)continue;

						if ((bih->biCompression == BI_RGB && bih->biBitCount == 32) ||
							(bih->biCompression == BI_RGB && bih->biBitCount == 24) ||
							bih->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2') ||
							bih->biCompression == MAKEFOURCC('M', 'J', 'P', 'G') ||
							bih->biCompression == MAKEFOURCC('Y', 'V', '1', '2') ||
							bih->biCompression == MAKEFOURCC('I', '4', '2', '0')
							)

						{
							bool bExist = false;

							CameraDataFormat fmt;
							fmt.width = bih->biWidth;
							fmt.height = bih->biHeight;
							fmt.fps = (unsigned int)(10000000.0 / ((double)AvgTimePerFrame + 0.1) + 0.5);//帧率

							for (int i = 0; i < pCamera.size_fmt; i++) {
								if (pCamera.m_arrFmt[i].width == bih->biWidth &&
									pCamera.m_arrFmt[i].height == bih->biHeight &&
									pCamera.m_arrFmt[i].AvgTimePerFrame == AvgTimePerFrame) {
									bExist = true; break;
								}
							}

							if (!bExist) {
								fmt.mt = WX_DT_VIDEO_INVALD;
								if (bih->biCompression == BI_RGB && bih->biBitCount == 32)
									fmt.mt = WX_DT_VIDEO_RGBA;
								else if (bih->biCompression == BI_RGB && bih->biBitCount == 24)
									fmt.mt = WX_DT_VIDEO_RGB24;
								else if (bih->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2'))
									fmt.mt = WX_DT_VIDEO_YUY2;
								else if (bih->biCompression == MAKEFOURCC('M', 'J', 'P', 'G'))
									fmt.mt = WX_DT_VIDEO_MJPG;
								else if (bih->biCompression == MAKEFOURCC('Y', 'V', '1', '2'))
									fmt.mt = WX_DT_VIDEO_YV12;
								else if (bih->biCompression == MAKEFOURCC('N', 'V', '1', '2'))
									fmt.mt = WX_DT_VIDEO_NV12;
								else if (bih->biCompression == MAKEFOURCC('I', '4', '2', '0'))
									fmt.mt = WX_DT_VIDEO_I420;

								fmt.index = index;
								fmt.AvgTimePerFrame = AvgTimePerFrame;
								memcpy(&pCamera.m_arrFmt[pCamera.size_fmt], &fmt, sizeof(CameraDataFormat));
								pCamera.size_fmt++;
								bSuppost = true;
							}
						}
						WXDeleteMediaType(pmt);
					}
				}
				if (bSuppost) {
					DShowCapture::GetList().push_back(pCamera);
					mapCameraInfoFromFile[strU8_guid] = pCamera;
				}
			}//count > 0
		}//#while(1)

		//保存json信息 
		//map为历史使用的摄像头信息
		std::ofstream outputFile(g_strJsonPath.str());
		if (outputFile.is_open()) {
			nlohmann::json json_camera;
			int nCameraIndex = 0;
			for (auto obj : mapCameraInfoFromFile)
			{
				nlohmann::json camera_info;
				WXString strGuid = mapCameraInfoFromFile[obj.first].m_strGuid;
				WXString strName = mapCameraInfoFromFile[obj.first].m_strName;
				camera_info["guid"] = strGuid.c_str();
				camera_info["name"] = strName.c_str();
				camera_info["size_fmt"] = mapCameraInfoFromFile[obj.first].size_fmt;
				nlohmann::json camera_fmt;
				for (size_t i = 0; i < mapCameraInfoFromFile[obj.first].size_fmt; i++)
				{
					nlohmann::json fmt;
					fmt["width"] = mapCameraInfoFromFile[obj.first].m_arrFmt[i].width;
					fmt["height"] = mapCameraInfoFromFile[obj.first].m_arrFmt[i].height;
					fmt["fps"] = mapCameraInfoFromFile[obj.first].m_arrFmt[i].fps;
					fmt["mt"] = mapCameraInfoFromFile[obj.first].m_arrFmt[i].mt;
					fmt["index"] = mapCameraInfoFromFile[obj.first].m_arrFmt[i].index;
					fmt["AvgTimePerFrame"] = (int)mapCameraInfoFromFile[obj.first].m_arrFmt[i].AvgTimePerFrame;
					camera_fmt[i] = fmt;
				}
				camera_info["arrFmt"] = camera_fmt;
				json_camera[nCameraIndex] = camera_info;
				nCameraIndex++;
			}
			outputFile << json_camera;
			outputFile.close();
		}
		return (int)DShowCapture::GetList().size();
	}
public:
	//自定义Render接收数据
	class __declspec(uuid("AAAADDDD-0000-1111-2222-AAAAAAAABBBB"))
		CVideoRender : public CBaseRenderer
	{
		DShowCapture* m_ctx = nullptr;
	public:
		CVideoRender(DShowCapture* ctx, __inout_opt LPUNKNOWN pUnk, __inout HRESULT* phr) :
			CBaseRenderer(__uuidof(this), (L"Video Grabber filter"), pUnk, phr) {
			m_ctx = ctx;
		}

		virtual HRESULT DoRenderSample(IMediaSample* pMediaSample) { //接收数据
			if (m_ctx && pMediaSample) {
				uint8_t* pBuf = nullptr;
				pMediaSample->GetPointer(&pBuf);
				int size = pMediaSample->GetActualDataLength();
				m_ctx->onVideoFrame(pBuf, size);
			}
			return S_OK;
		}

		virtual HRESULT CheckMediaType(const CMediaType* mt) { //连接协商
			if (mt->majortype == MEDIATYPE_Video && (
				mt->subtype == MEDIASUBTYPE_RGB32 ||
				mt->subtype == MEDIASUBTYPE_RGB24 ||
				mt->subtype == MEDIASUBTYPE_ARGB32 ||
				mt->subtype == MEDIASUBTYPE_YV12 ||
				mt->subtype == MEDIASUBTYPE_NV12 ||
				mt->subtype == MEDIASUBTYPE_YUY2 ||
				mt->subtype == MEDIASUBTYPE_MJPG ||
				mt->subtype == MEDIASUBTYPE_IJPG)) {
				if (m_ctx) {
					m_ctx->GetVideoType(mt);
				}
				return S_OK;
			}
			else {
				//打印消息
			}
			return E_FAIL;
		}

		virtual HRESULT OnStreamStop() {
			//m_ctx->onVideoStreamStop();
			return S_OK;
		}

		virtual ~CVideoRender() {}
	};

	int m_bDrawGame = FALSE;
	int m_nPosX = 0;
	int m_nPosY = 0;
	int m_nW = 0;
	int m_nH = 0;

	WXLocker m_mutex;

	bool m_bHFilp = false;

	int64_t m_iCount = 0;

	//获取图像的标记，1为水印前，2为水印后
	//可能有两路摄像头，不能用个static
	WXVideoFrame m_pYUVFrame;
	WXVideoFrame m_pYUVFrameMirror;
	WXDataBuffer m_dataI420Frame;//回调数据
	WXDataBuffer m_dataRGB32Frame;//回调数据
	WXVideoFrame m_pTmpFrame;

	WXString m_strDeviceGUID;
	WXString m_strDisplayName;
	int m_nFps = 25;
	int m_iWidth = 0;
	int m_iHeight = 0;

	int m_iSrcWidth = 0;
	int m_iSrcHeight = 0;

	HWND m_hWnd = nullptr;
	VideoCallBack m_cbI420 = nullptr;
	VideoCallBack m_cbRGB32 = nullptr;

	VideoCallBack2 m_cbNewI420 = nullptr;
	VideoCallBack2 m_cbNewRGB32 = nullptr;

	bool m_bOpen = false;

	CComPtr<IMediaControl> m_pMediaControl = nullptr;//控制Run Stop
	CComPtr<IGraphBuilder> m_pGraphBuilder = nullptr;//采集框架
	
	IBaseFilter* m_pBFSource = nullptr;//源数据节点
	
	CVideoRender* m_pVideoRender = nullptr;
	CComPtr <IBaseFilter> m_pVRFilter = nullptr;//自定义接收模块

	void* m_pWXVideoRender = NULL;//设置预览窗口
	BOOL m_bGetVideoType = FALSE;//
	int m_iPitch = 0;
	int m_iType = 0;
public:
	void SetDrawGame(int b, int x, int y, int w, int h) {
		if (b && x >= 0 && y >= 0 && w > 0 && h > 0) {
			m_bDrawGame = TRUE;
			m_nPosX = x;
			m_nPosY = y;
			m_nW = MIN(w, 640);
			m_nH = MIN(h, 480);
		}
		else {
			m_bDrawGame = FALSE;
			m_nPosX = 0;
			m_nPosY = 0;
			WXGameDrawCamera(NULL, 0, 0, 0, 0);
		}
	}
public:
	void    SetCbI420(VideoCallBack cb) {
		m_cbI420 = cb;
	}
	void    SetCbRGB32(VideoCallBack cb) {
		m_cbRGB32 = cb;
	}

	void    SetCbI420(VideoCallBack2 cb) {
		m_cbNewI420 = cb;
	}
	void    SetCbRGB32(VideoCallBack2 cb) {
		m_cbNewRGB32 = cb;
	}

	void    SetHFilp(int b) {
		m_bHFilp = !!b;
	}

	DShowCapture() {
	}

	virtual ~DShowCapture() {
		Close();
	}

	void CameraSetting(HWND hwnd) {
		if (m_bOpen) {
			CComQIPtr<ISpecifyPropertyPages> pPages = nullptr;
			HRESULT hr = m_pBFSource->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
			if (SUCCEEDED(hr)) {
				CAUUID cauuid;
				hr = pPages->GetPages(&cauuid);
				if (SUCCEEDED(hr) && cauuid.cElems > 0) {
					LPUNKNOWN* p = (IUnknown**)&m_pBFSource;
					hr = OleCreatePropertyFrame(hwnd, 30, 30, m_strDisplayName.str(), 1,
						p, cauuid.cElems,
						cauuid.pElems, 0, 0, nullptr);
					CoTaskMemFree(cauuid.pElems);
				}
			}
		}
	}

	void Close() {
		WXAutoLock al2(s_LockCamera);
		if (m_bOpen) {
			WXLogW(L"Camera Close  Count=%lld", m_iCount);

			if (m_pMediaControl) { //停止采集，移除COM控制对象
				m_pMediaControl->Stop();
				int tt1 = DBG_GET_REFCOUNT(m_pMediaControl);
				m_pMediaControl = nullptr;
			}

			if (m_pVRFilter) {	//移除COM渲染对象
				if (m_pGraphBuilder)
					m_pGraphBuilder->RemoveFilter(m_pVRFilter);
				m_pVRFilter = nullptr;
			}

			if (m_pBFSource) { //移除COM源对象
				if (m_pGraphBuilder)
					m_pGraphBuilder->RemoveFilter(m_pBFSource);//pBF是设备的IBaseFilter
				m_pBFSource->Release();
				m_pBFSource = nullptr;
			}

			if (m_pGraphBuilder) { //移除采集对象
				int tt2 = DBG_GET_REFCOUNT( m_pGraphBuilder);
				m_pGraphBuilder = nullptr;
			}

			//释放C++对象
			if (m_pVideoRender) {
				delete m_pVideoRender;
				m_pVideoRender = nullptr;
			}

			if (m_pWXVideoRender) {
				WXVideoRenderDestroy(m_pWXVideoRender);
				m_pWXVideoRender = nullptr;
			}

			m_bOpen = false;
		}
	}

	int  Open(WXCTSTR wszGUID, int width, int height, int fps, HWND hwnd) {
		WXAutoLock al(m_mutex);

		//配置参数
		m_iWidth = width;
		m_iHeight = height;
		m_nFps = fps;//来自于设备枚举，不会是0
		m_strDeviceGUID = wszGUID;
		m_hWnd = hwnd;

		CameraInfo* Cinfo = nullptr;
		int CameraCount = WXCameraGetCount();
		for (int i = 0; i < CameraCount; i++) {
			CameraInfo* info = WXCameraGetInfo(i);
			if (m_strDeviceGUID == info->m_strGuid) {
				Cinfo = info;
				break;
			}
		}
		if (Cinfo == nullptr) {
			WXLogW(L"DShowCapture Camera Device  %ws Non-existent", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN | WX_ERROR_VIDEO_NO_DEVICE;//视频设备打开失败
		}

		m_strDisplayName = Cinfo->m_strName;

		m_pBFSource = WXCreateIBaseFilterByGUID(m_strDeviceGUID.str());//名字生成BaseFilter
		if (m_pBFSource == nullptr) {
			WXLogW(L"DShowCapture CreateIBaseFilterByGUID Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}

		HRESULT hr = m_pGraphBuilder.CoCreateInstance(CLSID_FilterGraph);
		if (FAILED(hr) || m_pGraphBuilder == nullptr) {
			WXLogW(L"DShowCapture IID_IGraphBuilder Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}
		int vv1 = DBG_GET_REFCOUNT(m_pGraphBuilder);
		if (vv1 == 0) {
			m_pGraphBuilder.p->AddRef();//CoCreateInstance必须返回引用计数为1对象
		}
		hr = m_pGraphBuilder->QueryInterface(IID_IMediaControl, (void**)&m_pMediaControl);
		if (FAILED(hr)) {
			WXLogW(L"DShowCapture IID_IMediaControl Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}
		int vv2 = DBG_GET_REFCOUNT(m_pMediaControl);

		hr = m_pGraphBuilder->AddFilter(m_pBFSource, L"Capture Filter");
		if (FAILED(hr)) {
			WXLogW(L"DShowCapture m_pGB->AddFilter(m_pBFSource) Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}

		CComQIPtr <IAMStreamConfig> pSC = nullptr;//修改分辨率使用
		CComPtr<IPin>pCaptureOutPin = WXGetCapturePin(m_pBFSource, PINDIR_OUTPUT, PIN_CATEGORY_CAPTURE);
		if (pCaptureOutPin == nullptr) {
			pCaptureOutPin = WXGetCapturePin(m_pBFSource, PINDIR_OUTPUT, PIN_CATEGORY_PREVIEW);
		}
		if (pCaptureOutPin) {
			pSC = pCaptureOutPin;
			//hr = pCaptureOutPin->QueryInterface(IID_IAMStreamConfig, (void**)&pSC);
			if (FAILED(hr) || pSC == nullptr) {
				WXLogW(L"DShowCapture IID_IAMStreamConfig Camera Device  %ws Open Failed", m_strDeviceGUID.str());
				return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
			}
		}
		else {
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}

		int index_fmt = -1;
		for (int i = 0; i < Cinfo->size_fmt; i++) {
			if (Cinfo->m_arrFmt[i].width == m_iWidth &&
				Cinfo->m_arrFmt[i].height == m_iHeight &&
				m_nFps == Cinfo->m_arrFmt[i].fps) {
				m_iType = (WXMediaType)Cinfo->m_arrFmt[i].mt;
				index_fmt = Cinfo->m_arrFmt[i].index; //对应本设备第几个format
				break;
			}
		}

		if (index_fmt < 0) {
			WXLogW(L"DShowCapture The corresponding parameter cannot be found Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN | WX_ERROR_VIDEO_NO_PARAM;//视频设备打开失败
		}


		VIDEO_STREAM_CONFIG_CAPS scc;
		AM_MEDIA_TYPE* pmt = nullptr;
		hr = pSC->GetStreamCaps(index_fmt, &pmt, (BYTE*)&scc);
		if (FAILED(hr) || pmt == nullptr) {
			WXLogW(L"DShowCapture The corresponding parameter cannot be found Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN | WX_ERROR_VIDEO_NO_PARAM;//视频设备打开失败
		}
		hr = pSC->SetFormat(pmt);//设置视频采集参数
		WXDeleteMediaType(pmt);

		if (FAILED(hr)) {
			WXLogW(L"DShowCapture Video parameter setting error Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN | WX_ERROR_VIDEO_NO_PARAM;//视频设备打开失败
		}

		m_pVideoRender = new CVideoRender(this, nullptr, &hr);
		m_pVRFilter = m_pVideoRender;
		//hr = m_pVideoRender->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pVRFilter);

		hr = m_pGraphBuilder->AddFilter(m_pVRFilter, L"Grabber");
		if (FAILED(hr)) {
			WXLogW(L"DShowCapture m_pGB->AddFilter(pGrabBase) Camera Device  %ws Open Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//视频设备打开失败
		}
		hr = m_pGraphBuilder->AddFilter(m_pVRFilter, L"Video Renderer");
		hr = WXConnectFilter(m_pGraphBuilder, pCaptureOutPin, m_pVRFilter);
		if (FAILED(hr)) {
			WXLogW(L"DShowCapture %ws m_pGB->Connect(pCaptureOutPin, pGrabInPin) Failed", m_strDeviceGUID.str());
			return WX_ERROR_VIDEO_DEVICE_OPEN;//nullptrRender
		}

		m_bOpen = true;//打开成功
		m_pMediaControl->Run();//开始视频捕捉

		return WX_ERROR_SUCCESS;
	}

	//连接Filter时触发的回调函数
	void GetVideoType(const CMediaType* amGrabType) {  //连接Filter
		if (!m_bGetVideoType) {
			m_bGetVideoType = 1;
			BITMAPINFOHEADER* bih = nullptr;
			if (amGrabType->formattype == FORMAT_VideoInfo) {
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)amGrabType->pbFormat;
				bih = &vih->bmiHeader;
			}
			else if (amGrabType->formattype == FORMAT_VideoInfo2) {
				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)amGrabType->pbFormat;
				bih = &vih2->bmiHeader;
			}
			if (bih) {
				m_iWidth = bih->biWidth;
				m_iHeight = bih->biHeight;
				m_iPitch = bih->biSizeImage / m_iHeight;

				if (bih->biCompression == MAKEFOURCC('Y', 'V', '1', '2'))
					m_iType = WX_DT_VIDEO_YV12;

				if (bih->biCompression == MAKEFOURCC('N', 'V', '1', '2'))
					m_iType = WX_DT_VIDEO_NV12;

				if (bih->biCompression == MAKEFOURCC('I', '4', '2', '0'))
					m_iType = WX_DT_VIDEO_I420;

				if (bih->biCompression == MAKEFOURCC('Y', 'U', 'Y', '2'))
					m_iType = WX_DT_VIDEO_YUY2;

				if (bih->biCompression == 0 && bih->biBitCount == 32)
					m_iType = WX_DT_VIDEO_RGBA;

				if (bih->biCompression == 0 && bih->biBitCount == 24)
					m_iType = WX_DT_VIDEO_RGB24;
			}

			if (m_iWidth && m_iHeight) {
				//m_pVideoFrame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
				//m_videoBuffer.Init(NULL, m_iWidth * m_iHeight * 3 / 2);

				WXLogW(L"Camera Size is %dx%d", m_iWidth, m_iHeight);
				WXLogW(L"Camera Temp Size is %dx%d, Pitch = %d", m_iSrcWidth, m_iSrcHeight, m_iPitch);

				m_pYUVFrame.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
				m_pYUVFrameMirror.Init(AV_PIX_FMT_YUV420P, m_iWidth, m_iHeight);
				m_dataI420Frame.Init(nullptr, m_iWidth * m_iHeight * 3 / 2);
				m_dataRGB32Frame.Init(nullptr, m_iWidth * m_iHeight * 4);

				if (m_hWnd) {
					m_pWXVideoRender = WXVideoRenderCreate(m_hWnd, m_iWidth, m_iHeight);
				}
			}
		}
	}

	//采集视频回调处理
	int onVideoFrame(BYTE* buf, long size) {
		WXAutoLock al(m_mutex);
		{
			m_pYUVFrame.GetFrame()->pts = WXGetTimeMs();//时间戳
			m_iCount++;

			if (m_iType == WX_DT_VIDEO_YUY2) {
				if (size == 1920 * 1080 * 2) {
					//Wil 电脑 摄像头数据每次回调出来都是一个 1080P的数据帧， Picth 实际就是 1920*2
					if (m_pTmpFrame.GetFrame() == nullptr) {
						m_pTmpFrame.Init(AV_PIX_FMT_YUV420P, 1920, 1080);
						WXLogW(L"---DShowCapture m_pTmpFrame.Init(1920, 1080)");
					}
					libyuv::YUY2ToI420(buf, 3840,
						m_pTmpFrame.GetFrame()->data[0], m_pTmpFrame.GetFrame()->linesize[0],
						m_pTmpFrame.GetFrame()->data[1], m_pTmpFrame.GetFrame()->linesize[1],
						m_pTmpFrame.GetFrame()->data[2], m_pTmpFrame.GetFrame()->linesize[2],
						1920, 1080);
					libyuv::I420Scale(
						m_pTmpFrame.GetFrame()->data[0], m_pTmpFrame.GetFrame()->linesize[0],
						m_pTmpFrame.GetFrame()->data[1], m_pTmpFrame.GetFrame()->linesize[1],
						m_pTmpFrame.GetFrame()->data[2], m_pTmpFrame.GetFrame()->linesize[2],
						1920, 1080,
						m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
						m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
						m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
						m_iWidth, m_iHeight, libyuv::kFilterLinear);
				}
				else {
					libyuv::YUY2ToI420(buf, m_iWidth * 2,
						m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
						m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
						m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
						m_iWidth, m_iHeight);
				}
			}
			else if (m_iType == WX_DT_VIDEO_YV12) {
				libyuv::I420Copy(buf, m_iWidth,
					buf + m_iWidth * m_iHeight, m_iWidth / 2,
					buf + m_iWidth * m_iHeight * 5 / 4, m_iWidth / 2,
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_iWidth, m_iHeight);
			}

			else if (m_iType == WX_DT_VIDEO_I420) {
				libyuv::I420Copy(buf, m_iWidth,
					buf + m_iWidth * m_iHeight, m_iWidth / 2,
					buf + m_iWidth * m_iHeight * 5 / 4, m_iWidth / 2,
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight);
			}
			else if (m_iType == WX_DT_VIDEO_MJPG) {
				libyuv::MJPGToI420(buf, size,
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight, m_iWidth, m_iHeight);
			}
			else if (m_iType == WX_DT_VIDEO_RGBA) {
				libyuv::ARGBToI420(buf + (m_iHeight - 1) * m_iWidth * 4, -m_iWidth * 4,
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight);
			}
			else if (m_iType == WX_DT_VIDEO_RGB24) {
				libyuv::RGB24ToI420(buf + (m_iHeight - 1) * m_iWidth * 3, -m_iWidth * 3,
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_iWidth, m_iHeight);
			}


			//HFILP
			if (m_bHFilp) {
				m_pYUVFrameMirror.GetFrame()->pts = m_pYUVFrame.GetFrame()->pts;

				libyuv::I420Mirror(
					m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
					m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
					m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
					m_pYUVFrameMirror.GetFrame()->data[0], m_pYUVFrameMirror.GetFrame()->linesize[0],
					m_pYUVFrameMirror.GetFrame()->data[1], m_pYUVFrameMirror.GetFrame()->linesize[1],
					m_pYUVFrameMirror.GetFrame()->data[2], m_pYUVFrameMirror.GetFrame()->linesize[2],
					m_iWidth, m_iHeight
				);

				WXAirplayPush(m_pYUVFrameMirror.GetFrame());

				if (m_bDrawGame) {
					WXGameDrawCamera(m_pYUVFrameMirror.GetFrame(), m_nPosX, m_nPosY, m_nW, m_nH);
				}

				if (m_pWXVideoRender)
					WXVideoRenderDisplay(m_pWXVideoRender, m_pYUVFrameMirror.GetFrame(), 0, 0);

				if (m_cbI420) { //紧凑格式的 YUV420P
					libyuv::I420Copy(m_pYUVFrameMirror.GetFrame()->data[0], m_pYUVFrameMirror.GetFrame()->linesize[0],
						m_pYUVFrameMirror.GetFrame()->data[1], m_pYUVFrameMirror.GetFrame()->linesize[1],
						m_pYUVFrameMirror.GetFrame()->data[2], m_pYUVFrameMirror.GetFrame()->linesize[2],
						m_dataI420Frame.GetBuffer(), m_iWidth,
						m_dataI420Frame.GetBuffer() + m_iWidth * m_iHeight, m_iWidth / 2,
						m_dataI420Frame.GetBuffer() + m_iWidth * m_iHeight * 5 / 4, m_iWidth / 2,
						m_iWidth, m_iHeight);
					m_cbI420(m_dataI420Frame.GetBuffer(), m_iWidth, m_iHeight);
				}

				if (m_cbRGB32) {
					libyuv::I420ToARGB(m_pYUVFrameMirror.GetFrame()->data[0], m_pYUVFrameMirror.GetFrame()->linesize[0],
						m_pYUVFrameMirror.GetFrame()->data[1], m_pYUVFrameMirror.GetFrame()->linesize[1],
						m_pYUVFrameMirror.GetFrame()->data[2], m_pYUVFrameMirror.GetFrame()->linesize[2],
						m_dataRGB32Frame.GetBuffer(), m_iWidth * 4,
						m_iWidth, m_iHeight);
					m_cbRGB32(m_dataRGB32Frame.GetBuffer(), m_iWidth, m_iHeight);
				}

				CameraDataPush(m_pYUVFrameMirror.GetFrame()); //录屏叠加处理
			}
			else {
				WXAirplayPush(m_pYUVFrame.GetFrame());
				if (m_pWXVideoRender)
					WXVideoRenderDisplay(m_pWXVideoRender, m_pYUVFrame.GetFrame(), 0, 0);

				if (m_cbI420 || m_cbNewI420) { //紧凑格式的 YUV420P
					libyuv::I420Copy(m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
						m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
						m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
						m_dataI420Frame.GetBuffer(), m_iWidth,
						m_dataI420Frame.GetBuffer() + m_iWidth * m_iHeight, m_iWidth / 2,
						m_dataI420Frame.GetBuffer() + m_iWidth * m_iHeight * 5 / 4, m_iWidth / 2,
						m_iWidth, m_iHeight);
					if (m_cbI420)
						m_cbI420(m_dataI420Frame.GetBuffer(), m_iWidth, m_iHeight);
					if (m_cbNewI420)
						m_cbNewI420(this, m_dataI420Frame.GetBuffer(), m_iWidth, m_iHeight);
				}

				if (m_cbRGB32 || m_cbNewRGB32) {  //补充RGBA输出，当电脑不支持C# I420显示
					libyuv::I420ToARGB(m_pYUVFrame.GetFrame()->data[0], m_pYUVFrame.GetFrame()->linesize[0],
						m_pYUVFrame.GetFrame()->data[1], m_pYUVFrame.GetFrame()->linesize[1],
						m_pYUVFrame.GetFrame()->data[2], m_pYUVFrame.GetFrame()->linesize[2],
						m_dataRGB32Frame.GetBuffer(), m_iWidth * 4,
						m_iWidth, m_iHeight);
					if (m_cbRGB32)
						m_cbRGB32(m_dataRGB32Frame.GetBuffer(), m_iWidth, m_iHeight);
					if (m_cbNewRGB32)
						m_cbNewRGB32(this, m_dataRGB32Frame.GetBuffer(), m_iWidth, m_iHeight);
				}

				CameraDataPush(m_pYUVFrame.GetFrame()); //录屏叠加处理
			}
		}
		return 0;
	}
};


WXMEDIA_API void WXCameraInit() {
	WXAutoLock al(s_LockCamera);
	if (s_bInitCamera.load())return;
	s_bInitCamera.store(true);
	BEGIN_LOG_FUNC
	DShowCapture::ListCameraInfo();
}

WXMEDIA_API void WXCameraDeinit() {
	WXAutoLock al(s_LockCamera);
	s_bInitCamera.store(false);
	DShowCapture::GetList().clear();
}

WXMEDIA_API int  WXCameraGetCount() {
	WXAutoLock al(s_LockCamera);
	return (int)DShowCapture::GetList().size();
}

WXMEDIA_API CameraInfo* WXCameraGetInfo(int index) {
	WXAutoLock al(s_LockCamera);
	int size = (int)DShowCapture::GetList().size();
	if (index < 0 || index >= size)return nullptr;
	return &(DShowCapture::GetList()[index]);
}

//水平翻转
WXMEDIA_API void  WXCameraSetHFilp(void* ptr, int b) {
	WXAutoLock al(s_LockCamera);
	if (ptr) {
		DShowCapture* new_capture = (DShowCapture*)ptr;
		new_capture->SetHFilp(b);
	}
}

//支持多路输出的摄像头初始化
WXMEDIA_API void* WXCameraOpenWithSinkNew(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack2 cb, int bHFilp, int bRGB32) {
	WXAutoLock al(s_LockCamera);
	WXLogA("%s RGBA = %d", __FUNCTION__, bRGB32);

	DShowCapture* new_capture = new DShowCapture;
	if (bRGB32) {
		new_capture->SetCbRGB32(cb);
	}
	else {
		new_capture->SetCbI420(cb);
	}
	int ret = new_capture->Open(devGUID, width, height, iFps, nullptr);
	new_capture->SetHFilp(bHFilp);
	if (ret == WX_ERROR_SUCCESS) {
		//s_pCurrDevice = new_capture;
		return (void*)new_capture;
	}
	delete new_capture;
	return nullptr;
}

WXMEDIA_API void  WXCaptureCameraSetting(void* ptr, HWND hwnd) {
	WXAutoLock al(s_LockCamera);
	if (ptr) {
		DShowCapture* new_capture = (DShowCapture*)ptr;
		new_capture->CameraSetting(hwnd);
	}
}

WXMEDIA_API void  WXCameraSetting(void* ptr, HWND hwnd) {
	WXAutoLock al(s_LockCamera);
	if (ptr) {
		DShowCapture* new_capture = (DShowCapture*)ptr;
		new_capture->CameraSetting(hwnd);
	}
}

WXMEDIA_API void WXCameraSetDrawCamera(void* ptr, int b, int posx, int posy, int width, int height) {
	WXAutoLock al(s_LockCamera);
	if (ptr) {
		DShowCapture* new_capture = (DShowCapture*)ptr;
		new_capture->SetDrawGame(b, posx, posy, width, height);
	}
}

//启动摄像头并在窗口预览
WXMEDIA_API void* WXCameraOpenWithHwnd(WXCTSTR devGUID, int width, int height, int iFps, HWND hwnd, int Fixed) {
	return WXCameraOpenWithHwndExt(devGUID, width, height, iFps, hwnd, Fixed, 0);
}

//启动摄像头，回调YUV数据
WXMEDIA_API void* WXCameraOpenWithSink(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb) {
	return WXCameraOpenWithSinkExt(devGUID, width, height, iFps, cb, 0);
}

//启动摄像头，回调RGB数据
WXMEDIA_API void* WXCameraOpenWithSink2(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb) {
	return WXCameraOpenWithSinkExt2(devGUID, width, height, iFps, cb, 0);
}


//返回当前打开的摄像头设备

WXMEDIA_API void* WXCameraGetCurrDevice() {
	WXAutoLock al(s_LockCamera);
	return DShowCapture::Inst(nullptr);
}

//启动摄像头，并且在指定窗口预览
WXMEDIA_API void* WXCameraOpenWithHwndExt(WXCTSTR devGUID, int width, int height, int iFps, HWND hwnd,
	int Fixed, int bHFilp) {
	WXAutoLock al(s_LockCamera);
	if (DShowCapture::Inst(nullptr) != nullptr) { //摄像头对象已经存在
		WXLogA("%s Camera Object Exist!!!", __FUNCTION__);
		return nullptr;
	}
	DShowCapture* new_capture = new DShowCapture;
	new_capture->SetHFilp(bHFilp);
	int ret = new_capture->Open(devGUID, width, height, iFps, hwnd);
	if (ret == WX_ERROR_SUCCESS) {
		DShowCapture::Inst(new_capture);
		return (void*)new_capture;
	}
	delete new_capture;
	return nullptr;
}

//启动摄像头，回调YUV数据
WXMEDIA_API void* WXCameraOpenWithSinkExt(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp) {
	WXLogA("%s I420", __FUNCTION__);
	WXAutoLock al(s_LockCamera);
	if (DShowCapture::Inst(nullptr) != nullptr) { //摄像头对象已经存在
		WXLogA("%s Camera Object Exist!!!", __FUNCTION__);
		return nullptr;
	}

	DShowCapture* new_capture = new DShowCapture;
	new_capture->SetCbI420(cb);
	int ret = new_capture->Open(devGUID, width, height, iFps, nullptr);
	new_capture->SetHFilp(bHFilp);
	if (ret == WX_ERROR_SUCCESS) {
		DShowCapture::Inst(new_capture);
		return (void*)new_capture;
	}
	delete new_capture;
	return nullptr;
}


//启动摄像头，回调RGB数据
WXMEDIA_API void* WXCameraOpenWithSinkExt2(WXCTSTR devGUID, int width, int height, int iFps, VideoCallBack cb, int bHFilp) {
	WXLogA("%s RGBA", __FUNCTION__);
	WXAutoLock al(s_LockCamera);

	if (DShowCapture::Inst(nullptr) != nullptr) { //摄像头对象已经存在
		WXLogA("%s Camera Object Exist!!!", __FUNCTION__);
		return nullptr;
	}

	DShowCapture* new_capture = new DShowCapture;
	new_capture->SetCbRGB32(cb);
	int ret = new_capture->Open(devGUID, width, height, iFps, nullptr);
	new_capture->SetHFilp(bHFilp);
	if (ret == WX_ERROR_SUCCESS) {
		DShowCapture::Inst(new_capture);
		return (void*)new_capture;
	}
	delete new_capture;
	return nullptr;
}

//关闭摄像头
WXMEDIA_API void  WXCameraClose(void* ptr) {

	WXLogA("%s --- ", __FUNCTION__);

	WXAutoLock al(s_LockCamera);

	if (DShowCapture::Inst(nullptr) == nullptr) {
		WXLogA("s_pCurrDevice is NULL");
	}

	if (ptr) {
		DShowCapture* new_capture = (DShowCapture*)ptr;
		SAFE_DELETE(new_capture);
		DShowCapture::Inst(nullptr,0);
	}
}

#endif
