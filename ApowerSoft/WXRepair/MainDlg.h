// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "stdafx.h"
#include "resource.h"

#include "WXMedia.h"
#pragma comment(lib,"WXMedia.lib")
#pragma comment(lib,"version.lib")

class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CWinDataExchange<CMainDlg>
{
	enum { TimeID = 1001 };
	int  m_msTime = 100;//100ms
	UINT_PTR m_idTime = 0;//定时器


	CString m_strInput = L"D:\\test.gif.xws";
	CString m_strOutput = L"D:\\test.gif";
	BOOL m_bRunning = FALSE;

	CProgressBarCtrl m_prog;
public:
	enum { IDD = IDD_MAINDLG };
	
	//控件变量
	BEGIN_DDX_MAP(CMainDlg)
		DDX_TEXT(IDC_INPUT_NAME_REP, m_strInput)
		DDX_TEXT(IDC_OUTPUT_NAME_REP, m_strOutput)
		DDX_CONTROL_HANDLE(IDC_PROGRESS_REP, m_prog)
	END_DDX_MAP()

	// 消息映射
	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		COMMAND_HANDLER(IDC_INPUT_REP, BN_CLICKED, OnBnClickedInputRep)
		COMMAND_HANDLER(IDC_OUTPUT_REP, BN_CLICKED, OnBnClickedOutputRep)
		COMMAND_HANDLER(IDC_REP, BN_CLICKED, OnBnClickedRep)
		COMMAND_HANDLER(IDC_BREAK_REP, BN_CLICKED, OnBnClickedBreakRep)
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	LRESULT OnHotKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedInputRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedOutputRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedBreakRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
