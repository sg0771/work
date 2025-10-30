/*
	��Ƶ���Թ���
	Add By Tam
	2022.05.10
*/

#include "WXMediaCpp.h"

//��Ƶ�ɼ���������˷���ϻط�
EXTERN_C static void  _onCaptureAudio(void* ctx, uint8_t* buf, int buf_size);
class WXMicTest{
	void* m_pSoundPlayer = nullptr;//��˷�ط�
	void* m_pAudioCapture = nullptr;//��Ƶ�ɼ����
	int m_nVolume = 0;
	WXLocker m_mutex;
public:
	int GetVolume() {
		return m_nVolume;
	}
	bool Start(WXCTSTR wszDevName) {
		WXAutoLock al(m_mutex);
		WXLogW(L"WXMicTest %ws name=[%ws]", __FUNCTIONW__, wszDevName);
		m_pSoundPlayer = WXSoundPlayerCreate(AUDIO_SAMPLE_RATE, AUDIO_CHANNELS);//��˷������ط�
		m_pAudioCapture = WXAudioCaptureCreate();
		WXAudioCaptureSetSink(m_pAudioCapture, this, _onCaptureAudio);//������Ƶ�ɼ��ص�����ͻص�����
		WXAudioCaptureSetSampleRate(m_pAudioCapture, AUDIO_SAMPLE_RATE);//������Ƶ�ɼ�Ƶ��
		WXAudioCaptureSetChannel(m_pAudioCapture, AUDIO_CHANNELS);//������Ƶ�ɼ�����
		WXAudioCaptureSetSystemDevice(m_pAudioCapture, L"nullptr", 100, 100);//�����������ɼ���Ĭ��100

		if (WXGetSystemVersion() == 10) {
			WXAudioCaptureSetMicDevice(m_pAudioCapture, wszDevName, 100, 100, TRUE, TRUE, TRUE);//������˷�ɼ���Ĭ��100
		}else {
			WXAudioCaptureSetMicDevice(m_pAudioCapture, wszDevName, 100, 100, FALSE, FALSE, FALSE);//������˷�ɼ���Ĭ��100
		}
		int retAudio = WXAudioCaptureInit(m_pAudioCapture);//������Ƶ�豸
		if (retAudio != WX_ERROR_SUCCESS) { //����ʧ��
			if (m_pAudioCapture) {
				WXAudioCaptureStop(m_pAudioCapture);
				WXAudioCaptureDestroy(m_pAudioCapture);
				m_pAudioCapture = nullptr;
			}
			if (m_pSoundPlayer) {//ֹͣ����
				WXSoundPlayerDestroy(m_pSoundPlayer);
				m_pSoundPlayer = nullptr;
			}
			//WXLogW(L"%ws [%ws] BBBB ERROR", __FUNCTIONW__, wszDevName);
			return FALSE;
		}else {
			WXDataBuffer temp;
			temp.Init(nullptr, AUDIO_FRAME_SIZE);
			WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE);
			WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE);
			WXAudioCaptureStart(m_pAudioCapture);//�����ɼ��߳�
			//WXLogW(L"%ws [%ws] BBBB OK", __FUNCTIONW__, wszDevName);
		}
		return TRUE;
	}
	void Stop() {
		WXAutoLock al(m_mutex);
		if (m_pAudioCapture) { //ֹͣ�ɼ�
			WXAudioCaptureStop(m_pAudioCapture);
			WXAudioCaptureDestroy(m_pAudioCapture);
			m_pAudioCapture = nullptr;
		}
		//���100ms���ݼ�������
		WXDataBuffer temp;
		temp.Init(nullptr, AUDIO_FRAME_SIZE *10);
		WXSoundPlayerWriteData(m_pSoundPlayer, temp.GetBuffer(), AUDIO_FRAME_SIZE * 10);
		SLEEPMS(1000);
		if (m_pSoundPlayer) {//ֹͣ����
			WXSoundPlayerDestroy(m_pSoundPlayer);
			m_pSoundPlayer = nullptr;
		}
	}
	//��˷����ݻص���������
	void onAudio(uint8_t* buf, int buf_size) {
		int16_t* pcm = (int16_t*)buf;
		int volume = 0;
		for (int  i = 0; i < buf_size/2; i++){
			if (abs(pcm[i]) > volume)
				volume = abs(pcm[i]);
		}
		m_nVolume = volume;
		WXSoundPlayerWriteData(m_pSoundPlayer,buf, buf_size);
	}
};

EXTERN_C static void  _onCaptureAudio(void* ptr, uint8_t* buf, int buf_size) {
	WXMicTest* capture = (WXMicTest*)ptr;
	capture->onAudio(buf, buf_size);
}


//����:������Ƶ�������ܣ����ָ��AAC�ļ�������ͬʱ������Ƶ����
//wszDevName: ��Ƶ�豸��GUID�� ���� "default" / "conf" ��
// bSystem: �Ƿ��������豸
//filename : ��������������ʱ����ͬʱ����AAC��Ƶ�ļ��� AAC�ļ���,ΪNULLʱ��������Ƶ��ֻ������ǰϵͳ����
//����ֵ: �ɹ����ط���ָ�룬�Ƿ񷵻�0��������ϵͳ��������Ƶ�豸��������Ƶ�豸��ʧ��
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
	}else {
		delete test;
		//WXLogW(L"+++ %ws  int Thread[%d] ERROR  END", __FUNCTIONW__, dw);
		return NULL;
	}
}

//����:�ر�����������
//ptr: ���������WXAudioTestStart����ֵ
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