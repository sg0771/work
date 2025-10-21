// 运行ffmpeg.exe 等其它exe
//并且将输出结果显示在界面上

#include "stdafx.h"
#include "CCmd.h"

//窗体大小改变
LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	GetDlgItem(IDC_INPUT_CMD).MoveWindow(7, 7, 100, 45);
	GetDlgItem(IDC_CMD_NAME_CMD).MoveWindow(120, 7, rc.right - 370, 45);
	GetDlgItem(IDC_RUN_CMD).MoveWindow(rc.right - 235, 7, 100, 45);
	GetDlgItem(IDC_BREAK_CMD).MoveWindow(rc.right - 125, 7, 100, 45);

	GetDlgItem(IDC_PARAM_CMD).MoveWindow(7, 60, rc.right - 14, 100);
	GetDlgItem(IDC_LOG_CMD).MoveWindow(7, 170, rc.right - 14, rc.bottom - 175);
	return 0;
}

//初始化对话框
LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);


	std::wstring  strDir = WXBase::GetExePath();
	m_strCmdName = strDir.c_str();
	m_strCmdName += L"ffmpeg.exe";


	DoDataExchange(FALSE);

	return 0;
}

//定时器消息
LRESULT CLASS_NAME::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (wParam == TIMER_ID)
	{
		// 检查ffmpeg是否还在运行
		if (!IsFFmpegRunning())
		{
			// 停止定时器
			KillTimer(TIMER_ID);

			// 读取剩余输出
			CString strOutput;

			ReadPipeOutput(m_hPipeStdoutRead, strOutput);
			if (!strOutput.IsEmpty())
				m_strLog += strOutput;

			ReadPipeOutput(m_hPipeStderrRead, strOutput);
			if (!strOutput.IsEmpty())
				m_strLog += strOutput;

			// 清理资源
			CleanupResources();

			// 通知完成
			CString strMsg;
			strMsg.Format(_T(" %ws 处理完成\r\n"), (LPCWSTR)m_strCmdName);

			m_strLog += strMsg;
			//PostMessage(WM_UPDATE_LOG, 0, (LPARAM)(LPCTSTR)_T("ffmpeg处理完成\r\n"));

			// 更新按钮状态
			GetDlgItem(IDC_RUN_CMD).EnableWindow(TRUE);
			GetDlgItem(IDC_BREAK_CMD).EnableWindow(FALSE);

			// 限制日志长度，防止内存占用过大
			if (m_strLog.GetLength() > 100000)
			{
				m_strLog = m_strLog.Mid(m_strLog.GetLength() - 100000);
			}


			DoDataExchange(FALSE);
		}
		else
		{
			// 读取并显示输出
			CString strOutput;
			ReadPipeOutput(m_hPipeStdoutRead, strOutput);
			m_strLog += strOutput;

			ReadPipeOutput(m_hPipeStderrRead, strOutput);
			m_strLog += strOutput;

			// 限制日志长度，防止内存占用过大
			if (m_strLog.GetLength() > 100000)
			{
				m_strLog = m_strLog.Mid(m_strLog.GetLength() - 100000);
			}

			DoDataExchange(FALSE);
		}
	}

	return 0;
}

//销毁
LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	return 0;
}

