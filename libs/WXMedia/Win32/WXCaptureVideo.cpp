/*
录屏时的视频处理部分
*/
#include "VideoSource.h"


//招商银行的触摸屏使用DXGI采集会黑屏，要禁用DXGI录屏功能，只能用GDI
WXMEDIA_API void WXDisableDXGI(int b) {
	WXSetGlobalValue(L"DisableDXGI", !!b);
}

//默认值100
//游戏录制前设置
//录制结束后恢复为100
static int s_nScale = 100;
WXMEDIA_API void WXGameSetScale(int scale) {
	s_nScale = scale;
}

WXMEDIA_API int WXGameGetScale() {
	return s_nScale;
}


//视频处理部分
//数据来源于AirplayPush
//所有采集的输出最后都是AirplayPush,确保只有一个录制对象
class  WXCaptureVideo : public WXThread {
public:
	static WXCaptureVideo* s_pVideo /*= nullptr*/;

	static void __CameraDataPush(AVFrame* frame) {
		if (s_pVideo)
			s_pVideo->HandleFrame(frame);
	}
public:
	 int m_nCameraType = 0;      //叠加位置，默认右上角  
	 int m_nCameraWndWidth = 0;  //外部设置的数值
	 int m_nCameraWndHeight = 0; //根据传入的摄像头计算出来的摄像头区域的高度

	//I420 格式数据
	//大小和采集图像一样
	WXVideoFrame m_captureCameraFrame[2]; //摄像头渲染数据.pip-pop方式保存摄像头传进来的数据
	//摄像头数据进来的时候，如果pool已经没有数据了，那么就跳过，该帧数据忽略
	//如果可以pool 有数据， 取出frame后保存摄像头数据，保存到data_queue

	//合并视频和摄像头图像的时候
	//如果无法data_queue获得数据，就使用上一帧的图像
	//如果已经获得数据，把数据保存在当前帧之后，把对应frame还给pool

	WXVideoFrame m_cameraFrame;

	WXVideoFrame m_targetFrame;//叠加图像，一般应该是RGB32

	int m_dy = 0;

	//摄像头数据处理
	void  HandleFrame(AVFrame* avframe) {
		//简单的缩放处理
		WXVideoFrame* pFrame = m_pool_queue.Pop();
		if (pFrame == nullptr) {
			//队列不可写
			return;
		}
		if (pFrame->GetFrame() == nullptr ||
			pFrame->GetFrame()->format != avframe->format ||
			pFrame->GetFrame()->width != avframe->width ||
			pFrame->GetFrame()->height != avframe->height) {
			pFrame->Init(avframe->format, avframe->width, avframe->height);//动态改变大小
		}
		av_frame_copy(pFrame->GetFrame(), avframe);//拷贝数据
		m_data_queue.Push(pFrame);
	}

	ThreadSafeQueue<WXVideoFrame*>m_pool_queue;
	ThreadSafeQueue<WXVideoFrame*>m_data_queue;
public:
	int64_t  m_ptsLast = 0;

	void *m_pDataSink = nullptr;
	OnVideoData m_cbData = nullptr;//数据回调

	void *m_pEventSink = nullptr;//回调对象，可能是C++对象
	wxCallBack m_cbEvent = nullptr;//事件回调

	int m_bGameCapture = FALSE;//游戏录制！！

	VideoSource *m_pVideoSource = nullptr;

	int      m_iWidth = 0;
	int      m_iHeight = 0;

	int64_t  m_ptsLastVideo = 0;//时间戳（包含暂停时间中的）
	int64_t  m_ptsDelay = 0;//时间戳
	BOOL     m_bStart = FALSE;

	int64_t m_ptsStart = 0;
	int64_t m_ptsLastCapture = 0;

	int m_bFirst = FALSE;
	int m_bPause = FALSE;

public:

	WXCaptureVideo(void* cond, int* bFlag) {
		this->ThreadSetCond((WXCond*)cond, bFlag);
		memset(&m_video, 0, sizeof(m_video));
		memset(&m_text, 0,  sizeof(m_text));
		memset(&m_image, 0, sizeof(m_image));
		memset(&m_mouse, 0, sizeof(m_mouse));
	}
	void SetDataSink(void *sink, OnVideoData cbData) {
		m_pDataSink = sink;
		m_cbData = cbData;
	}
	void SetEventSink(void *sink, wxCallBack cbData) {
		m_pEventSink = sink;
		m_cbEvent = cbData;
	}

