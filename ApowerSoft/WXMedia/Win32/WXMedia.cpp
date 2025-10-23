/*

WXMedia.dll 初始化操作

*/
#include "WXMediaCpp.h"

#include <SDL2/SDL.h>
#ifdef _WIN32
#include <d3d11.h>

#pragma comment(lib, "libffmpeg.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "strmiids.lib") //DirectShow

//#pragma comment(lib, "libyuv.lib")
#pragma comment(lib, "libjpeg.lib")


#include <mfapi.h>    // Media Foundation API
#include <mfidl.h>    // MF interfaces
#include <mfobjects.h>
#include <mfplay.h>
#include <mfreadwrite.h>
#include <mferror.h>  // Media Foundation 错误代码

/*extern*/ WXLocker s_lockGlobal;

std::wstring GetFullPathW(const wchar_t* filename) {
	std::wstring fullpath = L"";
	if (WXBase::Exists(filename)) { //相对路径
		std::filesystem::path abs_path = std::filesystem::absolute(filename);
		fullpath = abs_path.wstring(); //当前路径
		return fullpath;
	}


	//初始运行路径
	// 获取当前运行目录
	std::wstring s_strRunPath = WXBase::GetRunPath();
	fullpath = s_strRunPath + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//
	fullpath = s_strRunPath + L"\\avs\\scripts\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//
	fullpath = s_strRunPath + L"\\trans\\avs\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//Medialib.dll/WXMedia.dll 所在目录
	std::wstring s_strDllDir = WXBase::GetDllPath();
	fullpath = s_strDllDir + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//
	fullpath = s_strDllDir + L"\\avs\\scripts\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//
	fullpath = s_strDllDir + L"\\trans\\avs\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	//在ProgramData 的路径
	wchar_t wszProgData[MAX_PATH] = { 0 };
	WXGetGlobalString(L"ProgramData", wszProgData, L""); //ProgramData 路径
	std::wstring s_strProgData = wszProgData;
	fullpath = s_strProgData + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	fullpath = s_strProgData + L"\\avs\\scripts\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	fullpath = s_strProgData + L"\\trans\\avs\\" + std::wstring(filename);
	if (WXBase::Exists(fullpath)) {
		return fullpath;
	}

	if (fullpath.length() == 0) {
		//::MessageBoxW(NULL, filename, L"File or Dir No Found!", MB_OK);
	}
	return L"";
}

std::string  GetFullPathA(const char* filename)
{
	std::wstring wstr = WXBase::UTF8ToUTF16(filename);
	std::wstring fullpath = GetFullPathW(wstr.c_str());
	return WXBase::UTF16ToUTF8(fullpath.c_str());
}

bool s_bFfmpegExe = false;//是否可以调用ffmpeg.exe
static std::wstring s_strFfmpegExe = L"ffmpeg.exe";
//EXTERN_C CrashCallBack crashcallback = NULL;
// 处理Unhandled Exception的回调函数
static DWORD ExecuteProcess(std::wstring exepath, DWORD dwTimeOut)
{
	WXLogW(L"%ws %ws", __FUNCTIONW__, exepath.c_str());

	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	// 设置不显示窗体
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	const wchar_t* data = exepath.c_str();
	int ret = ::CreateProcess(exepath.c_str(), NULL, 0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);

	// 等待进程结束
	DWORD result = WaitForSingleObject(pi.hProcess, dwTimeOut /*INFINITE*/);
	switch (result) {
	case WAIT_OBJECT_0: // 进程已经退出
		//	WXLogW(L"The process has exited ." );
		break;
	case WAIT_TIMEOUT:// 等待超时
		WXLogW(L"The waiting time has exceeded.");
		break;
	case WAIT_FAILED:// 等待失败
		WXLogW(L"WaitForSingleObject failed: %ul ", GetLastError());
		break;
	default:
		WXLogW(L"Unexpected result from WaitForSingleObject: %ul", result);
		break;
	}
	// 关闭进程和线程句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return result;
}

