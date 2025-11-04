// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "stdafx.h"
#include "resource.h"
#include <shellapi.h>  // 必须包含，定义了 HDROP、DragQueryFile、DragFinish 等
#include <windows.h>   // 基础 Windows 类型定义（通常 WTL 会间接包含，但保险起见可添加）
#include "MediaInfoDLL.h"
#include <WXMedia.h>
#pragma comment(lib,"WXMedia.lib")

class CMainDlg : 
	public CDialogImpl<CMainDlg>, 
	public CWinDataExchange<CMainDlg>
{

	CString m_strInput = L"1.mp4";
	CString m_strName = L"1.txt";
	CString m_strMsg = L"";

	void HandleMI();
	void HandleWX();

	BOOL m_bMI = FALSE;
public:

	enum { IDD = IDD_MAINDLG };
	
	//控件变量
	BEGIN_DDX_MAP(CMainDlg)
		DDX_TEXT(IDC_EDIT1, m_strMsg)
		DDX_CHECK(IDC_CHECK1, m_bMI)
	END_DDX_MAP()

	// 消息映射
	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		COMMAND_HANDLER(IDC_INPUT, BN_CLICKED, OnBnClickedInput)
		COMMAND_HANDLER(IDC_SAVE, BN_CLICKED, OnBnClickedSave)
		MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)    // 处理文件拖拽释放
	END_MSG_MAP()

	// Handler prototypes (uncomment arguments if needed):
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);    
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedInput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBnClickedSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

};
