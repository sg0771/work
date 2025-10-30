/**************************************************************
���ڼ���Ĭ��������/��˷������������С
***************************************************************/
#include "WasapiListenDevice.h"


//���ý�ȡ���γ���
static int s_arrWaveSystem[1024];
static int s_arrWaveMic[1024];
static int s_lenWave = 0;

//For S16
static void  GetS16DataImpl(int* pDst, int16_t* data, int count) {
	if (s_lenWave > 0) {
		int count1 = count / s_lenWave;
		for (int i = 0; i < s_lenWave; i++) {
			int16_t* buf = data + i * count1;
			int iMax = 0;
			for (int j = 0; j < count1; j++) {
				if (abs(buf[j]) > iMax)iMax = abs(buf[j]);
			}
			pDst[i] = iMax;
		}
	}
}

//For F32
static void  GetF32DataImpl(int* pDst, float* data, int count) {
	if (s_lenWave > 0) {
		int count1 = count / s_lenWave;
		for (int i = 0; i < s_lenWave; i++) {
			float* buf = (float*)data + i * count1;
			float fMax = 0;
			for (int j = 0; j < count1; j++) {
				if (fabs(buf[j]) > fMax)fMax = fabs(buf[j]);
			}
			pDst[i] = (int)(fMax * 32767);
		}
	}
}

//jian
WXMEDIA_API void WXSetListenWaveLength(int n) {
	memset(s_arrWaveSystem, 0, 1024 * sizeof(int));
	memset(s_arrWaveMic, 0, 1024 * sizeof(int));
	if (n < 0 || n > 250)return;
	s_lenWave = n;
}

//��ȡ��������
WXMEDIA_API int WXGetSystemData(int* pData) {
	if (s_lenWave > 0) {
		memcpy(pData, s_arrWaveSystem, s_lenWave * sizeof(int));
		return 1;
	}
	return 0;
}

//��ȡ��������
WXMEDIA_API int WXGetMicData(int* pData) {
	if (s_lenWave > 0) {
		memcpy(pData, s_arrWaveMic, s_lenWave * sizeof(int));
		return 1;
	}
	return 0;
}

//WASAPI COM ��������
int WasapiListenDevice::OnError(HRESULT hr, const wchar_t* wszMsg) {
	Reset();
	m_nError++;
	LogW(L"WasapiListenDevice GetAudioFormat  [%ws] Failed DX_Error [%x]", wszMsg, hr);
	return m_bSystem ? WX_ERROR_SOUND_SYSTEM_OPEN : WX_ERROR_SOUND_MIC_OPEN;
}

void WasapiListenDevice::Reset() {
	if (m_pCaptureAudioClient) {
		m_pCaptureAudioClient->Stop();
	}
	m_pDev = nullptr;
	m_pCaptureAudioClient = nullptr;
	m_pSoundCapture = nullptr;
	m_nVolume = 0;
}
void WasapiListenDevice::ThreadPrepare() {
	WXWasapiNotifyAddDevice((IWasapiDevice*)this);
}

void WasapiListenDevice::ThreadPost() {
	Reset();
	WXWasapiNotifyRemoveDevice((IWasapiDevice*)this);
}

void    WasapiListenDevice::ThreadProcess() {

	if (m_pSoundCapture == nullptr) { //����������Ƶ�豸������
		SLEEPMS(m_nDelay);
		OpenImpl();
	}

	while (m_pSoundCapture && true) { //���ϲɼ�����������MIC������
		UINT32 nNextPacketSize = 0;
		HRESULT hr = m_pSoundCapture->GetNextPacketSize(&nNextPacketSize); //��ѯ���������ж��ٿ�������
		if (SUCCEEDED(hr)) {
			if (nNextPacketSize > 0) {
				BYTE* pData = nullptr;
				UINT32 nSampleRead = 0;
				DWORD dwFlags = 0;
				hr = m_pSoundCapture->GetBuffer(&pData, &nSampleRead, &dwFlags, nullptr, nullptr);
				if (SUCCEEDED(hr)) {
					if (nSampleRead) {
						m_nVolume = m_bFloat32 ?
							GetF32Max((float*)pData, nSampleRead * m_nChannel) :
							GetS16Max((int16_t*)pData, nSampleRead * m_nChannel);
						if (s_lenWave) {
							if (m_bSystem) {
								m_bFloat32 ?
									GetF32DataImpl(s_arrWaveSystem, (float*)pData, nSampleRead * m_nChannel) :
									GetS16DataImpl(s_arrWaveSystem, (int16_t*)pData, nSampleRead * m_nChannel);
							}
							else {
								m_bFloat32 ?
									GetF32DataImpl(s_arrWaveMic, (float*)pData, nSampleRead * m_nChannel) :
									GetS16DataImpl(s_arrWaveMic, (int16_t*)pData, nSampleRead * m_nChannel);
							}
						}

					}
					m_pSoundCapture->ReleaseBuffer(nSampleRead);
				}
			}
			else {
				break;//�����Ѿ�û�ÿ��õ�����
			}
		}
		else {
			Reset();
		}
	}
	
	SLEEPMS(20);
}