DWORD ExecuteFfmpegExe(const wchar_t* wszArg)
{
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	// 设置不显示窗体
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	wchar_t data[4096];
	wcscpy_s(data, wszArg);

	int ret = ::CreateProcess(s_strFfmpegExe.c_str(), data, 0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE, 0, 0, &si, &pi);

	// 等待进程结束
	DWORD result = WaitForSingleObject(pi.hProcess, INFINITE);
	switch (result) {
	case WAIT_OBJECT_0: // 进程已经退出
		//WXLogW(L"ffmpegx.exe process has exited.");
		break;
	case WAIT_TIMEOUT:// 等待超时
		WXLogW(L"ffmpegx.exe waiting time has exceeded .");
		break;
	case WAIT_FAILED:// 等待失败
		WXLogW(L"ffmpegx.exe WaitForSingleObject failed: %u", GetLastError());
		break;
	default:
		WXLogW(L"Unexpected result from WaitForSingleObject: %d", (int)result);
		break;
	}
	// 关闭进程和线程句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return result;
}


// 定义 OSVERSIONINFOEX 结构体用于存储版本信息
typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)



OSVERSIONINFO g_osInfo; //extern in  LavFilter  

//工作线程
//工作线程
class WorkThread
{
#define MAX_TASK 2
	ThreadSafeQueue<WXTask>m_queueTask[MAX_TASK];//消息队列
	std::shared_ptr<std::thread> m_Thread[MAX_TASK];//线程
	std::atomic_bool m_bRun = false;//线程运行标志
	void ThreadProc(int index) {//线程函数
		while (m_bRun.load()) {
			WXTask task = m_queueTask[index].Pop();
			if (task)
				task();
			else
				SLEEPMS(1);
		}
	}
	void ThreadCreate(int index) {
		m_Thread[index] = std::make_shared<std::thread>([this, index]() {
			ThreadProc(index);
		});
	}

public:


	static WorkThread& GetInst() {
		static WorkThread s_WorkThread;
		return s_WorkThread;
	}

public:
	void Post(int index, WXTask task) {
		m_queueTask[index].Push(task);
	}
	WorkThread() {
		m_bRun.store(true);
		for (int i = 0; i < MAX_TASK; i++) {
			ThreadCreate(i);
		}
	}
	virtual ~WorkThread() {
		m_bRun.store(false);
		for (int i = 0; i < MAX_TASK; i++) {
			if (m_Thread[i]) {
				m_Thread[i]->join();
				m_Thread[i] = nullptr;
			}
		}
		exit(-1);
		//return;
	}

};

//异步执行
WXMEDIA_API void WXTaskPost(int index, WXTask task) {
	WorkThread::GetInst().Post(index, task);
}

//获取总内存 GB
static int WXGetMemory() {
	BEGIN_LOG_FUNC
	int nMemory = WXGetGlobalValue( L"Memory", -1);
	if (nMemory < 0) {
		nMemory = 2;
		MEMORYSTATUSEX statusex;
		statusex.dwLength = sizeof(statusex);
		if (::GlobalMemoryStatusEx(&statusex)) {
			nMemory = (int)(statusex.ullTotalPhys / 1024.0 / 1024.0 / 1024.0 + 0.5);
		}
		WXLogW(L"Memory = %d G ", nMemory);
		WXSetGlobalValue( L"Memory", nMemory);

	}
	WXSetGlobalValue(L"Memory", nMemory);
	return nMemory;
}


static int WXGetCpuNum() {
	BEGIN_LOG_FUNC
	int nCpu = WXGetGlobalValue(L"Cpu",-1);
	if (nCpu < 0) {
		SYSTEM_INFO sysinfo;
		::GetSystemInfo(&sysinfo);
		nCpu = (int)sysinfo.dwNumberOfProcessors;
		WXLogW(L"Cpu Number = %d", nCpu);
		WXSetGlobalValue( L"Cpu", nCpu);
	}
	WXSetGlobalValue(L"Cpu", nCpu);
	return nCpu;
}

static int WXGetCpuSpeed() {
	BEGIN_LOG_FUNC
	int nCpuSpeed = WXGetGlobalValue( L"CpuSpeed", -1);
	if (nCpuSpeed < 0) {
		LONG result = 0;
		HKEY hKey;
		DWORD dwSpeed;
		BYTE tmp[4] = { 0 };
		DWORD dataSize = 4;
		//打开注册表
		result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			_T("Hardware\\Description\\System\\CentralProcessor\\0"),
			0, KEY_QUERY_VALUE, &hKey);
		// 获取CPU的处理速度
		if (result == ERROR_SUCCESS) {
			result = ::RegQueryValueExW(hKey, _T("~MHz"),
				NULL, NULL, tmp, &dataSize);
			memcpy(&dwSpeed, tmp, 4);
			nCpuSpeed = (int)dwSpeed;
		}
		::RegCloseKey(hKey);//关闭注册表

		WXLogW(L"Cpu Speed = %d MHz", nCpuSpeed);
		WXSetGlobalValue( L"CpuSpeed", nCpuSpeed);
	}
	WXSetGlobalValue(L"CpuSpeed", nCpuSpeed);
	return nCpuSpeed;
}

