
//------------------------------------------------------------------------------------------------
/*
WASAPI �豸����
*/
#include <WXMediaCpp.h>
#include "WasapiDevice.h"
#include "WXCapture.h"



class WasapiDeviceManager {
public:
	WXLocker m_mutex;
	//������-Ĭ���豸
	WXString m_strDefaultSystemGuid = L"nullptr";
	WXString m_strDefaultSystemName = L"nullptr";

	//������-Ĭ��ͨ���豸
	WXString m_strDefaultCommSystemGuid = L"nullptr";
	WXString m_strDefaultCommSystemName = L"nullptr";

	//��˷�-Ĭ���豸
	WXString m_strDefaultMicGuid = L"nullptr";
	WXString m_strDefaultMicName = L"nullptr";

	//��˷�-Ĭ��ͨ���豸
	WXString m_strDefaultCommMicGuid = L"nullptr";
	WXString m_strDefaultCommMicName = L"nullptr";

	std::vector<SoundDeviceInfo> m_vecSoundRenderInfo;
	std::vector<SoundDeviceInfo> m_vecSoundCaptureInfo;

	//���ڼ������豸
	WasapiDevice* m_pListenSystemDevice = nullptr;//��ǰʹ�õ��������豸
	WasapiDevice* m_pListenMicDevice = nullptr;//��ǰʹ�õ�MIC�豸

	//�豸����-GUID ��Ӧ
	std::map<std::wstring, std::wstring>m_mapRenderDevice;
	std::map<std::wstring, std::wstring>m_mapCaptureDevice;

	//��ȡĬ���豸��GUIDֵ������
	void GetDefaultName(BOOL system) {
		if (system) {
			m_strDefaultSystemGuid = L"nullptr";
			m_strDefaultSystemName = L"nullptr";
		}
		else {
			m_strDefaultMicGuid = L"nullptr";
			m_strDefaultMicName = L"nullptr";
		}
		IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
		if (nullptr == pEnum) {
			WXLogA("CMMNotificationClient::GetEnum() failed");
			return;
		}
		HRESULT hr = S_OK;

		if (SUCCEEDED(hr)) {
			CComPtr<IMMDevice>pEndpoint = nullptr;
			hr = pEnum->GetDefaultAudioEndpoint(system ? eRender : eCapture, eConsole, &pEndpoint);//Ĭ���豸
			if (SUCCEEDED(hr)) {
				LPWSTR pwszID = nullptr;
				hr = pEndpoint->GetId(&pwszID);
				if (SUCCEEDED(hr)) {
					CComPtr<IPropertyStore>pProps = nullptr;//���Թ�����
					hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

					PROPVARIANT varName;//�豸��ʾ���֣��п����ظ�
					PropVariantInit(&varName);
					hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
					WXString strName = varName.bstrVal;
					/*if (wcsstr(strName.str(), L"Virtual") != nullptr) {
						WXLogW(L"WasapiDevice Failed Error[%ws]", strName.str());
						CoTaskMemFree(pwszID);
						PropVariantClear(&varName);
						return;
					}*/


					if (system) {
						m_strDefaultSystemGuid = pwszID;
						m_strDefaultSystemName = varName.bstrVal;
					}
					else {
						m_strDefaultMicGuid = pwszID;
						m_strDefaultMicName = varName.bstrVal;
					}
					CoTaskMemFree(pwszID);
					PropVariantClear(&varName);
				}
			}
		}
	}

	//��ȡĬ��ͨ���豸��GUIDֵ������
	void GetDefaultCommName(BOOL system) {
		if (system) {
			m_strDefaultCommSystemGuid = L"nullptr";
			m_strDefaultCommSystemName = L"nullptr";
		}
		else {
			m_strDefaultCommMicGuid = L"nullptr";
			m_strDefaultCommMicName = L"nullptr";
		}
		IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
		if (nullptr == pEnum) {
			WXLogA("CMMNotificationClient::GetEnum() failed");
			return;
		}
		HRESULT hr = S_OK;

		if (SUCCEEDED(hr)) {
			CComPtr<IMMDevice>pEndpoint = nullptr;
			hr = pEnum->GetDefaultAudioEndpoint(system ? eRender : eCapture, eCommunications, &pEndpoint);//Ĭ��ͨ���豸
			if (SUCCEEDED(hr)) {
				LPWSTR pwszID = nullptr;
				hr = pEndpoint->GetId(&pwszID);
				if (SUCCEEDED(hr)) {
					CComPtr<IPropertyStore>pProps = nullptr;//���Թ�����
					hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

					PROPVARIANT varName;//�豸��ʾ���֣��п����ظ�
					PropVariantInit(&varName);
					hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
					WXString strName = varName.bstrVal;
					/*if (wcsstr(strName.str(), L"Virtual") != nullptr) {
						WXLogW(L"WasapiDevice Failed Error[%ws]", strName.str());

						CoTaskMemFree(pwszID);
						PropVariantClear(&varName);
						return;
					}*/

					if (system) {
						m_strDefaultCommSystemGuid = pwszID;
						m_strDefaultCommSystemName = varName.bstrVal;
						WXLogW(L"WASAPI System Default[%ws][%ws]",
							m_strDefaultCommSystemName.str(), m_strDefaultCommSystemGuid.str());
					}
					else {
						m_strDefaultCommMicGuid = pwszID;
						m_strDefaultCommMicName = varName.bstrVal;
						WXLogW(L"WASAPI Mic Default[%s][%s]",
							m_strDefaultCommMicGuid.str(), m_strDefaultCommMicName.str());
					}
					CoTaskMemFree(pwszID);
					PropVariantClear(&varName);
				}
			}
		}
	}