	VideoDeviceParam    m_video;//视频录制参数
	TextWaterMarkParam  m_text;//文字水印录制参数
	ImageWaterMarkParam m_image;//图像水印录制参数
	MouseParam m_mouse;//鼠标配置参数

	void OnVideoFrame(AVFrame *frame) {  //每一帧的时间戳都是绝对时间！！！
		frame->pts -= m_ptsStart;//处理为相对时间

		if (frame->pts <= m_ptsLastVideo)
			return;//时间戳不动了

		//暂停处理
		if (m_bPause) {
			m_ptsDelay += (frame->pts - m_ptsLastVideo);//视频暂停时间
			m_ptsLastVideo = frame->pts;
			return;//暂停时间不写文件
		}
		else {
			m_ptsLastVideo = frame->pts;
		}
		int64_t ptsVideo = frame->pts - m_ptsDelay;//计算输出时间戳

		if (!m_bFirst) { //补0s时的图像
			m_bFirst = TRUE;
			frame->pts = 0; //实际的视频时间戳

			if (m_cbData)
				m_cbData(m_pDataSink, frame);//直接输出
		}

		frame->pts = ptsVideo; //实际的视频时间戳
		m_ptsLast = WXGetTimeMs();//最后一次视频实际编码时间

		if (m_cbData)
			m_cbData(m_pDataSink, frame);//直接输出
	}

	int64_t GetVideoTimeOut() {
		return m_ptsStart ? WXGetTimeMs() - m_ptsLast : -1;
	}

	void SetScreen(WXCTSTR strDisplay, int nFps) {
		if (strDisplay) {
			memset(&m_video, 0, sizeof(m_video));
			m_video.m_bUse = 1;
			m_video.m_bCamera = 0;
			wcscpy(m_video.m_wszDevName, strDisplay);
			m_video.m_iFps= nFps;
		}
	}

	void SetWindow(HWND hwnd, int nFps) {
		if (hwnd) {
			memset(&m_video, 0, sizeof(m_video));
			m_video.m_bUse = 1;
			m_video.m_bCamera = 2;
			m_video.m_hwndCapture = hwnd;
			m_video.m_iFps = nFps;
		}
	}

	void SetVideoParam(VideoDeviceParam *param) {
		if (param && param->m_bUse) {
			memcpy(&m_video, param, sizeof(m_video));
		}
	}

	void SetTextParam(TextWaterMarkParam *param) {
		if (param && param->m_bUsed) {
			memcpy(&m_text, param, sizeof(m_text));
		}
	}

	void SetImageParam(ImageWaterMarkParam *param) {
		if (param && param->m_bUsed) {
			memcpy(&m_image, param, sizeof(m_image));
		}
	}

	void SetMouseParam(MouseParam *param) {
		if (param && param->m_iUsed) {
			memcpy(&m_mouse, param, sizeof(m_mouse));
		}
	}

