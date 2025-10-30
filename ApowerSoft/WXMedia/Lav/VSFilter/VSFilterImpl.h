
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <afxwin.h> 

#ifdef _WIN64
#pragma warning(disable:4267) // hide warning C4267: conversion from 'size_t' to 'type', possible loss of data
#endif



#define WIN32_LEAN_AND_MEAN					// Exclude rarely-used stuff from Windows headers
#define VC_EXTRALEAN						// Exclude rarely-used stuff from Windows headers
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit



#include <cmath>
#include <algorithm>
#include <vector>
#include <list>
#include <memory>
#include <map>
#include <chrono>
#include <string>
#include <numeric>
#include <regex>
#include <mutex>

#include <atlcoll.h>
#include <atlpath.h>
#include <atlsync.h>
#include <atlcoll.h>
#include <atlbase.h>
#include <atlcom.h>

#include <time.h>
#include <intsafe.h>

#include <emmintrin.h>
#include <Vfw.h>
#include <Mferror.h>
#include <stdint.h>

#include <dshow.h>
#include <dvdmedia.h>
#include <wmcodecdsp.h>
#include <dxva2api.h>
#include "../BaseClasses/streams.h"

#include <WXBase.h>

EXTERN_C const wchar_t* WXGetSubtitleFontName();
EXTERN_C int WXGetSubtitleFontSize();
EXTERN_C int WXGetSubtitleFontColor();
EXTERN_C int WXGetSubtitleFontPos();
EXTERN_C int WXGetSubtitleFontBold();
EXTERN_C int WXGetSubtitleFontItalic();
EXTERN_C int WXGetSubtitleFontUnderLine();
EXTERN_C int WXGetSubtitleFontStrikeOut();

#include <initguid.h>

// {D2667A7E-4055-4244-A65F-DDDDF2B74BD7}
DEFINE_GUID(FORMAT_DiracVideoInfo,
	0xd2667a7e, 0x4055, 0x4244, 0xa6, 0x5f, 0xdd, 0xdd, 0xf2, 0xb7, 0x4b, 0xd7);

// {E487EB08-6B26-4be9-9DD3-993434D313FD}
DEFINE_GUID(MEDIATYPE_Subtitle,
	0xe487eb08, 0x6b26, 0x4be9, 0x9d, 0xd3, 0x99, 0x34, 0x34, 0xd3, 0x13, 0xfd);

// {30424752-0000-0010-8000-00AA00389B71} 'RGB[48]'
DEFINE_GUID(MEDIASUBTYPE_RGB48,
	0x30424752, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);


#define KILOBYTE          1024
#define MEGABYTE       1048576
#define GIGABYTE    1073741824

#define INVALID_TIME INT64_MIN

#define GETU16(b) *(UINT16*)(b)
#define GETU32(b) *(UINT32*)(b)
#define GETU64(b) *(UINT64*)(b)

#ifndef FCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))
#endif

#define SCALE64(a, b, c) (__int64)((double)(a) * (b) / (c)) // very fast, but it can give a small rounding error

#define ALIGN(x, a)           __ALIGN_MASK(x,(decltype(x))(a)-1)
#define __ALIGN_MASK(x, mask) (((x)+(mask))&~(mask))

#define SAFE_DELETE(p)       { if (p) { delete (p);     (p) = nullptr; } }
#define SAFE_DELETE_ARRAY(p) { if (p) { delete [] (p);  (p) = nullptr; } }

#define PCIV_ATI         0x1002
#define PCIV_nVidia      0x10DE
#define PCIV_Intel       0x8086
#define PCIV_S3_Graphics 0x5333

// values from MFVideoTransferMatrix (Win10SDK)
#define VIDEOTRANSFERMATRIX_BT2020_10 4
#define VIDEOTRANSFERMATRIX_BT2020_12 5
// non-standard values
#define VIDEOTRANSFERMATRIX_FCC       6
#define VIDEOTRANSFERMATRIX_YCgCo     7

