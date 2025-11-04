// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"
#include <memory>


void CMainDlg::LoadJpgFromResource() {
    // 1. 查找并加载资源

    HMODULE hExe =  GetModuleHandle(NULL);
    HRSRC hRsrc = FindResource(hExe, MAKEINTRESOURCE(IDR_JPG1), RT_JPG);
    if (!hRsrc) return;

    DWORD dwSize = SizeofResource(hExe, hRsrc);
    if (dwSize == 0) return;

    HGLOBAL hGlobal = LoadResource(hExe, hRsrc);
    if (!hGlobal) return;

    LPVOID pData = LockResource(hGlobal);
    if (!pData) return;

    // 2. 将资源数据写入IStream
    IStream* pStream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &pStream);
    if (FAILED(hr)) return;

    ULONG ulWritten = 0;
    pStream->Write(pData, dwSize, &ulWritten);
    LARGE_INTEGER liPos = { 0 };
    pStream->Seek(liPos, STREAM_SEEK_SET, nullptr); // 重置流指针

    // 3. GDI+加载图像
    std::shared_ptr<Image> pImage = std::shared_ptr<  Image>(Image::FromStream(pStream));
    pStream->Release(); // 释放流
    if (!pImage || pImage->GetLastStatus() != Ok) return;

    // 4. 转换为24位RGB格式（无alpha通道）
    UINT width = pImage->GetWidth();
    UINT height = pImage->GetHeight();

    //
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

	m_idTime = ::SetTimer(m_hWnd, TimeID, m_msTime, NULL);

    LoadJpgFromResource();
	return TRUE;
}

//热键事件
LRESULT CMainDlg::OnHotKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {


	return 0;
}


LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CSimpleDialog<IDD_ABOUTBOX, FALSE> dlg;
	dlg.DoModal();
	return 0;
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

LRESULT CMainDlg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == TimeID) {
		//定时器事件
	}
	return 0;
}
