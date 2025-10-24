/***************************************************************

使用SDL 播放S16音频数据

***************************************************************/
#if 1

#include "WXMediaCpp.h"
#include "LibInst.hpp"

//一般情况下不用考虑默认设备插拔切换的问题
#define MAX_AUDIO_BUF 5
class DSoundPlayer :public WXThread {
	WXString m_strDevName = L"None";

	int m_nSampleRate = 0;//采样频率
	int m_nChannel = 0; //声道数量
	int m_nSize = 0;//10ms输入数据量

	int64_t m_drop = 0;//丢弃帧
	int64_t m_nWrite = 0;//写入
	int64_t m_nRead = 0;//读取

	WXFifo m_audioFifo;

	CComPtr<IDirectSound8> m_pDS = NULL;
	CComPtr<IDirectSoundBuffer> m_pDSBuffer = NULL;
	CComPtr<IDirectSoundBuffer8>m_pDSBuffer8 = NULL;
	CComPtr<IDirectSoundNotify>m_pDSNotify = NULL;
	HANDLE m_event = nullptr;
	DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
	DWORD m_offset = AUDIO_FRAME_SIZE;
	DSBUFFERDESC m_dsbd;
	WAVEFORMATEX m_fmt;

	BOOL OpenImpl() {
		if (LibInst::GetInst().m_libDSound == nullptr)
			return FALSE;
		m_pDSNotify = nullptr;
		m_pDSBuffer8 = nullptr;	//used to manage sound buffers.
		m_pDSBuffer = nullptr;
		m_pDS = nullptr;
		WXString strHR;
		HRESULT hr = LibInst::GetInst().mDirectSoundCreate8(NULL, &m_pDS, NULL);
		if (SUCCEEDED(hr)) {
			strHR = L"DirectSoundCreate8 OK";
			hr = m_pDS->SetCooperativeLevel(::GetDesktopWindow(), DSSCL_NORMAL);
			if (SUCCEEDED(hr)) {
				strHR = L"SetCooperativeLevel OK";
				hr = m_pDS->CreateSoundBuffer(&m_dsbd, &m_pDSBuffer, NULL);
				if (SUCCEEDED(hr)) {
					strHR = L"CreateSoundBuffer OK";
					hr = m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8);
					if (SUCCEEDED(hr)) {
						strHR = L"IID_IDirectSoundBuffer8 OK";
						hr = m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify);
						if (SUCCEEDED(hr)) {
							strHR = L"IID_IDirectSoundNotify OK";
							for (int i = 0; i < MAX_AUDIO_BUF; i++) {
								m_pDSPosNotify[i].dwOffset = i * m_nSize;
								m_pDSPosNotify[i].hEventNotify = m_event;
							}
							hr = m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF, m_pDSPosNotify);
							if (SUCCEEDED(hr)) {
								strHR = L"SetNotificationPositions OK";
								hr = m_pDSBuffer8->SetCurrentPosition(0);
								if (SUCCEEDED(hr)) {
									strHR = L"SetCurrentPosition OK";
									hr = m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
									if (SUCCEEDED(hr)) {
										m_bPlaying.store(true);
									}
								}
							}
						}
					}
				}
			}
		}
		if (FAILED(hr)) {
			m_nError++;
			LogW(L"DSound HR --- %ws ", strHR.str());
		}
		return SUCCEEDED(hr);
	}
	//int64_t m_nWaitTimeOut = 0;
	std::atomic_bool m_bPlaying = false;

	WXDataBuffer m_tmpBuffer;

	int m_nVolume = 100;//默认音量


	void* m_pSink = nullptr;
	OnData m_cbAudio = nullptr;
