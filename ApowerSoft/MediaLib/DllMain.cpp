#include "ML_stdafx.h"
#include <gdiplus.h>
#include <dbghelp.h>
#include <libheif/heif.h>
#include <WXBase.h>

EXTERN_C{
	#include "utils/ffplay.h"
	#include <ffms2/ffms.h>
	#include <avisynth\avisynth_c.h>
	#include "MediaLibAPI.h"
	//ffmpeg.h
	void  FFMPEG_SetWaterMark(char* water);
	HRESULT FFMPEG_SetField(char* field, char* value);
	HRESULT FFMPEG_SetFieldInt(char* field, int value);
	HRESULT FFMPEG_SetProgressCallback(ProgressCallBack callback);
	HRESULT FFMPEG_SetConvertState(int state);
	int FFMPEG_ProcessCommand(int argc, char** argv);
	//Toolkit.c
	HRESULT capture_image(char* filename, float second, wchar_t* output, int height);
}

HRESULT capture_image(char* filename, float second, wchar_t* output, int height);

#include "D3D_Filters/ML_D3DRender.h"
#pragma comment(lib, "libffmpeg.lib")
#pragma comment(lib, "gdiplus.lib")
//#pragma comment(lib, "libyuv.lib")
#pragma comment(lib, "libheif.lib")

#pragma comment(lib, "strmiids.lib") //DirectShow

static bool s_bFfmpegExe = false;//是否可以调用ffmpeg.exe
static std::wstring s_strFfmpegExe = L"ffmpeg.exe";
//EXTERN_C CrashCallBack crashcallback = NULL;
// 处理Unhandled Exception的回调函数
static DWORD ExecuteProcess(std::wstring exepath, DWORD dwTimeOut)
{
	WXLogW(L"%ws %ws",__FUNCTIONW__, exepath.c_str());

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
	DWORD result =  WaitForSingleObject(pi.hProcess, dwTimeOut /*INFINITE*/);
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
		WXLogW(L"Unexpected result from WaitForSingleObject: %ul",result);
		break;
	}
	// 关闭进程和线程句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return result;
}