	int  UpdateConfig(){

		if (m_video.m_bUse == 0)
			return WX_ERROR_ERROR;

		if (m_video.m_bCamera == 1) { //虚拟设备
			m_video.m_iForceFps = 0;//强制CFR
			//WXLogW(L"Using CFR Mode");
			if (WXStrcmp(_T("WX_GAME"), m_video.m_wszDevName) == 0 ||
				WXStrcmp(_T("Game"), m_video.m_wszDevName) == 0) {// 游戏数据源输出
#ifdef _WIN32
				//RGB32
				WXLogW(L"Game Capture Mode");
				m_bGameCapture = TRUE;
				m_iWidth  = (WXGameGetWidth()  * WXGameGetScale()) / 100 / 2 * 2;
				m_iHeight = (WXGameGetHeight() * WXGameGetScale()) / 100 / 2 * 2;
				m_pVideoSource = VideoSource::Create(_T("Virtual"));
				m_pVideoSource->m_nScale = WXGameGetScale();
#endif
			}else {
				m_iWidth  = m_video.m_iCameraWidth;
				m_iHeight = m_video.m_iCameraHeight;
				m_pVideoSource = VideoSource::Create(_T("Virtual"));
				WXLogW(L"Airplay or Camera");
			}
		}
#ifdef _WIN32
		else if (m_video.m_bCamera == 0) { //显示器录制
			////RGB32
			if (!WXGetGlobalValue(L"DisableDXGI",0) && WXSupportDXGI() && m_video.m_bDXGI == 1 /*&& WXScreenGetCount() == 1*/) {
				MonitorInfo* info = WXScreenGetDefaultInfo();
				int rotate = WXScreenGetRotate(info->wszName);
				if (rotate == 0) //DXGI 不支持旋转
					m_pVideoSource = VideoSource::Create(_T("Mixer"));
			}

			//Test WGC
			//非区域录制 + 支持WGC + 单显示器
			if (nullptr == m_pVideoSource && !m_video.m_bRect && WXGetGlobalValue(L"WGC",0) && m_video.m_bDXGI == 2 && WXScreenGetCount() == 1) {
				MonitorInfo* info = WXScreenGetDefaultInfo();
				int rotate = WXScreenGetRotate(info->wszName);
				if (rotate == 0) //WGC 不支持旋转
					m_pVideoSource = VideoSource::Create(_T("WGC"));
			}


			if (nullptr == m_pVideoSource) {
				WXLogA("%s new GDI Capture", __FUNCTION__);
				m_pVideoSource = VideoSource::Create(_T("GDI"));
			}

		}
		else if (m_video.m_bCamera == 2) { //窗口录制，2018.12.10
			m_pVideoSource = VideoSource::Create(_T("Window"));//RGB32
		}
#endif
		if (m_pVideoSource) {
			m_pVideoSource->ThreadSetCond(this->GetCond(),this->GetFlag());
			m_pVideoSource->SetEventSink(m_pEventSink, m_cbEvent);//数据回调
			m_pVideoSource->SetVideoParam(&m_video);//视频参数
			m_pVideoSource->SetTextParam(&m_text);//文字水印
			m_pVideoSource->SetMouseParam(&m_mouse);//鼠标信息
			m_pVideoSource->SetImageParam(&m_image);//图像水印

			int ret = m_pVideoSource->Init();
			if (ret != WX_ERROR_SUCCESS) {
				WXLogW(L"Open Video Capture Error!!!!");
				VideoSource::Destroy(m_pVideoSource);
				m_pVideoSource = nullptr;
				return ret;
			}
			else {
#ifdef _WIN32
				if (m_bGameCapture) {
					WXGameStartRecord(m_video.m_iFps);
				}
#endif

				int width = m_pVideoSource->GetWidth();
				int height = m_pVideoSource->GetHeight();

				m_nCameraWndWidth = WXGetGlobalValue(L"Capture_CameraWindowWidth",0);
				if (m_nCameraWndWidth > 0) {  //考虑摄像头的叠加！

					m_nCameraWndWidth = m_nCameraWndWidth / 2 * 2;//避免奇数

					m_nCameraType = WXGetGlobalValue(L"Capture_CameraType", 0) % 4;

					m_iWidth = width + m_nCameraWndWidth; //宽度增加
					m_iHeight = ((m_iWidth * height / width) + 3) / 4 * 4; //新的高度
					m_dy = (m_iHeight - height) / 2;
					m_targetFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
					WXLogW(L"BaseSize[ %dx%d] TargetSize[%dx%d] dy = %d",
						width, height,
						m_iWidth, m_iHeight, m_dy);

					m_pool_queue.Push(&m_captureCameraFrame[0]);
					m_pool_queue.Push(&m_captureCameraFrame[1]);
					s_pVideo = this;
				}
				else {
					m_iWidth = width;
					m_iHeight = height;
					WXLogW(L"Set Video Size = %dx%d", m_iWidth, m_iHeight);
				}
			}
			return WX_ERROR_SUCCESS;
		}
		return WX_ERROR_ERROR;
	}

