//功能: 将文件内容读取为十六进制字符串

#if !defined(CReadToHex_H)
#define CReadToHex_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CReadToHex
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
    CString m_strName = L"";
    CString m_strHex = L"";
public:
    enum { IDD = IDD_ReadToHex};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_TEXT(IDC_FILENAME_HEX, m_strName)
        DDX_TEXT(IDC_MSG_HEX, m_strHex)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER(IDC_FILE_HEX, BN_CLICKED, OnBnClickedFile)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};

#endif // !defined(CReadToHex_H)
