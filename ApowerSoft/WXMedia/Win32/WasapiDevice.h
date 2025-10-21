/**************************************************************
����WASAPI����Ƶ�豸�ɼ����Ŵ���

ʹ�þ�����������Ȼ����ʹ�������������Ƚ�������������ĳЩʱ��ᵼ��ϵͳ������С

2019.07.27 ��Ϊʹ��˫�߳���������Ƶ����
***************************************************************/

#ifndef _WX_WASAPI_DEVICE_H_
#define _WX_WASAPI_DEVICE_H_

#include <WXMediaCpp.h>


class WasapiDevice:public WXThread, public IWasapiDevice{
	int m_nDelay = 100;
	BOOL m_bNotify = FALSE;//�Ƿ����Ȳ����Ϣ
	BOOL m_bComm = FALSE;//Ĭ��ͨ���豸
	//Ĭ���豸����Ĭ��ͨ���豸��Ҫ��Ӧ�豸�л���Ϣ
	//ȷ��Ĭ���豸��Ĭ��ͨ���豸һ�µ�ʱ�򲻻��ظ��ɼ����豸������

	int m_nCount = 0;
	EDataFlow m_flow;
	ERole     m_role;
	int       m_bSystem = 0;

	WXString m_strName;//�豸����
	WXString m_strGuid=L"";//�豸GUID

	AudioResampler m_AudioResampler = L"WasapiDevice";

	//�����ɼ�
	CComPtr<IMMDevice>m_pDev = nullptr;
	CComPtr<IAudioClient>m_pCaptureAudioClient = nullptr;
	CComPtr<IAudioCaptureClient>m_pSoundCapture = nullptr;

	//���4����ʱ�Ĵ���
	BOOL  m_bFloat32 = FALSE;//�����ʽ�Ƿ�FLOAT32����
	int  m_nInSampleRate = 0;
	int  m_nInChannel = 0;
	int  m_nBlockAlign = 0;

	//�������
	WXFifo       m_outputFifo;
	WXDataBuffer m_bufAudio;//48000 2ch S16 10ms ������

	uint8_t m_bufPwft[100];
	WAVEFORMATEX *m_pWFT = nullptr;

	HRESULT CheckFormat(int bFloat, int nChannel);//��Щ�豸Ĭ�ϸ�ʽ�޷���ʼ���ɼ�������
	int     OnError(HRESULT hr, WXCTSTR wszMsg);
	int     OpenImpl();
    void    Reset();
public:
	virtual  void     ThreadPrepare();
	virtual  void     ThreadWait();
	virtual  void     ThreadProcess();
	virtual  void     ThreadPost();
public:
	virtual int       Open(int bSystem, int bComm);
	virtual void      Close();
	virtual void      ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
public:
	 WXFifo *  GetOutput();
	 WXCTSTR   Name() { return m_strName.str(); }
	 WXCTSTR   Guid() { return m_strGuid.str(); }
	 void SetGuid(WXCTSTR _guid) { m_strGuid=_guid; }
};

//��ȡһ���豸ָ��
WasapiDevice *WasapiDeviceGetInstance(int bSystem, LPCWSTR guid, int bComm, void* cond, int* bFlag);

//�ͷ��豸ָ�룬�������豸�ǵ�ǰ���ڼ������豸����ɾ����������Ҫɾ���豸
void          WasapiDeviceReleaseInstance(WasapiDevice *dev);

#endif 