	void Start() { //启动
		if (m_pVideoSource) {
			m_pVideoSource->Start();
			ThreadSetName(L"WXCaptureVideo");
			ThreadStart(true);
		}
	}

	void Stop() { //结束
		s_pVideo = nullptr;
		ThreadStop();

		if (m_pVideoSource) {
			m_pVideoSource->Stop();
			VideoSource::Destroy(m_pVideoSource);
			m_pVideoSource = nullptr;
		}

#ifdef _WIN32
		if (m_bGameCapture) {
			WXGameStopRecord();
		}
#endif

		//if (m_ptsDelay) //暂停时间
		//	WXLogA("%s TimeDelay=%lld", __FUNCTION__, m_ptsDelay);

		//int64_t ptsDuration = WXGetTimeMs() - m_ptsStart;
		//WXLogA("%s TimeDuration=%lld", __FUNCTION__, ptsDuration);
	}

	virtual  void ThreadPrepare() {
	//	LogA("++++++  VideoCapture ThreadPrepare WXCond_Wait");
	}

	virtual  void ThreadWait() {
		m_ptsStart = WXGetTimeMs();
	}

	BOOL m_bGotPic = FALSE;
	int  m_nTypePic = 0;
	WXString m_strJpeg;


	virtual void ThreadProcess() {  //编码输出
		WXVideoFrame *video_frame = m_pVideoSource->GetVideoFrame();

		//onCameraData

		if (video_frame) {

			if (m_nCameraWndWidth == 0) { //原来的处理逻辑
				if (m_bGotPic) { //截图操作
					WXLogW(L"%ws %ws", __FUNCTIONW__, m_strJpeg.str());
					WXMediaUtilsSaveAsPicture(video_frame->GetFrame(), m_strJpeg.str(), 100);
					if (m_cbEvent) {
						m_cbEvent(m_pEventSink, m_nTypePic, (void*)m_strJpeg.str());
					}
					m_bGotPic = FALSE;
				}
				OnVideoFrame(video_frame->GetFrame());//编码输出
			}
			else {
				//叠加摄像头处理
				WXVideoFrame* pFrame = m_data_queue.Pop();
				if (pFrame) { //取出摄像头数据，如果没有新的就用原来的图像
					if (m_cameraFrame.GetFrame() == nullptr ||
						m_cameraFrame.GetWidth() != pFrame->GetWidth() ||
						m_cameraFrame.GetHeight() != pFrame->GetHeight()) {
						m_cameraFrame.Init(AV_PIX_FMT_RGB32, pFrame->GetWidth(), pFrame->GetHeight());
					}
					if (pFrame->GetFrame()->format == AV_PIX_FMT_RGB32) {
						libyuv::ARGBCopy(
							pFrame->GetFrame()->data[0],
							pFrame->GetFrame()->linesize[0],
							m_cameraFrame.GetFrame()->data[0],
							m_cameraFrame.GetFrame()->linesize[0],
							pFrame->GetWidth(), pFrame->GetHeight()
						);
					}else {
						libyuv::I420ToARGB(
							pFrame->GetFrame()->data[0],pFrame->GetFrame()->linesize[0],
							pFrame->GetFrame()->data[1], pFrame->GetFrame()->linesize[1],
							pFrame->GetFrame()->data[2], pFrame->GetFrame()->linesize[2], 
							m_cameraFrame.GetFrame()->data[0],
							m_cameraFrame.GetFrame()->linesize[0],
							pFrame->GetWidth(), pFrame->GetHeight()
						);
					}
					m_pool_queue.Push(pFrame);
				}

				//MixVideo
				//叠加屏幕图像
				uint8_t* pMixDst = nullptr;

				if (m_nCameraType == POS_RIGHT_TOP || m_nCameraType == POS_RIGHT_BUTTOM) { //摄像头在右边
					pMixDst = m_targetFrame.GetFrame()->data[0] + m_dy * m_targetFrame.GetFrame()->linesize[0];
				}else { //左边
					pMixDst = m_targetFrame.GetFrame()->data[0] + m_dy * m_targetFrame.GetFrame()->linesize[0] + 
						m_nCameraWndWidth * 4;
				}

				if (video_frame->GetFrame()->format == AV_PIX_FMT_RGB32) {
					libyuv::ARGBCopy(
						video_frame->GetFrame()->data[0], video_frame->GetFrame()->linesize[0],
						pMixDst, m_targetFrame.GetFrame()->linesize[0],
						video_frame->GetWidth(),
						video_frame->GetHeight()
					);
				}else {
					libyuv::I420ToARGB(
						video_frame->GetFrame()->data[0], video_frame->GetFrame()->linesize[0],
						video_frame->GetFrame()->data[1], video_frame->GetFrame()->linesize[1],
						video_frame->GetFrame()->data[2], video_frame->GetFrame()->linesize[2],
						pMixDst,
						m_targetFrame.GetFrame()->linesize[0],
						video_frame->GetWidth(),
						video_frame->GetHeight()
					);
				}

				//叠加摄像头图像
				if (m_cameraFrame.GetWidth() > 0) {//摄像头有数据

					
					//摄像头显示高度
					m_nCameraWndHeight = ((m_nCameraWndWidth * m_cameraFrame.GetHeight() / m_cameraFrame.GetWidth()) + 3) / 4 * 4;
				
					uint8_t* pCameraDst = nullptr;
					int nPosX = 0;
					int nPosY = 0;
					if (m_nCameraType == POS_RIGHT_TOP) { //摄像头在右上角
						nPosY = m_dy;
						nPosX = video_frame->GetWidth();

					}
					else if (m_nCameraType == POS_RIGHT_BUTTOM) { //摄像头在右下角
						nPosY = m_targetFrame.GetHeight() - m_nCameraWndHeight - m_dy;
						nPosX = video_frame->GetWidth();
					}
					else if (m_nCameraType == POS_LEFT_BUTTOM) {  //摄像头在左下角
						nPosY = m_targetFrame.GetHeight() - m_nCameraWndHeight - m_dy;
						nPosX = 0;
					}
					else if (m_nCameraType == POS_LEFT_TOP) {  //摄像头在左上角
						nPosY = m_dy;
						nPosX = 0;
					}

					pCameraDst = m_targetFrame.GetFrame()->data[0] + nPosY * m_targetFrame.GetFrame()->linesize[0] + nPosX * 4;

					libyuv::ARGBScale(
						m_cameraFrame.GetFrame()->data[0], 
						m_cameraFrame.GetFrame()->linesize[0],
						m_cameraFrame.GetWidth(), 
						m_cameraFrame.GetHeight(),
						pCameraDst,
						m_targetFrame.GetFrame()->linesize[0],
						m_nCameraWndWidth, m_nCameraWndHeight,
						libyuv::FilterMode::kFilterBilinear
					);
				}

				if (m_bGotPic) { //截图操作
					WXLogW(L"%ws %ws", __FUNCTIONW__, m_strJpeg.str());
					WXMediaUtilsSaveAsPicture(m_targetFrame.GetFrame(), m_strJpeg.str(), 100);
					if (m_cbEvent) {
						m_cbEvent(m_pEventSink, m_nTypePic, (void*)m_strJpeg.str());
					}
					m_bGotPic = FALSE;
				}
				m_targetFrame.GetFrame()->pts = video_frame->GetFrame()->pts;
				OnVideoFrame(m_targetFrame.GetFrame());//编码输出
			}
			m_pVideoSource->m_queuePool.Push(video_frame);
		}
		else {
			SLEEPMS(1);
		}
	}