	//��ѯ�����豸
	void ListSoundRenderDevice() {
		m_mapRenderDevice.clear();
		m_vecSoundRenderInfo.clear();
		GetDefaultName(TRUE);
		GetDefaultCommName(TRUE);

		IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
		if (nullptr == pEnum) {
			WXLogA("CMMNotificationClient::GetEnum() failed");
			return;
		}
		HRESULT hr = S_OK;

		CComPtr<IMMDeviceCollection>pCollection = nullptr;//�豸�ڵ����
		hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
		FAILED_RETURN2(hr, pCollection, L"pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection)")

		UINT  count = 0;
		hr = pCollection->GetCount(&count);//��ȡ�豸����
		FAILED_RETURN2(hr, count, L"�������豸Ϊ0")

			for (ULONG i = 0; i < count; i++) {
				CComPtr<IMMDevice>pEndpoint = nullptr;
				hr = pCollection->Item(i, &pEndpoint);
				FAILED_CONTINUE2(hr, pEndpoint, L"pCollection->Item(i, &pEndpoint)");

				//��ʱ��ĳЩ�����ռʱ���ʼ��ʧ��!!!����Ҫȡ����ռ����������
				CComPtr<IAudioClient>pClient = nullptr;
				hr = pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient);
				FAILED_CONTINUE2(hr, pClient, L"pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient)");

				LPWSTR pwszID = nullptr;//�豸GUIDֵ������Ψһ
				hr = pEndpoint->GetId(&pwszID);
				FAILED_CONTINUE2(hr, pwszID, L"pEndpoint->GetId(&pwszID)");

				CComPtr<IPropertyStore>pProps = nullptr;//���Թ�����
				hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

				PROPVARIANT varName;//�豸��ʾ���֣��п����ظ�
				PropVariantInit(&varName);
				hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
				FAILED_CONTINUE1(hr, L"pProps->GetValue(PKEY_Device_FriendlyName, &varName)");

				m_mapRenderDevice[varName.bstrVal] = pwszID;

				WXString strName = varName.bstrVal;
				/*if (wcsstr(strName.str(), L"Virtual") != nullptr) {
					WXLogW(L"WasapiDevice Failed Error[%ws]", strName.str());

					CoTaskMemFree(pwszID);
					PropVariantClear(&varName);
					break;
				}*/

				//����Ƿ�֧��Ĭ�� MixFormat
				//���豸GetMixFormat����0x80070491  ����
				WAVEFORMATEX* pwfx = nullptr;
				hr = pClient->GetMixFormat(&pwfx);//������Ĭ�ϵ�֧�ָ�ʽ
				if (FAILED(hr)) {
					WXLogW(L"pClient->GetMixFormat(&pwfx)=%08x", hr);
				}
				else {
					CoTaskMemFree(pwfx);
				}

				SoundDeviceInfo info;
				info.isDefalut = FALSE;
				info.isDefalutComm = FALSE;
				if (wcsicmp(m_strDefaultSystemGuid.str(), pwszID) == 0) {
					info.isDefalut = TRUE;
				}

				if (wcsicmp(m_strDefaultCommSystemGuid.str(), pwszID) == 0) {
					info.isDefalutComm = TRUE;
				}
				memset(info.m_strGuid, 0, sizeof(WXCHAR) * MAX_PATH);;
				WXStrcpy(info.m_strGuid, pwszID);

				memset(info.m_strName, 0, sizeof(WXCHAR) * MAX_PATH);;
				WXStrcpy(info.m_strName, varName.bstrVal);
				m_vecSoundRenderInfo.push_back(info);

				CoTaskMemFree(pwszID);
				PropVariantClear(&varName);
			}
	}

	void ListSoundCaptureDevice() {


		m_mapCaptureDevice.clear();

		m_vecSoundCaptureInfo.clear();
		GetDefaultName(FALSE);
		GetDefaultCommName(FALSE);

		IMMDeviceEnumerator* pEnum = CMMNotificationClient::GetEnum();
		if (nullptr == pEnum) {
			WXLogA("CMMNotificationClient::GetEnum() failed");
			return ;
		}
		HRESULT hr = S_OK;
		
		CComPtr<IMMDeviceCollection>pCollection = nullptr;
		hr = pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
		FAILED_RETURN2(hr, pCollection, L"pEnum->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection)")

			UINT  count = 0;
		hr = pCollection->GetCount(&count);
		FAILED_RETURN2(hr, count, L"Mic�豸Ϊ0")

			for (ULONG i = 0; i < count; i++) {
				CComPtr<IMMDevice>pEndpoint = nullptr;
				hr = pCollection->Item(i, &pEndpoint);
				FAILED_CONTINUE2(hr, pEndpoint, L"pCollection->Item(i, &pEndpoint)")

					//��ʱ��ĳЩ�����ռʱ���ʼ��ʧ��!!!����Ҫȡ����ռ����������
					CComPtr<IAudioClient>pClient = nullptr;
				hr = pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient);
				FAILED_CONTINUE2(hr, pClient, L"pEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pClient)");

				LPWSTR pwszID = nullptr;
				hr = pEndpoint->GetId(&pwszID);
				FAILED_CONTINUE2(hr, pwszID, L"pEndpoint->GetId(&pwszID)")

					CComPtr<IPropertyStore>pProps = nullptr;
				hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);

				PROPVARIANT varName;
				PropVariantInit(&varName);
				hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
				FAILED_CONTINUE1(hr, L"pProps->GetValue(PKEY_Device_FriendlyName, &varName)")

					WXString strName = varName.bstrVal;
				/*if (wcsstr(strName.str(), L"Virtual") != nullptr) {
					WXLogW(L"WasapiDevice Failed Error[%ws]", strName.str());

					CoTaskMemFree(pwszID);
					PropVariantClear(&varName);
					break;
				}*/
				m_mapCaptureDevice[varName.bstrVal] = pwszID;

				SoundDeviceInfo info;
				info.isDefalut = FALSE;
				info.isDefalutComm = FALSE;
				if (wcsicmp(m_strDefaultMicGuid.str(), pwszID) == 0) {
					info.isDefalut = TRUE;
				}
				if (wcsicmp(m_strDefaultCommMicGuid.str(), pwszID) == 0) {
					info.isDefalutComm = TRUE;
				}
				memset(info.m_strGuid, 0, sizeof(WXCHAR) * MAX_PATH);;
				WXStrcpy(info.m_strGuid, pwszID);
				memset(info.m_strName, 0, sizeof(WXCHAR) * MAX_PATH);;
				WXStrcpy(info.m_strName, varName.bstrVal);
				m_vecSoundCaptureInfo.push_back(info);
				CoTaskMemFree(pwszID);
				PropVariantClear(&varName);
			}
	}

	//�ر������������豸
	void CloseAllSoundDevice() {
		if (m_pListenSystemDevice) {
			m_pListenSystemDevice->Close();
			delete m_pListenSystemDevice;
			m_pListenSystemDevice = nullptr;
		}
		if (m_pListenMicDevice) {
			m_pListenMicDevice->Close();
			delete m_pListenMicDevice;
			m_pListenMicDevice = nullptr;
		}
	}

	void Reset() {
		ListSoundRenderDevice();
		ListSoundCaptureDevice();
	}

	void Clear() {
		CloseAllSoundDevice();
		m_vecSoundCaptureInfo.clear();
		m_vecSoundRenderInfo.clear();
	}

