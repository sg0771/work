/*
基于LavFilter 构建的视频播放器
WPS 视频播放器
*/

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include "FfmpegIncludes.h"
#define DBG_GET_REFCOUNT(spUnk)   (spUnk ? (spUnk.p->AddRef(), spUnk.p->Release() - 1) : 0)

#define USE_FILTER  1  //是否使用字幕功能

#if USE_FILTER
#include <afxwin.h>
#include "VSFilter/VSFilter.h"
#endif


#include "LavFilter/DSUtilLite.h"
#include "LavFilter/WXSplitter.h"
#include "LavFilter/LAVAudio.h"
#include "LavFilter/LAVVideo.h"
#include "LavFilter/moreuuids.h"
#include "SoundTouch.h"

#include <WXBase.h>
#include "WXMediaCpp.h"

#pragma comment(lib,"Wininet.lib")
#pragma comment(lib,"Mfplat.lib")


#define  SUB_HEIGHT  260 
#define  POOL_NUM 2
#define  CUTSIDE_THR 20

//声道切换
extern int g_nSoundMode /*= SOUND_MODE_NONE*/;

//双声道输出数据处理
static void HandleSoundData(short* pDst, short* pSrc, int nSample) {
	if (g_nSoundMode == SOUND_MODE_MAX) {
		for (int i = 0; i < nSample; i++) {
			int nMax = abs(pSrc[i * 2]) > abs(pSrc[i * 2 + 1]) ? pSrc[i * 2] : pSrc[i * 2 + 1];
			pDst[i * 2] = nMax;
			pDst[i * 2 + 1] = nMax;
		}
	}
	else 	if (g_nSoundMode == SOUND_MODE_AVG) {
		for (int i = 0; i < nSample; i++) {
			int nAvg = (pSrc[i * 2] + pSrc[i * 2 + 1]) / 2;
			pDst[i * 2] = nAvg;
			pDst[i * 2 + 1] = nAvg;
		}
	}
	else	if (g_nSoundMode == SOUND_MODE_LEFT) {
		for (int i = 0; i < nSample; i++) {
			pDst[i * 2] = pSrc[i * 2];
			pDst[i * 2 + 1] = pSrc[i * 2];
		}
	}
	else	if (g_nSoundMode == SOUND_MODE_RIGHT) {
		for (int i = 0; i < nSample; i++) {
			pDst[i * 2] = pSrc[i * 2 + 1];
			pDst[i * 2 + 1] = pSrc[i * 2 + 1];
		}
	}
}


EXTERN_C const wchar_t* WXGetSubtitleFontName();
EXTERN_C int WXGetSubtitleFontSize();
EXTERN_C int WXGetSubtitleFontColor();
EXTERN_C int WXGetSubtitleFontPos();
EXTERN_C int WXGetSubtitleFontBold();
EXTERN_C int WXGetSubtitleFontItalic();
EXTERN_C int WXGetSubtitleFontUnderLine();
EXTERN_C int WXGetSubtitleFontStrikeOut();

extern "C" {
	// 用于实时流数据的播放，和WXSoundPlayerWriteData配套使用
	typedef void* (*fPlayerCreate)(int inSampleRate, int inChannel);
	//向音频播放器填充数据
	typedef void  (*fPlayerWriteData)(void* ptr, uint8_t* buf, int buf_size);
	//设置音量，0表示静音，100是最大值
	typedef void  (*fPlayerVolume)(void* ptr, int volume);
	//销毁音频播放器
	typedef void  (*fPlayerDestroy)(void* ptr);
};

WXMEDIA_API int  WXFfmpegCutFile2(void* ptr, WXCTSTR strInput, WXCTSTR strOutput,
	int64_t ptsStart, int64_t ptsDuration, int width, int height);

static IPin* WXGetPin(IBaseFilter* pFilter, const WCHAR* inPartialName, PIN_DIRECTION pinDirection) {
	if (!pFilter)
		return nullptr;
	//zPIN_DIRECTION pinDirection = IsInput ? PINDIR_INPUT : PINDIR_OUTPUT;
	IPin* pFoundPin = nullptr;
	IEnumPins* pinEnum = nullptr;
	if (SUCCEEDED(pFilter->EnumPins(&pinEnum))) {
		pinEnum->Reset();
		IPin* pPin = nullptr;
		while (!pFoundPin && pinEnum->Next(1, &pPin, nullptr) == S_OK) {
			if (pPin) {
				PIN_INFO pinInfo;
				if (SUCCEEDED(pPin->QueryPinInfo(&pinInfo))) {
					pinInfo.pFilter->Release();

					if (pinInfo.dir == pinDirection) {
						if (!inPartialName) {
							pFoundPin = pPin;
							//pFoundPin->AddRef();
						}
						else {
							if (::wcsstr(pinInfo.achName, inPartialName)) {
								pFoundPin = pPin;
								//pFoundPin->AddRef();
							}
						}
					}
				}
				pPin->Release();
			}
		}
		pinEnum->Release();
	}
	return pFoundPin;
}

//将Filter连接到Pin
extern HRESULT WXConnectFilter(IGraphBuilder* pGB, IPin* srcPin, IBaseFilter* pFilter);

static HRESULT WXConnectFilter(IGraphBuilder* pGB, IBaseFilter* pSrcFilter, WXCTSTR strOut, IBaseFilter* pDstFilter) {
	CComPtr<IPin> pPin = WXGetPin(pSrcFilter, strOut, PINDIR_OUTPUT);
	if (pPin == nullptr) {
		return E_FAIL;
	}
	return WXConnectFilter(pGB, pPin, pDstFilter);
}

class AudioBase {
public:
	virtual HRESULT Audio_CheckMediaType(const CMediaType* mt) = 0;//连接时的媒体协商
	virtual HRESULT Audio_DoRenderSample(IMediaSample* pMediaSample) = 0;//音频解码包
	virtual HRESULT Audio_OnStreamStop() = 0;//流停止消息
};

class VideoBase {
public:
	virtual HRESULT Video_CheckMediaType(const CMediaType* mt) = 0;//连接时的媒体协商
	virtual HRESULT Video_DoRenderSample(IMediaSample* pMediaSample) = 0;//音频解码包
	virtual HRESULT Video_OnStreamStop() = 0;//流停止消息
};

//符合DirectShow的视频Render
class __declspec(uuid("AAAAAAAA-0000-1111-2222-AAAAAAAABBBB"))CVideoRender : public CBaseRenderer
{
	VideoBase* m_ctx = nullptr;
public:
	CVideoRender(VideoBase* ctx, __inout_opt LPUNKNOWN pUnk, __inout HRESULT* phr) :
		CBaseRenderer(__uuidof(this), (L"Video Render filter"), pUnk, phr) {
		m_ctx = ctx;
	}

	virtual ~CVideoRender() {}

	virtual HRESULT DoRenderSample(IMediaSample* pMediaSample) {
		return m_ctx->Video_DoRenderSample(pMediaSample);
	}

	virtual HRESULT CheckMediaType(const CMediaType* mt) {
		return m_ctx->Video_CheckMediaType(mt);
	}

	virtual HRESULT OnStreamStop() {
		return m_ctx->Video_OnStreamStop();
	}
};

//符合DirectShow的音频Render
class __declspec(uuid("AAAAAAAA-0000-1111-2222-AAAAAAAACCCC"))CAudioRender : public CBaseRenderer
{
	AudioBase* m_ctx = nullptr;
public:
	virtual ~CAudioRender() {}

	CAudioRender(AudioBase* ctx, __inout_opt LPUNKNOWN pUnk, __inout HRESULT* phr) :
		CBaseRenderer(__uuidof(this), (L"Audio Render filter"), pUnk, phr) {
		m_ctx = ctx;
	}

	virtual HRESULT DoRenderSample(IMediaSample* pMediaSample) {
		return m_ctx->Audio_DoRenderSample(pMediaSample);
	}

	virtual HRESULT CheckMediaType(const CMediaType* mt) {
		return m_ctx->Audio_CheckMediaType(mt);
	}

	virtual HRESULT OnStreamStop() {
		return m_ctx->Audio_OnStreamStop();
	}
};

class WXPlayer : public AudioBase, public VideoBase {
	//純音頻播放器
	class WXPlayerAudio :public AudioBase {
	public:
		HRESULT Audio_DoRenderSample(IMediaSample* pMediaSample) {
			if (pMediaSample) {
				REFERENCE_TIME tsStart = 0;
				REFERENCE_TIME tsStop = 0;
				pMediaSample->GetTime(&tsStart, &tsStop);
				if (tsStart < 0 || tsStop < 0)
					return S_OK;
				uint8_t* pBuf = nullptr;
				pMediaSample->GetPointer(&pBuf);
				int size = pMediaSample->GetActualDataLength();
				if (pBuf && size) {
					this->onAudioFrame(pBuf, size);
				}
			}
			return S_OK;
		}

		HRESULT Audio_CheckMediaType(const CMediaType* mt) {
			if (mt->majortype == MEDIATYPE_Audio && (mt->subtype == MEDIASUBTYPE_PCM ||
				mt->subtype == MEDIASUBTYPE_PCM)) {
				this->GetAudioType(mt);
				return S_OK;
			}
			return E_FAIL;
		}

		HRESULT Audio_OnStreamStop() {
			this->onAudioStreamStop();
			return S_OK;
		}

	private:
		//操作锁
		WXLocker m_mutex;
		fPlayerCreate    m_fPlayerCreate = nullptr;
		fPlayerWriteData m_fPlayerWriteData = nullptr;
		fPlayerVolume    m_fPlayerVolume = nullptr;
		fPlayerDestroy   m_fPlayerDestroy = nullptr;


		CComPtr<IBaseFilter> m_pSourceFilter = nullptr;

		//Lav源分离器
		std::shared_ptr<CLAVSplitter> m_pSP = nullptr;
		//Lav源分离器 DirectShow Com对象
		IBaseFilter* m_pSPFilter = nullptr;

		//Lav 音频解码器
		std::shared_ptr< CLAVAudio> m_pAD = nullptr;
		//Lav视频解码器 DirectShow Com对象
		IBaseFilter* m_pADFilter = nullptr;

		//音频停止标记
		std::atomic_bool m_bStopAudio = true;
		//自定义音频Render
		std::shared_ptr<CAudioRender> m_pAudioRender = nullptr;
		//自定义音频Render， DirectShow Com对象
		IBaseFilter* m_pARFilter = nullptr;

		//Direct Show 滤镜图，底层播放框架
		CComPtr <IFilterGraph2>	m_pGB = nullptr;
		//Direct Show 媒体操作器，主要实现Run Stop Pause操作
		CComPtr <IMediaControl>	m_pMC = nullptr; //Run Stop Pause
		//Direct Show 跳转控制器，主要实现Seek操作
		CComPtr <IMediaSeeking>	m_pSeek = nullptr;//Seek 
		//Direct Show 相关
		DWORD m_dwObjectTableEntry = 0;

		//Aduio
		//音频连接标记
		std::atomic_bool m_bGetAudioType = false;

		//音频变调处理
		std::shared_ptr<soundtouch::SoundTouch> m_soundTouch = nullptr;//变调处理

		//音频缓存
		std::shared_ptr < AudioResampler> m_resampler = nullptr;

		//WXMedia音频播放对象
		std::shared_ptr<void>m_pWXAudioRender = nullptr;

		//播放音量，默认100
		std::atomic<int> m_iVolume = 100;
		int m_nSize = 0;//10ms 数据量
		std::shared_ptr <WXDataBuffer> m_bufAudio = nullptr;
		std::shared_ptr <WXDataBuffer> m_bufTmp = nullptr;

		//输入文件名
		WXString m_strFileName = L"";

		//总时长
		int64_t m_nTimeTotal = 0;

		//Seek
		std::atomic<int64_t>m_iSeek = 0;

		//播放速率.默认100，相当于1.0倍速
		std::atomic<int> m_iSpeed = 100;

		WXString m_strError = L""; //错误提示

		std::atomic_bool m_bASF = false;
		std::atomic_bool m_bOpen = false;

		std::atomic_bool m_bS24 = false;//PCM S24
		std::shared_ptr <WXDataBuffer> m_bufS24 = nullptr;

	private:

		void onAudioStreamStop() {
			m_bStopAudio.store(true);
		}