	void ChangeRect(int x, int y, int w, int h) {
		if (m_pVideoSource)
			m_pVideoSource->ChangeRect(x, y, w, h); 
	}


	void GetPicture(int type, WXCTSTR wszName, int quality) {
		WXTask task = [this, type, wszName] {
			if (!m_bGotPic) {
				m_strJpeg = wszName;
				m_nTypePic = type;
				m_bGotPic = TRUE;
			}
		};
		RunTask(task);
	}

	void Pause(int bPause) {
		WXTask task = [this,bPause] {
			DWORD dw = GetCurrentThreadId();
			WXLogW(L"WXCaptureVideo %ws ---------- ThreadID = %08x", __FUNCTIONW__, dw);
			m_bPause = bPause;
		};
		RunTask(task);
	}
};

//静态成员
/*static */ WXCaptureVideo* WXCaptureVideo::s_pVideo = nullptr;

//推送一个AVFrame进来
/*extern */void CameraDataPush(AVFrame*  frame) {
	WXCaptureVideo::__CameraDataPush(frame);
}

//视频设备采集
//创建采集设备
WXMEDIA_API void*   WXVideoCaptureCreate() {
	WXCaptureVideo *capture = new WXCaptureVideo(nullptr, nullptr);
	return capture;
}

//创建对象,设置信号
WXMEDIA_API void WXVideoCaptureSetCond(void* ptr, void* cond, int* bFlag)
{
	if (ptr) {
		WXCaptureVideo* capture = (WXCaptureVideo*)ptr;
		capture->ThreadSetCond((WXCond*)cond, bFlag);
	}
}

