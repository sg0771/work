/*
音视频采集后封装为多媒体文件格式
*/

#include "FfmpegMuxer.h"
#include "WXCapture.h"
#include "AudioMixer.h"
#include "WasapiDevice.h"
#include "VideoSource.h"
#include <WXMediaCpp.h>




//音频数据回调操作
EXTERN_C void   _onAudioFrame(void *ctx, uint8_t *buf, int size) {
	FfmpegMuxer* pMuxer = (FfmpegMuxer*)ctx;
	pMuxer->WriteAudioFrame(buf,size);
}

//视频数据回调操作
EXTERN_C void   _onVideoFrame(void *ctx, AVFrame *avframe) {
	FfmpegMuxer* pMuxer = (FfmpegMuxer*)ctx;
	pMuxer->WriteVideoFrame(avframe);
}

void WXCapture::GetPicture(int type, WXCTSTR wszName, int quality) {
	WXVideoCaptureGetPicture(m_pWXVideo[0], type, wszName, quality);
}

int   WXCapture::CheckAudioParam(TWXAudioConfig& _audioParam,
	WXCTSTR wszExt) {
	int hasAudio = _audioParam.has_audio;
	if (hasAudio) { //检查设备和参数，查询是否支持音频采集
		if (WXStrcmp(wszExt, _T("jpg")) == 0 ||
			WXStrcmp(wszExt, _T("gif")) == 0) {
			hasAudio = false;
			WXLogW(L"FfmpegMuxer Not have Audio");
		}
		if (WXWasapiGetRenderCount() == 0 &&
			WXWasapiGetCaptureCount() == 0) {
			hasAudio = false;
			WXLogW(L"Not hav Audio Device");
		}
		if (WXStrlen(_audioParam.m_micName) == 0 &&
			WXStrlen(_audioParam.m_systemName) == 0) {
			hasAudio = false;
			WXLogW(L"Not hav Audio Device Name");
		}
		if (WXStrcmp(_audioParam.m_micName, L"nullptr") == 0 &&
			WXStrcmp(_audioParam.m_systemName, L"nullptr") == 0) {
			hasAudio = false;
			WXLogW(L"Not hav Audio Device Name");
		}
	}

	return hasAudio;
}
//音频采集
int  WXCapture::AddAudioEndpoint(const TWXAudioConfig& _audioParam,
	FfmpegMuxer* pMuxer) {
	if (m_pWXAudio == nullptr) {
		m_pWXAudio = WXAudioCaptureCreate();
		WXAudioCaptureSetCond(m_pWXAudio, (void*)(&m_condStart), &m_bStartFlag);//设置信号量
		WXAudioCaptureSetSampleRate(m_pWXAudio, _audioParam.nSampleRate);
		WXAudioCaptureSetChannel(m_pWXAudio, _audioParam.nChannel);
		WXAudioCaptureSetSystemDevice(m_pWXAudio,
			_audioParam.m_systemName, 
			_audioParam.nSystemLevel, _audioParam.nSystemScale);
		WXAudioCaptureSetMicDevice(m_pWXAudio, 
			_audioParam.m_micName, _audioParam.nMicLevel,
			_audioParam.nMicScale,
			_audioParam.bAGC, _audioParam.bNS, _audioParam.bVAD);
		WXAudioCaptureSetSink(m_pWXAudio, pMuxer, _onAudioFrame);
		int ret = WXAudioCaptureInit(m_pWXAudio);
		if (ret != WX_ERROR_SUCCESS) {
			WXLogW(L"No Video Data");
			WXAudioCaptureDestroy(m_pWXAudio);
			m_pWXAudio = nullptr;
			return 0;
		}
		else { //OK
			pMuxer->SetAudioConfig(_audioParam.nSampleRate,
				_audioParam.nChannel,
				_audioParam.nBitarte, 
				_audioParam.codec);
			return 1;
		}
	}
	else {
		WXAudioCaptureSetSink(m_pWXAudio, pMuxer, _onAudioFrame);
		pMuxer->SetAudioConfig(_audioParam.nSampleRate,
			_audioParam.nChannel,
			_audioParam.nBitarte,
			_audioParam.codec);
		return 1;
	}
	return 1;
}