LRESULT CLASS_NAME::OnBnClickedInputCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, L"exe files (*.exe)\0*.exe\0All files (*.*)\0*.*\0", m_hWnd);
	if (dlg.DoModal() == IDOK)
	{
		m_strCmdName = dlg.m_ofn.lpstrFile;
		DoDataExchange(FALSE);
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedRunCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	DoDataExchange(TRUE);

	if (m_hProcess != nullptr) {
		CString strMsg;
		strMsg.Format(_T(" %ws 正在运行中，请先停止！"), (LPCWSTR)m_strCmdName);
		::MessageBox(NULL, strMsg, _T("提示"), MB_OK);
		return 0;
	}
	m_strLog = L"";


	// 先清理之前的资源
	CleanupResources();

	// 创建管道的安全属性
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// 创建stdout管道
	if (!CreatePipe(&m_hPipeStdoutRead, &m_hPipeStdoutWrite, &sa, 0))
	{
		::MessageBox(NULL, _T("创建标准输出管道失败！"), _T("提示"), MB_OK);
		return -1;
	}
	SetHandleInformation(m_hPipeStdoutRead, HANDLE_FLAG_INHERIT, 0);

	// 创建stderr管道
	if (!CreatePipe(&m_hPipeStderrRead, &m_hPipeStderrWrite, &sa, 0)){
		::MessageBox(NULL, _T("创建错误输出管道失败！"), _T("提示"), MB_OK);
		CleanupResources();
		return -1;
	}
	SetHandleInformation(m_hPipeStderrRead, HANDLE_FLAG_INHERIT, 0);

	// 准备ffmpeg命令行
	CString strCmdLine;
	strCmdLine.Format(L"\"%ws\" %ws", (LPCWSTR)m_strCmdName, (LPCWSTR)m_strParam);

	// 设置进程启动信息
	STARTUPINFO si = { 0 };
	si.cb = sizeof(STARTUPINFO);
	si.hStdOutput = m_hPipeStdoutWrite;
	si.hStdError = m_hPipeStderrWrite;
	si.dwFlags = STARTF_USESTDHANDLES;

	// 启动ffmpeg进程
	PROCESS_INFORMATION pi = { 0 };
	if (!CreateProcess(NULL, strCmdLine.GetBuffer(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CString strErr;
		strErr.Format(_T("启动EXE失败！错误代码: %d"), GetLastError());
		::MessageBox(NULL, strErr, _T("提示"), MB_OK);
		CleanupResources();
		return -1;
	}
	strCmdLine.ReleaseBuffer();

	// 保存进程句柄，关闭线程句柄
	m_hProcess = pi.hProcess;
	CloseHandle(pi.hThread);

	// 启动定时器，每隔100ms读取一次输出
	SetTimer(TIMER_ID, 100, NULL);//设置定时器

	// 更新按钮状态
	GetDlgItem(IDC_RUN_CMD).EnableWindow(FALSE);
	GetDlgItem(IDC_BREAK_CMD).EnableWindow(TRUE);

	// 输出启动信息
	CString strInfo;
	strInfo.Format(_T("已启动进程，命令: %ws\r\n"), (LPCWSTR)strCmdLine);
	m_strLog += strInfo;
	DoDataExchange(FALSE);
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedBreakCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_hProcess != NULL)
	{
		// 强制终止进程
		TerminateProcess(m_hProcess, 0);
		//PostMessage(WM_UPDATE_LOG, 0, (LPARAM)(LPCTSTR)_T("已强制终止ffmpeg进程\r\n"));
	}

	// 清理资源
	CleanupResources();

	// 更新按钮状态
	GetDlgItem(IDC_RUN_CMD).EnableWindow(TRUE);
	GetDlgItem(IDC_BREAK_CMD).EnableWindow(FALSE);

	return 0;
}

// 清理资源
void CLASS_NAME::CleanupResources()
{
	// 停止定时器
	KillTimer(TIMER_ID);

	// 关闭管道句柄
	if (m_hPipeStdoutRead != NULL)
	{
		CloseHandle(m_hPipeStdoutRead);
		m_hPipeStdoutRead = NULL;
	}
	if (m_hPipeStdoutWrite != NULL)
	{
		CloseHandle(m_hPipeStdoutWrite);
		m_hPipeStdoutWrite = NULL;
	}
	if (m_hPipeStderrRead != NULL)
	{
		CloseHandle(m_hPipeStderrRead);
		m_hPipeStderrRead = NULL;
	}
	if (m_hPipeStderrWrite != NULL)
	{
		CloseHandle(m_hPipeStderrWrite);
		m_hPipeStderrWrite = NULL;
	}

	// 关闭进程句柄
	if (m_hProcess != NULL)
	{
		CloseHandle(m_hProcess);
		m_hProcess = NULL;
	}
}
// 检查ffmpeg是否正在运行
BOOL CLASS_NAME::IsFFmpegRunning()
{
	if (m_hProcess == NULL)
		return FALSE;

	DWORD dwExitCode;
	if (GetExitCodeProcess(m_hProcess, &dwExitCode))
	{
		return (dwExitCode == STILL_ACTIVE);
	}
	return FALSE;
}

// 读取管道输出
void CLASS_NAME::ReadPipeOutput(HANDLE hPipe, CString& strOutput)
{
	if (hPipe == NULL)
		return;

	// 检查管道是否有数据可读
	DWORD dwBytesAvailable;
	if (!PeekNamedPipe(hPipe, NULL, 0, NULL, &dwBytesAvailable, NULL) || dwBytesAvailable == 0)
		return;

	// 读取数据
	CHAR buf_UTF8[4096] = { 0 };//UTF8
	DWORD dwRead = 0;
	if (ReadFile(hPipe, buf_UTF8, sizeof(buf_UTF8) - 1, &dwRead, NULL) && dwRead > 0)
	{
		buf_UTF8[dwRead] = '\0';

		// 转换UTF-8到Unicode
		std::wstring wstrOutput = WXBase::UTF8ToUTF16(buf_UTF8);
		strOutput = wstrOutput.c_str();
	}
}