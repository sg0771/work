/*
具有外部采集线程的录屏输入
1. 游戏录制
2. Airplay Chromecast等投屏
3. 摄像头
进来的数据的时间戳应该为当前系统时间(单位毫秒)
*/

#include "WXMediaCpp.h"
#include "DataSource.h"
#include "WXCapture.h"


extern int g_bAllowResize/* = 0*/;
static WXLocker s_LockData;//数据推送锁
static DataSource *s_impl = nullptr;

//数据可能来源于 H264解码器(Airplay/ChromeCast)、摄像头、游戏录制
//外部线程输入
WXMEDIA_API void  WXAirplayPush(AVFrame *frame) {

	if (frame == nullptr)return;
	WXAutoLock al(s_LockData);
	if (s_impl) {
		if (g_bAllowResize)
		{
			s_impl->PushYUV2(frame);
		}else {
			s_impl->PushYUV(frame, RENDER_ROTATE_NONE);
		}

	}
}

//加旋转
WXMEDIA_API void WXAirplayPush2(struct AVFrame *frame, int rotate) {
	if (frame == nullptr)return;
	WXAutoLock al(s_LockData);
	if (s_impl) {
		//WXLogA("%s %dx%d [Rotate=%d]",__FUNCTION__,frame->width,frame->height,rotate);
		s_impl->PushYUV(frame, rotate);
	}
}

//来自VLC的 I420数据推入
WXMEDIA_API void  WXPushI420Data(uint8_t*buf, int width, int height) {
	if (buf == nullptr || width <= 0 || height <= 0)return;
	WXAutoLock al(s_LockData);
	if (s_impl) {
		s_impl->PushI420Data(buf, width, height);
	}
}

//来自VLC等播放器回调的 I420数据推送
WXMEDIA_API void  WXPushRGB32Data(uint8_t*buf, int width, int height, int pitch) {
	if (buf == nullptr || width <= 0 || height <= 0)return;
	WXAutoLock al(s_LockData);
	if (s_impl) {
		s_impl->PushRGB32Data(buf, width, height, pitch);
	}
}

//游戏录制数据推送
WXMEDIA_API void  WXRgbaPush(AVFrame *frame) {
	if (frame == nullptr)return;
	WXAutoLock al(s_LockData);
	if (s_impl) {
		s_impl->PushRGBA(frame);
	}
}

WXCTSTR DataSource::Type() {
	return _T("virtual");
}

void  DataSource::PushI420Data(uint8_t*buf, int width, int height) {
	if (!m_bStart)return;
	m_nCapture++;

	WXVideoFrame *VideoFrame = m_queuePool.Pop();
	if (VideoFrame == nullptr) { //队列已经满了
		m_nDrop++;//丢帧 PushI420Data
		return;
	}

	if (width == m_iWidth && height == m_iHeight) {
		int64_t ptsVideo = WXGetTimeMs();
		if (ptsVideo - m_ptsLast < m_iTime * 4 / 5) { //丢弃和上一帧时间间隔比较短的数据
			m_nDrop++;//丢帧 PushI420Data
			m_queuePool.Push(VideoFrame);
			return;
		}
		int ImageSize = width * height;
		AVFrame *pVideoFrame = VideoFrame->GetFrame();
		pVideoFrame->pts = ptsVideo;
		m_ptsLast = ptsVideo;//更新输入时间戳
		libyuv::I420Copy(
			buf, width,
			buf + ImageSize, width / 2,
			buf + ImageSize * 5 / 4, width / 2,
			pVideoFrame->data[0], pVideoFrame->linesize[0],
			pVideoFrame->data[1], pVideoFrame->linesize[1],
			pVideoFrame->data[2], pVideoFrame->linesize[2],
			width, height
		);
		this->PushVideoData(VideoFrame);
	}
	else {
		this->PushVideoData(VideoFrame);
	}
}

void  DataSource::PushRGB32Data(uint8_t*buf, int width, int height, int pitch) {
	if (!m_bStart)return;
	m_nCapture++;
	WXVideoFrame *VideoFrame = m_queuePool.Pop();
	if (VideoFrame == nullptr) { //队列已经满了
		m_nDrop++;//丢帧  PushRGB32Data
		return;
	}
	if (width == m_iWidth && height == m_iHeight) {
		int64_t ptsVideo = WXGetTimeMs();
		if (ptsVideo - m_ptsLast < m_iTime * 4 / 5) { //丢弃和上一帧时间间隔比较短的数据
			m_nDrop++;//丢帧 PushRGB32Data
			m_queuePool.Push(VideoFrame);
			return;
		}
		int ImageSize = width * height;
		AVFrame *pVideoFrame = VideoFrame->GetFrame();
		pVideoFrame->pts = ptsVideo;
		m_ptsLast = ptsVideo;//更新输入时间戳
		libyuv::ARGBCopy(
			buf, pitch,
			pVideoFrame->data[0], pVideoFrame->linesize[0],
			width, height
		);
		this->PushVideoData(VideoFrame);
	}
	else {
		this->PushVideoData(VideoFrame);
	}
}

