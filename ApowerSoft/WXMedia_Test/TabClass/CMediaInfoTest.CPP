
// 功能: 使用MediaInfo.DLL获取多媒体文件信息

#include "stdafx.h"
#include "CMediaInfoTest.h"

#include "MediaInfoDLL.h"

LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	GetDlgItem(IDC_FILE_MI).MoveWindow(7, 7, 100, 45);
	GetDlgItem(IDC_FILENAME_MI).MoveWindow(120, 7, rc.right - 130, 45);

	GetDlgItem(IDC_MSG_MI).MoveWindow(7, 60, rc.right - 14, rc.bottom - 70);


	return 0;
}

LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	return TRUE;
}
LRESULT CLASS_NAME::OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strInput = dlg.m_ofn.lpstrFile;

		MediaInfoDLL::MediaInfo MI;
		MI.Open((LPCTSTR)m_strInput);
		CString width, height, count, rate, duration;
		width = MI.Get(MediaInfoDLL::stream_t::Stream_Video, 0, L"Width").c_str();
		height = MI.Get(MediaInfoDLL::stream_t::Stream_Video, 0, L"Height").c_str();
		count = MI.Get(MediaInfoDLL::stream_t::Stream_Video, 0, L"FrameCount").c_str();
		rate = MI.Get(MediaInfoDLL::stream_t::Stream_Video, 0, L"FrameRate").c_str();
		duration = MI.Get(MediaInfoDLL::stream_t::Stream_Video, 0, L"Duration").c_str();

		m_strMsg = MI.Inform().c_str();

		MI.Close();

		//m_strMsg.Format(L"%ws\r\nSize[%wsx%ws]\r\nRate[%ws fps]\r\nduration[%ws ms]\r\n",
		//	m_strInput, width, height, rate, duration);

		DoDataExchange(FALSE);
	}
	return 0;
}