static WXLocker g_lckTool;
static DWORD ExecuteFfmpegExe(const wchar_t* wszArg)
{
	//WXAutoLock al(g_lckTool);
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

static int s_bInit = FALSE;
static VideoState* s_pCurr = nullptr;//唯一播放对象
extern INT32 InitAvisynth();

static WXLocker s_lckPlaySet;
static ErrorLogCallBack s_errorlog_call_back = NULL;
ErrorLogCallBack ML_GetErrorLogCallBack() {
	WXAutoLock al(s_lckPlaySet);
	return s_errorlog_call_back;
}

//设置错误回调
MEDIALIB_API   HRESULT  SetErrorLogCallback(ErrorLogCallBack  callback) {
	WXAutoLock al(s_lckPlaySet);
	s_errorlog_call_back = callback;
	return S_OK;
}


static RenderCallBack s_render_call_back = nullptr;
EXTERN_C RenderCallBack ML_GetRenderCallback() {
	WXAutoLock al(s_lckPlaySet);
	return s_render_call_back;
}
MEDIALIB_API  HRESULT  SetRenderCallback(VideoState* is, RenderCallBack  callback) {
	WXAutoLock al(s_lckPlaySet);
	s_render_call_back = callback;
	return S_OK;
}

static WXLocker s_lckDemux;
EXTERN_C void DemuxLock() {
	s_lckDemux.lock();
}
EXTERN_C void DemuxUnlock() {
	s_lckDemux.unlock();
}

static WXLocker s_lckPlay;
EXTERN_C void PlayLock() {
	s_lckPlay.lock();
}
EXTERN_C void PlayUnlock() {
	s_lckPlay.unlock();
}

std::wstring GetFullPathW(const wchar_t* filename) {
	std::wstring fullpath = L"";
	if (WXBase::Exists(filename)) { //相对路径
		std::filesystem::path abs_path = std::filesystem::absolute(filename);
		fullpath = abs_path.wstring(); //当前路径
		return fullpath;
	}


	//初始运行路径
	std::wstring s_strRunPath = ML_GetString(L"runpath");
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

	//相对于 Medialib.dll 的路径
	std::wstring s_strDllDir = ML_GetString(L"dlldir");
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
	std::wstring s_strProgData = ML_GetString(L"ProgramData");;
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

MEDIALIB_API void* BlackSurface()
{
	return 0;
}

MEDIALIB_API MediaInfomation* AnalyzeMedia(char* utf8_path) {
	if (!s_bInit) {
		MessageBoxW(NULL, L"Not init", L"Medialib", MB_OK);
		return nullptr;
	}
	if (strcmp(utf8_path, "") == 0 || strcmp(utf8_path, "custom") == 0)
		return nullptr;

	std::wstring utf16_path = WXBase::UTF8ToUTF16(utf8_path);
	if (!WXBase::Exists(utf16_path)) {
		::MessageBoxA(NULL, utf8_path, "File No Found!", MB_OK);
		return nullptr;;
	}

	MediaInfomation* pMi = NULL;
	int tt = -1;
	FFMS_Index* pIndex = FFMS_ProcessIndex(utf8_path, &tt);
      	if (pIndex != 0) {
		pMi = FFMS_MediaInformation(pIndex);
	}
	return pMi;
}

MEDIALIB_API MediaInfomation* AnalyzeMediaW(wchar_t* utf16_path) {
	if (!s_bInit) {
		MessageBoxW(NULL, L"Not init", L"Medialib", MB_OK);
		return nullptr;
	}
	if (wcscmp(utf16_path, L"") == 0 || wcscmp(utf16_path, L"custom") == 0)
		return nullptr;

	if (!WXBase::Exists(utf16_path)) {
		::MessageBoxW(NULL, utf16_path, L"File No Found!", MB_OK);
		return nullptr;;
	}

	MediaInfomation* pMi = NULL;
	int tt = -1;
	std::string utf8_path = WXBase::UTF16ToUTF8(utf16_path);
	FFMS_Index* pIndex = FFMS_ProcessIndex(utf8_path.c_str(), &tt);
	if (pIndex != 0) {
		pMi = FFMS_MediaInformation(pIndex);
	}
	return pMi;
}

MEDIALIB_API MediaInfomation* AnalyzeMedia2(char* utf8_path) {
	return AnalyzeMedia(utf8_path);
}
MEDIALIB_API MediaInfomation* AnalyzeMediaW2(wchar_t* utf16_path) {
	return AnalyzeMediaW(utf16_path);
}

MEDIALIB_API INT64 GetAssDuration(char* utf8_path)
{
	ASS_Track* track = AssEngine::InstanceML().Read(utf8_path);
	if (track != NULL && track->n_events > 0)
	{
		ASS_Event event = track->events[track->n_events - 1];
		ass_free_track(track);
		return event.Start + event.Duration;
	}
	return 0;
}


static bool MakeSureDirectoryPathExistsW(const std::wstring& path) {
	size_t pos = 0;
	std::wstring subdir;

	// 循环遍历每个目录层级
	while ((pos = path.find_first_of(L"/\\", pos + 1)) != std::wstring::npos) {
		subdir = path.substr(0, pos);
		int len = subdir.length();
		const wchar_t* tt = subdir.c_str();
		if (tt[len - 1] == L':') {
			continue;
		}
		if (!CreateDirectoryW(subdir.c_str(), nullptr)) {
			DWORD error = GetLastError();
			if (error != ERROR_ALREADY_EXISTS) {
				std::wcerr << L"无法创建目录：" << subdir << L"，错误代码：" << error << std::endl;
				return false;
			}
		}
	}

	// 确保路径的最后一部分被创建
	if (!CreateDirectoryW(path.c_str(), nullptr)) {
		DWORD error = GetLastError();
		if (error != ERROR_ALREADY_EXISTS) {
			std::wcerr << L"无法创建目录：" << path << L"，错误代码：" << error << std::endl;
			return false;
		}
	}

	return true;
}




//ini配置
static WXString g_strIniPath = L"";

//从ini获取数值
MEDIALIB_API int MLGetIniValue(WXCTSTR wszReg, WXCTSTR wszKey, int nDefaultValue) {
#ifdef _WIN32
	return ::GetPrivateProfileInt(wszReg, wszKey, nDefaultValue, g_strIniPath.str());
#else
	return nDefaultValue;
#endif
}

MEDIALIB_API void MLGetStringValue(WXCTSTR wszReg, WXCTSTR wszKey,  wchar_t* strValue) {
#ifdef _WIN32
	::GetPrivateProfileStringW(wszReg, wszKey, L"nullptr", strValue, MAX_PATH, g_strIniPath.str());
#endif
}

//配置ini的数值
MEDIALIB_API void MLSetIniValue(WXCTSTR wszReg, WXCTSTR wszKey, int nValue) {
#ifdef _WIN32
	WXString wxstr;
	wxstr.Format(L"%d", nValue);
	::WritePrivateProfileString(wszReg, wszKey, wxstr.str(), g_strIniPath.str());
#endif
}

//配置ini的字符串
MEDIALIB_API void MLSetIniString(const wchar_t* wszReg, const wchar_t* wszKey, const wchar_t* strValue) {
#ifdef _WIN32
	::WritePrivateProfileString(wszReg, wszKey, strValue, g_strIniPath.str());
#endif
}

#include <dxva2api.h>
#include <initguid.h>
DEFINE_GUID(ff_DXVA2_ModeH264_E, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(ff_DXVA2_ModeH264_F, 0x1b81be69, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);

DEFINE_GUID(ff_DXVADDI_Intel_ModeH264_E, 0x604F8E68, 0x4951, 0x4C54, 0x88, 0xFE, 0xAB, 0xD2, 0x5C, 0x15, 0xB3, 0xD6);

DEFINE_GUID(ff_DXVA2_ModeHEVC_VLD_Main, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);

DEFINE_GUID(ff_IID_IDirectXVideoDecoderService, 0xfc51a551, 0xd5e7, 0x11d9, 0xaf, 0x55, 0x00, 0x05, 0x4e, 0x43, 0xff, 0x02);

//硬解码能力检测
MEDIALIB_API  BOOL MLDetectDXVADecode() {

	BEGIN_LOG_FUNC

	//ini强制禁用硬解码
	int bDxvaSupport = MLGetIniValue(L"WXMedia", L"DxvaSupport", -1);
	if (bDxvaSupport == 0) // 不处理
		return 0;

	//D3D9
	if (LibInst::GetInst().m_libD3D9 == nullptr ||
		LibInst::GetInst().m_libDXVA2 == nullptr ||
		LibInst::GetInst().mDirect3DCreate9 == nullptr) {
		return 0;
	}
	//初始化检测
	MLSetIniValue(L"VideoDecoder", L"H264_DXVA", 0);
	MLSetIniValue(L"VideoDecoder", L"H265_DXVA", 0);

	//初始化
	HRESULT hr = S_OK;
	CComPtr<IDirect3D9> pD3D = LibInst::GetInst().mDirect3DCreate9(D3D_SDK_VERSION);
	if (pD3D == nullptr) {
		WXLogA("%ws Direct3DCreate9 Error.",__FUNCTIONW__);
		return FALSE;
	}

	// Get the adapter display mode
	D3DDISPLAYMODE d3ddm;
	if (FAILED(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) {
		WXLogA("%ws GetAdapterDisplayMode Error.", __FUNCTIONW__);
		return FALSE;
	}

	// Set up the presentation parameters
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = d3ddm.Format;

	// Create a Direct3D device
	CComPtr < IDirect3DDevice9> pD3DDevice = nullptr;
	if (FAILED(pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, GetDesktopWindow(),
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice))) {
		WXLogA("%ws D3D9 CreateDevice Error.", __FUNCTIONW__);
		return FALSE;
	}

	UINT resetToken = 0;
	CComPtr<IDirect3DDeviceManager9> pDeviceManager9;
	hr = LibInst::GetInst().mDXVA2CreateDirect3DDeviceManager9(&resetToken, &pDeviceManager9);
	if (FAILED(hr) || pDeviceManager9 == nullptr) {
		WXLogA("%ws DXVA2CreateDirect3DDeviceManager9 Error.", __FUNCTIONW__);
		return FALSE;
	}

	hr = pDeviceManager9->ResetDevice(pD3DDevice, resetToken);
	if (FAILED(hr)) {
		WXLogA("%ws IDirect3DDeviceManager9 ResetDevice Error.", __FUNCTIONW__);
		return FALSE;
	}

	HANDLE pDeviceHandle = nullptr;
	hr = pDeviceManager9->OpenDeviceHandle(&pDeviceHandle);
	if (FAILED(hr)) {
		WXLogA("%ws IDirect3DDeviceManager9 OpenDeviceHandle Error.", __FUNCTIONW__);
		pDeviceManager9->CloseDeviceHandle(&pDeviceHandle);
		pDeviceHandle = nullptr;
		return FALSE;
	}

	// Create a DirectX Video Acceleration device manager
	CComPtr < IDirectXVideoDecoderService> pDXVAService = nullptr;

	hr = pDeviceManager9->GetVideoService(pDeviceHandle, ff_IID_IDirectXVideoDecoderService, (void**)&pDXVAService);
	if (FAILED(hr)) {
		WXLogA("%ws IDirect3DDeviceManager9 GetVideoService Error.", __FUNCTIONW__);
		pDeviceManager9->CloseDeviceHandle(&pDeviceHandle);
		pDeviceHandle = nullptr;
		return FALSE;
	}

	// Check for H.264 decoder support
	UINT numProfiles = 0;
	GUID* pGuids = NULL;// = new GUID[numProfiles];
	if (FAILED(pDXVAService->GetDecoderDeviceGuids(&numProfiles, &pGuids)) || pGuids == nullptr) {
		WXLogA("%ws GetVideoService GetDecoderDeviceGuids Error.", __FUNCTIONW__);
		pDeviceManager9->CloseDeviceHandle(&pDeviceHandle);
		pDeviceHandle = nullptr;
		return FALSE;
	}

	int isDxvaSupport = 0;
	for (UINT i = 0; i < numProfiles; ++i) {
		if (pGuids[i] == ff_DXVA2_ModeH264_E) {
			MLSetIniValue(L"VideoDecoder", L"H264_DXVA", 1);
			isDxvaSupport = 1;
		}
		if (pGuids[i] == ff_DXVA2_ModeH264_F) {
			MLSetIniValue(L"VideoDecoder", L"H264_DXVA", 1);
			isDxvaSupport = 1;
		}
		if (pGuids[i] == ff_DXVADDI_Intel_ModeH264_E) {
			MLSetIniValue(L"VideoDecoder", L"H264_DXVA", 1);
			isDxvaSupport = 1;
		}
		if (pGuids[i] == ff_DXVA2_ModeHEVC_VLD_Main) {
			MLSetIniValue(L"VideoDecoder", L"H265_DXVA", 1);
			isDxvaSupport = 1;
		}
	}

	//delete[] pGuids;

	pDeviceManager9->CloseDeviceHandle(&pDeviceHandle);
	pDeviceHandle = nullptr;
	MLGetIniValue(L"WXMedia", L"DxvaSupport", isDxvaSupport);
	return isDxvaSupport;
}

//H264  H265 硬编码检测
#define TWDITH   1920
#define THEIGHT  1080
#define TBITRATE 8000*1000
#define STREAM_FRAME_RATE 25

static WXString sStrH264Codec = L"libx264";
static WXString sStrH265Codec = L"libx265";
static int MLCheckQSV(BOOL bH264, WXCTSTR wszCodec, WXString& strHWCodec, int& qsv_flag) {
	qsv_flag = 0;
	WXString str;
	str.Format(wszCodec);
	AVCodec* codec = avcodec_find_encoder_by_name(str.c_str());
	if (codec == nullptr)return 0;
	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	ctx->width = TWDITH;
	ctx->height = THEIGHT;
	ctx->time_base.num = 1;
	ctx->time_base.den = STREAM_FRAME_RATE;
	ctx->pix_fmt = codec->pix_fmts[0];
	ctx->bit_rate = TBITRATE;

	AVDictionary* opt = nullptr;
	if(!bH264)
		av_dict_set(&opt, "load_plugin", "hevc_hw", 0);

	int ret = avcodec_open2(ctx, codec, &opt);
	if (ret >= 0) {
		qsv_flag = 1;
		strHWCodec.Format(wszCodec);
		WXLogA("%s Support Hardware Codec =  %s OK ",__FUNCTION__, str.c_str());
		avcodec_close(ctx);
	}else {
		WXLogA("%s Not Support Hardware Codec =  %s OK ", __FUNCTION__, str.c_str());
	}
	avcodec_free_context(&ctx);
	return qsv_flag;
}
MEDIALIB_API const wchar_t* MLGetH265Codec() {
	return sStrH265Codec.str();
}
MEDIALIB_API const wchar_t* MLGetH264Codec() {
	return sStrH264Codec.str();
}
//是否支持H265硬编码
static WXLocker s_lckQSV;
static int MLSupportQSVCodec(BOOL bH264) {
	WXAutoLock al(s_lckQSV);
	int bCheckQSV = MLGetIniValue(_T("WXMedia"), _T("Support_QSV_Encoder"), -1);
	if (bCheckQSV == 0)
		return 0;

	const wchar_t *strHW = bH264 ? L"H264_HW" : L"H265_HW";
	const wchar_t* strCodec = bH264 ? L"h264_qsv" : L"hevc_qsv";

	int nHw = MLGetIniValue(L"WXMedia", strHW, -1);
	if (nHw > 0) {
		MLSetIniValue(_T("WXMedia"), _T("Support_QSV_Encoder"), 1);
		bH264 ? sStrH264Codec = L"h264_qsv": sStrH265Codec = L"hevc_qsv";
		return 1;
	}else if (nHw == 0) {
		return 0;
	}
	MLSetIniValue(L"WXMedia", strHW, 0);
	if (LibInst::GetInst().m_libD3D9 == nullptr) {
		//硬编码都需要D3D9 对应功能
		return 0;
	}
	int x = GetSystemMetrics(SM_CXSCREEN); //屏幕宽度
	int y = GetSystemMetrics(SM_CYSCREEN); //屏幕高度
	if (x * y <= 1366 * 768) { 
		return 0;
	}
	int qsv_flag = 0;
	MLCheckQSV(bH264, strCodec, bH264? sStrH264Codec: sStrH265Codec, qsv_flag);
	if (qsv_flag) {
		MLSetIniValue(_T("WXMedia"), _T("Support_QSV_Encoder"), 1);
	}
	WXLogA("%s qsv_flag=%d ", __FUNCTION__, qsv_flag);
	MLSetIniValue(L"WXMedia", strHW, qsv_flag);
	ML_SetValue(bH264?L"H264":L"H265", qsv_flag);
	return qsv_flag;
}

MEDIALIB_API int MLSupportH265Codec() {
	WXAutoLock al(s_lckQSV);
	BEGIN_LOG_FUNC
	return MLSupportQSVCodec(TRUE);
}

MEDIALIB_API int MLSupportH264Codec() {
	WXAutoLock al(s_lckQSV);
	BEGIN_LOG_FUNC
	return MLSupportQSVCodec(FALSE);
}

MEDIALIB_API  int  Initialize(const char* _company, const char* _product, bool mode) {

	if (s_bInit)
		return 0;

	BEGIN_LOG_FUNC
	std::wstring s_strCompany = WXBase::UTF8ToUTF16(_company);
	ML_SetString(L"company", s_strCompany.c_str());

	std::wstring s_strProduct = WXBase::UTF8ToUTF16(_product);
	ML_SetString(L"product", s_strProduct.c_str());

	std::wstring s_strLogDir = WXBase::getEnvVarW(L"appdata") + L"\\" + s_strCompany.c_str() + L"\\" + s_strProduct.c_str() + L"\\log\\";
	ML_SetString(L"logdir", s_strLogDir.c_str());

	std::wstring s_strDump = s_strLogDir + L"crash.dmp";
	ML_SetString(L"dump", s_strDump.c_str());

	std::wstring s_strTempDir = WXBase::getEnvVarW(L"temp") + L"\\" + s_strCompany.c_str() + L"\\" + s_strProduct.c_str() + L"\\";
	ML_SetString(L"temp", s_strTempDir.c_str());

	std::wstring s_strIndexDir = WXBase::getEnvVarW(L"temp") + L"\\" + s_strCompany.c_str() + L"\\" + s_strProduct.c_str() + L"\\Index+1128\\";
	ML_SetString(L"index", s_strIndexDir.c_str());

	std::wstring  s_strProgData = WXBase::getEnvVarW(L"ProgramData") + L"\\" + s_strCompany.c_str() + L"\\" + s_strProduct.c_str() + L"\\";
	ML_SetString(L"ProgramData", s_strProgData.c_str());

	// 获取当前运行目录
	std::wstring s_strRunPath = WXBase::GetRunPath();//当前运行目录
	ML_SetString(L"runpath", s_strRunPath.c_str());

	//Medialib.dll 所在目录
	std::wstring s_strDllDir = WXBase::GetDllPath();
	ML_SetString(L"dlldir", s_strDllDir.c_str());;

	if (WXBase::Exists(L"libffmpeg.dll")) {
		std::wstring Libffmpeg_DllPath = std::filesystem::absolute(L"libfffmpeg.dll").wstring();
		WXLogW(L"Medialib.dll Path = %ws", Libffmpeg_DllPath.c_str());
	}
	else {
		std::wstring Libffmpeg_DllPath = s_strDllDir + L"libffmpeg.dll";
		if (WXBase::Exists(Libffmpeg_DllPath)) {
			//libffmpeg.dll在同级目录
			::SetDllDirectoryW(s_strDllDir.c_str());//Medialib.dll
		}
		else {
			//最多在上一级目录
			int found1 = Libffmpeg_DllPath.find_last_of(L"/\\");
			std::wstring s1 = Libffmpeg_DllPath.substr(0, found1);
			int found2 = s1.find_last_of(L"/\\");
			std::wstring s2 = s1.substr(0, found2 + 1);
			Libffmpeg_DllPath = s2 + L"libffmpeg.dll";
			if (WXBase::Exists(Libffmpeg_DllPath)) {
				//libffmpeg.dll在同级目录
				::SetDllDirectoryW(s2.c_str());//Medialib.dll
			}
		}
	}

	//如果路径不存在就弹窗警告,然后exit
	if(LibInst::GetInst().m_libDbghelp)
		LibInst::GetInst().mMakeSureDirectoryPathExists(WXBase::UTF16ToGB(s_strLogDir).c_str());
	MakeSureDirectoryPathExistsW(s_strLogDir.c_str());//Log目录
	if (!WXBase::Exists(s_strLogDir)) {
		WXString strMsg;
		strMsg.Format(L"Sorry ,The Log Dir[%ws] is not exist!", s_strLogDir.c_str());
		MessageBoxW(NULL, strMsg.str(), L"Medialib.dll", MB_OK);
		exit(-1);
	}

	if (LibInst::GetInst().m_libDbghelp)
		LibInst::GetInst().mMakeSureDirectoryPathExists(WXBase::UTF16ToGB(s_strTempDir).c_str());
	MakeSureDirectoryPathExistsW(s_strTempDir.c_str());//Temp目录 缩略图
	if (!WXBase::Exists(s_strTempDir)) {
		WXString strMsg;
		strMsg.Format(L"Sorry ,The Temp Dir[%ws] is not exist!", s_strTempDir.c_str());
		MessageBoxW(NULL, strMsg.str(), L"Medialib.dll", MB_OK);
		exit(-1);
	}

	if (LibInst::GetInst().m_libDbghelp)
		LibInst::GetInst().mMakeSureDirectoryPathExists(WXBase::UTF16ToGB(s_strIndexDir).c_str());
	MakeSureDirectoryPathExistsW(s_strIndexDir);//Index目录
	if (!WXBase::Exists(s_strIndexDir)) {
		WXString strMsg;
		strMsg.Format(L"Sorry ,The Index Dir[%ws] is not exist!", s_strIndexDir.c_str());
		MessageBoxW(NULL, strMsg.str(), L"Medialib.dll", MB_OK);
		exit(-1);
	}

	//无法创建LOG文件就弹窗警告！然后exit
	std::wstring strLog = s_strLogDir + L"\\avisynth_128.log";
	g_strIniPath = s_strLogDir.c_str();
	g_strIniPath += L"WXMedia.ini";//同目录下的ini文件

	//EXE全名
	std::wstring wstrName = WXBase::GetExeName();
	if (wcsicmp(wstrName.c_str(),L"MediaLib_Test.exe") == 0) {
		strLog = s_strRunPath + L"debug.log";
	}
	bool bOpenLog = WXLogInit(strLog.c_str());
	if (!bOpenLog) {
		strLog = L"MyDebug.log";

	    bOpenLog = WXLogInit(strLog.c_str());
		if (!bOpenLog) {
			WXString strMsg;
			strMsg.Format(L"Sorry ,Create LogFile[%ws] Error!", strLog.c_str());
			MessageBoxW(NULL, strMsg.str(), L"Medialib.dll", MB_OK);
		}
	}
	
	LibInst::GetInst().LogMsg();
	WXLogWOnce(L"MediaLib Initialize: %ws, %ws", s_strCompany.c_str(), s_strProduct.c_str());

	GetFullPathW(L"medialib.dll");
	SetDllDirectoryW(s_strDllDir.c_str());

	::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM 初始化
	static ULONG_PTR m_gdiplusToken = 0;
	if (m_gdiplusToken == 0) {
		Gdiplus::GdiplusStartupInput StartupInput;//GDI+初始化
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &StartupInput, NULL);
	}
	//if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
	//	SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE", "1", 1);
	//SetEnvironmentVariable(_T("SDL_AUDIODRIVER"), _T("directsound"));
	//SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);

	//ffmpeg.exe 检测
	std::thread thFfmpeg([] {

		s_strFfmpegExe = GetFullPathW(L"ffmpeg.exe");
		int getExe = MLGetIniValue(L"ffmpeg.exe", L"exist", -1);
		if (getExe == -1)
		{
			DWORD dwTimeOut = 10000;
			DWORD result = ExecuteProcess(s_strFfmpegExe, dwTimeOut);
			if (result == WAIT_OBJECT_0) {
				s_bFfmpegExe = true;
				MLSetIniValue(L"ffmpeg.exe", L"exist", 1);
				WXLogW(L"%ws is exist!", s_strFfmpegExe.c_str());
			}
			else {
				MLSetIniValue(L"ffmpeg.exe", L"exist", 0);
				s_bFfmpegExe = false;
				WXLogW(L"%ws is not exist!", s_strFfmpegExe.c_str());
			}
		}
		else {
			s_bFfmpegExe = getExe;
		}
	});
	thFfmpeg.detach();

	std::thread thDXVA([] {
		MLDetectDXVADecode();//硬解码检测
	});
	thDXVA.detach();

	std::thread thH264([] {
		MLSupportH264Codec();//硬解码检测
	});
	thH264.detach();

	std::thread thH265([] {
		MLSupportH265Codec();//硬解码检测
	});
	thH265.detach();

	InitAvisynth();
	InitMedia();
	InitExceptionFilter(WXBase::UTF16ToUTF8(s_strDump).c_str());
	InitDevice(false);
	ImageProcessInitialize();
	s_bInit = 1;//init OK;
	return S_OK;
}

MEDIALIB_API  void Clean() {
	exit(0);
}

MEDIALIB_API  HRESULT  SetOpenCallbackAsync(VideoState* is, OpenCallBack callback) {
	return S_OK;
}

MEDIALIB_API  HRESULT  SetStateChangedCallbackAsync(VideoState* is, StateChangedCallBack callback) {
	return S_OK;
}

//全局播放器锁
static WXLocker g_lckPlay;
static WXLocker g_lckSeek;
std::queue<double>s_queueSeek;//Seek 位置， 在线程中Play之前 检测，每次调用最后一个

//轻编辑释放已经打开的对象
MEDIALIB_API void     ClearSource() {
	WXAutoLock al(g_lckPlay);
	if (s_pCurr != nullptr) {
		return;
	}
	FFMS_GetEnv()->Clear();
}

BOOL TimelineIsAvailable();


MEDIALIB_API HRESULT  CloseMedia(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	BEGIN_LOG_FUNC
	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
		return E_FAIL;

	//取消播放线程
	if (_is->m_threadPlay) { 
		_is->m_bStopPlay = true;
		std::thread* th = (std::thread*)_is->m_threadPlay;
		th->join();
		delete th;
		_is->m_threadPlay = nullptr;
	}
	s_pCurr->force_refresh = TRUE;
	Pause(s_pCurr);//暂停线程
	stream_close(s_pCurr);//关闭流
	s_pCurr = NULL;
	return S_OK;
}

//旧版本播放函数
//可以使用Resume代替
MEDIALIB_API  HRESULT  Play(VideoState* _is)
{
	//BEGIN_LOG_FUNC
	//return Resume(_is);
	return S_OK;
}

//跳转到指定位置
//防止无影手操作
MEDIALIB_API HRESULT  Seek(VideoState* _is, double second)
{
	WXAutoLock al(g_lckSeek);
	while (!s_queueSeek.empty()) {
		second = s_queueSeek.front();
		s_queueSeek.pop();
	}
	s_queueSeek.push(second);
	return S_OK;
}

//创建播放对象,成功之后可以使用 Resume开始播放
MEDIALIB_API   VideoState* OpenMedia(char* utf8_path, BOOL audiodisable, BOOL paused, float seeksecond){
	WXAutoLock al(g_lckPlay);
	//清理内存
	ClearSource();

	BEGIN_LOG_FUNC
	if (s_pCurr != nullptr) {
		stream_close(s_pCurr);
		s_pCurr = nullptr;
	}

	if (strlen(utf8_path) == 0) {
		return nullptr;
	}
	if ( stricmp(utf8_path, "custom") == 0 && !TimelineIsAvailable()) {
		return nullptr;
	}

	s_pCurr = stream_open(utf8_path, 0, audiodisable, paused, seeksecond);
	if (s_pCurr == nullptr) {
		return nullptr;
	}

	WXLogA("%s paused=%d seeksecond=%0.2f",__FUNCTION__, paused, seeksecond);
	s_pCurr->Hwnd = nullptr;
	//启动线程
	s_pCurr->m_bStopPlay = false;
	s_pCurr->m_threadPlay = (void*)(new std::thread([&] {
		while (!s_pCurr->m_bStopPlay) {

			{
				WXAutoLock al(g_lckSeek);
				if (s_queueSeek.size()) {
					double second = s_queueSeek.front();
					s_queueSeek.pop();
					//SEEK
					s_pCurr->force_refresh = TRUE;
					auto _second = second * s_pCurr->speed;
					_second = std::min(s_pCurr->ic->duration / (double)AV_TIME_BASE, second);
					_second = (float)((int)(second * 100)) / 100;
					if (s_pCurr->ic->duration / (double)AV_TIME_BASE > _second)
					{
						if (s_pCurr->seek_by_bytes || s_pCurr->ic->duration <= 0) {
							uint64_t size = avio_size(s_pCurr->ic->pb);
							stream_seek(s_pCurr, size * second * AV_TIME_BASE / s_pCurr->ic->duration, 0, false);
						}
						else {
							int64_t ts = (int64_t)(second * AV_TIME_BASE);
							if (s_pCurr->ic->start_time != AV_NOPTS_VALUE)
								ts += s_pCurr->ic->start_time;
							stream_seek(s_pCurr, ts, 0, false);
						}
					}
				}
			}
			refresh_loop(s_pCurr);
			SLEEPMS(10);
		}
	}));

	if (paused) {
		Pause(s_pCurr);
	}
	else {
		Resume(s_pCurr);
	}
	return s_pCurr;
}



MEDIALIB_API  HRESULT GetMediaInfo(VideoState* _is, MediaInfo* mi)
{
	if (g_lckPlay.try_lock()) {
		if (_is == nullptr || _is != s_pCurr || _is->abort_request) {
			g_lckPlay.unlock();
			return E_FAIL;
		}
		*mi = _GetMediaInfo(_is);
		if (_is->step) {
			mi->Paused = 1;
		}
		g_lckPlay.unlock();
		return S_OK;
	}
	return E_FAIL;
}

//切换暂停-恢复
MEDIALIB_API  HRESULT TogglePause(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
	return E_FAIL;
	s_pCurr->force_refresh = TRUE;
	toggle_pause(_is);
	return S_OK;
}

//暂停
MEDIALIB_API HRESULT  Pause(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
		return E_FAIL;
	//BEGIN_LOG_FUNC
	s_pCurr->force_refresh = TRUE;
	if (!s_pCurr->paused) {
		toggle_pause(s_pCurr);
	}
	return S_OK;
}

//恢复播放
MEDIALIB_API HRESULT  Resume(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
		return E_FAIL;
	_is->force_refresh = TRUE;
	if (_is->paused)
	{
		toggle_pause(_is);
	}
	return S_OK;
}

//设置音量
MEDIALIB_API HRESULT  SetVolume(VideoState* _is, int volume)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
		return E_FAIL;
	_is->audio_volume = volume;
	return S_OK;
}

//无效
MEDIALIB_API HRESULT  RefreshFrame()
{
	return S_OK;
}

//上一帧
MEDIALIB_API HRESULT  PreFrame(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is->abort_request)
		return E_FAIL;
	HRESULT hr = S_OK;
	{
		double Position = 0;
		if (s_pCurr->video_st)
			Position = get_clock(&s_pCurr->vidclk);
		else
			WXLogA("video_st is null");
		Position = std::max(Position - 0.05, 0.0);
		hr = Seek(s_pCurr, Position);
	}
	return hr;
}