// values from MFVideoPrimaries (Win10SDK)
#define VIDEOPRIMARIES_BT2020  9
#define VIDEOPRIMARIES_XYZ    10
#define VIDEOPRIMARIES_DCI_P3 11
#define VIDEOPRIMARIES_ACES   12

// values from MFVideoTransferFunction (Win10SDK)
#define VIDEOTRANSFUNC_Log_100     9
#define VIDEOTRANSFUNC_Log_316    10
#define VIDEOTRANSFUNC_709_sym    11
#define VIDEOTRANSFUNC_2020_const 12
#define VIDEOTRANSFUNC_2020       13
#define VIDEOTRANSFUNC_26         14
#define VIDEOTRANSFUNC_2084       15
#define VIDEOTRANSFUNC_HLG        16
#define VIDEOTRANSFUNC_10_rel     17


struct DIRACINFOHEADER {
	VIDEOINFOHEADER2 hdr;
	DWORD cbSequenceHeader;
	DWORD dwSequenceHeader[1];
};

struct WAVEFORMATEXPS2 : public WAVEFORMATEX {
	DWORD dwInterleave;

	struct WAVEFORMATEXPS2() {
		memset(this, 0, sizeof(*this));
		cbSize = sizeof(WAVEFORMATEXPS2) - sizeof(WAVEFORMATEX);
	}
};

typedef struct tagVORBISFORMAT {
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nMinBitsPerSec;
	DWORD nAvgBitsPerSec;
	DWORD nMaxBitsPerSec;
	float fQuality;
} VORBISFORMAT, * PVORBISFORMAT, FAR* LPVORBISFORMAT;

typedef struct tagVORBISFORMAT2 {
	DWORD Channels;
	DWORD SamplesPerSec;
	DWORD BitsPerSample;
	DWORD HeaderSize[3]; // 0: Identification, 1: Comment, 2: Setup
} VORBISFORMAT2, * PVORBISFORMAT2, FAR* LPVORBISFORMAT2;

#pragma pack(push, 1)
typedef struct {
	DWORD dwOffset;
	CHAR IsoLang[4]; // three letter lang code + terminating zero
	WCHAR TrackName[256]; // 256 chars ought to be enough for everyone :)
} SUBTITLEINFO;
#pragma pack(pop)

struct WAVEFORMATEX_HDMV_LPCM : public WAVEFORMATEX {
	BYTE channel_conf;

	struct WAVEFORMATEX_HDMV_LPCM() {
		memset(this, 0, sizeof(*this));
		cbSize = sizeof(WAVEFORMATEX_HDMV_LPCM) - sizeof(WAVEFORMATEX);
	}
};

#pragma pack(push, 1)
struct DVDALPCMFORMAT
{
	WAVEFORMATEX wfe;
	WORD  GroupAssignment;
	DWORD nSamplesPerSec2;
	WORD  wBitsPerSample2;
};
#pragma pack(pop)

struct WAVEFORMATEXFFMPEG
{
	int nCodecId;
	WAVEFORMATEX wfex;

	struct WAVEFORMATEXFFMPEG() {
		memset(this, 0, sizeof(*this));
	}
};

typedef struct tagDOLBYAC3WAVEFORMAT {
	WAVEFORMATEX	wfx;
	BYTE			bBigEndian;		// TRUE = Big Endian, FALSE little endian
	BYTE			bsid;
	BYTE			lfeon;
	BYTE			copyrightb;
	BYTE			nAuxBitsCode;	//  Aux bits per frame
} DOLBYAC3WAVEFORMAT;

struct fraction_t {
	int num;
	int den;
};

struct SyncPoint {
	REFERENCE_TIME rt;
	__int64 fp;
};

struct ColorSpace {
	BYTE MatrixCoefficients;
	BYTE Primaries;
	BYTE Range;
	BYTE TransferCharacteristics;
	BYTE ChromaLocation;
};

// A byte that is not initialized to std::vector when using the resize method.
struct NoInitByte
{
	uint8_t value;
	NoInitByte() {
		// do nothing
		static_assert(sizeof(*this) == sizeof(value), "invalid size");
		//static_assert(__alignof(*this) == __alignof(value), "invalid alignment");
	}
};


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2 /* PI/2 */
#define M_PI_2 1.57079632679489661923
#endif