//设置输出输出回调
WXMEDIA_API void    WXVideoCaptureSetDataSink(void* ptr, void *sink, OnVideoData cbData) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetDataSink(sink, cbData);
	}
}

//设置事件回调
WXMEDIA_API void    WXVideoCaptureSetEventSink(void* ptr, void *sink, wxCallBack cbData) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetEventSink(sink, cbData);
	}
}

//设置采集屏幕
WXMEDIA_API void    WXVideoCaptureSetScreen(void* ptr, WXCTSTR strDisplay, int nFps) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetScreen(strDisplay, nFps);
	}
}

//设置采集窗口
WXMEDIA_API void    WXVideoCaptureSetWindow(void* ptr, HWND hwnd, int nFps) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetWindow(hwnd, nFps);
	}
}

//设置采集参数
WXMEDIA_API void    WXVideoCaptureSetVideoParam(void* ptr, VideoDeviceParam *param) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetVideoParam(param);
	}
}

//文字水印信息
WXMEDIA_API void    WXVideoCaptureSetTextParam(void* ptr, TextWaterMarkParam *param) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetTextParam(param);
	}
}

//图像水印信息
WXMEDIA_API void    WXVideoCaptureSetImageParam(void* ptr, ImageWaterMarkParam *param) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetImageParam(param);
	}
}

//鼠标图像信息
WXMEDIA_API void    WXVideoCaptureSetMouseParam(void* ptr, MouseParam *param) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->SetMouseParam(param);
	}
}

//更新录制参数
WXMEDIA_API int     WXVideoCaptureUpdate(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		int ret = capture->UpdateConfig();
		return ret;
	}
	return WX_ERROR_ERROR;
}

//销毁
WXMEDIA_API void    WXVideoCaptureDestroy(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		delete capture;
	}
}

//启动录制推流
WXMEDIA_API void    WXVideoCaptureStart(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->Start();
	}
}

//关闭推流
WXMEDIA_API void    WXVideoCaptureStop(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->Stop();
	}
}

//获取宽度
WXMEDIA_API int     WXVideoCaptureGetWidth(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		return capture->m_iWidth;
	}
	return 0;
}

//获取高度
WXMEDIA_API int     WXVideoCaptureGetHeight(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		return capture->m_iHeight;
	}
	return 0;
}

//更改录制区域
WXMEDIA_API void    WXVideoCaptureChangeRect(void* ptr, int x, int y, int w, int h) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		//可能在UI线程调用，需要优化到底层
		capture->ChangeRect(x, y, w, h);
	}
}

//获取截图
WXMEDIA_API void    WXVideoCaptureGetPicture(void* ptr, int type, WXCTSTR wszName, int quality) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->GetPicture(type, wszName, quality);
	}
}

//获取超时时间
WXMEDIA_API int64_t WXVideoCaptureGetVideoTimeOut(void* ptr) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		return capture->GetVideoTimeOut();
	}
	return 0;
}

//暂停
WXMEDIA_API void    WXVideoCapturePause(void* ptr, int b) {
	if (ptr) {
		WXCaptureVideo *capture = (WXCaptureVideo*)ptr;
		capture->Pause(b);
	}
}