//下一帧
MEDIALIB_API HRESULT  NextFrame(VideoState* _is)
{
	WXAutoLock al(g_lckPlay);
	if (_is == nullptr || _is->abort_request)
		return E_FAIL;
	step_to_next_frame(_is);
	return S_OK;
}

//同样的画质，硬编码对应的CRF值应该给比软解码低3-5
int FFMPEG_ConvertCommand(char* cmds)
{
	//int bCheckQSV    = MLGetIniValue(_T("WXMedia"), _T("Support_QSV_Encoder"), -1);

	//int bCheckQSV = ML_GetValue(L"QSV");//H264 硬编码优化,界面设置

	//BOOL bSupportH264QSV = FALSE;
	//BOOL bSupportH265QSV = FALSE;
	//if (bCheckQSV)
	//{
	//	const wchar_t* wszH264 =  MLGetH264Codec();		
	//	if (wcsicmp(wszH264, L"h264_qsv") == 0)
	//		bSupportH264QSV = TRUE;
	//	const wchar_t* wszH265 =  MLGetH265Codec();
	//	if (wcsicmp(wszH265, L"hevc_qsv") == 0)
	//		bSupportH265QSV = TRUE;
	//	if (bSupportH264QSV == FALSE && bSupportH265QSV ==FALSE) {
	//		bCheckQSV = 0;
	//	}
	//}

	WXBase::StringList argc_list = WXBase::string_splitstr(cmds, "|");
	int argc = argc_list.size();
	if (argc <= 1) {
		WXLogA("Error %s %s argc=%d", __FUNCTION__, cmds, argc);
		return 0;
	}

	//if (bCheckQSV) {
	//	bool change_x264 = false;
	//	bool change_x265 = false;
	//	for (size_t i = 0; i < argc; i++)
	//	{
	//		if (bSupportH264QSV) {
	//			if (argc_list[i] == "libx264" || argc_list[i] == "h264" || argc_list[i] == "H264") {
	//				int support_qsv = MLGetIniValue(L"WXMedia", L"H264_HW", 0); //强制将lib264改成h264_qsv
	//				if (support_qsv) {
	//					argc_list[i] = "h264_qsv";
	//					change_x264 = true;
	//				}
	//			}
	//			if (change_x264)
	//			{
	//				if (argc_list[i] == "-crf") {
	//					argc_list[i] = "-global_quality";//qsv not use crf

	//					int crf = atoi(argc_list[i + 1].c_str());
	//					crf -= 3;
	//					char str[5];
	//					sprintf(str, "%d", crf);
	//					argc_list[i + 1] = str;
	//				}
	//			}
	//		}
	//		if (bSupportH265QSV) {
	//			if (argc_list[i] == "libx265" || argc_list[i] == "h265" || argc_list[i] == "H265" ||
	//				argc_list[i] == "HEVC" || argc_list[i] == "hevc") {
	//				int support_qsv = MLGetIniValue(L"WXMedia", L"H265_HW", 0); //强制将lib264改成h264_qsv
	//				if (support_qsv) {
	//					change_x265 = true;
	//					argc_list[i] = "hevc_qsv";
	//				}
	//			}
	//			if (change_x265) {
	//				if (argc_list[i] == "ultrafast") {
	//					argc_list[i] = "veryfast";
	//				}
	//				if (argc_list[i] == "-x265-params") {
	//					argc_list[i] = "-global_quality";//qsv not use crf
	//				}
	//				if (argc_list[i].size() >= 6 && argc_list[i].substr(0, 4) == "crf=") {
	//					// 提取后2字节（第4和第5字节，索引4和5）
	//					std::string crf_data = argc_list[i].substr(4, 2);
	//					int crf = atoi(crf_data.c_str());//硬编码crf值要比较小
	//					crf -= 3;
	//					char str[5];
	//					sprintf(str, "%d", crf);
	//					argc_list[i] = str;
	//				}
	//			}
	//		}
	//	}
	//}

	std::vector<char*>argv;
	for (size_t i = 0; i < argc; i++)
	{
		if(argc_list[i].length() > 0)
			argv.push_back((char*)argc_list[i].c_str());
	}
	return FFMPEG_ProcessCommand(argv.size(), argv.data());
}

