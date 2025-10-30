/**************************************************************
Vista ����ϵͳ��¼��������
edit by TamXie
2019.08.12 �����豸�쳣��ָ����ź��¼�ƹ���
***************************************************************/
#include "WasapiDevice.h"
#include "WXCapture.h"

//WASAPI COM ��������
int WasapiDevice::OnError(HRESULT hr, WXCTSTR wszMsg) {
	Reset();
	m_nError++;
	LogW(L"WasapiDevice [%ws] GetAudioFormat  [%ws] Failed DX_Error [%x]",m_strName.str(), wszMsg, hr);
	return WX_ERROR_ERROR;
}

void    WasapiDevice::ThreadPrepare() {
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);//�߳����ȼ�

	OpenImpl();
	WXWasapiNotifyAddDevice((IWasapiDevice*)this);

	//LogW(L"++++++ WasapiDevice ThreadPrepare [%ws] WXCond_Wait", m_strName.str());
}

void    WasapiDevice::ThreadWait() {
	if (m_pSoundCapture)
		m_pCaptureAudioClient->Start(); //����豸
}


void    WasapiDevice::ThreadPost() {
	Reset();
	WXWasapiNotifyRemoveDevice((IWasapiDevice*)this);
}

//8889000a �豸��ռ��
//0x88890004 �豸�������˻��߽���֮��
void    WasapiDevice::ThreadProcess() {
	if (m_pSoundCapture == nullptr) { //����������Ƶ�豸������
		SLEEPMS(m_nDelay);
		OpenImpl();
		if (m_pSoundCapture)
			m_pCaptureAudioClient->Start(); //����豸
	}

	while (m_pSoundCapture && true) { //���ϲɼ�����������MIC������
		UINT32 nNextPacketSize = 0;
		HRESULT hr = m_pSoundCapture->GetNextPacketSize(&nNextPacketSize); //��ѯ���������ж��ٿ�������
		if (SUCCEEDED(hr)) {
			if (nNextPacketSize > 0) {
				BYTE *pData = nullptr;
				UINT32 nSampleRead = 0;
				DWORD dwFlags = 0;
				hr = m_pSoundCapture->GetBuffer(&pData, &nSampleRead, &dwFlags, nullptr, nullptr);
				if (SUCCEEDED(hr)) {
					if (nSampleRead) {
						m_AudioResampler.Write(pData, nSampleRead * m_nBlockAlign);//��ʽת����48000 2ch S16 10ms ���ݰ�
						while (m_AudioResampler.Size() >= AUDIO_FRAME_SIZE) {  //д��ԭ����m_outputData
							m_AudioResampler.Read(m_bufAudio.GetBuffer(), AUDIO_FRAME_SIZE);
							m_outputFifo.Write(m_bufAudio.GetBuffer(), AUDIO_FRAME_SIZE);
						}
					}
					m_pSoundCapture->ReleaseBuffer(nSampleRead);
				}
			}else {
				break;//�����Ѿ�û�ÿ��õ�����
			}
		}else {
			//�豸�����á����ߡ���ռ��HRֵ����0x88890004 
			//������ȡ�����á����롢ȡ����ռ���޷��ָ�
			//��Ҫ��������
			Reset();
		}
	}
	SLEEPMS(1);
}


WXFifo * WasapiDevice::GetOutput() {  //¼�����FIFO
	return &m_outputFifo;
}

//ƥ����ʵ������ʽ
HRESULT WasapiDevice::CheckFormat(int bFloat, int nChannel) {
	//WXLogA("%s %d bFloat=%d nChannel=%d", __FUNCTION__, __LINE__, bFloat, nChannel);
	m_bFloat32 = bFloat;
	HRESULT hr = S_OK;
	WAVEFORMATEX* pwfx = nullptr;
	hr = m_pCaptureAudioClient->GetMixFormat(&pwfx);
	if (FAILED(hr) || pwfx == nullptr) {
		WXLogA("%s %d m_pCaptureAudioClient->GetMixFormat hr=0x%08x", __FUNCTION__, __LINE__, hr);
		CComPtr<IPropertyStore>pProps = nullptr;
		hr = m_pDev->OpenPropertyStore(STGM_READ, &pProps);
		PROPVARIANT prop;
		hr = pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &prop);
		if (FAILED(hr) || PropVariantIsEmpty(prop) || prop.blob.pBlobData == nullptr) {
			WXLogA("pProps->GetValue(PKEY_AudioEngine_DeviceFormat, &varName) %s %d hr=0x%08x", __FUNCTION__, __LINE__, hr);
			return S_FALSE;
		}
		pwfx = (PWAVEFORMATEX)prop.blob.pBlobData;
		memset(m_bufPwft, 0, 100);
		memcpy(m_bufPwft, pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
	}else {
		memset(m_bufPwft, 0, 100);
		memcpy(m_bufPwft, pwfx, sizeof(WAVEFORMATEX) + pwfx->cbSize);
		CoTaskMemFree(pwfx);
	}
	m_pWFT = (WAVEFORMATEX*)m_bufPwft;//OK
	//Ԥ������������
	if (nChannel)
		m_pWFT->nChannels = nChannel;

	if (m_pWFT->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(m_pWFT);
		pEx->SubFormat = bFloat ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
		pEx->Samples.wValidBitsPerSample = bFloat ? 32 : 16;
	}else {
		m_pWFT->wFormatTag = bFloat ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
	}
	m_pWFT->wBitsPerSample = bFloat ? 32 : 16;
	m_pWFT->nBlockAlign = m_pWFT->nChannels * m_pWFT->wBitsPerSample / 8;
	m_pWFT->nAvgBytesPerSec = m_pWFT->nBlockAlign * m_pWFT->nSamplesPerSec;
	m_nInSampleRate = m_pWFT->nSamplesPerSec;
	m_nInChannel    = m_pWFT->nChannels;
	m_nBlockAlign   = m_pWFT->nBlockAlign;
	hr = m_pCaptureAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
		m_bSystem ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
		0, 0, m_pWFT, 0);
	return hr;
}

