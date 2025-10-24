#include "WXLog.h"
#include "WXBase.h"
//日志类
class WXLogInstance {
	WXLocker m_mutex;
	WXString m_strFileName;
	std::ofstream m_fout;
public:
	WXLocker s_lockLog;//日志锁
public:
	int s_bInitLog = 0;
	bool s_bCreateFile = false;
	//限制程序一次运行的最大日志数量，避免日志过多
	int64_t s_nMaxLog = 20000;//程序运行中最多的日志数量
	int64_t s_nCountLog = 0;//log 数量
public:
	static WXLogInstance& GetInst() {
		static WXLogInstance s_inst;
		return s_inst;
	}
	WXLogInstance() {}

	virtual ~WXLogInstance() {
		Close();
		exit(-1);
	}

	void Close() {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.close();
		}
	}

	bool Open(const wchar_t* strFileName) {
		WXAutoLock al(m_mutex);
		Close();

		m_strFileName = strFileName;
		int64_t filesize = WXBase::Filesize(m_strFileName.c_str());
		if (filesize > 1 * 1000 * 1000) {
			int rename2 = 1;
			while (1) {
				WXString wxstr;
				wxstr.Format(L"%ws.%d", strFileName, rename2);

				bool bExist = WXBase::Exists(wxstr.c_str());
				if (bExist) {
					rename2++;
					continue;
				}
				else {
					bExist = FALSE;
					RENAME(m_strFileName.c_str(), wxstr.c_str()); //
					break;
				}
			}
		}

		if (WXBase::Exists(m_strFileName.str())) {
			s_bCreateFile = false;
		}
		else {
			s_bCreateFile = true;//创建文件
		}

		m_fout.open(m_strFileName.str(), std::ios::app | std::ios::out | std::ios::binary);
		if (m_fout.is_open()) {
			if (s_bCreateFile) {
				uint8_t headText[2] = { 0xff, 0xfe };
				m_fout.write((const char*)headText, 2);
			}
			return true;
		}
		return m_fout.is_open();
	}

	void Write(const wchar_t* wszMsg) {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.write((const char*)wszMsg, sizeof(wchar_t) * wcslen(wszMsg));
			m_fout.flush();
		}
	}

	void Write(const char* szMsg) {
		WXAutoLock al(m_mutex);
		if (m_fout.is_open()) {
			m_fout.write(szMsg, sizeof(char) * strlen(szMsg));
			m_fout.flush();
		}
	}

public:
	void WXLogV(const char* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志	
			char    szMsg[4096];
			memset(szMsg, 0, 4096);
			vsprintf(szMsg, format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);
			wxstr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d - %s\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, szMsg);
			Write(wxstr.str());
		}
	}

	void WXLogV(const wchar_t* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志
			wchar_t wszMsg[4096];
			memset(wszMsg, 0, 4096 * 2);
			vswprintf(wszMsg,
#ifndef _WIN32
				4096,
#endif
				format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);

			wxstr.Format(L"%04d-%02d-%02d %02d:%02d:%02d %03d - %ws\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, wszMsg);
			Write(wxstr.w_str());
		}
	}

	void  WXLogVOnce(const char* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog && s_bCreateFile) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志	
			char    szMsg[4096];
			memset(szMsg, 0, 4096);
			vsprintf(szMsg, format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);
			wxstr.Format("%04d-%02d-%02d %02d:%02d:%02d %03d - %s\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, szMsg);
			Write(wxstr.str());
		}
	}

	void  WXLogVOnce(const wchar_t* format, va_list args) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog && s_bCreateFile) {
			s_nCountLog++;
			if (s_nCountLog >= s_nMaxLog) {
				s_bInitLog = false;//禁止写日志
				return;
			}
			//写入日志
			wchar_t wszMsg[4096];
			memset(wszMsg, 0, 4096 * 2);
			vswprintf(wszMsg,
#ifndef _WIN32
				4096,
#endif
				format, args);
			WXString wxstr;
			auto now = std::chrono::system_clock::now();
			//通过不同精度获取相差的毫秒数
			uint64_t dis_millseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
				- std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() * 1000;
			time_t tt = std::chrono::system_clock::to_time_t(now);
			auto time_tm = localtime(&tt);

			wxstr.Format(L"%04d-%02d-%02d %02d:%02d:%02d %03d - %ws\r\n",
				time_tm->tm_year + 1900,
				time_tm->tm_mon + 1, time_tm->tm_mday, time_tm->tm_hour,
				time_tm->tm_min, time_tm->tm_sec, (int)dis_millseconds, wszMsg);
			Write(wxstr.w_str());
		}
	}
