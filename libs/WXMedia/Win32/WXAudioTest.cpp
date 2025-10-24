/*
	音频测试功能
	Add By Tam
	2022.05.10
*/

#include "WXMediaCpp.h"

//音频采集监听，麦克风加上回放
EXTERN_C{
	static void  _onCaptureAudio(void* ctx, uint8_t * buf, int buf_size);
}
class WXMicTest {
	void* m_pSoundPlayer = nullptr;//麦克风回放
	void* m_pAudioCapture = nullptr;//音频采集组件
	int m_nVolume = 0;
	WXLocker m_mutex;
public:
	int GetVolume() {
		return m_nVolume;
	}
	bool Start(WXCTSTR wszDevName) {
		WXAutoLock al(m_mutex);
		WXLogW(L"WXMicTest %ws name=[%ws]", __FUNCTIONW__, wszDevName);
		m_pSoundPlayer = WXSoundPlayerCreate(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS);//麦克风声音回放
		m_pAudioCapture = WXAudioCaptureCreate();
		WXAudioCaptureSetSink(m_pAudioCapture, this, _onCaptureAudio);//设置音频采集回调对象和回调函数
		WXAudioCaptureSetSampleRate(m_pAudioCapture, AUDIO_SAMPLE_RATE);//设置音频采集频率
		WXAudioCaptureSetChannel(m_pAudioCapture, AUDIO_CHANNELS);//设置音频采集声道
		WXAudioCaptureSetSystemDevice(m_pAudioCapture, L"nullptr", 100, 100);//设置扬声器采集，默认100

		if (WXGetSystemVersion() == 10) {
			WXAudioCaptureSetMicDevice(m_pAudioCapture, wszDevName, 100, 100, TRUE, TRUE, TRUE);//设置麦克风采集，默认100
		}
		else {
			WXAudioCaptureSetMicDevice(m_pAudioCapture, wszDevName, 100, 100, FALSE, FALSE, FALSE);//设置麦克风采集，默认100
		}
		int retAudio = WXAudioCaptureInit(m_pAudioCapture);//启动音频设备
		if (retAudio != WX_ERROR_SUCCESS) { //启动失败
			if (m_pAudioCapture) {
				WXAudioCaptureStop(m_pAudioCapture);
				WXAudioCaptureDestroy(m_pAudioCapture);
				m_pAudioCapture = nullptr;
			}
			if (m_pSoundPlayer) {//停止播放
				WXSoundPlayerDestroy(m_pSoundPlayer);
				m_pSoundPlayer = nullptr;
			}
			//WXLogW(L"%ws [%ws] BBBB ERROR", __FUNCTIONW__, wszDevName);
			return FALSE;
		}
		else {
			WXDataBuffer temp;
			temp.Init(nullptr, AUDIO_FRAME_SIZE);
			WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE);
			WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE);
			WXAudioCaptureStart(m_pAudioCapture);//启动采集线程
			//WXLogW(L"%ws [%ws] BBBB OK", __FUNCTIONW__, wszDevName);
		}
		return TRUE;
	}
	void Stop() {
		WXAutoLock al(m_mutex);
		if (m_pAudioCapture) { //停止采集
			WXAudioCaptureStop(m_pAudioCapture);
			WXAudioCaptureDestroy(m_pAudioCapture);
			m_pAudioCapture = nullptr;
		}
		//填充100ms数据继续播放
		WXDataBuffer temp;
		temp.Init(nullptr, AUDIO_FRAME_SIZE * 10);
		WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE * 10);
		SLEEPMS(1000);
		if (m_pSoundPlayer) {//停止播放
			WXSoundPlayerDestroy(m_pSoundPlayer);
			m_pSoundPlayer = nullptr;
		}
	}
	//麦克风数据回调，并播放
	void onAudio(uint8_t* buf, int buf_size) {
		int16_t* pcm = (int16_t*)buf;
		int volume = 0;
		for (int i = 0; i < buf_size / 2; i++) {
			if (abs(pcm[i]) > volume)
				volume = abs(pcm[i]);
		}
		m_nVolume = volume;
		WXSoundPlayerWriteData(m_pSoundPlayer, buf, buf_size);
	}
};

EXTERN_C{
	static void  _onCaptureAudio(void* ptr, uint8_t* buf, int buf_size) {
		WXMicTest* capture = (WXMicTest*)ptr;
		capture->onAudio(buf, buf_size);
	}
}


//功能:启动音频监听功能，如果指定AAC文件名，则同时进行音频播放
//wszDevName: 音频设备的GUID， 或者 "default" / "conf" 等
// bSystem: 是否扬声器设备
//filename : 启动扬声器监听时可以同时播放AAC音频文件， AAC文件名,为NULL时不播放音频，只监听当前系统声音
//返回值: 成功返回非零指针，是否返回0，可能是系统不存在音频设备，或者音频设备打开失败
//static WXLocker s_lockGlobal;
WXMEDIA_API void* WXAudioTestStart(WXCTSTR wszDevName, int bSystem) {
	WXAutoLock al(s_lockGlobal);
	//BEGIN_LOG_FUNC
	//int dw = (int)GetCurrentThreadId();
	//WXLogW(L"+++ %ws  int Thread[%d] BEGIN", __FUNCTIONW__, dw);

	WXMicTest* test = new WXMicTest;
	BOOL bOpen = test->Start(wszDevName);
	if (bOpen) {
		//WXLogW(L"+++ %ws  int Thread[%d] OK  END", __FUNCTIONW__, dw);
		return test;
	}
	else {
		delete test;
		//WXLogW(L"+++ %ws  int Thread[%d] ERROR  END", __FUNCTIONW__, dw);
		return NULL;
	}
}

//功能:关闭扬声器测试
//ptr: 操作句柄，WXAudioTestStart返回值
WXMEDIA_API void  WXAudioTestStop(void* ptr) {
	WXAutoLock al(s_lockGlobal);
	if (ptr) {
		//int dw = (int)GetCurrentThreadId();
		//WXLogW(L"+++ %ws  int Thread[%d]  BEGIN", __FUNCTIONW__, dw);
		WXMicTest* test = (WXMicTest*)ptr;
		//WXLogW(L"+++ %ws  int Thread[%d]  Stop--", __FUNCTIONW__, dw);
		test->Stop();
		//WXLogW(L"+++ %ws  int Thread[%d]  Delete--", __FUNCTIONW__, dw);
		delete test;
		//WXLogW(L"+++ %ws  int Thread[%d]  END", __FUNCTIONW__, dw);
	}

}



WXMEDIA_API int   WXAudioTestGetVolume(void* ptr) {
	if (ptr) {
		WXMicTest* test = (WXMicTest*)ptr;
		return test->GetVolume();
	}
	return 0;
}