// 判断文件是否存在
WXMEDIA_API int WXIsFileExist(WXCTSTR wszFile)
{
	DWORD dwAttrib = GetFileAttributes(wszFile);
	return INVALID_FILE_ATTRIBUTES != dwAttrib && 0 == (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

static void ParseIni() {

	WXSetGlobalString( _T("Notify"), _T("\"WXMedia.dll Param Config\""));

	int bCheckDXGI = WXGetGlobalValue( L"SupportDXGI", -1);
	if (bCheckDXGI == -1) {
		WXSetGlobalValue(_T("SupportDXGI"), WXGetSystemVersion() != 7);
	}

	int bSupportD3DX = WXGetGlobalValue( L"SupportD3DX", -1);
	if (bSupportD3DX == -1) {
		WXSetGlobalValue(_T("SupportD3DX"), 1);
	}

	int bSupportD3D = WXGetGlobalValue(L"SupportD3D", -1);
	if (bSupportD3D == -1) {
		WXSetGlobalValue(_T("SupportD3D"), 1);
	}

	int nQSV = WXGetGlobalValue( L"Support_QSV_Encoder", -1);
	if (nQSV == -1) {
		WXSetGlobalValue( _T("Support_QSV_Encoder"), 1);
	}
}

#endif


EXTERN_C  void  log_callback(void* ptr, int level, const char* fmt, va_list vl) {
	//printf("AV Log\n");
}

extern void WXGameInit();//游戏录制
WXString g_strLog;
static int s_bDeviceInit = 0;


WXMEDIA_API void WXDeviceInitMirror(WXCTSTR logfile) {
	WXDeviceInit(logfile);
}


WXMEDIA_API void WXDeviceDeinit() {
	WXAutoLock al(s_lockGlobal);
	//关闭摄像头
	WXLogA("%s", __FUNCTION__);
#ifdef _WIN32
	void* camera = WXCameraGetCurrDevice(); //摄像头还活着！！
	if (camera) {
		WXCameraClose(camera);
		camera = 0;
	}
	AudioDeviceClose(1);
	AudioDeviceClose(0);
#endif
}

WXMEDIA_API void WXDeviceDeinitMirror() {
	WXAutoLock al(s_lockGlobal);
}

/*
获取本机操作系统的版本号，6为XP，7为Win7，8为Win8.0,9为Win8.1，10为Win10
*/
#ifdef _WIN32


#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.System.h>

static WXLocker s_lckDXGI;
//获得系统版本号
WXMEDIA_API int  WXGetSystemVersion() {

	WXAutoLock al(s_lckDXGI);
	BEGIN_LOG_FUNC

	int nSystemVersion = WXGetGlobalValue(L"SystemVersion", -1);
	if (nSystemVersion > 0)
		return nSystemVersion;

	WXSetGlobalValue(L"SystemVersion", 0);//默认版本号
	WXSetGlobalValue(L"1903", 0);//是否Win10 1903
	WXSetGlobalValue(L"WGC", 0);//是否Win10 1903

	int s_system_version = 0;
	int s_b1903 = 0;
	int s_bWGC = 0;//是否支持WGC采集

	//先判断是否为win8.1或win10  
	if (LibInst::GetInst().m_libNTDLL) {
		RTL_OSVERSIONINFOW info = { 0 };
		info.dwOSVersionInfoSize = sizeof(info);

		if (LibInst::GetInst().m_RtlGetVersion) {
			// 检查是否大于版本 1803
			LibInst::GetInst().m_RtlGetVersion(&info);
			if (info.dwMajorVersion > 10) {
				s_b1903 = 1;
				s_system_version = 10;
			}
			else if (info.dwMajorVersion == 10) {
				// 检查次版本
				s_b1903 = (info.dwMinorVersion > 0 ||   // Windows 10 没有使用次版本号
					(info.dwMinorVersion == 0 && info.dwBuildNumber > 18362)); // 1903 的内部版本号是 18362
			}
		}

		//HRESULT hrMF = E_FAIL;
		//if (s_b1903) {
		//	hrMF = MFStartup(MF_VERSION);
		//}
		WXSetGlobalValue(L"1903", s_b1903);
		if (s_b1903/* && SUCCEEDED(hrMF)*/) {
			try {
				/* no contract for IGraphicsCaptureItemInterop, verify 10.0.18362.0 */
				s_bWGC = winrt::Windows::Foundation::Metadata::ApiInformation::
					IsApiContractPresent(L"Windows.Foundation.UniversalApiContract",
						8);
			}
			catch (...) {
				s_bWGC = 0;
			}
		}

		if (!s_b1903) {
			if (LibInst::GetInst().m_RtlGetNtVersionNumbers) {
				DWORD dwMajor, dwMinor, dwBuildNumber;
				LibInst::GetInst().m_RtlGetNtVersionNumbers(&dwMajor, &dwMinor, &dwBuildNumber);
				if (dwMajor == 5) {
					s_system_version = 6; //XP
				}
				if (dwMajor == 6 && dwMinor == 1) {
					s_system_version = 7;//Win7
				}
				if (dwMajor == 6 && dwMinor == 2) {
					s_system_version = 8;//win 8.0
				}
				if (dwMajor == 6 && dwMinor == 3) {
					s_system_version = 9;//win 8.1
				}
				if (dwMajor >= 10) {
					s_system_version = 10;//win 10  
				}
			}
		}
		else {
			s_system_version = 10;//Win10
		}
	}

	g_osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionExW(&g_osInfo);

	if (s_system_version == 0) {
		//判断win8.1以下的版本  
		if (g_osInfo.dwMinorVersion == 5)
			s_system_version = 6;
		if (g_osInfo.dwMinorVersion >= 6)
				s_system_version = 7;//Vista Win7 ...
	}
	WXSetGlobalValue(L"SystemVersion", s_system_version);//默认版本号
	WXSetGlobalValue(L"1903", s_b1903);//是否Win10 1903
	WXSetGlobalValue(L"WGC", s_bWGC);//是否支持WGC



	return s_system_version;
}

//检测是否支持DXGI采集
WXMEDIA_API int  WXSupportDXGI() {

	WXAutoLock al(s_lckDXGI);
	BEGIN_LOG_FUNC

	int nDXGI = WXGetGlobalValue(L"DXGI", -1);
	if (nDXGI > 0) {
		return 1;
	}
	else if (nDXGI == 0) {
		return 0;
	}
	//nDXGI==-1
	WXSetGlobalValue(L"DXGI", 0);
	WXSetGlobalValue(L"DXGI", 0);

	//是否强制设置不采集DXGI
	int g_bCheckDXGI = WXGetGlobalValue(L"SupportDXGI", 1);
	if (!g_bCheckDXGI) {
		return 0;
	}

	if (WXGetSystemVersion() == 7|| 
		LibInst::GetInst().m_libD3D11 == nullptr ||
		LibInst::GetInst().m_libDXGI == nullptr) {
		return 0;
	}

	D3D_FEATURE_LEVEL FeatureLevel;

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[3] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = 3;

	D3D_FEATURE_LEVEL FeatureLevels[4] = {
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = 4;

	HRESULT hr = S_OK;

	CComPtr <ID3D11Device>pDevice = nullptr;
	CComPtr<ID3D11DeviceContext>pImmediateContext = nullptr;

	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
		hr = LibInst::GetInst().mD3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &pDevice, &FeatureLevel, &pImmediateContext);
		if (SUCCEEDED(hr)) {
			break;
		}
	}
	if (FAILED(hr)) {
		WXLogW(L"\tD3D11CreateDevice DX Error[%x]", hr);
		return 0;
	}

	// Get DXGI device
	CComPtr<IDXGIDevice>DxgiDevice = nullptr;
	hr = pDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
	if (FAILED(hr)) {
		WXLogW(L"\tQueryInterface(IDXGIDevice) DX Error[%x]", hr);
		return 0;
	}

	// Get DXGI adapter
	CComPtr<IDXGIAdapter>DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	if (FAILED(hr)) {
		WXLogW(L"\tGetParent(IDXGIAdapter) DX Error[%x]", hr);
		return 0;
	}

	// Get output
	CComPtr<IDXGIOutput>DxgiOutput = nullptr;
	hr = DxgiAdapter->EnumOutputs(0, &DxgiOutput);//0 是否表示第一个显示器
	if (FAILED(hr)) {
		WXLogW(L"\tEnumOutputs DX Error[%x]", hr);
		return 0;
	}

	// QI for Output 1
	CComPtr<IDXGIOutput1>DxgiOutput1 = nullptr;
	hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	if (FAILED(hr)) {
		WXLogW(L"QueryInterface(IDXGIOutput1) DX Error[%x]", hr);
		return 0;
	}
	CComPtr < IDXGIOutputDuplication>ppOutputDuplication = nullptr;
	hr = DxgiOutput1->DuplicateOutput(pDevice, &ppOutputDuplication);//固定对象
	if (FAILED(hr)) {
		WXLogW(L"\tDuplicateOutput DX Error[%x]", hr);
		return 0;
	}

	WXLogW(L"[%ws]Support DXGI Capture!!!!!",__FUNCTIONW__);
	WXSetGlobalValue(L"DXGI", 1);
	return 1;
}

/*
获得WXMedia.dll 版本号
也就是文件详细信息里面 1.0.0.xxx 里面xxx
*/
static BOOL VersionMsg(LPCTSTR strFile, WXString& strVersion)
{
	if (LibInst::GetInst().m_libVersion == nullptr) {
		strVersion.Format("0.0.0.0");
		return FALSE;
	}
	
	TCHAR szVersionBuffer[1000] = _T("");
	DWORD dwVerSize;
	DWORD dwHandle;

	dwVerSize = LibInst::GetInst().mGetFileVersionInfoSizeW(strFile, &dwHandle);
	if (dwVerSize == 0)
		return FALSE;

	if (LibInst::GetInst().mGetFileVersionInfoW(strFile, 0, dwVerSize, szVersionBuffer))
	{
		VS_FIXEDFILEINFO* pInfo;
		unsigned int nInfoLen;

		if (LibInst::GetInst().mVerQueryValueW(szVersionBuffer, _T("\\"), (void**)&pInfo, &nInfoLen))
		{
			strVersion.Format(("%d.%d.%d.%d"),
				HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS),
				HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));
			return TRUE;
		}
	}

	return TRUE;
}
#endif



