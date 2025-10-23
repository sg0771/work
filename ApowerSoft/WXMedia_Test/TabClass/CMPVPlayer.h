// MPV 播放器测试对话框

#if !defined(CMPVPlayer_H)
#define CMPVPlayer_H

#include "stdafx.h"
#include "resource.h"

#include  <libmpv/client.h>
#include  <LibInst.hpp>

extern "C" {
	typedef mpv_handle* (*my_mpv_create)(void);
	typedef int (*my_mpv_set_option)(mpv_handle* ctx, const char* name, mpv_format format, void* data);
	typedef int (*my_mpv_initialize)(mpv_handle* ctx);
	typedef int (*my_mpv_observe_property)(mpv_handle* mpv, uint64_t reply_userdata, const char* name, mpv_format format);
	typedef void (*my_mpv_terminate_destroy)(mpv_handle* ctx);
	typedef int (*my_mpv_command)(mpv_handle* ctx, const char** args);
	typedef int (*my_mpv_set_property)(mpv_handle* ctx, const char* name, mpv_format format, void* data);
	typedef mpv_event* (*my_mpv_wait_event)(mpv_handle* ctx, double timeout);
}

class LibMPV
{
public:
	std::shared_ptr<LibInst::MyLib> m_lib = nullptr;
public:
	my_mpv_create m_mpv_create = nullptr;
	my_mpv_set_option m_mpv_set_option = nullptr;
	my_mpv_initialize m_mpv_initialize = nullptr;
	my_mpv_observe_property m_mpv_observe_property = nullptr;
	my_mpv_terminate_destroy m_mpv_terminate_destroy = nullptr;
	my_mpv_command m_mpv_command = nullptr;
	my_mpv_set_property m_mpv_set_property = nullptr;
	my_mpv_wait_event m_mpv_wait_event = nullptr;

public:
	LibMPV(const wchar_t* filename) {
		m_lib = std::make_shared<MyLib>(filename);
		if (m_lib->Enabled()) {
			m_mpv_create = (my_mpv_create)m_lib->GetFunction("mpv_create");
			m_mpv_set_option = (my_mpv_set_option)m_lib->GetFunction("mpv_set_option");
			m_mpv_initialize = (my_mpv_initialize)m_lib->GetFunction("mpv_initialize");
			m_mpv_observe_property = (my_mpv_observe_property)m_lib->GetFunction("mpv_observe_property");
			m_mpv_terminate_destroy = (my_mpv_terminate_destroy)m_lib->GetFunction("mpv_terminate_destroy");
			m_mpv_command = (my_mpv_command)m_lib->GetFunction("mpv_command");
			m_mpv_set_property = (my_mpv_set_property)m_lib->GetFunction("mpv_set_property");
			m_mpv_wait_event = (my_mpv_wait_event)m_lib->GetFunction("mpv_wait_event");
		}
	}
	static LibMPV& Inst() {
		static LibMPV s_inst(L"libmpv.dll");//
		return s_inst;
	}
};


#undef  CLASS_NAME
#define CLASS_NAME CMPVPlayer
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
	CString m_strFileName = L"test.mp4";


	BOOL m_bInitMPV = FALSE;//是否初始化了mpv

	BOOL m_bHW = TRUE;//是否使用硬解码

	CComboBox m_cmbSpeed;
    CTrackBarCtrl  m_slider;

	HWND m_hWndVideo = nullptr;//视频显示窗口句柄

	// libmpv 句柄
	mpv_handle* m_mpv = nullptr;

	// 状态变量
	bool m_bIsPlaying = false;
	bool m_bIsPaused = false;
	bool m_bIsSeeking = false; // 防止拖动滑块时与定时器更新冲突
	double m_duration = 0.0;   // 视频总时长

	// 自定义函数
	void TerminateMpv();
	void HandleMpvEvents();
	void UpdatePlaybackTime(double position);
	CString FormatTime(double seconds);
public:
    enum { IDD = IDD_MPV};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

	BEGIN_DDX_MAP(CLASS_NAME)
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
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    LRESULT OnBnClickedStartStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangeComboSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


};


#endif // !defined(CMPVPlayer_H)