//视频导出
MEDIALIB_API HRESULT  FFMpegCommand(char* utf8_cmds)
{
	WXAutoLock al(g_lckPlay);
	//清理内存
	ClearSource();

	BEGIN_LOG_FUNC

	int hr = -1;
	std::wstring wstr = WXBase::UTF8ToUTF16(utf8_cmds);
	WXLogW(L"%ws %ws Begin", __FUNCTIONW__, wstr.c_str());
	hr = FFMPEG_ConvertCommand(utf8_cmds);
	WXLogW(L"%ws %ws End [%08x]", __FUNCTIONW__, wstr.c_str(), hr);
	return hr;
}


//设置视频导出回调函数
MEDIALIB_API HRESULT  SetProgressCallback(ProgressCallBack callback)
{
	FFMPEG_SetProgressCallback(callback);
	return S_OK;
}

//恢复-暂停-停止视频导出
////0:run, 1:Pause, 2:Stop,
MEDIALIB_API HRESULT  SetConvertState(int state)
{
	return FFMPEG_SetConvertState(state);
}

//设置预览窗口
MEDIALIB_API HRESULT  SetRenderHwnd(VideoState* _is, HWND hwnd)
{
	WXAutoLock al(g_lckPlay);
	WXLogA("%s: %08x Begin", __FUNCTION__, _is);

	if (_is == nullptr || _is != s_pCurr || _is->abort_request)
	return E_FAIL;
	s_pCurr->Hwnd = hwnd;
	RECT rect;
	if (hwnd)
	{
		GetWindowRect(hwnd, &rect);
		s_pCurr->width = rect.right - rect.left;
		s_pCurr->height = rect.bottom - rect.top;
	}
	return S_OK;
}

//无效
MEDIALIB_API HRESULT  SetVideoFilter(VideoState* is, char* filter) {
	return S_OK;
}
//无效
MEDIALIB_API HRESULT  SetAudioFilter(VideoState* is, char* filter) {
	return S_OK;
}

//无效
MEDIALIB_API HRESULT  SelectChannel(VideoState* is, int codec_type, int streamindex) {
	return S_OK;
}

//无效
MEDIALIB_API void     BeginUpdateTimeline() {}

//无效 
MEDIALIB_API void     EndUpdateTimeline(){}

//AVFrame 转换为 JPEG PNG GIF
static HRESULT SaveAsPicture(struct AVFrame* frame, const wchar_t* wszName) {
	HRESULT hr = E_FAIL;
	WXString strFilename;
	strFilename.Format(wszName);

	//WXLogW(L" %ws %ws ", __FUNCTIONW__, strFilename.str());
	int ret = 0;
	AVFormatContext* pFormatCtx = nullptr;

	ret = avformat_alloc_output_context2(&pFormatCtx, nullptr, nullptr, strFilename.c_str());
	if (ret >= 0) {
		ret = avio_open(&pFormatCtx->pb, strFilename.c_str(), AVIO_FLAG_READ_WRITE);
		if (ret >= 0) {
			AVCodec* videoCodec = nullptr;
			if (wcsicmp(strFilename.Left(4), L".png") == 0)
				videoCodec = avcodec_find_encoder(AV_CODEC_ID_PNG);
			else if (wcsicmp(strFilename.Left(4), L".bmp") == 0)
				videoCodec = avcodec_find_encoder(AV_CODEC_ID_BMP);
			else
				videoCodec = avcodec_find_encoder(pFormatCtx->oformat->video_codec);

			AVStream* videoStream = avformat_new_stream(pFormatCtx, videoCodec);
			AVCodecContext* videoCtx = videoStream->codec;
			videoCtx->pix_fmt = videoCodec->pix_fmts[0];
			videoCtx->width = frame->width;
			videoCtx->height = frame->height;
			videoCtx->time_base.num = 1;
			videoCtx->time_base.den = 1;
			ret = avcodec_open2(videoCtx, videoCodec, NULL); //SaveAsPicture
			if (ret >= 0) {
				ret = avformat_write_header(pFormatCtx, NULL);
				if (ret >= 0) {
					AVFrame* dstFrame = frame;
					WXVideoFrame videoFrame;
					if (frame->format != videoCtx->pix_fmt) {
						videoFrame.Init(videoCtx->pix_fmt, frame->width, frame->height);
						WXVideoConvert videoConv;
						videoConv.Convert(frame, videoFrame.GetFrame());
						dstFrame = videoFrame.GetFrame();
					}

					AVPacket  packet;
					av_init_packet(&packet);
					packet.data = NULL;
					packet.size = 0;
					int got_frame = 0;
					ret = avcodec_encode_video2(videoCtx, &packet, dstFrame, &got_frame);
					if (ret >= 0) {
						packet.stream_index = videoStream->index;
						av_write_frame(pFormatCtx, &packet);
						av_write_trailer(pFormatCtx);
						av_packet_unref(&packet);
						hr = S_OK;
					}
				}
				avcodec_close(videoCtx);
			}
			avio_close(pFormatCtx->pb);
			avformat_free_context(pFormatCtx);
		}
	}
	//if (ret >= 0) {
	//	WXLogA(" %s %s OK", __FUNCTION__, strFilename.c_str());
	//}
	//else {
	//	WXLogA(" %s %s Error", __FUNCTION__, strFilename.c_str());
	//}
	return hr;
}

//HEIC AVIF 转JPEG
static HRESULT capture_heic(wchar_t* heif_name, wchar_t* output) {

	int file_size = WXBase::Filesize(heif_name);
	if (file_size == 0)
		return E_FAIL;

	utils::ML_FileBuffer file_buffer;
	file_buffer.read_file(heif_name, file_size);

	
	HRESULT hr = E_FAIL;
	{
		//HEIC 文件容器
		std::shared_ptr<heif_context> m_ctx = std::shared_ptr<heif_context>(heif_context_alloc(),
			[](heif_context* c) { heif_context_free(c); });

		struct heif_error err;
		err = heif_context_read_from_memory(m_ctx.get(), file_buffer.ptr<uint8_t>(), file_size, nullptr);
		if (err.code != 0) {
			//WXLogW(L"heif_context_read_from_memory Error\r\n");
			return E_FAIL;
		}
		int num_images = heif_context_get_number_of_top_level_images(m_ctx.get());
		if (num_images == 0) {
			//WXLogW(L"heif_context_get_number_of_top_level_images Error\r\n");
			return E_FAIL;
		}

		heif_item_id* m_image_IDs = (heif_item_id*)av_malloc(num_images * sizeof(heif_item_id));
		heif_context_get_list_of_top_level_image_IDs(m_ctx.get(), m_image_IDs, num_images);

		//如果有多帧图像，解码第2帧
		size_t image_index = 0;  // Image filenames are "1" based.
		if (num_images > 1) {
			image_index = 1;
		}
		struct heif_image_handle* m_handle = nullptr;
		err = heif_context_get_image_handle(m_ctx.get(), m_image_IDs[image_index], &m_handle);
		if (err.code) {//获取第几帧的句柄
			//WXLogW(L" heif_context_get_image_handle Error\r\n");
			return E_FAIL;
		}

		int has_alpha = heif_image_handle_has_alpha_channel(m_handle);  //是否包含透明通道
		struct heif_decoding_options* m_decode_options = heif_decoding_options_alloc();

		int _bit_depth = heif_image_handle_get_luma_bits_per_pixel(m_handle);
		if (_bit_depth < 0) { //图像位深度
			heif_decoding_options_free(m_decode_options);
			heif_image_handle_release(m_handle);

			//WXLogW(L" heif_image_handle_get_luma_bits_per_pixel Error\r\n");
			return E_FAIL;
		}
		//解码操作
		struct heif_image* m_image = nullptr;
		err = heif_decode_image(m_handle, &m_image,
			heif_colorspace_YCbCr,
			heif_chroma_420, m_decode_options);
		if (err.code) {
			heif_image_handle_release(m_handle);
			//WXLogW(L" heif_decode_image Error\r\n");
			return E_FAIL;
		}

		if (m_image) { //解码成功
			int width = heif_image_get_width(m_image, heif_channel_Y);  //分辨率宽度
			int height = heif_image_get_height(m_image, heif_channel_Y);//分辨率高度

			int stride_y = 0;
			const uint8_t* src_y = heif_image_get_plane_readonly(m_image, heif_channel_Y, &stride_y);
			int stride_u = 0;
			const uint8_t* src_u = heif_image_get_plane_readonly(m_image, heif_channel_Cb, &stride_u);
			int stride_v = 0;
			const uint8_t* src_v = heif_image_get_plane_readonly(m_image, heif_channel_Cr, &stride_v);

			WXVideoFrame argbFrame;
			argbFrame.Init(AV_PIX_FMT_RGB32, width, height);

			libyuv::J420ToARGB(src_y, stride_y,
				src_u, stride_u,
				src_v, stride_v,
				argbFrame.GetFrame()->data[0], width * 4,
				width, height
			);


			heif_image_release(m_image);

			//GDI+
			hr = SaveAsPicture(argbFrame.GetFrame(), output);//转码输出
		}
		heif_decoding_options_free(m_decode_options); //解码出图像
		heif_image_handle_release(m_handle);
	}
	return S_OK;
}

//图像转格式
MEDIALIB_API HRESULT  CaptureHEIC(wchar_t* filename, wchar_t* output) {
	HRESULT hr = S_OK;
	hr = capture_heic(filename, output);
	return hr;
}


// 获取图像编码器的 CLSID
static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT num = 0;          // 图像编码器的数量
	UINT size = 0;         // 图像编码器的大小

	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0 || num == 0) {
		return -1;  // 无编码器
	}

	Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size * num));
	if (pImageCodecInfo == 0) {
		return -1;  // 无编码器
	}

	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
	if (pImageCodecInfo == nullptr)
		return -1;
	for (UINT j = 0; j < num; ++j) {
		if (wcsicmp(pImageCodecInfo[j].MimeType, format) == 0) {
			memcpy(pClsid, &pImageCodecInfo[j].Clsid, sizeof(CLSID));
			free(pImageCodecInfo);
			return j;  // 获取到CLSID
		}
	}
	free(pImageCodecInfo);
	return -1;
}

static  HRESULT  Capture_Image_Gdiplus(const WCHAR* filename, const WCHAR* outputFile, int out_height) {

	// 加载图像
	std::shared_ptr< Gdiplus::Image> image = std::shared_ptr< Gdiplus::Image>( new Gdiplus::Image(filename));
	int width = image->GetWidth();
	int height = image->GetHeight();
	if (width <= 0 || height <= 0)
		return E_FAIL;
	int out_width = (width * out_height / height) / 4 * 4;//输出

	// 创建内存中的新位图
	std::shared_ptr < Gdiplus::Bitmap> newImage = std::shared_ptr < Gdiplus::Bitmap >(new Gdiplus::Bitmap(out_width, out_height, image->GetPixelFormat()));

	// 获取图形对象
	std::shared_ptr < Gdiplus::Graphics> graphics = std::shared_ptr <Gdiplus::Graphics >(Gdiplus::Graphics::FromImage(newImage.get()));

	// 设置缩放质量
	graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

	// 画图并缩放到新的位图
	graphics->DrawImage(image.get(), 0, 0, out_width, out_height);

	// 保存新的图像
	CLSID pngClsid;
	const wchar_t* Left4 = filename + (wcslen(filename) - 4);
	if (wcsicmp(Left4, L".png") == 0) {
		GetEncoderClsid(L"image/png", &pngClsid);
		newImage->Save(outputFile, &pngClsid, NULL);
	}else if (wcsicmp(Left4, L".jpg") == 0 || wcsicmp(Left4, L"jpeg") == 0) {
		GetEncoderClsid(L"image/jpeg", &pngClsid);
		newImage->Save(outputFile, &pngClsid, NULL);
	}
	return S_OK;
}



//视频转换操作线程



MEDIALIB_API HRESULT  CaptureImage(char* utf8_path, float second, wchar_t* output, int height) {

	if (height > 10000 || height < 0) {
		height = 360;
	}
	//先根据文件判断输入是否图像
	std::string strInput = utf8_path;
	std::wstring wstrInput = WXBase::UTF8ToUTF16(utf8_path);
	std::wstring wstrOutput = output;
	const wchar_t* Left4 = wstrInput.c_str() + (wstrInput.length() - 4);
	if (wcsicmp(Left4, L".png") == 0 ||
		wcsicmp(Left4, L"jpeg") == 0 ||
		wcsicmp(Left4, L".jpg") == 0) {
		return Capture_Image_Gdiplus(wstrInput.c_str(), output, height);
	}

	//WXAutoLock al(g_lckTool);
	// filename 为 "custom" 为截取轨道图像， 比如调整缩放
	capture_image((char*)strInput.c_str(), second, (wchar_t*)wstrOutput.c_str(), height);
	return S_OK;
}