public:

	int Init(const wchar_t* strFileName) {
		WXAutoLock al(s_lockLog);
		if (s_bInitLog)
			return 1;
		int bRet = Open(strFileName);
		s_bInitLog = bRet;
		return bRet;
	}
};


#include "LibInst.hpp"
std::shared_ptr<LibInst >s_LibInst = nullptr;

//设置日志文件
EXTERN_C int   WXLogInit(const wchar_t* strFileName) {
	return WXLogInstance::GetInst().Init(strFileName);
}

EXTERN_C void  WXLogAOnce(const char* format, ...) {
	va_list args = nullptr;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogVOnce(format, args);    // 调用内部处理函数
	va_end(args);
}

EXTERN_C void  WXLogWOnce(const wchar_t* format, ...) {
	va_list args = nullptr;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogVOnce(format, args);    // 调用内部处理函数
	va_end(args);
}

EXTERN_C void  WXLogA(const char* format, ...) {
	va_list args
#ifdef _WIN32
		= nullptr
#endif
		;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogV(format, args);    // 调用内部处理函数
	va_end(args);
}

EXTERN_C void  WXLogW(const wchar_t* format, ...) {
	va_list args
#ifdef _WIN32
		= nullptr
#endif
		;
	va_start(args, format);
	WXLogInstance::GetInst().WXLogV(format, args);    // 调用内部处理函数
	va_end(args);
}


//-----------------------  全局参数设置 ----------------------------

#ifdef _WIN32
#include <string>
#define WXTAG L"libffmpeg"
//ini配置
static std::wstring s_strIniPath = L"";
//设置ini路径，只能设置一次
EXTERN_C void WXSetGlobalPath(const wchar_t* wszPath) {
	if (s_strIniPath.length() == 0) {
		s_strIniPath = wszPath;
	}
}

//配置ini的数值
EXTERN_C void WXSetGlobalValue(const wchar_t* wszKey, int nValue) {
	std::wstring strValue = std::to_wstring(nValue);
	::WritePrivateProfileStringW(WXTAG, wszKey, strValue.c_str(), s_strIniPath.c_str());
}

//配置ini的字符串
EXTERN_C void WXSetGlobalString(const wchar_t* wszKey, const wchar_t* strValue) {
	::WritePrivateProfileStringW(WXTAG, wszKey, strValue, s_strIniPath.c_str());
}

//从ini获取数值,不存在则返回默认值
EXTERN_C int WXGetGlobalValue(const wchar_t* wszKey, int nDefValue) {
	return ::GetPrivateProfileIntW(WXTAG, wszKey, nDefValue, s_strIniPath.c_str());
}

//从ini获取字符串,不存在则返回默认值
//strRetValue 返回值, 默认长度MAX_PATH
//strDefValue 默认值
EXTERN_C void WXGetGlobalString(const wchar_t* wszKey, wchar_t* strRetValue, wchar_t* strDefValue) {
	::GetPrivateProfileStringW(WXTAG, wszKey, strDefValue, strRetValue, MAX_PATH, s_strIniPath.c_str());
}



#include <windows.h>
#include <gdiplus.h>

HINSTANCE g_hInst = nullptr;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	g_hInst = (HINSTANCE)hModule;
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		timeBeginPeriod(1);
		::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM 初始化
		static ULONG_PTR m_gdiplusToken = 0;
		if (m_gdiplusToken == 0) {
			Gdiplus::GdiplusStartupInput StartupInput;//GDI+初始化
			Gdiplus::GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);
		}
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



