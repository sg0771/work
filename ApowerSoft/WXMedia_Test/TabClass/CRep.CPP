#include "stdafx.h"
#include "CRep.h"

LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	m_prog.SetRange(0, 100);
	m_prog.SetPos(0);
	SetTimer(TIMER_ID, 100, NULL);//设置定时器
	return TRUE;
}

LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	KillTimer(TIMER_ID);
	return 0;
}

LRESULT CLASS_NAME::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

	if (wParam == TIMER_ID) {
		//定时器事件
		int pRate = WXMediaFileEditGetRate();
		if (pRate == 100) {
			//转换完成
			//结束线程
		}
		m_prog.SetPos(pRate);
	}

	return 0;
}


LRESULT CLASS_NAME::OnBnClickedInput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	//设置输入路径
	CFileDialog dlg(TRUE);
	if (dlg.DoModal() == IDOK) {
		std::wstring strInput = dlg.m_ofn.lpstrFile;
		//检测后缀名
		int error = 0;
		void* pInfo = WXMediaInfoCreate(strInput.c_str(), &error);
		if (error != 0 || pInfo == NULL) {
			CString msg;
			msg.Format(L"%ws 不是有效的视频文件,错误码:%d", strInput.c_str(), error);
			::MessageBox(NULL, msg, L"Error", MB_OK);
			return 0;
		}

		m_strInput = strInput.c_str();
		std::wstring strName = m_strInput;
		std::wstring strNewName;
		// 查找最后一个 '.' 的位置
		size_t dotPos = strName.find_last_of(L'.');

		// 提取后缀名
		std::wstring suffix = strName.substr(dotPos + 1);

		// 不区分大小写判断是否为mp4
		if (wcsicmp(suffix.c_str(), L"xws") == 0 || wcsicmp(suffix.c_str(), L"wxm") == 0) {
			// 替换后缀为 .flv
			strNewName = strName.substr(0, dotPos + 1) + L"mp4";
			m_strOutput = strNewName.c_str();
		}

		DoDataExchange(FALSE);
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedOutput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// 设置输出路径
	CFileDialog dlg(FALSE);
	if (dlg.DoModal() == IDOK) {
		m_strOutput = dlg.m_ofn.lpstrFile;
		DoDataExchange(FALSE);
	}
	return 0;
}


LRESULT CLASS_NAME::OnBnClickedRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	DoDataExchange(TRUE);
	if (m_strInput.IsEmpty() || m_strOutput.IsEmpty()) {
		::MessageBox(NULL, L"请输入有效的路径", L"Error", MB_OK);
		return 0;
	}

	if (!m_bRunning) {
		std::thread([this]() {
			m_bRunning = TRUE;

			int ret = WXConvertFast2(m_strInput, m_strOutput, 0);
			if (ret == 1) {
				::MessageBox(NULL, L"转换成功", L"OK", MB_OK);
			}else if (ret == 0) {
				::MessageBox(NULL, L"解析输入文件失败", L"Error", MB_OK);
			}else {
				::MessageBox(NULL, L"转换失败", L"Error", MB_OK);
			}
			m_bRunning = FALSE;
		}).detach();
	}
	else {
		::MessageBox(NULL, L"正在转换", L"Error", MB_OK);
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WXMediaFileEditBreak();//中断运行
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedCut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	DoDataExchange(TRUE);
	if (m_strInput.IsEmpty() || m_strOutput.IsEmpty()) {
		::MessageBox(NULL, L"请输入有效的路径", L"Error", MB_OK);
		return 0;
	}

	if (!m_bRunning) {
		std::thread([this]() {
			m_bRunning = TRUE;

			int ret = WXMediaFileEdit2(m_strInput, m_strOutput, 0, 10 * 1000);
			if (ret == WX_ERROR_SUCCESS) {
				::MessageBox(NULL, L"转换成功", L"OK", MB_OK);
			}
			else{
				::MessageBox(NULL, L"转换失败", L"Error", MB_OK);
			}
			m_bRunning = FALSE;
			}).detach();
	}
	else {
		::MessageBox(NULL, L"正在转换", L"Error", MB_OK);
	}
	return 0;
}
