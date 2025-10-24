#include "WXCapture.h"

//设置默认录制参数
WXMEDIA_API  void WXCaptureDefaultParam(WXCaptureParam* param) {
	if (param) {
		param->m_sink = nullptr;
		param->m_cb = nullptr;
        param->m_mode = MODE_NORMAL;//正常模式
		memset(param->m_wszFileName, 0, MAX_PATH * sizeof(WXCHAR));
        
		memset(param->m_audio.m_systemName, 0, MAX_PATH * sizeof(wchar_t));
		WXStrcpy(param->m_audio.m_systemName, _T("nullptr"));	//不启用

		memset(param->m_audio.m_micName, 0, MAX_PATH * sizeof(wchar_t));
		WXStrcpy(param->m_audio.m_micName, _T("nullptr"));//不启用
        
		memset(param->m_audio.codec, 0, 16 * sizeof(wchar_t));

		param->m_audio.has_audio = 1;
		param->m_audio.bAGC = 0;
		param->m_audio.bNS = 0;
        
        memset(param->m_video.m_wszCodec, 0, 16 * sizeof(wchar_t));
		memset(param->m_video.m_wszDevName, 0, MAX_PATH * sizeof(WXCHAR));
		param->m_video.m_bUse = 0;
		param->m_video.m_iFps = 24;
		param->m_video.m_iBitrate = 1000 * 1000;//bitrate or qp
		param->m_video.m_bCamera = 0;
		param->m_video.m_iCameraWidth = 0;
		param->m_video.m_iCameraHeight = 0;
		param->m_video.m_hwndPreview = nullptr;

		param->m_video.onVideoData = nullptr;//数据回调
#ifdef _WIN32
		param->m_video.m_bFollowMouse = 0;
		param->m_video.m_bRect = 0;
		param->m_video.m_rcScreen.left = 0;
		param->m_video.m_rcScreen.top = 0;
		param->m_video.m_rcScreen.right = 100;
		param->m_video.m_rcScreen.bottom = 100;
#endif
		param->m_video.m_bCaptureBlt = 0;
		param->m_video.m_bForceHDC = 0;//GDI 采集是否每次都申请内存DC
		param->m_video.m_bUseHW = 0;//是否使用H264 QSV编码器
		param->m_video.m_bDXGI = 0;//是否使用DXGI 桌面采集

		param->m_video.m_iForceFps = 0;//插帧处理
		param->m_video.m_iAntiAliasing = 0;//抗锯齿处理

		param->m_text.m_bUsed = false;
#ifdef _WIN32
		memset(param->m_text.m_wszText, 0, MAX_PATH * sizeof(WXCHAR));
		WXStrcpy(param->m_text.m_wszText, _T("Apowersoft"));
		param->m_text.m_iColor = RGB(255, 0, 255); //文字颜色
		param->m_text.m_BkColor = RGB(0, 0, 0); //背景颜色
		param->m_text.m_iPosX = 000; //文字水印位置X
		param->m_text.m_iPosY = 200; //文字水印位置Y
		memset(param->m_text.m_wszFontName, 0, MAX_PATH * sizeof(WXCHAR));
		WXStrcpy(param->m_text.m_wszFontName, _T("Microsoft YaHei"));
		param->m_text.m_iFontSize = 72;
		param->m_text.m_nStyle = 0;
		
        param->m_text.m_hfont = nullptr;
#endif

		param->m_image.m_bUsed = false; //是否使用图像水印
        param->m_mouse.m_iUsed = false;
        
#ifdef _WIN32
		memset(param->m_image.m_wszFileName, 0, MAX_PATH * sizeof(wchar_t));
		param->m_image.m_fAlpha = 1.0 /*0.73579f*/;//透明度
		param->m_image.m_iPosX = 0; //指定图像位置 x
		param->m_image.m_iPosY = 0;  //指定图像位置 y
		param->m_image.m_iDelay = 0;

		param->m_mouse.m_bMouseHotdot = false;
		param->m_mouse.m_iHotdotRadius = 15;
		param->m_mouse.m_colorMouse = RGB(255, 255, 0);
		param->m_mouse.m_fAlphaHotdot = 0.5;// 0.73579f;//热点透明度

		//鼠标点击动画
		param->m_mouse.m_bMouseAnimation = false;
		param->m_mouse.m_iAnimationRadius = 30;
		param->m_mouse.m_colorLeft = RGB(255, 0, 0);
		param->m_mouse.m_colorRight = RGB(0, 0, 255);
		param->m_mouse.m_fAlphaAnimation = 1.0;// 0.73579f;//动画透明度
#endif
	}
}

