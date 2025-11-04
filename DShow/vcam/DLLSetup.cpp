/*
将主显示器抓屏为RGB32数据，作为虚拟摄像头
*/
#include <streams.h>
#include <initguid.h>

#include "VCamDesktop.h"


//#pragma comment(lib,"WXBase.lib")
#pragma comment(lib,"BaseClasses.lib")
#pragma comment(lib,"winmm.lib")
//#pragma comment(lib,"libyuv.lib")
#pragma comment(lib,"strmiids.lib")

// Filter setup data
const AMOVIESETUP_MEDIATYPE sudOpPinTypes ={
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOutputPinDesktop = {
    L"Output",      // Obsolete, not used.
    FALSE,          // Is this pin rendered?
    TRUE,           // Is it an output pin?
    FALSE,          // Can the filter create zero instances?
    FALSE,          // Does the filter create multiple instances?
    &CLSID_NULL,    // Obsolete.
    NULL,           // Obsolete.
    1,              // Number of media types.
    &sudOpPinTypes  // Pointer to media types.
};

const AMOVIESETUP_FILTER sudPushSourceDesktop =
{
    &CLSID_PushSourceDesktop,// Filter CLSID
    g_wszPushDesktop,       // String name
    MERIT_DO_NOT_USE,       // Filter merit
    1,                      // Number pins
    &sudOutputPinDesktop    // Pin details
};

CFactoryTemplate g_Templates[] = 
{
    { 
      g_wszPushDesktop,               // Name
      &CLSID_PushSourceDesktop,       // CLSID
      CPushSourceDesktop::CreateInstance, // Method to create an instance of MyComponent
      NULL,                           // Initialization function
      &sudPushSourceDesktop           // Set-up information (for filters)
    },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    

REGFILTER2 rf2FilterReg = {
	1,
	MERIT_NORMAL,
	1,
	&sudOutputPinDesktop
};

STDAPI DllRegisterServer()
{
	HRESULT hr;
	IFilterMapper2 *pFM2 = NULL;

	hr = AMovieDllRegisterServer2(TRUE);
	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
		IID_IFilterMapper2, (void **)&pFM2);

	if (FAILED(hr))
		return hr;

	hr = pFM2->RegisterFilter(
		CLSID_PushSourceDesktop, // Filter CLSID. 
		g_wszPushDesktop, // Filter name.
		NULL, // Device moniker. 
		&CLSID_VideoInputDeviceCategory, // Video compressor category.
		g_wszPushDesktop, // Instance data.
		&rf2FilterReg // Pointer to filter information.
		);
	pFM2->Release();
	return hr;
}

STDAPI DllUnregisterServer(){
	HRESULT hr;

	IFilterMapper2 *pFM2 = NULL;
	hr = AMovieDllRegisterServer2(FALSE);

	if (FAILED(hr))
		return hr;

	hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
		IID_IFilterMapper2, (void **)&pFM2);

	if (FAILED(hr))
		return hr;

	hr = pFM2->UnregisterFilter(&CLSID_VideoInputDeviceCategory,
		g_wszPushDesktop, CLSID_PushSourceDesktop);

	pFM2->Release();
	return hr;
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

