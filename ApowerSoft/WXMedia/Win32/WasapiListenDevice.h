/**************************************************************
����WASAPI����Ƶ�豸�ɼ����Ŵ���

ʹ�þ�����������Ȼ����ʹ�������������Ƚ�������������ĳЩʱ��ᵼ��ϵͳ������С

2019.07.27 ��Ϊʹ��˫�߳���������Ƶ����
***************************************************************/

#ifndef _WX_WASAPI_DEVICE_H_
#define _WX_WASAPI_DEVICE_H_

#include <WXMediaCpp.h>

class WasapiListenDevice:public IWasapiDevice, public WXThread {
public:
	int m_nDelay = 1000;
	CComPtr<IMMDevice>m_pDev = nullptr;
	CComPtr<IAudioClient>m_pCaptureAudioClient = nullptr;
	CComPtr<IAudioCaptureClient>m_pSoundCapture = nullptr;
	BOOL m_bFloat32 = FALSE;//�����ʽ�Ƿ�FLOAT32����
	int  m_nVolume = 0;//����
	EDataFlow m_flow;
	ERole     m_role;
	BOOL     m_bSystem = FALSE;  //�Ƿ�������
	BOOL     m_bComm = FALSE;
	int      m_nChannel = 2;
	int      m_nSampleRate = 0;
	int      m_nBlockAlign = 0;
	int      OnError(HRESULT hr, const wchar_t *format);

	uint8_t m_bufPwft[100];
	WAVEFORMATEX *m_pWFT = nullptr;
	HRESULT CheckFormat(int bFloat, int nChannel);//��Щ�豸Ĭ�ϸ�ʽ�޷���ʼ���ɼ�������
	int     OpenImpl();
	virtual void     Reset();
public:
	virtual  void ThreadPrepare();//�߳�ѭ��ǰ�ĳ�ʼ��
	virtual  void ThreadProcess();//�߳�ѭ������,����ʵ��
	virtual  void ThreadPost();//�߳�ѭ����������˳�����
public:
	virtual int      Open(int bSystem, int bComm);
	virtual void     Close();
	virtual int      GetVolume();
	virtual void     ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
};

#endif 