#ifndef DEBUG_OR_LOG
#if defined(_DEBUG_LOGFILE) || defined(_DEBUG)
#define DEBUG_OR_LOG
#endif
#endif

#ifdef _DEBUG_LOGFILE
#define DLog(...) Logger::Log2File(__VA_ARGS__)
#define DLogIf(f,...) {if (f) Logger::Log2File(__VA_ARGS__);}
#define DLogError(...) Logger::Log2File(__VA_ARGS__)
#elif _DEBUG
#define DLog(...) DbgLogInfo(LOG_TRACE, 3, __VA_ARGS__)
#define DLogIf(f,...) {if (f) DbgLogInfo(LOG_TRACE, 3, __VA_ARGS__);}
#define DLogError(...) DbgLogInfo(LOG_ERROR, 3, __VA_ARGS__)
#else
#define DLog(...) __noop
#define DLogIf(f,...) __noop
#define DLogError(...) __noop
#endif



// this operator is needed to use GUID or CLSID as key in std::map
inline bool operator < (const GUID& a, const GUID& b)
{
	return memcmp(&a, &b, sizeof(GUID)) < 0;
}

// returns an iterator on the found element, or last if nothing is found
template <class T>
auto FindInListByPointer(std::list<T>& list, const T* p)
{
	auto it = list.begin();
	for (; it != list.end(); ++it) {
		if (&(*it) == p) {
			break;
		}
	}

	return it;
}

template <class T>
[[nodiscard]] bool Contains(const T& container, const typename T::value_type& item)
{
	return std::find(container.cbegin(), container.cend(), item) != container.cend();
}

template <typename V, typename... T>
constexpr auto MakeArray(T&&... t)
-> std::array < V, sizeof...(T) >
{
	return { { std::forward<T>(t)... } };
}

template <class T>
CStringT<T, StrTraitMFC<T>> RegExpParse(const T* szIn, const T* szRE)
{
	using StringT = CStringT<T, StrTraitMFC<T>>;
	try {
		const std::basic_regex<T> regex(szRE);
		std::match_results<const T*> match;
		if (std::regex_search(szIn, match, regex) && match.size() == 2) {
			return StringT(match[1].first, match[1].length());
		}
	}
	catch (const std::regex_error& e) {
		UNREFERENCED_PARAMETER(e);
		//DLog(L"RegExpParse(): regex error - '%S'", e.what());
		ASSERT(FALSE);
	}

	return StringT();
};


#define LCID_NOSUBTITLES    -1

#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p) = nullptr; } }
#define SAFE_CLOSE_HANDLE(p) { if (p) { if ((p) != INVALID_HANDLE_VALUE) VERIFY(CloseHandle(p)); (p) = nullptr; } }

#define EXIT_ON_ERROR(hres)  { if (FAILED(hres)) return hres; }

#define IsWaveFormatExtensible(wfe) (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfe->cbSize == 22)

#define QI(i)  (riid == __uuidof(i)) ? GetInterface((i*)this, ppv) :
#define QI2(i) (riid == IID_##i) ? GetInterface((i*)this, ppv) :