		void GetAudioType(const CMediaType* amGrabType) {
			if (!m_bGetAudioType.load()) {
				m_bGetAudioType.store(true);
				WAVEFORMAT* wfmt = (WAVEFORMAT*)(amGrabType->pbFormat);
				int inSampleRate = wfmt->nSamplesPerSec;
				int bFloat = FALSE;
				int inChannel = wfmt->nChannels;

				if (wfmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) { //特别处理
					PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(wfmt);
					GUID gg = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
					bFloat = (pEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
					if (!bFloat) {
						if (pEx->Format.wBitsPerSample == 24) {
							m_bS24.store(true);//PCM S24
							m_bufS24 = std::make_shared<WXDataBuffer>();
							m_bufS24->Init(nullptr, inSampleRate * 8);
						}
					}
				}
				else {
					bFloat = (wfmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
				}

				int outSampleRate = inSampleRate;
				if (inSampleRate % 100 != 0) {
					outSampleRate = AUDIO_SAMPLE_RATE;//输入频率不是100的倍数，不能得到整数的10ms数据量
				}

				m_resampler = std::make_shared< AudioResampler>(L"WXPlayer");
				m_resampler->Init(bFloat, inSampleRate, inChannel,
					FALSE, outSampleRate, AUDIO_CHANNELS
				);//short pcm

				m_nSize = outSampleRate * 2 * AUDIO_CHANNELS / 100;//10ms数据量
				m_bufAudio = std::make_shared<WXDataBuffer>();
				m_bufAudio->Init(nullptr, m_nSize);
				m_bufTmp = std::make_shared<WXDataBuffer>();
				m_bufTmp->Init(nullptr, m_nSize);
				//强制双声道输出
				m_soundTouch = std::make_shared<soundtouch::SoundTouch>();
				m_soundTouch->setChannels(AUDIO_CHANNELS);//
				m_soundTouch->setSampleRate(outSampleRate);//
				m_soundTouch->setPitch(1.0f);
				m_pWXAudioRender = std::shared_ptr<void>(m_fPlayerCreate(outSampleRate, AUDIO_CHANNELS),
					[this](void* ptr) {
						this->m_fPlayerDestroy(ptr);
						ptr = nullptr;
					});
			}
		}

		void RenderAudioData(uint8_t* pData, int nDataSize) {
			if (g_nSoundMode == SOUND_MODE_NONE) {
				m_fPlayerWriteData(m_pWXAudioRender.get(), pData, nDataSize);
			}
			else {
				HandleSoundData((short*)m_bufTmp->GetBuffer(), (short*)pData, nDataSize / 4);
				m_fPlayerWriteData(m_pWXAudioRender.get(), m_bufTmp->GetBuffer(), nDataSize);
			}
		}

		//接收音频数据
		void onAudioFrame(uint8_t* _pBuf, int _size) {
			if (!m_bOpen.load()) {
				return;//强制退出
			}
			if (m_pWXAudioRender) {

				uint8_t* pBuf = _pBuf;
				int size = _size;
				if (m_bS24.load()) {
					int targetSize = _size * 2 / 3;
					uint8_t* dst = m_bufS24->GetBuffer();
					for (int i = 0; i < _size / 3; i++) {
						dst[i * 2 + 0] = _pBuf[i * 3 + 1];
						dst[i * 2 + 1] = _pBuf[i * 3 + 2];
					}
					pBuf = dst;
					size = targetSize;
				}

				m_resampler->Write(pBuf, size);//转PCM S16 格式
				while (true) {
					int totalSize = m_resampler->Size();
					if (totalSize >= m_nSize) {
						m_resampler->Read(m_bufAudio->GetBuffer(), m_nSize);
						if (m_iSpeed != 100) { //变调处理
							m_soundTouch->putSamples((short*)m_bufAudio->GetBuffer(), m_nSize / 4);
							while (true) {
								int dstCount = m_soundTouch->receiveSamples((short*)m_bufAudio->GetBuffer(), m_nSize / 4);
								if (dstCount <= 0) {
									break;
								}
								else {
									RenderAudioData(m_bufAudio->GetBuffer(), dstCount * 4);
								}
							}
						}
						else {
							RenderAudioData(m_bufAudio->GetBuffer(), m_nSize);
						}
					}
					else {
						break;
					}
				}
			}
		}

		bool HandleAudioStream(IBaseFilter* pSourceFilter, int bASF) {
			//音频输出节点
			HRESULT hr = S_OK;
			CComPtr<IPin> pSP_Audio = WXGetPin(pSourceFilter, L"Audio", PINDIR_OUTPUT);
			if (pSP_Audio) {
				if (!bASF) {
					//音频解码器
					if (m_pAD.get() == nullptr)
						m_pAD = std::make_shared<CLAVAudio>(nullptr, &hr);
					m_pADFilter = nullptr;
					m_pAD->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pADFilter);
					m_pGB->AddFilter(m_pADFilter, L"Lav Audio Decoder");
					m_pADFilter->JoinFilterGraph(m_pGB, L"Lav Audio Decoder");
				}


				//音频渲染
				if (m_pAudioRender.get() == nullptr)
					m_pAudioRender = std::make_shared<CAudioRender>(this, nullptr, &hr);
				m_pARFilter = nullptr;
				m_pAudioRender->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pARFilter);
				m_pGB->AddFilter(m_pARFilter, L"Audio Renderer");
				m_pARFilter->JoinFilterGraph(m_pGB, L"Audio Renderer");

				if (m_pADFilter) {
					hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pADFilter);
					if (SUCCEEDED(hr)) {
						//解码器连接音频渲染
						hr = WXConnectFilter(m_pGB, m_pADFilter, L"Out", m_pARFilter);
					}
					else { // 某些WMV WAV 不支持 LavAudioDecoder
						WXLogW(L"%ws %ws Not Surpport Lav Audio Decoder", __FUNCTIONW__, m_strFileName.str());
						//分离器连接音频渲染
						m_pGB->RemoveFilter(m_pADFilter);
						hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pARFilter);
					}
				}
				else {
					hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pARFilter);
				}
				if (SUCCEEDED(hr)) {
					m_bStopAudio = FALSE;
				}
				else {
					m_pGB->RemoveFilter(m_pARFilter);
				}
			}
			return !m_bStopAudio.load();
		}

		//打开ASF WMV文件
		//bool OpenNoLav() {
		//	HRESULT hr = m_pGB->AddSourceFilter(m_strFileName.str(), L"Source", &m_pSourceFilter);
		//	if (FAILED(hr)) {
		//		std::wstring wstr = HrToString(hr);
		//		WXLogW(L"m_pGB->QueryInterface(IID_IMediaSeeking) Error %ws", wstr.c_str());
		//		return FALSE;
		//	}
		//	m_pGB->AddFilter(m_pSourceFilter, L"ASF Splitter");

		//	hr = m_pSourceFilter->JoinFilterGraph(nullptr, nullptr);
		//	hr = m_pSourceFilter->JoinFilterGraph(m_pGB, L"Lav Splitter");


		//	HandleAudioStream(m_pSourceFilter, TRUE);

		//	if (m_bStopAudio) {
		//		hr = E_FAIL;
		//		WXLogW(L"[%ws][%ws] Error", __FUNCTIONW__, m_strFileName.str());
		//		return FALSE;
		//	}

		//	if (m_pARFilter) {
		//		CComPtr<IPin> pPin = WXGetPin(m_pARFilter, L"IN", PINDIR_OUTPUT);
		//		if (pPin) {
		//			hr |= m_pGB->Render(pPin);
		//		}
		//	}

		//	if (FAILED(hr)) {
		//		return FALSE;
		//	}
		//	m_bASF.store(true);
		//	return true;
		//}

		//打开MP4等文件
		bool OpenLav() {
			HRESULT hr = S_OK;
			//源分离器
			if (m_pSP.get() == nullptr)
				m_pSP = std::make_shared<CLAVSplitter>(nullptr, &hr);
			hr = m_pSP->Load(m_strFileName.str(), nullptr);
			if (FAILED(hr)) {
				m_strError.Format("%s", m_pSP->GetError());
				m_pSP = nullptr;
				WXLogW(L"Load[%ws] Error", m_strFileName.str());
				return FALSE;
			}
			m_pSP->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pSPFilter);
			m_pGB->AddFilter(m_pSPFilter, L"Lav Splitter");
			hr = m_pSPFilter->JoinFilterGraph(nullptr, nullptr);
			hr = m_pSPFilter->JoinFilterGraph(m_pGB, L"Lav Splitter");

			HandleAudioStream(m_pSPFilter, FALSE);

			if (m_bStopAudio.load()) {
				hr = E_FAIL;
				WXLogW(L"[%ws][%ws] Error", __FUNCTIONW__, m_strFileName.str());
				return FALSE;
			}
			return true;
		}


	public:
		bool IsOpen() {
			return m_bOpen.load();
		}

		WXPlayerAudio() {
			m_fPlayerCreate = WXSoundPlayerCreate;
			m_fPlayerWriteData = WXSoundPlayerWriteData;
			m_fPlayerVolume = WXSoundPlayerVolume;
			m_fPlayerDestroy = WXSoundPlayerDestroy;
		}

		virtual ~WXPlayerAudio() {
			Close();
		}

		//解析文件/URL
		bool OpenFile() {

			m_pGB = nullptr;
			HRESULT hr = ::CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IFilterGraph2, (void**)&m_pGB);
			if (FAILED(hr)) {
				std::wstring wstr = HrToString(hr);
				WXLogW(L"CLSID_FilterGraph m_pGB is nullptr %ws", wstr.c_str());
				return FALSE;
			}
			int vv1 = DBG_GET_REFCOUNT(m_pGB);

			CComPtr<IRunningObjectTable> objectTable = 0;
			if (SUCCEEDED(::GetRunningObjectTable(0, &objectTable))) {
				WCHAR wsz[256];
				wsprintfW(wsz, L"WXGraph.FilterGraph pid %08x", GetCurrentProcessId());
				CComPtr<IMoniker> pMoniker = 0;
				hr |= ::CreateItemMoniker(L"!", wsz, &pMoniker);
				if (SUCCEEDED(hr)) {
					m_dwObjectTableEntry = 0;
					hr = objectTable->Register(0, m_pGB, pMoniker, &m_dwObjectTableEntry);
					pMoniker = nullptr;
				}
				if (FAILED(hr)) {
					return FALSE;
				}
				objectTable = nullptr;
			}
			m_pMC = nullptr;
			hr = m_pGB->QueryInterface(IID_IMediaControl, (void**)&m_pMC);
			if (FAILED(hr) || m_pMC == nullptr) {
				std::wstring wstr = HrToString(hr);
				WXLogW(L"m_pGB->QueryInterface(IID_IMediaControl) Error %ws", wstr.c_str());
				return FALSE;
			}
			m_pSeek = nullptr;
			hr = m_pGB->QueryInterface(IID_IMediaSeeking, (void**)&m_pSeek);
			if (FAILED(hr)) {
				std::wstring wstr = HrToString(hr);
				WXLogW(L"m_pGB->QueryInterface(IID_IMediaSeeking) Error %ws", wstr.c_str());
				return FALSE;
			}

			bool bOpen = false;

			bOpen = OpenLav();//使用LAV解码器,系统播放器

			//WXString strExt = WXBase::GetFileNameSuffix(m_strFileName.str());
			//if (wcsicmp(strExt.str(), L"MP3") == 0
			//	|| wcsicmp(strExt.str(), L"AAC") == 0
			//	|| wcsicmp(strExt.str(), L"WMA") == 0) {
			//	bOpen = OpenNoLav();//使用系统内置解码器
			//}
			//if (!bOpen) {
			//	bOpen = OpenLav();//使用LAV解码器
			//}

			m_bOpen.store(bOpen);
			if (m_bOpen.load()) {
				m_nTimeTotal = GetTotalTime();
			}
			return bOpen;
		}

	public:

		//结束及释放内存
		void Close() {

			WXAutoLock al(m_mutex);

			m_bOpen.store(false);
			if (m_pWXAudioRender.get()) { //消音
				m_fPlayerVolume(m_pWXAudioRender.get(), 0);
			}

			if (m_pMC) { //停止播放
				m_pMC->Stop(); //
			}

			if (m_pGB) {
				m_pSourceFilter = nullptr;
				m_pSPFilter = nullptr;
				m_pADFilter = nullptr;
				m_pARFilter = nullptr;
				CComPtr<IRunningObjectTable> objectTable = 0;
				if (SUCCEEDED(::GetRunningObjectTable(0, &objectTable))) {
					objectTable->Revoke(m_dwObjectTableEntry);
					m_dwObjectTableEntry = 0;
				}
				m_pMC = nullptr; //Run Stop Pause
				m_pSeek = nullptr;//Seek 
				m_pGB = nullptr;
				m_pSP = nullptr; //源分离器
				m_pAD = nullptr; //音频解码器
				m_pAudioRender = nullptr;//音频接收器
				m_soundTouch = nullptr;//音频变调器
				m_pWXAudioRender = nullptr;
			}
			m_resampler = nullptr;
			m_bufAudio = nullptr;
			m_bufS24 = nullptr;
			m_bufTmp = nullptr;

			m_bStopAudio.store(true);
			m_bGetAudioType.store(false);
			m_iVolume.store(100);
			m_nSize = 0;//10ms 数据量
			m_strFileName = L"";
			m_nTimeTotal = 0;
			m_iSeek.store(0);
			m_iSpeed.store(100);
			m_strError = L""; //错误提示
			m_bASF.store(false);
			m_bOpen.store(false);
			m_bS24.store(false);//PCM S24
		}

		bool InitAudio(WXCTSTR wszInput, int speed, int64_t time) {

			WXAutoLock al(m_mutex);

			if (wszInput == nullptr || wcslen(wszInput) == 0 || wcsicmp(m_strFileName.str(), wszInput) == 0) {
				return true;
			}
			if (wszInput && wcslen(wszInput) > 0 && m_strFileName.length() > 0 && wcsicmp(m_strFileName.str(), wszInput) != 0) {
				Close();//与旧文件名不一样
				m_strFileName = L"";
				m_iSpeed.store(100);
				m_iSeek.store(0);
			}
			m_strFileName = wszInput;
			m_iSpeed.store(speed);
			m_iSeek.store(time);
			int bRet = OpenFile();
			if (!bRet) {
				Close();
				m_strFileName = L"";
				m_iSpeed.store(100);
				m_iSeek.store(0);
				m_bOpen.store(false);
				return false;
			}
			m_bOpen.store(true);
			return true;
		}

		//运行Graph
		bool Run() {
			WXAutoLock al(m_mutex);
			if (!m_bOpen.load()) {
				WXString strErr;
				strErr.Format(L"%ws 打开文件失败 Run Error", m_strFileName.str());
				WXLogW(strErr.str());
				MessageBox(NULL, strErr.str(), L"提示", MB_OK);
				return false;
			}

			if (m_pWXAudioRender.get()) {
				m_fPlayerVolume(m_pWXAudioRender.get(), m_iVolume.load());
			}
			if (m_pGB && m_pMC) {
				m_pMC->Run();
				return true;
			}
			return false;
		}

		int64_t GetCurrTime() {
			WXAutoLock al(m_mutex);
			if (m_pSeek) {
				int64_t position = 0;
				HRESULT hr = m_pSeek->GetCurrentPosition(&position);
				if (SUCCEEDED(hr)) {
					return (position) / 10000;
				}
			}
			return 0;
		}

		int64_t GetTotalTime() {
			WXAutoLock al(m_mutex);
			int nTotalTime = 0;
			if (m_pSeek && nTotalTime == 0) {
				int64_t length = 0;
				HRESULT hr = m_pSeek->GetDuration(&length);
				if (SUCCEEDED(hr)) {
					nTotalTime = (int64_t)(length / 10000);
					return nTotalTime;
				}
				else {
					std::wstring wstr = HrToString(hr);
					WXLogW(L"%ws Error %ws", __FUNCTIONW__, wstr.c_str());
				}
			}
			return nTotalTime;
		}

		void Seek(int64_t seek) { //Seek

			WXAutoLock al(m_mutex);
			int64_t new_seek = seek;
			if (new_seek < 0) {
				new_seek = 0;
			}
			else if (m_nTimeTotal > 0 && new_seek > m_nTimeTotal) {
				new_seek = m_nTimeTotal;
			}
			m_iSeek.store(new_seek);
			int64_t position = (int64_t)(10000 * (m_iSeek.load()));
			m_pSeek->SetPositions(&position,
				AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);

		}

		void SetSpeed(int speed) {

			WXAutoLock al(m_mutex);
			if (m_bASF.load())  //WMV ASF 不支持 Speed操作
				return;
			int new_speed = speed;
			if (new_speed > 200) {
				new_speed = 200;
			}
			else if (new_speed < 50) {
				new_speed = 50;
			}
			m_iSpeed.store(new_speed);
			if (m_soundTouch) {
				m_soundTouch->setRate((float)m_iSpeed.load() / 100.0f);
				m_soundTouch->setPitch(100.0f / (float)m_iSpeed.load());
			}
			if (m_pSeek) {
				m_pSeek->SetRate(m_iSpeed.load() / 100.0);
			}
		}

		bool SetVolume(int volume) {
			WXAutoLock al(m_mutex);
			m_iVolume.store(av_clip(volume, 0, 100));
			if (m_pWXAudioRender.get()) {
				m_fPlayerVolume(m_pWXAudioRender.get(), m_iVolume.load());
			}
			return false;
		}

		bool Start() { //主线程
			WXAutoLock al(m_mutex);
			if (!m_bOpen.load()) {
				WXString strErr;
				strErr.Format(L"%ws 打开文件失败 Satrt Error", m_strFileName.str());
				WXLogW(strErr.str());
				MessageBox(NULL, strErr.str(), L"提示", MB_OK);
				return false;
			}

			if (m_pARFilter) {
				m_bStopAudio.store(false);
			}

			HRESULT hr = S_OK;
			if (m_pSeek && m_iSeek.load() != 0) {
				int64_t position = (int64_t)(10000 * (m_iSeek.load()));
				HRESULT hr = m_pSeek->SetPositions(&position, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);
			}

			SetSpeed(m_iSpeed);
			hr = m_pMC->Run();

			if (FAILED(hr)) {
				std::wstring wstr = HrToString(hr);
				WXLogW(L"%ws Error %ws", __FUNCTIONW__, wstr.c_str());
				return SUCCEEDED(hr);
			}

			int64_t position = (int64_t)(10000 * (m_iSeek.load()));
			m_pSeek->SetPositions(&position,
				AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);

	

			return SUCCEEDED(hr);
		}

		bool Stop() {
			WXAutoLock al(m_mutex);
			if (!m_bOpen.load()) {
				WXLogW(L"File Open Error");
			}
			if (m_pWXAudioRender.get()) {
				m_fPlayerVolume(m_pWXAudioRender.get(), 0);
			}
			if (m_pMC) {
				m_pMC->Stop();
			}
			return true;
		}

		bool Pause() {
			WXAutoLock al(m_mutex);
			if (!m_bOpen.load()) {
				WXLogW(L"File Open Error, Pause()");
				return false;
			}
			if (m_pWXAudioRender.get()) {
				m_fPlayerVolume(m_pWXAudioRender.get(), 0);
			}
			if (m_pGB && m_pMC) {
				m_pMC->Pause();
			}
			return true;
		}
	};
public:
	HRESULT Video_DoRenderSample(IMediaSample* pMediaSample) {
		if (pMediaSample) {
			REFERENCE_TIME tsStart = 0;
			REFERENCE_TIME tsStop = 0;
			pMediaSample->GetTime(&tsStart, &tsStop);
			if (tsStart < 0 || tsStop < 0)
				return S_OK;

			uint8_t* pBuf = nullptr;
			pMediaSample->GetPointer(&pBuf);
			int size = pMediaSample->GetActualDataLength();
			if (pBuf && size)
				this->onVideoFrame(pBuf, size);
		}
		return S_OK;
	}

	HRESULT Video_CheckMediaType(const CMediaType* mt) {

		if (m_bRGB_Render) {
			if (mt->majortype == MEDIATYPE_Video && (
				mt->subtype == MEDIASUBTYPE_RGB32 ||
				mt->subtype == MEDIASUBTYPE_RGB24 ||
				mt->subtype == MEDIASUBTYPE_ARGB32
				)) {
				this->GetVideoType(mt);
				return S_OK;
			}
		}
		else {
			if (mt->majortype == MEDIATYPE_Video && (
				mt->subtype == MEDIASUBTYPE_YV12 ||
				mt->subtype == MEDIASUBTYPE_I420 ||
				mt->subtype == MEDIASUBTYPE_NV12 ||
				mt->subtype == MEDIASUBTYPE_YUY2)) {
				this->GetVideoType(mt);
				return S_OK;
			}
		}

		return E_FAIL;
	}

	HRESULT Video_OnStreamStop() {
		this->onVideoStreamStop();
		return S_OK;
	}
public:
	HRESULT Audio_DoRenderSample(IMediaSample* pMediaSample) {
		if (pMediaSample) {
			REFERENCE_TIME tsStart = 0;
			REFERENCE_TIME tsStop = 0;
			pMediaSample->GetTime(&tsStart, &tsStop);
			if (tsStart < 0 || tsStop < 0)
				return S_OK;
			uint8_t* pBuf = nullptr;
			pMediaSample->GetPointer(&pBuf);
			int size = pMediaSample->GetActualDataLength();
			if (pBuf && size) {
				this->onAudioFrame(pBuf, size);
			}
		}
		return S_OK;
	}

	HRESULT Audio_CheckMediaType(const CMediaType* mt) {
		if (mt->majortype == MEDIATYPE_Audio && (mt->subtype == MEDIASUBTYPE_PCM ||
			mt->subtype == MEDIASUBTYPE_PCM)) {
			this->GetAudioType(mt);
			return S_OK;
		}
		return E_FAIL;
	}

	HRESULT Audio_OnStreamStop() {
		this->onAudioStreamStop();
		return S_OK;
	}

