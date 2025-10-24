/*
* 混合了DXGI/GDI的录屏类，在主显示器区域录制的时候优先使用DXGI采集
*/
#ifdef _WIN32
#include "MixerCapture.h"

int MixerCapture::OpenDevice(WXCTSTR wszText) {
	if (LibInst::GetInst().m_libD3D11 == nullptr) {
		return WX_ERROR_ERROR;
	}

	//LogW(L"%ws[%ws] Begin!!!! m_nTimeOutIndex=%d", __FUNCTIONW__, wszText, m_nTimeOutIndex);

	if (wcsicmp(m_video.m_wszDevName, L"default") == 0) {
		MonitorInfo *info = WXScreenGetDefaultInfo();
		m_strName = info->wszName;
	}
	else {
		m_strName = m_video.m_wszDevName;
	}

	m_nTimeOutIndex = 0;
	m_pDev = nullptr;
	m_pContext = nullptr;
	m_pDesktopDevice = nullptr;
	m_pTexture = nullptr;

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[3] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = 3;

	D3D_FEATURE_LEVEL FeatureLevels[5] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = 5;

	HRESULT hr = S_OK;

	// Create D3D11 device
	D3D_FEATURE_LEVEL FeatureLevel;
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; DriverTypeIndex++) {
		hr = LibInst::GetInst().mD3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &m_pDev, &FeatureLevel, &m_pContext);
		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (m_pContext == nullptr) {
		m_nError++;
		LogW(L"D3D11CreateDevice DX Error[%x] ", hr);
		return WX_ERROR_ERROR;;
	}

	// Get DXGI device
	CComPtr<IDXGIDevice>DxgiDevice = nullptr;
	hr = m_pDev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
	if (SUCCEEDED(hr)) {
		CComPtr<IDXGIAdapter>DxgiAdapter = nullptr;
		hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));	// Get DXGI adapter
		if (SUCCEEDED(hr)) {
			// Get output
			CComPtr<IDXGIOutput>DxgiOutput = nullptr;
			UINT i = 0; //枚举所有的显示器，匹配名字
			while (SUCCEEDED(DxgiAdapter->EnumOutputs(i, &DxgiOutput))) {
				DXGI_OUTPUT_DESC desc;//可以获取名字
				DxgiOutput->GetDesc(&desc);
				if (_wcsicmp(m_strName.str(), desc.DeviceName) == 0) { //找到同名显示器
					CComPtr<IDXGIOutput1>DxgiOutput1 = nullptr;
					hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1)); // QI for Output 1
					if (SUCCEEDED(hr)) {
						hr = DxgiOutput1->DuplicateOutput(m_pDev, &m_pDesktopDevice);//获取桌面采集对象
						if (SUCCEEDED(hr)) {
							DxgiOutput = nullptr;
							DxgiOutput1 = nullptr;
							//LogW(L"%ws OK", __FUNCTIONW__);
							return WX_ERROR_SUCCESS;
						}
					}
					DxgiOutput1 = nullptr;
					DxgiOutput = nullptr;//没有合适的设备
					break;//找到合适的设备
				}
				DxgiOutput = nullptr;//没有合适的设备
				i++;
			}
		}
		else {
			m_nError++;
			LogA("%s GetParent->IDXGIAdapter Error", __FUNCTION__);
		}
		DxgiAdapter = nullptr;
	}
	else {
		m_nError++;
		LogA("%s QueryInterface-IDXGIDevice Error", __FUNCTION__);
	}
	//LogA("%s Error", __FUNCTION__);
	DxgiDevice = nullptr;
	return WX_ERROR_ERROR;
}