MEDIALIB_API HRESULT  CaptureImageW(wchar_t* utf16_path, float second, wchar_t* output, int height) {

	if (height > 10000 || height < 0) {
		height = 360;
	}
	//先根据文件判断输入是否图像

	std::wstring wstrInput = utf16_path;// WXBase::UTF8ToUTF16(filename);
	std::string utf8_path = WXBase::UTF16ToUTF8(utf16_path);
	std::wstring wstrOutput = output;
	const wchar_t* Left4 = wstrInput.c_str() + (wstrInput.length() - 4);
	if (wcsicmp(Left4, L".png") == 0 ||
		wcsicmp(Left4, L"jpeg") == 0 ||
		wcsicmp(Left4, L".jpg") == 0) {
		return Capture_Image_Gdiplus(wstrInput.c_str(), output, height);
	}

	//WXAutoLock al(g_lckTool);
	// filename 为 "custom" 为截取轨道图像， 比如调整缩放
	capture_image((char*)utf8_path.c_str(), second, (wchar_t*)wstrOutput.c_str(), height);
	return S_OK;
}

//无效
MEDIALIB_API HRESULT  DetectAudio(char* filename, char* output) {
	return S_OK;
}


MEDIALIB_API  Stream_Info* GetStreamInfo(char* path, INT32* psize, int64_t* duration) {

	//WXAutoLock al(g_lckTool);
	auto dst = AnalyzeMedia2(path);
	if (dst == NULL)return NULL;
	Stream_Info* streams = NULL;
	{ //GetStreamInfo
		AVFormatContext * pFormatCtx;
		pFormatCtx = avformat_alloc_context();
		int ret = avformat_open_input(&pFormatCtx, path, NULL, NULL);
		int audioindex = -1;
		avformat_find_stream_info(pFormatCtx, 0);
		int bestvideo = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
		streams = (Stream_Info*)calloc(sizeof(Stream_Info), pFormatCtx->nb_streams);
		*psize = pFormatCtx->nb_streams;
		for (UINT i = 0; i < pFormatCtx->nb_streams; i++) {

			AVCodecContext* codec = pFormatCtx->streams[i]->codec;
			if (codec->codec_id == AV_CODEC_ID_NONE)
			{
				continue;
			}

			streams[i].width = codec->width;
			double ratio = av_q2d(codec->sample_aspect_ratio);
			if (ratio > 1)
			{
				streams[i].width = (int)streams[i].width * ratio;
			}
			streams[i].width = streams[i].width / 2 * 2;
			streams[i].height = codec->height;
			streams[i].height = streams[i].height / 2 * 2;


			streams[i].codec_type = codec->codec_type;
			streams[i].index = pFormatCtx->streams[i]->index;
			streams[i].SampleRate = codec->sample_rate;
			if (codec->codec_type == AVMEDIA_TYPE_AUDIO) {
				audioindex = i;
				if (codec->sample_rate == 0 || dst->audiocount == 0)
				{
					streams[i].codec_type = AVMEDIA_TYPE_DATA;
				}
			}
			av_get_channel_layout_string(streams[i].channel_layout, 256, av_get_channel_layout_nb_channels(codec->channel_layout), codec->channel_layout);
			streams[i].BitRate = codec->bit_rate;
			if (codec->framerate.num * codec->framerate.den != 0) {
				streams[i].FrameRate = (double)codec->framerate.num / (double)codec->framerate.den;
			}

			AVDictionaryEntry* t = NULL;
			if (t = av_dict_get(pFormatCtx->streams[i]->metadata, "Title", NULL, 0)) {
				sprintf(streams[i].title, t->value);
			}
			if (t = av_dict_get(pFormatCtx->streams[i]->metadata, "Language", NULL, 0)) {
				sprintf(streams[i].language, t->value);
			}
			sprintf(streams[i].codec_name, avcodec_get_name(codec->codec_id));
		}
		avformat_close_input(&pFormatCtx);
	}
	return streams;
}

MEDIALIB_API void  SetWaterMark(char* water) {
	//WXAutoLock al(g_lckTool);
	FFMPEG_SetWaterMark(water);
}

//无效
MEDIALIB_API void  ProcessFilterCommand(VideoState* is, char* command) {}

MEDIALIB_API HRESULT  SetPlayField(VideoState* is, char* field, char* value) {
	HRESULT ret = S_OK;
#if CONFIG_AVFILTER
	WXLogA("SetPlayField:%s-%s", field, value);
	if (!is) {
		return -1;
	}
	if (strcmp(field, "speed") == 0) {
		ret = sscanf(value, "%f", &is->speed);
		is->reinitaudiofilter = TRUE;
		is->reinitvideofilter = TRUE;
	}
	else	 if (strcmp(field, "adelay") == 0) {
		ret = sscanf(value, "%f", &is->adelay);
		is->adelay = is->adelay / 1000;
		is->reinitaudiofilter = TRUE;
	}
	else if (strcmp(field, "subtitles") == 0) {
		is->subtitles = _strdup(value);
		is->reinitvideofilter = TRUE;
	}
	else if (strcmp(field, "duration") == 0) {
		ret = sscanf(value, "%" SCNd64, &is->ic->duration);
	}
#endif
	return ret;
}

MEDIALIB_API HRESULT  SetField(char* field, char* value) {
	return FFMPEG_SetField(field, value);
}

MEDIALIB_API HRESULT  SetFieldInt(char* field, int value) {
	return FFMPEG_SetFieldInt(field, value);

}

//无效
MEDIALIB_API HRESULT   RemoveSource(char* name) {
	return S_OK;
}


//构造Index缓存
MEDIALIB_API  int  BuildIndex(char* source, char* cachefile)
{
	int Track = -1;
	FFMS_ProcessIndex(source, &Track);
	return 1;
}

//---------------------- 视频文件关键帧 -----------------------

static WXLocker s_lckKeyTime;
//视频文件关键帧时间戳
static std::map<std::string, std::vector<int>>s_mapKeyTime;  //类似于 KeyFrame 的 Index

//分析视频文件关键帧
MEDIALIB_API int WXAnylizeVideoKeyTime(const char* strName) {
	WXAutoLock al(s_lckKeyTime);
	std::string str = strName;
	if (s_mapKeyTime.count(str) > 0) {
		return 0;
	}

	//int64_t s_nNum = 0;
	AVFormatContext* fCtx = avformat_alloc_context();
	std::shared_ptr<AVFormatContext>pFmtCtx = std::shared_ptr<AVFormatContext>(fCtx,
		[](AVFormatContext* fmtCtx) {
			if (fmtCtx) {
				avformat_close_input(&fmtCtx);
				avformat_free_context(fmtCtx);
				fmtCtx = nullptr;
			}
		}
	);

	if (avformat_open_input(&fCtx, strName, NULL, NULL) < 0) { //指定格式
		return -1; //不能打开文件
	}

	if (avformat_find_stream_info(pFmtCtx.get(), NULL) < 0) { //无法解析
		pFmtCtx = nullptr;
		return -1;
	}

	int video_index = -1;
	AVRational tbVideo;
	AVStream* pVStream = nullptr;
	for (int i = 0; i < pFmtCtx->nb_streams; i++) {
		if (pFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
			!(pFmtCtx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)) {
			video_index = i;
			pVStream = pFmtCtx->streams[i];
			tbVideo.den = pVStream->time_base.den;
			tbVideo.num = pVStream->time_base.num;
			break;
		}
	}

	int64_t hasData = 0;
	if (video_index != -1) {

		AVPacket pkt;
		while (av_read_frame(pFmtCtx.get(), &pkt) != AVERROR_EOF) { //WXAnylizeVideoKeyTime 读取一个包
			if (pkt.stream_index == video_index) {
				if (pkt.flags & AV_PKT_FLAG_KEY) {
					//时间戳转换为MS
					int pkt_pts = ((float)pkt.pts * 1000.0f * (float)tbVideo.num / (float)tbVideo.den + 999.0f) / 1000.0f;
					//float pkt_dts = (float)pkt.dts * 1000.0f * (float)tbVideo.num / (float)tbVideo.den;
					s_mapKeyTime[str].push_back(pkt_pts);//精度为秒
					hasData++;
				}
				//s_nNum++;
			}
			av_packet_unref(&pkt);//释放包
		}
	}

	pFmtCtx = nullptr;//关闭文件
	return hasData ? 0 : -1;
}

//查询对应视频文件关键帧个数
MEDIALIB_API int WXGetKeyTimeNum(const char* strName) {
	WXAutoLock al(s_lckKeyTime);
	std::string str = strName;
	if (s_mapKeyTime.count(str) > 0) {
		return s_mapKeyTime[str].size();
	}
	return 0;
}

//查询对应视频文件关键帧时间戳(单位秒)
MEDIALIB_API int* WXGetKeyTimeData(const char* strName){
	WXAutoLock al(s_lckKeyTime);
	std::string str = strName;
	if (s_mapKeyTime.count(str) > 0) {
		return s_mapKeyTime[str].data();
	}
	return nullptr;
}

//-------------------- 视频分割文件 -----------------------------

extern "C" {
	typedef struct WXCtx WXCtx;
	// in ffmpeg.c
	extern WXCtx*  avffmpeg_create();
	extern void    avffmpeg_destroy(WXCtx* octx);
	extern int     avffmpeg_convert(WXCtx* octx, int argc, char** argv);
	extern void    avffmpeg_interrupt();
	extern int64_t avffmpeg_getCurrTime(WXCtx* octx);
	extern int64_t avffmpeg_getTotalTime(WXCtx* octx);
	extern int     avffmpeg_getState(WXCtx* octx);
}


//---------------------------------------------------------------------------------------------------------
//从原视频文件切割多个时间段合并成新的视频文件
//成功返回0，失败返回小于0的值
#define TYPE_COPY 0 //不重编码处理，速度最快，但是位置可能不准
#define TYPE_SW   1 //软编码
#define TYPE_HW   2 //硬编码

MEDIALIB_API int WXCutVideoFile(const char* szInput,//输入文件
	const char* szOutput,  //输出文件
	int nNum, // 分段格式 对应的 ptsTime[2*n] ptsTime[2*n+1] 是一组开始结束时间
	int64_t* ptsTime  //裁剪时间, 有nNum*2 个
	, int type //处理类型
) {
	std::wstring wstrOutput = WXBase::UTF8ToUTF16(szOutput);
	const wchar_t* wszOutput = wstrOutput.c_str();

	std::wstring wstrInput = WXBase::UTF8ToUTF16(szInput);
	const wchar_t* wszInput = wstrInput.c_str();

	std::vector<const char*> argv;
	std::vector <WXString> arrStrArgv;

	//输入文件解析
	wchar_t fileName[MAX_PATH + 1] = { 0 };
	wcscpy(fileName, wszOutput);
	//获取exe执行文件名字(去掉后缀".mp4")
	*wcsrchr(fileName, L'.') = '\0';   // 从最左边开始最后一次出现"."的位置(注：strchr/strrchr函数使用)
	int ret = -1;
	if (nNum > 0) {
		std::vector < WXString> arrStrOutputTemp(nNum);//
		for (size_t i = 0; i < nNum; i++) {
			argv.clear();
			arrStrArgv.clear();
			arrStrOutputTemp[i].Format(L"%ws.temp.%d.ts", fileName, i);
			arrStrArgv.push_back(L"ffmpeg");
			arrStrArgv.push_back(L"-ss");
			WXString wstr;
			wstr.Format("%d", (int)ptsTime[i * 2]); //开始时间
			arrStrArgv.push_back(wstr);
			arrStrArgv.push_back(L"-t");
			wstr.Format("%d", (int)(ptsTime[i * 2 + 1] - ptsTime[i * 2]));//结束时间
			arrStrArgv.push_back(wstr);

			arrStrArgv.push_back(L"-i");
			arrStrArgv.push_back(wszInput);

			if (type == TYPE_COPY) { //快速裁剪
				arrStrArgv.push_back(L"-c");
				arrStrArgv.push_back(L"copy");
			}
			else if (type == TYPE_SW) { //h264+aac
				arrStrArgv.push_back(L"-c:a");
				arrStrArgv.push_back(L"aac");
				arrStrArgv.push_back(L"-c:v");
				arrStrArgv.push_back(L"h264");
			}
			else if (type == TYPE_HW) { //h264_qsv+aac
				arrStrArgv.push_back(L"-c:a");
				arrStrArgv.push_back(L"aac");
				arrStrArgv.push_back(L"-c:v");
				arrStrArgv.push_back(L"h264_qsv");
			}
			arrStrArgv.push_back(arrStrOutputTemp[i]);
			arrStrArgv.push_back(L"-y");

			WXCtx* ctx = avffmpeg_create();
			for (int i = 0; i < arrStrArgv.size(); i++)
				argv.push_back(arrStrArgv[i].c_str());

			int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
			avffmpeg_destroy(ctx);
			if (err != 0) {
				//WXLogW(L"Convert Param ------------ ");
				for (int i = 0; i < arrStrArgv.size(); i++) {
					//WXLogW(L"argv[%d] = %ws", i, arrStrArgv[i].str());
				}
				//WXLogW(L"avffmpeg_convert Error!!!!!!!!!!");
				return -1;//转换失败
			}
		}

		//return 0;

		//合并多个TS
		std::ofstream fout;
		fout.open(arrStrOutputTemp[0].str(), std::ios::app | std::ios::binary); //追加数据到第一个文件
		if (fout.is_open()) {
			for (size_t i = 1; i < nNum; i++) {
				std::ifstream fin;
				fin.open(arrStrOutputTemp[i].str(), std::ios::binary);
				if (fin.is_open()) {
#define MaxData 4096
					char temp[MaxData];
					do {
						fin.read(temp, MaxData);
						fout.write(temp, fin.gcount());
					} while (!fin.eof());
					fin.close();
				}
			}
			fout.close();

			{  //TS ----> MP4
				argv.clear();
				arrStrArgv.clear();
				arrStrArgv.push_back(L"ffmpeg");
				arrStrArgv.push_back(L"-i");
				arrStrArgv.push_back(arrStrOutputTemp[0]);
				arrStrArgv.push_back(L"-c");
				arrStrArgv.push_back(L"copy");
				arrStrArgv.push_back(wszOutput);
				arrStrArgv.push_back(L"-y");

				WXCtx* ctx = avffmpeg_create();
				for (int i = 0; i < arrStrArgv.size(); i++)
					argv.push_back(arrStrArgv[i].c_str());
				int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
				avffmpeg_destroy(ctx);

				if (err != 0) { //合并后的视频转换失败
					//WXLogW(L"Convert Param ------------ ");
					for (int i = 0; i < arrStrArgv.size(); i++) {
						//WXLogW(L"argv[%d] = %ws", i, arrStrArgv[i].str());
					}
					//WXLogW(L"Convert TS Error!!!!!!!!!!");
					ret = -1;//转换失败
				}
				else {
					ret = 0;
				}

				for (size_t i = 0; i < nNum; i++) {
					::DeleteFileW(arrStrOutputTemp[i].str()); //删除临时文件
				}
			}
		}
	}
	else {
		argv.clear();
		arrStrArgv.clear();
		arrStrArgv.push_back(L"ffmpeg");
		arrStrArgv.push_back(L"-i");
		arrStrArgv.push_back(wszInput);
		arrStrArgv.push_back(L"-c");
		arrStrArgv.push_back(L"copy");
		arrStrArgv.push_back(wszOutput);
		arrStrArgv.push_back(L"-y");

		WXCtx* ctx = avffmpeg_create();
		for (int i = 0; i < arrStrArgv.size(); i++)
			argv.push_back(arrStrArgv[i].c_str());
		int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
		avffmpeg_destroy(ctx);

		if (err != 0) {
			//WXLogW(L"Convert Param ------------ ");
			for (int i = 0; i < arrStrArgv.size(); i++) {
				//WXLogW(L"argv[%d] = %ws", i, arrStrArgv[i].str());
			}
			//WXLogW(L"Convert TS Error!!!!!!!!!!");
			ret = -1;//转换失败
		}
	}
	return ret;
}

