#include <stdint.h>
#include "MiraCastProcess.h"
#include "MiraCastCommon.h"
#include <Windows.h>
#include <process.h> 
#include "WXMedia.h"
#include <Tlhelp32.h>
#include "WXMedia.h"

#pragma comment(lib,"ws2_32.lib")
static int s_decodeMode = 1;
MiraCastManager::MiraCastManager()
{
}

MiraCastManager::~MiraCastManager()
{
}
HandleStreamMiraCast::HandleStreamMiraCast()
{
}

HandleStreamMiraCast::~HandleStreamMiraCast()
{
}

MiraCastManager& MiraCastManager::Get()
{
	static MiraCastManager sMiraCastManager;
	return sMiraCastManager;
}
unsigned long long g_uniqueid = 0;
std::map<unsigned long long, HandleStreamMiraCast*>g_MapMirror;
void* m_play = NULL;
unsigned __stdcall  PlayFun(void* param);
extern "C" void MyCallBack(void* param, uint8_t* data, size_t size);
std::string strPipe = "miracastc2s";
std::wstring get_upper_dir(const wchar_t* dir_path)
{
	if (NULL == dir_path || '\0' == dir_path[0])
	{
		return L"";
	}

	std::wstring dir = dir_path;
	wchar_t last_char = dir[dir.size() - 1];
	if (last_char == '\\' || last_char == '/')
	{
		// 末尾就是 '\\'，处理之前先去掉末尾的 '\\'
		dir.resize(dir.size() - 1);
	}
	std::wstring::size_type pos1 = dir.rfind('\\');
	std::wstring::size_type pos2 = dir.rfind('/');
	std::wstring::size_type pos;
	if (pos1 != std::wstring::npos && pos2 != std::wstring::npos)
	{
		pos = max(pos1, pos2);
	}
	else
	{
		pos = (pos1 != std::wstring::npos ? pos1 : pos2);
	}

	if (0)
	{
		return L"";
	}
	else
	{
		return dir.substr(0, pos + 1);
	}
}
std::wstring get_current_exe_path()
{
	wchar_t modulePath[MAX_PATH];
	if (GetModuleFileNameW(NULL, modulePath, MAX_PATH)) // 获取所在EXE名称
	{
		return modulePath;
	}
	else
	{
		return L"";
	}
}
std::wstring get_current_exe_dir()
{
	//return L"D:\\Apowersoft\\windows\\WXCommon\\src\\WX.MiraCast\\Debug\\";
	return get_upper_dir(get_current_exe_path().c_str());
}
BOOL FindAndKillProcessByName(LPCTSTR strProcessName)
{
	if (NULL == strProcessName)
	{
		return FALSE;
	}
	HANDLE handle32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == handle32Snapshot)
	{
		return FALSE;
	}

	PROCESSENTRY32 pEntry;
	pEntry.dwSize = sizeof(PROCESSENTRY32);

	//Search for all the process and terminate it
	if (Process32First(handle32Snapshot, &pEntry))
	{
		BOOL bFound = FALSE;
		if (!_tcsicmp(pEntry.szExeFile, strProcessName))
		{
			bFound = TRUE;
		}
		while ((!bFound) && Process32Next(handle32Snapshot, &pEntry))
		{
			if (!_tcsicmp(pEntry.szExeFile, strProcessName))
			{
				bFound = TRUE;
			}
		}
		if (bFound)
		{
			CloseHandle(handle32Snapshot);
			HANDLE handLe = OpenProcess(PROCESS_TERMINATE, FALSE, pEntry.th32ProcessID);
			BOOL bResult = TerminateProcess(handLe, 0);
			return bResult;
		}
	}

	CloseHandle(handle32Snapshot);
	return FALSE;
}
int MiraCastManager::Start(const std::wstring& strAppName, const std::wstring& strLogName)
{
	FindAndKillProcessByName(L"WXMCast.exe");
	//system("taskkill /f /im 'TSVNCache.exe'");
	std::wstring strCurPath = get_current_exe_dir();
	strCurPath += L"WXMCast.exe";

	if (ipc_pipe_server_start(&mSingle, "miracasts2c", MyCallBack, NULL))
	{
		printf("Start PIPE\n\r");
	}
	else
	{
		printf("Start PIPE error!\n\r");
		return ErrorCode_StartMiracastServerFail;
	}
	PROCESS_INFORMATION pi = { 0 };
	STARTUPINFOW si = { 0 };
	si.cb = sizeof(si);
	std::wstring strCmdLine = L"";
	strCmdLine += L" -log ";
	strCmdLine += strLogName;
	//strCmdLine += L" -name ";
	//strCmdLine += ;
	strCmdLine += L" -noreportdata ";
	strCmdLine += L"1";
	bool success = !!CreateProcessW(strCurPath.c_str(), (LPWSTR)strCmdLine.c_str(), NULL, NULL,
		false, /*CREATE_NEW_CONSOLE*/CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (success)
	{
		CloseHandle(pi.hThread);
		Sleep(1000);
		bPipeInit = ipc_pipe_client_open(&pipe, strPipe.c_str());
	}
	else
	{
		return ErrorCode_StartMiracastServerFail;
	}
	return 0;
}


void MiraCastManager::Stop(bool bWait)
{

	//stop wxmcast
	ipc_pipe_server_free(&mSingle);
	if (bPipeInit)
	{
		ipc_pipe_client_write(&pipe, "stop", strlen("stop"));
		ipc_pipe_client_free(&pipe);
		Sleep(1000);
		FindAndKillProcessByName(L"WXMCast.exe");

	}
}

void MiraCastManager::SetCallBackFuns(const WXMiraCastManagerStruct* stMiraCast)
{
	if (stMiraCast->m_arcAppName != L"")
	{
		m_stMiraCast.m_arcAppName = stMiraCast->m_arcAppName;
	}
	else
	{
		m_stMiraCast.m_arcAppName = L"WXMiracast";

	}
	if (stMiraCast->m_arcLogName != L"")
	{
		m_stMiraCast.m_arcLogName = stMiraCast->m_arcLogName;
	}
	else
	{
		m_stMiraCast.m_arcLogName = L"WXMiracast.log";
	}
	if (stMiraCast->m_flag == 0)
	{
		if (stMiraCast->M_WXMiraCastCallBackConnectInfo != NULL) {
			m_stMiraCast.M_WXMiraCastCallBackConnectInfo = stMiraCast->M_WXMiraCastCallBackConnectInfo;
		}
		if (stMiraCast->M_WXMiraCastCallBackGetParentWindow != NULL) {
			m_stMiraCast.M_WXMiraCastCallBackGetParentWindow = stMiraCast->M_WXMiraCastCallBackGetParentWindow;
		}
		if (stMiraCast->M_WXMiraCastCallBackWindowStatus != NULL) {
			m_stMiraCast.M_WXMiraCastCallBackWindowStatus = stMiraCast->M_WXMiraCastCallBackWindowStatus;
		}
		if (stMiraCast->M_WXMiraCastCallBackIsSupport != NULL)
		{
			m_stMiraCast.M_WXMiraCastCallBackIsSupport = stMiraCast->M_WXMiraCastCallBackIsSupport;
		}
	}
	else
	{
		if (stMiraCast->m_WXMirrorCastDataR != NULL)
		{
			m_stMiraCast.m_WXMirrorCastDataR = stMiraCast->m_WXMirrorCastDataR;
		}
		if (stMiraCast->m_WXMirrorCastConnectInfo != NULL)
		{
			m_stMiraCast.m_WXMirrorCastConnectInfo = stMiraCast->m_WXMirrorCastConnectInfo;
		}
		if (stMiraCast->m_WXMirrorCastIsSupport != NULL)
		{
			m_stMiraCast.m_WXMirrorCastIsSupport = stMiraCast->m_WXMirrorCastIsSupport;
		}
	}
}


BOOL StringToWString(const std::string& str, std::wstring& wstr)
{
	int nLen = (int)str.length();
	wstr.resize(nLen, L' ');

	int nResult = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), nLen, (LPWSTR)wstr.c_str(), nLen);

	if (nResult == 0)
	{
		return FALSE;
	}

	return TRUE;
}