//填充数据到缓存区
//Airplay和摄像头采集的数据
void DataSource::PushYUV(AVFrame *srcFrame, int rotate) {

	////if (!m_bStart)return;
	m_nCapture++;
	WXVideoFrame *VideoFrame = m_queuePool.Pop();
	if (VideoFrame == nullptr) { //队列已经满了
		m_nDrop++;//丢帧
		return;
	}

	BOOL bRotate = rotate % 2;
	BOOL canWrite = FALSE;

	//Patch For Android Mirror Record
	if (!bRotate && srcFrame->width / 4 * 4 == m_iWidth / 4 * 4 &&
		srcFrame->height / 4 * 4 == m_iHeight / 4 * 4)
		canWrite = TRUE;

	if (bRotate && srcFrame->width / 4 * 4 == m_iHeight / 4 * 4 &&
		srcFrame->height / 4 * 4 == m_iWidth / 4 * 4)
		canWrite = TRUE;

	if (canWrite) {
		m_nCapture++;
		int64_t ptsVideo = srcFrame->pts = WXGetTimeMs();
		if (ptsVideo - m_ptsLast < m_iTime * 4 / 5) { //丢弃和上一帧时间间隔比较短的数据
			m_nDrop++;//丢帧
			m_queuePool.Push(VideoFrame);
			return;
		}
		AVFrame *dstFrame = m_InputVideoFrame.GetFrame();
		dstFrame->pts = srcFrame->pts;

		libyuv::I420Rotate(
			srcFrame->data[0], srcFrame->linesize[0],
			srcFrame->data[1], srcFrame->linesize[1],
			srcFrame->data[2], srcFrame->linesize[2],
			dstFrame->data[0], dstFrame->linesize[0],
			dstFrame->data[1], dstFrame->linesize[1],
			dstFrame->data[2], dstFrame->linesize[2],
			bRotate ? dstFrame->height : dstFrame->width,
			bRotate ? dstFrame->width : dstFrame->height,
			(libyuv::RotationMode)(rotate * 90));
#ifdef _WIN32
		if (m_bText || m_bImage1 || m_bImage2) {
			if (m_hDC == nullptr) {
				m_hDC = ::GetDC(nullptr);
				m_hMemDC = ::CreateCompatibleDC(m_hDC);
			}

			libyuv::I420ToARGB(dstFrame->data[0], dstFrame->linesize[0],
				dstFrame->data[1], dstFrame->linesize[1],
				dstFrame->data[2], dstFrame->linesize[2],
				m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
				m_iWidth, m_iHeight);

			m_hBitmap = ::CreateBitmap(m_iWidth, m_iHeight, 1, 32, m_tempFrame.GetFrame()->data[0]);
			HBITMAP oldHBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);
			this->DrawEx(0, 0, 0);
			m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, oldHBitmap);
			int bGetDIBits = ::GetDIBits(m_hMemDC, m_hBitmap, 0, m_iHeight,
				(LPSTR)m_tempFrame.GetFrame()->data[0], (BITMAPINFO*)m_tempFrame.GetBIH(), DIB_RGB_COLORS);
			if (bGetDIBits <= 0) { //GetDIBits失败
				m_nDrop++;
				SAFE_RELEASE_DC(nullptr, m_hDC)
					SAFE_DELETE_OBJECT(m_hBitmap)
					SAFE_DELETE_DC(m_hMemDC)
					m_queuePool.Push(VideoFrame);
				return;
			}

			SAFE_DELETE_OBJECT(m_hBitmap);
			libyuv::ARGBToI420(m_tempFrame.GetFrame()->data[0], m_tempFrame.GetFrame()->linesize[0],
				dstFrame->data[0], dstFrame->linesize[0],
				dstFrame->data[1], dstFrame->linesize[1],
				dstFrame->data[2], dstFrame->linesize[2],
				m_iWidth, m_iHeight);
		}