int MixerCapture::GrabFrameImpl(WXVideoFrame* frame) {

	int64_t ptsVideo = WXGetTimeMs();
	int ret = 0;
	if (m_video.m_bFollowMouse) {
		ret = GDIGrabFrame(frame); //跟随鼠标模式，使用GDI
	}
	else {
		if (m_iRectPosX < m_iScreenLeft ||
			m_iRectPosY < m_iScreenTop ||
			m_iRectPosX + m_iWidth > m_iScreenLeft + m_iScreenWidth ||
			m_iRectPosY + m_iHeight > m_iScreenTop + m_iScreenHeight) {
			ret = GDIGrabFrame(frame); //跟随鼠标模式，使用GDI,超出主显示器图像区域
		}
		else {
			ret = DXGIGrabFrame(frame);
			if (ret == 0)
				ret = GDIGrabFrame(frame);
		}
	}

	if (ret) {
		frame->GetFrame()->pts = ptsVideo;
	}
	return 1;
}

//初始化GDI采集
int MixerCapture::Init() {
	SetCapture();
	//主显示器区域
	MonitorInfo* infoDefault = WXScreenGetDefaultInfo();
	if (infoDefault == nullptr) {
		WXScreenInit();
		infoDefault = WXScreenGetDefaultInfo();
		if (infoDefault == nullptr) {
			//WXLogA("%s Error", __FUNCTION__);
			return WX_ERROR_ERROR;
		}
	}

	m_iScreenLeft = infoDefault->left;
	m_iScreenTop = infoDefault->top;
	m_iScreenRight = infoDefault->left + infoDefault->width;
	m_iScreenBottom = infoDefault->top + infoDefault->height;
	//获取主显示器大小

	m_iScreenWidth = infoDefault->width / 2 * 2;
	m_iScreenHeight = infoDefault->height / 2 * 2;// 主屏幕大小
	m_pScreenFrame.Init(AV_PIX_FMT_RGB32, m_iScreenWidth, m_iScreenHeight);	//屏幕数据
	m_pScreenBase.Init(AV_PIX_FMT_RGB32, m_iScreenWidth, m_iScreenHeight);	//屏幕数据
	//LogA("%s Create DXGI Screen=[%dx%d]", __FUNCTION__, m_iScreenWidth, m_iScreenHeight);

	if (m_video.m_bRect) { //区域录制
		//LogA("s Desktop Capture Use area screenshots", __FUNCTION__);
		m_iRectPosX = m_video.m_rcScreen.left;
		m_iRectPosY = m_video.m_rcScreen.top;
		m_iRectW = (m_video.m_rcScreen.right - m_video.m_rcScreen.left) / 2 * 2;
		m_iRectH = (m_video.m_rcScreen.bottom - m_video.m_rcScreen.top) / 2 * 2;
	}
	else {
		WXString strName;
		strName.Format(m_video.m_wszDevName);
		//LogA("%s Open Desktop Device [%s]", __FUNCTION__, strName.c_str());
		MonitorInfo* info = WXScreenGetInfoByName(m_video.m_wszDevName);
		if (info == nullptr) {
			info = WXScreenGetDefaultInfo();
		}
		m_video.m_bFollowMouse = FALSE;
		m_iRectPosX = info->left;
		m_iRectPosY = info->top;
		m_iRectW = info->width / 2 * 2;
		m_iRectH = info->height / 2 * 2;
	}

	m_iWidth = m_iRectW;
	m_iHeight = m_iRectH;

	//m_InputVideoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//采集图像

	m_strName = m_video.m_wszDevName;
	/*m_hDC = ::GetDC(nullptr);
	m_hMemDC = ::CreateCompatibleDC(NULL);
	m_pBits = nullptr;
	m_hBitmap = ::CreateDIBSection(m_hMemDC, (const BITMAPINFO*)m_InputVideoFrame.GetBIH(),
		DIB_RGB_COLORS, (void**)&m_pBits, NULL, 0);*/

	//DXGI部分
	if (m_mouse.m_iUsed) { //鼠标动画
		m_cursor.Capture();//获取鼠标信息
		if (m_mouse.m_bMouseHotdot || m_mouse.m_bMouseAnimation) {

			if (m_mouse.m_bMouseHotdot) {
				m_iAlphaHotdot = m_mouse.m_fAlphaHotdot * 100;
				int ImageSize = m_mouse.m_iHotdotRadius * 2;
				m_cursorHotdotFrame.Init(AV_PIX_FMT_RGB32, ImageSize, ImageSize);
				m_cursorHotdotFrame.DrawCircle(m_mouse.m_colorMouse);
			}

			if (m_mouse.m_bMouseAnimation) {
				m_iAlphaAnimation = m_mouse.m_fAlphaAnimation * 100;
				int ImageSize = m_mouse.m_iHotdotRadius * 2;//和热点大小一样
				m_cursorRightFrame.Init(AV_PIX_FMT_RGB32, ImageSize, ImageSize);
				m_cursorRightFrame.DrawCircle(m_mouse.m_colorRight);
				m_cursorLeftFrame.Init(AV_PIX_FMT_RGB32, ImageSize, ImageSize);
				m_cursorLeftFrame.DrawCircle(m_mouse.m_colorLeft);
			}
		}
	}

	if (m_text.m_bUsed && WXStrlen(m_text.m_wszText)) {
		m_pTextMark = new WXTextWaterMark(&m_text);
	}

	if (m_image1.m_bUsed && WXStrlen(m_image1.m_wszFileName)) {
		m_pImageMark1 = new WXImageWaterMark;
		int ImgInit = m_pImageMark1->Init(m_image1.m_wszFileName);
		if (ImgInit == 0) {
			delete m_pImageMark1;
			m_pImageMark1 = nullptr;
		}
	}
	if (m_image2.m_bUsed && WXStrlen(m_image2.m_wszFileName)) {
		m_pImageMark2 = new WXImageWaterMark;
		int ImgInit = m_pImageMark2->Init(m_image2.m_wszFileName);
		if (ImgInit == 0) {
			delete m_pImageMark2;
			m_pImageMark2 = nullptr;
		}
	}

	m_dstFmt = AV_PIX_FMT_RGB32;

	m_nPool = m_iMachLevel == LEVEL_BEST ? MAX_POOL : 1;
	m_aData = new WXVideoFrame[m_nPool];
	for (int i = 0; i < m_nPool; i++) {  //初始化
		m_aData[i].Init(m_dstFmt, m_iWidth, m_iHeight);
		m_queuePool.Push(&m_aData[i]);
	}
	//LogA("%s OK", __FUNCTION__);
	return WX_ERROR_SUCCESS;
}