//设置默认录制参数
WXMEDIA_API  void TWXCaptureConfigDefault(TWXCaptureConfig* param) {
	if (param) {
		////视频片头片尾设置
		//param->nParam[0] = 100;//nSpeed
		param->nVideoScale = 100;//nVideoScale

		//memset(param->wszHeadImage, 0, MAX_PATH * sizeof(WXCHAR));
		//param->nHeadDuration = 0;
		//memset(param->wszTrailImage, 0, MAX_PATH * sizeof(WXCHAR));
		//param->nTrailDuration = 0;

		param->m_sink = nullptr;
		param->m_cb = nullptr;
		memset(param->m_wszFileName, 0, MAX_PATH * sizeof(WXCHAR));
		memset(param->m_audio.m_systemName, 0, MAX_PATH * sizeof(wchar_t));
		WXStrcpy(param->m_audio.m_systemName, _T("nullptr"));	//不启用

		memset(param->m_audio.m_micName, 0, MAX_PATH * sizeof(wchar_t));
		WXStrcpy(param->m_audio.m_micName, _T("nullptr"));//不启用
		memset(param->m_audio.codec, 0, 16 * sizeof(wchar_t));

		param->m_audio.bAGC = 0;
		param->m_audio.bNS = 0;
		param->m_audio.bVAD = 0;

		param->m_audio.nBitarte = 128000;
		param->m_audio.nChannel = 2;
		param->m_audio.nSampleRate = 48000;

		param->m_audio.nMicLevel = 100;
		param->m_audio.nSystemLevel = 100;

		param->m_audio.nSystemScale = 100;
		param->m_audio.nMicScale = 100;

		//param->bUseTemp = 0;
		//memset(param->wszOtherFileName, 0, MAX_PATH * sizeof(WXCHAR));//第二路输出 

		memset(param->m_video.m_wszCodec, 0, 16 * sizeof(wchar_t));
		param->m_mode = MODE_NORMAL;//正常模式
		param->m_audio.has_audio = 1;
		memset(param->m_video.m_wszDevName, 0, MAX_PATH * sizeof(WXCHAR));
		param->m_video.m_bUse = 0;
		param->m_video.m_iFps = 24;
		param->m_video.m_iBitrate = 1000 * 1000;//bitrate or qp
		param->m_video.m_bCamera = 0;
		param->m_video.m_iCameraWidth = 0;
		param->m_video.m_iCameraHeight = 0;
		
		param->m_video.m_hwndPreview = nullptr;

		param->m_video.onVideoData = nullptr;//数据回调
#ifdef _WIN32
		param->m_video.m_bFollowMouse = 0;
		param->m_video.m_bRect = 0;
		param->m_video.m_rcScreen.left = 0;
		param->m_video.m_rcScreen.top = 0;
		param->m_video.m_rcScreen.right = 100;
		param->m_video.m_rcScreen.bottom = 100;
#endif
		param->m_video.m_bCaptureBlt = 0;
		param->m_video.m_bForceHDC = 0;//GDI 采集是否每次都申请内存DC
		param->m_video.m_bUseHW = 0;//是否使用H264 QSV编码器
		param->m_video.m_bDXGI = 0;//是否使用DXGI 桌面采集

		param->m_video.m_iForceFps = 0;//恒定帧率
		param->m_video.m_iAntiAliasing = 0;//抗锯齿处理

		param->m_text.m_bUsed = false;


		param->m_image.m_bUsed = false; //是否使用图像水印
        param->m_mouse.m_iUsed = false;
#ifdef _WIN32
        memset(param->m_text.m_wszText, 0, MAX_PATH * sizeof(WXCHAR));
        WXStrcpy(param->m_text.m_wszText, _T("Apowersoft"));
        param->m_text.m_iColor = RGB(255, 0, 255); //文字颜色
        param->m_text.m_BkColor = RGB(0, 0, 0); //背景颜色
        param->m_text.m_iPosX = 0; //文字水印位置X
        param->m_text.m_iPosY = 200; //文字水印位置Y
        memset(param->m_text.m_wszFontName, 0, MAX_PATH * sizeof(WXCHAR));
		const wchar_t *wzsFont = _T("Microsoft YaHei");
        wcscpy(param->m_text.m_wszFontName, wzsFont);
        param->m_text.m_iFontSize = 72;
        param->m_text.m_nStyle = 0;
        param->m_text.m_hfont = nullptr;
        memset(param->m_image.m_wszFileName, 0, MAX_PATH * sizeof(wchar_t));
		param->m_image.m_fAlpha = 1.0 /*0.73579f*/;//透明度
		param->m_image.m_iPosX = 0; //指定图像位置 x
		param->m_image.m_iPosY = 0;  //指定图像位置 y
		param->m_image.m_iDelay = 0;

		param->m_mouse.m_bMouseHotdot = false;
		param->m_mouse.m_iHotdotRadius = 15;
		param->m_mouse.m_colorMouse = RGB(255, 255, 0);
		param->m_mouse.m_fAlphaHotdot = 0.5;// 0.73579f;//热点透明度

											//鼠标点击动画
		param->m_mouse.m_bMouseAnimation = false;
		param->m_mouse.m_iAnimationRadius = 30;
		param->m_mouse.m_colorLeft = RGB(255, 0, 0);
		param->m_mouse.m_colorRight = RGB(0, 0, 255);
		param->m_mouse.m_fAlphaAnimation = 1.0;// 0.73579f;//动画透明度
#endif
	}
}

