/*
WXCapture音频处理部分
*/

#include "AudioMixer.h"
#include "AudioMixerWithProcess.h"
#include "WasapiDevice.h"
#include <WXMediaCpp.h>
#include "AudioDataSound.hpp"

class  WXCaptureAudio : public WXThread {
public:
	int m_nOutSampleRate = AUDIO_SAMPLE_RATE;
	int m_nOutChannel    = AUDIO_CHANNELS;
	int m_iOutFrameSize  = AUDIO_FRAME_SIZE;// (AUDIO_AAC_SAMPLE * 4);//For AAC

	//多个输出
	std::vector<void*> m_arrSink;//回调对象
	std::vector<OnAudioData> m_arrFunc;//回调函数

	WXString m_strSystem = L"nullptr";
	int m_nSystemLevel = 100;
	int m_nSystemScale = 100;

	WXString m_strMic = L"nullptr";
	int m_nMicLevel = 100;
	int m_nMicScale = 100;
	int m_bAGC = 0;
	int m_bNS = 0;
	int m_bVAD = 0;

	AudioResampler m_resampler = L"AudioCapture";

	void SetSampleRate(int nSampleRate) {
		m_nOutSampleRate = nSampleRate;
	}

	void SetChannel(int nChannel) {
		m_nOutChannel = nChannel;
	}

	void SetSystemDevice(WXCTSTR strSystem, int nLevel, int nScale) {
		m_strSystem = strSystem;
		m_nSystemLevel = nLevel;
		m_nSystemScale = nScale;
	}

	void SetMicDevice(WXCTSTR strMic, int nLevel, int nScale, int bAGC, int bNS, int bVAD) {
		m_strMic = strMic;
		m_nMicLevel = nLevel;
		m_nMicScale = nScale;
		m_bAGC = bAGC;
		m_bNS = bNS;
		m_bVAD = bVAD;
	}

	void SetSink(void* sink, OnAudioData cbData) {
		if (cbData) {
			m_arrSink.push_back(sink);
			m_arrFunc.push_back(cbData);
		}
	}
public:
	int64_t m_ptsStart = 0;
	int64_t m_ptsRead = 0;
	WXDataBuffer m_bufAudio;

	std::vector<WasapiDevice*>m_vecDevice;

	AudioMixerWithProcess* m_pAudioMixer = nullptr;//总混音输出

	AudioDataSound* m_pDataSound = nullptr;//投屏数据

	AudioMixer * m_pSystemAudioMixer = nullptr;//扬声器混音器,带音频处理
	
	AudioMixer* m_pMicAudioMixer = nullptr;//MIC混音器

	void AddSystemSoundDevice(LPCWSTR guid, int bComm) {
		if (wcsicmp(guid,L"data")==0) {//DataSound
			m_pDataSound = new AudioDataSound(m_nOutSampleRate,m_nOutChannel);

			if (nullptr == m_pSystemAudioMixer) {
				m_pSystemAudioMixer = new AudioMixer(L"SystemAudioMixer");//扬声器音频混音处理器
				if (nullptr == m_pAudioMixer) {
					m_pAudioMixer = new AudioMixerWithProcess(L"AllAudioMixer");//音频混音器
				}
				m_pAudioMixer->AddSystem(m_pSystemAudioMixer->GetOutput(), m_nSystemScale, m_nSystemLevel);
			}
			m_pSystemAudioMixer->AddInput(m_pDataSound->GetOutput());
		}
		else {
			WasapiDevice* dev = WasapiDeviceGetInstance(TRUE, guid, bComm, this->GetCond(), this->GetFlag());
			if (dev != nullptr) {
				WXLogW(L"Add System Device OK[%ws]", guid);
				m_vecDevice.push_back(dev);
				if (nullptr == m_pSystemAudioMixer) {
					m_pSystemAudioMixer = new AudioMixer(L"SystemAudioMixer");//扬声器音频混音处理器
					if (nullptr == m_pAudioMixer) {
						m_pAudioMixer = new AudioMixerWithProcess(L"AllAudioMixer");//音频混音器
					}
					m_pAudioMixer->AddSystem(m_pSystemAudioMixer->GetOutput(), m_nSystemScale, m_nSystemLevel);
				}
				m_pSystemAudioMixer->AddInput(dev->GetOutput());
			}
		}

	}

	void AddMicSoundDevice(LPCWSTR guid, int bComm) {
		WasapiDevice* dev = WasapiDeviceGetInstance(FALSE, guid, bComm, this->GetCond(), this->GetFlag());
		if (dev != nullptr) {
			WXLogW(L"Add MIC Device OK[%ws]", guid);
			m_vecDevice.push_back(dev);
			if (nullptr == m_pMicAudioMixer) {
				m_pMicAudioMixer = new AudioMixer(L"MicAudioMixer");//扬声器音频混音处理器
				if (nullptr == m_pAudioMixer) {
					m_pAudioMixer = new AudioMixerWithProcess(L"AllAudioMixer");//音频混音器
				}
				m_pAudioMixer->AddMic(m_pMicAudioMixer->GetOutput(),
					m_nMicScale, m_nMicLevel, m_bNS, m_bAGC, m_bVAD);
			}
			m_pMicAudioMixer->AddInput(dev->GetOutput());
		}
	}


