// stdafx.cpp : source file that includes just the standard includes
//  TabViewDemo.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#include "CMainFrame.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
    CMessageLoop theLoop;
    _Module.AddMessageLoop(&theLoop);
    CMainFrame wndMain;

    if (wndMain.CreateEx() == NULL) {
        ATLTRACE(_T("Main window creation failed!\n"));
        return 0;
    }

    wndMain.ShowWindow(nCmdShow);
    int nRet = theLoop.Run();
    _Module.RemoveMessageLoop();
    return nRet;
}




static BOOL _IsHaveInstance()
{
    #define  AppSingletonMutex _T("{5676532A-0000-460D-A1F0-81D6E68F046A}")

    // 单实例运行
    HANDLE hMutex = ::CreateMutex(NULL, TRUE, AppSingletonMutex);
    if (hMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(0, _T("上次程序运行还没完全退出，请稍后再启动！"), _T("WXMedia"), MB_OK);
		exit(0);
        return TRUE;
    }
    return FALSE;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{

    _IsHaveInstance();

    //Com Init
    HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    ATLASSERT(SUCCEEDED(hRes));

    AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

    Initialize("Apowersoft", "WXTest+++", 0);//库的初始化
	WXDeviceInit(L"WXMedia_Test.log");//WXMedia初始化

    //GDI+ Init
    static Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    static ULONG_PTR gdiplusToken = 0;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    // If you are running on NT 4.0 or higher you can use the following call instead to
    // make the EXE free threaded. This means that calls come in on a random RPC thread.
    //  HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ATLASSERT(SUCCEEDED(hRes));
#if (_WIN32_IE >= 0x0300)
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_COOL_CLASSES | ICC_BAR_CLASSES;
    BOOL bRet = ::InitCommonControlsEx(&iccx);
    bRet;
    ATLASSERT(bRet);
#else
    ::InitCommonControls();
#endif
    hRes = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hRes));
    AtlAxWinInit();
    int nRet = Run(lpstrCmdLine, nCmdShow);
    _Module.Term();
    ::CoUninitialize();
    return nRet;
}

