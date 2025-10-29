// Mediainfo.DLL C++ Test Application
// 功能: 使用MediaInfo.DLL获取多媒体文件信息
// 动态加载 MediaInfo.DLL 如果没有找到则提示用户

#if !defined(CMediaInfoTest_H)
#define CMediaInfoTest_H

#include "stdafx.h"
#include "resource.h"
#include <atlwin.h>
#include <atlctrls.h> // 包含 WTL EDIT 控件相关定义
#include <shellapi.h>  // 必须包含，定义了 HDROP、DragQueryFile、DragFinish 等
#include <windows.h>   // 基础 Windows 类型定义（通常 WTL 会间接包含，但保险起见可添加）
#include <filesystem>

//#pragma comment(lib,"shellapi.lib")

#undef  CLASS_NAME
#define CLASS_NAME CMediaInfoTest
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
    CString m_strInput = L"1.mp4";
    std::wstring m_strName = L"1.txt";
    CString m_strMsg = L"";
    void Handle();
public:
    enum { IDD = IDD_MediaInfoTest};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_TEXT(IDC_MSG_MI, m_strMsg)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_HANDLER(IDC_FILE_MI, BN_CLICKED, OnBnClickedFile)
        MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)    // 处理文件拖拽释放
        COMMAND_HANDLER(IDC_SAVE_MI, BN_CLICKED, OnBnClickedSaveMi)
        COMMAND_HANDLER(IDC_WX_MI, BN_CLICKED, OnBnClickedWxMi)
    END_MSG_MAP()

    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSaveMi(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    LRESULT OnBnClickedWxMi(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CReadToHex_H)
