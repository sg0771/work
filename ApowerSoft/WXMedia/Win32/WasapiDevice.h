/**************************************************************
基于WASAPI的音频设备采集播放处理

使用静音数据流虽然可以使得扬声器声音比较连续，但是在某些时候会导致系统音量变小

2019.07.27 改为使用双线程来处理音频数据
***************************************************************/

#ifndef _WX_WASAPI_DEVICE_H_
#define _WX_WASAPI_DEVICE_H_

#include <WXMediaCpp.h>


class WasapiDevice:public WXThread, public IWasapiDevice{
	int m_nDelay = 100;
	BOOL m_bNotify = FALSE;//是否处理热插拔消息
	BOOL m_bComm = FALSE;//默认通信设备
	//默认设备或者默认通信设备需要响应设备切换消息
	//确保默认设备和默认通信设备一致的时候不会重复采集该设备！！！

	int m_nCount = 0;
	EDataFlow m_flow;
	ERole     m_role;
	int       m_bSystem = 0;

	WXString m_strName;//设备名字
	WXString m_strGuid=L"";//设备GUID

	AudioResampler m_AudioResampler = L"WasapiDevice";

	//声音采集
	CComPtr<IMMDevice>m_pDev = nullptr;
	CComPtr<IAudioClient>m_pCaptureAudioClient = nullptr;
	CComPtr<IAudioCaptureClient>m_pSoundCapture = nullptr;

	//输出4声道时的处理
	BOOL  m_bFloat32 = FALSE;//输出格式是否FLOAT32数据
	int  m_nInSampleRate = 0;
	int  m_nInChannel = 0;
	int  m_nBlockAlign = 0;

	//输出队列
	WXFifo       m_outputFifo;
	WXDataBuffer m_bufAudio;//48000 2ch S16 10ms 数据量

	uint8_t m_bufPwft[100];
	WAVEFORMATEX *m_pWFT = nullptr;

	HRESULT CheckFormat(int bFloat, int nChannel);//有些设备默认格式无法初始化采集！！！
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

//获取一个设备指针
WasapiDevice *WasapiDeviceGetInstance(int bSystem, LPCWSTR guid, int bComm, void* cond, int* bFlag);

//释放设备指针，如果这个设备是当前正在监听的设备，则不删除，否则需要删除设备
void          WasapiDeviceReleaseInstance(WasapiDevice *dev);

#endif 