	WXCaptureAudio(void* cond, int* bFlag) {
		ThreadSetCond((WXCond*)cond, bFlag);
	}

	virtual ~WXCaptureAudio() {
		Stop();
	}

	//音频线程函数
	virtual void  ThreadPrepare() {

		//LogA("++++++  AudioCapture ThreadPrepare WXCond_Wait");
		m_bufAudio.Init(nullptr, std::max(AUDIO_FRAME_SIZE, m_iOutFrameSize));
	}

	virtual  void ThreadWait() {
		m_ptsStart = WXGetTimeMs();
		if (m_pDataSound) {
			m_pDataSound->Enable(TRUE);
		}
	}

	//定时强制取数据
	//10ms 处理一次采集混音
	virtual void  ThreadProcess() {

		int64_t ptsCurr = WXGetTimeMs() - m_ptsStart - m_ptsRead; 
		while (ptsCurr >= 10){
			ptsCurr -= 10;
			m_ptsRead += 10;
			if (m_pSystemAudioMixer) {
				m_pSystemAudioMixer->Process();
			}
			if (m_pMicAudioMixer) {
				m_pMicAudioMixer->Process();
			}
			m_pAudioMixer->Process();
			m_pAudioMixer->GetOutput()->Read(m_bufAudio.GetBuffer(), AUDIO_FRAME_SIZE);
			m_resampler.Write(m_bufAudio.GetBuffer(), AUDIO_FRAME_SIZE);

			while (m_resampler.Size() >= m_iOutFrameSize) {
				m_resampler.Read(m_bufAudio.GetBuffer(), m_iOutFrameSize);
				if (m_arrFunc.size() > 0) { //编码回调
					for (int i = 0; i < m_arrFunc.size(); i++){
						m_arrFunc[i](m_arrSink[i], m_bufAudio.GetBuffer(), m_iOutFrameSize);
					}
				}
			}
		}

		SLEEPMS(10);//音频编码
	}

	virtual void  ThreadPost() {

		ThreadProcess();//剩余的数据处理
		SAFE_DELETE(m_pAudioMixer);
		SAFE_DELETE(m_pSystemAudioMixer);
		SAFE_DELETE(m_pMicAudioMixer);
		SAFE_DELETE(m_pDataSound);
		for (int i = 0; i < m_vecDevice.size(); i++) {
			WasapiDeviceReleaseInstance(m_vecDevice[i]);
		}
	}

public://API
	//初始化配置
	int Init() {

		m_iOutFrameSize = m_nOutSampleRate * sizeof(int16_t) * m_nOutChannel / 100; //10ms数据量
		m_resampler.Init(FALSE, AUDIO_SAMPLE_RATE, AUDIO_CHANNELS, FALSE, m_nOutSampleRate, m_nOutChannel);

		AudioDeviceResetDefault();//重新获得默认设备名字

		if (m_strSystem.length() != 0 &&
			WXStrcmp(m_strSystem.str(), _T("nullptr")) != 0 &&
			WXStrcmp(m_strSystem.str(), _T("null")) != 0) {
			if (WXStrcmp(m_strSystem.str(), _T("conf")) == 0) {
				//会议模式，使用默认设备和默认通信设备
				//支持切换，但是如果默认设备和默认通信设备是同一个，只能启动一个采集
				AddSystemSoundDevice(L"default", 0);
				AddSystemSoundDevice(L"comm", 1);
			}
			else if (WXStrcmp(m_strSystem.str(), _T("all")) == 0 ||
				WXStrcmp(m_strSystem.str(), _T("default")) == 0
				) {	//普通模式，使用默认设备
				AddSystemSoundDevice(L"default", 0);
			}
			else if (WXStrcmp(m_strSystem.str(), _T("data")) == 0) {	//普通模式，使用默认设备
				AddSystemSoundDevice(L"data", 0);
			}
			else {
				//判断一下是否默认设备
				WXCTSTR wszDefault = WXWasapiGetDefaultGuid(TRUE);
				WXCTSTR wszComm = WXWasapiGetDefaultCommGuid(TRUE);
				if (wcsicmp(m_strSystem.str() , wszDefault) == 0) {
					AddSystemSoundDevice(L"default", FALSE);
				}else if (wcsicmp(m_strSystem.str(), wszComm) == 0) {
					AddSystemSoundDevice(L"comm", TRUE);
				}else {
					AddSystemSoundDevice(m_strSystem.str(), 0);//指定GUID采集，不支持动态切换
				}
			}
		}

		if (m_strMic.length() != 0 && 
			WXStrcmp(m_strMic.str(), _T("nullptr")) != 0 && 
			WXStrcmp(m_strMic.str(), _T("null")) != 0) {
			if (WXStrcmp(m_strMic.str(), _T("conf")) == 0) { 
				//会议模式，使用默认设备和默认通信设备
				//支持切换，但是如果默认设备和默认通信设备是同一个，只能启动一个采集
				AddMicSoundDevice(L"default", 0);
				AddMicSoundDevice(L"comm", 1);
			}else if (WXStrcmp(m_strMic.str(), _T("all")) == 0 || 
				WXStrcmp(m_strMic.str(), _T("default")) == 0) {//普通模式，使用默认设备
				AddMicSoundDevice(L"default", 0);
			}else {
				//判断一下是否默认设备
				WXCTSTR wszDefault = WXWasapiGetDefaultGuid(FALSE);
				WXCTSTR wszComm = WXWasapiGetDefaultCommGuid(FALSE);
				if (wcsicmp(m_strMic.str(), wszDefault) == 0) {
					AddMicSoundDevice(L"default", FALSE);
				}else if (wcsicmp(m_strMic.str(), wszComm) == 0) {
					AddMicSoundDevice(L"comm", TRUE);
				}else {
					AddMicSoundDevice(m_strMic.str(), 0);//指定GUID采集
				}
			}
		}

		if (m_pAudioMixer) {
			return WX_ERROR_SUCCESS;
		}else {
			return WX_ERROR_ERROR;
		}
	}

