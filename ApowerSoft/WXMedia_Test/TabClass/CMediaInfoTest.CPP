
// 功能: 使用MediaInfo.DLL获取多媒体文件信息

#include "stdafx.h"
#include "CMediaInfoTest.h"

#include "MediaInfoDLL.h"


LRESULT CLASS_NAME::OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HDROP hDrop = reinterpret_cast<HDROP>(wParam); // 拖拽句柄
	TCHAR szFilePath[MAX_PATH] = { 0 };              // 存储文件路径（支持长路径）

	// 1. 检查拖拽的文件数量（这里仅处理“单个文件”，若需多文件可循环）
	UINT nFileCount = ::DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	if (nFileCount == 0)
	{
		::DragFinish(hDrop); // 必须释放拖拽句柄，避免内存泄漏
		return 0;
	}

	// 2. 提取第一个文件的路径（DragQueryFile 第二个参数为 0 表示第一个文件）
	::DragQueryFile(hDrop, 0, szFilePath, MAX_PATH);

	// 3. 将文件路径显示到 EDIT 控件中


	// 4. 关键：释放拖拽句柄（必须调用，否则会导致资源泄漏）
	::DragFinish(hDrop);

	m_strInput = szFilePath;
	Handle();
	return 0;
}

LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	GetDlgItem(IDC_FILE_MI).MoveWindow(7, 7, 100, 45);
	GetDlgItem(IDC_SAVE_MI).MoveWindow(120, 7, 150, 45);
	GetDlgItem(IDC_WX_MI).MoveWindow(280, 7, 150, 45);
	GetDlgItem(IDC_MSG_MI).MoveWindow(7, 60, rc.right - 14, rc.bottom - 70);


	return 0;
}

LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	::DragAcceptFiles(m_hWnd, TRUE); // 第二个参数必须为 TRUE（允许接受）


	return TRUE;
}
LRESULT CLASS_NAME::OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strInput = dlg.m_ofn.lpstrFile;
		Handle();
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedSaveMi(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// 步骤1：弹出“保存文件”对话框，过滤 TXT 文件
	CFileDialog dlg(FALSE, _T("txt"), m_strName.c_str(),
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		_T("文本文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||"));

	// 若用户点击“保存”按钮（IDOK）
	if (dlg.DoModal(m_hWnd) == IDOK)
	{
		std::ofstream ofs(dlg.m_ofn.lpstrFile, std::ios::out | std::ios::binary);
		if (ofs.is_open())
		{
			ofs.write((char*)m_strMsg.GetBuffer(), m_strMsg.GetLength() * 2);
			ofs.close();
		}
	}

	return 0;
}


void CLASS_NAME::Handle() {

	std::wstring s_FullName = (LPCTSTR)m_strInput; //全路径
	std::filesystem::path path(s_FullName);
	//std::wstring s_ExeName = path.filename().wstring();//EXE名字
	std::wstring s_Name = path.stem().wstring();//EXE名字
	m_strName = s_Name.c_str();
	m_strName += L".txt";

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

	DoDataExchange(FALSE);

}
LRESULT CLASS_NAME::OnBnClickedWxMi(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
		// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strInput = dlg.m_ofn.lpstrFile;
		int err = 0;
		void* pMI = WXMediaInfoCreateFast(m_strInput, &err);
		if (pMI) {
			int64_t time = WXMediaInfoGetFileDuration(pMI);
			int64_t vb   = WXMediaInfoGetVideoBitrate(pMI);
			m_strMsg.Format(L"%ws\r\nTime=%lld\r\nVB=%lld\r\n", m_strInput, time,vb);
			WXMediaInfoDestroy(pMI);
			DoDataExchange(FALSE);
		}
	}
	return 0;
}