void MixerCapture::Start() {
	BEGIN_LOG_FUNC
	m_bStart = TRUE;
	DxgiGrab();
	DxgiGrab();
	ThreadSetName(L"MixerCapture");
	ThreadStart(true);
	//LogA("%s OK", __FUNCTION__);
}

//关闭设备
void MixerCapture::Stop() {
	//BEGIN_LOG_FUNC
	//LogW(L"Capture=%lld", m_nVideoFrame);
	//LogW(L"TimeOut=%lld", m_nVideoFrameTimeOut);
	if (m_bStart) {
		m_bStart = FALSE;
		ThreadStop();
		SAFE_RELEASE_DC(nullptr, m_hDC)
		SAFE_DELETE_DC(m_hMemDC)
		SAFE_DELETE_OBJECT(m_hBitmap)

		SAFE_DELETE(m_pImageMark1)
		SAFE_DELETE(m_pImageMark2)
		SAFE_DELETE(m_pTextMark)
		m_pDev = nullptr;
		m_pContext = nullptr;
		m_pDesktopDevice = nullptr;
		m_pTexture = nullptr;
	}
}

int  MixerCapture::GDIGrabFrame(WXVideoFrame *frame) {

	m_bMouseVisable = FALSE;
	//使用鼠标或者跟随鼠标
	if (m_mouse.m_iUsed || m_video.m_bFollowMouse) { //获取鼠标位置和信息

		// 获取当前光标记起位置
		::GetCursorPos(&m_ptCursor);//基于主屏幕的鼠标位置
		CURSORINFO cursorInfo;
		cursorInfo.cbSize = sizeof(CURSORINFO);
		if (::GetCursorInfo(&cursorInfo)) {
			// 获取光标的图像数据
			if (::GetIconInfo(cursorInfo.hCursor, &m_iconInfo)) {
				if (m_iconInfo.hbmMask != nullptr) {
					DeleteObject(m_iconInfo.hbmMask);
				}
				if (m_iconInfo.hbmColor != nullptr) {
					DeleteObject(m_iconInfo.hbmColor);
				}
				m_bMouseVisable = TRUE; //有鼠标图像
				m_hCursor = cursorInfo.hCursor;
				

			/*	m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
				m_ptCursor.y -= ((int)m_iconInfo.yHotspot);*/
			}
		}
	}
	m_InputVideoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//采集图像

	if (nullptr == m_hDC) {
		m_hDC = ::GetDC(nullptr);
		m_hMemDC = ::CreateCompatibleDC(NULL);
		m_pBits = nullptr;
		m_hBitmap = ::CreateDIBSection(m_hMemDC, (const BITMAPINFO*)m_InputVideoFrame.GetBIH(),
			DIB_RGB_COLORS, (void**)&m_pBits, NULL, 0);
	}

	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);

	BOOL bScale = FALSE;
	BOOL bCopyDC = FALSE;

	if (m_iRectW != m_iWidth || m_iRectH != m_iHeight) {
		bScale = TRUE;
		SetStretchBltMode(m_hMemDC, HALFTONE);//硬件缩放
		bCopyDC = ::StretchBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight,
			m_hDC,
			m_video.m_bFollowMouse ? m_ptCursor.x - m_iWidth / 2- ((int)m_iconInfo.xHotspot) : m_iRectPosX,
			m_video.m_bFollowMouse ? m_ptCursor.y - m_iHeight / 2- ((int)m_iconInfo.yHotspot) : m_iRectPosY,
			m_iRectW, m_iRectH,
			m_uCaptureBlt);//有拖影
	}
	else {
		bCopyDC = ::BitBlt(m_hMemDC, 0, 0, m_iWidth, m_iHeight,
			m_hDC,
			m_video.m_bFollowMouse ? m_ptCursor.x - m_iWidth / 2- ((int)m_iconInfo.xHotspot) : m_iRectPosX,
			m_video.m_bFollowMouse ? m_ptCursor.y - m_iHeight / 2- ((int)m_iconInfo.yHotspot) : m_iRectPosY,
			m_uCaptureBlt);//有拖影
	}

	if (!bCopyDC) { //从主桌面获取数据失败
		m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, hOldBitmap);
		SAFE_RELEASE_DC(nullptr, m_hDC)
		SAFE_DELETE_OBJECT(m_hBitmap)
		SAFE_DELETE_DC(m_hMemDC)
		return 0;
	}

	if (m_bMouseVisable) { //绘制鼠标
		if (m_video.m_bFollowMouse) { //鼠标强制居中
			DrawEx(m_ptsVideo, m_iWidth / 2, m_iHeight / 2);
		}
		else {
			//鼠标在当前屏幕的位置，虚拟坐标位置
			//暂时不考虑图像缩放后鼠标的缩放问题！
			m_ptCursor.x -= m_iRectPosX;
			m_ptCursor.y -= m_iRectPosY;
			if (bScale) {
				m_ptCursor.x = m_ptCursor.x * m_iWidth / m_iRectW;
				m_ptCursor.y = m_ptCursor.y * m_iHeight / m_iRectH;
			}
			m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
			m_ptCursor.y -= ((int)m_iconInfo.yHotspot);
			DrawEx(m_ptsVideo, m_ptCursor.x, m_ptCursor.y);
		}
	}
	else {
		DrawEx(m_ptsVideo, 0, 0);
	}
	//获取 m_hMemDC RGBA 数据
	m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, hOldBitmap);
	memcpy(frame->GetFrame()->data[0], m_pBits, m_iWidth * m_iHeight * 4);
	return 1;
}