static  void TWXCaptureConfigCopy(TWXCaptureConfig *paramExt, const WXCaptureParam *param) {
	paramExt->m_sink = param->m_sink;
	paramExt->m_cb = param->m_cb;
	memcpy(paramExt->m_wszFileName, param->m_wszFileName, MAX_PATH * sizeof(WXCHAR));
	paramExt->m_mode = param->m_mode;
	memcpy(&paramExt->m_audio, &param->m_audio, sizeof(AudioDeviceParam));
	memcpy(&paramExt->m_video, &param->m_video, sizeof(VideoDeviceParam));
	memcpy(&paramExt->m_text,  &param->m_text,  sizeof(TextWaterMarkParam));
	memcpy(&paramExt->m_image, &param->m_image, sizeof(ImageWaterMarkParam));
	memcpy(&paramExt->m_mouse, &param->m_mouse, sizeof(MouseParam));
}

static  void TWXCaptureConfigLog(const TWXCaptureConfig* param) {
#ifdef _WIN32
	//BEGIN_LOG_FUNC
	WXLogW(L"Filename = %ws", param->m_wszFileName);

	wchar_t wszMode[3][10] = { L"FAST",L"NORMAL",L"BEST" };
	WXLogW(L"Record Mode = %ws", wszMode[(int)param->m_mode]);

	WXLogW(L"WXCaptureParamLog Audio = %d", (int)param->m_audio.has_audio);
	if (param->m_audio.has_audio) {
		WXLogW(L"\t System Guid = %ws", param->m_audio.m_systemName);
		WXLogW(L"\t Mic GUID = %ws", param->m_audio.m_micName);
		WXLogW(L"\t Audio Codec = %ws", param->m_audio.codec);
		WXLogW(L"\t AGC = %d", param->m_audio.bAGC);
		WXLogW(L"\t NS = %d", param->m_audio.bNS);

		WXLogW(L"\t VAD = %d", param->m_audio.bVAD);
		WXLogW(L"\t SampleRate = %d", param->m_audio.nSampleRate);
		WXLogW(L"\t Channel = %d", param->m_audio.nChannel);
		WXLogW(L"\t System Level = %d", param->m_audio.nSystemLevel);

		WXLogW(L"\t Mic Level = %d", param->m_audio.nMicLevel);
		WXLogW(L"\t System Scale = %d", param->m_audio.nSystemScale);
		WXLogW(L"\t Mic Scale = %d", param->m_audio.nMicScale);
		WXLogW(L"\t Audio Bitarte = %d", param->m_audio.nBitarte);
		//New Log
	}


	WXLogW(L"WXCaptureParamLog Video = %d", (int)param->m_video.m_bUse);
	if (param->m_video.m_bUse) {
		WXLogW(L"\t Video Fps = %d", (int)param->m_video.m_iFps);

		WXLogW(L"\t Use Hardware Codec = %d", (int)param->m_video.m_bUseHW);
		WXLogW(L"\t Video Codec = %ws", param->m_video.m_wszCodec);
		WXLogW(L"\t Video Bitrate = %d", (int)param->m_video.m_iBitrate);
		WXLogW(L"\t Video Device Name = %ws", param->m_video.m_wszDevName);
		WXLogW(L"\t bCamera = %d", (int)param->m_video.m_bCamera);
		WXLogW(L"\t CameraWidth = %d", (int)param->m_video.m_iCameraWidth);
		WXLogW(L"\t CameraHeight = %d", (int)param->m_video.m_iCameraHeight);

		WXLogW(L"\t bDXGI = %d", (int)param->m_video.m_bDXGI);
        

		WXLogW(L"\t bRect = %d", (int)param->m_video.m_bRect);
		if (param->m_video.m_bRect) {
			WXLogW(L"\t Rect = [%d,%d,%d,%d]",
				(int)param->m_video.m_rcScreen.left,
				(int)param->m_video.m_rcScreen.top,
				(int)param->m_video.m_rcScreen.right,
				(int)param->m_video.m_rcScreen.bottom
			);
		}
		WXLogW(L"\t FollowMouse = %d", param->m_video.m_bFollowMouse);
		WXLogW(L"\t CaptureBlt = %d", param->m_video.m_bCaptureBlt);
		WXLogW(L"\t ForceHDC = %d", param->m_video.m_bForceHDC);
		WXLogW(L"\t ForceFps = %d", param->m_video.m_iForceFps);
		WXLogW(L"\t AntiAliasing = %d", param->m_video.m_iAntiAliasing);
	}

	WXLogW(L"WXCaptureParamLog Text=%d", param->m_text.m_bUsed);
	if (param->m_text.m_bUsed) {
		WXLogW(L"\tm_iPosX = %d", param->m_text.m_iPosX);
		WXLogW(L"\tm_iPosY = %d", param->m_text.m_iPosY);
		WXLogW(L"\tm_nStyle = %02x", param->m_text.m_nStyle);
		WXLogW(L"\tText = %ws", param->m_text.m_wszText);
		WXLogW(L"\tm_iColor = %08x", param->m_text.m_iColor);
		WXLogW(L"\tm_BkColor = %08x", param->m_text.m_BkColor);
		WXLogW(L"\tFont Name = %ws", param->m_text.m_wszFontName);
		WXLogW(L"\tm_iFontSize = %d", param->m_text.m_iFontSize);
	}

	WXLogW(L"WXCaptureParamLog Image=%d", param->m_image.m_bUsed);
	if (param->m_image.m_bUsed) {
		WXLogW(L"\tm_iPosX = %d", param->m_image.m_iPosX);
		WXLogW(L"\tm_iPosY = %d", param->m_image.m_iPosY);
		WXLogW(L"\tm_iDelay = %d", param->m_image.m_iDelay);
		WXLogW(L"\tm_fAlpha = %f", param->m_image.m_fAlpha);
		WXLogW(L"\tImage FileName = %ws", param->m_image.m_wszFileName);
	}


	WXLogW(L"WXCaptureParamLog Mouse=%d", param->m_mouse.m_iUsed);
	if (param->m_mouse.m_iUsed) {
		if (param->m_mouse.m_bMouseHotdot) {
			WXLogW(L"\tm_bMouseHotdot = %d", param->m_mouse.m_bMouseHotdot);
			WXLogW(L"\tm_iHotdotRadius = %d", param->m_mouse.m_iHotdotRadius);
			WXLogW(L"\tm_colorMouse = %08x", param->m_mouse.m_colorMouse);
			WXLogW(L"\tm_fAlphaHotdot = %f", param->m_mouse.m_fAlphaHotdot);
		}

		if (param->m_mouse.m_bMouseAnimation) {
			WXLogW(L"\tm_bMouseAnimation = %d", param->m_mouse.m_bMouseAnimation);
			WXLogW(L"\tm_iAnimationRadius = %d", param->m_mouse.m_iAnimationRadius);
			WXLogW(L"\tm_colorLeft = %08x", param->m_mouse.m_colorLeft);
			WXLogW(L"\tm_colorRight = %08x", param->m_mouse.m_colorRight);
			WXLogW(L"\tm_fAlphaAnimation = %f", param->m_mouse.m_fAlphaAnimation);
		}
		WXLogW(L"\tm_mouse.m_fAlphaAnimation = %03f", param->m_mouse.m_fAlphaAnimation);
	}
#endif
}