//ƥ����ʵ������ʽ
HRESULT WasapiListenDevice::CheckFormat(int bFloat, int nChannel) {

	//WXLogA("%s %d bFloat=%d nChannel=%d", __FUNCTION__, __LINE__, bFloat, nChannel);
	m_bFloat32 = bFloat;
	HRESULT hr = S_OK;
	{
		WAVEFORMATEX* pwfx = nullptr;
		hr = m_pCaptureAudioClient->GetMixFormat(&pwfx);
		if (FAILED(hr) || pwfx == nullptr) {

			WXLogA("m_pCaptureAudioClient->GetMixFormat %s %d hr=0x%08x", __FUNCTION__, __LINE__, hr);

			CComPtr<IPropertyStore>pProps = nullptr;
			hr = m_pDev->OpenPropertyStore(STGM_READ, &pProps);
			if (FAILED(hr) || nullptr == pProps) {
				WXLogA("Device OpenPropertyStore %s %d hr=0x%08x", __FUNCTION__, __LINE__, hr);
				return E_FAIL;
			}
			PROPVARIANT prop;
			hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
			if (FAILED(hr) || PropVariantIsEmpty(prop) || prop.blob.pBlobData == nullptr) {
				WXLogA("pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varName) %s %d hr=0x%08x", __FUNCTION__, __LINE__, hr);
				return E_FAIL;
			}
			WAVEFORMATEX* mft1 = (PWAVEFORMATEX)prop.blob.pBlobData;
			memset(m_bufPwft, 0, 100);
			memcpy(m_bufPwft, mft1, sizeof(WAVEFORMATEX) + mft1->cbSize);
			m_pWFT = (WAVEFORMATEX*)m_bufPwft;//OK
		}
		else {
			//WXLogA("m_pCaptureAudioClient->GetMixFormat %s OK", __FUNCTION__);
			memset(m_bufPwft, 0, 100);
			memcpy(m_bufPwft, pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
			m_pWFT = (WAVEFORMATEX*)m_bufPwft;//OK
			CoTaskMemFree(pwfx);
		}
	}
	//Ԥ������������
	if (nChannel)
		m_pWFT->nChannels = nChannel;

	if (m_pWFT->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(m_pWFT);
		pEx->SubFormat = bFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
		pEx->Samples.wValidBitsPerSample = bFloat ? 32 : 16;
	}
	else {
		m_pWFT->wFormatTag = bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	}
	m_pWFT->wBitsPerSample = bFloat ? 32 : 16;
	m_pWFT->nBlockAlign = m_pWFT->nChannels * m_pWFT->wBitsPerSample / 8;
	m_pWFT->nAvgBytesPerSec = m_pWFT->nBlockAlign * m_pWFT->nSamplesPerSec;

	m_nSampleRate = m_pWFT->nSamplesPerSec;
	m_nChannel = m_pWFT->nChannels;
	m_nBlockAlign = m_pWFT->nBlockAlign;

	hr = m_pCaptureAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
		m_bSystem ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
		0, 0, m_pWFT, 0);
	return hr;
}

void   WasapiListenDevice::ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
	if (flow == m_flow && role == m_role && (role == eConsole || role == eMultimedia)) { //������Ƶ��Ĭ���豸�ı�
		WXTask task = [this] { //ǿ�ƹرյ�ǰ�豸
			if (m_pCaptureAudioClient) {
				m_pCaptureAudioClient->Stop();
				m_pCaptureAudioClient = nullptr;
			}
			m_pSoundCapture = nullptr;
			m_pDev = nullptr;
			m_nDelay = 1000;
		};
		RunTask(task);
	}
}

int  WasapiListenDevice::Open(int bSystem, int bComm) {
	if(this->IsRunning())
		return WX_ERROR_SUCCESS;

	m_bSystem = bSystem;
	m_bComm = bComm;
	m_flow = bSystem ? eRender : eCapture;
	m_role = bComm ? eCommunications : eConsole;
	ThreadSetName(L"WasapiListenDevice");
	ThreadStart(true);
	return WX_ERROR_SUCCESS;
}