public:
	virtual  void ThreadPrepare() {
		//WXLogW(L"Dsound Play Start!!!!!");
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);//线程优先级
		OpenImpl();
	}

	void   ThreadWait() {
	}

	virtual  void ThreadPost() {
		if (m_pDSBuffer8)
			m_pDSBuffer8->Stop();
		m_pDSNotify = nullptr;
		m_pDSBuffer8 = nullptr;
		m_pDSBuffer = nullptr;
		m_pDS = nullptr;
	}

	virtual  void ThreadProcess() {
		if (!m_bPlaying.load()) {
			OpenImpl();
			return;
		}
		DWORD res = ::WaitForSingleObjectEx(m_event, 10, FALSE);
		if (res == WAIT_OBJECT_0) {
			LPVOID pData = NULL;
			DWORD  nDataSize = 0;
			HRESULT hr = m_pDSBuffer8->Lock(m_offset, m_nSize, &pData, &nDataSize, NULL, NULL, 0);
			if (SUCCEEDED(hr) && pData) {

				if (m_cbAudio) {
					m_cbAudio(m_pSink, (uint8_t*)pData, nDataSize);//请求数据
				}
				else {
					int Ret = m_audioFifo.Read3((uint8_t*)pData, nDataSize);//请求数据或者回调
					//请求数据
					m_nRead += Ret;
				}


				if (m_nVolume == 0) {//静音状态
					memset(pData, 0, nDataSize);
				}
				else if (m_nVolume == 100) {//不用处理音量

				}
				else {//处理音量
					short* pDst = (short*)pData;
					short* pSrc = (short*)pData;
					for (size_t i = 0; i < nDataSize / 2; i++) {
						pDst[i] = short((int)pSrc[i] * m_nVolume / 100);
					}
				}

				m_pDSBuffer8->Unlock(pData, nDataSize, NULL, 0);
				m_offset += nDataSize;
				m_offset %= (m_nSize * MAX_AUDIO_BUF);
			}
			else {
				//ERROR need Reset
				m_bPlaying.store(false);
				LogW(L"m_pDSBuffer8->Lock ERROR , Reset!!");
				if (m_pDSBuffer8)
					m_pDSBuffer8->Stop();
				m_pDSNotify = nullptr;
				m_pDSBuffer8 = nullptr;	//used to manage sound buffers.
				m_pDSBuffer = nullptr;
				m_pDS = nullptr;//超时过多，可能设备已经无效
			}
		}
		else { //超时
			//m_nWaitTimeOut++;
		}
	}

	BOOL Init(int nSampleRate, int nChannel, void* pSink, OnData cb) {
		m_pSink = pSink;
		m_cbAudio = cb;
		m_nSampleRate = nSampleRate;
		m_nChannel = nChannel;
		m_nSize = m_nSampleRate * m_nChannel * 2 / 100;//10ms
		m_offset = m_nSize;

		m_tmpBuffer.Init(nullptr, m_nSampleRate * m_nChannel * 4 / 100 * 2);

		//音频格式处理
		memset(&m_fmt, 0, sizeof(m_fmt));
		m_fmt.wFormatTag = WAVE_FORMAT_PCM; //音频格式
		m_fmt.nChannels = m_nChannel; //声道
		m_fmt.nSamplesPerSec = m_nSampleRate;//采样频率
		m_fmt.nAvgBytesPerSec = m_nSampleRate * 2 * m_nChannel;
		m_fmt.nBlockAlign = 2 * m_nChannel;
		m_fmt.wBitsPerSample = 16;
		m_fmt.cbSize = 0;
		memset(&m_dsbd, 0, sizeof(m_dsbd));
		m_dsbd.dwSize = sizeof(m_dsbd);
		m_dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
		m_dsbd.dwBufferBytes = MAX_AUDIO_BUF * m_nSize;
		m_dsbd.lpwfxFormat = &m_fmt;
		if (m_event == nullptr)
			m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		return TRUE;
	}
public:
	void SetVolume(int volume) {
		m_nVolume = volume;
	}
	//填充数据
	void WriteData(uint8_t* buf, int buf_size) { //外部直接填充数据
		uint8_t* pcm = buf;
		int pcm_size = buf_size;
		int audio_size = m_audioFifo.Size();//播放缓冲区数据量
		if (audio_size >= 30 * m_nSize) {//缓冲区超过200ms数据就丢包
			m_drop += pcm_size;
			return;
		}
		m_nWrite += pcm_size;
		m_audioFifo.Write(pcm, pcm_size);
	}

	BOOL Start(int nSampleRate, int nChannel, void* pSink = nullptr, OnData cb = nullptr) {
		BOOL bInit = Init(nSampleRate, nChannel, pSink, cb);
		if (bInit) {
			ThreadSetName(L"DsoundPlayer PCM");
			ThreadStart();//默认都能启动，如果有设备问题
		}
		else {
			LogW(L"Do not Support Sound Play!!!");
		}
		return bInit;
	}

	void Stop() {
		for (size_t i = 0; i < MAX_AUDIO_BUF; i++)
		{
			WriteData(m_tmpBuffer.GetBuffer(), m_tmpBuffer.m_iBufSize);
		}
		ThreadStop();
		if (m_event) {
			CloseHandle(m_event);
			m_event = NULL;
		}
	}

	DSoundPlayer() {

	}

	virtual ~DSoundPlayer() {
		this->Stop();
	}
};

//用于实时流数据的播放，和DSoundPlayerWriteData配套使用
WXMEDIA_API void* WXSoundPlayerCreate(int inSampleRate, int inChannel) {
	if (LibInst::GetInst().m_libDSound == nullptr) {
		return nullptr;
	}
	DSoundPlayer* player = new DSoundPlayer;
	bool bRet = player->Start(inSampleRate, inChannel);
	if (!bRet) {
		delete player;
		return nullptr;
	}
	return (void*)player;
}

//回调音频数据
WXMEDIA_API void* WXSoundPlayerCreateEx(int inSampleRate, int inChannel, void* pSink, OnData cb) {
	if (LibInst::GetInst().m_libDSound == nullptr) {
		return nullptr;
	}
	DSoundPlayer* player = new DSoundPlayer;
	bool bRet = player->Start(inSampleRate, inChannel, pSink, cb);
	if (!bRet) {
		delete player;
		return nullptr;
	}
	return (void*)player;
}

//销毁对象
WXMEDIA_API void  WXSoundPlayerDestroy(void* ptr) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		delete player;//容易闪退
	}
}

//设置音量
WXMEDIA_API void  WXSoundPlayerVolume(void* ptr, int volume) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		player->SetVolume(volume);
	}
}

//直接填充音频数据
WXMEDIA_API void  WXSoundPlayerWriteData(void* ptr, uint8_t* buf, int buf_size) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		player->WriteData(buf, buf_size);
	}
}

#endif
