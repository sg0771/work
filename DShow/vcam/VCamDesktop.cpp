
#include <streams.h>

#include "VCamDesktop.h"

CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CSource *pFilter)
        : CSourceStream(NAME("Push Source Desktop"), phr, pFilter, L"Out"){
	m_pParent = (CPushSourceDesktop*)pFilter;
	m_iImageWidth = m_capture.GetWidth();
	m_iImageHeight = m_capture.GetHeight();
	//m_pFrame = std::make_shared<uint8_t> (new uint8_t[ m_iImageWidth * m_iImageHeight * 4]);
}

CPushPinDesktop::~CPushPinDesktop(){ 
}

HRESULT CPushPinDesktop::GetMediaType(int iPosition, CMediaType *pmt){
    CheckPointer(pmt,E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;

    VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
    if(NULL == pvi) return(E_OUTOFMEMORY);

    ZeroMemory(pvi, sizeof(VIDEOINFO));

	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount = 32;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_iImageWidth;
    pvi->bmiHeader.biHeight     = m_iImageHeight;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;
	pvi->AvgTimePerFrame = FPS_10;
    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle
    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    return NOERROR;
} 


HRESULT CPushPinDesktop::CheckMediaType(const CMediaType *pMediaType){
    CheckPointer(pMediaType,E_POINTER);
    if((*(pMediaType->Type()) != MEDIATYPE_Video) || !(pMediaType->IsFixedSize())){                                                  
        return E_INVALIDARG;
    }

    // Check for the subtypes we support
    const GUID *SubType = pMediaType->Subtype();
    if (SubType == NULL)return E_INVALIDARG;
    if(*SubType != MEDIASUBTYPE_RGB32)return E_INVALIDARG;

    // Get the format area of the media type
    VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();
    if(pvi == NULL)return E_INVALIDARG;
    if(pvi->bmiHeader.biWidth!= m_iImageWidth || abs(pvi->bmiHeader.biHeight) != m_iImageHeight){
        return E_INVALIDARG;
    }
	if (pvi->bmiHeader.biHeight < 0) {
		return E_INVALIDARG;
	}
    return S_OK;  // This format is acceptable.
} // CheckMediaType


HRESULT CPushPinDesktop::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties){
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;
    VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;
    ASSERT(pProperties->cbBuffer);
    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) {return E_FAIL; }
    return S_OK;
} 


HRESULT CPushPinDesktop::SetMediaType(const CMediaType *pMediaType){
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = CSourceStream::SetMediaType(pMediaType);
    if(SUCCEEDED(hr)){
        VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
        if (pvi == NULL)return E_UNEXPECTED;
		if (pvi->bmiHeader.biBitCount == 32){
			m_MediaType = *pMediaType;
			return S_OK;
		}else{
            return E_INVALIDARG;
        }
    } 
    return hr;
} // SetMediaType

HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample){
	//WXAutoLock al(this);
	ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    CheckPointer(pSample, E_POINTER);

	BYTE *pData = nullptr;
    pSample->GetPointer(&pData);
	if (pData != nullptr) {
		long cbData = pSample->GetSize();
		m_capture.GetData(pData);
		REFERENCE_TIME rtStart = m_iFrameNumber * m_rtFrameLength;
		REFERENCE_TIME rtStop = rtStart + m_rtFrameLength;
		pSample->SetTime(&rtStart, &rtStop);
		m_iFrameNumber++;
		pSample->SetSyncPoint(TRUE);
	}

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CPushPinDesktop::SetFormat(AM_MEDIA_TYPE *pmt) {
	VIDEOINFOHEADER* pvi = (VIDEOINFOHEADER*)m_mt.pbFormat;
	if (pmt != NULL) {
		//只支持RGB32格式
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)pmt->pbFormat;
		if (pvi->bmiHeader.biWidth != m_iImageWidth ||
			pvi->bmiHeader.biHeight != m_iImageHeight ||
			pvi->bmiHeader.biCompression != BI_RGB ||
			pvi->bmiHeader.biBitCount != 32)
			return S_FALSE;
	}else{
		m_mt.cbFormat = sizeof(VIDEOINFOHEADER);
		m_mt.pbFormat = (PBYTE)CoTaskMemAlloc(m_mt.cbFormat);
		VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.pbFormat;
		pvi->bmiHeader.biCompression = BI_RGB;
		pvi->bmiHeader.biBitCount = 32;
		pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth  = m_iImageWidth;
		pvi->bmiHeader.biHeight = m_iImageHeight;
		pvi->bmiHeader.biPlanes = 1;
		pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
		pvi->bmiHeader.biClrImportant = 0;
		pvi->bmiHeader.biClrUsed = 0;
		pvi->AvgTimePerFrame = FPS_10;
		SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
		SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle
		m_mt.majortype = MEDIATYPE_Video;
		m_mt.subtype = MEDIASUBTYPE_RGB32;
		m_mt.formattype = FORMAT_VideoInfo;
		m_mt.bTemporalCompression = FALSE;
		m_mt.bFixedSizeSamples = 1;
		m_mt.lSampleSize = pvi->bmiHeader.biSizeImage;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetFormat(AM_MEDIA_TYPE **ppmt){
	SetFormat(NULL);
	*ppmt = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetNumberOfCapabilities(int *piCount, int *piSize){
	*piCount = 1;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CPushPinDesktop::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC){
	
	SetFormat(nullptr);

	*pmt = CreateMediaType(&m_mt);
	VIDEO_STREAM_CONFIG_CAPS  *pvscc = (VIDEO_STREAM_CONFIG_CAPS*)pSCC;
	pvscc->guid = FORMAT_VideoInfo;
	pvscc->VideoStandard = AnalogVideo_None;
	pvscc->InputSize.cx = m_iImageWidth;
	pvscc->InputSize.cy = m_iImageHeight;
	pvscc->MinCroppingSize.cx = 160;//80
	pvscc->MinCroppingSize.cy = 120;//60
	pvscc->MaxCroppingSize.cx = 320;
	pvscc->MaxCroppingSize.cy = 240;
	pvscc->CropGranularityX = 4;
	pvscc->CropGranularityY = 8;
	pvscc->CropAlignX = 2;
	pvscc->CropAlignY = 4;

	pvscc->MinOutputSize.cx = 160;//80
	pvscc->MinOutputSize.cy = 120;//60
	pvscc->MaxOutputSize.cx = 320;
	pvscc->MaxOutputSize.cy = 240;
	pvscc->OutputGranularityX = 4;
	pvscc->OutputGranularityY = 4;
	pvscc->StretchTapsX = 0;
	pvscc->StretchTapsY = 0;
	pvscc->ShrinkTapsX = 0;
	pvscc->ShrinkTapsY = 0;
	pvscc->MinFrameInterval = 1000000 / 10;   //20 fps
	pvscc->MaxFrameInterval = 1000000 / 3; // 10 fps
	pvscc->MinBitsPerSecond = (m_iImageWidth * m_iImageHeight * 24) * 10;
	pvscc->MaxBitsPerSecond = m_iImageWidth * m_iImageHeight * 24 * 50;
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////
HRESULT CPushPinDesktop::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{
	return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CPushPinDesktop::Get(REFGUID guidPropSet,   // Which property set.
	DWORD dwPropID,        // Which property in that set.
	void *pInstanceData,   // Instance data (ignore).
	DWORD cbInstanceData,  // Size of the instance data (ignore).
	void *pPropData,       // Buffer to receive the property data.
	DWORD cbPropData,      // Size of the buffer.
	DWORD *pcbReturned     // Return the size of the property.
	)
{
	if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
	if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

	if (pcbReturned) *pcbReturned = sizeof(GUID);
	if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
	if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.

	*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
	return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CPushPinDesktop::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{	
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	// We support getting this property, but not setting it.
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
	return S_OK;
}


/**********************************************
 *
 *  CPushSourceDesktop Class
 *
 **********************************************/
CPushSourceDesktop::CPushSourceDesktop(IUnknown *pUnk, HRESULT *phr): CSource(NAME("PushSourceDesktop"), pUnk, CLSID_PushSourceDesktop){
    m_pPin = new CPushPinDesktop(phr, this);
	if (phr){
		if (m_pPin == NULL)
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}  
}


CPushSourceDesktop::~CPushSourceDesktop(){
	if (m_pPin) {
		delete m_pPin;
		m_pPin = nullptr;
	}
}


CUnknown * WINAPI CPushSourceDesktop::CreateInstance(IUnknown *pUnk, HRESULT *phr){
    CPushSourceDesktop *pNewFilter = new CPushSourceDesktop(pUnk, phr );
	if (phr){
		if (pNewFilter == NULL) 
			*phr = E_OUTOFMEMORY;
		else
			*phr = S_OK;
	}
    return pNewFilter;
}

STDMETHODIMP CPushSourceDesktop::QueryInterface(REFIID riid, void **ppv){
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else
		return CSource::QueryInterface(riid, ppv);
}