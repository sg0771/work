#pragma once

#include <list>
#include <string>
#include <DShow.h>
#include <algorithm>
#include <Windows.h>
#include <Commctrl.h>
#include <atlbase.h>
#include <atlconv.h>
#include "../BaseClasses/streams.h"
#include "../BaseClasses/renbase.h"

// Initialize the GUIDs
#include "common_defines.h"
#include "DSUtilLite_growarray.h"

#define LCID_NOSUBTITLES			-1

// SafeRelease Template, for type safety
template <class T> void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

#define DBG_TIMING(x,l,y) y;
#define DbgSetLogFile(sz)
#define DbgSetLogFileDesktop(sz)
#define DbgCloseLogFile()

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(x) if (x) { delete [] x; x = nullptr; }
#endif

// some common macros
#ifndef SAFE_DELETE
#define SAFE_DELETE(pPtr) { delete pPtr; pPtr = nullptr; }
#endif
#define SAFE_CO_FREE(pPtr) { CoTaskMemFree(pPtr); pPtr = nullptr; }
#define CHECK_HR(hr) if (FAILED(hr)) { goto done; }
#define QI(i) (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :
#define countof( array ) ( sizeof( array )/sizeof( array[0] ) )

// Gennenric IUnknown creation function
template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
  *phr = S_OK;
  CUnknown *punk = new T(lpunk, phr);
  if(punk == nullptr) {
    *phr = E_OUTOFMEMORY;
  }
  return punk;
}

 void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);

void split(const std::string& text, const std::string& separators, std::list<std::string>& words);

// Filter Registration
void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCWSTR chkbytes, ...);
void RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, std::list<LPCWSTR> chkbytes, ...);
void UnRegisterSourceFilter(const GUID& subtype);

void RegisterProtocolSourceFilter(const CLSID& clsid, LPCWSTR protocol);
 void UnRegisterProtocolSourceFilter(LPCWSTR protocol);

 BOOL CheckApplicationBlackList(LPCTSTR subkey);

// Locale
 std::string ISO6391ToLanguage(LPCSTR code);
 std::string ISO6392ToLanguage(LPCSTR code);
 std::string ProbeLangForLanguage(LPCSTR code);
 LCID ISO6391ToLcid(LPCSTR code);
 LCID ISO6392ToLcid(LPCSTR code);
 LCID ProbeLangForLCID(LPCSTR code);
 std::string ISO6391To6392(LPCSTR code);
 std::string ISO6392To6391(LPCSTR code);
 std::string ProbeForISO6392(LPCSTR lang);

// FilterGraphUtils
 IBaseFilter *FindFilter(const GUID& clsid, IFilterGraph *pFG);
 BOOL FilterInGraph(const GUID& clsid, IFilterGraph *pFG);
 BOOL FilterInGraphWithInputSubtype(const GUID& clsid, IFilterGraph *pFG, const GUID& clsidSubtype);
 IBaseFilter* GetFilterFromPin(IPin* pPin);
 HRESULT FindIntefaceInGraph(IPin *pPin, REFIID refiid, void **pUnknown);
 HRESULT FindPinIntefaceInGraph(IPin *pPin, REFIID refiid, void **pUnknown);
 HRESULT FindFilterSafe(IPin *pPin, const GUID &guid, IBaseFilter **ppFilter, BOOL bReverse = FALSE);
 BOOL FilterInGraphSafe(IPin *pPin, const GUID &guid, BOOL bReverse = FALSE);
 BOOL HasSourceWithType(IPin *pPin, const GUID &mediaType);


BSTR ConvertCharToBSTR(const char *sz);

int SafeMultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int SafeWideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);
LPWSTR CoTaskGetWideCharFromMultiByte(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte);
LPSTR CoTaskGetMultiByteFromWideChar(UINT CodePage, DWORD dwFlags, LPCWSTR lpMultiByteStr, int cbMultiByte);

unsigned int lav_xiphlacing(unsigned char *s, unsigned int v);

void videoFormatTypeHandler(const AM_MEDIA_TYPE &mt, BITMAPINFOHEADER **pBMI = nullptr, REFERENCE_TIME *prtAvgTime = nullptr, DWORD *pDwAspectX = nullptr, DWORD *pDwAspectY = nullptr);
void videoFormatTypeHandler(const BYTE *format, const GUID *formattype, BITMAPINFOHEADER **pBMI = nullptr, REFERENCE_TIME *prtAvgTime = nullptr, DWORD *pDwAspectX = nullptr, DWORD *pDwAspectY = nullptr);
void audioFormatTypeHandler(const BYTE *format, const GUID *formattype, DWORD *pnSamples, WORD *pnChannels, WORD *pnBitsPerSample, WORD *pnBlockAlign, DWORD *pnBytesPerSec);
void getExtraData(const AM_MEDIA_TYPE &mt, BYTE *extra, size_t *extralen);
void getExtraData(const BYTE *format, const GUID *formattype, const size_t formatlen, BYTE *extra, size_t *extralen);

struct AVPacket;
struct MediaSideDataFFMpeg;
void CopyMediaSideDataFF(AVPacket *dst, const MediaSideDataFFMpeg **sd);
BOOL IsWindows10BuildOrNewer(DWORD dwBuild);

