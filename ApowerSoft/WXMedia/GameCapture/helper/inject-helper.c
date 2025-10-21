#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windows.h>
#include <shellapi.h>
#include <stdbool.h>
#include "../wx_hook_helper.h"

#define INJECT_ERROR_INJECT_FAILED     -1
#define INJECT_ERROR_INVALID_PARAMS    -2
#define INJECT_ERROR_OPEN_PROCESS_FAIL -3
#define INJECT_ERROR_UNLIKELY_FAIL     -4

static int inject_library_obf(HANDLE process, const wchar_t *dll,
	const char *create_remote_thread_obf, uint64_t obf1,
	const char *write_process_memory_obf, uint64_t obf2,
	const char *virtual_alloc_ex_obf, uint64_t obf3,
	const char *virtual_free_ex_obf, uint64_t obf4,
	const char *load_library_w_obf, uint64_t obf5);

static int inject_library_safe_obf(DWORD thread_id, const wchar_t *dll,
	const char *set_windows_hook_ex_obf, uint64_t obf1);

static void load_debug_privilege(void)
{
	const DWORD flags = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
	TOKEN_PRIVILEGES tp;
	HANDLE token;
	LUID val;

	if (!OpenProcessToken(GetCurrentProcess(), flags, &token)) {
		return;
	}

	if (!!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &val)) {
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = val;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(token, false, &tp,
				sizeof(tp), NULL, NULL);
	}

	CloseHandle(token);
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
		DWORD process_id)
{
	HANDLE (WINAPI *open_process_proc)(DWORD, BOOL, DWORD);
	open_process_proc = get_obfuscated_func(GetModuleHandleW(L"KERNEL32"),
			"HxjcQrmkb|~", 0xc82efdf78201df87);
	return open_process_proc(desired_access, inherit_handle, process_id);
}

static int inject_helper(wchar_t *argv[], const wchar_t *dll)
{
	wxlog("%s Start ---------", __FUNCTION__);
	DWORD use_safe_inject = wcstol(argv[2], NULL, 10);
	DWORD id = wcstol(argv[3], NULL, 10);
	if (id == 0) {
		return INJECT_ERROR_INVALID_PARAMS;
	}
	int ret = 0;
	if (use_safe_inject) {
		ret = inject_library_safe_obf(id, dll,
			"[bs^fbkmwuKfmfOvI", 0xEAD293602FCF9778ULL);
		wxlog("%s AAA ret= %d", __FUNCTION__, ret);
	}else {
		HANDLE process = open_process(PROCESS_ALL_ACCESS, false, id);
		if (process) {
			ret = inject_library_obf(process, dll,
				"E}mo|d[cefubWk~bgk", 0x7c3371986918e8f6,
				"Rqbr`T{cnor{Bnlgwz", 0x81bf81adc9456b35,
				"]`~wrl`KeghiCt", 0xadc6a7b9acd73c9b,
				"Zh}{}agHzfd@{", 0x57135138eb08ff1c,
				"DnafGhj}l~sX", 0x350bfacdf81b2018);
			CloseHandle(process);
			wxlog("%s BBB ret= %d", __FUNCTION__, ret);
		}else {
			ret = INJECT_ERROR_OPEN_PROCESS_FAIL;
			wxlog("%s CCC ret= %d", __FUNCTION__, ret);
		}
	}
	return ret;
}


int inject_helper_main(int argc, char *argv_ansi[])
{
	wchar_t dll_path[MAX_PATH];
	LPWSTR pCommandLineW;
	LPWSTR *argv;
	int ret = INJECT_ERROR_INVALID_PARAMS;

	SetErrorMode(SEM_FAILCRITICALERRORS);
	load_debug_privilege();

	pCommandLineW = GetCommandLineW();
	argv = CommandLineToArgvW(pCommandLineW, &argc);
	if (argv && argc == 4) {
		DWORD size = GetModuleFileNameW(NULL,
				dll_path, MAX_PATH);
		if (size) {
			wchar_t *name_start = wcsrchr(dll_path, '\\');
			if (name_start) {
				*(++name_start) = 0;
				wcscpy(name_start, argv[1]);
				ret = inject_helper(argv, dll_path);
			}
		}
	}
	LocalFree(argv);
	return ret;//ret = 0  HOOK OK
}

extern int get_offsets();

int main(int argc, char *argv_ansi[]) {
	if (argc == 4) {
		return inject_helper_main(argc, argv_ansi);
	}
	return get_offsets();
}

