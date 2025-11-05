// WXMedia 播放器测试对话框

#include "stdafx.h"
#include "CWXPlayer.h"

//缩放窗口
LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	//GetDlgItem(IDC_START_STOP_PLAY).MoveWindow  (7,  7, 100, 45);
	//GetDlgItem(IDC_PAUSE_RESUME_PLAY).MoveWindow(117,7, 100, 45);
	//GetDlgItem(IDC_CHECK_LAV_PLAY).MoveWindow   (227, 15, 100, 45);
	//GetDlgItem(IDC_CHECK_HW_PLAY).MoveWindow      (337,15, 100, 45);
	//GetDlgItem(IDC_COMBO_SPEED_PLAY).MoveWindow (447, 23, 100, 45);
	//GetDlgItem(IDC_TIME_PLAY).MoveWindow(557, 23, 150, 45);
	//GetDlgItem(IDC_SLIDER_PLAY).MoveWindow      (7,  60, rc.right - 14, 45);
	GetDlgItem(IDC_PIC_PLAY).MoveWindow(7, 120, rc.right - 14, rc.bottom - 130);
	return 0;
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
	if(m_player == nullptr)
	{
		CFileDialog fileDlg(TRUE);
		if (IDOK == fileDlg.DoModal())
		{
			m_strFileName = fileDlg.m_ofn.lpstrFile;
		}
		else {
			return 0;
		}
		WXIniSetValue(L"HwVideoDecode", m_bHW); //使用硬解码
		WXIniSetValue(L"MediaPlayer", m_bLav); // 是否使用LAV播放器
		m_player = WXFfplayCreate(L"FFPLAY", m_strFileName, 100, 0);
		if (m_player) {
			WXFfplaySetView(m_player, m_hWndVideo);
			WXFfplayStart(m_player);
			m_bPause = false;
			WXFfplayResume(m_player);
			m_duration = WXFfplayGetTotalTime(m_player); //总时长 MS
			GetDlgItem(IDC_START_STOP_PLAY).SetWindowTextW(L"停止");
		}
	}
	else
	{
		WXFfplayPause(m_player);
		WXFfplayStop(m_player);
		WXFfplayDestroy(m_player);
		m_player = nullptr;
		GetDlgItem(IDC_START_STOP_PLAY).SetWindowTextW(L"播放");
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_player != nullptr)
	{
		//暂停和继续
		if (m_bPause)
		{
			WXFfplayResume(m_player);
			m_bPause = false;
			GetDlgItem(IDC_PAUSE_RESUME_PLAY).SetWindowTextW(L"Pause");
		}
		else {
			WXFfplayPause(m_player);
			m_bPause = true;
			GetDlgItem(IDC_PAUSE_RESUME_PLAY).SetWindowTextW(L"Resume");
		}
	}
	return 0;
}

//倍速选择
LRESULT CLASS_NAME::OnCbnSelchangeComboSpeed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	int nIndex = m_cmbSpeed.GetCurSel();
	if(nIndex != CB_ERR && m_player != nullptr)
	{
		float speed = 1.0f;
		switch (nIndex)
		{
		case 0:
			speed = 0.5f;
			break;
		case 1:
			speed = 1.0f;
			break;
		case 2:
			speed = 1.5f;
			break;
		case 3:
			speed = 2.0f;
			break;
		case 4:
			speed = 4.0f;
			break;
		default:
			break;
		}
		WXFfplaySpeed(m_player, speed);
	}
	return 0;
}

LRESULT CLASS_NAME::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
		// 检查是否是IDC_SLIDER1发送的消息
    // 检查是否是IDC_SLIDER1发送的消息
    if ((::GetDlgCtrlID((HWND)lParam)) == IDC_SLIDER_PLAY && m_player != nullptr)
    {
		int nPos = m_slider.GetPos();

		int64_t seekPos = (m_duration * nPos) / 100; //计算要跳转到的时间位置 MS
		// 使用控件变量更新滑块位置
		//m_slider.SetPos(nPos);
		WXFfplaySeek(m_player, seekPos);//播放跳转

		return 0; // 已处理消息
    }

	// 不是目标滑块控件的消息，交给默认处理
	bHandled = FALSE;
	return 0;
}

LRESULT CLASS_NAME::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (wParam == TIMER_ID && m_player) {
		int64_t currentPos = WXFfplayGetCurrTime(m_player); //获取当前播放位置 MS
		if (m_duration > 0) {
			int pos = (int)((currentPos * 100) / m_duration);
			m_slider.SetPos(pos);

			int sec1 = (currentPos/1000) % 60;//秒
			int min1 = ((currentPos / 1000) / 60) % 60;//分
			int hour1 = (currentPos / 1000) / 3600;

			int sec2 = (m_duration/1000) % 60;//秒
			int min2 = ((m_duration / 1000) / 60) % 60;//分
			int hour2 = (m_duration / 1000) / 3600;
			CString str;
			str.Format(L"%02d:%02d:%02d/%02d:%02d:%02d", hour1, min1, sec1, hour2, min2, sec2);
			GetDlgItem(IDC_TIME_PLAY).SetWindowText(str);
		}
	}
	return 0;
}