//设置截取波形长度
static int s_arrWave[1024];
int s_lenWave = 0;
WXMEDIA_API void WXSetWaveLength(int n) {
	if (n < 0 || n > 250)return;
	s_lenWave = n;
	memset(s_arrWave, 0, 1024 * sizeof(int));
}

//获取波形数据
WXMEDIA_API int WXGetWaveData(int *pData) {
	//WXAutoLock al(&sWavLocker);
	if (s_lenWave > 0) {
		memcpy(pData, s_arrWave, s_lenWave * sizeof(int));
		return 1;
	}
	return 0;
}

//For S16
void  GetWaveDataImpl(uint8_t *data, int size) {
	if (s_lenWave > 0) {
		int count = size / 2;
		int count1 = count / s_lenWave;
		for (int i = 0; i < s_lenWave; i++) {
			int16_t *buf = (int16_t*)data + i * count1;
			int iMax = 0;
			for (int j = 0; j < count1; j++) {
				if (abs(buf[j]) > iMax)iMax = abs(buf[j]);
			}
			s_arrWave[i] = iMax;
		}
	}
}


//--------------------------------------------------------------
//启动录屏录音
//static WXLocker s_lockGlobal;//全局锁，主要用来锁住应该再界面上执行的API
WXMEDIA_API void*  WXCaptureStart(WXCaptureParam* param, int *error) {
	WXAutoLock al(s_lockGlobal);
	return WXCaptureStartWithLevel(param, error, 100, 100);
}