private:
	class WXPlayerFrame {
	public:
		WXVideoFrame m_pVideoFrame; //解码图像
		WXVideoFrame m_pDarVideoFrame;//DAR图像
		WXVideoFrame m_pRotateVideoFrame;//DAR图像

		void Init(int width, int height, int dst_width, int rotate) {
			m_pVideoFrame.Init(AV_PIX_FMT_YUV420P, width, height);

			if (dst_width != width)
				m_pDarVideoFrame.Init(AV_PIX_FMT_YUV420P, dst_width, height);

			if (rotate == 0 || rotate == 180) {
				m_pRotateVideoFrame.Init(AV_PIX_FMT_YUV420P, dst_width, height);
			}
			else {
				m_pRotateVideoFrame.Init(AV_PIX_FMT_YUV420P, height, dst_width);
			}
		}
	};
	fPlayerCreate    m_fPlayerCreate = nullptr;
	fPlayerWriteData m_fPlayerWriteData = nullptr;
	fPlayerVolume    m_fPlayerVolume = nullptr;
	fPlayerDestroy   m_fPlayerDestroy = nullptr;

	//操作锁
	WXLocker m_mutex;

	//是否隐藏字幕
	bool m_bHideSubtitle = false;
	//字幕文件
	WXString m_strSubtitle = L"";
	//字幕延时
	int64_t m_nSubtitleDelay = 0;
	//字幕字体名字
	WXString m_strFontName = L"";
	//字幕字体大小
	int m_nFontSize = 0;
	//字幕文字颜色
	int m_dwFontColor = 0;//默认白色

	//显示高度
	int m_nFontPos = 0;

	int m_bBold = 0;  //粗体
	int m_bItalic = 0;//斜体
	int m_bUnderLine = 0;//下划线
	int m_bStrikeOut = 0;//删除线
private:

	CComPtr<IBaseFilter> m_pSourceFilter = nullptr;

	//Lav源分离器
	std::shared_ptr<CLAVSplitter> m_pSP = nullptr;
	//Lav源分离器 DirectShow Com对象
	IBaseFilter* m_pSPFilter = nullptr;

	//Lav 视频解码器
	std::shared_ptr< CLAVVideo> m_pVD = nullptr;
	//Lav视频解码器 DirectShow Com对象
	IBaseFilter* m_pVDFilter = nullptr;

	//Lav 音频解码器
	std::shared_ptr< CLAVAudio> m_pAD = nullptr;
	//Lav视频解码器 DirectShow Com对象
	IBaseFilter* m_pADFilter = nullptr;

#if USE_FILTER
	//VSFilter字幕操作对象
	std::shared_ptr<CDirectVobSubFilter> m_pDVS = nullptr;
	//VSFilter字幕滤镜， DirectShow Com对象
	IBaseFilter* m_pDVSFilter = nullptr;
#endif

	//视频停止标记
	std::atomic_bool m_bStopVideo = true;
	//自定义视频Render
	std::shared_ptr<CVideoRender> m_pVideoRender = nullptr;
	//自定义视频Render， DirectShow Com对象
	IBaseFilter* m_pVRFilter = nullptr;

	//音频停止标记
	std::atomic_bool m_bStopAudio = true;
	//自定义音频Render
	std::shared_ptr<CAudioRender> m_pAudioRender = nullptr;
	//自定义音频Render， DirectShow Com对象
	IBaseFilter* m_pARFilter = nullptr;

	//Direct Show 滤镜图，底层播放框架
	CComPtr <IFilterGraph2>	m_pGB = nullptr;
	//Direct Show 媒体操作器，主要实现Run Stop Pause操作
	CComPtr <IMediaControl>	m_pMC = nullptr; //Run Stop Pause
	//Direct Show 跳转控制器，主要实现Seek操作
	CComPtr <IMediaSeeking>	m_pSeek = nullptr;//Seek 
	//Direct Show 相关
	DWORD m_dwObjectTableEntry = 0;

	//Video
	//视频连接标记
	std::atomic_bool m_bGetVideoType = false;

	//分辨率回调函数
	ffplayOnSize m_cbSize = nullptr;

	int m_iRotate = 0;

	//分辨率
	int m_iSrcWidth = 0;
	int m_iSrcHeight = 0;

	//分辨率
	int m_iWidth = 0;
	int m_iHeight = 0;

	//画面刷新标记
	std::atomic_bool m_bVideoRefresh = false;

	//自定义预览窗口
	HWND  m_hwndDisplay = nullptr;
	//WXMedia视频显示
	std::shared_ptr<void> m_pWXVideoRender = nullptr;

	//视频回调函数，回调AVFrame数据
	void* m_pSink = nullptr; //回调对象
	OnVideoData m_cbVideoFrame = nullptr;//回调函数
	int m_nID = 0;
	onAVFrame m_cbVideoFrame2 = nullptr;

	std::shared_ptr<WXDataBuffer> m_bufRGBA;
	std::shared_ptr<WXDataBuffer> m_bufTarget;//最大回调内存
	std::shared_ptr<WXVideoFrame> m_frameCut;//CutSize

	//视频回调函数，回调YUV420数据，全局函数，没有回调对象
	WXFfmpegOnVideoData m_cbVideoData = nullptr;
	WXFfmpegOnVideoData2 m_cbVideoData2 = nullptr;

	//DXFilter
	void* m_pD3DRender = nullptr;
	uint64_t m_uid = 0;
	WXFfmpegOnVideoData3 m_cbSurface = nullptr;
	//WXFfmpegOnAudioData m_cbAudioData = nullptr;

	//显示比例和分辨率比例不一致，需要缩放处理
	int m_bHasDAR = FALSE;
	int m_dar_num = 1;
	int m_dar_den = 1;
	int m_iDstWidth = 0;

	//视频数据缓存
	AVFrame* m_pCurrFrame = nullptr;//当前视频帧

	std::shared_ptr<WXPlayerFrame>m_arrVideoFrameA = nullptr;
	std::shared_ptr<WXPlayerFrame>m_arrVideoFrameB = nullptr;

	ThreadSafeQueue<WXPlayerFrame*> m_queueData;
	ThreadSafeQueue<WXPlayerFrame*> m_queuePool;

	//传入的视频数据类型
	int m_iType = WX_DT_VIDEO_INVALD;

	//Aduio
		//音频连接标记
	std::atomic_bool m_bGetAudioType = false;

	//音频变调处理
	std::shared_ptr<soundtouch::SoundTouch> m_soundTouch = nullptr;//变调处理

	//音频缓存
	std::shared_ptr < AudioResampler> m_resampler = nullptr;

	//WXMedia音频播放对象
	std::shared_ptr<void>m_pWXAudioRender = nullptr;

	//播放音量，默认100
	std::atomic<int> m_iVolume = 100;
	int m_nSize = 0;//10ms 数据量
	std::shared_ptr <WXDataBuffer> m_bufAudio;
	std::shared_ptr <WXDataBuffer> m_bufTmp;

	//Player
	std::atomic_bool m_bThreadStop = true;
	std::shared_ptr<std::thread>m_threadWork = nullptr;//工作线程
	std::shared_ptr<std::thread>m_threadVideo = nullptr;//视频线程

	//输入文件名
	WXString m_strFileName = _T("");

	//附加音频对象
	std::shared_ptr<WXPlayerAudio>m_audioPlay = nullptr;
	//附加音频延迟时间
	std::atomic<int64_t> m_nAudioDelay = 0;

	//总时长
	int64_t m_nTimeTotal = 0;

	class QueueTime {
		WXLocker m_lock;
		std::queue<int64_t>m_queue;
	public:
		void Push(int64_t time) { //填充一个新的Seek地址
			WXAutoLock al(m_lock);
			m_queue.push(time);
		}
		int64_t Pop() { //获取Seek地址，-1 表示不可用
			WXAutoLock al(m_lock);
			if (m_queue.size() == 0) {
				return -1;//不可用
			}
			int64_t time = 0;
			while (!m_queue.empty())
			{
				time = m_queue.front();
				m_queue.pop();
			}
			return time;
		}
		void Clear() {//清空数据
			WXAutoLock al(m_lock);
			int64_t time = 0;
			while (!m_queue.empty())
			{
				time = m_queue.front();
				m_queue.pop();
			}
		}
	};
	//Seek
	QueueTime m_newSeek;
	std::atomic<int64_t>  m_iSeek = 0;

	//播放速率.默认100，相当于1.0倍速
	std::atomic<int> m_newSpeed = 100;
	std::atomic<int> m_iSpeed = 100;

	//截图设置
	WXString  m_strJPG = _T("");

	//对象的事件名字
	WXString m_strIDEvent = _T("null");
	//事件回调对象
	void* m_ownerEvent = nullptr;
	//事件回调函数
	WXFfmpegOnEvent m_cbEvent = nullptr;

	std::atomic_bool m_bHasVideo = false;//是否带有视频
	std::atomic_bool m_bHasAudio = false;//是否带有音频

	//计算上下黑边
	int m_nRenderWidth = 0;
	int m_nRenderHeight = 0;
	int m_sideTop = 0;
	int m_sideBottom = 0;
	int m_sideLeft = 0;
	int m_sideRight = 0;

	WXString m_strError; //错误提示

	std::atomic_bool m_bLoading = false;
	//true表示 OpenFile() 已经结束退出
	std::atomic_bool m_bExcute = false;
	std::atomic_bool m_bASF = false;
	std::atomic_bool m_bOpen = false;

	//功能1，在Start前设置一个独立音轨,音轨文件名，延迟时间
	//动态修改当前音轨文件或延迟时间（需要先关闭原来的音轨）
	//还有跳转处理
	std::atomic_bool m_bAttachAudio = false;//是否有附加音轨操作

	std::atomic_bool m_bWillStop = false;

	std::atomic_bool m_bS24 = false;//PCM S24
	std::shared_ptr <WXDataBuffer> m_bufS24;

	//设置去黑边
	std::atomic_bool m_bCutSide = false;// 是否去黑边
	std::atomic_bool m_bWillCutSide = false;//执行一次去黑边处理
	std::atomic_bool m_bMuted = false;

	//渲染帧率计算
	int64_t m_iRecv = 0;
	int64_t m_iRender = 0;

	std::atomic_bool m_bGetPic = false;
	WXString m_strImage = L"";

	std::atomic<int> m_iStatus = FFPLAY_STATE_UNAVAILABLE;//默认不可用

	std::atomic_bool m_bCutVideo = false;
	WXString m_strCutVideoName = L"";
	int64_t m_tsCutVideoStart = 0;
	int64_t m_tsCutVideoStop = 0;
	int m_dstCutVideoWidth = 0;
	int m_dstCutVideoHeight = 0;
