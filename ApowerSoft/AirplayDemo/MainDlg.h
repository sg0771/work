// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "stdafx.h"
#include <WXMedia.h>
#include <WXBase.h>
#include "../libairplay/WXAirPlayAPI.h"
#include "resource.h"

#pragma comment(lib,"libairplay.lib")
//#pragma comment(lib,"libmedia.lib")

class CMainDlg : public CDialogImpl<CMainDlg>, public CWinDataExchange<CMainDlg>
{
	CString m_strPath = L"";
	
	RECT m_rect;
	LONG_PTR m_style = 0;
	LONG_PTR m_ex_style = 0;

	BOOL m_bFullScreen = FALSE;
	ATOM m_atomFullScreen = 0;   //全屏显示  Alt+Q

	int m_bCaptureState = 0;
	ATOM m_atomStartAndStop = 0;   // 启动/结束热键  Ctrl+Alt+1

	bool m_bHW = FALSE;
	ATOM m_atomHW = 0;    //设置是否使用硬解码

	bool m_bFixed = true;
	ATOM m_atomFixed = 0;    //是否填充显示

	int  m_bLut = 0;//是否以I420(BT601)显示
	ATOM m_atomI420 = 0;    //是否填充显示

	ATOM m_atomRotate0 = 0;    //恢复原图
	ATOM m_atomRotate90 = 0;    //旋转90度
	ATOM m_atomRotate180 = 0;    //旋转180度
	ATOM m_atomRotate270 = 0;    //旋转270
	ATOM m_atomFlipX = 0;    //水平翻转
	ATOM m_atomFlipY = 0;    //垂直翻转

	ATOM m_atomPNG = 0;    //PNG截图
	ATOM m_atomJPG = 0;    //JPG截图

	void Exit();

	UINT_PTR m_idTime = 0;
	int64_t m_tsStartCapture = 0;
	WXLocker m_lock;
public:
	enum { IDD = IDD_MAINDLG };
	
	BEGIN_DDX_MAP(CMainDlg)
	END_DDX_MAP()

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
	END_MSG_MAP()

	LRESULT OnHotKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);//热键事件
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void Start();
	void Stop();
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};