WXMEDIA_API void WXSetCrashDumpFlag(int bEnable) {
	WXSetGlobalValue(L"DumpUse", !!bEnable);
}

WXMEDIA_API void WXSetCrashDumpFile(WXCTSTR strFile) {
	if (nullptr != strFile) {
		WXSetCrashDumpFlag(TRUE);
		WXSetGlobalString(L"DumpFile", strFile);
	}
	else {
		WXSetCrashDumpFlag(TRUE);
		WXSetGlobalString(L"DumpFile", L"");
	}
}


WXMEDIA_API void SetDumpCallBackExe(WXCTSTR strExeFile, WXCTSTR strExeParam) {
	if (nullptr != strExeFile && WXStrlen(strExeFile) > 0) {
		//s_bCallBackExe = TRUE;
		WXSetGlobalValue(L"DumpCallBack", TRUE);
		WXSetGlobalString(L"DumpCallBackExe", strExeFile);
		if (nullptr != strExeParam && WXStrlen(strExeParam) > 0) {
			WXSetGlobalString(L"DumpCallBackParam", strExeParam);
		}
	}
	else {
		//s_bCallBackExe = FALSE;
		WXSetGlobalValue(L"DumpCallBack", FALSE);
		WXSetGlobalString(L"DumpCallBackExe", L"");
		WXSetGlobalString(L"DumpCallBackParam", L"");
	}
}