WXMEDIA_API void* WXCaptureStartExt2(TWXCaptureConfig* param, int* error) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	return WXCaptureStartExt(param,error);
}

WXMEDIA_API void* WXCaptureStartWithLevel(WXCaptureParam* param, int* error, int systemLevel, int micLevel) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	TWXCaptureConfig config;
	TWXCaptureConfigDefault(&config);
	config.m_audio.nSystemLevel = systemLevel;
	config.m_audio.nMicLevel = micLevel;
	TWXCaptureConfigCopy(&config, param);
	return WXCaptureStartExt(&config, error);
}

static int URLisNetStream(WXCTSTR url) {
	if (wcsncmp(url, L"rtsp://", 7) == 0 ||
		wcsncmp(url, L"rtmp://", 7) == 0 ||
		wcsncmp(url, L"http://", 7) == 0 ||
		wcsncmp(url, L"https://", 8) == 0 ||
		wcsncmp(url, L"udp://", 6) == 0 ||
		wcsncmp(url, L"tcp://", 6) == 0 ||
		wcsncmp(url, L"rtp://", 6) == 0)
		return 1;
	return 0;
}

//创建同名文件检测该目录是否可写入
//失败返回0，回调 文件不可创建的失败
//成功删除临时文件并返回1
static int URLisAvailable(WXCTSTR url) {
	WXString strFile = url;
	strFile += L".wxmedia.tmp";
	std::ofstream of(url/*strFile.str()*/);
	if (of.is_open()) {
		of.close();
		::DeleteFile(url/*strFile.str()*/);
		WXLogW(L"%ws %ws can create file !!", __FUNCTIONW__, url);
		return 1;
	}
	WXLogW(L"%ws %ws can not create file !!", __FUNCTIONW__, url);
	return 0;
}