//---------------------------------------
typedef HANDLE(WINAPI *create_remote_thread_t)(HANDLE, LPSECURITY_ATTRIBUTES,SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
typedef BOOL(WINAPI *write_process_memory_t)(HANDLE, LPVOID, LPCVOID, SIZE_T,SIZE_T*);
typedef LPVOID(WINAPI *virtual_alloc_ex_t)(HANDLE, LPVOID, SIZE_T, DWORD,DWORD);
typedef BOOL(WINAPI *virtual_free_ex_t)(HANDLE, LPVOID, SIZE_T, DWORD);

static int inject_library_obf(HANDLE process, const wchar_t *dll,
	const char *create_remote_thread_obf, uint64_t obf1,
	const char *write_process_memory_obf, uint64_t obf2,
	const char *virtual_alloc_ex_obf, uint64_t obf3,
	const char *virtual_free_ex_obf, uint64_t obf4,
	const char *load_library_w_obf, uint64_t obf5)
{
	int ret = INJECT_ERROR_UNLIKELY_FAIL;
	DWORD last_error = 0;

	HMODULE kernel32 = GetModuleHandleW(L"KERNEL32");
	create_remote_thread_t create_remote_thread = get_obfuscated_func(kernel32,create_remote_thread_obf, obf1);
	write_process_memory_t write_process_memory = get_obfuscated_func(kernel32,write_process_memory_obf, obf2);
	virtual_alloc_ex_t virtual_alloc_ex = get_obfuscated_func(kernel32,virtual_alloc_ex_obf, obf3);
	virtual_free_ex_t virtual_free_ex = get_obfuscated_func(kernel32,virtual_free_ex_obf, obf4);
	FARPROC load_library_w = get_obfuscated_func(kernel32,load_library_w_obf, obf5);

	size_t size = (wcslen(dll) + 1) * sizeof(wchar_t);
	void * mem = virtual_alloc_ex(process, NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!mem) {
		wxlog("%s virtual_alloc_ex error", __FUNCTION__);
		goto fail;
	}

	size_t written_size = 0;
	BOOL success = write_process_memory(process, mem, dll,
		size, &written_size);
	if (!success) {
		wxlog("%s write_process_memory error", __FUNCTION__);
		goto fail;
	}

	DWORD thread_id = 0;
	HANDLE thread = NULL;
	thread = create_remote_thread(process, NULL, 0,
		(LPTHREAD_START_ROUTINE)load_library_w, mem, 0,
		&thread_id);
	if (!thread) {
		wxlog("%s create_remote_thread error", __FUNCTION__);
		goto fail;
	}

	if (WaitForSingleObject(thread, 4000) == WAIT_OBJECT_0) {
		DWORD code = 0;
		GetExitCodeThread(thread, &code);
		ret = (code != 0) ? 0 : INJECT_ERROR_INJECT_FAILED;
		if (ret != 0) {
			wxlog("%s WaitForSingleObject error", __FUNCTION__);
		}
		SetLastError(0);
	}

fail:
	if (ret == INJECT_ERROR_UNLIKELY_FAIL) {
		last_error = GetLastError();
	}
	if (thread) {
		CloseHandle(thread);
	}
	if (mem) {
		virtual_free_ex(process, mem, 0, MEM_RELEASE);
	}
	if (last_error != 0) {
		SetLastError(last_error);
	}
	return ret;
}

/* ------------------- 安全模式注入 ---------------------------------------- */

typedef HHOOK(WINAPI *set_windows_hook_ex_t)(int, HOOKPROC, HINSTANCE, DWORD);

#define RETRY_INTERVAL_MS      500
#define TOTAL_RETRY_TIME_MS    4000
#define RETRY_COUNT            (TOTAL_RETRY_TIME_MS / RETRY_INTERVAL_MS)

static int inject_library_safe_obf(DWORD thread_id, const wchar_t *dll,
	const char *set_windows_hook_ex_obf, uint64_t obf1)
{
	HMODULE user32 = GetModuleHandleW(L"USER32");
	if (!user32) {
		wxlog("%s GetModuleHandleW(L\"USER32\") error", __FUNCTION__);
		return INJECT_ERROR_UNLIKELY_FAIL;
	}

	HMODULE lib = LoadLibraryW(dll);
	if (!lib) {
		wxlog("%s LoadLibraryW(dll) error", __FUNCTION__);
		return INJECT_ERROR_UNLIKELY_FAIL;
	}

#ifdef _WIN64
	LPVOID proc = GetProcAddress(lib, "dummy_debug_proc");
#else
	LPVOID proc = GetProcAddress(lib, "_dummy_debug_proc@12");
#endif

	if (!proc) {
		wxlog("%s GetProcAddress(lib, \"dummy_debug_proc\") error", __FUNCTION__);
		return INJECT_ERROR_UNLIKELY_FAIL;
	}

	set_windows_hook_ex_t set_windows_hook_ex = get_obfuscated_func(user32,set_windows_hook_ex_obf, obf1);

	HHOOK hook = set_windows_hook_ex(WH_GETMESSAGE, proc, lib, thread_id);
	if (!hook) {
		wxlog("%s set_windows_hook_ex error", __FUNCTION__);
		return GetLastError();
	}

	/* SetWindowsHookEx does not inject the library in to the target
	* process unless the event associated with it has occurred, so
	* repeatedly send the hook message to start the hook at small
	* intervals to signal to SetWindowsHookEx to process the message and
	* therefore inject the library in to the target process.  Repeating
	* this is mostly just a precaution. */

	for (size_t i = 0; i < RETRY_COUNT; i++) {
		Sleep(RETRY_INTERVAL_MS);
		PostThreadMessage(thread_id, WM_USER + 432, 0, (LPARAM)hook);
	}
	return 0;
}