	void Start() { //启动
		ThreadSetName(L"WXCaptureAudio");
		ThreadStart();
	}

	//void SetFrameSize(int size) {
	//	m_iOutFrameSize = size;
	//}

	void  Stop() { //结束
		ThreadStop();
	}

	int   GetAudioLevel(int bSystem) {
		return 0;
	}

	void  SetAudioLevel(int bSystem, int nLevel) {
		m_pAudioMixer->ChangeLevel(bSystem, nLevel);
	}


	int   GetAudioScale(int bSystem) {
		return 0;
	}

	void  SetAudioScale(int bSystem, int nScale) {
		m_pAudioMixer->ChangeScale(bSystem, nScale);
	}
};

WXMEDIA_API void WXAudioCaptureSetCond(void* ptr, void* cond, int* bFlag)//创建对象.设置线程等待信号
{
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		return capture->ThreadSetCond((WXCond*)cond,bFlag);
	}
}

//音频采集接口API
WXMEDIA_API void* WXAudioCaptureCreate() {
	WXCaptureAudio* capture = new WXCaptureAudio(nullptr, nullptr);
	return (void*)capture;
}

WXMEDIA_API int   WXAudioCaptureInit(void* ptr) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		return capture->Init();
	}
	return WX_ERROR_ERROR;
}

WXMEDIA_API void  WXAudioCaptureSetSampleRate(void* ptr, int nSampleRate) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetSampleRate(nSampleRate);
	}
}

WXMEDIA_API void  WXAudioCaptureSetChannel(void* ptr, int nChannel) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetChannel(nChannel);
	}
}

WXMEDIA_API void  WXAudioCaptureSetSystemDevice(void* ptr, WXCTSTR strSystem, int nLevel, int nScale) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetSystemDevice(strSystem, nLevel, nScale);
	}
}

WXMEDIA_API void  WXAudioCaptureSetMicDevice(void* ptr, WXCTSTR strMic, int nLevel, int nScale, int bAGC, int bNS, int bVAD) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetMicDevice(strMic, nLevel, nScale, bAGC, bNS, bVAD);
	}
}

WXMEDIA_API void  WXAudioCaptureSetSink(void* ptr, void* sink, OnAudioData cbData) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetSink(sink, cbData);
	}
}

WXMEDIA_API void  WXAudioCaptureDestroy(void* ptr) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		delete capture;
	}
}

//指定每一帧输出的数据长度
//指定10ms数据量
WXMEDIA_API void      WXAudioCaptureSetFrameSize(void* ptr, int size) {
	//if (ptr) {
	//	WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
	//	capture->SetFrameSize(size);
	//}
}


WXMEDIA_API void  WXAudioCaptureStart(void* ptr) {

	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->Start();
	}
}

WXMEDIA_API void  WXAudioCaptureStop(void* ptr) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->Stop();
	}
}

WXMEDIA_API int   WXAudioCaptureGetAudioLevel(void* ptr, int bSystem) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		return capture->GetAudioLevel(bSystem);
	}
	return 0;
}

WXMEDIA_API void  WXAudioCaptureSetAudioLevel(void* ptr, int bSystem, int level) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetAudioLevel(bSystem, level);
	}
}

WXMEDIA_API int  WXAudioCaptureGetAudioScale(void* ptr, int bSystem) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		return capture->GetAudioScale(bSystem);
	}
	return 100;
}

WXMEDIA_API void WXAudioCaptureSetAudioScale(void* ptr, int bSystem, int level) {
	if (ptr) {
		WXCaptureAudio* capture = (WXCaptureAudio*)ptr;
		capture->SetAudioScale(bSystem, level);
	}
}