WXMEDIA_API void* WXCaptureStartExt(TWXCaptureConfig* param, int *error) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	*error = 0;
	int bNetStream = URLisNetStream(param->m_wszFileName);
	if (!bNetStream) {  //检测目录是否可写
		int isAvailable = URLisAvailable(param->m_wszFileName);
		if (!isAvailable) {
			if (param->m_cb)
				param->m_cb(param->m_sink, WX_EVENT_CREATE_FILE, NULL);//文件创建失败回调
			return nullptr;
		}
	}
	//else {
	//	WXLogW(L"Output NetStream %ws !!", __FUNCTIONW__, param->m_wszFileName);
	//}

	WXCapture *new_capture = new WXCapture;
	if ( param->m_video.m_iFps <= 5) {
		param->m_video.m_bUseHW = 0;
	}

	TWXCaptureConfigLog(param); //把录屏参数log出来
	int error_code = new_capture->Create(param);//先Create 试一下参数是否正常
	if (error_code != WX_ERROR_SUCCESS) {
		//int64_t t2 = WXGetTimeMs() - t1;;
		//WXLogW(L"+++++%ws Error = [%d]", __FUNCTIONW__, error_code);
		SAFE_DELETE(new_capture);
		*error = error_code;//错误码
		if (param->m_cb)
			param->m_cb(param->m_sink, error_code, NULL);//失败回调
		return nullptr;
	}
	new_capture->Start();//音视频通道同时Start

	//int64_t t2 = WXGetTimeMs() - t1;;
	//WXLogW(L"+++++%ws End in Thread[%d]  Time=%d MS", __FUNCTIONW__, tid, (int)t2);
	return (void*)new_capture;
}



//关闭录屏录音
WXMEDIA_API void  WXCaptureStop(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	///BEGIN_LOG_FUNC;
	WXCaptureStopEx(ptr, 1);
}

WXMEDIA_API void WXCaptureStopEx(void* ptr, int bVip){
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	if (ptr) {
		WXCapture* cap = (WXCapture*)ptr;
		cap->Stop();
	}
}

//暂停录屏录音
WXMEDIA_API void  WXCapturePause(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->Pause();
	}
}


//恢复录屏录音
WXMEDIA_API void  WXCaptureResume(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->Resume();
	}
}

WXMEDIA_API void  WXCaptureGetPicture1(void* ptr, WXCTSTR wsz, int quality) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->GetPicture(1, wsz, quality);
	}
}

WXMEDIA_API void  WXCaptureGetPicture2(void* ptr, WXCTSTR wsz, int quality) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->GetPicture(2, wsz, quality);
	}
}

WXMEDIA_API void  WXCaptureChangeRect(void* ptr, int x, int y) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->ChangeRect(x, y);
	}
}

WXMEDIA_API void  WXCaptureChangeRect2(void* ptr, int x, int y, int w, int h) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->ChangeRect(x, y, w, h);
	}
}

//当前时间戳
WXMEDIA_API int64_t WXCaptureGetTime(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_TOTAL_TIME);
	}
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetVideoTime(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_VIDEO_TIME);
	}
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetAudioTime(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_AUDIO_TIME);
	}
	return 0;
}


WXMEDIA_API int64_t WXCaptureGetVideoSize(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_VIDEO_SIZE);
	}
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetFileSize(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_TOTAL_SIZE);
	}
	return 0;
}



WXMEDIA_API int64_t WXCaptureGetVideoFrame(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_VIDEO_FRAME);
	}
	return 0;
}


WXMEDIA_API int64_t WXCaptureGetAudioSize(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_AUDIO_SIZE);
	}
	return 0;
}

WXMEDIA_API int64_t WXCaptureGetAudioFrame(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetMuxerInfo(CAPTURE_AUDIO_FRAME);
	}
	return 0;
}

//获取音频设备音量
WXMEDIA_API int WXCaptureGetAudioLevel(void* ptr, int system) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetAudioLevel(system);
	}
	return 0;
}

//获取音频设备音量
WXMEDIA_API int WXCaptureGetSystemLevel(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetAudioLevel(TRUE);
	}
	return 0;
}

WXMEDIA_API int WXCaptureGetMicLevel(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetAudioLevel(FALSE);
	}
	return 0;
}