private:
	void onVideoStreamStop() {
		m_bStopVideo.store(true);
		if (m_bStopAudio.load() && m_bStopVideo.load()) {
			m_bWillStop.store(true);
		}
	}

	void onAudioStreamStop() {
		m_bStopAudio.store(true);
		if (m_bStopAudio.load() && m_bStopVideo.load()) {
			m_bWillStop.store(true);
		}
	}

	void GetVideoType(const CMediaType* amGrabType) {
		if (!m_bGetVideoType.load()) {
			m_bGetVideoType.store(true);
			BITMAPINFOHEADER* bih = nullptr;
			if (amGrabType->formattype == FORMAT_VideoInfo) {
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)amGrabType->pbFormat;
				bih = &vih->bmiHeader;
				m_iSrcWidth = bih->biWidth;
				m_iSrcHeight = bih->biHeight;
				m_iWidth = (m_iSrcWidth + 1) / 2 * 2;
				m_iHeight = (m_iSrcHeight + 1) / 2 * 2;
				m_iDstWidth = m_iWidth;

				m_dar_num = m_iSrcWidth;
				m_dar_den = m_iSrcHeight;
			}
			else if (amGrabType->formattype == FORMAT_VideoInfo2) {
				VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)amGrabType->pbFormat;
				bih = &vih2->bmiHeader;
				m_iSrcWidth = bih->biWidth;
				m_iSrcHeight = bih->biHeight;
				m_iWidth = (m_iSrcWidth + 1) / 2 * 2;
				m_iHeight = (m_iSrcHeight + 1) / 2 * 2;
				m_iDstWidth = m_iWidth;

				m_dar_num = vih2->dwPictAspectRatioX;
				m_dar_den = vih2->dwPictAspectRatioY;
			}

			if (bih) {
				if (amGrabType->subtype == MEDIASUBTYPE_YV12)
					m_iType = WX_DT_VIDEO_YV12;
				else if (amGrabType->subtype == MEDIASUBTYPE_NV12)
					m_iType = WX_DT_VIDEO_NV12;
				else if (amGrabType->subtype == MEDIASUBTYPE_I420)
					m_iType = WX_DT_VIDEO_I420;
				else if (amGrabType->subtype == MEDIASUBTYPE_YUY2)
					m_iType = WX_DT_VIDEO_YUY2;
				else if (amGrabType->subtype == MEDIASUBTYPE_RGB32 ||
					amGrabType->subtype == MEDIASUBTYPE_ARGB32)
					m_iType = WX_DT_VIDEO_RGBA;
				else if (amGrabType->subtype == MEDIASUBTYPE_RGB24)
					m_iType = WX_DT_VIDEO_RGB24;
			}
		}
	}

	void GetAudioType(const CMediaType* amGrabType) {
		if (!m_bGetAudioType.load()) {
			m_bGetAudioType.store(true);
			WAVEFORMAT* wfmt = (WAVEFORMAT*)(amGrabType->pbFormat);
			int inSampleRate = wfmt->nSamplesPerSec;
			int bFloat = FALSE;
			int inChannel = wfmt->nChannels;

			if (wfmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) { //特别处理
				PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(wfmt);
				GUID gg = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
				bFloat = (pEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
				if (!bFloat) {
					if (pEx->Format.wBitsPerSample == 24) {
						m_bS24.store(true);//PCM S24
						m_bufS24 = std::make_shared<WXDataBuffer>();
						m_bufS24->Init(nullptr, inSampleRate * 8);
					}
				}
			}
			else {
				bFloat = (wfmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
			}

			int outSampleRate = inSampleRate;
			if (inSampleRate % 100 != 0) {
				outSampleRate = AUDIO_SAMPLE_RATE;//输入频率不是100的倍数，不能得到整数的10ms数据量
			}

			m_resampler = std::make_shared< AudioResampler>(L"WXPlayer");
			m_resampler->Init(bFloat, inSampleRate, inChannel,
				FALSE, outSampleRate, AUDIO_CHANNELS
			);//short pcm

			m_nSize = outSampleRate * 2 * AUDIO_CHANNELS / 100;//10ms数据量
			m_bufAudio = std::make_shared<WXDataBuffer>();
			m_bufAudio->Init(nullptr, m_nSize);
			m_bufTmp = std::make_shared<WXDataBuffer>();
			m_bufTmp->Init(nullptr, m_nSize);
			//强制双声道输出
			m_soundTouch = std::make_shared<soundtouch::SoundTouch>();
			m_soundTouch->setChannels(AUDIO_CHANNELS);//
			m_soundTouch->setSampleRate(outSampleRate);//
			m_soundTouch->setPitch(1.0f);
			m_pWXAudioRender = std::shared_ptr<void>(m_fPlayerCreate(outSampleRate, AUDIO_CHANNELS),
				[this](void* ptr) {
					this->m_fPlayerDestroy(ptr);
					ptr = nullptr;
				});
		}
	}

	void RenderAudioData(uint8_t* pData, int nDataSize) {
		if (g_nSoundMode == SOUND_MODE_NONE) {
			m_fPlayerWriteData(m_pWXAudioRender.get(), pData, nDataSize);
		}
		else {
			HandleSoundData((short*)m_bufTmp->GetBuffer(), (short*)pData, nDataSize / 4);
			m_fPlayerWriteData(m_pWXAudioRender.get(), m_bufTmp->GetBuffer(), nDataSize);
		}
	}

	//接收音频数据
	void onAudioFrame(uint8_t* _pBuf, int _size) {
		if (!m_bOpen.load()) {
			return;//强制退出
		}
		if (m_pWXAudioRender) {

			uint8_t* pBuf = _pBuf;
			int size = _size;
			if (m_bS24.load()) {
				int targetSize = _size * 2 / 3;
				uint8_t* dst = m_bufS24->GetBuffer();
				for (int i = 0; i < _size / 3; i++) {
					dst[i * 2 + 0] = _pBuf[i * 3 + 1];
					dst[i * 2 + 1] = _pBuf[i * 3 + 2];
				}
				pBuf = dst;
				size = targetSize;
			}

			m_resampler->Write(pBuf, size);//转PCM S16 格式
			while (true) {
				int totalSize = m_resampler->Size();
				if (totalSize >= m_nSize) {
					m_resampler->Read(m_bufAudio->GetBuffer(), m_nSize);
					if (m_iSpeed != 100) { //变调处理
						m_soundTouch->putSamples((short*)m_bufAudio->GetBuffer(), m_nSize / 4);
						while (true) {
							int dstCount = m_soundTouch->receiveSamples((short*)m_bufAudio->GetBuffer(), m_nSize / 4);
							if (dstCount <= 0) {
								break;
							}
							else {
								RenderAudioData(m_bufAudio->GetBuffer(), dstCount * 4);
							}
						}
					}
					else {
						RenderAudioData(m_bufAudio->GetBuffer(), m_nSize);
					}
				}
				else {
					break;
				}
			}
		}
	}

	void ThreadVideoProc() { //视频帧处理

		while (!m_bThreadStop.load()) {

			WXPlayerFrame* pFrame = m_queueData.Pop();
			if (pFrame == nullptr) {
				SLEEPMS(1);
				continue;
			}

			if (m_iRotate == 0) {
				//Render
				m_pCurrFrame = m_bHasDAR ? pFrame->m_pDarVideoFrame.GetFrame() : pFrame->m_pVideoFrame.GetFrame();
			}
			else {
				AVFrame* srcFrame = m_bHasDAR ? pFrame->m_pDarVideoFrame.GetFrame() : pFrame->m_pVideoFrame.GetFrame();
				m_pCurrFrame = pFrame->m_pRotateVideoFrame.GetFrame();
				libyuv::I420Rotate(
					srcFrame->data[0], srcFrame->linesize[0],
					srcFrame->data[1], srcFrame->linesize[1],
					srcFrame->data[2], srcFrame->linesize[2],
					m_pCurrFrame->data[0], m_pCurrFrame->linesize[0],
					m_pCurrFrame->data[1], m_pCurrFrame->linesize[1],
					m_pCurrFrame->data[2], m_pCurrFrame->linesize[2],
					srcFrame->width, srcFrame->height,
					(libyuv::RotationMode)(m_iRotate)
				);
			}


			if (m_bWillCutSide.load()) {
				CutSize(m_pCurrFrame); //执行一次求黑边计算
				m_bWillCutSide.store(false);
			}

			if (m_bCutSide.load() && (
				m_sideLeft != 0 || m_sideRight != 0 ||
				m_sideTop != 0 || m_sideBottom != 0
				)) {//按照设置好的黑边切割

				int newW = m_pCurrFrame->width - m_sideLeft - m_sideRight;
				int newH = m_pCurrFrame->height - m_sideTop - m_sideBottom;
				if (newW == 0 || newH == 0)
					return;
				if (m_frameCut->GetFrame()->width != newW ||
					m_frameCut->GetFrame()->height != newH) {
					m_frameCut->Init(AV_PIX_FMT_YUV420P, newW, newH);
				}

				libyuv::I420Copy(
					m_pCurrFrame->data[0] + m_sideTop * m_pCurrFrame->linesize[0] + m_sideLeft,
					m_pCurrFrame->linesize[0],
					m_pCurrFrame->data[1] + ((m_sideTop / 2) * m_pCurrFrame->linesize[1]) + m_sideLeft / 2,
					m_pCurrFrame->linesize[1],
					m_pCurrFrame->data[2] + ((m_sideTop / 2) * m_pCurrFrame->linesize[2]) + m_sideLeft / 2,
					m_pCurrFrame->linesize[2],
					m_frameCut->GetFrame()->data[0], m_frameCut->GetFrame()->linesize[0],
					m_frameCut->GetFrame()->data[1], m_frameCut->GetFrame()->linesize[1],
					m_frameCut->GetFrame()->data[2], m_frameCut->GetFrame()->linesize[2],
					m_frameCut->GetFrame()->width, m_frameCut->GetFrame()->height
				);
				DoVideoFrame(m_frameCut->GetFrame());
			}
			else {
				DoVideoFrame(m_pCurrFrame);
			}
			m_queuePool.Push(pFrame);
		}

	}

	void onVideoFrame(BYTE* buf, long size) {

		if (!m_bOpen.load()) {
			return;//强制退出
		}

		m_iRecv++;
		WXPlayerFrame* pFrame = m_queuePool.Pop();
		if (pFrame == nullptr)
			return;
		m_iRender++;

		int nSrcPitch = size / m_iHeight;
		int m_iImageSize = m_iWidth * m_iHeight;
		AVFrame* pVideoFrame = pFrame->m_pVideoFrame.GetFrame();
		switch (m_iType)
		{
		case WX_DT_VIDEO_YV12:
			libyuv::I420Copy(
				buf, (m_iSrcWidth + 1) / 2 * 2,
				buf + m_iImageSize * 5 / 4, ((m_iSrcWidth / 2) + 1) / 2 * 2,
				buf + m_iImageSize, ((m_iSrcWidth / 2) + 1) / 2 * 2,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		case WX_DT_VIDEO_I420:
			libyuv::I420Copy(
				buf, (m_iSrcWidth + 1) / 2 * 2,
				buf + m_iImageSize * 5 / 4, ((m_iSrcWidth / 2) + 1) / 2 * 2,
				buf + m_iImageSize, ((m_iSrcWidth / 2) + 1) / 2 * 2,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		case WX_DT_VIDEO_NV12:  //JPEG Format
			libyuv::NV12ToI420(
				buf, (m_iSrcWidth + 1) / 2 * 2,
				buf + m_iImageSize, (m_iSrcWidth + 1) / 2 * 2,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		case WX_DT_VIDEO_YUY2:
			libyuv::YUY2ToI420(
				buf, nSrcPitch,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		case WX_DT_VIDEO_RGBA:
			libyuv::ARGBToI420(
				buf + (m_iHeight - 1) * nSrcPitch, -nSrcPitch,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		case WX_DT_VIDEO_RGB24:
			libyuv::RGB24ToI420(
				buf + (m_iHeight-1)* nSrcPitch, -nSrcPitch,
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				m_iWidth, m_iHeight
			);
			break;
		default:
			break;
		}

		if (m_bHasDAR) {
			AVFrame* pVideoFrame2 = pFrame->m_pDarVideoFrame.GetFrame();
			libyuv::I420Scale(
				pVideoFrame->data[0], pVideoFrame->linesize[0],
				pVideoFrame->data[1], pVideoFrame->linesize[1],
				pVideoFrame->data[2], pVideoFrame->linesize[2],
				pVideoFrame->width, pVideoFrame->height,
				pVideoFrame2->data[0], pVideoFrame2->linesize[0],
				pVideoFrame2->data[1], pVideoFrame2->linesize[1],
				pVideoFrame2->data[2], pVideoFrame2->linesize[2],
				pVideoFrame2->width, pVideoFrame2->height,
				libyuv::FilterMode::kFilterBilinear
			);
		}

		m_queueData.Push(pFrame);
		m_bVideoRefresh.store(false);
	};

	int  GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
	{
		UINT num = 0; // number of image encoders
		UINT size = 0; // size of the image encoder array in bytes
		Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
		Gdiplus::GetImageEncodersSize(&num, &size);
		if (size == 0)
			return -1; // Failure
		pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
		if (pImageCodecInfo == NULL)
			return -1; // Failure
		Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
		for (UINT j = 0; j < num; ++j)
		{
			if (wcsicmp(pImageCodecInfo[j].MimeType, format) == 0)
			{
				*pClsid = pImageCodecInfo[j].Clsid;
				free(pImageCodecInfo);
				return j; // Success
			}
		}
		free(pImageCodecInfo);
		return -1; // Failure
	}

	void SaveAsPicture(WXCTSTR wszName) {

		if (m_pCurrFrame == nullptr) {
			return;
		}
		WXString strExt = WXBase::GetFileNameSuffix(wszName);
		CLSID encoderClsid;
		HRESULT hr = E_FAIL;
		if (wcsicmp(strExt.str(), L"jpg") == 0 || wcsicmp(strExt.str(), L"jpeg") == 0) {
			hr = GetEncoderClsid(L"image/jpeg", &encoderClsid);
		}
		else if (wcsicmp(strExt.str(), L"png") == 0) {
			hr = GetEncoderClsid(L"image/png", &encoderClsid);
		}
		else {
			hr = GetEncoderClsid(L"image/bmp", &encoderClsid);
		}


		if (SUCCEEDED(hr)) {
			//To RGB 32
			libyuv::I420ToRGB24(
				m_pCurrFrame->data[0], m_pCurrFrame->linesize[0],
				m_pCurrFrame->data[1], m_pCurrFrame->linesize[1],
				m_pCurrFrame->data[2], m_pCurrFrame->linesize[2],
				m_bufRGBA->GetBuffer(), m_iDstWidth * 3,
				m_iDstWidth, m_iHeight
			);

			Gdiplus::Bitmap bmp(m_iDstWidth, m_iHeight, m_iDstWidth * 3,
				PixelFormat24bppRGB, m_bufRGBA->GetBuffer());
			Gdiplus::Status status = bmp.Save(wszName, &encoderClsid);
			if (status == Gdiplus::Ok) {
				if (m_cbEvent) {
					m_cbEvent(m_ownerEvent, m_strIDEvent.str(), FFPLAY_ERROR_OK_GET_PICTURE, wszName);
				}
			}
		}
	}

	//最后使用RGB 输出
	bool m_bRGB_Render = false;

	HRESULT ConnectToVideoRenderFilter(IBaseFilter* srcFilter, WXCTSTR strName) {
		m_bRGB_Render = false;
		HRESULT hr = WXConnectFilter(m_pGB, srcFilter, strName, m_pVRFilter);//连接到视频Render
		if (FAILED(hr)) {
			WXLogW(L"LavFilter Unsupport YUV Output, Use RGB");
			m_bRGB_Render = true;
			hr = WXConnectFilter(m_pGB, srcFilter, strName, m_pVRFilter);//连接到视频Render
		}
		return hr;
	}

	bool HandleVideoStream(IBaseFilter* pSourceFilter, int bASF) {
		//视频输出节点
		HRESULT hr = S_OK;

		if (!bASF) {
			//视频解码器
			if (m_pVD.get() == nullptr)
				m_pVD = std::make_shared<CLAVVideo>(nullptr, &hr);
			//使用硬解码
			//int bHw = WXGetGlobalValue(L"HwVideoDecode");
			//if (bHw)
			//{  //强制硬解碼
			//	m_pVD->SetHWAccelCodec(HWCodec_H264, TRUE);
			//	m_pVD->SetHWAccelCodec(HWCodec_HEVC, TRUE);
			//	m_pVD->SetHWAccel(HWAccel_DXVA2);
			//}
			m_pVDFilter = nullptr;
			m_pVD->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pVDFilter);
			m_pGB->AddFilter(m_pVDFilter, L"Lav Video Decoder");
			hr = m_pVDFilter->JoinFilterGraph(m_pGB, L"Lav Video Decoder");
		}

#if USE_FILTER
		//字幕操作对象
		if (m_pDVS.get() == nullptr)
			m_pDVS = std::make_shared<CDirectVobSubFilter>(nullptr, &hr);
		m_pDVSFilter = nullptr;
		hr = m_pDVS->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pDVSFilter);
		hr |= m_pGB->AddFilter(m_pDVSFilter, L"DirectVobSub");
		hr |= m_pDVSFilter->JoinFilterGraph(m_pGB, L"DirectVobSub");
#endif

		//视频渲染对象
		if (m_pVideoRender.get() == nullptr)
			m_pVideoRender = std::make_shared<CVideoRender>(this, nullptr, &hr);
		m_pVRFilter = nullptr;
		m_pVideoRender->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pVRFilter);
		m_pGB->AddFilter(m_pVRFilter, L"Video Renderer");
		m_pVRFilter->JoinFilterGraph(m_pGB, L"Video Renderer");

		//默认YUV输出
		{
			m_bRGB_Render = false;
			if (m_pVDFilter) {
				hr = WXConnectFilter(m_pGB, pSourceFilter, L"Video", m_pVDFilter);
				if (SUCCEEDED(hr)) { //支持视频解码器
					if (m_pDVSFilter) {
						hr = WXConnectFilter(m_pGB, m_pVDFilter, L"Out", m_pDVSFilter);//解码器连接字幕
						if (SUCCEEDED(hr)) {
							hr = ConnectToVideoRenderFilter(m_pDVSFilter, L"Out");//字幕连接渲染
						}
						else {
							WXLogW(L"%ws %ws Not Support SubVob", __FUNCTIONW__, m_strFileName.c_str());
							m_pGB->RemoveFilter(m_pDVSFilter);
							hr = ConnectToVideoRenderFilter(m_pVDFilter, L"Out");//解码器连接渲染
						}
					}
					else
					{
						hr = ConnectToVideoRenderFilter(m_pVDFilter, L"Out");//解码器连接渲染
					}
				}
				else { //不支持视频解码器
					m_pGB->RemoveFilter(m_pVDFilter);
					WXLogW(L"%ws %ws Not Support Lav Video Decoder", __FUNCTIONW__, m_strFileName.str());
					hr = WXConnectFilter(m_pGB, pSourceFilter, L"Video", m_pDVSFilter); //分离器连接字幕
					if (SUCCEEDED(hr)) {
						hr = ConnectToVideoRenderFilter(m_pDVSFilter, L"Out");//字幕连接渲染
					}
					else
					{
						m_pGB->RemoveFilter(m_pDVSFilter);
						WXLogW(L"%ws %ws Not Support SubVob", __FUNCTIONW__, m_strFileName.str());
						hr = ConnectToVideoRenderFilter(pSourceFilter, L"Video");//分离器连接渲染
					}
				}
			}
			else {
#if USE_FILTER
				hr = WXConnectFilter(m_pGB, pSourceFilter, L"Video", m_pDVSFilter);
				if (SUCCEEDED(hr)) {
					hr = ConnectToVideoRenderFilter(m_pDVSFilter, L"Out");//字幕连接渲染
				}
				else
#endif
				{
#if USE_FILTER
					m_pGB->RemoveFilter(m_pDVSFilter);
#endif
					WXLogW(L"%ws %ws Not Support SubVob", __FUNCTIONW__, m_strFileName.str());
					hr = ConnectToVideoRenderFilter(pSourceFilter, L"Video");//分离器连接渲染
				}
			}
		}


		if (SUCCEEDED(hr)) {
			m_bStopVideo.store(false);
		}
		else {
			m_pGB->RemoveFilter(m_pVRFilter);
		}
		return !m_bStopVideo.load();
	}

	bool HandleAudioStream(IBaseFilter* pSourceFilter, int bASF) {
		//音频输出节点
		HRESULT hr = S_OK;
		CComPtr<IPin> pSP_Audio = WXGetPin(pSourceFilter, L"Audio", PINDIR_OUTPUT);
		if (pSP_Audio) {
			if (!bASF) {
				//音频解码器
				if (m_pAD.get() == nullptr)
					m_pAD = std::make_shared<CLAVAudio>(nullptr, &hr);
				m_pADFilter = nullptr;
				m_pAD->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pADFilter);
				m_pGB->AddFilter(m_pADFilter, L"Lav Audio Decoder");
				m_pADFilter->JoinFilterGraph(m_pGB, L"Lav Audio Decoder");
			}


			//音频渲染
			if (m_pAudioRender.get() == nullptr)
				m_pAudioRender = std::make_shared<CAudioRender>(this, nullptr, &hr);
			m_pARFilter = nullptr;
			m_pAudioRender->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pARFilter);
			m_pGB->AddFilter(m_pARFilter, L"Audio Renderer");
			m_pARFilter->JoinFilterGraph(m_pGB, L"Audio Renderer");

			if (m_pADFilter) {
				hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pADFilter);
				if (SUCCEEDED(hr)) {
					//解码器连接音频渲染
					hr = WXConnectFilter(m_pGB, m_pADFilter, L"Out", m_pARFilter);
				}
				else { // 某些WMV WAV 不支持 LavAudioDecoder
					WXLogW(L"%ws %ws Not Surpport Lav Audio Decoder", __FUNCTIONW__, m_strFileName.str());
					//分离器连接音频渲染
					m_pGB->RemoveFilter(m_pADFilter);
					hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pARFilter);
				}
			}
			else {
				hr = WXConnectFilter(m_pGB, pSourceFilter, L"Audio", m_pARFilter);
			}
			if (SUCCEEDED(hr)) {
				m_bStopAudio = FALSE;
			}
			else {
				m_pGB->RemoveFilter(m_pARFilter);
			}
		}
		return !m_bStopAudio.load();
	}



	void HandleAudioDelay() {
		//有新的 Start Run Seek 或者 SetDelay操作

		int64_t now_Delay = m_nAudioDelay.load();
		int64_t new_Delay = now_Delay;//如果obj为NULL, 就使用原来的Delay
		BOOL m_bHas = FALSE;
		MyStruct* obj = m_queueDelay.Pop();
		if (obj) {
			m_bHas = TRUE;
			if (wcsicmp(obj->strName.str(), L"Close") == 0) {
				m_audioPlay->Close();//关闭附加音轨
			}
			else {
				if (wcslen(obj->strName.str()) > 0) {
					int bRet = m_audioPlay->InitAudio(obj->strName.str(), m_iSpeed.load(), m_iSeek.load());//重置AudioPlayer
					if (bRet) {
						m_audioPlay->SetVolume(m_iVolume.load());//同步音量
						m_audioPlay->Start();
						m_audioPlay->Pause();
					}
				}
			}
			new_Delay = obj->delay;
			m_nAudioDelay.store(new_Delay);
			delete obj;
		}

		bool bAttachAudio = m_bAttachAudio.load();
		if (m_audioPlay->IsOpen() && (bAttachAudio || m_bHas || new_Delay != now_Delay)) { //音轨操作队列
			int64_t currTime = this->GetCurrTime();
			int64_t currTimeA = m_audioPlay->GetCurrTime();
			int64_t delay = abs(currTime - currTimeA - new_Delay);

			if (new_Delay > currTime + 100) { //设置的偏移值比主轨道当前事件大，需要先暂停播放附加音频
				m_audioPlay->Pause();
			}
			else { //检测是否需要将附加音轨恢复播放或者Seek处理
				int64_t targetTime = currTime - new_Delay;
				if (delay > 500 && targetTime >= 0) {//允许操作附加音轨
					m_audioPlay->Seek(targetTime);//音轨跳转到指定位置
					if (IsRunning()) {
						m_audioPlay->Run();//同步播放
						WXLogW(L"Handle Audio Delay %lld  Curr=%lld targetTime=%lld Run!!!!!", new_Delay, currTime, targetTime);
					}
					else {
						WXLogW(L"Handle Audio Delay %lld  Curr=%lld targetTime=%lld NotRun!!!!", new_Delay, currTime, targetTime);
					}
					m_bAttachAudio.store(false);//清空队列
				}
			}
		}

	}

	void HandleSeek() {
		//检查上层是否传入新的Seek操作指令
	    //不为-1表示上层传入一个新的Seek位置
		int64_t new_seek = m_newSeek.Pop();
		if (new_seek != -1) {
			int64_t tsCurr = this->GetCurrTime();
			int64_t delay = abs(new_seek - tsCurr);
			if (delay <= 500) { //当前位置与Seek位置小于 500ms，不Seek
				WXLogW(L"Not Seek At %lld，Curr=%lld,  Delay=%lld", new_seek, tsCurr, delay);
				return;
			}
			WXLogW(L"Now Seek At %lld", new_seek);
			m_iSeek.store(new_seek);
			if (m_audioPlay->IsOpen()) {
				int64_t targetAudioTime = new_seek - m_nAudioDelay.load();
				if (targetAudioTime <= 0) {
					m_audioPlay->Pause();
					m_bAttachAudio.store(true);//清空队列
				}
				else {
					m_audioPlay->Seek(targetAudioTime);
				}
			}
			if (m_pSeek) {
				int64_t position = (int64_t)(10000 * (m_iSeek.load()));
				HRESULT hr = m_pSeek->SetPositions(&position, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);
				if (!IsRunning()) { //暂停和停止状态要等一下
					Refresh(); //画面刷新
				}
			}
		}
	}

	void HandleSpeed() {
		//SPEED 检测传入的Speed值与当前Speed 值是否一致
		int new_speed = m_newSpeed.load();
		if (!m_bASF.load() && new_speed != m_iSpeed.load()) {
			m_iSpeed.store(new_speed);
			if (m_soundTouch) {
				m_soundTouch->setRate((float)m_iSpeed.load() / 100.0f);
				m_soundTouch->setPitch(100.0f / (float)m_iSpeed.load());
			}
			if (m_pSeek) {
				m_pSeek->SetRate(m_iSpeed.load() / 100.0);
			}
			if (m_audioPlay->IsOpen()) {
				m_audioPlay->SetSpeed(m_iSpeed.load());
			}
		}
	}

	void HandleStop() {
		//音视频流结束处理
		if (m_bWillStop.load()) { //音视频流结束处理
			if (m_bStopAudio.load() && m_bStopVideo.load()) {
				this->Stop();
				m_bStopVideo.store(false);
				m_bStopAudio.store(false);
			}
			m_bWillStop.store(false);
		}
	}

	void HandleGetPicture() {
		//截图操作
		if (m_bGetPic.load()) {
			SaveAsPicture(m_strImage.str());
			m_bGetPic.store(false);
		}
	}

	void HandleCutVideo() {

		//截取视频操作
		if (m_bCutVideo.load()) {
			void* obj = WXFfmpegParamCreate();
			int ret = 0;
			if (m_dstCutVideoWidth == 0 && m_dstCutVideoHeight == 0) {
				//原分辨率处理，可能有DAR处理
				ret = WXFfmpegCutFile2(obj, m_strFileName.str(), m_strCutVideoName.str(),
					m_tsCutVideoStart, m_tsCutVideoStop - m_tsCutVideoStart, m_bHasDAR ? m_iDstWidth : m_iWidth, m_iHeight);
			}
			else {
				ret = WXFfmpegCutFile2(obj, m_strFileName.str(), m_strCutVideoName.str(),
					m_tsCutVideoStart, m_tsCutVideoStop - m_tsCutVideoStart, m_dstCutVideoWidth, m_dstCutVideoHeight);
			}

			WXFfmpegParamDestroy(obj);
			if (ret >= 0) {
				//转换成功
				if (m_cbEvent) {
					m_cbEvent(m_ownerEvent, m_strIDEvent.str(), FFPLAY_ERROR_OK_GET_VIDEO, m_strCutVideoName.str());
				}
			}
			m_bCutVideo.store(false);//恢复可用
		}
	}

	void ThreadWorkProc() {
		while (!m_bThreadStop.load()) {
			HandleAudioDelay();
			HandleSeek();
			HandleSpeed();
			HandleStop();
			HandleGetPicture();
			HandleCutVideo();
			SLEEPMS(1);
		}
	}

	void CutSize(AVFrame* frame) {
		int sideTop = 0;
		int sideBottom = 0;
		int sideLeft = 0;
		int sideRight = 0;
		if (frame->format == AV_PIX_FMT_YUV420P || frame->format == AV_PIX_FMT_YUVJ420P) {
			int width = frame->width;
			int height = frame->height;
			int pitch = frame->linesize[0];
			uint8_t* buf = frame->data[0];
			for (sideTop = 0; sideTop < height / 2 - 1; sideTop++) {
				int64_t sumTop = 0;
				for (int w = 0; w < width; w++) {
					sumTop += buf[sideTop * pitch + w];
				}
				if (sumTop > CUTSIDE_THR * width)
					break;
			}
			for (sideBottom = 0; sideBottom < height / 2 - 1; sideBottom++) {
				int64_t sumBottom = 0;
				for (int w = 0; w < width; w++) {
					sumBottom += buf[(height - 1 - sideBottom) * pitch + w];
				}
				if (sumBottom > CUTSIDE_THR * width)
					break;
			}
			for (sideLeft = 0; sideLeft < width / 2 - 1; sideLeft++) {
				int64_t sumLeft = 0;
				for (int h = 0; h < height; h++) {
					sumLeft += buf[h * pitch + sideLeft];
				}
				if (sumLeft > CUTSIDE_THR * height)
					break;
			}
			for (sideRight = 0; sideRight < width / 2 - 1; sideRight++) {
				int64_t sumRight = 0;
				for (int h = 0; h < height; h++) {
					sumRight += buf[h * pitch + (width - 1 - sideRight)];
				}
				if (sumRight > CUTSIDE_THR * height)
					break;
			}
		}
		m_sideTop = (sideTop + 1) / 2 * 2;//不能为奇数
		m_sideBottom = (sideBottom + 1) / 2 * 2;//不能为奇数
		m_sideLeft = (sideLeft + 1) / 2 * 2;//不能为奇数
		m_sideRight = (sideRight + 1) / 2 * 2;//不能为奇数

		int newW = frame->width - m_sideLeft - m_sideRight;
		int newH = frame->height - m_sideTop - m_sideBottom;
		if (newW == 0 || newH == 0) { //当前图像全黑
			m_sideTop = 0;
			m_sideBottom = 0;
			m_sideLeft = 0;
			m_sideRight = 0;
		}
	}

	void DoVideoFrame(AVFrame* currFrame) {
		if (currFrame == nullptr)
			return;

		if (m_bOpen.load() && m_hwndDisplay) {
			if (m_nRenderWidth != currFrame->width ||
				m_nRenderHeight != currFrame->height) {
				m_pWXVideoRender = nullptr;
			}

			if (m_pWXVideoRender.get() == nullptr) {
				m_nRenderWidth = currFrame->width;
				m_nRenderHeight = currFrame->height;
				m_pWXVideoRender = std::shared_ptr<void>(WXVideoRenderCreate(m_hwndDisplay, m_nRenderWidth, m_nRenderHeight),
					[this](void* ptr) {
						WXVideoRenderDestroy(ptr);
						ptr = nullptr;
					});
				WXVideoRenderSetID(m_pWXVideoRender.get(), this);//关联当前播放器
			}
		}

		if (m_bOpen.load() && m_pWXVideoRender.get()) { //预览
			WXVideoRenderDisplayEx(m_pWXVideoRender.get(), currFrame, TRUE, RENDER_ROTATE_NONE, this);
		}

		if (m_bOpen.load() && m_cbVideoFrame) { //回调AVFrame
			m_cbVideoFrame(m_pSink, currFrame);
		}
		if (m_bOpen.load() && m_cbVideoFrame2) { //回调AVFrame
			m_cbVideoFrame2(m_pSink,m_nID, currFrame);
		}

		if (m_bOpen.load() && m_cbSurface && m_pD3DRender) { //回调Surface 给WPF

			int ret = WXDXFilterDraw(m_pD3DRender, m_uid, currFrame);
			if (ret) {
				if (m_bOpen.load())
					m_cbSurface(m_pD3DRender, currFrame->width, currFrame->height);
			}
		}

		if (m_bOpen.load() && (m_cbVideoData || m_cbVideoData2)) {//回调I420数据
			int size = currFrame->width * currFrame->height;
			libyuv::I420Copy(currFrame->data[0], currFrame->linesize[0],
				currFrame->data[1], currFrame->linesize[1],
				currFrame->data[2], currFrame->linesize[2],
				m_bufTarget->GetBuffer(), currFrame->width,
				m_bufTarget->GetBuffer() + size, currFrame->width / 2,
				m_bufTarget->GetBuffer() + size * 5 / 4, currFrame->width / 2,
				currFrame->width, currFrame->height
			);
			if (m_cbVideoData) {
				m_cbVideoData(m_bufTarget->GetBuffer(),
					currFrame->width, currFrame->height);
			}
			if (m_cbVideoData2) {
				int64_t ts = GetCurrTime() / 1000;
				m_cbVideoData2(m_bufTarget->GetBuffer(),
					currFrame->width, currFrame->height, ts);
			}
		}

	}
	//打开ASF WMV文件
	bool OpenNoLav() {
		HRESULT hr = m_pGB->AddSourceFilter(m_strFileName.str(), L"Source", &m_pSourceFilter);
		if (FAILED(hr) || m_pSourceFilter == nullptr) {
			std::wstring wstr = HrToString(hr);
			WXLogW(L"[%ws] m_pGB->AddSourceFilter Error %ws", wstr.c_str());
			return false;
		}
		hr |= m_pGB->AddFilter(m_pSourceFilter, L"ASF Splitter");

		hr |= m_pSourceFilter->JoinFilterGraph(nullptr, nullptr);
		hr |= m_pSourceFilter->JoinFilterGraph(m_pGB, L"ASF Splitter");
		
		if (m_bHasVideo.load()) {
			HandleVideoStream(m_pSourceFilter, TRUE);
		}
		if (m_bHasAudio.load()) {
			HandleAudioStream(m_pSourceFilter, TRUE);
		}

		if (m_bStopAudio && m_bStopVideo) {
			hr = E_FAIL;
			WXLogW(L"[%ws][%ws] Error", __FUNCTIONW__, m_strFileName.str());
			return false;
		}

		if (m_pVRFilter) {
			CComPtr<IPin> pPin = WXGetPin(m_pVRFilter, L"IN", PINDIR_OUTPUT);
			if (pPin) {
				hr |= m_pGB->Render(pPin);//视频链路
			}
		}

		if (m_pARFilter) {
			CComPtr<IPin> pPin = WXGetPin(m_pARFilter, L"IN", PINDIR_OUTPUT);
			if (pPin) {
				hr |= m_pGB->Render(pPin);//音频链路
			}
		}

		if (FAILED(hr)) {
			return false;
		}
		m_bASF.store(true);
		return true;
	}

	//打开MP4等文件
	bool OpenLav() {
		HRESULT hr = S_OK;
		//源分离器
		if (m_pSP.get() == nullptr)
			m_pSP = std::make_shared<CLAVSplitter>(nullptr, &hr);

		m_bLoading.store(true);
		hr = m_pSP->Load(m_strFileName.str(), nullptr);
		//加载文件或者URL，如果是URL，可能会只能会执行比较久，需要外部调用m_pSP->Stop();强行结束
		m_bLoading.store(false);

		if (FAILED(hr)) {
			m_strError.Format("%s", m_pSP->GetError());
			m_pSP = nullptr;
			WXLogW(L"OpenLav Load[%ws] Error", m_strFileName.str());
			return false;
		}

		m_pSP->NonDelegatingQueryInterface(IID_IBaseFilter, (void**)&m_pSPFilter);
		m_pGB->AddFilter(m_pSPFilter, L"Lav Splitter");
		hr = m_pSPFilter->JoinFilterGraph(nullptr, nullptr);
		hr = m_pSPFilter->JoinFilterGraph(m_pGB, L"Lav Splitter");

		if (m_bHasVideo.load()) {
			HandleVideoStream(m_pSPFilter, FALSE);
		}
		if (m_bHasAudio.load()) {
			HandleAudioStream(m_pSPFilter, FALSE);
		}

		if (m_bStopAudio.load() && m_bStopVideo.load()) {
			hr = E_FAIL;
			WXLogW(L"[%ws][%ws] Error", __FUNCTIONW__, m_strFileName.str());
			return FALSE;
		}
		return true;
	}

	//设置字幕
	void SetSubtitleFontImpl() {
#if USE_FILTER
		if (m_pDVS) {//运行中修改
			WXSetGlobalString(L"FontName", m_strFontName.str());
			WXSetGlobalValue(L"FontSize", m_nFontSize);
			WXSetGlobalValue(L"FontColor", m_dwFontColor);
			WXSetGlobalValue(L"FontPos", m_nFontPos);
			WXSetGlobalValue(L"FontBold", m_bBold);
			WXSetGlobalValue(L"FontItalic", m_bItalic);
			WXSetGlobalValue(L"FontUnderLine", m_bUnderLine);
			WXSetGlobalValue(L"FontStrikeOut", m_bStrikeOut);

			m_pDVS->put_HideSubtitles(!!m_bHideSubtitle);

			int a = 0;
			int b = 0;
			int c = 0;
			HRESULT hr = m_pDVS->get_SubtitleTiming(&a, &b, &c);
			hr = m_pDVS->put_SubtitleTiming(m_nSubtitleDelay, b, c);//设置延迟，单位毫秒;
			STSStyle st;
			m_pDVS->get_TextSettings(&st);
			st.fontNameW = m_strFontName.str();
			st.fontSize = m_nFontSize;
			st.colors[0] = m_dwFontColor;
			if (m_nFontPos <= 50) {
				int targetBottom = SUB_HEIGHT * m_nFontPos / 100;
				if (targetBottom < 2)
					targetBottom = 2;
				st.marginRect.bottom = targetBottom;
				st.scrAlignment = 2;
			}
			else {
				int targetTop = 288 * m_nFontPos / 100;
				if (targetTop < 2)
					targetTop = 2;
				st.marginRect.top = 288 - targetTop;
				st.scrAlignment = 8;
			}
			m_pDVS->put_TextSettings(&st);

			//暂停状态下修改字幕属性时刷新画面
			if (m_pGB && m_pMC && m_pDVS && !IsRunning()) {
				m_pDVS->Redraw();
				IMediaSample* pSample = m_pDVS->Redraw();
				m_pVideoRender->DoRenderSample(pSample);
			}
		}
#endif
	}

