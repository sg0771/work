// AirplayDemo.cpp : main source file for AirplayDemo.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int /*nCmdShow*/)
{

	// 初始化GDI+
	Gdiplus::GdiplusStartupInput gdiplusInput;
	ULONG_PTR m_gdiplusToken = 0;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusInput, nullptr);

	// 加载并解码资源中的JPG


	//LoadJpgFromResource();


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
