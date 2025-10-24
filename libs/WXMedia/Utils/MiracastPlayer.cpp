///*
//基于硬解码的Miracast 投屏
//硬解码网络流
//播放rtp流
//*/
//
//#ifdef _WIN32
//#include <WinSock2.h>
//#include "WXMediaCpp.h"
//#include "FfmpegIncludes.h"
//
//extern int g_nStreamWidth/* = 0*/;
//extern int g_nStreamHeight/* = 0*/;
//static int s_bFixed = FALSE;
//static int s_modeRender = RENDER_TYPE_D3DX;
//static int s_nRotate = RENDER_ROTATE_NONE;
//static int s_modeStream = MIRACAST_THREAD_AUDIO_VIDEO;// MIRACAST_THREAD_NO;
//
//#define TS_PACKET_LEN 188
//
//class MiracastDataQueue {
//public:
//	ThreadSafeQueue<WXDataBuffer*>m_queue;
//public:
//	MiracastDataQueue() {}
//	virtual ~MiracastDataQueue() {
//		Flush();
//	}
//	void  Push(uint8_t *buf, int buf_size) {
//		WXDataBuffer *data = new WXDataBuffer(buf, buf_size);
//		m_queue.Push(data);
//	}
//
//	WXDataBuffer *Pop() {
//		return m_queue.Pop();
//	}
//	void Flush() {
//		m_queue.Flush();
//	}
//};
//
//class MiracastVideoRender :public WXThread {
//public:
//	void*  m_pSink = nullptr;//回调对象
//	ffplayOnSize m_cbSize = nullptr;//回调显示图像大小
//	void *m_h264Dec = nullptr;//视频解码器
//	int m_iWidth = 0;
//	int m_iHeight = 0;
//	HWND m_hWnd = nullptr;
//	void *m_pRender = nullptr;
//	int64_t m_ptsLast = 0;
//
//	MiracastDataQueue m_queueVideo;
//	virtual void ThreadProcess() {
//		WXDataBuffer *data = m_queueVideo.Pop();
//		if (data) {
//			DoVideo(data->GetBuffer(), data->m_iBufSize);
//			delete data;
//		}else {
//			SLEEPMS(1);
//		}
//	}
//	virtual  void ThreadPost() {
//		//退出
//		m_queueVideo.Flush();
//		if (m_pRender) {
//			WXVideoRenderDestroy(m_pRender);
//			m_pRender = nullptr;
//		}
//
//		if (m_h264Dec) {
//			WXH264DecDestroy(m_h264Dec);
//			m_h264Dec = nullptr;
//		}
//
//
//	}//线程循环结束后的退出处理
//	void DoVideo(uint8_t* es_data, int es_data_len) {
//		if (nullptr == m_h264Dec) {
//			m_h264Dec = WXH264DecCreate(es_data, es_data_len);
//			WXLogWriteNewW(L"ProcessMepgtsData WXH264DecCreate AAA");
//			if (m_h264Dec) {
//				int w = 0;
//				int h = 0;
//				H264GetSize(es_data, es_data_len, &w, &h);
//				g_nStreamWidth  = m_iWidth  = w;
//				g_nStreamHeight = m_iHeight = h;
//				WXLogWriteNewW(L"ProcessMepgtsData WXH264DecCreate BBB %dx%d", m_iWidth, m_iHeight);
//				if (m_iWidth && m_iHeight) {
//					WXLogWriteNewW(L"ProcessMepgtsData WXH264DecCreate CCC %dx%d", m_iWidth, m_iHeight);
//					if (m_cbSize) {
//						m_cbSize(m_pSink, s_nRotate, m_iWidth, m_iHeight);
//					}
//					if (m_hWnd && nullptr == m_pRender) {
//						m_pRender = WXVideoRenderCreate(m_hWnd, m_iWidth, m_iHeight);
//					}
//				}
//			}
//		}
//		else {
//			WXH264DecSendPacket(m_h264Dec, es_data, es_data_len, 0);
//			int64_t pts = WXGetTimeMs();
//			if (pts - m_ptsLast >= 40) { //高帧率影响性能，降低到最高50fps
//				m_ptsLast = pts;
//				AVFrame *videoFrame = WXH264DecGetFrame(m_h264Dec, TYPE_CURR_FRAME);
//				if (m_pRender) {
//					WXAirplayPush(videoFrame);
//					WXVideoRenderChangeMode(m_pRender, s_modeRender);
//					WXVideoRenderDisplay(m_pRender, videoFrame, s_bFixed, s_nRotate);
//				}
//			}
//		}
//	}
//};
//
//class MiracastAudioRender :public WXThread {
//public:
//	bool m_bOpenAudio = false;
//	AacDecoder m_pAacDecoder;
//	void *m_pSoundPlay = nullptr;
//	MiracastDataQueue m_queueAudio;
//	std::thread *m_threadAudio = nullptr;
//	virtual  void ThreadProcess() {
//		WXDataBuffer *data = m_queueAudio.Pop();
//		if (data) {
//			DoAudio(data->GetBuffer(), data->m_iBufSize);
//			delete data;
//		}
//		else {
//			SLEEPMS(1);
//		}
//	}
//	virtual  void ThreadPost() {
//		//退出
//		m_pAacDecoder.Close();
//		m_queueAudio.Flush();
//		if (m_pSoundPlay) {
//			WXSoundPlayerDestroy(m_pSoundPlay);
//			m_pSoundPlay = nullptr;
//		}
//	}//线程循环结束后的退出处理
//	void DoAudio(uint8_t* es_data, int es_data_len) {
//		if (!m_bOpenAudio) {
//			m_pAacDecoder.Open();
//			m_bOpenAudio = true;
//		}
//		if (m_bOpenAudio) {
//			uint8_t *pcm = nullptr;
//			int size = 0;
//			int ret = m_pAacDecoder.DecodeFrame(es_data, es_data_len, pcm, size);
//			if (ret) {
//				if (m_pSoundPlay == nullptr && m_pAacDecoder.GetSampleRate() && m_pAacDecoder.GetChannel()) {
//					m_pSoundPlay = WXSoundPlayerCreate(m_pAacDecoder.GetSampleRate(), m_pAacDecoder.GetChannel());
//				}
//				if (m_pSoundPlay) {
//					WXSoundPlayerWriteData(m_pSoundPlay, pcm, size);
//				}
//			}
//		}
//	}
//};
//
//EXTERN_C static void _PushTsData(void *ctx, uint8_t *buf, int buf_size);
//class MiracastPlayer :public WXThread{
//	WXLocker m_mutex;
//	WXFifo m_fifo;
//	void *m_pTsDemux = nullptr;
//
//	MiracastVideoRender m_video;
//	MiracastAudioRender m_audio;
//	void *m_udp = nullptr;
//
//public:
//	virtual  void ThreadPrepare() {
//		WXLogWriteNew("%s", __FUNCTION__);
//
//		m_video.ThreadSetName(L"Miracast Video!");
//		m_video.ThreadStart();//视频处理
//
//		m_audio.ThreadSetName(L"Miracast Audio!");
//		m_audio.ThreadStart();//音频处理
//
//		m_pTsDemux = TSDemuxCreate();//MPEGTS处理
//		m_udp = WXUdpRecvCreate(62853, this, _PushTsData);//RTP-UDP接收数据
//	}
//
//	virtual void  ThreadProcess() {
//		uint8_t tsData[TS_PACKET_LEN];
//		if (m_fifo.Size() >= TS_PACKET_LEN) {
//			{
//				WXAutoLock al(m_mutex);
//				m_fifo.Read(tsData, TS_PACKET_LEN);
//			}
//			int ret = TSDemuxWriteData(m_pTsDemux, tsData, TS_PACKET_LEN);
//			if (ret == TS_TYPE_AUDIO) {
//				uint8_t *pAudio = nullptr;
//				int nAudioSize = 0;
//				TSDemuxGetAudioData(m_pTsDemux, &pAudio, &nAudioSize);
//				//DoAudio(pAudio, nAudioSize);
//				(s_modeStream == MIRACAST_THREAD_AUDIO || s_modeStream == MIRACAST_THREAD_AUDIO_VIDEO) ?
//					m_audio.m_queueAudio.Push(pAudio, nAudioSize) :
//					m_audio.DoAudio(pAudio, nAudioSize);
//			}
//			else if (ret == TS_TYPE_VIDEO) {
//				if (m_video.m_h264Dec == nullptr) {
//					uint8_t *pExtra = nullptr;
//					int nExtraSize = 0;
//					TSDemuxGetExtraData(m_pTsDemux, &pExtra, &nExtraSize);
//					m_video.DoVideo(pExtra, nExtraSize);
//				}
//				if (m_video.m_h264Dec) {
//					uint8_t *pVideo = nullptr;
//					int nVideoSize = 0;
//					TSDemuxGetVideoData(m_pTsDemux, &pVideo, &nVideoSize);
//					//DoVideo(pVideo, nVideoSize);
//					(s_modeStream == MIRACAST_THREAD_VIDEO || s_modeStream == MIRACAST_THREAD_AUDIO_VIDEO) ?
//						m_video.m_queueVideo.Push(pVideo, nVideoSize) :
//						m_video.DoVideo(pVideo, nVideoSize);
//				}
//			}
//		}
//		else {
//			SLEEPMS(1);
//		}
//	}
//
//	virtual void  ThreadPost() {
//		if (m_udp) {
//			WXUdpRecvDestroy(m_udp);
//			m_udp = nullptr;
//		}
//		m_video.ThreadStop();
//		m_audio.ThreadStop();
//		if (m_pTsDemux) {
//			TSDemuxDestroy(m_pTsDemux);
//			m_pTsDemux = nullptr;
//		}
//	}
//
//	void Start(WXCTSTR wszName, HWND hWnd, int bHwDecode, void *pSink, ffplayOnSize cbSize) {
//		//m_strFileName.Format(wszName);
//		m_video.m_pSink = pSink;
//		m_video.m_cbSize = cbSize;
//		m_video.m_hWnd = hWnd;
//		g_nStreamWidth = 0;
//		g_nStreamHeight = 0;
//		WXH264DecSetHw(bHwDecode);
//		m_fifo.Init(1920 * 1080 * 10);
//		ThreadSetName(L"MiracastPlayer");
//		ThreadStart();
//	}
//
//	void Stop() {
//		ThreadStop();
//	}
//
//	void Push(uint8_t *buf, int buf_size) {
//		WXAutoLock al(m_mutex);
//		if (buf_size > 12) {
//			m_fifo.Write(buf + 12, buf_size - 12);
//		}
//	}
//};
//
//EXTERN_C static void _PushTsData(void *ctx, uint8_t *buf, int buf_size) {
//	MiracastPlayer* play = (MiracastPlayer*)ctx;
//	play->Push(buf,buf_size);
//}
////默认有一个读文件的线程
//// 0 表示单线程，在读文件的线程解码音视频并显示播放
//// 1 表示 音频线程独立
//// 2 表示 视频线程独立
//// 3 表示 音视频线程独立
//// 暂时不使用基于PTS的音视频同步
//WXMEDIA_API void StreamPlayerSetThreadMode(int mode) {
//	s_modeStream = av_clip(mode, 0, 3);
//	WXLogWriteNew("%s s_modeStream=%d", __FUNCTION__, s_modeStream);
//}
//
//WXMEDIA_API void StreamPlayerSetFixed(int bFixed) {
//	s_bFixed = !!bFixed;
//	WXLogWriteNew("%s s_bFixed=%d", __FUNCTION__, s_bFixed);
//}
//
//WXMEDIA_API void StreamPlayerSetMode(int mode) {
//	s_modeRender = mode;
//	WXLogWriteNew("%s s_modeRender=%d", __FUNCTION__, s_modeRender);
//}
//
//WXMEDIA_API void StreamPlayerSetRotate(int rotate) {
//	s_nRotate = av_clip(rotate, 0, 3);
//	WXLogWriteNew("%s s_modeRender=%d", __FUNCTION__, s_modeRender);
//}
//
////参数1.H264网络流URL
////参数2.显示窗口
////参数3.是否H264硬解码
////参数4.回调对象
////参数5.分辨率回调函数
//WXMEDIA_API void* StreamPlayerStart(WXCTSTR wszName, HWND hWnd, int bHwDecode, void *pSink, ffplayOnSize cbSize) {
//	MiracastPlayer *play = new MiracastPlayer;
//	WXLogWriteNewW(L"%ws bHwDecode=%d", __FUNCTIONW__, bHwDecode);
//	play->Start(wszName, hWnd, bHwDecode, pSink, cbSize);
//	return (void*)play;
//}
//
//WXMEDIA_API void  StreamPlayerStop(void *p) {
//	if (p) {
//		MiracastPlayer *play = (MiracastPlayer*)p;
//		play->Stop();
//		delete p;
//	}
//}
//
//#endif