int WXCapture::CheckVideoParam(VideoDeviceParam& _videoParam, WXCTSTR wszExt) {
	int hasVideo = _videoParam.m_bUse;
	if (hasVideo) {
		if (_videoParam.m_iBitrate < 0)
			_videoParam.m_iBitrate = 0;
		if (WXStrcmp(wszExt, _T("wav")) == 0 ||
			WXStrcmp(wszExt, _T("wma")) == 0 ||
			WXStrcmp(wszExt, _T("aac")) == 0 ||
			WXStrcmp(wszExt, _T("m4a")) == 0 ||
			WXStrcmp(wszExt, _T("flac")) == 0 ||
			WXStrcmp(wszExt, _T("mp3")) == 0) {
			hasVideo = false;
			WXLogW(L"FfmpegMuxer Not have Video");
		}
	}
	return hasVideo;
}
int WXCapture::AddVideoEndpoint(WXCaptureMode _mode,
	VideoDeviceParam* _videoParam,
	TextWaterMarkParam* _textParam,
	ImageWaterMarkParam* _imageParam,
	MouseParam* _mouseParam,
	FfmpegMuxer* pMuxer) {
	void* pWXVideo = WXVideoCaptureCreate(); //添加视频设备
	WXVideoCaptureSetCond(pWXVideo, (void*)(&m_condStart), &m_bStartFlag);//设置信号量
	WXVideoCaptureSetDataSink(pWXVideo, pMuxer, _onVideoFrame);//数据回调
	WXVideoCaptureSetEventSink(pWXVideo, m_pEventSink, m_cbEvent);//消息回调
	WXVideoCaptureSetVideoParam(pWXVideo, _videoParam);//视频采集参数
	WXVideoCaptureSetImageParam(pWXVideo, _imageParam);//图像水印
	WXVideoCaptureSetMouseParam(pWXVideo, _mouseParam);//鼠标图像
	WXVideoCaptureSetTextParam(pWXVideo, _textParam);//文字水印
	int ret = WXVideoCaptureUpdate(pWXVideo);
	if (ret != WX_ERROR_SUCCESS) {
		WXLogW(L"Open Video Device Error!!!");
		WXVideoCaptureDestroy(pWXVideo);
		pWXVideo = nullptr;
		return 0;
	}
	else {
		m_pWXVideo[m_nVideoCount++] = pWXVideo;
		//m_nVideoCount++;
		int nWidth = WXVideoCaptureGetWidth(pWXVideo);
		int nHeight = WXVideoCaptureGetHeight(pWXVideo);
		pMuxer->SetVideoConfig(nWidth, nHeight,
			(int)_videoParam->m_iFps,
			_videoParam->m_iBitrate,
			_mode,
			_videoParam->m_bUseHW,
			_videoParam->m_iForceFps,
			_videoParam->m_iAntiAliasing,
			m_config.nVideoScale,//缩放
			_videoParam->m_wszCodec);
	}
	return 1;
}