public:

	WXCTSTR GetGuid(int bSystem, WXCTSTR wszName) {
		GetDefaultName(bSystem);
		if (bSystem && m_mapRenderDevice.count(wszName) != 0) {
			return m_mapRenderDevice[wszName].c_str();
		}
		else if (!bSystem && m_mapCaptureDevice.count(wszName) != 0) {
			return m_mapCaptureDevice[wszName].c_str();
		}
		return  L"default";
	}
	void ReleaseInstance(WasapiDevice* dev) {
		WXAutoLock al(m_mutex);
		if (dev == nullptr)return;
		dev->Close();
		delete dev;
	}
	//��һ�������豸
	//���ĺ���
	WasapiDevice* GetInstance(int bSystem, LPCWSTR guid, int bComm, void* cond, int* bFlag) {
		WXAutoLock al(m_mutex);
		WasapiDevice* dev = new WasapiDevice;
		dev->ThreadSetCond((WXCond*)cond, bFlag);
		dev->SetGuid(guid);
		if (dev->Open(bSystem, bComm) != WX_ERROR_SUCCESS) {
			delete dev;
			dev = nullptr;//����ʵ��GUIDֵ������WASAPI�豸
		}
		return dev;
	}

public:
	static WasapiDeviceManager& GetInst() {
		static WasapiDeviceManager s_inst;
		return s_inst;
	}
};
//�豸����Ĺ����л�����Ϊfalse