//停止截取预览图

static BOOL s_PauseCapture = FALSE;

MEDIALIB_API HRESULT   StopCaptureThumb() {
	s_PauseCapture = TRUE;
	return S_OK;
}

// 从 std::wstring 中获取文件后缀名
std::wstring getFileExtension(const std::wstring& filePath) {
	// 查找最后一个点的位置
	size_t dotPos = filePath.find_last_of(L".");
	// 如果找到了点且不是字符串的最后一个字符
	if (dotPos != std::wstring::npos && dotPos < filePath.length() - 1) {
		// 从点的下一个位置开始提取后缀名
		return filePath.substr(dotPos + 1);
	}
	// 如果没有找到合适的点，返回空字符串
	return L"";
}

//ffmpeg.exe 截取多种图片预览
//在程序初始化是检测是否可以调用ffmpeg.exe
//如果不能调用ffmpeg.exe 则使用ffmpeg.dll
//判断是否TS后缀名，如果是，-ss 在 -i 之前 截图可能会模糊
//优先使用DXVA硬解码，如果失败切换到软解码
MEDIALIB_API HRESULT  CaptureImages(char* utf8_path, INT64* _seconds, int _size, wchar_t* _output){
	//WXAutoLock al(g_lckTool);
	
	auto dst = AnalyzeMedia2(utf8_path);
	if (dst == NULL)
		return E_FAIL;

	//LOG_FUNC

	s_PauseCapture = FALSE;//重置

	std::wstring wstrInput = WXBase::UTF8ToUTF16(utf8_path);
	std::wstring wstrOutput = _output;
	BOOL bTS = FALSE;
	std::wstring Extend = getFileExtension(wstrInput);
	if (Extend == L"ts" || Extend == L"TS") {
		bTS = TRUE;
	}

	for (int index = 0; index < _size; index++){
		INT64 time = _seconds[index];
		const wchar_t* wszInput = wstrInput.c_str();
		WXString wxstrOutput;;//输出文件
		wxstrOutput.Format(L"%ws_%lld.jpg", wstrOutput.c_str(), time);

		if (s_bFfmpegExe)
		{
			WXString strCmd;
			//默认使用硬解码
			strCmd.Format(L" -hwaccel dxva2  -accurate_seek   -ss %f  -i \"%ws\"  -vf scale=-2:80 -vframes 1 -f image2 \"%ws\" -y",
				(double)time / 1000000.0f,
				wszInput,
				wxstrOutput.str());
			ExecuteFfmpegExe(strCmd.str());
			int nExist = WXBase::Exists(wxstrOutput.c_str());
			int nSize = WXBase::Filesize(wxstrOutput.str());
			if (nExist == 0 || nSize == 0) {//切换到软解码
				strCmd.Format(L" -accurate_seek   -ss %f  -i \"%ws\"  -vf scale=-2:80 -vframes 1 -f image2 \"%ws\" -y",
					(double)time / 1000000.0f,
					wszInput,
					wxstrOutput.str());
				ExecuteFfmpegExe(strCmd.str());
				nExist = WXBase::Exists(wxstrOutput.c_str());
				nSize = WXBase::Filesize(wxstrOutput.str());
				if (nExist == 0 || nSize == 0) {
					WXLogW(L"CaptureImages ffmpeg.exe Error");
					return E_FAIL;
				}
			}
		}else {
			std::vector<const char*> argv;
			std::vector <WXString> arrStrArgv;
			arrStrArgv.push_back(L"ffmpeg");
			arrStrArgv.push_back(L"-ss");
			WXString wxstrTime;
			wxstrTime.Format(L"%f", (double)(time) / 1000000.0);
			arrStrArgv.push_back(wxstrTime);//参数位毫秒
			arrStrArgv.push_back(L"-i");
			arrStrArgv.push_back(wszInput);//输入文件
			arrStrArgv.push_back(L"-vf");
			arrStrArgv.push_back(L"scale=-2:80");
			arrStrArgv.push_back(L"-vframes");
			arrStrArgv.push_back(L"1");
			arrStrArgv.push_back(L"-f");
			arrStrArgv.push_back(L"image2");
			arrStrArgv.push_back(wxstrOutput);//输出文件

			WXCtx* ctx = avffmpeg_create();
			for (int argc = 0; argc < arrStrArgv.size(); argc++)
				argv.push_back(arrStrArgv[argc].c_str());
			int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
			avffmpeg_destroy(ctx);

			int nExist = WXBase::Exists(wxstrOutput.c_str());
			int nSize = WXBase::Filesize(wxstrOutput.str());
			if (nExist == 0 || nSize == 0) {
				WXLogW(L"CaptureImages Impl Error");
				return E_FAIL;
			}
		}
	}
	
	return S_OK;
}

//ffmpeg.exe 截取多种图片预览
//在程序初始化是检测是否可以调用ffmpeg.exe
//如果不能调用ffmpeg.exe 则使用ffmpeg.dll
//判断是否TS后缀名，如果是，-ss 在 -i 之前 截图可能会模糊
//优先使用DXVA硬解码，如果失败切换到软解码
MEDIALIB_API HRESULT  CaptureImagesW(wchar_t* utf16_path, INT64* _seconds, int _size, wchar_t* _output) {
	//WXAutoLock al(g_lckTool);

	auto dst = AnalyzeMediaW2(utf16_path);
	if (dst == NULL)
		return E_FAIL;

	//LOG_FUNC

	s_PauseCapture = FALSE;//重置

	std::wstring wstrInput = utf16_path;// WXBase::UTF8ToUTF16(_filename);
	std::wstring wstrOutput = _output;
	BOOL bTS = FALSE;
	std::wstring Extend = getFileExtension(wstrInput);
	if (Extend == L"ts" || Extend == L"TS") {
		bTS = TRUE;
	}

	for (int index = 0; index < _size; index++) {
		INT64 time = _seconds[index];
		const wchar_t* wszInput = wstrInput.c_str();
		WXString wxstrOutput;;//输出文件
		wxstrOutput.Format(L"%ws_%lld.jpg", wstrOutput.c_str(), time);

		if (s_bFfmpegExe)
		{
			WXString strCmd;
			//默认使用硬解码
			strCmd.Format(L" -hwaccel dxva2  -accurate_seek   -ss %f  -i \"%ws\"  -vf scale=-2:80 -vframes 1 -f image2 \"%ws\" -y",
				(double)time / 1000000.0f,
				wszInput,
				wxstrOutput.str());
			ExecuteFfmpegExe(strCmd.str());
			int nExist = WXBase::Exists(wxstrOutput.c_str());
			int nSize = WXBase::Filesize(wxstrOutput.str());
			if (nExist == 0 || nSize == 0) {//切换到软解码
				strCmd.Format(L" -accurate_seek   -ss %f  -i \"%ws\"  -vf scale=-2:80 -vframes 1 -f image2 \"%ws\" -y",
					(double)time / 1000000.0f,
					wszInput,
					wxstrOutput.str());
				ExecuteFfmpegExe(strCmd.str());
				nExist = WXBase::Exists(wxstrOutput.c_str());
				nSize = WXBase::Filesize(wxstrOutput.str());
				if (nExist == 0 || nSize == 0) {
					WXLogW(L"CaptureImages ffmpeg.exe Error");
					return E_FAIL;
				}
			}
		}
		else {
			std::vector<const char*> argv;
			std::vector <WXString> arrStrArgv;
			arrStrArgv.push_back(L"ffmpeg");
			arrStrArgv.push_back(L"-ss");
			WXString wxstrTime;
			wxstrTime.Format(L"%f", (double)(time) / 1000000.0);
			arrStrArgv.push_back(wxstrTime);//参数位毫秒
			arrStrArgv.push_back(L"-i");
			arrStrArgv.push_back(wszInput);//输入文件
			arrStrArgv.push_back(L"-vf");
			arrStrArgv.push_back(L"scale=-2:80");
			arrStrArgv.push_back(L"-vframes");
			arrStrArgv.push_back(L"1");
			arrStrArgv.push_back(L"-f");
			arrStrArgv.push_back(L"image2");
			arrStrArgv.push_back(wxstrOutput);//输出文件

			WXCtx* ctx = avffmpeg_create();
			for (int argc = 0; argc < arrStrArgv.size(); argc++)
				argv.push_back(arrStrArgv[argc].c_str());
			int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
			avffmpeg_destroy(ctx);

			int nExist = WXBase::Exists(wxstrOutput.c_str());
			int nSize = WXBase::Filesize(wxstrOutput.str());
			if (nExist == 0 || nSize == 0) {
				WXLogW(L"CaptureImages Impl Error");
				return E_FAIL;
			}
		}
	}

	return S_OK;
}