int  WXCapture::Create(TWXCaptureConfig* inputConfig) {
	WXLogA("%s Video Param ", __FUNCTION__);


	memcpy(&m_config, inputConfig, sizeof(TWXCaptureConfig));

	WXString strFileName;//文件名
	WXString strName;//文件名，不含后缀
	WXString strExt;//后缀名

	WXCHAR wszFileName[MAX_PATH]; //输出文件名
	wcscpy(wszFileName, m_config.m_wszFileName);

	m_strFileName = m_config.m_wszFileName;//


	//文件名处理
	strFileName.Format(wszFileName);//输出文件名
	wcsrchr(wszFileName, L'.')[0] = '\0';
	strName.Format(wszFileName);
	int  length = strFileName.length();
	int pos = 0;
	for (int i = length - 1; i >= 0; i--) {
		if (strFileName.str()[i] == L'.') {
			pos = i;
			break;
		}
	}
	strExt.Format(strFileName.Left(length - pos - 1)); //后缀名

	int have_video = TRUE;
	int have_audio = TRUE;

	if (WXStrcmp(strExt.str(), _T("wav")) == 0 ||
		WXStrcmp(strExt.str(), _T("wma")) == 0 ||
		WXStrcmp(strExt.str(), _T("aac")) == 0 ||
		WXStrcmp(strExt.str(), _T("m4a")) == 0 ||
		WXStrcmp(strExt.str(), _T("flac")) == 0 ||
		WXStrcmp(strExt.str(), _T("mp3")) == 0) {
		have_video = FALSE;
		m_config.m_video.m_bUse = 0;//
		WXLogW(L"FfmpegMuxer Not have Video");
	}

	if (WXStrcmp(strExt.str(), _T("gif")) == 0 ||
		WXStrcmp(strExt.str(), _T("bmp")) == 0 ||
		WXStrcmp(strExt.str(), _T("jpg")) == 0 ||
		WXStrcmp(strExt.str(), _T("tiff")) == 0) {
		have_audio = FALSE;
		m_config.m_audio.has_audio = 0;//
		WXLogW(L"FfmpegMuxer Not have Video");
	}



	m_pEventSink = m_config.m_sink;//回调对象，可能是C++对象
	m_cbEvent = m_config.m_cb;//结束回调，函数

	//录制所有设备
	WXScreenInit();

	WXBase::WstringList device_list = WXBase::string_splitstr(m_config.m_video.m_wszDevName, L";");;

	int nScreenCount = WXScreenGetCount();
	//如果大于1，最后一个是"FullScreen"
		//多屏录制，bRect复位为0
	if ((m_config.m_video.m_wszDevName, L"all") == 0 || device_list.size() > 1) {
		m_config.m_video.m_bRect = 0;
	}

	if ((device_list.size() <= 1 &&
		(wcsicmp(m_config.m_video.m_wszDevName, L"all") != 0))
		|| nScreenCount == 1 || !m_config.m_video.m_bUse) {
		//单设备，原来的逻辑
		FfmpegMuxer* pMuxer = new FfmpegMuxer; //文件混流器
		VideoDeviceParam   videoParam;//视频录制参数
		memcpy(&videoParam, &m_config.m_video, sizeof(videoParam));

		MonitorInfo* info = nullptr;
		if (nScreenCount == 1) {//只有一个屏幕
			info = WXScreenGetDefaultInfo();//默认设备名
		}
		else { //指定名字
			//虚拟屏幕逻辑 
			//2023.08.31
			info = WXScreenGetInfoByName(m_config.m_video.m_wszDevName);

			//如果指定切黑边处理，需要获取当前显示器的区域和分辨率，并截图一张计算黑边
			int bCutSizeMode = WXGetGlobalValue(L"CutSizeMode",0);
			if (bCutSizeMode == 1) {
				int width = info->width;
				int height = info->height;
				HDC hDC = ::GetDC(nullptr);
				HDC hMemDC = ::CreateCompatibleDC(NULL);
				void* pBits = nullptr;
				WXVideoFrame VideoFrame;
				VideoFrame.Init(AV_PIX_FMT_RGB32, width, height);
				HBITMAP hBitmap = ::CreateDIBSection(hMemDC, (const BITMAPINFO*)VideoFrame.GetBIH(),
					DIB_RGB_COLORS, (void**)&pBits, NULL, 0);

				HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
				BOOL bCopyDC = ::BitBlt(hMemDC, 0, 0, width, height,
					hDC, info->left, info->top, SRCCOPY);

				//获取 m_hMemDC RGBA 数据
				hBitmap = (HBITMAP)::SelectObject(hMemDC, hOldBitmap);

				//计算 pBits 的黑边
				uint32_t* pSrc = (uint32_t*)pBits;
				int dx = 0; //竖屏模式是左右黑边对齐

				for (int i = 0; i < width / 2; i++)
				{
					for (int j = 0; j < height; j++)
					{
						if (pSrc[j * width + i] != 0xFF000000) {
							dx = i;
							break;
						}
					}
					if (dx != 0)
						break;
				}

				if(dx != 0){ //切黑边
					videoParam.m_bRect = 1;
					videoParam.m_rcScreen.left = info->left + dx;	
					videoParam.m_rcScreen.right = info->left + width - dx;
					videoParam.m_rcScreen.top = info->top;
					videoParam.m_rcScreen.bottom = info->top + height;
				}
				SAFE_RELEASE_DC(nullptr, hDC)
				SAFE_DELETE_OBJECT(hBitmap)
				SAFE_DELETE_DC(hMemDC)
			}
		}
		wcscpy(videoParam.m_wszDevName, info->wszName);//设备名

		int hasVideo = CheckVideoParam(videoParam, strExt.str());
		if (hasVideo) {
			hasVideo = AddVideoEndpoint(
				m_config.m_mode,
				&videoParam,
				&m_config.m_text,
				&m_config.m_image,
				&m_config.m_mouse,
				pMuxer);
		}

		TWXAudioConfig audioParam;//音频录制参数，扩展模式  //2019.03.20 修改
		memcpy(&audioParam, &m_config.m_audio, sizeof(audioParam));
		int hasAudio = CheckAudioParam(audioParam, strExt.str());
		if (hasAudio) {
			hasAudio = AddAudioEndpoint(audioParam, pMuxer);
		}

		if (hasVideo == 0 && hasAudio == 0) {
			WXLogW(L"No Media data");
			return WX_EVENT_NO_DATA;
		}
		int retMuxer = pMuxer->Open(strFileName.str()); //创建Muxer编解码器,创建文件失败会直接退出
		if (retMuxer != WX_ERROR_SUCCESS) {
			Stop();
			WXLogW(L"Create File Failed = %ws, exit", strFileName.str());
			return retMuxer; //严重错误，crash。。
		}
		m_pMuxer[m_nMuxerCount++] = pMuxer;//只有一个输出
	}
	else if(m_config.m_video.m_bUse &&  wcsicmp(m_config.m_video.m_wszDevName,L"all") == 0) {
		
		//所有设备
		for (int index = 0; index < MIN(nScreenCount - 1,MAX_CAPTURE); index++)
		{
			WXString strTargetName;
			strTargetName.Format(L"%ws-%d.%s", 
				strName.str(),
				index + 1,
				strExt.str());

			FfmpegMuxer* pMuxer = new FfmpegMuxer; //文件混流器

			MonitorInfo* info = WXScreenGetInfo(index);
			VideoDeviceParam   videoParam;//视频录制参数
			memcpy(&videoParam, &m_config.m_video, sizeof(videoParam));

			if (!info->isPrimary) { //非主屏幕
				videoParam.m_bRect = 1;
				videoParam.m_rcScreen.left = info->left;
				videoParam.m_rcScreen.top = info->top;
				videoParam.m_rcScreen.right = info->left + info->width;
				videoParam.m_rcScreen.bottom = info->top + info->height;
				videoParam.m_bUseHW = 0;//主屏幕外只能用软编码
				if (videoParam.m_iFps > 24) {
					videoParam.m_iFps = 24;//主屏幕外最多24fps
				}
			}else { //主屏幕争取使用DXGI
				wcscpy(videoParam.m_wszDevName, L"default");
			}

			int hasVideo = CheckVideoParam(videoParam, strExt.str());
			if (hasVideo) {
				hasVideo = AddVideoEndpoint(
					m_config.m_mode,
					&videoParam,
					&m_config.m_text,
					&m_config.m_image,
					&m_config.m_mouse,
					pMuxer);
			}

			TWXAudioConfig audioParam;//音频录制参数，扩展模式  //2019.03.20 修改
			memcpy(&audioParam, &m_config.m_audio, sizeof(audioParam));
			int hasAudio = CheckAudioParam(audioParam, strExt.str());
			if (hasAudio) {
				hasAudio = AddAudioEndpoint(audioParam, pMuxer);
			}

			if (hasVideo == 0 && hasAudio == 0) {
				WXLogW(L"No Media data");
				return WX_EVENT_NO_DATA;
			}
			int retMuxer = pMuxer->Open(strTargetName.str()); //创建Muxer编解码器,创建文件失败会直接退出
			if (retMuxer != WX_ERROR_SUCCESS) {
				//continue; //严重错误，crash。。
				return retMuxer;//有一个错误就退出 2022.08.16
			}
			m_pMuxer[m_nMuxerCount++] = pMuxer;
		}
	
	}else if(m_config.m_video.m_bUse  && device_list.size() > 1) {
		//多设备
		for (int index = 0; index < device_list.size(); index++)
		{
			WXString strTargetName;
			strTargetName.Format(L"%ws-%d.%s",
				strName.str(),
				index + 1,
				strExt.str());

			FfmpegMuxer* pMuxer = new FfmpegMuxer; //文件混流器
			MonitorInfo* info = WXScreenGetInfoByName(device_list[index].c_str());
			VideoDeviceParam   videoParam;//视频录制参数
			memcpy(&videoParam, &m_config.m_video, sizeof(videoParam));
			if (!info->isPrimary) { //非主屏幕
				videoParam.m_bRect = 1;
				videoParam.m_rcScreen.left = info->left;
				videoParam.m_rcScreen.top = info->top;
				videoParam.m_rcScreen.right = info->left + info->width;
				videoParam.m_rcScreen.bottom = info->top + info->height;
				videoParam.m_bUseHW = 0;//主屏幕外只能用软编码
				if (videoParam.m_iFps > 24) {
					videoParam.m_iFps = 24;//主屏幕外最多24fps
				}
			}else { //主屏幕争取使用DXGI
				wcscpy(videoParam.m_wszDevName, L"default");
			}

			int hasVideo = CheckVideoParam(videoParam, strExt.str());
			if (hasVideo) {
				hasVideo = AddVideoEndpoint(
					m_config.m_mode,
					&videoParam,
					&m_config.m_text,
					&m_config.m_image,
					&m_config.m_mouse,
					pMuxer);
			}

			TWXAudioConfig audioParam;//音频录制参数，扩展模式  //2019.03.20 修改
			memcpy(&audioParam, &m_config.m_audio, sizeof(audioParam));
			int hasAudio = CheckAudioParam(audioParam, strExt.str());
			if (hasAudio) {
				hasAudio = AddAudioEndpoint(audioParam, pMuxer);
			}

			if (hasVideo == 0 && hasAudio == 0) {
				WXLogW(L"No Media data");
				return WX_EVENT_NO_DATA;
			}
			int retMuxer = pMuxer->Open(strTargetName.str()); //创建Muxer编解码器,创建文件失败会直接退出
			if (retMuxer != WX_ERROR_SUCCESS) {
				//continue; //严重错误，crash。。
				return retMuxer;//有一个错误就退出 2022.08.16
			}
			m_pMuxer[m_nMuxerCount++] = pMuxer;
		}
	}

	if (m_nMuxerCount == 0) {
		return WX_EVENT_NO_DATA;
	}
	return WX_ERROR_SUCCESS;//启动录制成功
}



