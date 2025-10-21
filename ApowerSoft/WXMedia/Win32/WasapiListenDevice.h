/**************************************************************
基于WASAPI的音频设备采集播放处理

使用静音数据流虽然可以使得扬声器声音比较连续，但是在某些时候会导致系统音量变小

2019.07.27 改为使用双线程来处理音频数据
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
	BOOL m_bFloat32 = FALSE;//输出格式是否FLOAT32数据
	int  m_nVolume = 0;//音量
	EDataFlow m_flow;
	ERole     m_role;
	BOOL     m_bSystem = FALSE;  //是否扬声器
	BOOL     m_bComm = FALSE;
	int      m_nChannel = 2;
	int      m_nSampleRate = 0;
	int      m_nBlockAlign = 0;
	int      OnError(HRESULT hr, const wchar_t *format);

	uint8_t m_bufPwft[100];
	WAVEFORMATEX *m_pWFT = nullptr;
	HRESULT CheckFormat(int bFloat, int nChannel);//有些设备默认格式无法初始化采集！！！
	int     OpenImpl();
	virtual void     Reset();
public:
	virtual  void ThreadPrepare();//线程循环前的初始化
	virtual  void ThreadProcess();//线程循环函数,必须实现
	virtual  void ThreadPost();//线程循环结束后的退出处理
public:
	virtual int      Open(int bSystem, int bComm);
	virtual void     Close();
	virtual int      GetVolume();
	virtual void     ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
};

#endif 