public:
	void ShotPicture(WXCTSTR wszName, int quality) {
		if (!m_bGetPic.load()) {
			m_strImage = wszName;
			m_bGetPic.store(true);
		}
	}

	struct MyStruct
	{
		WXString strName;
		int64_t  delay;
	};
	class QueueAudioDelay {
		WXLocker m_lock;
		std::queue<MyStruct*>m_queue;
	public:
		void Push(WXCTSTR wszName, int64_t time) { //填充一个新的Seek地址
			WXAutoLock al(m_lock);
			MyStruct* obj = new MyStruct;
			obj->strName = wszName ? wszName : L"";
			obj->delay = time;
			m_queue.push(obj);
		}
		MyStruct* Pop() { //获取Seek地址，-1 表示不可用
			WXAutoLock al(m_lock);
			if (m_queue.size() == 0) {
				return nullptr;//不可用
			}
			MyStruct* obj = nullptr;
			while (!m_queue.empty())
			{
				obj = m_queue.front();
				m_queue.pop();
			}
			return obj;
		}
		void Clear() {//清空数据
			WXAutoLock al(m_lock);
			int64_t time = 0;
			MyStruct* obj = nullptr;
			while (!m_queue.empty())
			{
				obj = m_queue.front();
				m_queue.pop();
				delete obj;
			}
		}
	};
	QueueAudioDelay m_queueDelay;
	WXString m_strAudio = L"";

	int AttachAudio(WXCTSTR wszAudio, int64_t delay) {
		WXAutoLock al(m_mutex);
		if (wszAudio && wcsicmp(L"Main", wszAudio) == 0) {
			if (m_pAD) {
				if (delay)
					m_pAD->SetAudioDelay(TRUE, delay);
				else
					m_pAD->SetAudioDelay(FALSE, delay);
				int64_t ts = this->GetCurrTime();
				Seek(ts - 500);
			}
			return false;
		}

		if (wszAudio && wcslen(wszAudio) > 0) {
			m_strAudio = wszAudio;//更新m_strAudio
		}
		m_queueDelay.Push(m_strAudio.str(), delay);
		m_bAttachAudio.store(true);
		return true;
	}

	WXPlayer() {
		static std::atomic_bool s_bInitLav = false;
		if (!s_bInitLav.load()) {
			s_bInitLav.store(true);
			WXSetGlobalString(L"FontName", L"Arial");
			WXSetGlobalValue(L"FontSize", 14);
			WXSetGlobalValue(L"FontColor", 0x00FFFFFF);
			WXSetGlobalValue(L"FontPos", 0);
			WXSetGlobalValue(L"FontBold", TRUE);
			WXSetGlobalValue(L"FontItalic", FALSE);
			WXSetGlobalValue(L"FontUnderLine", FALSE);
			WXSetGlobalValue(L"FontStrikeOut", FALSE);
		}

		m_fPlayerCreate = WXSoundPlayerCreate;
		m_fPlayerWriteData = WXSoundPlayerWriteData;
		m_fPlayerVolume = WXSoundPlayerVolume;
		m_fPlayerDestroy = WXSoundPlayerDestroy;

		/*	
		int AudioMode = WXGetGlobalValue(L"AudioMode");
		if (AudioMode == 1) { //WASAPI
			m_fPlayerCreate = WXWasapiPlayerCreate;
			m_fPlayerWriteData = WXWasapiPlayerWriteData;
			m_fPlayerVolume = WXWasapiPlayerVolume;
			m_fPlayerDestroy = WXWasapiPlayerDestroy;
		}
		*/


		//字幕字体名字
		m_strFontName = WXGetSubtitleFontName();
		//字幕字体大小
		m_nFontSize = WXGetSubtitleFontSize();
		//字幕文字颜色
		m_dwFontColor = WXGetSubtitleFontColor();//默认白色

		//显示高度
		m_nFontPos = WXGetSubtitleFontPos();

		m_bBold = WXGetSubtitleFontBold();  //粗体
		m_bItalic = WXGetSubtitleFontItalic();//斜体
		m_bUnderLine = WXGetSubtitleFontUnderLine();//下划线
		m_bStrikeOut = WXGetSubtitleFontStrikeOut();//删除线

		m_audioPlay = std::make_shared<WXPlayerAudio>();
	}

	virtual ~WXPlayer() {
		Close();
		m_audioPlay = nullptr;//退出
	}

	//结束及释放内存
	void Close() {
		//WXLogW(L"%ws Close() ----- %08x", m_strFileName.str(), (void*)this);

		m_bOpen.store(false);
		if (m_bLoading.load()) { //文件加载中强行中断
			m_pSP->Stop();
			WXLogW(L"%ws is Loading, Break", m_strFileName.str());
		}

		WXAutoLock al(m_mutex);

		if (m_pWXAudioRender.get()) { //消音
			m_fPlayerVolume(m_pWXAudioRender.get(), 0);
		}

		while (!m_bExcute.load()) { //等到OpenFile执行完毕
			SLEEPMS(10);
		}

		WXLogW(L"%ws Close() Rate=%0.2f",
			m_strFileName.str(), (float)m_iRender / (float)m_iRecv);

		if (m_audioPlay->IsOpen()) { //附加音频流
			m_audioPlay->Close(); //关闭附加音轨
		}

		if (m_pMC && !IsStopped()) { //停止播放
			m_pMC->Stop(); //
		}

		m_bThreadStop.store(true);
		if (m_threadWork) { //工作线程
			m_threadWork->join();
			m_threadWork = nullptr;
		}
		if (m_threadVideo) { //工作线程
			m_threadVideo->join();
			m_threadVideo = nullptr;
		}
		if (m_pGB) {

			if (m_pSourceFilter) {
				m_pSourceFilter = nullptr;
			}

			if (m_pSPFilter) {
				m_pGB->RemoveFilter(m_pSPFilter);
				m_pSPFilter = nullptr;
			}

			if (m_pVDFilter) {
				m_pGB->RemoveFilter(m_pVDFilter);
				m_pVDFilter = nullptr;
			}

			if (m_pADFilter) {
				m_pGB->RemoveFilter(m_pADFilter);
				m_pADFilter = nullptr;
			}

			if (m_pVRFilter) {
				m_pGB->RemoveFilter(m_pVRFilter);
				m_pVRFilter = nullptr;
			}

			if (m_pARFilter) {
				m_pGB->RemoveFilter(m_pARFilter);
				m_pARFilter = nullptr;
			}
#if USE_FILTER
			if (m_pDVSFilter) {
				m_pGB->RemoveFilter(m_pDVSFilter);
				m_pDVSFilter = nullptr;
			}
#endif

			CComPtr<IRunningObjectTable> objectTable = 0;
			if (SUCCEEDED(::GetRunningObjectTable(0, &objectTable))) {
				objectTable->Revoke(m_dwObjectTableEntry);
				m_dwObjectTableEntry = 0;
			}
			m_pGB = nullptr;
		}

		m_pSP = nullptr; //源分离器
		m_pVD = nullptr; //视频解码器
		m_pAD = nullptr; //音频解码器
#if USE_FILTER
		m_pDVS = nullptr;//字幕处理器
#endif
		m_pVideoRender = nullptr;//视频接收器
		m_pAudioRender = nullptr;//音频接收器
		m_soundTouch = nullptr;//音频变调器

		m_pWXVideoRender = nullptr; //自定义视频显示
		m_pWXAudioRender = nullptr;//自定义音频显示

		m_resampler = nullptr;

		m_bufAudio = nullptr;
		m_bufS24 = nullptr;
		m_bufTarget = nullptr;
		m_bufTmp = nullptr;
		m_bufRGBA = nullptr;
		m_frameCut = nullptr;

		m_bGetAudioType.store(false);
		m_bGetVideoType.store(false);
		//WXLogW(L"%ws Close() CCCC", m_strFileName.str());

		while (m_queueData.Size()) {
			m_queueData.Pop();
		}
		while (m_queuePool.Size()) {
			m_queuePool.Pop();
		}
		m_arrVideoFrameA = nullptr;
		m_arrVideoFrameB = nullptr;
		m_pSourceFilter = nullptr;

		//Lav源分离器
		m_pSP = nullptr;
		//Lav源分离器 DirectShow Com对象
		m_pSPFilter = nullptr;

		//Lav 视频解码器
		m_pVD = nullptr;
		//Lav视频解码器 DirectShow Com对象
		m_pVDFilter = nullptr;

		//Lav 音频解码器
		m_pAD = nullptr;
		//Lav视频解码器 DirectShow Com对象
		m_pADFilter = nullptr;

#if USE_VSFILTER
		//VSFilter字幕操作对象
		m_pDVS = nullptr;
		//VSFilter字幕滤镜， DirectShow Com对象
		m_pDVSFilter = nullptr;
#endif

		//视频停止标记
		m_bStopVideo.store(true);
		//自定义视频Render
		m_pVideoRender = nullptr;
		//自定义视频Render， DirectShow Com对象
		m_pVRFilter = nullptr;

		//音频停止标记
		m_bStopAudio.store(true);
		//自定义音频Render
		m_pAudioRender = nullptr;
		//自定义音频Render， DirectShow Com对象
		m_pARFilter = nullptr;

		//Direct Show 滤镜图，底层播放框架
		m_pGB = nullptr;
		//Direct Show 媒体操作器，主要实现Run Stop Pause操作
		m_pMC = nullptr; //Run Stop Pause
		//Direct Show 跳转控制器，主要实现Seek操作
		m_pSeek = nullptr;//Seek 
		//Direct Show 相关
		m_dwObjectTableEntry = 0;

		//Video
		//视频连接标记
		m_bGetVideoType.store(false);

		//分辨率回调函数
		m_cbSize = nullptr;

		m_iRotate = 0;

		//分辨率
		m_iSrcWidth = 0;
		m_iSrcHeight = 0;

		//分辨率
		m_iWidth = 0;
		m_iHeight = 0;

		//画面刷新标记
		m_bVideoRefresh.store(false);

		//自定义预览窗口
		m_hwndDisplay = nullptr;
		//WXMedia视频显示
		m_pWXVideoRender = nullptr;

		//视频回调函数，回调AVFrame数据
		m_pSink = nullptr; //回调对象
		m_cbVideoFrame = nullptr;//回调函数


		//视频回调函数，回调YUV420数据，全局函数，没有回调对象
		m_cbVideoData = nullptr;
		m_cbVideoData2 = nullptr;

		//DXFilter
		m_pD3DRender = nullptr;
		m_uid = 0;
		m_cbSurface = nullptr;
		//WXFfmpegOnAudioData m_cbAudioData = nullptr;

		//显示比例和分辨率比例不一致，需要缩放处理
		m_bHasDAR = FALSE;
		m_dar_num = 1;
		m_dar_den = 1;
		m_iDstWidth = 0;

		//视频数据缓存
		m_pCurrFrame = nullptr;//当前视频帧

		//传入的视频数据类型
		m_iType = WX_DT_VIDEO_INVALD;

		//Aduio
			//音频连接标记
		m_bGetAudioType.store(false);

		//音频变调处理
		m_soundTouch = nullptr;//变调处理

		//WXMedia音频播放对象
		m_pWXAudioRender = nullptr;

		//播放音量，默认100
		m_iVolume.store(100);
		m_nSize = 0;//10ms 数据量
		m_bufAudio = nullptr;
		m_bufTmp = nullptr;

		//Player
		m_bThreadStop = true;
		m_threadWork = nullptr;//工作线程
		m_threadVideo = nullptr;//视频线程

		//输入文件名
		m_strFileName = _T("");

		//附加音频对象
		std::shared_ptr<WXPlayerAudio>m_audioPlay = nullptr;
		//附加音频延迟时间
		m_nAudioDelay.store(0);

		//总时长
		m_nTimeTotal = 0;

		//Seek
		m_newSeek.Clear();
		m_iSeek.store(0);

		//播放速率.默认100，相当于1.0倍速
		m_newSpeed.store(100);
		m_iSpeed.store(100);

		//截图设置
		m_strJPG = _T("");

		//对象的事件名字
		m_strIDEvent = _T("null");
		//事件回调对象
		m_ownerEvent = nullptr;
		//事件回调函数
		m_cbEvent = nullptr;

		m_bHasVideo.store(true);

		//计算上下黑边
		m_nRenderWidth = 0;
		m_nRenderHeight = 0;
		m_sideTop = 0;
		m_sideBottom = 0;
		m_sideLeft = 0;
		m_sideRight = 0;

		m_strError = L""; //错误提示

		m_bLoading.store(false);
		//true表示 OpenFile() 已经结束退出
		m_bExcute.store(false);
		m_bASF.store(false);
		m_bOpen.store(false);

		//功能1，在Start前设置一个独立音轨,音轨文件名，延迟时间
		//动态修改当前音轨文件或延迟时间（需要先关闭原来的音轨）
		//还有跳转处理
		m_bAttachAudio.store(false);

		m_bWillStop.store(false);

		m_bS24.store(false);
		m_bufS24 = nullptr;

		//设置去黑边
		m_bCutSide.store(false);
		m_bWillCutSide.store(false);
		m_bMuted.store(false);

		//渲染帧率计算
		m_iRecv = 0;
		m_iRender = 0;

		m_bGetPic.store(false);
		m_strImage = L"";

		m_iStatus.store(FFPLAY_STATE_UNAVAILABLE);//默认不可用

		m_bCutVideo.store(false);
		m_strCutVideoName = L"";
		m_tsCutVideoStart = 0;
		m_tsCutVideoStop = 0;
		m_dstCutVideoWidth = 0;
		m_dstCutVideoHeight = 0;
	}

	int  GetState() {
		return m_iStatus.load();
	}

	void SetFileName(WXCTSTR wszInput) {
		m_strFileName = wszInput;
	}

	void StartSpeed(int speed) {
		m_iSpeed.store(speed);
	}

	void StartTime(int64_t time) {
		m_iSeek.store(time);
	}

	void DisableVideo() {
		m_bHasVideo.store(false);
	}

	WXCTSTR GetError() {
		return m_strError.str();
	}
	//解析文件/URL
	bool OpenFile() {

		int bParser = false;
		int error = 0;
		void* pInfo = WXMediaInfoCreateFast(m_strFileName.str(), &error);
		if (pInfo) {
			int hasVideo = WXMediaInfoHasVideo(pInfo);
			m_bHasVideo.store(hasVideo);//

			int hasAudio = WXMediaInfoHasAudio(pInfo);
			m_bHasAudio.store(hasAudio);//
			WXMediaInfoDestroy(pInfo);
			bParser = 1;
		}
		if (!bParser || (!m_bHasVideo.load() && !m_bHasAudio.load())) {
			WXLogW(L"%ws Not has Data", m_strFileName.str());
			return false;
		}

		m_pGB = nullptr;
		HRESULT hr = ::CoCreateInstance(CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IFilterGraph2, (void**)&m_pGB);
		if (FAILED(hr)) {
			std::wstring wstr = HrToString(hr);
			WXLogW(L"CLSID_FilterGraph m_pGB is nullptr %ws", wstr.c_str());
			return false;
		}

		int vv1 = DBG_GET_REFCOUNT(m_pGB);

		CComPtr<IRunningObjectTable> objectTable = 0;
		if (SUCCEEDED(::GetRunningObjectTable(0, &objectTable))) {
			WCHAR wsz[256];
			wsprintfW(wsz, L"WXGraph.FilterGraph pid %08x", GetCurrentProcessId());
			CComPtr<IMoniker> pMoniker = 0;
			hr |= ::CreateItemMoniker(L"!", wsz, &pMoniker);
			if (SUCCEEDED(hr)) {
				m_dwObjectTableEntry = 0;
				hr = objectTable->Register(0, m_pGB, pMoniker, &m_dwObjectTableEntry);
				pMoniker = nullptr;
			}
			if (FAILED(hr)) {
				return FALSE;
			}
			objectTable = nullptr;
		}
		m_pMC = nullptr;
		hr = m_pGB->QueryInterface(IID_IMediaControl, (void**)&m_pMC);
		if (FAILED(hr) || m_pMC == nullptr) {
			std::wstring wstr = HrToString(hr);
			WXLogW(L"m_pGB->QueryInterface(IID_IMediaControl) Error %ws", wstr.c_str());
			return FALSE;
		}
		m_pSeek = nullptr;
		hr = m_pGB->QueryInterface(IID_IMediaSeeking, (void**)&m_pSeek);
		if (FAILED(hr)) {
			std::wstring wstr = HrToString(hr);
			WXLogW(L"m_pGB->QueryInterface(IID_IMediaSeeking) Error %ws", wstr.c_str());
			return FALSE;
		}

		bool bOpen = false;
		//WXString strExt = WXBase::GetFileNameSuffix(m_strFileName.str());
		//if (wcsicmp(strExt.str(), L"ASF") == 0
		//	|| wcsicmp(strExt.str(), L"WMV") == 0
		//	|| wcsicmp(strExt.str(), L"WMA") == 0) {
		//	bOpen = OpenNoLav();//使用系统内置解码器
		//}
		if (!bOpen) {
			bOpen = OpenLav();//使用LAV解码器
		}

		m_bOpen.store(bOpen);
		if (m_bOpen.load()) {
			m_bThreadStop.store(false);
			m_threadWork = std::make_shared<std::thread>(&WXPlayer::ThreadWorkProc, this);
			if (m_iWidth && m_iHeight) {

				int sa = m_iSrcWidth * m_dar_den;
				int sb = m_iSrcHeight * m_dar_num;
				if (sa != sb) {
					m_bHasDAR = TRUE;
					m_iDstWidth = (m_iHeight * m_dar_num / m_dar_den) / 4 * 4;
				}
				if (m_iDstWidth == m_iSrcWidth) {
					//比如 852x480(16:9) 计算出来m_iDstWidth 也是852， 需要修复
					m_bHasDAR = FALSE;
				}
				m_bufTarget = std::make_shared<WXDataBuffer>();
				m_bufTarget->Init(nullptr, m_iDstWidth * m_iHeight * 4);

				m_bufRGBA = std::make_shared<WXDataBuffer>();
				m_bufRGBA->Init(nullptr, m_iDstWidth * m_iHeight * 4);

				m_frameCut = std::make_shared<WXVideoFrame>();
				m_frameCut->Init(AV_PIX_FMT_YUV420P, m_iDstWidth, m_iHeight);

				if (m_cbSize) {
					m_cbSize(nullptr, 0, m_iWidth, m_iHeight);
				}
				m_iRotate = m_pSP.get() ? m_pSP->GetRotate() : 0;
				m_arrVideoFrameA = std::make_shared<WXPlayerFrame>();
				m_arrVideoFrameA->Init(m_iWidth, m_iHeight, m_iDstWidth, m_iRotate);
				m_arrVideoFrameB = std::make_shared<WXPlayerFrame>();
				m_arrVideoFrameB->Init(m_iWidth, m_iHeight, m_iDstWidth, m_iRotate);
				m_queuePool.Push(m_arrVideoFrameA.get());
				m_queuePool.Push(m_arrVideoFrameB.get());
				m_threadVideo = std::make_shared<std::thread>(&WXPlayer::ThreadVideoProc, this);
			}
			m_nTimeTotal = GetTotalTime();
			m_bExcute.store(true);
			m_iStatus.store(FFPLAY_STATE_WAITING);
		}
		else {
			m_bExcute.store(true);
			m_iStatus.store(FFPLAY_STATE_ERROR);
		}

		if (m_cbEvent) { //OpenFile 设置了 事件回调
			if (bOpen) {
				m_cbEvent((void*)this, L"OK", (uint32_t)(100 + FFMPEG_ERROR_OK), L"URL");
			}
			else {
				WXLogW(L"OpenFile Error[%ws] cbEvent=%08x", this->GetError(), m_cbEvent);
				m_cbEvent((void*)this, L"ERROR", (uint32_t)(100 + FFMPEG_ERROR_NOFILE), L"URL");
			}
		}
		return bOpen;
	}

	//运行Graph
	bool Run() {
		WXAutoLock al(m_mutex);

		if (!m_bExcute.load()) {
			WXString strErr;
			strErr.Format(L"%ws 正在加载文件 Run Error", m_strFileName.str());
			WXLogW(strErr.str());
			MessageBox(NULL, strErr.str(), L"提示", MB_OK);
			return false;
		}
		if (!m_bOpen.load()) {
			WXString strErr;
			strErr.Format(L"%ws 打开文件失败 Run Error", m_strFileName.str());
			WXLogW(strErr.str());
			MessageBox(NULL, strErr.str(), L"提示", MB_OK);
			return false;
		}
		if (m_pWXAudioRender.get()) {
			m_fPlayerVolume(m_pWXAudioRender.get(), m_bMuted ? 0 : m_iVolume.load());
		}
		if (m_pGB && m_pMC) {
			if (!IsRunning()) {
				HRESULT hr = m_pMC->Run();
				if (SUCCEEDED(hr)) {
					m_iStatus.store(FFPLAY_STATE_PLAYING);
					m_bAttachAudio.store(true);
					return true;
				}
				else {
					m_iStatus.store(FFPLAY_STATE_PAUSE);
				}
			}
			else {
				m_iStatus.store(FFPLAY_STATE_PLAYING);
				return true;
			}
		}
		return false;
	}

	//设置字幕
	void SetSubtitle(WXCTSTR  wszName) {
		WXAutoLock al(m_mutex);
		m_strSubtitle = wszName;
#if USE_FILTER
		if (m_pDVS) {//运行中修改
			m_pDVS->put_FileName((WCHAR*)m_strSubtitle.str());
		}
		SetSubtitleFontImpl();
#endif
	}

	void SetSubtitleHide(int bHide) {
		WXAutoLock al(m_mutex);
		m_bHideSubtitle = !!bHide;
		SetSubtitleFontImpl();
	}

	//设置字幕字体名字、字体大小、字体颜色
	void SetSubtitleStlye(WXCTSTR  strFontName = L"", //字体名字
		int nFontSize = -1,//14  //字体大小
		int dwFontColor = -1,//0x00FFFFFF,//字体颜色
		int nDelay = 0,// 0,  //字幕延时
		int nPostion = -1, //字体位置
		int bBold = TRUE, //是否粗体
		int bItalic = FALSE, //是否斜体
		int bUnderLine = FALSE,//是否有下划线
		int bStrikeOut = FALSE) { //是否有删除线

		WXAutoLock al(m_mutex);
		m_nSubtitleDelay = nDelay;
		m_bBold = bBold;
		m_bItalic = bItalic;
		m_bUnderLine = bUnderLine;
		m_bStrikeOut = bStrikeOut;

		if (nPostion != -1)
			m_nFontPos = nPostion;

		if (nFontSize != -1)
			m_nFontSize = nFontSize;

		if (dwFontColor != -1)
			m_dwFontColor = dwFontColor;

		if (strFontName != nullptr && wcslen(strFontName) > 0) {
			m_strFontName = strFontName;
			//WXLogW(L"%ws[1] Name=%ws size=%d color=%08x",__FUNCTIONW__, strFontName, m_nFontSize, m_dwFontColor);
		}
		else {
			//WXLogW(L"%ws[2]  size=%d color=%08x",__FUNCTIONW__, m_nFontSize, m_dwFontColor);
		}

		SetSubtitleFontImpl();
	}


	//截取视频文件
	void CutVideo(WXCTSTR strName, int64_t tsStart, int64_t tsStop, int dstWidth, int dstHeight) {
		if (!m_bCutVideo.load()) {
			m_strCutVideoName = strName;
			m_tsCutVideoStart = tsStart;
			m_tsCutVideoStop = tsStop;
			m_dstCutVideoWidth = dstWidth;
			m_dstCutVideoHeight = dstHeight;
			m_bCutVideo.store(true);
		}
	}


	void SetUsingCutSide(int bCutSide) {
		WXAutoLock al(m_mutex);

		if (bCutSide) {
			m_bWillCutSide.store(true);
			m_bCutSide.store(true);
		}
		else {
			m_bCutSide.store(false);
		}

		//暂停状态下修改字幕属性时刷新画面
#if USE_FILTER
		if (m_pGB && m_pMC && m_pDVS && !IsRunning()) {
			m_pDVS->Redraw();
			IMediaSample* pSample = m_pDVS->Redraw();
			m_pVideoRender->DoRenderSample(pSample);
		}
#endif
	}

	void Refresh() { //Seek时的画面刷新
		WXAutoLock al(m_mutex);
		if (m_pGB && m_pMC && m_pVRFilter) {
			if (!IsRunning()) {
				m_bVideoRefresh.store(true);
				int nSaveVolume = m_iVolume.load();
				SetVolume(0);//静音
				Run();
				int nCount = 0;
				while (TRUE) {
					if (!m_bVideoRefresh.load() || nCount >= 500) {
						break;
					}
					else {
						SLEEPMS(5);
						nCount++;
					}
				}
				Pause();//停止
				SetVolume(nSaveVolume);//恢复音量
				m_bVideoRefresh.store(false);
			}
		}
	}

	bool IsRunning(void) {
		if (m_pGB && m_pMC) {
			OAFilterState state = State_Stopped;
			HRESULT hr = m_pMC->GetState(10, &state);
			if (SUCCEEDED(hr)) {
				return state == State_Running;
			}
		}
		return false;
	}

	bool IsStopped(void) {
		if (m_pGB && m_pMC) {
			OAFilterState state = State_Stopped;
			HRESULT hr = m_pMC->GetState(10, &state);
			if (SUCCEEDED(hr)) {
				return state == State_Stopped;
			}
		}
		return false;
	}

	bool IsPaused(void) {
		if (m_pGB && m_pMC) {
			OAFilterState state = State_Stopped;
			HRESULT hr = m_pMC->GetState(10, &state);
			if (SUCCEEDED(hr)) {
				return state == State_Paused;
			}
		}
		return false;
	}

	int64_t GetCurrTime() {
		WXAutoLock al(m_mutex);
		if (m_pSeek) {
			int64_t position = 0;
			HRESULT hr = m_pSeek->GetCurrentPosition(&position);
			if (SUCCEEDED(hr)) {
				return (position) / 10000;
			}
		}
		return 0;
	}

	int64_t GetTotalTime() {
		WXAutoLock al(m_mutex);
		int nTotalTime = 0;
		if (m_pSeek && nTotalTime == 0) {
			int64_t length = 0;
			HRESULT hr = m_pSeek->GetDuration(&length);
			if (SUCCEEDED(hr)) {
				nTotalTime = (int64_t)(length / 10000);
				return nTotalTime;
			}
			else {
				std::wstring wstr = HrToString(hr);
				WXLogW(L"%ws Error %ws", __FUNCTIONW__, wstr.c_str());
			}
		}
		return nTotalTime;
	}

	void Seek(int64_t seek) { //异步Seek
		int64_t new_seek = seek;
		if (new_seek < 0) {
			new_seek = 0;
		}
		else if (m_nTimeTotal > 0 && new_seek > m_nTimeTotal) {
			new_seek = m_nTimeTotal;
		}

		if (!IsRunning()) {
			int64_t ts = GetCurrTime();
			if (new_seek == ts) {
				return;
			}
		}
		WXLogW(L"%ws Seek=%lld", __FUNCTIONW__, new_seek);
		m_newSeek.Push(new_seek); //通知底层Seek
		m_bAttachAudio.store(true);//附加音轨
	}

	void SetSpeed(int speed) {
		if (m_bASF.load())  //WMV ASF 不支持 Speed操作
			return;
		int new_speed = speed;
		if (new_speed > 200) {
			new_speed = 200;
		}
		else if (new_speed < 50) {
			new_speed = 50;
		}
		m_newSpeed.store(new_speed);
	}

	double GetSpeed() {
		WXAutoLock al(m_mutex);
		return m_iSpeed / 100.0;
	}

	int    GetVolume() {
		WXAutoLock al(m_mutex);
		return m_iVolume.load();
	}


	void Mute(int bMuted) { //主轨道静音
		WXAutoLock al(m_mutex);
		m_bMuted.store(!!bMuted);
		if (m_pWXAudioRender.get()) {
			m_fPlayerVolume(m_pWXAudioRender.get(), m_bMuted.load() ? 0 : m_iVolume.load());
		}
	}

	bool   SetVolume(int volume) {
		WXAutoLock al(m_mutex);
		m_iVolume.store(av_clip(volume, 0, 100));
		if (m_pWXAudioRender.get()) {
			m_fPlayerVolume(m_pWXAudioRender.get(), m_bMuted.load() ? 0 : m_iVolume.load());
		}
		if (m_audioPlay->IsOpen()) {
			m_audioPlay->SetVolume(m_iVolume.load());
		}
		return false;
	}

	void   SetVideoCB(WXFfmpegOnVideoData cbData) {
		WXAutoLock al(m_mutex);
		if (cbData != nullptr) {
			m_cbVideoData = cbData;
		}
	}

	void   SetVideoCB(WXFfmpegOnVideoData2 cbData) {
		WXAutoLock al(m_mutex);
		if (cbData != nullptr) {
			m_cbVideoData2 = cbData;
		}
	}


	int    SetVideoCB(void* filter, WXFfmpegOnVideoData3 cbData) {
		WXAutoLock al(m_mutex);
		if (filter != nullptr && cbData != nullptr) {
			m_uid = (int64_t)(void*)this;
			m_pD3DRender = filter;
			m_cbSurface = cbData;
			WXDXFilterSetID(m_pD3DRender, m_uid);
			return 1;
		}
		return 0;
	}

	void   SetVideoCB(void* ctx, OnVideoData cbVideoFrame) {
		WXAutoLock al(m_mutex);
		if (cbVideoFrame != nullptr) {
			m_pSink = ctx;
			m_cbVideoFrame = cbVideoFrame;
		}
	}

	void SetAVFrameCB(void* pCtx, int nID, onAVFrame cb) {
		WXAutoLock al(m_mutex);
		if (cb != nullptr) {
			m_pSink = pCtx;
			m_nID = nID;
			m_cbVideoFrame2 = cb;
		}
	}

	void SetView(HWND hwnd) {
		WXAutoLock al(m_mutex);
		m_hwndDisplay = hwnd;
	}

	void Reset() {
		WXAutoLock al(m_mutex);
		Seek(m_iSeek.load());
	}

	void SetSizeCB(ffplayOnSize cbSize) {
		WXAutoLock al(m_mutex);
		if (cbSize)
			m_cbSize = cbSize;
	}

	void SetEventID(WXCTSTR  wsz) {
		WXAutoLock al(m_mutex);
		m_strIDEvent = wsz;
	}

	WXCTSTR  GetEventID() {
		return m_strIDEvent.str();
	}

	void SetEventOwner(void* owner) {
		WXAutoLock al(m_mutex);
		m_ownerEvent = owner;
	}

	void SetEventCB(WXFfmpegOnEvent cbEvent) {
		WXAutoLock al(m_mutex);
		if (cbEvent) {
			WXLogW(L"SetEventCb [%08x]", cbEvent);
			m_cbEvent = cbEvent;
		}
	}

	bool Start(WXCTSTR wszType) { //主线程
		WXAutoLock al(m_mutex);
		if (!m_bExcute.load()) {
			WXString strErr;
			strErr.Format(L"%ws 正在加载文件 %ws Error", m_strFileName.str(), wszType);
			WXLogW(strErr.str());
			MessageBox(NULL, strErr.str(), L"提示", MB_OK);
			return false;
		}
		if (!m_bOpen.load()) {
			WXString strErr;
			strErr.Format(L"%ws 打开文件失败 %ws Error", m_strFileName.str(), wszType);
			WXLogW(strErr.str());
			MessageBox(NULL, strErr.str(), L"提示", MB_OK);
			return false;
		}

		if (m_pARFilter) {
			m_bStopAudio.store(false);
		}

		if (m_pVRFilter) {
			m_bStopVideo.store(false);
		}

		HRESULT hr = S_OK;
#if USE_FILTER
		if (m_pDVS && m_strSubtitle.length() > 0) {
			hr = m_pDVS->put_FileName((WCHAR*)m_strSubtitle.str());
			SetSubtitleFontImpl();
		}
#endif
		if (m_pSeek && m_iSeek.load() != 0) {
			int64_t position = (int64_t)(10000 * (m_iSeek.load()));
			HRESULT hr = m_pSeek->SetPositions(&position, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);
		}

		SetSpeed(m_iSpeed);
		//hr = m_pMC->Run();

		//if (FAILED(hr)) {
		//	std::wstring wstr = HrToString(hr);
		//	WXLogW(L"%ws Error %ws", __FUNCTIONW__, wstr.c_str());
		//	return SUCCEEDED(hr);
		//}


		//如果存在视频链路，就运行刷新一下，出第一帧图像
		if (m_pGB && m_pMC && m_pVRFilter) {
			m_bVideoRefresh.store(true);
			int nSaveVolume = m_iVolume.load();
			SetVolume(0);//静音
			Run();
			int nCount = 0;
			while (TRUE) {
				if (!m_bVideoRefresh.load() || nCount >= 400) {
					break;
				}
				else {
					SLEEPMS(5);
					nCount++;
				}
			}
			Pause();//停止
			SetVolume(nSaveVolume);//恢复音量
			if (m_bVideoRefresh.load()) {
				WXLogA("---- [%s] Refresh new video Frame Error", __FUNCTION__);
			}else {
				WXLogA("---- [%s] Refresh new video Frame OK [nCount=%d]", __FUNCTION__, nCount);
			}
			m_bVideoRefresh.store(false);
		}

		int64_t position = (int64_t)(10000 * (m_iSeek.load()));
		m_pSeek->SetPositions(&position,
			AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0, AM_SEEKING_NoPositioning);

		m_bAttachAudio.store(true);
		Pause();//停止
		m_iStatus.store(FFPLAY_STATE_PAUSE);
		return SUCCEEDED(hr);
	}

	//获取和附加音轨的实际播放延迟
	int64_t  GetDelay() {
		WXAutoLock al(m_mutex);
		if (m_audioPlay->IsOpen()) {
			int64_t t1 = this->GetCurrTime();
			int64_t t2 = m_audioPlay->GetCurrTime();
			return t1 - t2;
		}
		return 0;
	}

	bool Stop() {
		WXAutoLock al(m_mutex);
		if (!m_bExcute.load()) {
			WXLogW(L"OpenFile Is Running, Stop() Error!!");
			return false;
		}
		if (!m_bOpen.load()) {
			WXLogW(L"File Open Error");
		}
		if (m_pWXAudioRender.get()) {
			m_fPlayerVolume(m_pWXAudioRender.get(), 0);
		}
		if (m_audioPlay->IsOpen()) {
			m_audioPlay->Stop();//关闭附加轨道
		}
		if (m_pMC) {
			if (!IsStopped()) {
				m_pMC->Stop();
				m_iStatus.store(FFPLAY_STATE_PLAYING_END);
			}
		}
		return true;
	}

	bool Pause() {
		WXAutoLock al(m_mutex);
		if (!m_bExcute.load()) {
			WXLogW(L"OpenFile Is Running, Stop() Error!!");
			return false;
		}
		if (!m_bOpen.load()) {
			WXLogW(L"File Open Error, Pause()");
			return false;
		}
		if (m_audioPlay->IsOpen()) {
			m_audioPlay->Pause();
		}
		if (m_pWXAudioRender.get()) {
			m_fPlayerVolume(m_pWXAudioRender.get(), 0);
		}
		if (m_pGB && m_pMC) {
			if (!IsPaused()) {
				HRESULT hr = m_pMC->Pause();
				m_iStatus.store(FFPLAY_STATE_PAUSE);
			}
		}
		return true;
	}
};