void WXCapture::Start() {
	if (!m_bStartFlag && (m_pWXAudio || m_nVideoCount)) {

		g_bWXCaptureStopFlag = FALSE;//复位
		for (int i = 0; i < m_nVideoCount; i++) {
			WXVideoCaptureStart(m_pWXVideo[i]);//视频线程
		}

		if (m_pWXAudio) {
			int nFrameSize = m_pMuxer[0]->GetAudioFrameSize();
			WXAudioCaptureSetFrameSize(m_pWXAudio, nFrameSize);
			WXAudioCaptureStart(m_pWXAudio);//音频线程
		}

		//所有音视频采集和编码线程堵塞
		//WXLogW(L"+++++++ WXCapture WXCond_Notify");
		WXBase::WXCond_Notify(&m_condStart, &m_bStartFlag, TRUE);//激活所有音视频采集线程
	}
}

void WXCapture::Stop() {
	//BEGIN_LOG_FUNC
	if (m_bStartFlag) {
		m_bStartFlag = 0;
		for (int i = 0; i < m_nVideoCount; i++){
			if (m_pWXVideo[i]) {
				WXVideoCaptureStop(m_pWXVideo[i]);//音频线程
				WXVideoCaptureDestroy(m_pWXVideo[i]);//音频线程
				m_pWXVideo[i] = nullptr;
			}
		}

		if (m_pWXAudio) {
			WXAudioCaptureStop(m_pWXAudio);//音频线程
			WXAudioCaptureDestroy(m_pWXAudio);//音频线程
			m_pWXAudio = nullptr;
		}


		int bCloseStutas = WX_ERROR_SUCCESS;//结束状态
		for (int i = 0; i < m_nMuxerCount; i++){ //录制结束
			if (m_pMuxer[i]) {
				int ret = m_pMuxer[i]->Close();
				//提示录制成功或者失败！
				if (ret != 0) {
					bCloseStutas = ret;//有不成功的结束
				}
				delete m_pMuxer[i];
				m_pMuxer[i] = nullptr;
			}
		}

		//if (m_cbEvent) {
		//	WXLogA("WXCapture m_cbEvent Start");
		//	m_cbEvent(m_pEventSink, bCloseStutas, (void*)m_strFileName.str());//录制结束消息
		//	WXLogA("WXCapture m_cbEvent Stop");
		//}

		if (m_cbEvent) {
			void* pEventSink = m_pEventSink;//回调对象，可能是C++对象
			wxCallBack cbEvent = m_cbEvent;//事件回调
			std::wstring strName = m_strFileName.str();
			WXTask task = [cbEvent, pEventSink, bCloseStutas, strName] {
				cbEvent(pEventSink, bCloseStutas, (void*)(strName.c_str()));
			};
			WXTaskPost(OTHER_WORK_THREAD, task);
		}
		g_bWXCaptureStopFlag = TRUE;//复位
	}
}



