#ifndef _APP_HELPER_H_
#define _APP_HELPER_H_

#include <stdbool.h>
#include <windows.h>
#include <stdio.h>
#include <sddl.h>
#include <winternl.h>
#include <inttypes.h>
#include <windows.h>
#include <dxgi.h>
#include <emmintrin.h>
#include <WXMediaCpp.h>
#include <Win32/WXCursorCapture.h>

#include "wx_hook_helper.h"

#define THREAD_STATE_WAITING 5
#define THREAD_WAIT_REASON_SUSPENDED 5

typedef struct _OBS_SYSTEM_PROCESS_INFORMATION2 {
	ULONG NextEntryOffset;
	ULONG ThreadCount;
	BYTE Reserved1[48];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	BYTE Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} OBS_SYSTEM_PROCESS_INFORMATION2;

typedef struct _OBS_SYSTEM_THREAD_INFORMATION {
	FILETIME KernelTime;
	FILETIME UserTime;
	FILETIME CreateTime;
	DWORD WaitTime;
	PVOID Address;
	HANDLE UniqueProcessId;
	HANDLE UniqueThreadId;
	DWORD Priority;
	DWORD BasePriority;
	DWORD ContextSwitches;
	DWORD ThreadState;
	DWORD WaitReason;
	DWORD Reserved1;
} OBS_SYSTEM_THREAD_INFORMATION;

#ifndef NT_SUCCESS
#define NT_SUCCESS(status) ((NTSTATUS)(status) >= 0)
#endif

#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

#define init_named_attribs(o, name) \
	do { \
		(o)->Length = sizeof(*(o)); \
		(o)->ObjectName = name; \
		(o)->RootDirectory = NULL; \
		(o)->Attributes = 0; \
		(o)->SecurityDescriptor = NULL; \
		(o)->SecurityQualityOfService = NULL; \
	} while (false)

typedef void (WINAPI *RTLINITUNICODESTRINGFUNC)(PCUNICODE_STRING pstr, const wchar_t *lpstrName);
typedef NTSTATUS(WINAPI *NTOPENFUNC)(PHANDLE phandle, ACCESS_MASK access, POBJECT_ATTRIBUTES objattr);
typedef ULONG(WINAPI *RTLNTSTATUSTODOSERRORFUNC)(NTSTATUS status);
typedef NTSTATUS(WINAPI *NTQUERYSYSTEMINFORMATIONFUNC)(SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);

static FARPROC get_nt_func(const char *name)
{
	static bool initialized = false;
	static HMODULE ntdll = NULL;
	if (!initialized) {
		ntdll = GetModuleHandleW(L"ntdll");
		initialized = true;
	}

	return GetProcAddress(ntdll, name);
}

static void nt_set_last_error(NTSTATUS status)
{
	static bool initialized = false;
	static RTLNTSTATUSTODOSERRORFUNC func = NULL;

	if (!initialized) {
		func = (RTLNTSTATUSTODOSERRORFUNC)get_nt_func(
			"RtlNtStatusToDosError");
		initialized = true;
	}

	if (func)
		SetLastError(func(status));
}

static void rtl_init_str(UNICODE_STRING *unistr, const wchar_t *str)
{
	static bool initialized = false;
	static RTLINITUNICODESTRINGFUNC func = NULL;

	if (!initialized) {
		func = (RTLINITUNICODESTRINGFUNC)get_nt_func(
			"RtlInitUnicodeString");
		initialized = true;
	}

	if (func)
		func(unistr, str);
}