//----------------------------------------------- LavFilter 播放器 API -------------------------

WXLocker g_lockPlayer; //播放器全局锁
static ThreadSafeQueue<WXPlayer*> g_queuePlayerPool; //播放器对象队列池

//创建播放器对象
//功能：创建播放器对象
//参数：
//wszType: 无意义，一般用nullptr
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//返回值: 播放器对象，返回nullptr表示失败
WXMEDIA_API void* WXPlayerCreate(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek) {
	return WXPlayerCreateEx(wszType, wszInput, speed, seek, nullptr);
}

//功能:创建播放器对象，回调AVFrame数据类型
WXMEDIA_API void* WXPlayerCreate2(WXCTSTR wszType, WXCTSTR wszInput, void* pCtx, OnVideoData cb) {
	WXPlayer* play = (WXPlayer*)WXPlayerCreateEx(wszType, wszInput, 100, 0, nullptr);
	if (play) {
		play->SetVideoCB(pCtx, cb);
	}
	return play;
}

//创建播放器对象,异步执行OpenFile操作，
// 其它线程定时通过WXPlayerGetState()查询该对象状态，
// 等到它返回 FFPLAY_STATE_INIT_OK 就可以执行 Start Run等操作进行播放
//功能：创建播放器对象，并且异步进行文件打开操作
//参数：
//wszType: 无意义，一般用nullptr
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//返回值: 播放器对象，返回nullptr表示失败
WXMEDIA_API void* WXPlayerCreateAsync(WXCTSTR wszType, WXCTSTR wszInput, int speed, int64_t seek) {
	WXAutoLock al(g_lockPlayer);
	if (wszInput == nullptr || wcslen(wszInput) == 0) {
		::MessageBoxW(NULL, L"Empty File/URL Name", L"Notify", MB_OK);
		return nullptr;
	}

	WXPlayer* play = g_queuePlayerPool.Pop();
	if (play == nullptr) {
		play = new WXPlayer();
		WXLogW(L"Create WXPlayer ++ ");
	}
	play->SetFileName(wszInput);
	play->StartSpeed(speed);
	if (wszType && wcsicmp(wszType, L"image") == 0)
		play->StartTime(0);
	else
		play->StartTime(seek);
	if (wszType && wcsicmp(wszType, L"Audio") == 0)
		play->DisableVideo();
	WXTask task = [play] {
		bool bOpen = play->OpenFile();
	};
	WXTaskPost(MAIN_WORK_THREAD, task); //异步执行OpenFile
	int id = (int)GetCurrentThreadId();
	WXLogW(L"%ws [%ws] in[%d] %08x", __FUNCTIONW__, wszInput, id, (void*)play);
	return (void*)play;
}

