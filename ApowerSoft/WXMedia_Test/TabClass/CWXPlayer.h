// WXMedia 播放器测试对话框

#if !defined(CWXPlayer_H)
#define CWXPlayer_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CWXPlayer
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{

	CString m_strFileName = L"test.mp4";
	BOOL m_bLav = TRUE;//是否使用lav解码
	BOOL m_bHW = TRUE;//是否使用硬解码
	CComboBox m_cmbSpeed;
    CTrackBarCtrl  m_slider;
	void* m_player = nullptr;//播放器句柄
	HWND m_hWndVideo = nullptr;//视频显示窗口句柄
	bool m_bPause = false;
	int64_t m_duration = 0;//总时长 MS
public:
    enum { IDD = IDD_PLAYTEST};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

	BEGIN_DDX_MAP(CLASS_NAME)
		DDX_CHECK(IDC_CHECK_LAV_PLAY, m_bLav)
        DDX_CHECK(IDC_CHECK_HW_PLAY, m_bHW)
		DDX_CONTROL_HANDLE(IDC_COMBO_SPEED_PLAY, m_cmbSpeed)
        DDX_CONTROL_HANDLE(IDC_SLIDER_PLAY, m_slider)
	END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)   
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_START_STOP_PLAY,   BN_CLICKED, OnBnClickedStartStop)
        COMMAND_HANDLER(IDC_PAUSE_RESUME_PLAY, BN_CLICKED, OnBnClickedPauseResume)
        COMMAND_HANDLER(IDC_COMBO_SPEED_PLAY,  CBN_SELCHANGE, OnCbnSelchangeComboSpeed)
    END_MSG_MAP()

    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnBnClickedStartStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangeComboSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CWorkWithShader_H)