#define BeginEnumFilters(pFilterGraph, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if (pFilterGraph && SUCCEEDED(pFilterGraph->EnumFilters(&pEnumFilters))) \
	{ \
		for (CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = nullptr) \
		{ \

#define EndEnumFilters }}}

#define BeginEnumCachedFilters(pGraphConfig, pEnumFilters, pBaseFilter) \
	{CComPtr<IEnumFilters> pEnumFilters; \
	if (pGraphConfig && SUCCEEDED(pGraphConfig->EnumCacheFilter(&pEnumFilters))) \
	{ \
		for (CComPtr<IBaseFilter> pBaseFilter; S_OK == pEnumFilters->Next(1, &pBaseFilter, 0); pBaseFilter = nullptr) \
		{ \

#define EndEnumCachedFilters }}}

#define BeginEnumPins(pBaseFilter, pEnumPins, pPin) \
	{CComPtr<IEnumPins> pEnumPins; \
	if (pBaseFilter && SUCCEEDED(pBaseFilter->EnumPins(&pEnumPins))) \
	{ \
		for (CComPtr<IPin> pPin; S_OK == pEnumPins->Next(1, &pPin, 0); pPin = nullptr) \
		{ \

#define EndEnumPins }}}

#define BeginEnumMediaTypes(pPin, pEnumMediaTypes, pMediaType) \
	{CComPtr<IEnumMediaTypes> pEnumMediaTypes; \
	if (pPin && SUCCEEDED(pPin->EnumMediaTypes(&pEnumMediaTypes))) \
	{ \
		AM_MEDIA_TYPE* pMediaType = nullptr; \
		for (; S_OK == pEnumMediaTypes->Next(1, &pMediaType, nullptr); DeleteMediaType(pMediaType), pMediaType = nullptr) \
		{ \

#define EndEnumMediaTypes(pMediaType) } if (pMediaType) DeleteMediaType(pMediaType); }}

#define BeginEnumSysDev(clsid, pMoniker) \
	{CComPtr<ICreateDevEnum> pDevEnum4$##clsid; \
	pDevEnum4$##clsid.CoCreateInstance(CLSID_SystemDeviceEnum); \
	CComPtr<IEnumMoniker> pClassEnum4$##clsid; \
	if (SUCCEEDED(pDevEnum4$##clsid->CreateClassEnumerator(clsid, &pClassEnum4$##clsid, 0)) \
	&& pClassEnum4$##clsid) \
	{ \
		for (CComPtr<IMoniker> pMoniker; pClassEnum4$##clsid->Next(1, &pMoniker, 0) == S_OK; pMoniker = nullptr) \
		{ \

#define EndEnumSysDev }}}


extern int				CountPins(IBaseFilter* pBF, int& nIn, int& nOut, int& nInC, int& nOutC);
extern IPin* GetFirstPin(IBaseFilter* pBF, PIN_DIRECTION dir = PINDIR_INPUT);
extern IBaseFilter* VS_FindFilter(LPCWSTR clsid, IFilterGraph* pFG);
extern IBaseFilter* VS_FindFilter(const CLSID& clsid, IFilterGraph* pFG);
extern IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const AM_MEDIA_TYPE* pRequestedMT);
extern IPin* FindPin(IBaseFilter* pBF, PIN_DIRECTION direction, const GUID majortype);
extern std::wstring	    GetFilterName(IBaseFilter* pBF);
extern std::wstring		GetPinName(IPin* pPin);

extern IBaseFilter* VS_GetFilterFromPin(IPin* pPin);
extern IPin* AppendFilter(IPin* pPin, std::wstring DisplayName, IGraphBuilder* pGB);
extern IBaseFilter* AppendFilter(IPin* pPin, IMoniker* pMoniker, IGraphBuilder* pGB);
extern IPin* InsertFilter(IPin* pPin, std::wstring DisplayName, IGraphBuilder* pGB);
extern bool				CreateFilter(std::wstring DisplayName, IBaseFilter** ppBF, std::wstring& FriendlyName);
extern bool				HasMediaType(IFilterGraph* pFilterGraph, const GUID& mediaType);

extern void				ExtractMediaTypes(IPin* pPin, std::vector<GUID>& types);
extern void				ExtractMediaTypes(IPin* pPin, std::list<CMediaType>& mts);
extern bool				ExtractBIH(const AM_MEDIA_TYPE* pmt, BITMAPINFOHEADER* bih);
extern bool				ExtractBIH(IMediaSample* pMS, BITMAPINFOHEADER* bih);
extern bool				ExtractAvgTimePerFrame(const AM_MEDIA_TYPE* pmt, REFERENCE_TIME& rtAvgTimePerFrame);
extern bool				ExtractDim(const AM_MEDIA_TYPE* pmt, int& w, int& h, int& arx, int& ary);

extern CLSID			GetCLSID(IBaseFilter* pBF);
extern CLSID			GetCLSID(IPin* pPin);

extern bool				IsCLSIDRegistered(LPCWSTR clsid);
extern bool				IsCLSIDRegistered(const CLSID& clsid);

enum cdrom_t {
	CDROM_NotFound,
	CDROM_Audio,
	CDROM_VideoCD,
	CDROM_DVDVideo,
	CDROM_BDVideo,
	CDROM_DVDAudio,
	CDROM_Unknown
};


extern DVD_HMSF_TIMECODE	RT2HMSF(REFERENCE_TIME rt, double fps = 0); // use to remember the current position
extern DVD_HMSF_TIMECODE	RT2HMS_r(REFERENCE_TIME rt);                // use only for information (for display on the screen)
extern REFERENCE_TIME		HMSF2RT(DVD_HMSF_TIMECODE hmsf, double fps = 0);
extern std::wstring				ReftimeToString(const REFERENCE_TIME& rtVal);  // hour, minute, second, millisec
extern std::wstring				ReftimeToString2(const REFERENCE_TIME& rtVal); // hour, minute, second (round)
extern std::wstring				DVDtimeToString(const DVD_HMSF_TIMECODE& rtVal, bool bAlwaysShowHours = false);
extern REFERENCE_TIME		StringToReftime(LPCWSTR strVal);
extern REFERENCE_TIME		StringToReftime2(LPCWSTR strVal);

extern std::wstring			GetFriendlyName(std::wstring DisplayName);
extern HRESULT			LoadExternalObject(LPCWSTR path, REFCLSID clsid, REFIID iid, void** ppv);
extern HRESULT			LoadExternalFilter(LPCWSTR path, REFCLSID clsid, IBaseFilter** ppBF);
extern HRESULT			LoadExternalPropertyPage(IPersist* pP, REFCLSID clsid, IPropertyPage** ppPP);
extern void				UnloadExternalObjects();

extern std::wstring			MakeFullPath(LPCWSTR path);
// simple file system path detector
extern bool				IsLikelyFilePath(const std::wstring& str);

extern GUID				GUIDFromCString(std::wstring str);
extern HRESULT			GUIDFromCString(std::wstring str, GUID& guid);
extern std::wstring			VS_CStringFromGUID(const GUID& guid);

extern std::wstring			ISO6391ToLanguage(LPCSTR code);
extern std::wstring			ISO6392ToLanguage(LPCSTR code);

extern bool				IsISO639Language(LPCSTR code);
extern std::wstring			ISO639XToLanguage(LPCSTR code, bool bCheckForFullLangName = false);
extern LCID				VS_ISO6391ToLcid(LPCSTR code);
extern LCID				VS_ISO6392ToLcid(LPCSTR code);
extern std::wstring			ISO6391To6392(LPCSTR code);
extern std::wstring			ISO6392To6391(LPCSTR code);
extern std::wstring			LanguageToISO6392(LPCWSTR lang);

extern bool				DeleteRegKey(LPCWSTR pszKey, LPCWSTR pszSubkey);
extern bool				SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValueName, LPCWSTR pszValue);
extern bool				SetRegKeyValue(LPCWSTR pszKey, LPCWSTR pszSubkey, LPCWSTR pszValue);

extern void				RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, LPCWSTR chkbytes, LPCWSTR ext = nullptr, ...);
extern void				RegisterSourceFilter(const CLSID& clsid, const GUID& subtype2, const std::list<std::wstring>& chkbytes, LPCWSTR ext = nullptr, ...);
extern void				UnRegisterSourceFilter(const GUID& subtype);

extern void				TraceFilterInfo(IBaseFilter* pBF);
extern void				TracePinInfo(IPin* pPin);

extern void				getExtraData(const BYTE* format, const GUID* formattype, const ULONG formatlen, BYTE* extra, unsigned int* extralen);

extern int				MakeAACInitData(BYTE* pData, int profile, int freq, int channels);
extern bool				MakeMPEG2MediaType(CMediaType& mt, BYTE* seqhdr, DWORD len, int w, int h);
extern HRESULT			CreateMPEG2VIfromAVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, SIZE pictAR, BYTE* extra, size_t extralen);
extern HRESULT			CreateMPEG2VIfromMVC(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, SIZE pictAR, BYTE* extra, size_t extralen);
extern HRESULT			CreateMPEG2VISimple(CMediaType* mt, BITMAPINFOHEADER* pbmi, REFERENCE_TIME AvgTimePerFrame, SIZE pictAR, BYTE* extra, size_t extralen, DWORD dwProfile = 0, DWORD dwLevel = 0, DWORD dwFlags = 0);

extern std::string			VobSubDefHeader(int w, int h, std::string palette = "");

extern inline const LONGLONG GetPerfCounter();

class CPinInfo : public PIN_INFO
{
public:
	CPinInfo() { pFilter = nullptr; }
	~CPinInfo() { if (pFilter) { pFilter->Release(); } }
};

class CFilterInfo : public FILTER_INFO
{
public:
	CFilterInfo() { pGraph = nullptr; }
	~CFilterInfo() { if (pGraph) { pGraph->Release(); } }
};

template <class T>
static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
	*phr = S_OK;
	CUnknown* punk = new  T(lpunk, phr);
	if (punk == nullptr) {
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}




template <typename T>
// If the specified value is out of range, set to default values.
inline T discard(T const& val, T const& def, T const& lo, T const& hi)
{
	return (val > hi || val < lo) ? def : val;
}

template <typename T>
// If the specified value is out of set, set to default values.
inline T discard(T const& val, T const& def, const std::vector<T>& vars)
{
	if (val != def) {
		for (const auto& v : vars) {
			if (val == v) {
				return val;
			}
		}
	}
	return def;
}

template <typename T>
// If the specified value is out of range, update lo or hi values.
inline void expand_range(T const& val, T& lo, T& hi)
{
	if (val > hi) {
		hi = val;
	}
	if (val < lo) { // do not use "else if" because at the beginning 'lo' may be greater than 'hi'
		lo = val;
	}
}

uint32_t CountBits(uint32_t v);
uint32_t BitNum(uint32_t v, uint32_t b);

void fill_u32(void* dst, uint32_t c, size_t count);
void memset_u32(void* dst, uint32_t c, size_t nbytes);
void memset_u16(void* dst, uint16_t c, size_t nbytes);

// a * b / c
uint64_t RescaleU64x32(uint64_t a, uint32_t b, uint32_t c);
// a * b / c
int64_t RescaleI64x32(int64_t a, uint32_t b, uint32_t c);
// a * b / c
int64_t RescaleI64(int64_t a, int64_t b, int64_t c);

template <typename T>
inline void ReduceDim(T& num, T& den)
{
	if (den > 0 && num > 0) {
		const auto gcd = std::gcd(num, den);
		num /= gcd;
		den /= gcd;
	}
}

inline void ReduceDim(SIZE& dim)
{
	ReduceDim(dim.cx, dim.cy);
}

SIZE ReduceDim(double value);

int IncreaseByGrid(int value, const int step);
int DecreaseByGrid(int value, const int step);

// steps < 0  mean 1.0/(-step)
double IncreaseFloatByGrid(double value, const int step);
// steps < 0  mean 1.0/(-step)
double DecreaseFloatByGrid(double value, const int step);

// checks the multiplicity of the angle of 90 degrees and brings it to the values of 0, 90, 270
bool AngleStep90(int& angle);

// Functions to convert strings to numeric values. On error, the current value does not change and false is returned.

bool StrToInt32(const wchar_t* str, int32_t& value);
bool StrToUInt32(const wchar_t* str, uint32_t& value);
bool StrToInt64(const wchar_t* str, int64_t& value);
bool StrToUInt64(const wchar_t* str, uint64_t& value);
bool StrHexToUInt32(const wchar_t* str, uint32_t& value);
bool StrHexToUInt64(const wchar_t* str, uint64_t& value);
bool StrToDouble(const wchar_t* str, double& value);

//CStringW HR2Str(const HRESULT hr);

// Usage: SetThreadName((DWORD)-1, "MainThread") or SetThreadName(DWORD_MAX, "MainThread")
void VS_SetThreadName(DWORD dwThreadID, LPCSTR szThreadName);