//创建播放器对象
//功能：创建播放器对象，并且异步进行文件打开操作，通过回调来判断该文件是否可以进行其它操作
//参数：
//wszType: 无意义，一般用nullptr
//wszInput: 输入文件名
//speed: 播放速率，默认100，范围50-200
//seek: 开始播放位置，默认为0
//cbEvent: 异步回调消息
//返回值: 播放器对象，返回nullptr表示失败
WXMEDIA_API void* WXPlayerCreateEx(WXCTSTR wszType,
	WXCTSTR wszInput, int speed, int64_t seek, WXFfmpegOnEvent cbEvent) {
	WXAutoLock al(g_lockPlayer);
	if (wszInput == nullptr || wcslen(wszInput) == 0) {
		::MessageBoxW(NULL, L"Empty File/URL Name", L"Notify", MB_OK);
		return nullptr;
	}
	WXPlayer* play = g_queuePlayerPool.Pop();
	if (play == nullptr) {
		play = new WXPlayer();
		WXLogW(L"Create WXPlayer ++ ");
	}
	play->SetFileName(wszInput);
	play->StartSpeed(speed);
	if (wszType && wcsicmp(wszType, L"image") == 0)
		play->StartTime(0);
	else
		play->StartTime(seek);
	if (wszType && wcsicmp(wszType, L"Audio") == 0)
		play->DisableVideo();

	int id = (int)GetCurrentThreadId();
	WXLogW(L"%ws  [%ws] \r\nin[%d]  [%08x]  cbEvent=[%08x]",
		__FUNCTIONW__, wszInput, id, (void*)play, cbEvent);

	if (cbEvent) {
		play->SetEventCB(cbEvent);
		WXTask task = [play] {
			play->OpenFile();
		};
		WXTaskPost(MAIN_WORK_THREAD, task);
	}
	else {
		int bRet = play->OpenFile();
		if (!bRet) {
			WXPlayerDestroy(play);
			return nullptr;
		}
	}
	return (void*)play;
}