void WXWasapiNotifyInit() {
	CMMNotificationClient::GetInst().Register();
}
void WXWasapiNotifyAddDevice(IWasapiDevice* device) {
	CMMNotificationClient::GetInst().AddDevice(device);
}
void WXWasapiNotifyRemoveDevice(IWasapiDevice* device) {
	CMMNotificationClient::GetInst().RemoveDevice(device);
}

WXMEDIA_API void WXWasapiInit() {
	//BEGIN_LOG_FUNC
	WasapiDeviceManager::GetInst().Clear();
	WasapiDeviceManager::GetInst().Reset();
}

WXMEDIA_API void WXWasapiDeinit() {
	//BEGIN_LOG_FUNC
	WasapiDeviceManager::GetInst().Clear();
}
//��ȡ�豸GUID
WXCTSTR  WXWasapiGetGuid(int bSystem, WXCTSTR wszName) {
	return WasapiDeviceManager::GetInst().GetGuid(bSystem, wszName);
}
WXMEDIA_API int  WXWasapiGetRenderCount() {
    return (int)WasapiDeviceManager::GetInst().m_vecSoundRenderInfo.size();
}

WXMEDIA_API SoundDeviceInfo* WXWasapiGetRenderInfo(int index) {
    int nSize = (int)WasapiDeviceManager::GetInst().m_vecSoundRenderInfo.size();
    if (index < 0 || index >= nSize)return nullptr;
    return &(WasapiDeviceManager::GetInst().m_vecSoundRenderInfo[index]);
}

WXMEDIA_API int  WXWasapiGetCaptureCount() {
    return (int)WasapiDeviceManager::GetInst().m_vecSoundCaptureInfo.size();
}

WXMEDIA_API SoundDeviceInfo* WXWasapiGetCaptureInfo(int index) {
	//BEGIN_LOG_FUNC
    int nSize = (int)WasapiDeviceManager::GetInst().m_vecSoundCaptureInfo.size();
    if (index < 0 || index >= nSize)return nullptr;
    return &(WasapiDeviceManager::GetInst().m_vecSoundCaptureInfo[index]);
}

WXMEDIA_API  void   AudioDeviceResetDefault() {
	WasapiDeviceManager::GetInst().GetDefaultName(TRUE);//��ȡĬ���豸����
	WasapiDeviceManager::GetInst().GetDefaultName(FALSE);//��ȡĬ���豸����
}

//��һ�������豸
WasapiDevice *WasapiDeviceGetInstance(int bSystem, LPCWSTR guid, int bComm, void* cond, int* bFlag) {
    return WasapiDeviceManager::GetInst().GetInstance(bSystem, guid, bComm, cond, bFlag);
}

//ɾ��һ���豸������
void  WasapiDeviceReleaseInstance(WasapiDevice *dev) {
	WasapiDeviceManager::GetInst().ReleaseInstance(dev);
}

//Ĭ���豸����
WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultGuid(int bSystem) {
	WasapiDeviceManager::GetInst().GetDefaultName(bSystem);
    return bSystem ? WasapiDeviceManager::GetInst().m_strDefaultSystemGuid.str() : WasapiDeviceManager::GetInst().m_strDefaultMicGuid.str();
}

WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultName(int bSystem) {
	WasapiDeviceManager::GetInst().GetDefaultName(bSystem);
    return bSystem ? WasapiDeviceManager::GetInst().m_strDefaultSystemName.str() : WasapiDeviceManager::GetInst().m_strDefaultMicName.str();
}

//Ĭ��ͨ���豸����
WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultCommGuid(int bSystem) {
	WasapiDeviceManager::GetInst().GetDefaultCommName(bSystem);
	return bSystem ? WasapiDeviceManager::GetInst().m_strDefaultCommSystemGuid.str() : WasapiDeviceManager::GetInst().m_strDefaultCommMicGuid.str();
}

WXMEDIA_API  WXCTSTR  WXWasapiGetDefaultCommName(int bSystem) {
	WasapiDeviceManager::GetInst().GetDefaultCommName(bSystem);
	return bSystem ? WasapiDeviceManager::GetInst().m_strDefaultCommSystemName.str() : WasapiDeviceManager::GetInst().m_strDefaultCommMicName.str();
}

