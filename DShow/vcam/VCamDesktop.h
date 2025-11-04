#include <strsafe.h>
#include "DesktopCapture.h"
#include <memory>
#include <stdint.h>

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);
const REFERENCE_TIME FPS_10 = UNITS / 10;

// {4EA6930A-2C8A-4ae6-A561-56E4B5044437}
DEFINE_GUID(CLSID_PushSourceDesktop,0x4ea6930a, 0x2c8a, 0x4ae6, 0xa5, 0x61, 0x56, 0xe4, 0xb5, 0x4, 0x44, 0x37);
// Filter name strings
#define g_wszPushDesktop    L"VCam"

class CPushSourceDesktop;
class CPushPinDesktop : public CSourceStream, public IAMStreamConfig, public IKsPropertySet{
protected:
	CPushSourceDesktop *m_pParent;
	WindowsDesktopCapture m_capture;
    int m_FramesWritten=0;				// To track where we are in the file
    CRefTime m_rtSampleTime;	        // The time stamp for each sample
    int m_iFrameNumber = 0;
    const REFERENCE_TIME m_rtFrameLength = FPS_10;
    int m_iImageHeight = 0;                 // The current image height
    int m_iImageWidth = 0;                  // And current image width
    int m_iRepeatTime;                  // Time in msec between frames
    CMediaType m_MediaType;
	std::shared_ptr<uint8_t> m_pFrame = nullptr;
public:
    CPushPinDesktop(HRESULT *phr, CSource *pFilter);
    ~CPushPinDesktop();

    // Override the version that offers exactly one media type
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
    
    // Set the agreed media type and set up the necessary parameters
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // Support multiple display formats
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);

    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q){
        return E_FAIL;
    }


	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv){
		if (riid == _uuidof(IAMStreamConfig))
			*ppv = (IAMStreamConfig*)this;
		else if (riid == _uuidof(IKsPropertySet))
			*ppv = (IKsPropertySet*)this;
		else
			return CSourceStream::QueryInterface(riid, ppv);

		AddRef();
		return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
};

class CPushSourceDesktop : public CSource{
private:
    // Constructor is private because you have to use CreateInstance
    CPushSourceDesktop(IUnknown *pUnk, HRESULT *phr);
    ~CPushSourceDesktop();
    CPushPinDesktop *m_pPin;
public:
    static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);  
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFilterGraph *GetGraph() { return m_pGraph; }
};