int64_t WXCapture::GetMuxerInfo(int type) {
	if (m_pMuxer[0] == nullptr)return 0;
	switch (type){
	case  CAPTURE_TOTAL_TIME:
		return FFMAX(m_pMuxer[0]->GetVideoTime(),
			m_pMuxer[0]->GetAudioTime());
	case CAPTURE_VIDEO_TIME:
		return m_pMuxer[0]->GetVideoTime();
	case CAPTURE_VIDEO_SIZE:
		return m_pMuxer[0]->GetVideoSize();
	case CAPTURE_AUDIO_TIME:
		return m_pMuxer[0]->GetAudioTime();
	case CAPTURE_AUDIO_SIZE:
		return m_pMuxer[0]->GetAudioSize();
	case CAPTURE_TOTAL_SIZE:
		return m_pMuxer[0]->GetFileSize();
	case CAPTURE_VIDEO_FRAME:
		return m_pMuxer[0]->GetVideoFrameSize();
	default:
		break;
	}
	return 0;
}

int  WXCapture::GetAudioLevel(int bSystem) {
	return WXAudioCaptureGetAudioLevel(m_pWXAudio, bSystem);
}

void WXCapture::SetAudioLevel(int bSystem, int level) {
	WXAudioCaptureSetAudioLevel(m_pWXAudio, bSystem,level);
}

int   WXCapture::GetAudioScale(int bSystem) {
	return WXAudioCaptureGetAudioScale(m_pWXAudio, bSystem);
}
void   WXCapture::SetAudioScale(int bSystem, int nScale){
	WXAudioCaptureSetAudioScale(m_pWXAudio, bSystem, nScale);
}

void WXCapture::ChangeRect(int x, int y, int w, int h) {
	WXVideoCaptureChangeRect(m_pWXVideo[0], x, y, w, h);
}

void WXCapture::Pause() {
	WXLogW(L" WXCapture pause ");
	for (int i = 0; i < m_nMuxerCount; i++) { //暂停
		m_pMuxer[i]->Pause(TRUE);
	}
	for (int i = 0; i < m_nVideoCount; i++) {
		WXVideoCapturePause(m_pWXVideo[i], 1);
	}
}

void WXCapture::Resume() {
	WXLogW(L" WXCapture Resume ");
	for (int i = 0; i < m_nMuxerCount; i++){
		m_pMuxer[i]->Pause(FALSE);
	}
	for (int i = 0; i < m_nVideoCount; i++) {
		WXVideoCapturePause(m_pWXVideo[i], 0);
	}
}

int64_t WXCapture::GetVideoTimeOut() {
	return 0;
}