WXCTSTR MixerCapture::Type() {
	return L"Mixer";
}

void MixerCapture::DrawDxgiInfo(int PosX, int PosY) {

	//暂时不考虑DXGI缩放时的水印处理

	//图片水印
	if (m_pImageMark1)
		m_pImageMark1->DxgiMask(m_pScreenFrame.GetFrame(), m_image1.m_iPosX + PosX, m_image1.m_iPosY + PosY);

	if (m_pImageMark2)
		m_pImageMark2->DxgiMask(m_pScreenFrame.GetFrame(), m_image2.m_iPosX + PosX, m_image2.m_iPosY + PosY);

	//文字水印
	if (m_pTextMark)
		m_pTextMark->Mask(m_pScreenFrame.GetFrame(), m_text.m_iPosX + PosX, m_text.m_iPosY + PosY);
}

void MixerCapture::DrawMouseInfo(int64_t ptsCapture) {

	if (m_bUseMouse && m_bMouseVisable) { //鼠标的相关操作
		BOOL bDrawHotDot = TRUE;

		if (m_mouse.m_iUsed) { //获取鼠标位置和信息

			// 获取当前光标记起位置
			::GetCursorPos(&m_ptCursor);//基于主屏幕的鼠标位置
			CURSORINFO cursorInfo;
			cursorInfo.cbSize = sizeof(CURSORINFO);
			if (::GetCursorInfo(&cursorInfo)) {
				// 获取光标的图像数据
				if (::GetIconInfo(cursorInfo.hCursor, &m_iconInfo)) {
					if (m_iconInfo.hbmMask != nullptr) {
						DeleteObject(m_iconInfo.hbmMask);
					}
					if (m_iconInfo.hbmColor != nullptr) {
						DeleteObject(m_iconInfo.hbmColor);
					}
					m_bMouseVisable = TRUE; //有鼠标图像
					m_hCursor = cursorInfo.hCursor;

					//m_ptCursor.x -= m_iRectPosX;
					//m_ptCursor.y -= m_iRectPosY;
					if (m_iRectW != m_iScreenWidth) {
						m_ptCursor.x = m_ptCursor.x * m_iWidth / m_iRectW;
						m_ptCursor.y = m_ptCursor.y * m_iHeight / m_iRectH;
					}
					m_ptCursor.x -= ((int)m_iconInfo.xHotspot);
					m_ptCursor.y -= ((int)m_iconInfo.yHotspot);
				}
			}
		}

		if (m_mouse.m_iUsed) {

			//鼠标动画
			if (m_mouse.m_bMouseAnimation) {
				if (KEY_DOWN(MOUSE_MOVED)) { //左键按下
					m_ptsMouseLeftAction = ptsCapture;
					m_bClickLeft = TRUE;
					m_bClickRight = FALSE;
					m_lastX = m_ptCursor.x - m_iRectPosX;
					m_lastY = m_ptCursor.x - m_iRectPosY;
					bDrawHotDot = FALSE;
				}
				else if (KEY_DOWN(MOUSE_EVENT)) {//右键按下
					m_ptsMouseRightAction = ptsCapture;
					m_bClickLeft = FALSE;
					m_bClickRight = TRUE;
					m_lastX = m_ptCursor.x - m_iRectPosX;
					m_lastY = m_ptCursor.x - m_iRectPosY;
					bDrawHotDot = FALSE;
				}

				if (m_bClickLeft) {
					int delay = ptsCapture - m_ptsMouseLeftAction;
					delay = delay / 5 * 5;
					if (delay <= MOUSE_TIME) {
						bDrawHotDot = FALSE;
						//绘制鼠标左键动画
						m_cursorLeftFrame.DrawCircle2(m_mouse.m_colorLeft, delay);
						RgbaData::MixRect(m_pScreenFrame.GetFrame(),
							m_cursor.cursor_pos.x - m_cursorLeftFrame.GetWidth() / 2,
							m_cursor.cursor_pos.y - m_cursorLeftFrame.GetHeight() / 2,
							m_iAlphaAnimation,
							m_cursorLeftFrame.GetFrame());
						//叠加鼠标图像 
						m_cursor.Draw(m_pScreenFrame.GetFrame(),
							m_ptCursor.x,
							m_ptCursor.y);
					}
					else if (delay <= MOUSE_TIME + 100) {
						bDrawHotDot = FALSE;
						//叠加鼠标图像 
						m_cursor.Draw(m_pScreenFrame.GetFrame(),m_ptCursor.x,m_ptCursor.y);
					}
					else {
						m_bClickLeft = FALSE;
					}
				}

				if (m_bClickRight) {
					int delay = ptsCapture - m_ptsMouseRightAction;
					delay = delay / 5 * 5;
					if (delay <= MOUSE_TIME) {
						bDrawHotDot = FALSE;
						//绘制鼠标右键动画
						m_cursorRightFrame.DrawCircle2(m_mouse.m_colorRight, delay);
						RgbaData::MixRect(m_pScreenFrame.GetFrame(),
							m_cursor.cursor_pos.x - m_cursorRightFrame.GetWidth() / 2,
							m_cursor.cursor_pos.y - m_cursorRightFrame.GetHeight() / 2,
							m_iAlphaAnimation,
							m_cursorRightFrame.GetFrame());
						//叠加鼠标图像 
						m_cursor.Draw(m_pScreenFrame.GetFrame(),m_ptCursor.x,m_ptCursor.y);
					}
					else if (delay <= MOUSE_TIME + 100) {
						bDrawHotDot = FALSE;
						//叠加鼠标图像 
						m_cursor.Draw(m_pScreenFrame.GetFrame(),m_ptCursor.x,m_ptCursor.y);
					}
					else {
						m_bClickRight = FALSE;
					}
				}
			}
			
			//鼠标热点
			if (bDrawHotDot && m_mouse.m_bMouseHotdot) {
				RgbaData::MixRect(m_pScreenFrame.GetFrame(),
					m_cursor.cursor_pos.x - m_cursorHotdotFrame.GetWidth() / 2,
					m_cursor.cursor_pos.y - m_cursorHotdotFrame.GetHeight() / 2,
					m_iAlphaHotdot,
					m_cursorHotdotFrame.GetFrame());
			}

			//叠加鼠标图像 
			m_cursor.Draw(m_pScreenFrame.GetFrame(), m_ptCursor.x, m_ptCursor.y);
		}
	}
}