//功能:异步销毁播放器对象
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void  WXPlayerDestroy(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	int id = (int)GetCurrentThreadId();
	WXLogW(L"%ws in[%d] %08x", __FUNCTIONW__, id, (void*)ptr);
	void* pp1 = ptr;
	if (pp1) {
		WXTask task = [pp1] { //异步销毁}
			WXPlayer* play = (WXPlayer*)pp1;
			if (play) {
				play->Close();
				g_queuePlayerPool.Push(play);
				WXLogW(L"Player Queue Size = %d", g_queuePlayerPool.Size());
			}
		};
		WXTaskPost(OTHER_WORK_THREAD, task);//延时执行
	}
}

//功能:给当前播放对象增加附加音轨和附加音轨延迟时间
//参数:
//ptr:播放器对象
//wszAudio:音频文件名，当wszAudio为nullptr时表示调节当前附加音轨的同步时间
//delay:音频播放相对于视频播放的延迟
//大于0表示附加音轨相对主轨道延后播放，小于0表示播放，默认0表示同步播放
//返回值:添加成功返回1，失败返回0
WXMEDIA_API int     WXPlayerAttachAudio(void* ptr, WXCTSTR wszAudio, int64_t delay) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->AttachAudio(wszAudio, delay);
	}
	return 0;
}


//功能:设置循环播放
//参数:
//ptr:播放器对象
//bLoop: 1表示循环播放，0表示不循环
//返回值:无
WXMEDIA_API void    WXPlayerLoop(void* ptr, int bLoop) {
	WXAutoLock al(g_lockPlayer);
	if (ptr) {

	}
}

//功能:获取当前播放速率
//参数:
//ptr:播放器对象
//返回值: 当前播放速率，范围 0.5-2.0
WXMEDIA_API double  WXPlayerGetSpeed(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	return play ? play->GetSpeed() : 0;
}


//功能:设置播放器视频显示窗口(使用底层渲染)
//参数:
//ptr:播放器对象
//hwnd:显示窗口句柄
//返回值:无
WXMEDIA_API void     WXPlayerSetView(void* ptr, HWND hwnd) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetView(hwnd);
}

//功能:设置播放器数据回调函数，返回YUV420P数据类型，可供WPF显示，只适合一个播放实例时的数据回调
//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXPlayerSetVideoCB(void* ptr, WXFfmpegOnVideoData cb) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) play->SetVideoCB(cb);
}

//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXPlayerSetVideoTimeCB(void* ptr, WXFfmpegOnVideoData2 cb) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) play->SetVideoCB(cb);
}

//功能：设置播放器视频回调函数，回调Surface给WPF
//参数:
//ptr:播放器对象
//filter: DXFilter对象，通过WXDXFilter创建
//cb:回调函数
//返回值1表示底层支持YUV处理
//返回值0表示底层不支持YUV处理
WXMEDIA_API int  WXPlayerSetVideoSurfaceCB(void* ptr, void* filter, WXFfmpegOnVideoData3 cb) {
	WXAutoLock al(g_lockPlayer);
	//WXLogW(L"%ws filter=%08x  cb=%08x",__FUNCTIONW__, filter, cb);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		int ret = play->SetVideoCB(filter, cb);
		return ret;
	}
	return 0;
}


//功能:设置音量
//参数:
//ptr:播放器对象
//volum:设置的音量值，范围0-100
//返回值:无
WXMEDIA_API void     WXPlayerSetVolume(void* ptr, int volume) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetVolume(volume);
	}
}

//功能:主轨道静音
//参数:
//ptr:播放器对象
//bMuted: 1 表示静音， 0 表示取消静音
//返回值:无
WXMEDIA_API void     WXPlayerMute(void* ptr, int bMuted) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Mute(bMuted);
	}
}

//功能:设置播放器数据回调函数，返回AVFrame数据类型
//参数:
//ptr:播放器对象
//ctx:对调对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXPlayerSetAVFrameCB(void* ptr, void* ctx, OnVideoData cb) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetVideoCB(ctx, cb);
}

WXMEDIA_API void     WXPlayerSetAVFrameCB2(void* ptr, void* pCtx, int nID, onAVFrame cb) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetAVFrameCB(pCtx,nID, cb);
}


//功能:设置播放器消息回调对象
//参数:
//ptr:播放器对象
//owner:消息回调对象
//返回值:无
WXMEDIA_API void     WXPlayerSetEventOwner(void* ptr, void* owner) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetEventOwner(owner);
}

//功能:设置播放器消息回调函数
//参数:
//ptr:播放器对象
//cb:消息回调函数
//返回值:无
WXMEDIA_API void     WXPlayerSetEventCb(void* ptr, WXFfmpegOnEvent cb) {
	int dw = (int)GetCurrentThreadId(); //Destroy
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetEventCB(cb);
}

//功能:设置播放器大小回调函数，可以返回当前视频的分辨率
//参数:
//ptr:播放器对象
//cb:回调函数
//返回值:无
WXMEDIA_API void     WXPlayerSetSizeCb(void* ptr, ffplayOnSize cb) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)play->SetSizeCB(cb);
}

//功能:设置播放器事件ID名字
//参数:
//ptr:播放器对象
//szID::事件ID名字
//返回值:无
WXMEDIA_API void     WXPlayerSetEventID(void* ptr, WXCTSTR szID) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)return play->SetEventID(szID);
}

//功能:获取播放器事件ID名字
//参数:
//ptr:播放器对象
//返回值:播放器事件ID名字
WXMEDIA_API WXCTSTR  WXPlayerGetEventID(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)return play->GetEventID();
	return nullptr;
}

//功能:开始播放
//参数:
//ptr:播放器对象
//返回值:成功返回1，失败返回0
WXMEDIA_API int      WXPlayerStartEx(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	int id = (int)GetCurrentThreadId();
	WXLogA("%s in [%d] [%08x]", __FUNCTION__, id, ptr);
	WXPlayer* play = (WXPlayer*)ptr;
	int ret = 0;
	if (play) {
		ret = play->Start(L"StartEx");
		return ret;
	}
	return 0;
}

//功能:开始播放
//参数:
//ptr:播放器对象
//返回值:成功返回1，失败返回0
WXMEDIA_API int      WXPlayerStart(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	int id = (int)GetCurrentThreadId();
	WXLogA("%s in [%d] [%08x]", __FUNCTION__, id, ptr);
	WXPlayer* play = (WXPlayer*)ptr;
	int ret = 0;
	if (play) {
		ret = play->Start(L"Start");
		return ret;
	}
	return 0;
}

//功能:结束播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXPlayerStop(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Stop();
	}
}


//功能:获取附加音轨时的实际播放延迟
//参数:
//ptr:播放器对象
//返回值:附加音轨时的实际播放延迟
WXMEDIA_API int64_t  WXPlayerGetDelay(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->GetDelay();
	}
	return 0;
}

//功能:播放时截图
//参数:
//ptr:播放器对象
//wszName:截图文件路径
//quality:截图编码系数
//返回值:无
WXMEDIA_API void     WXPlayerShotPicture(void* ptr, WXCTSTR  wszName, int quality) {
	WXLogW(L"%ws %ws", __FUNCTIONW__, wszName);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->ShotPicture(wszName, quality);
	}
}

//功能:判断播放器是否正在运行
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API int  WXPlayerRunning(void* ptr) {
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->IsRunning();
	}
	return 0;
}

//功能:暂停播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXPlayerPause(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Pause();
	}
}

//功能:恢复播放
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXPlayerResume(void* ptr) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Run();
	}
}


//功能:获取当前播放时间，单位毫秒
//参数:
//ptr:播放器对象
//返回值:获取当前播放时间，单位毫秒
WXMEDIA_API int64_t  WXPlayerGetCurrTime(void* ptr) {
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->GetCurrTime();
	}
	return 0;
}

//功能:获取文件播放总时长，单位毫秒
//参数:
//ptr:播放器对象
//返回值:文件播放总时长，单位毫秒
WXMEDIA_API int64_t  WXPlayerGetTotalTime(void* ptr) {
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->GetTotalTime();
	}
	return 0;
}

//获取当前播放音量，默认100，范围0-100
//功能:
//参数:
//ptr:播放器对象
//返回值:当前播放音量，默认100，范围0-100
WXMEDIA_API int      WXPlayerGetVolume(void* ptr) {
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		return play->GetVolume();
	}
	return 0;
}

//功能:设置播放跳转
//参数:
//ptr:播放器对象
//pts: 跳转值，单位毫秒
//返回值:
WXMEDIA_API void     WXPlayerSeek(void* ptr, int64_t pts) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		WXLogW(L"%ws %lld", __FUNCTIONW__, pts);
		play->Seek(pts);
	}
}

//功能:设置播放器速率
//参数:
//ptr:播放器对象
// speed:播放器速率，范围50-200(对应返回速率的05.-2.0)
//返回值:
WXMEDIA_API void     WXPlayerSpeed(void* ptr, int speed) {
	WXAutoLock al(g_lockPlayer);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSpeed(speed);
	}
}

//功能:获取播放器状态
//参数:
//ptr:播放器对象
//返回值:播放器状态
WXMEDIA_API int      WXPlayerGetState(void* ptr) {
	WXPlayer* play = (WXPlayer*)ptr;
	if (play)return play->GetState();
	return 0;
}


//功能:重置播放器
//参数:
//ptr:播放器对象
//返回值:
WXMEDIA_API void     WXPlayerSetReset(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Reset();
	}
}

//功能:刷新播放器,
//参数:
//ptr:播放器对象
//返回值:无
WXMEDIA_API void     WXPlayerRefresh(void* ptr) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->Refresh();
	}
}



//功能:设置播放器字幕
//参数:
//ptr:播放器对象
//szName:字幕文件名，支持srt、ass等
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitle(void* ptr, WXCTSTR  wszName) {
	//WXLogW(L"[%p]%ws [%ws]", ptr, __FUNCTIONW__, wszName);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitle(wszName);
	}
}


//功能:设置播放器字幕延迟时间，单位为毫秒
//参数:
//ptr:播放器对象
//szName:字幕文件名，支持srt、ass等
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitleDelay(void* ptr, int64_t  nDelay) {
	//WXLogW(L"[%p]%ws nDelay=%lld", ptr, __FUNCTIONW__, nDelay);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(
			L"", -1, -1, nDelay
		);
	}
}


//功能:设置播放器字幕字体
//参数:
//ptr:播放器对象
//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitleFontName(void* ptr, WXCTSTR  strFontName) {
	//WXLogW(L"[%p]%ws FontName=[%ws]",  ptr, __FUNCTIONW__, strFontName);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(strFontName);
	}
}

//功能:设置播放器字幕字体大小，默认14
//参数:
//ptr:播放器对象
//FontSize:字体大小，默认14
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitleFontSize(void* ptr, int nFontSize) {
	//WXLogW(L"[%p] %ws FontSize=%d", ptr, __FUNCTIONW__, nFontSize);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(L"", nFontSize);
	}
}

//功能:设置播放器字幕字体颜色
//参数:
//ptr:播放器对象
//FontColor: RGB值， 如 0xFF00FF 等
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitleFontColor(void* ptr, int dwFontColor) {
	//WXLogW(L"[%p] %ws color=%08x", ptr, __FUNCTIONW__, dwFontColor);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(L"", -1, dwFontColor);
	}
}



//功能:设置播放器字幕隐藏
//参数:
//ptr:播放器对象
//bHide: 隐藏字幕
//返回值:无
WXMEDIA_API void     WXPlayerSetSubtitleHide(void* ptr, int bHide) {
	//	WXLogW(L"[%p] %ws bHide=[%d]",ptr, __FUNCTIONW__, bHide);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleHide(bHide);
	}
}

//功能:设置播放器字幕字体颜色
//参数:
//ptr:播放器对象
//bCutSide: 是否在底层切黑边
//返回值:无
WXMEDIA_API void     WXPlayerSetUsingCutSide(void* ptr, int bCutSide) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetUsingCutSide(bCutSide);
	}
}


//功能:设置播放器字幕字体
//参数:
//ptr:播放器对象
//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
//FontSize:字体大小，默认14
//dwFontColor: RGB值， 如 0xFF00FF 等
//返回值:无
WXMEDIA_API void   WXPlayerSetSubtitleFont(void* ptr, WXCTSTR  strFontName, int nFontSize, int dwFontColor) {
	WXPlayerSetSubtitleFontEx(ptr, strFontName, nFontSize, dwFontColor,
		TRUE, FALSE, FALSE, FALSE);
}


//功能:设置播放器字幕字体
//参数:
//ptr:播放器对象
//strFontName:字体名字,如"宋体"、"黑体"等, 默认"Arial"
//FontSize:字体大小，默认14
//dwFontColor: RGB值， 如 0xFF00FF 等
//bBold: 是否粗体
//bItalic: 是否斜体
//bUnderLine: 是否有下划线
//bStrikeOut: 是否有删除线
//返回值:无
WXMEDIA_API void   WXPlayerSetSubtitleFontEx(void* ptr,
	WXCTSTR  strFontName, int nFontSize, int dwFontColor,
	int bBold, int bItalic, int bUnderLine, int bStrikeOut) {

	//WXLogW(L"[%p] %ws  [%ws][%d][%08x]",ptr, __FUNCTIONW__, strFontName, nFontSize, dwFontColor);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(strFontName, nFontSize, dwFontColor,
			0, -1,
			bBold, bItalic, bUnderLine, bUnderLine);
	}
}


//功能:设置播放器字幕显示位置
//参数:
//ptr:播放器对象
//postion: 显示位置
//返回值:无
WXMEDIA_API void   WXPlayerSetSubtitlePostion(void* ptr, int postion) {
	//WXLogW(L"[%p] %ws Pos=%d",ptr, __FUNCTIONW__, postion);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->SetSubtitleStlye(
			L"", -1, -1, 0,
			postion);
	}
}


//功能:播放器裁剪视频，一般是 MP4 GIF 
//参数:
//ptr:播放器对象
//strName: 显示位置
//tsStart: 开始时间，单位毫秒
//tsStop:  结束时间，单位毫秒
//dstWidth,dstHeight: 输出文件的分辨率，两个为0时表示原分辨率输出
//返回值:无
WXMEDIA_API void   WXPlayerCutVideo(void* ptr, WXCTSTR strName, int64_t tsStart, int64_t tsStop, int dstWidth, int dstHeight) {
	//WXLogW(L"%ws", __FUNCTIONW__);
	WXPlayer* play = (WXPlayer*)ptr;
	if (play) {
		play->CutVideo(strName, tsStart, tsStop, dstWidth, dstHeight);
	}
}