#ifdef _WIN32
static LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
	if (LibInst::GetInst().m_libDbghelp == nullptr)
		return 0;
	wchar_t wszDumpPath[MAX_PATH] = { 0 };
	//创建Dump文件
	WXGetGlobalString(L"DumpFile", wszDumpPath, L"");
	std::wstring strDump = wszDumpPath;
	if (strDump.length()) {// 是否创建Dump文件 
		HANDLE hDumpFile = CreateFile(strDump.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hDumpFile) {
			WXLogA("Create Dump File %s Create Dump File [%s]", __FUNCTION__, strDump.c_str());
			// Dump信息 
			MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
			dumpInfo.ExceptionPointers = pException;
			dumpInfo.ThreadId = GetCurrentThreadId();
			dumpInfo.ClientPointers = TRUE;

			// 写入Dump文件内容  
			LibInst::GetInst().mMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
				hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
			CloseHandle(hDumpFile);
		}
	}

	//生成Dump后执行的外部EXE，BugReports
	if (WXGetGlobalValue(L"DumpCallBack",0)) {

		BOOL bHasParam = FALSE;
		wchar_t wszDumpParam[MAX_PATH] = {0};
		WXGetGlobalString(L"DumpCallBackParam", wszDumpParam, L"");

		if (wcslen(wszDumpParam) > 0) {
			bHasParam = TRUE;
		}
		PROCESS_INFORMATION pi = { 0 };
		STARTUPINFO si = { 0 };
		si.cb = sizeof(si);
		BOOL ret = ::CreateProcessW(strDump.c_str(),
			bHasParam ? wszDumpParam : NULL,
			NULL, NULL, false, 
			CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		WXLogW(L"++++ %ws __ \"%ws%ws\" = %d", __FUNCTIONW__, strDump.c_str(), bHasParam ? wszDumpParam : L"NULL", ret);
		if (ret) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
		}
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif


std::vector<WXString>g_arrGpuName;
void ListGpuName() {
	BEGIN_LOG_FUNC
	g_arrGpuName.clear();

	if (LibInst::GetInst().m_libDXGI == nullptr)
		return;

	int nGPU = WXGetGlobalValue( L"Gpu", -1);
	if (nGPU > 0) {
		//从Ini读取GPU数量和名字
		for (int i = 0; i < nGPU; i++)
		{
			WXString strGpuName;
			strGpuName.Format(L"Gpu-%d", i);
			wchar_t buffer[MAX_PATH];
			WXGetGlobalString(strGpuName.str(), buffer,L"");
			WXString str = buffer;
			g_arrGpuName.push_back(str);
		}
		return;
	}

	nGPU = 0;

	// 参数定义  
	CComPtr<IDXGIFactory>pFactory = nullptr;

	//创建一个DXGI工厂 (必须用CreateDXGIFactory1，如果用CreateDXGIFactory只能取出一块显卡信息) 
	HRESULT hr = LibInst::GetInst().mCreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)(&pFactory));
	if (FAILED(hr)) {
		WXLogW(L"%ws CreateDXGIFactory1 Error", __FUNCTION__);
		return;
	}

	CComPtr<IDXGIAdapter>pAdapter = nullptr;
	int iIndex = 0; // 显卡的数量 
	int iAdapterNum = 0; // 显卡的数量 
	// 枚举适配器  
	while (pFactory->EnumAdapters(iIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
		// 获取信息  
		DXGI_ADAPTER_DESC adapterDesc;
		pAdapter->GetDesc(&adapterDesc);
		if (wcsicmp(L"Microsoft Basic Render Driver", adapterDesc.Description) != 0) {
			WXString strName = adapterDesc.Description;
			int bExist = 0;
			for (int i = 0; i < g_arrGpuName.size(); i++){
				if (wcsicmp(g_arrGpuName[i].str(), strName.str()) == 0) {
					bExist = 1;
					break;
				}
			}
			if (!bExist) {
				g_arrGpuName.push_back(strName);
				iAdapterNum++;
			}
		}
		iIndex++;
		pAdapter = nullptr;
	}
	nGPU = g_arrGpuName.size();
	WXSetGlobalValue(L"Gpu", nGPU);
	for (int i = 0; i < nGPU; i++)
	{ 
		WXString strGpuName;
		strGpuName.Format(L"Gpu-%d", i);
		WXSetGlobalString(strGpuName.str(), g_arrGpuName[i].str());
	}
	//WXLogW(L"%ws GPU = %d", __FUNCTIONW__, iAdapterNum);
}