MEDIALIB_API HRESULT SplitAudioW(wchar_t* utf16_in, wchar_t* utf16_out) {
	HRESULT hr = 0;
	//WXAutoLock al(g_lckTool);


	if (s_bFfmpegExe) {
		std::wstring wstrInput = utf16_in;
		std::wstring wstrOutput = utf16_out;
		WXString strCmd;
		strCmd.Format(L" -i \"%ws\" -ar 8000 -ac 1 -c:a pcm_u8 -f wav \"%ws\" -y", wstrInput.c_str(), wstrOutput.c_str());
		ExecuteFfmpegExe(strCmd.str());
		if (!WXBase::Exists(wstrOutput.c_str())) {
			WXLogW(L"SplitAudio Convert Error!!!------------ \"%ws\" %ws", s_strFfmpegExe.c_str(), strCmd.str());
			return E_FAIL;
		}
		else {
			if (!WXBase::Filesize(wstrOutput.c_str())) {
				WXLogW(L"SplitAudio Convert Empty !!!------------ \"%ws\"  %ws", s_strFfmpegExe.c_str(), strCmd.str());
				return E_FAIL;
			}
		}
		return S_OK;
	}
	else { //SplitAudio
		std::string strInput = WXBase::UTF16ToUTF8(utf16_in);
		std::string strOutput = WXBase::UTF16ToUTF8(utf16_out);
		std::vector<const char*> argv;
		argv.push_back("ffmpeg");
		argv.push_back("-i");
		argv.push_back(strInput.c_str());//输入文件
		argv.push_back("-ar");
		argv.push_back("8000");
		argv.push_back("-ac");
		argv.push_back("1");
		argv.push_back("-c:a");
		argv.push_back("pcm_u8");
		argv.push_back("-f");
		argv.push_back("wav");
		argv.push_back(strOutput.c_str());//输出文件

		WXCtx* ctx = avffmpeg_create();
		int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
		avffmpeg_destroy(ctx);
		if (!WXBase::Exists(utf16_out)) {
			WXLogW(L"SplitAudio Convert Error!!!");
			return E_FAIL;
		}
		else {
			if (!WXBase::Filesize(utf16_out)) {
				WXLogW(L"SplitAudio Convert Empty!!!");
				return E_FAIL;
			}
		}
	}
	return hr;
}
MEDIALIB_API HRESULT  SplitAudio(char* filename, char* output) {
	HRESULT hr = 0;

	WXLogA("%s %ws %ws", __FUNCTION__, filename, output);
	std::wstring wstrInput = WXBase::UTF8ToUTF16(filename);
	std::wstring wstrOutput = WXBase::UTF8ToUTF16(output);
	WXLogW(L"%ws %ws %ws",__FUNCTIONW__, wstrInput.c_str(), wstrOutput.c_str());

	if (s_bFfmpegExe) {
		WXString strCmd;
		strCmd.Format(L" -i \"%ws\" -ar 8000 -ac 1 -c:a pcm_u8 -f wav \"%ws\" -y", wstrInput.c_str(), wstrOutput.c_str());
		ExecuteFfmpegExe(strCmd.str());
		if (!WXBase::Exists(wstrOutput.c_str())) {
			WXLogW(L"SplitAudio Convert Error!!!------------ \"%ws\" %ws",s_strFfmpegExe.c_str(),  strCmd.str());
			return E_FAIL;
		}else{
			if (!WXBase::Filesize(wstrOutput.c_str())) {
				WXLogW(L"SplitAudio Convert Empty !!!------------ \"%ws\"  %ws", s_strFfmpegExe.c_str(), strCmd.str());
				return E_FAIL;
			}
		}
		return S_OK;
	}else{ //SplitAudio
		std::string strInput = filename;
		std::string strOutput = output;
		std::vector<const char*> argv;
		argv.push_back("ffmpeg");
		argv.push_back("-i");
		argv.push_back(strInput.c_str());//输入文件
		argv.push_back("-ar");
		argv.push_back("8000");
		argv.push_back("-ac");
		argv.push_back("1");
		argv.push_back("-c:a");
		argv.push_back("pcm_u8");
		argv.push_back("-f");
		argv.push_back("wav");
		argv.push_back(strOutput.c_str());//输出文件

		WXCtx* ctx = avffmpeg_create();
		int err = avffmpeg_convert(ctx, argv.size(), (char**)argv.data());
		avffmpeg_destroy(ctx);
		if (!WXBase::Exists(wstrOutput.c_str())) {
			WXLogW(L"SplitAudio Convert Error!!!");
			return E_FAIL;
		}
		else {
			if (!WXBase::Filesize(wstrOutput.c_str())) {
				WXLogW(L"SplitAudio Convert Empty!!!");
				return E_FAIL;
			}
		}
	}
	return hr;
}




MEDIALIB_API HRESULT SetCrashReportPath(WCHAR* path)
{
	ML_SetString(L"CrashReportPath", path);
	return 0;
}

EXTERN_C static LONG ApplicationCrashHandler(EXCEPTION_POINTERS* pException)
{
	if (LibInst::GetInst().m_libDbghelp == nullptr)
		return -1;
	std::wstring s_dumppath = ML_GetString(L"dump");
	HANDLE hDumpFile = CreateFile(s_dumppath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile == INVALID_HANDLE_VALUE) {
	//	WXLogW(L"%ws CreateFile %ws Error",__FUNCTIONW__, s_dumppath.c_str());
		return -1;
	}
	else {
		// Dump信息
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;
		// 写入Dump文件内容
		LibInst::GetInst().mMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}

	std::wstring strCrashrReportPath = ML_GetString(L"CrashReportPath");
	ExecuteProcess(strCrashrReportPath,20*1000);
	return EXCEPTION_EXECUTE_HANDLER;
}

//dump 
MEDIALIB_API int  InitExceptionFilter(const char* _dumppath)
{
	std::wstring strDump = WXBase::UTF8ToUTF16(_dumppath);
	ML_SetString(L"dump", strDump.c_str());
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler);
	return 0;
}

HINSTANCE g_instance = nullptr;
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	g_instance = instance;
    return TRUE;
}



//全局参数设置读取
//全局参数设置
class WXMediaGlobalValue {
public:
	static std::map<std::wstring, int>& GetInt() {
		static std::map<std::wstring, int>s_mapGlobalValue;
		return s_mapGlobalValue;
	}
	static std::map<std::wstring, std::wstring>& GetString() {
		static std::map<std::wstring, std::wstring>s_mapGlobalString;
		return s_mapGlobalString;
	}
};
MEDIALIB_API void           ML_SetString(const wchar_t* name, const wchar_t* value) {
	std::wstring strName = name;
	WXMediaGlobalValue::GetString()[strName] = value;
}
MEDIALIB_API const wchar_t* ML_GetString(const wchar_t* name) {
	std::wstring strName = name;
	if (WXMediaGlobalValue::GetString().count(strName)) {
		return WXMediaGlobalValue::GetString()[strName].c_str();
	}
	return L"";
}

MEDIALIB_API void           ML_SetValue(const wchar_t* name, int value) {
	std::wstring strName = name;
	WXMediaGlobalValue::GetInt()[strName] = value;
}
MEDIALIB_API int ML_GetValue(const wchar_t* name) {
	std::wstring strName = name;
	if (WXMediaGlobalValue::GetInt().count(strName)) {
		return WXMediaGlobalValue::GetInt()[strName];
	}
	return 0;
}


//快速合并同规格的多个TS/MP4文件
MEDIALIB_API int ML_FastMerge(WXCTSTR outFilename, int inCount, WXCTSTR* arrInput) {
	if (inCount < 1)return WX_ERROR_ERROR;
	WXString strOutput;
	strOutput.Format(outFilename);
	int m_bInit = FALSE;
	AVFormatContext* m_pOutputFmtCtx = NULL;
	AVStream* m_pOutputVideoStream = nullptr;
	AVCodecContext* m_pOutputVideoCtx = nullptr;
	AVStream* m_pOutputAudioStream = nullptr;
	AVCodecContext* m_pOutputAudioCtx = nullptr;

	int64_t  ptsTime = 0;
	int nAudioIndex = -1;//输出音频Track
	int nVideoIndex = -1;//输出视频Track
	//输入文件处理
	for (int i = 0; i < inCount; i++) {
		WXString strFileName;
		strFileName.Format(arrInput[i]);

		AVFormatContext* m_pInputFmtCtx = avformat_alloc_context(); //第一个文件解析
		if (avformat_open_input(&m_pInputFmtCtx, strFileName.c_str(), NULL, NULL) < 0) { 
			continue;
		}

		if (avformat_find_stream_info(m_pInputFmtCtx, NULL) < 0) {
			avformat_close_input(&m_pInputFmtCtx);
			avformat_free_context(m_pInputFmtCtx);
			continue;
		}
		AVCodecContext* pVideoCtx = nullptr;
		AVStream* pVideoStream = nullptr;
		AVCodec* videoCodec = nullptr;
		AVRational tbVideo{ 1,1000 }; //转ms
		int video_index = -1;//输入视频Track

		AVCodecContext* pAudioCtx = nullptr;
		AVStream* pAudioStream = nullptr;
		AVRational tbAudio{ 1,1000 }; //转ms
		AVCodec* audioCodec = nullptr;
		int audio_index = -1;//输入音频Track
		for (int i = 0; i < m_pInputFmtCtx->nb_streams; i++) {
			if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && pVideoCtx == nullptr) {
				pVideoStream = m_pInputFmtCtx->streams[i];
				pVideoCtx = pVideoStream->codec;
				video_index = i;
				tbVideo.den = pVideoStream->time_base.den;
				tbVideo.num = pVideoStream->time_base.num;
			}
			if (m_pInputFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO && pAudioCtx == nullptr) {
				pAudioStream = m_pInputFmtCtx->streams[i];
				pAudioCtx = pAudioStream->codec;
				audio_index = i;
				tbAudio.den = pAudioStream->time_base.den;
				tbAudio.num = pAudioStream->time_base.num;
			}
		}
		if (pAudioCtx && pVideoCtx) { //输入文件有音视频数据
			if (!m_bInit) {  //初始化输出文件
				avformat_alloc_output_context2(&m_pOutputFmtCtx, NULL, NULL, strOutput.c_str());
				if (m_pOutputFmtCtx == NULL)return -1;

				m_pOutputVideoStream = avformat_new_stream(m_pOutputFmtCtx, videoCodec);
				avcodec_copy_context(m_pOutputVideoStream->codec, pVideoCtx);
				m_pOutputVideoStream->codec->codec_tag = 0;
				nVideoIndex = m_pOutputVideoStream->index;
				m_pOutputVideoCtx = m_pOutputVideoStream->codec;
				if (m_pOutputFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
					m_pOutputVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				m_pOutputVideoStream->time_base.den = pVideoStream->time_base.den;
				m_pOutputVideoStream->time_base.num = pVideoStream->time_base.num;

				m_pOutputAudioStream = avformat_new_stream(m_pOutputFmtCtx, audioCodec);
				avcodec_copy_context(m_pOutputAudioStream->codec, pAudioCtx);
				m_pOutputAudioCtx = m_pOutputAudioStream->codec;
				m_pOutputAudioStream->codec->codec_tag = 0;
				if (m_pOutputFmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
					m_pOutputAudioStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				nAudioIndex = m_pOutputAudioStream->index;
				m_pOutputAudioStream->time_base.den = pAudioStream->time_base.den;
				m_pOutputAudioStream->time_base.num = pAudioStream->time_base.num;

				av_dump_format(m_pOutputFmtCtx, 0, strOutput.c_str(), 1);
				if (!(m_pOutputFmtCtx->oformat->flags & AVFMT_NOFILE)) {
					if (avio_open(&m_pOutputFmtCtx->pb, strOutput.c_str(), AVIO_FLAG_WRITE) < 0) {
						avformat_close_input(&m_pInputFmtCtx);
						avformat_free_context(m_pInputFmtCtx);
						return WX_ERROR_ERROR;
					}
				}
				//Write file header
				if (avformat_write_header(m_pOutputFmtCtx, NULL) < 0) {
					avformat_close_input(&m_pInputFmtCtx);
					avformat_free_context(m_pInputFmtCtx);
					return WX_ERROR_ERROR;
				}
				m_bInit = TRUE; //输入初始化
			}
			if (m_bInit) { //从头开始处理文件并输出
				int64_t ptsFileVideo = 0;
				int64_t ptsFileAudio = 0;

				AVPacket pkt;
				while (av_read_frame(m_pInputFmtCtx, &pkt) != AVERROR_EOF) {
					if (pkt.stream_index == video_index) {
						float pkt_pts = (float)pkt.pts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS
						float pkt_dts = (float)pkt.dts * 1000.0f * (float)pVideoStream->time_base.num / (float)pVideoStream->time_base.den;//时间戳转换为MS

						pkt_dts = pkt_dts + ptsTime;
						pkt_pts = pkt_pts + ptsTime;
						ptsFileVideo = FFMAX(pkt_dts, ptsFileVideo);

						pkt.stream_index = nVideoIndex;
						pkt.pts = pkt_pts * (float)m_pOutputVideoStream->time_base.den / (1000.0f * (float)m_pOutputVideoStream->time_base.num);//时间戳转换为MS
						pkt.dts = pkt_dts * (float)m_pOutputVideoStream->time_base.den / (1000.0f * (float)m_pOutputVideoStream->time_base.num);//时间戳转换为MS
						av_interleaved_write_frame(m_pOutputFmtCtx, &pkt);
					}
					else if (pkt.stream_index == audio_index) {
						float pkt_pts = (float)pkt.pts * 1000.0f * (float)pAudioStream->time_base.num / (float)pAudioStream->time_base.den;//时间戳转换为MS
						float pkt_dts = (float)pkt.dts * 1000.0f * (float)pAudioStream->time_base.num / (float)pAudioStream->time_base.den;//时间戳转换为MS

						pkt_dts = pkt_dts + ptsTime;
						pkt_pts = pkt_pts + ptsTime;
						ptsFileAudio = FFMAX(pkt_dts, ptsFileAudio);

						pkt.stream_index = nAudioIndex;
						pkt.pts = pkt_pts * (float)m_pOutputAudioStream->time_base.den / (1000.0f * (float)m_pOutputAudioStream->time_base.num);//时间戳转换为MS
						pkt.dts = pkt_dts * (float)m_pOutputAudioStream->time_base.den / (1000.0f * (float)m_pOutputAudioStream->time_base.num);//时间戳转换为MS
						av_interleaved_write_frame(m_pOutputFmtCtx, &pkt);
					}
					av_packet_unref(&pkt);
				}

				if (m_pInputFmtCtx->duration > 0) {
					int64_t ptsFile = m_pInputFmtCtx->duration / 1000; //该文件总时间。ms
					ptsTime += ptsFile;//当前总时间
				}
				else {
					int64_t ptsFile = FFMAX(ptsFileAudio, ptsFileVideo); //该文件总时间。ms
					ptsTime += ptsFile;//当前总时间
				}
			}
		}
		avformat_close_input(&m_pInputFmtCtx);
		avformat_free_context(m_pInputFmtCtx);//关闭输入文件
	}

	if (m_bInit) { //关闭输出文件
		av_write_trailer(m_pOutputFmtCtx);
		if (!(m_pOutputFmtCtx->oformat->flags & AVFMT_NOFILE))
			avio_closep(&m_pOutputFmtCtx->pb);
		avformat_free_context(m_pOutputFmtCtx);
		return WX_ERROR_SUCCESS;
	}
	return WX_ERROR_ERROR;
}

//--------------------- 基于DSound 的声音播放 ------------------------
//一般情况下不用考虑默认设备插拔切换的问题
#define MAX_AUDIO_BUF 5
#define AUDIO_FRAME_SIZE   1920  //对应10ms  S16 数据量

DEFINE_GUID(IID_IDirectSoundNotify, 0xb0210783, 0x89cd, 0x11d0, 0xaf, 0x8, 0x0, 0xa0, 0xc9, 0x25, 0xcd, 0x16);
DEFINE_GUID(IID_IDirectSoundBuffer8, 0x6825a449, 0x7524, 0x4d82, 0x92, 0x0f, 0x50, 0xe3, 0x6a, 0xb3, 0xab, 0x1e);

class DSoundPlayer :public WXThread {
	WXString m_strDevName = L"None";

	int m_nSampleRate = 0;//采样频率
	int m_nChannel = 0; //声道数量
	int m_nSize = 0;//10ms输入数据量

	int64_t m_drop = 0;//丢弃帧
	int64_t m_nWrite = 0;//写入
	int64_t m_nRead = 0;//读取

	WXFifo m_audioFifo;

	CComPtr<IDirectSound8> m_pDS = NULL;
	CComPtr<IDirectSoundBuffer> m_pDSBuffer = NULL;
	CComPtr<IDirectSoundBuffer8>m_pDSBuffer8 = NULL;
	CComPtr<IDirectSoundNotify>m_pDSNotify = NULL;
	HANDLE m_event = nullptr;
	DSBPOSITIONNOTIFY m_pDSPosNotify[MAX_AUDIO_BUF];
	DWORD m_offset = AUDIO_FRAME_SIZE;
	DSBUFFERDESC m_dsbd;
	WAVEFORMATEX m_fmt;

	BOOL OpenImpl() {
		if (LibInst::GetInst().m_libDSound == nullptr)
			return FALSE;
		m_pDSNotify = nullptr;
		m_pDSBuffer8 = nullptr;	//used to manage sound buffers.
		m_pDSBuffer = nullptr;
		m_pDS = nullptr;
		WXString strHR;
		HRESULT hr = LibInst::GetInst().mDirectSoundCreate8(NULL, &m_pDS, NULL);
		if (SUCCEEDED(hr)) {
			strHR = L"DirectSoundCreate8 OK";
			hr = m_pDS->SetCooperativeLevel(::GetDesktopWindow(), DSSCL_NORMAL);
			if (SUCCEEDED(hr)) {
				strHR = L"SetCooperativeLevel OK";
				hr = m_pDS->CreateSoundBuffer(&m_dsbd, &m_pDSBuffer, NULL);
				if (SUCCEEDED(hr)) {
					strHR = L"CreateSoundBuffer OK";
					hr = m_pDSBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&m_pDSBuffer8);
					if (SUCCEEDED(hr)) {
						strHR = L"IID_IDirectSoundBuffer8 OK";
						hr = m_pDSBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&m_pDSNotify);
						if (SUCCEEDED(hr)) {
							strHR = L"IID_IDirectSoundNotify OK";
							for (int i = 0; i < MAX_AUDIO_BUF; i++) {
								m_pDSPosNotify[i].dwOffset = i * m_nSize;
								m_pDSPosNotify[i].hEventNotify = m_event;
							}
							hr = m_pDSNotify->SetNotificationPositions(MAX_AUDIO_BUF, m_pDSPosNotify);
							if (SUCCEEDED(hr)) {
								strHR = L"SetNotificationPositions OK";
								hr = m_pDSBuffer8->SetCurrentPosition(0);
								if (SUCCEEDED(hr)) {
									strHR = L"SetCurrentPosition OK";
									hr = m_pDSBuffer8->Play(0, 0, DSBPLAY_LOOPING);
									if (SUCCEEDED(hr)) {
										m_bPlaying.store(true);
									}
								}
							}
						}
					}
				}
			}
		}
		if (FAILED(hr)) {
			m_nError++;
			LogW(L"DSound HR --- %ws ", strHR.str());
		}
		return SUCCEEDED(hr);
	}
	//int64_t m_nWaitTimeOut = 0;
	std::atomic_bool m_bPlaying = false;

	WXDataBuffer m_tmpBuffer;

	int m_nVolume = 100;//默认音量


	void* m_pSink = nullptr;
	OnData m_cbAudio = nullptr;
