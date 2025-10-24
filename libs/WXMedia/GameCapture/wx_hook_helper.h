/*
游戏Hook通用头文件
*/

#ifndef _WX_HOOK_HELPER__H_
#define _WX_HOOK_HELPER__H_

#include <Windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <string.h>

#define MEMSIZE 1024
#define SHARED_OFFSETS  L"WXMedia_Offsets"

#define EVENT_CAPTURE_RESTART L"CaptureHook_Restart"
#define EVENT_CAPTURE_STOP    L"CaptureHook_Stop"

#define EVENT_HOOK_READY      L"CaptureHook_HookReady"
#define EVENT_HOOK_EXIT       L"CaptureHook_Exit"

#define EVENT_HOOK_INIT       L"CaptureHook_Initialize"

#define WINDOW_HOOK_KEEPALIVE L"CaptureHook_KeepAlive"

#define MUTEX_TEXTURE1        L"CaptureHook_TextureMutex1"
#define MUTEX_TEXTURE2        L"CaptureHook_TextureMutex2"

#define SHMEM_HOOK_INFO       L"CaptureHook_HookInfo"
#define SHMEM_TEXTURE         L"CaptureHook_Texture"

#define GC_EVENT_FLAGS (EVENT_MODIFY_STATE | SYNCHRONIZE)
#define GC_MUTEX_FLAGS (SYNCHRONIZE)

static inline HANDLE create_event(const wchar_t *name){
	return CreateEventW(NULL, false, false, name);
}

static inline HANDLE open_event(const wchar_t *name){
	return OpenEventW(GC_EVENT_FLAGS, false, name);
}

static inline HANDLE create_mutex(const wchar_t *name){
	return CreateMutexW(NULL, false, name);
}

static inline HANDLE open_mutex(const wchar_t *name){
	return OpenMutexW(GC_MUTEX_FLAGS, false, name);
}

static inline HANDLE create_event_plus_id(const wchar_t *name, DWORD id){
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", name, id);
	return create_event(new_name);
}

static inline HANDLE create_mutex_plus_id(const wchar_t *name, DWORD id){
	wchar_t new_name[64];
	_snwprintf(new_name, 64, L"%s%lu", name, id);
	return create_mutex(new_name);
}

static inline bool object_signalled(HANDLE event){
	if (!event)
		return false;
	return WaitForSingleObject(event, 0) == WAIT_OBJECT_0;
}

#pragma pack(push, 8)

struct d3d8_offsets {
	uint32_t present;
};

struct d3d9_offsets {
	uint32_t present;
	uint32_t present_ex;
	uint32_t present_swap;
	uint32_t d3d9_clsoff;
	uint32_t is_d3d9ex_clsoff;
};

struct dxgi_offsets {
	uint32_t present;
	uint32_t resize;
	uint32_t present1;
};

struct shmem_data {
	volatile int last_tex;
	uint32_t tex1_offset;
	uint32_t tex2_offset;
};

struct shtex_data {
	uint32_t tex_handle;
};

struct graphics_offsets {
	struct d3d8_offsets d3d8;
	struct d3d9_offsets d3d9;
	struct dxgi_offsets dxgi;
};

enum HOOK_TYPE {
	HOOK_TYPE_UNKNOWN = -1,
	HOOK_TYPE_D3D8 = 0,
	HOOK_TYPE_D3D9 = 1,
	HOOK_TYPE_D3D10 = 2,
	HOOK_TYPE_D3D11 = 3,
	HOOK_TYPE_D3D12 = 5,
	HOOK_TYPE_OPENGL = 6,
	HOOK_TYPE_DDRAW = 7,
};

#define MAX_CAMERA_WIDTH  640
#define MAX_CAMERA_HEIGHT 480
struct hook_info {
	/* capture info */
	uint32_t                       window;
	uint32_t                       format;
	uint32_t                       cx;
	uint32_t                       cy;
	uint32_t                       pitch;
	uint32_t                       map_id;
	uint32_t                       map_size;
	bool                           flip;

	int      m_bCapture;//是否录制，退出HOOK 应该重置0
	int      m_modeFPS; //0按秒显示，1按帧显示
	int      m_nFps;//底层FPS统计值

	/* hook addresses */
	struct graphics_offsets        offsets;

	int  m_iType;//hook的类型

	//需要FreeType
	int  m_bDrawText;//0不绘制 ， 1，2，3，4 表示绘制位置
	int  m_bChangeText;//文字修改

	int  m_iFontSize;//字体大小
	wchar_t m_strFont[MAX_PATH];//字体名字
	uint32_t m_colorText;// = 0x00FFFFFF;//红色
	uint32_t m_colorBk;// = 0x00FFFFFF;//红色
	int      m_posxText;//绘制位置
	int      m_posyText;
	wchar_t HookMsg[MAX_PATH];//文字内容