WXMEDIA_API void WXCaptureSetAudioLevel(void* ptr, int bSystem, int level) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioLevel(bSystem, level);
	}
}

WXMEDIA_API void WXCaptureSetSystemLevel(void* ptr, int level) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioLevel(TRUE, level);
	}
}


WXMEDIA_API int WXCaptureGetAudioScale(void* ptr, int bSystem) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		return cap->GetAudioScale(FALSE);
	}
	return 100;
}

WXMEDIA_API void WXCaptureSetAudioScale(void* ptr, int bSystem, int nScale) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioScale(bSystem, nScale);
	}
}

WXMEDIA_API void WXCaptureSetSystemSilence(void* ptr, int bSilence) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioLevel(TRUE, bSilence ? 0 : 100);
	}
}

WXMEDIA_API void WXCaptureSetMicLevel(void* ptr, int level) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioLevel(FALSE, level);
	}
}

WXMEDIA_API void WXCaptureSetMicSilence(void* ptr, int bSilence) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		WXCapture *cap = (WXCapture*)ptr;
		cap->SetAudioLevel(FALSE, bSilence ? 0 : 100);
	}
}



//查询是否需要结束录制，比如 窗口录制中的 窗口销毁或者窗口大小改变
int g_bWXCaptureStopFlag = FALSE;//初始化
WXMEDIA_API int WXCaptureGetStopFlag() {
	return g_bWXCaptureStopFlag;
}



static WXCapture* g_pWXCapture = nullptr;
// 2021.04.01
// Airplay、AMCast 投屏录制
// DataCaptureStart启动， 
// AirplayPush推送视频数据
// AudioDataPush 推送音频数据
// DataCaptureStop 结束
// DataCaptureStart 返回1 表示启动成功， 返回0 表示启动失败
// wszName 输出文件路径
// nWidth 输出宽度 
// nHeight 输出高度 
// nFps 视频帧率，默认30 
// nSoundType 音频类型 0，无音频，1 仅投屏声音， 2仅麦克风， 3 投屏声音+麦克风 
// bHw 是否硬编码   1硬编码，0软编码
// nVideoMode 画质， 0 最差， 1 一般， 2最好
// bAllowResize AirplayPush 是否允许输入不同分辨率的数据
// 比如Airplay 中会因为横竖屏的切换导致数据分辨率不一致 
int g_bAllowResize = 0;
WXMEDIA_API int     DataCaptureStart(WXCTSTR wszName, int nWidth, int nHeight, int nFps,
	int nSoundType, int bHw, int nVideoMode, int bAllowResize) {


	//BEGIN_LOG_FUNC;

	WXAutoLock al(s_lockGlobal);
	g_bAllowResize = bAllowResize;
	WXCaptureParam Param;
	WXCaptureDefaultParam(&Param);//默认初始化

	//输出文件
	wcscpy(Param.m_wszFileName, wszName);

	//音频
	Param.m_audio.has_audio = nSoundType > 0;
	
	if (nSoundType & 0x01) {
		wcscpy(Param.m_audio.m_systemName, L"data");//投屏声音
	}

	if (nSoundType & 0x02) {
		wcscpy(Param.m_audio.m_micName, L"default");//默认麦克风
	}

	//视频类型
	Param.m_video.m_bUse = 1;
	Param.m_video.m_bCamera = 1;
	wcscpy(Param.m_video.m_wszDevName, L"WX_AIRPLAY");

	//视频参数
	Param.m_mode = (WXCaptureMode)nVideoMode;//编码模式
	Param.m_video.m_bUseHW = bHw;
	Param.m_video.m_iCameraWidth = nWidth;
	Param.m_video.m_iCameraHeight = nHeight;
	Param.m_video.m_iFps = nFps;
	Param.m_video.m_iForceFps = 0;
	Param.m_video.m_iBitrate = 0;
	int error = 0;
	g_pWXCapture = (WXCapture*)WXCaptureStart(&Param, &error);
	return g_pWXCapture != nullptr;
}

WXMEDIA_API void    DataCaptureStop() {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC;
	if (g_pWXCapture) {
		g_pWXCapture->Stop();
		delete g_pWXCapture;
		g_pWXCapture = nullptr;
		g_bAllowResize = 0;
	}
}