WXMEDIA_API void WXUtilsInit(WXCTSTR logfile) {
	return;
}

extern "C" {
	extern AVInputFormat  ff_wxm_demuxer; //基于FLV的修改
	//extern AVOutputFormat ff_wxm_muxer;
}

static const AVInputFormat*  const in_lists[3] = {
	& ff_wxm_demuxer ,
	NULL
};

static const AVOutputFormat* const out_lists[3] = {
	//& ff_wxm_muxer ,
	NULL
};


static void WINAPI WX_LogW(const wchar_t* format, va_list args) {
	wchar_t wszMsg[4096];
	memset(wszMsg, 0, 4096 * 2);
	vswprintf(wszMsg, format, args);
	WXLogW(L"%ws", wszMsg);
}
WXString g_strJsonPath = L"WXCamera.json";//Camera.json

WXMEDIA_API void WXDeviceInit(WXCTSTR logfile) {

	WXAutoLock al(s_lockGlobal);
	BEGIN_LOG_FUNC  //WXDeviceInit

	if (logfile == nullptr)
		return;

	if (s_bDeviceInit == 1) {
		return;//只能初始化一次
	}
	s_bDeviceInit = 1;

	g_strLog = logfile;

	//ibInst:LInit();
	timeBeginPeriod(1); //设置Sleep的精度为1ms
	SetDllDirectory(L"");
	HRESULT hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM 初始化

	std::wstring s_LogPath = L"";//log目录
	//获取INI绝对路径
	std::wstring s_strLog = logfile;//log路径
	std::wstring s_strLogPath = L"";//log目录
	size_t pos = s_strLog.find(L":");
	std::wstring s_ExePath = WXBase::GetExePath();//EXE目录
	if (pos == std::wstring::npos) { //不是全路径
		s_strLog = s_ExePath.c_str();
		s_LogPath = s_ExePath;
	}
	else { //全路径
		std::filesystem::path path1(logfile);
		s_LogPath = path1.parent_path().wstring() + L"\\";
	}
	std::wstring strIniPath = s_LogPath.c_str();
	strIniPath += L"WXMedia.ini";//INI 路径
	WXSetGlobalPath(strIniPath.c_str());

	WXSetGlobalValue(L"MachLevel", LEVEL_BETTER);//默认机器类型一般

	WXString strDllVersion;
	VersionMsg(L"WXMedia.dll", strDllVersion);
	WXSetGlobalString(L"DllVersion", strDllVersion.str());

	std::wstring s_RunPath = WXBase::GetRunPath();  //当前运行目录
	WXSetGlobalString(L"RunPath", s_RunPath.c_str());

	std::wstring s_FullExeName = WXBase::GetFullExeName(); //当前EXE全路径
	WXSetGlobalString(L"FullExeName", s_FullExeName.c_str());


	WXSetGlobalString(L"ExePath", s_ExePath.c_str());

	std::wstring s_ExeName = WXBase::GetExeName();//EXE名字
	WXSetGlobalString(L"ExeName", s_ExeName.c_str());


	g_strJsonPath = s_LogPath.c_str();
	g_strJsonPath += L"WXCamera.json";//JSON 路径

	std::wstring strLogFileName = s_strLog;
	strLogFileName += s_ExeName.c_str();

	std::wstring strDumpFile = strLogFileName.c_str();
	strDumpFile += strDllVersion.str();
	strDumpFile += L".dmp";  //DUMP文件

	strLogFileName += L".WXMedia.";//WXMedia.dll
	strLogFileName += strDllVersion.str();//版本号
	strLogFileName += L".log";   //log文件

	WXLogInit(strLogFileName.c_str());

	//WXLogA("%s Begin =================== ", __FUNCTION__);
	LibInst_LogMsg(WX_LogW);
	WXLogA("%s %d",__FUNCTION__,__LINE__);
	//注册自定义WXM格式处理
	avcodec_register_all();
	avfilter_register_all();
	av_register_all();
	avformat_network_init();
	av_log_set_callback(log_callback);
	av_register_input_format(&ff_wxm_demuxer);
	//av_register_output_format(&ff_wxm_muxer);
	avpriv_register_devices(out_lists, in_lists);

	//播放器使用
	//SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	//WXLogA("%s %d", __FUNCTION__, __LINE__);

	ParseIni();//解析INI文件

	WXLogA("%s %d", __FUNCTION__, __LINE__);

	//WXMedia 创建窗口
	WNDCLASS wc = { 0 };
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.lpszClassName = WXMEDIA_WNDCLASS;
	ATOM at = ::RegisterClass(&wc);


	WXLogA("%s %d", __FUNCTION__, __LINE__);

	// 设置处理Unhandled Exception的回调函数 
	WXLogW(L"--- Start DUMP s_strDumpFile=%ws", strDumpFile.c_str());
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
	WXSetGlobalString(L"DumpFile", strDumpFile.c_str());//默认DMP文件名

	//GDI+初始化
	static ULONG_PTR m_gdiplusToken = 0;
	if (m_gdiplusToken == 0) {
		Gdiplus::GdiplusStartupInput StartupInput;
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);
	}

	//音频设备初始化
	WXWasapiNotifyInit();
	WXWasapiInit();

	//枚举显示器
	WXScreenInit();
	
	//----------------------------------------
	std::thread thBase([] {
		//读取和检测常用数据
		WXGetCpuNum();
		WXGetMemory();
		WXGetCpuSpeed();
		ListGpuName();
	});
	thBase.detach();

	std::thread thDXGI([] {
		WXSupportDXGI();//检测是否支持DXGI加速采集  OK
	});
	thDXGI.detach();


	std::thread thQSV([] {
		WXSupportH264Codec();//硬编码检测
		WXSupportH265Codec();//硬编码检测
		});
	thQSV.detach();

	std::thread thCamera([] {
		WXCameraInit();//Patch for Camera Init  OK
	});
	thCamera.detach();
	//WXGameInit();//游戏录制功能


	//ffmpeg.exe 检测
	std::thread thFfmpeg([] {

		s_strFfmpegExe = GetFullPathW(L"ffmpeg.exe");
		int getExe = WXGetGlobalValue(L"ffmpeg_exist", -1);
		if (getExe == -1)
		{
			DWORD dwTimeOut = 10000;
			DWORD result = ExecuteProcess(s_strFfmpegExe, dwTimeOut);
			if (result == WAIT_OBJECT_0) {
				s_bFfmpegExe = true;
				WXSetGlobalValue(L"ffmpeg_exist", 1);
				WXSetGlobalString(L"ffmpeg_path", s_strFfmpegExe.c_str());
				WXLogW(L"%ws is exist!", s_strFfmpegExe.c_str());
			}
			else {
				WXSetGlobalValue(L"ffmpeg_exist", 0);
				WXSetGlobalString(L"ffmpeg_path", L"");
				s_bFfmpegExe = false;
				WXLogW(L"%ws is not exist!", s_strFfmpegExe.c_str());
			}
		}
		else {
			s_bFfmpegExe = getExe;
		}
		});
	thFfmpeg.detach();

	// 程序初始化调用
	WXSetGlobalValue(L"MediaPlayer", MEDIAPLAYER_LAV); // 强制使用LAV播放器
}