extern "C" void MyCallBack(void* param, uint8_t* data, size_t size)
{
	const char* str = (const char*)data;
	if (size == 0)
	{
		return;
	}
	if (_strnicmp(str, "unsupportwifi", strlen("unsupportwifi")) == 0)
	{
		if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackIsSupport != NULL)
		{
			MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackIsSupport();
		}
	}
	if (_strnicmp(str, "connect", strlen("connect")) == 0)
	{
		char* pszIp = new char[20];
		memset(pszIp, 0, 20);
		memcpy(pszIp, str + strlen("connect-"), size - strlen("connect-"));
		if (MiraCastManager::Get().m_stMiraCast.m_flag == 1)
		{
			MiraCastManager::Get().m_stMiraCast.m_WXMirrorCastDataR(pszIp, size - strlen("connect-"));
		}
		else
		{
			HANDLE hRecordThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)PlayFun, pszIp, 0, NULL);
			CloseHandle(hRecordThread);
		}
	}
	else if (_strnicmp(str, "disconnect", strlen("disconnect")) == 0)
	{
		char pszIp[20] = { 0 };
		memcpy(pszIp, str + strlen("disconnect-"), size - strlen("disconnect-"));
		unsigned long long uniqueid = inet_addr(pszIp);
		ConnectStatusStruct stConnect;
		stConnect.eConnectStatus = DISCONNECT;
		stConnect.iConnectType = 1;
		std::wstring szBuffer = L"";
		StringToWString(pszIp, szBuffer);
		stConnect.connectip = (wchar_t*)szBuffer.c_str();


		if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo != NULL)
		{
			MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo(stConnect, uniqueid);
		}
		if (m_play) {
			/*WXFfplayStop(m_play);
			WXFfplayDestroy(m_play);
			m_play = nullptr;*/
			//StreamPlayerStop(m_play);
			m_play = nullptr;
		}
	}
}
void MyFfplayOnSize(void* cbSink, int bRotate, int width, int height)
{
	unsigned long long uniqueid = g_uniqueid;// (unsigned long long)cbSink;
	HandleStreamMiraCast* obj = new HandleStreamMiraCast;
	g_MapMirror[uniqueid] = obj;
	g_MapMirror[uniqueid]->m_iWidth = WXGetNetStreamWidth();
	g_MapMirror[uniqueid]->m_iHeight = WXGetNetStreamHeight();
	//
	WINDOWSHOWSTATUS eWindowsStatus = WSS_PORTRAIT;// LANDSCAPE;
	//if (bRotate == 1) 
	//{
	//	eWindowsStatus = PORTRAIT;
	//}
	WindowShowStruct stWindowShow;
	stWindowShow.eWindowStatus = eWindowsStatus;
	stWindowShow.iScreenW = WXGetNetStreamWidth();
	stWindowShow.iScreenH = WXGetNetStreamHeight();
	if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackWindowStatus != NULL) {
		MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackWindowStatus(stWindowShow, uniqueid);
	}
}
void MiraCastManager::DisconnectMiraCastMirror(const UINT64 uniqueid)
{
	if (bPipeInit)
	{
		//disconnect-
		std::string strTmp = "disconnect-";
		IN_ADDR tmp;
		tmp.S_un.S_addr = uniqueid;
		char* szIp = inet_ntoa(tmp);
		strTmp += szIp;
		ipc_pipe_client_write(&pipe, strTmp.c_str(), strTmp.size());
		//
		unsigned long long id = inet_addr(szIp);
		ConnectStatusStruct stConnect;
		stConnect.eConnectStatus = DISCONNECT;
		stConnect.iConnectType = 1;
		std::wstring szBuffer = L"";
		StringToWString(szIp, szBuffer);
		stConnect.connectip = (wchar_t*)szBuffer.c_str();
		if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo != NULL)
		{
			MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo(stConnect, id);
		}
		if (m_play) {
			//StreamPlayerStop(m_play);
			m_play = nullptr;
		}
	}
}
void  cbSize(void* cbSink, int bRotate, int width, int height) {
	printf("printf cbSize  begin");
	unsigned long long uniqueid = g_uniqueid;// (unsigned long long)cbSink;
	HandleStreamMiraCast* obj = new HandleStreamMiraCast;
	g_MapMirror[uniqueid] = obj;
	g_MapMirror[uniqueid]->m_iWidth = width;
	g_MapMirror[uniqueid]->m_iHeight = height;
	//
	WINDOWSHOWSTATUS eWindowsStatus = WSS_LANDSCAPE;// LANDSCAPE;
	//if (bRotate == 1) 
	//{
	//	eWindowsStatus = PORTRAIT;
	//}
	WindowShowStruct stWindowShow;
	stWindowShow.eWindowStatus = eWindowsStatus;
	stWindowShow.iScreenW = width;
	stWindowShow.iScreenH = height;
	if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackWindowStatus != NULL) {
		printf("printf Mira printf Mira printf Mira");
		MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackWindowStatus(stWindowShow, uniqueid);
	}
}
unsigned __stdcall  PlayFun(void* param)
{
	char* pszIp = (char*)param;
	g_uniqueid = inet_addr(pszIp);
	//
	ConnectStatusStruct stConnect;
	stConnect.eConnectStatus = CONNECTED;
	stConnect.iConnectType = 1;
	std::wstring szBuffer = L"";
	StringToWString(pszIp, szBuffer);
	stConnect.connectip = (wchar_t*)szBuffer.c_str();
	if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo != NULL) {//not allow connect
		if (MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackConnectInfo(stConnect, g_uniqueid) == 0) {
			MiraCastManager::Get().DisconnectMiraCastMirror(g_uniqueid);
		}
	}
	HWND hwnd = (HWND)MiraCastManager::Get().m_stMiraCast.M_WXMiraCastCallBackGetParentWindow(g_uniqueid);
	if (m_play == nullptr) {
		m_play = 0;//StreamPlayerStart(L"rtp://127.0.0.1:62853", hwnd, s_decodeMode, nullptr, cbSize);
	}
	/*m_play = WXFfplayCreate(L"FFPLAY", L"rtp://127.0.0.1:62853", 100, 0);
	WXFfplaySetView(m_play, hwnd);
	WXFfplaySetSizeCb(m_play, MyFfplayOnSize);
	WXFfplaySetEventOwner(m_play, (void*)g_uniqueid);
	WXFfplayStart(m_play);*/

	if (pszIp != nullptr)
	{
		delete pszIp;
		pszIp = nullptr;
	}
	return 0;
}

MIRACAST_API void SetDecodeMode(int mode)
{
	s_decodeMode = mode;
}