int MixerCapture::DXGIGrabFrame(WXVideoFrame* frame) {

	if (DxgiGrab()) {

		if (WXMediaAVFrameIsBlackRGB32(m_pScreenBase.GetFrame())) {
			return 0;//纯黑帧！！
		}

		libyuv::ARGBCopy(m_pScreenBase.GetFrame()->data[0],
			m_pScreenBase.GetFrame()->linesize[0],
			m_pScreenFrame.GetFrame()->data[0],
			m_pScreenFrame.GetFrame()->linesize[0],
			m_iScreenWidth,
			m_iScreenHeight
		);

		m_bMouseVisable = FALSE;//鼠标信息
		if (m_bUseMouse) {
			//m_cursor.Capture();//
			m_bMouseVisable = m_cursor.m_visible;//是否可见
		}

		// 在屏幕采集数据上画鼠标
		DrawMouseInfo(m_ptsVideo);//绘制鼠标
		//绘制图片、文字水印
		int PosX = m_video.m_bFollowMouse ? (m_cursor.cursor_pos.x - m_iRectW / 2) : m_iRectPosX;
		int PosY = m_video.m_bFollowMouse ? (m_cursor.cursor_pos.y - m_iRectH / 2) : m_iRectPosY;
		DrawDxgiInfo(PosX, PosY);

		if (m_iRectPosX == 0 &&
			m_iRectPosY == 0 &&
			m_iRectW == m_iScreenWidth &&
			m_iRectH == m_iScreenHeight) {
			//全屏图像，直接输出
			av_frame_copy(frame->GetFrame(), m_pScreenFrame.GetFrame());
		}
		else { //区域录制

			if (m_iRectW == m_iWidth && m_iRectH == m_iHeight) { //不缩放处理
				RgbaData::CopyRect(m_pScreenFrame.GetFrame(), PosX, PosY, frame->GetFrame()); //把图像拷贝到区域
			}else { //缩放处理
				m_RectFrame.Init(AV_PIX_FMT_RGB32, m_iRectW, m_iRectH);
				RgbaData::CopyRect(m_pScreenFrame.GetFrame(), PosX, PosY, m_RectFrame.GetFrame()); //把图像拷贝到区域
				libyuv::ARGBScale(m_RectFrame.GetFrame()->data[0], m_RectFrame.GetFrame()->linesize[0],
					m_RectFrame.GetFrame()->width, m_RectFrame.GetFrame()->height,
					frame->GetFrame()->data[0], frame->GetFrame()->linesize[0],
					frame->GetFrame()->width, frame->GetFrame()->height,
					libyuv::FilterMode::kFilterBilinear);
			}
		}
		return 1;
	}
	return 0;
}

