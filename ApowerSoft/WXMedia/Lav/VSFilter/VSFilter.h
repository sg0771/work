#pragma once

#include <atlbase.h>
#include <atlsync.h>
#include <atlcoll.h>
#include <dshow.h>

#include <vector>
#include <memory>
#include <list>

#include <dshow.h>
#include <dxva2api.h>
#include <dvdmedia.h>
#include "../BaseClasses/streams.h"
#include "BaseVideoFilter.h"
#include "IDirectVobSub.h"
#include "STS.h"
#include "ISubPic.h"

class MyMediaSample: public  IMediaSample2
{
public:
	AM_SAMPLE2_PROPERTIES m_propsIn;
	WXDataBuffer m_pBuffer;
	long m_iBufSize = 0;
	REFERENCE_TIME m_TimeStart = 0;
	REFERENCE_TIME m_TimeEnd = 0;
	AM_MEDIA_TYPE  m_mt;
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) {
		return S_OK;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void) {
		return 0;
	}

	virtual ULONG STDMETHODCALLTYPE Release(void) {
		return 0;
	}

	virtual HRESULT STDMETHODCALLTYPE GetProperties(
		/* [in] */ DWORD cbProperties,
		/* [annotation][size_is][out] */
		_Out_writes_bytes_(cbProperties)  BYTE* pbProperties) {
		memcpy(pbProperties, &m_propsIn, cbProperties);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetProperties(
		/* [in] */ DWORD cbProperties,
		/* [annotation][size_is][in] */
		_In_reads_bytes_(cbProperties)  const BYTE* pbProperties) {
		memcpy(&m_propsIn, pbProperties, cbProperties);
		return S_OK;
	}

	//保存当前Sample
	HRESULT Clone(IMediaSample* pIn) {
		BYTE* pDataIn = nullptr;
		if (FAILED(pIn->GetPointer(&pDataIn)) || !pDataIn) {
			return S_FALSE;
		}
		long size = pIn->GetSize();
		if (size == 0) {
			return S_FALSE;
		}

		if (m_pBuffer.GetBuffer() == nullptr) {
			m_pBuffer.Init(nullptr,size);
			m_iBufSize = size;
		}
		memcpy(m_pBuffer.GetBuffer(), pDataIn, m_iBufSize);

		REFERENCE_TIME rtStart, rtStop;
		HRESULT hr = pIn->GetTime(&rtStart, &rtStop);
		this->SetTime(&rtStart, &rtStop);

		AM_MEDIA_TYPE* mt = nullptr;
		hr = pIn->GetMediaType(&mt);
		if (FAILED(hr) || mt == nullptr) {
			return S_FALSE;
		}
		this->SetMediaType(mt);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPointer(BYTE** ppBuffer) {
		*ppBuffer = m_pBuffer.GetBuffer();
		return S_OK;
	}

	virtual long STDMETHODCALLTYPE GetSize(void) {
		return m_iBufSize;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTime(
		/* [annotation][out] */
		_Out_  REFERENCE_TIME* pTimeStart,
		/* [annotation][out] */
		_Out_  REFERENCE_TIME* pTimeEnd) {
		*pTimeStart = m_TimeStart;
		*pTimeEnd = m_TimeEnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetTime(
		/* [annotation][in] */
		_In_opt_  REFERENCE_TIME* pTimeStart,
		/* [annotation][in] */
		_In_opt_  REFERENCE_TIME* pTimeEnd) {
		m_TimeStart = *pTimeStart;
		m_TimeEnd = *pTimeEnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE IsSyncPoint(void) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetSyncPoint(
		BOOL bIsSyncPoint) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE IsPreroll(void) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetPreroll(
		BOOL bIsPreroll) {
		return S_OK;
	}

	virtual long STDMETHODCALLTYPE GetActualDataLength(void) {
		return m_iBufSize;
	}

	virtual HRESULT STDMETHODCALLTYPE SetActualDataLength(
		long __MIDL__IMediaSample0000) {
		m_iBufSize = __MIDL__IMediaSample0000;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMediaType(
		/* [annotation][out] */
		_Out_  AM_MEDIA_TYPE** ppMediaType) {
		memcpy(*ppMediaType, &m_mt, sizeof(AM_MEDIA_TYPE));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMediaType(
		/* [annotation][in] */
		_In_  AM_MEDIA_TYPE* pMediaType) {
		memcpy(&m_mt, pMediaType,  sizeof(AM_MEDIA_TYPE));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE IsDiscontinuity(void) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetDiscontinuity(
		BOOL bDiscontinuity) {
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMediaTime(
		/* [annotation][out] */
		_Out_  LONGLONG* pTimeStart,
		/* [annotation][out] */
		_Out_  LONGLONG* pTimeEnd) {
		*pTimeStart = m_TimeStart;
		*pTimeEnd = m_TimeEnd;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMediaTime(
		/* [annotation][in] */
		_In_opt_  LONGLONG* pTimeStart,
		/* [annotation][in] */
		_In_opt_  LONGLONG* pTimeEnd) {
		if (pTimeStart && pTimeEnd) {
			m_TimeStart = *pTimeStart;
			m_TimeEnd = *pTimeEnd;
		}
		return S_OK;
	}
};

class CDirectVobSub
	: public IDirectVobSub2
	, public IDirectVobSub3
{
public:
	std::wstring m_strLang = L"";

	bool m_fDisabledReloader = false;
	int m_horizontal = 0;
	int m_vertical = 0;
	int m_resx2 = 0;
	int m_resx2minw = 384;
	int m_resx2minh = 288;

	int m_level = 0;
	bool m_fExternalLoad = true;
	bool m_fWebLoad = false;
	bool m_fEmbeddedLoad = true;
protected:
	CDirectVobSub();
	virtual ~CDirectVobSub();

protected:
	CCritSec m_propsLock;

	CString m_FileName = L"";
	int m_iSelectedLanguage = 0;
	bool m_bHideSubtitles = false;
	unsigned int m_uSubPictToBuffer = 0;
	bool m_bAnimWhenBuffering = 1;
	bool m_bAllowDropSubPic = 1;
	bool m_bOverridePlacement = 0;
	int	m_PlacementXperc = 50;
	int m_PlacementYperc = 90;
	bool m_bBufferVobSub = true;
	bool m_bOnlyShowForcedVobSubs = false;
	bool m_bPolygonize = false;
	EPARCompensationType m_ePARCompensationType = EPCTDisabled;

	STSStyle m_defStyle;

	bool m_bAdvancedRenderer = false;
	bool m_bFlipPicture = false;
	bool m_bFlipSubtitles = false;
	bool m_bOSD = false;
	int m_nReloaderDisableCount = 1;
	int m_SubtitleDelay = 0;
	int m_SubtitleSpeedMul = 1000;
	int m_SubtitleSpeedDiv = 1000;
	bool m_bMediaFPSEnabled = false;
	double m_MediaFPS = 25.0;
	bool m_bSaveFullPath = false;
	NORMALIZEDRECT m_ZoomRect{ 0,0,1.0,1.0 };

	CComPtr<ISubClock> m_pSubClock;
	bool m_bForced = false;

public:

	// IDirectVobSub

	STDMETHODIMP get_FileName(WCHAR* fn);
	STDMETHODIMP put_FileName(WCHAR* fn);
	STDMETHODIMP get_LanguageCount(int* nLangs);
	STDMETHODIMP get_LanguageName(int iLanguage, WCHAR** ppName);
	STDMETHODIMP get_SelectedLanguage(int* iSelected);
	STDMETHODIMP put_SelectedLanguage(int iSelected);
	STDMETHODIMP get_HideSubtitles(bool* fHideSubtitles);
	STDMETHODIMP put_HideSubtitles(bool fHideSubtitles);
	STDMETHODIMP get_PreBuffering(bool* fDoPreBuffering); // deprecated
	STDMETHODIMP put_PreBuffering(bool fDoPreBuffering);  // deprecated
	STDMETHODIMP get_SubPictToBuffer(unsigned int* uSubPictToBuffer);
	STDMETHODIMP put_SubPictToBuffer(unsigned int uSubPictToBuffer);
	STDMETHODIMP get_AnimWhenBuffering(bool* fAnimWhenBuffering);
	STDMETHODIMP put_AnimWhenBuffering(bool fAnimWhenBuffering);
	STDMETHODIMP get_AllowDropSubPic(bool* fAllowDropSubPic);
	STDMETHODIMP put_AllowDropSubPic(bool fAllowDropSubPic);
	STDMETHODIMP get_Placement(bool* fOverridePlacement, int* xperc, int* yperc);
	STDMETHODIMP put_Placement(bool fOverridePlacement, int xperc, int yperc);
	STDMETHODIMP get_VobSubSettings(bool* fBuffer, bool* fOnlyShowForcedSubs, bool* fPolygonize);
	STDMETHODIMP put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fPolygonize);
	STDMETHODIMP get_TextSettings(void* lf, int lflen, COLORREF* color, bool* fShadow, bool* fOutline, bool* fAdvancedRenderer);
	STDMETHODIMP put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer);
	STDMETHODIMP get_Flip(bool* fPicture, bool* fSubtitles);
	STDMETHODIMP put_Flip(bool fPicture, bool fSubtitles);
	STDMETHODIMP get_OSD(bool* fShowOSD);
	STDMETHODIMP put_OSD(bool fShowOSD);
	STDMETHODIMP get_SaveFullPath(bool* fSaveFullPath);
	STDMETHODIMP put_SaveFullPath(bool fSaveFullPath);
	STDMETHODIMP get_SubtitleTiming(int* delay, int* speedmul, int* speeddiv);
	STDMETHODIMP put_SubtitleTiming(int delay, int speedmul, int speeddiv);
	STDMETHODIMP get_MediaFPS(bool* fEnabled, double* fps);
	STDMETHODIMP put_MediaFPS(bool fEnabled, double fps);
	STDMETHODIMP get_ZoomRect(NORMALIZEDRECT* rect);
	STDMETHODIMP put_ZoomRect(NORMALIZEDRECT* rect);
	STDMETHODIMP get_ColorFormat(int* iPosition) {
		return E_NOTIMPL;
	}
	STDMETHODIMP put_ColorFormat(int iPosition) {
		return E_NOTIMPL;
	}

	STDMETHODIMP UpdateRegistry();

	STDMETHODIMP HasConfigDialog(int iSelected);
	STDMETHODIMP ShowConfigDialog(int iSelected, HWND hWndParent);

	// settings for the rest are stored in the registry

	STDMETHODIMP IsSubtitleReloaderLocked(bool* fLocked);
	STDMETHODIMP LockSubtitleReloader(bool fLock);
	STDMETHODIMP get_SubtitleReloader(bool* fDisabled);
	STDMETHODIMP put_SubtitleReloader(bool fDisable);

	// the followings need a partial or full reloading of the filter

	STDMETHODIMP get_ExtendPicture(int* horizontal, int* vertical, int* resx2, int* resx2minw, int* resx2minh);
	STDMETHODIMP put_ExtendPicture(int horizontal, int vertical, int resx2, int resx2minw, int resx2minh);
	STDMETHODIMP get_LoadSettings(int* level, bool* fExternalLoad, bool* fWebLoad, bool* fEmbeddedLoad);
	STDMETHODIMP put_LoadSettings(int level, bool fExternalLoad, bool fWebLoad, bool fEmbeddedLoad);

	// IDirectVobSub2

	STDMETHODIMP AdviseSubClock(ISubClock* pSubClock);
	STDMETHODIMP_(bool) get_Forced();
	STDMETHODIMP put_Forced(bool fForced);
	STDMETHODIMP get_TextSettings(STSStyle* pDefStyle);
	STDMETHODIMP put_TextSettings(STSStyle* pDefStyle);
	STDMETHODIMP get_AspectRatioSettings(EPARCompensationType* ePARCompensationType);
	STDMETHODIMP put_AspectRatioSettings(EPARCompensationType* ePARCompensationType);

	// IDirectVobSub3

	STDMETHODIMP get_LanguageType(int iLanguage, int* pType) {
		return E_NOTIMPL;
	}

};

/* This is for graphedit */

class __declspec(uuid("93A22E7A-5091-45ef-BA61-6DA26156A5D0"))
	CDirectVobSubFilter
	: public CBaseVideoFilter
	, public CDirectVobSub
	, public IAMStreamSelect
	, public CAMThread
{
	friend class CTextInputPin;

	CCritSec m_csQueueLock;
	CComPtr<ISubPicQueue> m_pSubPicQueue;
	void InitSubPicQueue();
	SubPicDesc m_spd;

	bool AdjustFrameSize(CSize& s);

	HANDLE m_hEvtTransform = nullptr;

	HRESULT CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced = false);

protected:
	void GetOutputFormats(int& nNumber, VIDEO_OUTPUT_FORMATS** ppFormats);
	void GetOutputSize(int& w, int& h, int& arx, int& ary) override;


	MyMediaSample m_inSample;
	MyMediaSample m_outSample;

public:
	IMediaSample* Redraw();
	HRESULT Transform(IMediaSample* pIn);
	HRESULT HandleImage(BOOL bRender);

public:
	CDirectVobSubFilter(LPUNKNOWN punk, HRESULT* phr, const GUID& clsid = __uuidof(CDirectVobSubFilter));
	virtual ~CDirectVobSubFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// CBaseFilter

	CBasePin* GetPin(int n);
	int GetPinCount();

	STDMETHODIMP JoinFilterGraph(IFilterGraph* pGraph, LPCWSTR pName);
	STDMETHODIMP QueryFilterInfo(FILTER_INFO* pInfo);

	// CTransformFilter
	HRESULT GetMediaType(int iPosition, CMediaType* pMediaType) override;
	HRESULT SetMediaType(PIN_DIRECTION dir, const CMediaType* pMediaType);
	HRESULT CompleteConnect(PIN_DIRECTION dir, IPin* pReceivePin);
	HRESULT BreakConnect(PIN_DIRECTION dir);
	HRESULT StartStreaming();
	HRESULT StopStreaming();
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckOutputType(const CMediaType& mtOut);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DoCheckTransform(const CMediaType* mtIn, const CMediaType* mtOut, bool checkReconnection);

	std::vector<CTextInputPin*> m_pTextInputs;

	// IDirectVobSub
	STDMETHODIMP put_FileName(WCHAR* fn);
	STDMETHODIMP get_LanguageCount(int* nLangs);
	STDMETHODIMP get_LanguageName(int iLanguage, WCHAR** ppName);
	STDMETHODIMP put_SelectedLanguage(int iSelected);
	STDMETHODIMP put_HideSubtitles(bool fHideSubtitles);
	STDMETHODIMP put_PreBuffering(bool fDoPreBuffering); // deprecated
	STDMETHODIMP put_SubPictToBuffer(unsigned int uSubPictToBuffer);
	STDMETHODIMP put_AnimWhenBuffering(bool fAnimWhenBuffering);
	STDMETHODIMP put_AllowDropSubPic(bool fAllowDropSubPic);
	STDMETHODIMP put_Placement(bool fOverridePlacement, int xperc, int yperc);
	STDMETHODIMP put_VobSubSettings(bool fBuffer, bool fOnlyShowForcedSubs, bool fPolygonize);
	STDMETHODIMP put_TextSettings(void* lf, int lflen, COLORREF color, bool fShadow, bool fOutline, bool fAdvancedRenderer);
	STDMETHODIMP put_SubtitleTiming(int delay, int speedmul, int speeddiv);
	STDMETHODIMP get_MediaFPS(bool* fEnabled, double* fps);
	STDMETHODIMP put_MediaFPS(bool fEnabled, double fps);
	STDMETHODIMP get_ZoomRect(NORMALIZEDRECT* rect);
	STDMETHODIMP put_ZoomRect(NORMALIZEDRECT* rect);
	STDMETHODIMP HasConfigDialog(int iSelected);
	STDMETHODIMP ShowConfigDialog(int iSelected, HWND hWndParent);

	// IDirectVobSub2
	STDMETHODIMP put_TextSettings(STSStyle* pDefStyle);
	STDMETHODIMP put_AspectRatioSettings(EPARCompensationType* ePARCompensationType);

	// IDirectVobSub3
	STDMETHODIMP get_LanguageType(int iLanguage, int* pType);

	// IAMStreamSelect
	STDMETHODIMP Count(DWORD* pcStreams);
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);

	// CPersistStream
	STDMETHODIMP GetClassID(CLSID* pClsid);

protected:
	std::vector<VIDEO_OUTPUT_FORMATS> m_VideoOutputFormats;

	HDC m_hdc = nullptr;
	HBITMAP m_hbm = nullptr;
	HFONT m_hfont = nullptr;
	void PrintMessages(BYTE* pOut);

	/* ResX2 */
	std::unique_ptr<BYTE> m_pTempPicBuff;
	HRESULT Copy(BYTE* pSub, BYTE* pIn, CSize sub, CSize in, int bpp, const GUID& subtype, DWORD black);

	// segment start time, absolute time
	CRefTime m_tPrev;
	REFERENCE_TIME CalcCurrentTime();

	double m_fps = 25;

	// 3.x- versions of microsoft's mpeg4 codec output flipped image
	bool m_bMSMpeg4Fix = false;

	// don't set the "hide subtitles" stream until we are finished with loading
	bool m_bLoading = true;

	CString m_videoFileName = L"";

	bool Open();

	int FindPreferedLanguage(bool fHideToo = true);
	void UpdatePreferedLanguages(CString lang);

	CCritSec m_csSubLock;
	CInterfaceList<ISubStream> m_pSubStreams;
	DWORD_PTR m_nSubtitleId = 0;
	void UpdateSubtitle(bool fApplyDefStyle = true);
	void SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle = true);
	void InvalidateSubtitle(REFERENCE_TIME rtInvalidate = -1, DWORD_PTR nSubtitleId = -1);

	// the text input pin is using these
	void AddSubStream(ISubStream* pSubStream);
	void RemoveSubStream(ISubStream* pSubStream);
	void Post_EC_OLE_EVENT(CString str, DWORD_PTR nSubtitleId = -1);

private:
	class CFileReloaderData
	{
	public:
		ATL::CEvent EndThreadEvent, RefreshEvent;
		std::list<CString> files;
		std::vector<CTime> mtime;
	} m_frd;

	void SetupFRD(CStringArray& paths, std::vector<HANDLE>& handles);
	DWORD ThreadProc();

private:
	VIDEOINFOHEADER2 m_CurrentVIH2;

	bool m_bExternalSubtitle = false;
	std::list<ISubStream*> m_ExternalSubstreams;
	int get_ExternalSubstreamsLanguageCount();
};




