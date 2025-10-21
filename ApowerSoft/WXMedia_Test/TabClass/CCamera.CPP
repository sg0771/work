//功能: 将文件内容读取为十六进制字符串

#include "stdafx.h"
#include "CCamera.h"

LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);

	//GetDlgItem(IDC_CAMERA).MoveWindow(7, 7, 100, 45);
	//GetDlgItem(IDC_CB_NAME_CAMERA).MoveWindow(120, 17, 150, 45);
	//GetDlgItem(IDC_CB_DECS_CAMERA).MoveWindow(280, 17, 150, 45);
	//GetDlgItem(IDC_VB_CAMERA).MoveWindow(440, 7, 100, 45);
	//GetDlgItem(IDC_SETTING_CAMERA).MoveWindow(550, 7, 100, 45);

	GetDlgItem(IDC_PIC_CAMERA).MoveWindow(7, 60, rc.right - 14, rc.bottom - 70);
	return 0;
}

LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);

	WXCameraInit();
	int count = WXCameraGetCount();
	if (count > 0) {
		m_cbCameraDevice.ResetContent();
		for (int i = 0; i < count; i++) {
			CameraInfo* info = WXCameraGetInfo(i);
			CString stName = info->m_strName;
			m_cbCameraDevice.InsertString(i, stName);
		}
		m_cbCameraDevice.SetCurSel(0);
		m_cbCameraDevice.EnableWindow(TRUE);
		m_arrFmt.ResetContent();
		CameraInfo* info = WXCameraGetInfo(0);
		if (info) {
			int index = 0;
			int max_w = 0;
			for (int i = 0; i < info->size_fmt; i++) {
				CString str;
				str.Format(L"%dx%d %dfps", info->m_arrFmt[i].width, info->m_arrFmt[i].height, info->m_arrFmt[i].fps);
				m_arrFmt.InsertString(i, str);
				if (info->m_arrFmt[i].width > max_w) {
					max_w = info->m_arrFmt[i].width;
					index = i;
				}
			}
			m_arrFmt.SetCurSel(index);
		}
	}
	DoDataExchange(FALSE);
	return TRUE;
}

LRESULT CLASS_NAME::OnBnClickedCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	DoDataExchange(TRUE);
	if (m_pCameraPreview == nullptr) {
		int index1 = m_cbCameraDevice.GetCurSel();
		CameraInfo* info = WXCameraGetInfo(index1);
		if (info) {
			if (info->size_fmt > 0) {
				HWND hwnd = GetDlgItem(IDC_PIC_CAMERA).m_hWnd;
				int index2 = m_arrFmt.GetCurSel();
				int iWidth = info->m_arrFmt[index2].width;
				int iHeight = info->m_arrFmt[index2].height;
				int iFps = info->m_arrFmt[index2].fps;

				m_pCameraPreview = WXCameraOpenWithHwnd(info->m_strGuid,iWidth,iHeight,iFps,hwnd,TRUE);
				if (m_pCameraPreview) {
					GetDlgItem(IDC_CAMERA).SetWindowText(L"关闭");
				}
			}
		}
	}
	else {
		WXCameraClose(m_pCameraPreview);
		m_pCameraPreview = nullptr;
		GetDlgItem(IDC_CAMERA).SetWindowText(L"打开");
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedVbCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedSettingCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码

	return 0;
}

LRESULT CLASS_NAME::OnCbnSelchangeCbNameCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// 切换摄像头设备
	int nIndex = m_cbCameraDevice.GetCurSel();
	CameraInfo* info = WXCameraGetInfo(nIndex);
	if (info) {
		int index = 0;
		int max_w = 0;
		for (int i = 0; i < info->size_fmt; i++) {
			CString str;
			str.Format(L"%dx%d %dfps", info->m_arrFmt[i].width, info->m_arrFmt[i].height, info->m_arrFmt[i].fps);
			m_arrFmt.InsertString(i, str);
			if (info->m_arrFmt[i].width > max_w) {
				max_w = info->m_arrFmt[i].width;
				index = i;
			}
		}
		m_arrFmt.SetCurSel(index);
	}
	return 0;
}
