// MPV 播放器测试对话框 

#include "stdafx.h"
#include "CMPVPlayer.h"

EXTERN_C void  WXLogW(const wchar_t* format, ...) {}

//缩放窗口
LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	//GetDlgItem(IDC_START_STOP_PLAY).MoveWindow(7, 7, 100, 45);
	//GetDlgItem(IDC_PAUSE_RESUME_PLAY).MoveWindow(117, 7, 100, 45);
	//GetDlgItem(IDC_CHECK_LAV_PLAY).MoveWindow(227, 15, 100, 45);
	//GetDlgItem(IDC_CHECK_HW_PLAY).MoveWindow(337, 15, 100, 45);
	//GetDlgItem(IDC_COMBO_SPEED_PLAY).MoveWindow(447, 23, 100, 45);
	//GetDlgItem(IDC_TIME_PLAY).MoveWindow(557, 23, 150, 45);
	//GetDlgItem(IDC_SLIDER_PLAY).MoveWindow(7, 60, rc.right - 14, 45);
	GetDlgItem(IDC_PIC_PLAY).MoveWindow(7, 120, rc.right - 14, rc.bottom - 130);
	return 0;
}

// 处理 mpv 事件
void CLASS_NAME::HandleMpvEvents()
{
	if (!m_mpv) return;

	while (true)
	{
		mpv_event* event = LibMPV::Inst().m_mpv_wait_event(m_mpv, 0); // 使用0超时进行非阻塞轮询
		if (event->event_id == MPV_EVENT_NONE) {
			break; // 没有更多事件了
		}

		if (event->event_id == MPV_EVENT_SHUTDOWN) {
			TerminateMpv();
			break;
		}

		if (event->event_id == MPV_EVENT_PROPERTY_CHANGE) {
			mpv_event_property* prop = (mpv_event_property*)event->data;
			if (strcmp(prop->name, "time-pos") == 0) {
				if (prop->data && prop->format == MPV_FORMAT_DOUBLE) {
					double position = *(double*)prop->data;
					UpdatePlaybackTime(position);
				}
			}
			else if (strcmp(prop->name, "duration") == 0) {
				if (prop->data && prop->format == MPV_FORMAT_DOUBLE) {
					m_duration = *(double*)prop->data;
					m_slider.SetRange(0, (int)m_duration);
				}
			}
			else if (strcmp(prop->name, "pause") == 0) {
				if (prop->data && prop->format == MPV_FORMAT_FLAG) {
					m_bIsPaused = *(int*)prop->data;
					GetDlgItem(IDC_PAUSE_RESUME_PLAY).SetWindowText(m_bIsPaused ? _T("播放") : _T("停止"));
				}
			}
		}
		else if (event->event_id == MPV_EVENT_END_FILE) {
			// 播放结束
			BOOL b = TRUE;
			OnBnClickedStartStop(0,0,0,b);
		}
	}
}

// 更新播放时间和滑块位置
void CLASS_NAME::UpdatePlaybackTime(double position)
{
	// 如果用户没有在拖动滑块，才更新滑块位置
	if (!m_bIsSeeking)
	{
		m_slider.SetPos((int)position);
	}

	CString timeStr;
	timeStr.Format(_T("%s / %s"), FormatTime(position), FormatTime(m_duration));
	GetDlgItem(IDC_TIME_PLAY).SetWindowText(timeStr);
}

//格式化时间为 HH:MM:SS
CString CLASS_NAME::FormatTime(double seconds)
{
	if (seconds < 0) return _T("00:00:00");
	int i_seconds = (int)seconds;
	int hours = i_seconds / 3600;
	int minutes = (i_seconds % 3600) / 60;
	int secs = i_seconds % 60;
	CString formatted_time;
	formatted_time.Format(_T("%02d:%02d:%02d"), hours, minutes, secs);
	return formatted_time;
}

// 终止并销毁 mpv 实例
void CLASS_NAME::TerminateMpv()
{
	if (m_mpv)
	{
		LibMPV::Inst().m_mpv_terminate_destroy(m_mpv);
		m_mpv = nullptr;
	}
}