#endif

		AVFrame *pVideoFrame = VideoFrame->GetFrame();
		pVideoFrame->pts = ptsVideo;
		libyuv::I420Copy(
			dstFrame->data[0], dstFrame->linesize[0],
			dstFrame->data[1], dstFrame->linesize[1],
			dstFrame->data[2], dstFrame->linesize[2],
			pVideoFrame->data[0], pVideoFrame->linesize[0],
			pVideoFrame->data[1], pVideoFrame->linesize[1],
			pVideoFrame->data[2], pVideoFrame->linesize[2],
			pVideoFrame->width, pVideoFrame->height
		);
		m_ptsLast = ptsVideo;//更新输入时间戳

		//m_queueData.Push(VideoFrame);
		this->PushVideoData(VideoFrame);
	}
	else {
		m_queuePool.Push(VideoFrame);
	}
}


//填充数据到缓存区
//Airplay和摄像头采集的数据
//支持分辨率不一致的数据输入
void DataSource::PushYUV2(AVFrame *srcFrame) {
	if (!m_bStart)return;
	m_nCapture++;

	int64_t ptsVideo = srcFrame->pts = WXGetTimeMs();
	if (ptsVideo - m_ptsLast < m_iTime * 4 / 5) { //丢弃和上一帧时间间隔比较短的数据
		m_nDrop++;//丢帧
		return;
	}

	WXVideoFrame *VideoFrame = m_queuePool.Pop();
	if (VideoFrame == nullptr) { //队列已经满了
		m_nDrop++;//丢帧
		return;
	}

	AVFrame *dstFrame = VideoFrame->GetFrame();
	dstFrame->pts = ptsVideo;
	if (srcFrame->width != m_iWidth || srcFrame->height != m_iHeight) {
		int dx = 0;
		int dy = 0;
		GetXY(srcFrame->width, srcFrame->height, m_iWidth, m_iHeight, dx, dy);

		memset(dstFrame->data[0], 0, dstFrame->linesize[0] * dstFrame->height);
		memset(dstFrame->data[1], 128, dstFrame->linesize[1] * dstFrame->height / 2);
		memset(dstFrame->data[2], 128, dstFrame->linesize[2] * dstFrame->height / 2);

		libyuv::I420Scale(srcFrame->data[0], srcFrame->linesize[0],
			srcFrame->data[1], srcFrame->linesize[1],
			srcFrame->data[2], srcFrame->linesize[2],
			srcFrame->width, srcFrame->height,
			dstFrame->data[0] + dx + dy * dstFrame->linesize[0], dstFrame->linesize[0],
			dstFrame->data[1] + dx / 2 + (dy / 2) * dstFrame->linesize[1], dstFrame->linesize[1],
			dstFrame->data[2] + dx / 2 + (dy / 2) * dstFrame->linesize[2], dstFrame->linesize[2],
			m_iWidth - 2 * dx, m_iHeight - 2 * dy, libyuv::FilterMode::kFilterLinear);
	}
	else {
		libyuv::I420Copy(srcFrame->data[0], srcFrame->linesize[0],
			srcFrame->data[1], srcFrame->linesize[1],
			srcFrame->data[2], srcFrame->linesize[2], 
			dstFrame->data[0], dstFrame->linesize[0],
			dstFrame->data[1], dstFrame->linesize[1],
			dstFrame->data[2], dstFrame->linesize[2],
			m_iWidth, m_iHeight);
	}
	m_ptsLast = ptsVideo;
	this->PushVideoData(VideoFrame);
}