public:
	virtual  void ThreadPrepare() {
		//WXLogW(L"Dsound Play Start!!!!!");
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);//线程优先级
		OpenImpl();
	}

	void   ThreadWait() {
	}

	virtual  void ThreadPost() {
		if (m_pDSBuffer8)
			m_pDSBuffer8->Stop();
		m_pDSNotify = nullptr;
		m_pDSBuffer8 = nullptr;
		m_pDSBuffer = nullptr;
		m_pDS = nullptr;
	}

	virtual  void ThreadProcess() {
		if (!m_bPlaying.load()) {
			OpenImpl();
			return;
		}
		DWORD res = ::WaitForSingleObjectEx(m_event, 10, FALSE);
		if (res == WAIT_OBJECT_0) {
			LPVOID pData = NULL;
			DWORD  nDataSize = 0;
			HRESULT hr = m_pDSBuffer8->Lock(m_offset, m_nSize, &pData, &nDataSize, NULL, NULL, 0);
			if (SUCCEEDED(hr) && pData) {

				if (m_cbAudio) {
					m_cbAudio(m_pSink, (uint8_t*)pData, nDataSize);//请求数据
				}
				else {
					int Ret = m_audioFifo.Read3((uint8_t*)pData, nDataSize);//请求数据或者回调
					//请求数据
					m_nRead += Ret;
				}


				if (m_nVolume == 0) {//静音状态
					memset(pData, 0, nDataSize);
				}
				else if (m_nVolume == 100) {//不用处理音量

				}
				else {//处理音量
					short* pDst = (short*)pData;
					short* pSrc = (short*)pData;
					for (size_t i = 0; i < nDataSize / 2; i++) {
						pDst[i] = short((int)pSrc[i] * m_nVolume / 100);
					}
				}

				m_pDSBuffer8->Unlock(pData, nDataSize, NULL, 0);
				m_offset += nDataSize;
				m_offset %= (m_nSize * MAX_AUDIO_BUF);
			}
			else {
				//ERROR need Reset
				m_bPlaying.store(false);
				LogW(L"m_pDSBuffer8->Lock ERROR , Reset!!");
				if (m_pDSBuffer8)
					m_pDSBuffer8->Stop();
				m_pDSNotify = nullptr;
				m_pDSBuffer8 = nullptr;	//used to manage sound buffers.
				m_pDSBuffer = nullptr;
				m_pDS = nullptr;//超时过多，可能设备已经无效
			}
		}
		else { //超时
			//m_nWaitTimeOut++;
		}
	}

	BOOL Init(int nSampleRate, int nChannel, void* pSink, OnData cb) {
		m_pSink = pSink;
		m_cbAudio = cb;
		m_nSampleRate = nSampleRate;
		m_nChannel = nChannel;
		m_nSize = m_nSampleRate * m_nChannel * 2 / 100;//10ms
		m_offset = m_nSize;

		m_tmpBuffer.Init(nullptr, m_nSampleRate * m_nChannel * 4 / 100 * 2);

		//音频格式处理
		memset(&m_fmt, 0, sizeof(m_fmt));
		m_fmt.wFormatTag = WAVE_FORMAT_PCM; //音频格式
		m_fmt.nChannels = m_nChannel; //声道
		m_fmt.nSamplesPerSec = m_nSampleRate;//采样频率
		m_fmt.nAvgBytesPerSec = m_nSampleRate * 2 * m_nChannel;
		m_fmt.nBlockAlign = 2 * m_nChannel;
		m_fmt.wBitsPerSample = 16;
		m_fmt.cbSize = 0;
		memset(&m_dsbd, 0, sizeof(m_dsbd));
		m_dsbd.dwSize = sizeof(m_dsbd);
		m_dsbd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2;
		m_dsbd.dwBufferBytes = MAX_AUDIO_BUF * m_nSize;
		m_dsbd.lpwfxFormat = &m_fmt;
		if (m_event == nullptr)
			m_event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		return TRUE;
	}
public:
	void SetVolume(int volume) {
		m_nVolume = volume;
	}
	//填充数据
	void WriteData(uint8_t* buf, int buf_size) { //外部直接填充数据
		uint8_t* pcm = buf;
		int pcm_size = buf_size;
		int audio_size = m_audioFifo.Size();//播放缓冲区数据量
		if (audio_size >= 30 * m_nSize) {//缓冲区超过200ms数据就丢包
			m_drop += pcm_size;
			return;
		}
		m_nWrite += pcm_size;
		m_audioFifo.Write(pcm, pcm_size);
	}

	BOOL Start(int nSampleRate, int nChannel, void* pSink = nullptr, OnData cb = nullptr) {
		BOOL bInit = Init(nSampleRate, nChannel, pSink, cb);
		if (bInit) {
			ThreadSetName(L"DsoundPlayer PCM");
			ThreadStart();//默认都能启动，如果有设备问题
		}
		else {
			LogW(L"Do not Support Sound Play!!!");
		}
		return bInit;
	}

	void Stop() {
		for (size_t i = 0; i < MAX_AUDIO_BUF; i++)
		{
			WriteData(m_tmpBuffer.GetBuffer(), m_tmpBuffer.m_iBufSize);
		}
		ThreadStop();
		if (m_event) {
			CloseHandle(m_event);
			m_event = NULL;
		}
	}

	DSoundPlayer() {

	}

	virtual ~DSoundPlayer() {
		this->Stop();
	}
};

//用于实时流数据的播放，和DSoundPlayerWriteData配套使用
EXTERN_C void* MLSoundPlayerCreate(int inSampleRate, int inChannel) {
	if (LibInst::GetInst().m_libDSound == nullptr) {
		return nullptr;
	}
	DSoundPlayer* player = new DSoundPlayer;
	bool bRet = player->Start(inSampleRate, inChannel);
	if (!bRet) {
		delete player;
		return nullptr;
	}
	return (void*)player;
}

//回调音频数据
EXTERN_C void* MLSoundPlayerCreateEx(int inSampleRate, int inChannel, void* pSink, OnData cb) {
	if (LibInst::GetInst().m_libDSound == nullptr) {
		return nullptr;
	}
	DSoundPlayer* player = new DSoundPlayer;
	bool bRet = player->Start(inSampleRate, inChannel, pSink, cb);
	if (!bRet) {
		delete player;
		return nullptr;
	}
	return (void*)player;
}

//销毁对象
EXTERN_C void  MLSoundPlayerDestroy(void* ptr) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		delete player;//容易闪退
	}
}

//设置音量
EXTERN_C void  MLSoundPlayerVolume(void* ptr, int volume) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		player->SetVolume(volume);
	}
}

//直接填充音频数据
EXTERN_C void  MLSoundPlayerWriteData(void* ptr, uint8_t* buf, int buf_size) {
	if (ptr) {
		DSoundPlayer* player = (DSoundPlayer*)ptr;
		player->WriteData(buf, buf_size);
	}
}