#define MAKE_NT_OPEN_FUNC(func_name, nt_name, access) \
static HANDLE func_name(const wchar_t *name) \
{ \
	static bool initialized = false; \
	static NTOPENFUNC open = NULL; \
	HANDLE handle; \
	NTSTATUS status; \
	UNICODE_STRING unistr; \
	OBJECT_ATTRIBUTES attr; \
\
	if (!initialized) { \
		open = (NTOPENFUNC)get_nt_func(#nt_name); \
		initialized = true; \
	} \
\
	if (!open) \
		return NULL; \
\
	rtl_init_str(&unistr, name); \
	init_named_attribs(&attr, &unistr); \
\
	status = open(&handle, access, &attr); \
	if (NT_SUCCESS(status)) \
		return handle; \
	nt_set_last_error(status); \
	return NULL; \
}

MAKE_NT_OPEN_FUNC(nt_open_mutex, NtOpenMutant, SYNCHRONIZE)
MAKE_NT_OPEN_FUNC(nt_open_event, NtOpenEvent, EVENT_MODIFY_STATE | SYNCHRONIZE)
MAKE_NT_OPEN_FUNC(nt_open_map, NtOpenSection, FILE_MAP_READ | FILE_MAP_WRITE)

static bool is_app(HANDLE process)
{
	DWORD size_ret;
	DWORD ret = 0;
	HANDLE token;

	if (OpenProcessToken(process, TOKEN_QUERY, &token)) {
		BOOL success = GetTokenInformation(token, TokenIsAppContainer,
			&ret, sizeof(ret), &size_ret);
		if (!success) {
			DWORD error = GetLastError();
			int test = 0;
		}

		CloseHandle(token);
	}
	return !!ret;
}

static const wchar_t *path_format =
L"\\Sessions\\%lu\\AppContainerNamedObjects\\%s\\%s";

static HANDLE open_app_mutex(const wchar_t *sid, const wchar_t *name)
{
	wchar_t path[MAX_PATH];
	DWORD session_id = WTSGetActiveConsoleSessionId();
	_snwprintf(path, MAX_PATH, path_format, session_id, sid, name);
	return nt_open_mutex(path);
}

static HANDLE open_app_event(const wchar_t *sid, const wchar_t *name)
{
	wchar_t path[MAX_PATH];
	DWORD session_id = WTSGetActiveConsoleSessionId();
	_snwprintf(path, MAX_PATH, path_format, session_id, sid, name);
	return nt_open_event(path);
}

static HANDLE open_app_map(const wchar_t *sid, const wchar_t *name)
{
	wchar_t path[MAX_PATH];
	DWORD session_id = WTSGetActiveConsoleSessionId();
	_snwprintf(path, MAX_PATH, path_format, session_id, sid, name);
	return nt_open_map(path);
}


enum capture_result {
	CAPTURE_FAIL,
	CAPTURE_RETRY,
	CAPTURE_SUCCESS
};

static inline bool is_16bit_format(uint32_t format) {
	return format == DXGI_FORMAT_B5G5R5A1_UNORM ||
		format == DXGI_FORMAT_B5G6R5_UNORM;
}

enum gs_color_format {
	GS_UNKNOWN,
	GS_A8,
	GS_R8,
	GS_RGBA,
	GS_BGRX,
	GS_BGRA,
	GS_R10G10B10A2,
	GS_RGBA16,
	GS_R16,
	GS_RGBA16F,
	GS_RGBA32F,
	GS_RG16F,
	GS_RG32F,
	GS_R16F,
	GS_R32F,
	GS_DXT1,
	GS_DXT3,
	GS_DXT5
};

static inline enum gs_color_format convert_format(uint32_t format) {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:     return GS_RGBA;
	case DXGI_FORMAT_B8G8R8X8_UNORM:     return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_UNORM:     return GS_BGRA;//µ¹Á¢RGBA
	case DXGI_FORMAT_R10G10B10A2_UNORM:  return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM: return GS_RGBA16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT: return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return GS_RGBA32F;
	}
	return GS_UNKNOWN;
}

static WXCTSTR WXGetHookType(int type) {
	switch (type)
	{
	case 0:
		return L"D3D8";
	case 1:
		return L"D3D9";
	case 2:
		return L"D3D10";
	case 3:
		return L"D3D11";
	case 4:
		return L"D3D11_1";
	case 5:
		return L"D3D12";
	case 6:
		return L"OPENGL";
	default:
		return L"UNKNOWN";
	}
}

struct DXGI_R10G10B10A2 {
	unsigned int r : 10;
	unsigned int g : 10;
	unsigned int b : 10;
	unsigned int a : 2;
};

struct Win_RGB32 {
	unsigned int b : 8;
	unsigned int g : 8;
	unsigned int r : 8;
	unsigned int a : 8;
};

static void R10G10B10A2_To_RGB32_Impl(DXGI_R10G10B10A2 src, Win_RGB32 &dst) {
	dst.b = src.b >> 2;
	dst.g = src.g >> 2;
	dst.r = src.r >> 2;
	dst.a = src.a << 6;
}

static void R10G10B10A2_To_RGB32(const uint8_t* pSrc, int srcPitch,
	uint8_t* pDst, int dstPitch,
	int cx, int cy) {
	for (int h1 = 0; h1 < cy; h1++) {
		DXGI_R10G10B10A2 *src = (DXGI_R10G10B10A2*)(pSrc + h1 * srcPitch);
		Win_RGB32 *dst = (Win_RGB32*)(pDst + h1 * dstPitch);
		for (int w1 = 0; w1 < cx; w1++) {
			R10G10B10A2_To_RGB32_Impl(src[w1], dst[w1]);
		}
	}
}

#endif