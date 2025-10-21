// Medialib.dll  播放和导出 测试Demo

#if !defined(CMedialib_H)
#define CMedialib_H

#include "stdafx.h"
#include "resource.h"

#include "../MediaLib/MediaLibAPI.h"
#pragma comment(lib,"medialib.lib")

#undef  CLASS_NAME
#define CLASS_NAME CMedialib
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
    int m_iWidth = 1280;
    int m_iHeight = 720;
    int m_iFps = 20;
    BOOL m_bHW_Dec = TRUE;
    BOOL m_bHW_Enc = TRUE;
    CString m_strDIR = L"D:\\";

    CListViewCtrl m_listView;//列表
    CTrackBarCtrl  m_s1;//滑动条

    //为TRUE时禁止操作文件列表
    BOOL m_bInConv = FALSE;//正在转换

    BOOL m_bPause = FALSE;
    VideoState* m_play = nullptr;//播放器对象

    std::thread* m_threadTask = nullptr;// 转换进程
    void funcConv();//转换任务
public:
    enum { IDD = IDD_Medialib};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_CHECK(IDC_HW_DEC, m_bHW_Dec)
        DDX_CHECK(IDC_HW_ENC, m_bHW_Enc)
        DDX_INT(IDC_EDIT_WIDTH, m_iWidth)
        DDX_INT(IDC_EDIT_HEIGHT, m_iHeight)
        DDX_INT(IDC_EDIT_FPS, m_iFps)
        DDX_TEXT(IDC_EDIT_DIR, m_strDIR)
        DDX_CONTROL_HANDLE(IDC_SLIDER1, m_s1)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_TIMER,   OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_ADD, BN_CLICKED, OnBnClickedAdd)
        COMMAND_HANDLER(IDC_DEL, BN_CLICKED, OnBnClickedDel)
        COMMAND_HANDLER(IDC_PLAY_START_STOP, BN_CLICKED, OnBnClickedPlayStartStop)
        COMMAND_HANDLER(IDC_PLAY_PAUSE_RESUME, BN_CLICKED, OnBnClickedPlayPauseResume)
        COMMAND_HANDLER(IDC_EXPORT, BN_CLICKED, OnBnClickedExport)
        COMMAND_HANDLER(IDC_EXPORT_BREAK, BN_CLICKED, OnBnClickedExportBreak)
        COMMAND_HANDLER(IDC_EXPORT_DIR, BN_CLICKED, OnBnClickedExportDir)
    END_MSG_MAP()


    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedPlayStartStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedPlayPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedExportBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedExportDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CFfmpeg_H)