//填充数据到缓存区
//一般是游戏录制的数据
void DataSource::PushRGBA(AVFrame *srcFrame) {
	if (!m_bStart)return;
	m_nCapture++;
	WXVideoFrame* VideoFrame = m_queuePool.Pop();
	if (VideoFrame == nullptr) { //队列已经满了
		m_nDrop++;//丢帧
		return;
	}
	srcFrame->width = srcFrame->width / 2 * 2;
	srcFrame->height = srcFrame->height / 2 * 2;
	if ((srcFrame->width == m_iWidth && srcFrame->height == m_iHeight) || m_nScale != 100) {
		m_nCapture++;
		int64_t ptsVideo = srcFrame->pts = WXGetTimeMs();
		if (ptsVideo - m_ptsLast < m_iTime * 4 / 5) { //丢弃和上一帧时间间隔比较短的数据
			m_nDrop++;//丢帧
			m_queuePool.Push(VideoFrame);
			return;
		}
		AVFrame *dstFrame = m_InputVideoFrame.GetFrame();
		dstFrame->pts = srcFrame->pts;

		if (m_nScale == 100) {
			//av_frame_copy(dstFrame, srcFrame);
			libyuv::ARGBCopy(srcFrame->data[0], srcFrame->linesize[0], dstFrame->data[0], dstFrame->linesize[0], dstFrame->width, dstFrame->height);
		}
		else {
			libyuv::ARGBScale(srcFrame->data[0], srcFrame->linesize[0], srcFrame->width, srcFrame->height,
				dstFrame->data[0], dstFrame->linesize[0], dstFrame->width, dstFrame->height,
				libyuv::FilterMode::kFilterBilinear
			);
		}

#ifdef _WIN32
		if (m_bText || m_bImage1 || m_bImage2) {
			if (m_hDC == nullptr) {
				m_hDC = ::GetDC(nullptr);
				m_hMemDC = ::CreateCompatibleDC(m_hDC);
			}

			m_hBitmap = ::CreateBitmap(m_iWidth, m_iHeight, 1, 32, dstFrame->data[0]);
			HBITMAP oldHBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);
			this->DrawEx(0, 0, 0);
			m_hBitmap = (HBITMAP)::SelectObject(m_hMemDC, m_hBitmap);
			//获取数据
			int bGetDIBits = ::GetDIBits(m_hMemDC, m_hBitmap, 0, m_iHeight,
				(LPSTR)m_InputVideoFrame.GetFrame()->data[0],
				(BITMAPINFO*)m_InputVideoFrame.GetBIH(), DIB_RGB_COLORS);
			if (bGetDIBits <= 0) { //GetDIBits失败
				m_nDrop++;
				SAFE_RELEASE_DC(nullptr, m_hDC)
					SAFE_DELETE_OBJECT(m_hBitmap)
					SAFE_DELETE_DC(m_hMemDC)
					m_queuePool.Push(VideoFrame);
				return;
			}
			SAFE_DELETE_OBJECT(m_hBitmap);
		}

#endif

		AVFrame *pVideoFrame = VideoFrame->GetFrame();
		pVideoFrame->pts = ptsVideo;
		libyuv::ARGBCopy(
			dstFrame->data[0], dstFrame->linesize[0],
			pVideoFrame->data[0], pVideoFrame->linesize[0],
			pVideoFrame->width, pVideoFrame->height
		);
		m_ptsLast = ptsVideo;//更新输入时间戳
		this->PushVideoData(VideoFrame);
	}
	else {
		m_queuePool.Push(VideoFrame);
	}

}

void DataSource::Stop() {

	WXAutoLock al2(s_LockData);//停止数据输入
	if (m_bStart) {
		m_bStart = FALSE;
		WXLogA("%s @_@ Capture=[%lld] Drop=[%lld] m_nInsert=[%lld]",
			__FUNCTION__, m_nCapture, m_nDrop, m_nInsert);
		s_impl = nullptr;
	}
}

int  DataSource::Init() {
	SetCapture();

	if (m_nScale == 100) {
		m_iWidth = m_video.m_iCameraWidth / 2 * 2;
		m_iHeight = m_video.m_iCameraHeight / 2 * 2;
	}
	else {
		m_iWidth = (m_video.m_iCameraWidth * m_nScale) / 100 / 2 * 2;
		m_iHeight = (m_video.m_iCameraHeight * m_nScale) / 100 / 2 * 2;
	}

	WXLogA("%s size=%dx%d", __FUNCTION__, m_iWidth, m_iHeight);

	if (m_iWidth < 100 || m_iHeight < 100)return WX_ERROR_VIDEO_DEVICE_OPEN;

	if (WXStrcmp(_T("WX_GAME"), m_video.m_wszDevName) == 0 ||
		WXStrcmp(_T("Game"), m_video.m_wszDevName) == 0) {
		m_dstFmt = AV_PIX_FMT_RGB32;
	}
	else {
		m_dstFmt = AV_PIX_FMT_YUV420P;
	}

	m_tempFrame.Init(AV_PIX_FMT_RGB32, m_iWidth, m_iHeight);
#ifdef _WIN32
	m_hDC = ::GetDC(nullptr);
	m_hMemDC = ::CreateCompatibleDC(m_hDC);
#endif
	m_nPool = MAX_POOL;
	//m_nPool = m_video.m_iFps; //恢复成旧版本写法
	m_InputVideoFrame.Init(m_dstFmt, m_iWidth, m_iHeight);//采集图像
	m_aData = new WXVideoFrame[m_nPool];
	for (int i = 0; i < m_nPool; i++) {  //初始化
		m_aData[i].Init(m_dstFmt, m_iWidth, m_iHeight);
		m_queuePool.Push(&m_aData[i]);
	}
	return WX_ERROR_SUCCESS;
}

void DataSource::Start() {
	WXAutoLock al2(s_LockData);//开始数据输入
	m_bStart = TRUE;
	s_impl = this;
}