BOOL MixerCapture::DxgiGrab() {

	if (m_pDesktopDevice == nullptr) {
		OpenDevice(L"Normal");//重新打开设备
	}

	if (m_pDesktopDevice == nullptr) {
		//重新打开设备失败
		return FALSE;
	}

	CComPtr<IDXGIResource>DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	memset(&FrameInfo, 0, sizeof(FrameInfo));
	HRESULT hr = m_pDesktopDevice->AcquireNextFrame(0, &FrameInfo, &DesktopResource);//获取桌面数据
	if (hr == DXGI_ERROR_ACCESS_LOST) { // 游戏启动过程会独占设备
       //WXLogW(L"DXGI_ERROR_ACCESS_LOST!!");
		SLEEPMS(1000);
		OpenDevice(L"DXGI_ERROR_ACCESS_LOST");//重新打开设备
		return DxgiGrab();
	}
	else if (hr == DXGI_ERROR_WAIT_TIMEOUT) { //获取超时

		//LogW(L"DXGI_ERROR_WAIT_TIMEOUT!!");
		//继续使用上一帧的数据
		m_nTimeOutIndex++;
		if (m_nTimeOutIndex < 30) {
			m_nVideoFrameTimeOut++;
			return !!m_pTexture;
		}else {
			//连续多次采集超时
			m_nTimeOutIndex = 0;
			OpenDevice(L"DXGI_ERROR_WAIT_TIMEOUT");//重新打开设备
			return DxgiGrab();
		}
	}
	else if (SUCCEEDED(hr)) { //正常采集
		m_nTimeOutIndex = 0;

		//DXGI 有自己的鼠标绘制逻辑
		m_cursor.m_visible = FrameInfo.PointerPosition.Visible;//鼠标是否可见

		//更新鼠标坐标
		m_cursor.cursor_pos.x = FrameInfo.PointerPosition.Position.x;//鼠标坐标X
		m_cursor.cursor_pos.y = FrameInfo.PointerPosition.Position.y;//鼠标坐标Y

		if (FrameInfo.PointerShapeBufferSize > 0 && m_cursor.m_visible) {

			if (FrameInfo.PointerShapeBufferSize > m_cursor.m_dxgiMouseBuf.m_iBufSize) {
				m_cursor.m_dxgiMouseBuf.Init(nullptr, FrameInfo.PointerShapeBufferSize);//更新缓冲区
				WXLogA("DXGI Mouse buffer Resize to %d", (int)FrameInfo.PointerShapeBufferSize);
			}

			//PointerShapeBufferSize > 0 表示有新的鼠标
			UINT BufferSizeRequired = 0;
			DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo = {0};
			HRESULT hr = m_pDesktopDevice->GetFramePointerShape(FrameInfo.PointerShapeBufferSize, reinterpret_cast<VOID*>(m_cursor.m_dxgiMouseBuf.GetBuffer()), &BufferSizeRequired, &ShapeInfo);
			if (S_OK == hr && BufferSizeRequired >0 && BufferSizeRequired <= FrameInfo.PointerShapeBufferSize) {
				int nW    = ShapeInfo.Width;
				int nH    = (ShapeInfo.Type == (int)DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)? ShapeInfo.Height / 2 : ShapeInfo.Height;
				m_cursor.m_nType = ShapeInfo.Type;//鼠标类型
				m_cursor.m_nW = nW;//鼠标大小
				m_cursor.m_nH = nH;
			}
		}

		CComPtr<ID3D11Texture2D>AcquiredDesktopImage = nullptr;
		hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&AcquiredDesktopImage));//获取桌面数据
		if (SUCCEEDED(hr)) {
			D3D11_TEXTURE2D_DESC frameDescriptor;
			AcquiredDesktopImage->GetDesc(&frameDescriptor); //注意一下采集的数据格式是否 DXGI_FORMAT_B8G8R8A8_UNORM
			if (frameDescriptor.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
				m_nError++;
				//LogW(L"DXGI Error Format=%d", (int)frameDescriptor.Format);
				m_pDesktopDevice->ReleaseFrame();
				AcquiredDesktopImage = nullptr;
				DesktopResource = nullptr;
				return FALSE;
			}

			frameDescriptor.Usage = D3D11_USAGE_STAGING;
			frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			frameDescriptor.BindFlags = 0;
			frameDescriptor.MiscFlags = 0;
			frameDescriptor.MipLevels = 1;
			frameDescriptor.ArraySize = 1;
			frameDescriptor.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			frameDescriptor.SampleDesc.Count = 1;
			//2019.01.08 修复因为UAC造成的停止视频录制问题
			if (m_pTexture == nullptr ||  //UAC操作之后为NULL
				m_tmpWidth != frameDescriptor.Width || //大小不匹配，比如某些游戏造成的问题
				m_tmpHeight != frameDescriptor.Height) {
				m_tmpWidth  = frameDescriptor.Width;//当前抓取的图像大小
				m_tmpHeight = frameDescriptor.Height;
				m_nError++;
				//LogW(L"DXGI NeSize=[%dx%d]", m_tmpWidth, m_tmpHeight);
				m_pTexture = nullptr;
				hr = m_pDev->CreateTexture2D(&frameDescriptor, nullptr, &m_pTexture);//创建内存表面
			}

			if (m_pTexture == nullptr) {  //创建显存失败！
				m_nError++;
				LogW(L" %ws %d Fail", __FUNCTIONW__, __LINE__);

				m_pDesktopDevice->ReleaseFrame();
				AcquiredDesktopImage = nullptr;
				DesktopResource = nullptr;
				return FALSE;
			}

			m_pContext->CopyResource(m_pTexture, AcquiredDesktopImage);// 桌面拷贝数据到显存

			m_pDesktopDevice->ReleaseFrame();
			AcquiredDesktopImage = nullptr;
			DesktopResource = nullptr;

			D3D11_MAPPED_SUBRESOURCE mappedRect = {};   //显存到内存
			hr = m_pContext->Map(m_pTexture, 0, D3D11_MAP_READ, 0, &mappedRect);//类似于D3D LockRect
			if (SUCCEEDED(hr)) {
				if (m_tmpWidth != m_iScreenWidth || m_tmpHeight != m_iScreenHeight) {
					//分辨率被切换，一般是在游戏中
					//可能会造成鼠标/水印等样式不准
					libyuv::ARGBScale((uint8_t*)mappedRect.pData, mappedRect.RowPitch,
						m_tmpWidth, m_tmpHeight,
						m_pScreenBase.GetFrame()->data[0], m_pScreenBase.GetFrame()->linesize[0],
						m_iScreenWidth, m_iScreenHeight,
						libyuv::kFilterLinear);
				}else {
					libyuv::ARGBCopy((uint8_t*)mappedRect.pData, mappedRect.RowPitch,
						m_pScreenBase.GetFrame()->data[0], m_pScreenBase.GetFrame()->linesize[0],
						m_iScreenWidth, m_iScreenHeight);
				}
				m_pContext->Unmap(m_pTexture, 0);//不做操作
				return TRUE;//成功抓图
			}
		}
	}
	else {
		m_nError++;
		WXLogA("%s HR = %x", __FUNCTION__, hr);
		SLEEPMS(1000);
		OpenDevice(L"Other HR");//重新打开设备
		return DxgiGrab();
	}
	return FALSE;
}


#endif