int   WasapiListenDevice::OpenImpl() {


	IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
	if (nullptr == pEnum) {
		WXLogA("CMMNotificationClient::GetEnum() failed");
		return WX_ERROR_ERROR;
	}
	HRESULT hr = S_OK;
	if (eCommunications == m_role) {
		//Ĭ��ͨ���豸
		WXString strDefault = L"";
		CComPtr<IMMDevice>pDev = nullptr;
		hr = pEnum->GetDefaultAudioEndpoint(m_flow, eConsole, &pDev);//Ĭ���豸����Ĭ��ͨ���豸
		if (SUCCEEDED(hr)) {
			CComPtr<IPropertyStore>pProps = nullptr;
			hr = pDev->OpenPropertyStore(STGM_READ, &pProps);
			if (SUCCEEDED(hr)) {
				//��ȡ�豸����
				PROPVARIANT varName;
				PropVariantInit(&varName);
				hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
				if (SUCCEEDED(hr)) {
					strDefault = varName.bstrVal;
					PropVariantClear(&varName);
				}
			}
		}
		WXString  strDevName = L"";
		m_pDev = nullptr;
		hr = pEnum->GetDefaultAudioEndpoint(m_flow, m_role, &m_pDev);//Ĭ���豸����Ĭ��ͨ���豸
		if (FAILED(hr) || nullptr == m_pDev) {
			return OnError(hr, L"m_pEnum->GetDefaultAudioEndpoint");
		}
		CComPtr<IPropertyStore>pProps = nullptr;
		hr = m_pDev->OpenPropertyStore(STGM_READ, &pProps);
		if (SUCCEEDED(hr)) {
			//��ȡ�豸����
			PROPVARIANT varName;
			PropVariantInit(&varName);
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			strDevName = varName.bstrVal;
			PropVariantClear(&varName);
			LogW(L"WasapiListenDevice [%ws][System=%d][Comm=%d]", strDevName.str(),m_bSystem,m_bComm);
		}
		if (strDevName == strDefault) {
			m_nError++;
			m_nDelay = 500;
			//LogW(L"Ĭ���豸��Ĭ��ͨ���豸һ�£�", strDevName.str());
			return WX_ERROR_ERROR;
		}
	}
	else {
		WXString  strDevName = L"";
		m_pDev = nullptr;
		hr = pEnum->GetDefaultAudioEndpoint(m_flow, m_role, &m_pDev);//Ĭ���豸����Ĭ��ͨ���豸
		if (FAILED(hr) || nullptr == m_pDev) {
			return OnError(hr, L"m_pEnum->GetDefaultAudioEndpoint");
		}
		CComPtr<IPropertyStore>pProps = nullptr;
		hr = m_pDev->OpenPropertyStore(STGM_READ, &pProps);
		if (SUCCEEDED(hr)) {
			//��ȡ�豸����
			PROPVARIANT varName;
			PropVariantInit(&varName);
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			strDevName = varName.bstrVal;
			PropVariantClear(&varName);			
			LogW(L"WasapiListenDevice [%ws][System=%d][Comm=%d]", strDevName.str(), m_bSystem, m_bComm);
		}
	}

	m_pCaptureAudioClient = nullptr;
	hr = m_pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_pCaptureAudioClient);
	if (FAILED(hr) || nullptr == m_pCaptureAudioClient) {
		return OnError(hr, L"m_pDev->Activate");
	}
	hr = E_FAIL;

	if (FAILED(hr)) {
		hr = CheckFormat(TRUE, 0);
	}


	if (FAILED(hr)) {
		hr = CheckFormat(FALSE, 0);
	}

	if (FAILED(hr)) {
		hr = CheckFormat(TRUE, 2);
	}


	if (FAILED(hr)) {
		hr = CheckFormat(FALSE, 2);
	}

	if (FAILED(hr)) {
		hr = CheckFormat(TRUE, 1);
	}

	if (FAILED(hr)) {
		hr = CheckFormat(FALSE, 1);
	}


	if (FAILED(hr)) {
		return OnError(hr, L"Wasapi Listen Device");
	}

	//��ʽ���óɹ�
	m_pSoundCapture = nullptr;
	hr = m_pCaptureAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_pSoundCapture);
	if (FAILED(hr)) {
		return OnError(hr, L"m_pCaptureAudioClient->GetService");
	}

	hr = m_pCaptureAudioClient->Start(); //�豸��ʼ��

	if (SUCCEEDED(hr)) {
		m_nChannel = m_pWFT->nChannels;
		//LogW(L"%ws OK Channel=%d", __FUNCTIONW__, m_nChannel);
		m_nVolume = 0;
		return WX_ERROR_SUCCESS;
	}else {
		Reset();
		return OnError(hr, L"m_pCaptureAudioClient->Start()");
	}
}

