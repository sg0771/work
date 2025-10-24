// AirplayDemo.cpp : main source file for AirplayDemo.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;



#define  AppSingletonMutex _T("{AAAAAAAA-6F70-460D-A1F0-81D6E68F046A}")
static BOOL _IsHaveInstance()
{
	// 单实例运行
	HANDLE hMutex = ::CreateMutex(NULL, TRUE, AppSingletonMutex);
	if (hMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(0, _T("上次程序运行还没完全退出，请稍后再启动！"), _T("TeamTalk"), MB_OK);
		exit(-1);
		return TRUE;
	}

	return FALSE;
}


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{

	_IsHaveInstance();

	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);//COM init


	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	//add by Tam
	//CMessageLoop theLoop;
	//_Module.AddMessageLoop(&theLoop);

	int nRet = 0;
	// BLOCK: Run application
	{
		CMainDlg dlgMain;
		nRet = (int)dlgMain.DoModal();
	}

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