void LibInst::LogMsg() {
	if (m_logmsg == 0) {
		m_logmsg = 1;
		for (auto obj : m_arrLibs)
		{
			WXLogWOnce(L"LoadLibrary %ws Init", obj->GetNameW().c_str());
		}
	}
}
//各种库的加载
LibInst::LibInst() {
	LoadOLE32();//COM 初始化

	LoadOpenGLFunctions();//加载OpenGL库

	m_libCombase = std::shared_ptr<MyLib>(new MyLib(L"combase.dll"));
	if (m_libCombase->Enabled()) {
		m_arrLibs.push_back(m_libCombase.get());
		m_RoOriginateLanguageException = (PFNRoOriginateLanguageException)m_libCombase->GetFunction("RoOriginateLanguageException");
		m_RoGetActivationFactory = (PFNRoGetActivationFactory)m_libCombase->GetFunction("RoGetActivationFactory");
	}
	else {
		m_libCombase = nullptr;
	}

	//MNN Humanseg
	m_libHumanseg = std::shared_ptr<MyLib>(new MyLib(L"Humanseg.dll"));
	if (m_libHumanseg->Enabled()) {
		m_arrLibs.push_back(m_libHumanseg.get());
		mHumansegCreate = (funcHumansegCreate)m_libHumanseg->GetFunction("HumansegCreate");
		mHumansegDetect = (funcHumansegDetect)m_libHumanseg->GetFunction("HumansegDetect");
		mHumansegDestroy = (funcHumansegDestroy)m_libHumanseg->GetFunction("HumansegDestroy");
		mWXSupportVulkan = (funcWXSupportVulkan)m_libHumanseg->GetFunction("WXSupportVulkan");
	}
	else {
		m_libHumanseg = nullptr;
	}

	m_libD3D9 = std::shared_ptr<MyLib>(new MyLib(L"d3d9.dll"));
	if (m_libD3D9->Enabled()) {
		//D3D9.dll
		mDirect3DCreate9 = (cbDirect3DCreate9)m_libD3D9->GetFunction("Direct3DCreate9");
		mDirect3DCreate9Ex = (cbDirect3DCreate9Ex)m_libD3D9->GetFunction("Direct3DCreate9Ex");

		//来源于WXDXFilter.dll 显卡检测
		//如果 Direct3DCreate9Ex 无法正常返回
		//硬编码硬解码以及DXGI功能可能都会有问题 
		//需要升级显卡驱动
		CComPtr<IDirect3D9Ex> pD3DEx = nullptr;
		HRESULT hr = mDirect3DCreate9Ex(D3D_SDK_VERSION, &pD3DEx);
		if (FAILED(hr)) {
			mDirect3DCreate9 = nullptr;
			mDirect3DCreate9Ex = nullptr;
			m_libD3D9 = nullptr;
		}
		else {
			m_arrLibs.push_back(m_libD3D9.get());
		}
	}

	if (m_libD3D9) { //D3DX9 依赖于D3D9功能
		m_libD3DX9 = std::shared_ptr<MyLib>(new MyLib(L"d3dx9_43.dll"));
		//D3DX9.dll
		if (m_libD3DX9->Enabled()) {
			m_arrLibs.push_back(m_libD3DX9.get());
			mD3DXCompileShader = (cbD3DXCompileShader)m_libD3DX9->GetFunction("D3DXCompileShader");
			mD3DXCreateFontIndirectW = (cbD3DXCreateFontIndirectW)m_libD3DX9->GetFunction("D3DXCreateFontIndirectW");

			mD3DXMatrixPerspectiveFovLH = (cbD3DXMatrixPerspectiveFovLH)m_libD3DX9->GetFunction("D3DXMatrixPerspectiveFovLH");
			mD3DXGetShaderConstantTable = (cbD3DXGetShaderConstantTable)m_libD3DX9->GetFunction("D3DXGetShaderConstantTable");
			mD3DXCreateTexture = (cbD3DXCreateTexture)m_libD3DX9->GetFunction("D3DXCreateTexture");
			mD3DXCreateVolumeTexture = (cbD3DXCreateVolumeTexture)m_libD3DX9->GetFunction("D3DXCreateVolumeTexture");
		}
		else {
			m_libD3DX9 = nullptr;
		}
		m_libD3D11 = std::shared_ptr<MyLib>(new MyLib(L"d3d11.dll"));
		if (m_libD3D11->Enabled()) {
			m_arrLibs.push_back(m_libD3D11.get());
			mD3D11CreateDevice = (cbD3D11CreateDevice)m_libD3D11->GetFunction("D3D11CreateDevice");
			mCreateDirect3D11DeviceFromDXGIDevice = (cbCreateDirect3D11DeviceFromDXGIDevice)m_libD3D11->GetFunction("CreateDirect3D11DeviceFromDXGIDevice");
		}
		else {
			m_libD3D11 = nullptr;
		}

		m_libDXGI = std::shared_ptr<MyLib>(new MyLib(L"dxgi.dll"));
		if (m_libDXGI->Enabled()) {
			m_arrLibs.push_back(m_libDXGI.get());
			mCreateDXGIFactory1 = (cbCreateDXGIFactory1)m_libDXGI->GetFunction("CreateDXGIFactory1");
		}
		else {
			m_libDXGI = nullptr;
		}
	}
	m_libDXVA2 = std::shared_ptr<MyLib>(new MyLib(L"dxva2.dll"));
	if (m_libDXVA2->Enabled()) {
		m_arrLibs.push_back(m_libDXVA2.get());
		mDXVA2CreateDirect3DDeviceManager9 = (cbDXVA2CreateDirect3DDeviceManager9)m_libDXVA2->GetFunction("DXVA2CreateDirect3DDeviceManager9");
	}
	else {
		m_libDXVA2 = nullptr;
	}

	//DDRAW ，好像没什么用
	m_libDraw = std::shared_ptr<MyLib>(new MyLib(L"ddraw.dll"));
	if (m_libDraw->Enabled()) {
		m_arrLibs.push_back(m_libDraw.get());
		mDirectDrawCreate = (cbDirectDrawCreate)m_libDraw->GetFunction("DirectDrawCreate");
	}
	else {
		m_libDraw = nullptr;
	}

	//DSound 声音播放
	m_libDSound = std::shared_ptr<MyLib>(new MyLib(L"dsound.dll"));
	if (m_libDSound->Enabled()) {
		mDirectSoundCreate8 = (cbDirectSoundCreate8)m_libDSound->GetFunction("DirectSoundCreate8");
		CComPtr<IDirectSound8> pDS = NULL;
		HRESULT hr = mDirectSoundCreate8(NULL, &pDS, NULL);
		if (FAILED(hr)) { //不支持DSound，需要升级声卡驱动
			mDirectSoundCreate8 = nullptr;
			m_libDSound = nullptr;
		}
		else {

			m_arrLibs.push_back(m_libDSound.get());
		}
	}
	else {
		m_libDSound = nullptr;
	}

	//version.dll
	m_libVersion = std::shared_ptr<MyLib>(new MyLib(L"version.dll"));
	if (m_libVersion->Enabled()) {
		m_arrLibs.push_back(m_libVersion.get());
		mGetFileVersionInfoSizeW = (cbGetFileVersionInfoSizeW)m_libVersion->GetFunction("GetFileVersionInfoSizeW");
		mGetFileVersionInfoW = (cbGetFileVersionInfoW)m_libVersion->GetFunction("GetFileVersionInfoW");
		mVerQueryValueW = (cbVerQueryValueW)m_libVersion->GetFunction("VerQueryValueW");
	}
	else {
		m_libVersion = nullptr;
	}

	//DUMP
	m_libDbghelp = std::shared_ptr<MyLib>(new MyLib(L"dbghelp.dll"));
	if (m_libDbghelp->Enabled()) {
		m_arrLibs.push_back(m_libDbghelp.get());
		mMiniDumpWriteDump = (cbMiniDumpWriteDump)m_libDbghelp->GetFunction("MiniDumpWriteDump");

		mMakeSureDirectoryPathExists = (cbMakeSureDirectoryPathExists)m_libDbghelp->GetFunction("MakeSureDirectoryPathExists");
	}
	else {
		m_libDbghelp = nullptr;
	}


	m_libShcore = std::shared_ptr<MyLib>(new MyLib(L"shcore.dll"));//("shcore.dll");
	if (m_libShcore->Enabled()) {
		m_arrLibs.push_back(m_libShcore.get());
		m_SetProcessDpiAwareness = (PFN_SetProcessDpiAwareness)m_libShcore->GetFunction("SetProcessDpiAwareness");
		m_GetDpiForMonitor = (PFN_GetDpiForMonitor)m_libShcore->GetFunction("GetDpiForMonitor");
	}
	else {
		m_libShcore = nullptr;
	}


	m_libUser32 = std::shared_ptr<MyLib>(new MyLib(L"user32.dll"));//("user32.dll");
	if (m_libUser32->Enabled()) {
		m_arrLibs.push_back(m_libUser32.get());
		m_SetProcessDPIAware = (PFN_SetProcessDPIAware)m_libUser32->GetFunction("SetProcessDPIAware");
		m_ChangeWindowMessageFilterEx = (PFN_ChangeWindowMessageFilterEx)m_libUser32->GetFunction("ChangeWindowMessageFilterEx");
		m_EnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)m_libUser32->GetFunction("EnableNonClientDpiScaling");
		m_SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)m_libUser32->GetFunction("SetProcessDpiAwarenessContext");
		m_GetDpiForWindow = (PFN_GetDpiForWindow)m_libUser32->GetFunction("GetDpiForWindow");
		m_AdjustWindowRectExForDpi = (PFN_AdjustWindowRectExForDpi)m_libUser32->GetFunction("AdjustWindowRectExForDpi");
		m_GetSystemMetricsForDpi = (PFN_GetSystemMetricsForDpi)m_libUser32->GetFunction("GetSystemMetricsForDpi");
	}
	else {
		m_libUser32 = nullptr;
	}


	m_libDwmapi = std::shared_ptr<MyLib>(new MyLib(L"dwmapi.dll"));//("user32.dll");
	if (m_libDwmapi->Enabled()) {
		m_arrLibs.push_back(m_libDwmapi.get());
		m_DwmIsCompositionEnabled = (PFN_DwmIsCompositionEnabled)m_libDwmapi->GetFunction("DwmIsCompositionEnabled");
		m_DwmFlush = (PFN_DwmFlush)m_libDwmapi->GetFunction("DwmFlush");
		m_DwmEnableBlurBehindWindow = (PFN_DwmEnableBlurBehindWindow)m_libDwmapi->GetFunction("DwmEnableBlurBehindWindow");
		m_DwmGetColorizationColor = (PFN_DwmGetColorizationColor)m_libDwmapi->GetFunction("DwmGetColorizationColor");
	}
	else {
		m_libDwmapi = nullptr;
	}


	//VC++ 
	m_libNTDLL = std::shared_ptr<MyLib>(new MyLib(L"NTDLL.dll"));;//("NTDLL.dll");
	if (m_libNTDLL->Enabled()) {
		m_arrLibs.push_back(m_libNTDLL.get());
		m_RtlGetVersion = (PFNRtlGetVersion)m_libNTDLL->GetFunction("RtlGetVersion");
		m_RtlGetNtVersionNumbers = (PFNRtlGetNtVersionNumbers)m_libNTDLL->GetFunction("RtlGetNtVersionNumbers");
		m_RtlVerifyVersionInfo = (PFNRtlVerifyVersionInfo)m_libNTDLL->GetFunction("RtlVerifyVersionInfo");
	}
	else {
		m_libNTDLL = nullptr;
	}
	m_libVCRUNTIME140_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"VCRUNTIME140_CLR0400.dll"));
	if (m_libVCRUNTIME140_CLR0400->Enabled()) {
		m_arrLibs.push_back(m_libVCRUNTIME140_CLR0400.get());
	}
	m_libUCRTBASE_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"UCRTBASE_CLR0400.dll"));
	if (m_libUCRTBASE_CLR0400->Enabled()) {
		m_arrLibs.push_back(m_libUCRTBASE_CLR0400.get());
	}
	m_libMSVCP140_CLR0400 = std::shared_ptr<MyLib>(new MyLib(L"MSVCP140_CLR0400.dll"));
	if (m_libMSVCP140_CLR0400->Enabled()) {
		m_arrLibs.push_back(m_libMSVCP140_CLR0400.get());
	}

}