#ifdef _WIN32

//static WXString s_strLocalName = L"";
//static WXString s_strLocalUserName = L"";
//static std::vector<WXString>s_arrIP;
//WXMEDIA_API WXCTSTR WXGetLocalName() {
//	return s_strLocalName.str();
//}
//
//WXMEDIA_API WXCTSTR WXGetLocalUserName() {
//	return s_strLocalUserName.str();
//}
//
//
//WXMEDIA_API int   WXNetworkGetIPCount() {
//	return (int)s_arrIP.size();
//}
//
//WXMEDIA_API WXCTSTR WXNetworkGetIPAddr(int index) {
//	if (s_arrIP.size() == 0)
//		return nullptr;
//	if (index < 0 || index >= s_arrIP.size())
//		return s_arrIP[0].str();
//	return s_arrIP[index].str();
//}
//
//static BOOL WXMediaInitNetwork() {
//	WSADATA wsaData;
//	BOOL ret = FALSE;
//	ret = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
//	if (ret != 0) {
//		return FALSE;
//	}
//
//
//	WCHAR abyUserName[MAX_PATH] = { 0 };
//	DWORD dwSize = MAX_PATH;
//	::GetUserName(abyUserName, &dwSize);
//	s_strLocalUserName = abyUserName;
//
//	// 获取机器名字和本机IP地址
//	char szDevName[MAX_PATH];
//	::gethostname(szDevName, MAX_PATH);//机器名字
//	struct hostent* phe = ::gethostbyname(szDevName);
//	if (NULL != phe) {
//		s_strLocalName.Format(szDevName);
//		char** ppAddr = phe->h_addr_list;
//		for (; *ppAddr != NULL; ppAddr++) {
//			const char* szIP = inet_ntoa(*(LPIN_ADDR) * (ppAddr));
//			WXString strIP;
//			strIP.Format(szIP);
//			s_arrIP.push_back(strIP);
//		}
//	}
//	return TRUE;
//}

HINSTANCE g_hInst = nullptr;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	g_hInst = (HINSTANCE)hModule;
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		//这个函数使用了其他dll， 容易造成临界区死锁， 迁移到其他init函数中
		//WXMediaInitNetwork();
		break;
	case DLL_PROCESS_DETACH:
		timeEndPeriod(1);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return (TRUE);
}
#endif