//初始化窗口
LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);

	m_hWndVideo = GetDlgItem(IDC_PIC_PLAY).m_hWnd;
	m_cmbSpeed.InsertString(0, L"0.5X");
	m_cmbSpeed.InsertString(1, L"1.0X");
	m_cmbSpeed.InsertString(2, L"1.5X");
	m_cmbSpeed.InsertString(3, L"2.0X");
	m_cmbSpeed.InsertString(4, L"4.0X");
	m_cmbSpeed.SetCurSel(1);

	m_slider.SetRange(0, 100);
	m_slider.SetPos(0);


	// 获取一行文本的高度
	int nItemHeight = m_cmbSpeed.GetItemHeight(-1); // -1 表示列表项高度
	if (nItemHeight > 0)
	{
		// 计算下拉高度（一行高度 + 边框补偿）
		int nDroppedHeight = nItemHeight * 5 + 8;

		// 发送 CB_SETDROPPEDHEIGHT 消息设置下拉高度
		m_cmbSpeed.SendMessage(CB_SETDROPPEDHEIGHT, nDroppedHeight, 0);
	}


	//init code
	DoDataExchange(FALSE);

	SetTimer(TIMER_ID, 500, NULL);//设置定时器

	if (!LibMPV::Inst().m_lib->Enabled()) {
		::MessageBox(NULL, _T("加载 libmpv.dll 失败!"), _T("ERROR"), MB_OK);
		return -1;
	}

	// 创建 mpv 实例
	m_mpv = LibMPV::Inst().m_mpv_create();
	if (!m_mpv) {
		::MessageBox(NULL, _T("创建 mpv 实例失败!"), _T("ERROR"), MB_OK);
		return  -1;
	}


	// 初始化 mpv 实例
	if (LibMPV::Inst().m_mpv_initialize(m_mpv) < 0) {
		::MessageBox(NULL, _T("初始化 mpv 失败!"), _T("ERROR"),MB_OK);
		TerminateMpv();
		return -1;
	}



	m_bInitMPV = TRUE;

	// 将视频输出绑定到我们的静态控件上
	m_hWndVideo = GetDlgItem(IDC_PIC_PLAY);
	int64_t wid = (int64_t)m_hWndVideo;
	LibMPV::Inst().m_mpv_set_option(m_mpv, "wid", MPV_FORMAT_INT64, &wid);

	// 监听我们需要的属性变化，这样可以异步更新UI
	LibMPV::Inst().m_mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
	LibMPV::Inst().m_mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
	LibMPV::Inst().m_mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);

	return TRUE;
}

LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	KillTimer(TIMER_ID);
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedStartStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	DoDataExchange(TRUE);
	if (!m_bIsPlaying) {
		// CFileDialog最后一个参数改为TRUE表示是打开文件对话框
		CFileDialog fileDlg(TRUE);

		if (fileDlg.DoModal() == IDOK)
		{
			CString filePath = fileDlg.m_ofn.lpstrFile;
			// libmpv 需要 UTF-8 编码的路径
			CT2A utf8Path(filePath, CP_UTF8);
			const char* cmd[] = { "loadfile", utf8Path.m_psz, NULL };
			LibMPV::Inst().m_mpv_command(m_mpv, cmd);

			m_bIsPlaying = true;
			m_bIsPaused = false;

			GetDlgItem(IDC_START_STOP_PLAY).EnableWindow(TRUE);
			GetDlgItem(IDC_PAUSE_RESUME_PLAY).EnableWindow(TRUE);
			m_slider.EnableWindow(TRUE);
			GetDlgItem(IDC_START_STOP_PLAY).SetWindowText(_T("暂停"));
		}
	}
	else {
		if (!m_bIsPlaying || !m_mpv) return  -1;

		const char* cmd[] = { "stop", NULL };
		LibMPV::Inst().m_mpv_command(m_mpv, cmd);

		m_bIsPlaying = false;
		m_bIsPaused = false;
		m_duration = 0.0;

		m_slider.SetPos(0);
		GetDlgItem(IDC_TIME_PLAY).SetWindowText(_T("00:00:00 / 00:00:00"));

		GetDlgItem(IDC_START_STOP_PLAY).SetWindowText(_T("播放"));
		GetDlgItem(IDC_PAUSE_RESUME_PLAY).EnableWindow(FALSE);
		GetDlgItem(IDC_START_STOP_PLAY).EnableWindow(FALSE);
		m_slider.EnableWindow(FALSE);
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (!m_bIsPlaying || !m_mpv) return  -1;

	m_bIsPaused = !m_bIsPaused;
	int flag = m_bIsPaused ? 1 : 0;
	LibMPV::Inst().m_mpv_set_property(m_mpv, "pause", MPV_FORMAT_FLAG, &flag);
	return 0;
}

//倍速选择
LRESULT CLASS_NAME::OnCbnSelchangeComboSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (!m_mpv) return -1;

	int sel = m_cmbSpeed.GetCurSel();
	double speed = 1.0;
	switch (sel) {
	case 0: speed = 0.5; break;
	case 1: speed = 1.0; break;
	case 2: speed = 1.5; break;
	case 3: speed = 2.0; break;
	}
	LibMPV::Inst().m_mpv_set_property(m_mpv, "speed", MPV_FORMAT_DOUBLE, &speed);
	return 0;
}

//水平滑动条事件
LRESULT CLASS_NAME::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
		// 检查是否是IDC_SLIDER1发送的消息
    // 检查是否是IDC_SLIDER1发送的消息
    if ((::GetDlgCtrlID((HWND)lParam)) == IDC_SLIDER_PLAY && m_mpv)
    {
		int nPos = m_slider.GetPos();

		double new_pos = (m_duration * nPos) / 100; //计算要跳转到的时间位置 MS

		//		// 当用户释放滑块时才进行跳转
		//if (nSBCode == TB_THUMBPOSITION)

		m_bIsSeeking = false;
		char pos_str[64];
		sprintf_s(pos_str, "%f", new_pos);
		const char* cmd[] = { "seek", pos_str, "absolute", NULL };
		LibMPV::Inst().m_mpv_command(m_mpv, cmd);

		return 0; // 已处理消息
    }

	// 不是目标滑块控件的消息，交给默认处理
	bHandled = FALSE;
	return 0;
}

//定时器事件
LRESULT CLASS_NAME::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (wParam == TIMER_ID) {
		HandleMpvEvents();
	}
	return 0;
}


