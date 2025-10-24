// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "MediaInfoDLL.h"

LRESULT CMainDlg::OnDropFiles(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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


// 从完整路径中提取不含扩展名的文件名（宽字符串版本）
// 从完整路径（CString）中提取不含扩展名的文件名
CString GetFileNameWithoutExtension(const CString& strFullPath) {
	// 步骤1：提取文件名（含扩展名）
	// 查找最后一个路径分隔符（同时处理 Windows 的\和 Linux/macOS 的/）
	int nLastSlash = strFullPath.ReverseFind('\\'); // 查找最后一个反斜杠
	int nLastBackSlash = strFullPath.ReverseFind('/'); // 查找最后一个斜杠
	int nLastSep = std::max(nLastSlash, nLastBackSlash); // 取最后出现的分隔符位置

	CString strFileName;
	if (nLastSep == -1) {
		// 路径中没有分隔符，整个字符串即为文件名
		strFileName = strFullPath;
	}
	else {
		// 截取分隔符之后的部分作为文件名（含扩展名）
		strFileName = strFullPath.Mid(nLastSep + 1);
	}

	// 步骤2：移除文件名中的扩展名（最后一个.之后的部分）
	int nLastDot = strFileName.ReverseFind('.');
	if (nLastDot == -1) {
		// 文件名无扩展名，直接返回
		return strFileName;
	}
	else {
		// 截取最后一个.之前的部分（不含.）
		return strFileName.Left(nLastDot);
	}
}
void CMainDlg::Handle() {
	std::wstring s_Name = GetFileNameWithoutExtension(m_strInput);//无目录，无后缀的文件名

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
LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);
	::DragAcceptFiles(m_hWnd, TRUE); // 第二个参数必须为 TRUE（允许接受）
	return TRUE;
}


LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}


LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	return 0;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	GetDlgItem(IDC_INPUT).MoveWindow(7, 7, 100, 45);
	GetDlgItem(IDC_SAVE).MoveWindow(127, 7, 100, 45);
	GetDlgItem(IDC_EDIT1).MoveWindow(7, 60, rc.right - 14, rc.bottom - 70);
	return 0;
}

LRESULT CMainDlg::OnBnClickedInput(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strInput = dlg.m_ofn.lpstrFile;
		Handle();
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// 步骤1：弹出“保存文件”对话框，过滤 TXT 文件
	CFileDialog dlg(FALSE, _T("txt"), m_strName,
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		_T("文本文件 (*.txt)|*.txt|所有文件 (*.*)|*.*||"));

	// 若用户点击“保存”按钮（IDOK）
	if (dlg.DoModal(m_hWnd) == IDOK)
	{
		FILE* fout = _wfopen(dlg.m_ofn.lpstrFile,L"wb");
		if (fout) {
			fwrite((char*)m_strMsg.GetBuffer(), m_strMsg.GetLength() * 2, 1, fout);
			fclose(fout);
		}
	}
	return 0;
}
