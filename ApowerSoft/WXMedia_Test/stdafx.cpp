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

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
    //Com Init
    HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    ATLASSERT(SUCCEEDED(hRes));

    AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

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

