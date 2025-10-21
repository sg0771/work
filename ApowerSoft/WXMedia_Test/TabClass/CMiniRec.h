// ApowerSoft Corporation. All Rights Reserved.
// 显示器录制+声音录制+摄像头录制+系统声音录制+麦克风录制+鼠标点击录制+键盘按键录制

#if !defined(CMiniRec_H)
#define CMiniRec_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CMiniRec
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{

    BOOL m_bHW = TRUE;
    BOOL m_bDXGI = TRUE;
    CComboBox m_cbDisplay;//显示器列表
    CComboBox m_cbSystem; //扬声器列表
    CComboBox m_cbMic;    //麦克风列表
    CComboBox m_cbVC;     //视频编码器
    CComboBox m_cbAC;     //音频编码器
    CComboBox m_cbFPS;    //֡帧率
    CComboBox m_cbVB;     //视频码率
    CComboBox m_cbType;     //文件格式

    BOOL m_bPause = FALSE;//暂停状态
    ATOM m_atomStartAndStop = 0;      //启动结束  ALT+C
    ATOM m_atomPauseAndResume = 0;    //暂停恢复 ALT+K

	CString m_strDir = L"";//输出目录
    CString m_strFileName = L"";//录制文件

    void* m_pCapture = nullptr;//录屏对象
    void ProcDir(const wchar_t* dir);//创建路径

    void SetDir();//设置输出目录
    void OpenDir();//打开输出目录
    void Start();//启动录制
    void Stop();//结束录制
    void Pause();//暂停录制
    void Resume();//恢复录制

    DWORD m_tsStart = 0;
public:
    enum { IDD = IDD_MINIREC };

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)     
        DDX_CHECK(IDC_CHECK_DXGI, m_bDXGI)
        DDX_CHECK(IDC_CHECK_HW, m_bHW)
        DDX_CONTROL_HANDLE(IDC_CB_DISPLAY, m_cbDisplay)
        DDX_CONTROL_HANDLE(IDC_CB_SYSTEM, m_cbSystem)
        DDX_CONTROL_HANDLE(IDC_CB_MIC, m_cbMic)
        DDX_CONTROL_HANDLE(IDC_CB_VIDEO_CODEC, m_cbVC)
        DDX_CONTROL_HANDLE(IDC_CB_AUDIO_CODEC, m_cbAC)
        DDX_CONTROL_HANDLE(IDC_CB_FPS, m_cbFPS)
        DDX_CONTROL_HANDLE(IDC_CB_VB, m_cbVB)
        DDX_CONTROL_HANDLE(IDC_CB_FORMAT, m_cbType)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_START_REC, BN_CLICKED, OnBnClickedStartRec)
        COMMAND_HANDLER(IDC_SET_DIR_REC, BN_CLICKED, OnBnClickedSetDirRec)
        COMMAND_HANDLER(IDC_OPEN_DIR_REC, BN_CLICKED, OnBnClickedOpenDirRec)
        COMMAND_HANDLER(IDC_CB_FORMAT, CBN_SELCHANGE, OnCbnSelchangeCbFormat)
    END_MSG_MAP()


    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedStartRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSetDirRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedOpenDirRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangeCbFormat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CMiniRec_H)