void  WasapiListenDevice::Close() {
	ThreadStop();//ԭʼ���ݲɼ��߳�
	Reset();
}

int   WasapiListenDevice::GetVolume() {
	return m_nVolume;
}



WXMEDIA_API void AudioDeviceListenComm(int bComm) {

}

static WasapiListenDevice g_defaultSpeak; //������Ĭ���豸
static WasapiListenDevice g_commSpeak;    //������Ĭ��ͨ���豸

static WasapiListenDevice g_defaultMic;   //��˷�Ĭ���豸
static WasapiListenDevice g_commMic;      // ��˷�Ĭ��ͨ���豸

//��ȡ������С
WXMEDIA_API  int AudioDeviceGetVolume2(int type, int bComm) {
	return AudioDeviceGetVolume(type);
}

WXMEDIA_API  int AudioDeviceGetVolume(int type) {
	int value = 0;
	if (type == AUDIO_DEVIDE_MIC) {
		int v1 = g_commMic.GetVolume();
		int v2 = g_defaultMic.GetVolume();
		value = v1 + v2;
	}else if (type == AUDIO_DEVIDE_SYS) {
		int v1 = g_commSpeak.GetVolume();
		int v2 = g_defaultSpeak.GetVolume();
		value = v1 + v2;
	}else if (type == AUDIO_DEVIDE_ALL) {
		int v1 = g_commSpeak.GetVolume();
		int v2 = g_defaultSpeak.GetVolume();
		int v3 = g_commMic.GetVolume();
		int v4 = g_defaultMic.GetVolume();
		value = v1 + v2 + v3 + v4;
	}
	if (value > 32767)value = 32767;
	if (value < 0)
		value = 0;
	return value;
}


//��������
//��������
#define AUDIO_DEVIDE_MIC   0      //0 ��˷� Ĭ���豸
#define AUDIO_DEVIDE_SYS   1      //1 ������ Ĭ���豸
#define AUDIO_DEVIDE_ALL   2      //2 ������+��˷� Ĭ���豸

#define AUDIO_DEVIDE_MIC_CONF    3   //3 ��˷� Ĭ���豸+Ĭ��ͨ���豸
#define AUDIO_DEVIDE_SYS_CONF    4   //4 ������ Ĭ���豸+Ĭ��ͨ���豸
#define AUDIO_DEVIDE_CONF        5   //5 ������+��˷� Ĭ���豸+Ĭ��ͨ���豸 

WXMEDIA_API  void  AudioDeviceOpen(WXCTSTR guid, int type) {
	//BEGIN_LOG_FUNC
	if (type == AUDIO_DEVIDE_MIC) {
		g_defaultSpeak.Open(TRUE, FALSE);
	}
	else if (type == AUDIO_DEVIDE_SYS) {
		g_defaultMic.Open(FALSE, FALSE);
	}
	else if (AUDIO_DEVIDE_ALL == type) {
		g_defaultSpeak.Open(TRUE, FALSE);
		g_defaultMic.Open(FALSE, FALSE);
	}

	else if (type == AUDIO_DEVIDE_SYS_CONF) {
		g_defaultSpeak.Open(TRUE, FALSE);
		g_commSpeak.Open(TRUE, FALSE);
	}
	else if (type == AUDIO_DEVIDE_MIC_CONF) {
		g_defaultMic.Open(FALSE, FALSE);
		g_commMic.Open(FALSE, TRUE);
	}
	else if (AUDIO_DEVIDE_CONF == type) {
		g_defaultSpeak.Open(TRUE, FALSE);
		g_commSpeak.Open(TRUE, FALSE);
		g_defaultMic.Open(FALSE, FALSE);
		g_commMic.Open(FALSE, TRUE);
	}
}

WXMEDIA_API  void  AudioDeviceClose(int type) {
	if (type == AUDIO_DEVIDE_SYS) { //�ر�����������
		g_defaultSpeak.Close();
		g_commSpeak.Close();
	}
	else if(type == AUDIO_DEVIDE_MIC) { //�ر���˷����
		g_defaultMic.Close();
		g_commMic.Close();
	}
	else if (type == AUDIO_DEVIDE_ALL) { //�ر������豸����
		g_defaultSpeak.Close();
		g_commSpeak.Close();
		g_defaultMic.Close();
		g_commMic.Close();
	}
}