	//需要GDI+把数据转换为
	int  m_bDrawImage;//0不绘制 ， 1，2，3，4 表示绘制位置
	int  m_bChangeImage;
	int      m_posxImage;//绘制位置
	int      m_posyImage;
	wchar_t m_HookImage[MAX_PATH];//水印图片名字

	//Camera 数据
	int  m_bDrawCamera;//0不绘制 ， 1，2，3，4 表示绘制位置
	int  m_bChangeCamera;
	int  m_nPosXCamera;//绘制位置
	int  m_nPosYCamera;
	int  m_nWidthCamera; //宽度，最大640？
	int  m_nHeightCamera;//高度，最大480
	uint8_t m_pCamera[1];//640x480 RGBA...注意缩放处理！
};

#pragma pack(pop)

static inline HANDLE create_hook_info(DWORD id){
	wchar_t new_name[64];
	memset(new_name,0,64*sizeof(wchar_t));
	_snwprintf(new_name, 64, L"%s%lu", SHMEM_HOOK_INFO, id);
	return CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
		0, sizeof(struct hook_info) + MAX_CAMERA_WIDTH * MAX_CAMERA_HEIGHT * 4, new_name);
}

static void wxlog(const char *format, ...){
	va_list args;
	va_start(args, format);
	char messageA[1024] = "WXMedia Tam ";
	char message[1024] = "";
	int num = _vsprintf_p(message, 1024, format, args);
	if (num) {
		strcat(messageA, message);
		OutputDebugStringA(messageA);
	}
	va_end(args);
}

static void wxlogW(const wchar_t *format, ...) {
	va_list args;
	va_start(args, format);
	wchar_t messageA[1024] = L"WXMedia Tam ";
	wchar_t message[1024] = L"";
	int num = vswprintf(message, 1024, format, args);
	if (num) {
		wcscat(messageA, message);
		OutputDebugString(messageA);
	}
	va_end(args);
}

static bool is_uwp_window(HWND hwnd) {
	wchar_t name[256];
	name[0] = 0;
	if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
		return false;
	return wcscmp(name, L"ApplicationFrameWindow") == 0;
}

static HWND get_uwp_actual_window(HWND parent) {
	DWORD parent_id = 0;
	HWND child;
	GetWindowThreadProcessId(parent, &parent_id);
	child = FindWindowEx(parent, NULL, NULL, NULL);
	while (child) {
		DWORD child_id = 0;
		GetWindowThreadProcessId(child, &child_id);
		if (child_id != parent_id)
			return child;
		child = FindWindowEx(parent, child, NULL, NULL);
	}
	return NULL;
}

static inline bool is_64bit_windows(void) {
	BOOL x86 = false;
	bool success = !!IsWow64Process(GetCurrentProcess(), &x86);
	return success && !!x86;
}

static inline bool is_64bit_process(HANDLE process) {
	BOOL x86 = true;
	if (is_64bit_windows()) {
		bool success = !!IsWow64Process(process, &x86);
		if (!success) {
			return false;
		}
	}

	return !x86;
}

static void close_handle(HANDLE *p_handle) {
	HANDLE handle = *p_handle;
	if (handle) {
		if (handle != INVALID_HANDLE_VALUE)
			CloseHandle(handle);
		*p_handle = NULL;
	}
}

static void WXGameExcute(LPCSTR strExe) {
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOA si = { 0 };
	si.cb = sizeof(si);
	bool success = !!CreateProcessA(strExe, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (success) {
		//Wait for exit
		WaitForSingleObject(pi.hProcess, INFINITE);
		DWORD exitCode = 0;
		if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
			wxlog(" %s  Exit code = %d/n", strExe, exitCode);
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

/* this is a workaround to A/Vs going crazy whenever certain functions (such as
* OpenProcess) are used */
#define LOWER_HALFBYTE(x) ((x) & 0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

static void deobfuscate_str(char *str, uint64_t val)
{
	uint8_t *dec_val = (uint8_t*)&val;
	int i = 0;

	while (*str != 0) {
		int pos = i / 2;
		bool bottom = (i % 2) == 0;
		uint8_t *ch = (uint8_t*)str;
		uint8_t _xor = bottom ?
			LOWER_HALFBYTE(dec_val[pos]) :
			UPPER_HALFBYTE(dec_val[pos]);

		*ch ^= _xor;

		if (++i == sizeof(uint64_t) * 2)
			i = 0;

		str++;
	}
}

static void *get_obfuscated_func(HMODULE module, const char *str, uint64_t val)
{
	char new_name[128];
	strcpy(new_name, str);
	deobfuscate_str(new_name, val);
	return GetProcAddress(module, new_name);
}

#endif //_WX_HOOK_HELPER__H_