void WasapiDevice::ChangeDefaultDevice(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
	if (m_bNotify && flow == m_flow) {
		//Ĭ���豸����Ĭ��ͨ���豸���л�
		WXTask task = [this] { //ǿ�ƹرյ�ǰ�豸
			LogW(L"Audio ChangeDevice flow=%d role=%d  name=[%ws] guid=[%ws] Count=%d",
				(int)m_flow, (int)m_role, m_strName.str(), m_strGuid.str(), m_nCount);
			m_nCount++;
			if (m_pCaptureAudioClient) {
				m_pCaptureAudioClient->Stop();
				m_pCaptureAudioClient = nullptr;
			}
			m_pSoundCapture = nullptr;
			m_pDev = nullptr;
		};
		RunTask(task);
	}
}

int  WasapiDevice::Open(int bSystem, int bComm) {
	WXLogA("%s bSystem=%d bComm=%d",__FUNCTION__, bSystem, bComm);
	m_bSystem = bSystem;
	m_bufAudio.Init(nullptr, AUDIO_FRAME_SIZE);
	ThreadSetName(L"Wasapi Device");
	ThreadStart(true);
	return WX_ERROR_SUCCESS;
}

int  WasapiDevice::OpenImpl() {
	IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
	if (nullptr == pEnum) {
		WXLogA("CMMNotificationClient::GetEnum() failed");
		return WX_ERROR_ERROR;
	}
	HRESULT hr = S_OK;
	m_pDev = nullptr;
	if (m_strGuid == L"default"){ //Ĭ���豸
		m_flow = m_bSystem ? eRender : eCapture;
		m_role = eConsole;
		m_bNotify = TRUE;
		hr = pEnum->GetDefaultAudioEndpoint(m_flow, m_role, &m_pDev);//Ĭ���豸
	}else 	if (m_strGuid == L"comm") { //Ĭ��ͨ���豸
		m_flow = m_bSystem ? eRender : eCapture;
		m_role = eCommunications;
		m_bNotify = TRUE;
		m_bComm = TRUE;
		hr = pEnum->GetDefaultAudioEndpoint(m_flow, m_role, &m_pDev);//Ĭ��ͨ���豸
	}else{
		hr = pEnum->GetDevice(m_strGuid.str(), &m_pDev);//ָ��GUID�ɼ�
	}

	if (FAILED(hr) || nullptr == m_pDev) {
		return OnError(hr, L"IMMDevice Device creation");
	}

	CComPtr<IPropertyStore>pProps = nullptr;
	hr = m_pDev->OpenPropertyStore(STGM_READ, &pProps);
	if (FAILED(hr) || nullptr == pProps ) {
		return OnError(hr, L"IMMDevice Device OpenPropertyStore");
	}
	//��ȡ�豸����
	PROPVARIANT varName;
	PropVariantInit(&varName);
	hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
	m_strName = varName.bstrVal;
	PropVariantClear(&varName);

	if (m_bComm) {
		//���Ĭ���豸����
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
		if (m_strName == strDefault) {
			m_nError++;
			m_nDelay = 500;
			//LogW(L"Comm Device is Default Device!!!!");
			Reset();
			return WX_ERROR_ERROR;
		}
	}
	//if (wcsstr(m_strName.str(),L"Virtual") != nullptr) {
	//	LogW(L"WasapiDevice Failed Error[%ws]", m_strName.str());
	//	return WX_ERROR_ERROR;
	//}
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
		return OnError(hr, L"Wasapi Device");
	}

	//��ʽ���óɹ�
	m_pSoundCapture = nullptr;
	hr = m_pCaptureAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_pSoundCapture);
	if (FAILED(hr)) {
		return OnError(hr, L"m_pCaptureAudioClient->GetService Error");
	}

	if(SUCCEEDED(hr)) {
		LogW(L"\r\n\tIMMDevice Open\r\n\tname=[%ws]\r\n\tCaptureSamplerate=%d CaptureChannel=%d bFloat=%d",
			m_strName.str(),
			m_nInSampleRate,
			m_nInChannel,
			m_bFloat32);
		m_AudioResampler.Init(m_bFloat32, m_nInSampleRate, m_nInChannel,FALSE, AUDIO_SAMPLE_RATE, AUDIO_CHANNELS);
		m_nDelay = 50;
		return WX_ERROR_SUCCESS;
	}else {
		return OnError(hr, L"m_pCaptureAudioClient->Start()");
	}
}

void    WasapiDevice::Close() {
	ThreadStop();//ԭʼ���ݲɼ��߳�
}

void  WasapiDevice::Reset() {
	if (m_pCaptureAudioClient) {
		m_pCaptureAudioClient->Stop();
		m_pCaptureAudioClient = nullptr;
	}
	m_pSoundCapture = nullptr;
	m_pDev = nullptr;
}

