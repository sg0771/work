// Mediainfo.DLL C++ Test Application
// 功能: 使用MediaInfo.DLL获取多媒体文件信息
// 动态加载 MediaInfo.DLL 如果没有找到则提示用户

#if !defined(CMediaInfoTest_H)
#define CMediaInfoTest_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CMediaInfoTest
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
	CString m_strInput = L"1.mp4";
    CString m_strMsg = L"";
public:
    enum { IDD = IDD_MediaInfoTest};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_TEXT(IDC_FILENAME_MI, m_strInput)
        DDX_TEXT(IDC_MSG_MI, m_strMsg)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_HANDLER(IDC_FILE_MI, BN_CLICKED, OnBnClickedFile)
    END_MSG_MAP()

    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CReadToHex_H)
