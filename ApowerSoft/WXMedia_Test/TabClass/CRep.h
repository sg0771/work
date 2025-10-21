// TabDemoView1.h : interface of the CTabDemoView1 class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(CRep_H)
#define CRep_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CRep
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
    CString m_strInput = L"D:\\test.gif.xws";
    CString m_strOutput = L"D:\\test.gif";
	BOOL m_bRunning = FALSE;

    CProgressBarCtrl m_prog;
public:
    enum { IDD = IDD_WXREP};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_TEXT(IDC_INPUT_NAME_REP, m_strInput)
        DDX_TEXT(IDC_OUTPUT_NAME_REP, m_strOutput)
        DDX_CONTROL_HANDLE(IDC_PROGRESS_REP, m_prog)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_INPUT_REP, BN_CLICKED, OnBnClickedInput)
        COMMAND_HANDLER(IDC_OUTPUT_REP, BN_CLICKED, OnBnClickedOutput)
        COMMAND_HANDLER(IDC_REP, BN_CLICKED, OnBnClickedRep)
        COMMAND_HANDLER(IDC_BREAK_REP, BN_CLICKED, OnBnClickedBreak)
        COMMAND_HANDLER(IDC_CUT, BN_CLICKED, OnBnClickedCut)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    LRESULT OnBnClickedInput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedOutput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CRep_H)