//程序退出使用
LibInst::~LibInst() {
	exit(-1);
}

void LibInst::LoadOpenGLFunctions() {
	m_libOpengl32 = std::shared_ptr<MyLib>(new MyLib(L"opengl32.dll"));
	if (m_libOpengl32->Enabled()) {
		m_arrLibs.push_back(m_libOpengl32.get());
		m_glClear = (PFNGLCLEARPROC)m_libOpengl32->GetFunction("glClear");
		m_glGetString = (PFNGLGETSTRINGPROC)m_libOpengl32->GetFunction("glGetString");
		m_glGetIntegerv = (PFNGLGETINTEGERVPROC)m_libOpengl32->GetFunction("glGetIntegerv");
		m_glGetStringi = (PFNGLGETSTRINGIPROC)m_libOpengl32->GetFunction("glGetStringi");

		m_wglCreateContext = (PFN_wglCreateContext)m_libOpengl32->GetFunction("wglCreateContext");
		m_wglDeleteContext = (PFN_wglDeleteContext)m_libOpengl32->GetFunction("wglDeleteContext");
		m_wglGetProcAddress = (PFN_wglGetProcAddress)m_libOpengl32->GetFunction("wglGetProcAddress");
		m_wglGetCurrentDC = (PFN_wglGetCurrentDC)m_libOpengl32->GetFunction("wglGetCurrentDC");
		m_wglGetCurrentContext = (PFN_wglGetCurrentContext)m_libOpengl32->GetFunction("wglGetCurrentContext");
		m_wglMakeCurrent = (PFN_wglMakeCurrent)m_libOpengl32->GetFunction("wglMakeCurrent");
		m_wglShareLists = (PFN_wglShareLists)m_libOpengl32->GetFunction("wglShareLists");

		m_wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)m_libOpengl32->GetFunction("wglSwapIntervalEXT");
		m_wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)m_libOpengl32->GetFunction("wglGetPixelFormatAttribivARB");
		m_wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)m_libOpengl32->GetFunction("wglGetExtensionsStringEXT");
		m_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)m_libOpengl32->GetFunction("wglGetExtensionsStringARB");
		m_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)m_libOpengl32->GetFunction("wglCreateContextAttribsARB");

		m_glTexImage2D = (PFNglTexImage2D)m_libOpengl32->GetFunction("glTexImage2D");
		m_glDrawBuffer = (PFNglDrawBuffer)m_libOpengl32->GetFunction("glDrawBuffer");
		m_glDrawArrays = (PFNglDrawArrays)m_libOpengl32->GetFunction("glDrawArrays");
		m_glDisable = (PFNglDisable)m_libOpengl32->GetFunction("glDisable");
		m_glDrawElements = (PFNglDrawElements)m_libOpengl32->GetFunction("glDrawElements");
		m_glFinish = (PFNglFinish)m_libOpengl32->GetFunction("glFinish");
		m_glFlush = (PFNglFlush)m_libOpengl32->GetFunction("glFlush");
		m_glReadBuffer = (PFNglReadBuffer)m_libOpengl32->GetFunction("glReadBuffer");
		m_glReadPixels = (PFNglReadPixels)m_libOpengl32->GetFunction("glReadPixels");
		m_glTexParameteri = (PFNglTexParameteri)m_libOpengl32->GetFunction("glTexParameteri");
		m_glTexSubImage2D = (PFNglTexSubImage2D)m_libOpengl32->GetFunction("glTexSubImage2D");
		m_glViewport = (PFNglViewport)m_libOpengl32->GetFunction("glViewport");
		m_glBindTexture = (PFNglBindTexture)m_libOpengl32->GetFunction("glBindTexture");
		m_glCopyTexSubImage2D = (PFNglCopyTexSubImage2D)m_libOpengl32->GetFunction("glCopyTexSubImage2D");
		m_glGetError = (PFNglGetError)m_libOpengl32->GetFunction("glGetError");
		m_glDeleteTextures = (PFNglDeleteTextures)m_libOpengl32->GetFunction("glDeleteTextures");
		m_glGenTextures = (PFNglGenTextures)m_libOpengl32->GetFunction("glGenTextures");
		m_glBlendFunc = (PFNglBlendFunc)m_libOpengl32->GetFunction("glBlendFunc");
		m_glEnable = (PFNglEnable)m_libOpengl32->GetFunction("glEnable");
		m_glClearColor = (PFNglClearColor)m_libOpengl32->GetFunction("glClearColor");
		m_glGetFloatv = (PFNglGetFloatv)m_libOpengl32->GetFunction("glGetFloatv");
		m_glTexParameterfv = (PFNglTexParameterfv)m_libOpengl32->GetFunction("glTexParameterfv");

	}
	else {
		m_libOpengl32 = nullptr;
	}
}

void LibInst::LoadOLE32() {
	m_libOLE32 = std::shared_ptr<MyLib>(new MyLib(L"ole32.dll"));
	if (m_libOLE32->Enabled()) {
		m_arrLibs.push_back(m_libOLE32.get());
		m_CoInitializeEx = (PFNCoInitializeEx)m_libOLE32->GetFunction("CoInitializeEx");
		m_CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
	}
	else {
		m_libOLE32 = nullptr;
	}
}

EXTERN_C LibInst* LibInst_GetInst() {
	if (s_LibInst == nullptr) {
		s_LibInst = std::make_shared<LibInst>();
	}
	return s_LibInst.get();
}

EXTERN_C void LibInst_LogMsg() {
	if (s_LibInst != nullptr) {
		s_LibInst->LogMsg();
	}
}
#endif

