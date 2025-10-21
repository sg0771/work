//功能: 将文件内容读取为十六进制字符串

#include "stdafx.h"
#include "CReadToHex.h"


LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	return TRUE;
}

LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	GetDlgItem(IDC_FILE_HEX).MoveWindow(7, 7, 100, 45);
	GetDlgItem(IDC_FILENAME_HEX).MoveWindow(120, 7, rc.right - 130, 45);

	GetDlgItem(IDC_MSG_HEX).MoveWindow(7, 60, rc.right - 14, rc.bottom - 70);

	return 0;
}

const int Max_Num = 3000;
#define Num 4096
LRESULT CLASS_NAME::OnBnClickedFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{

	WXMediaFileEdit2(L"D:\\a.flv", L"D:\\aa.mp4", 0, 10 * 1000);
	return 0;
	
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strName = dlg.m_ofn.lpstrFile;
		int length = WXBase::Filesize((LPCTSTR)m_strName);
		if (length == 0) {
			MessageBox(L"File is empty");
			return 0;
		}
		if (length > 2 * 1024 * 1024) { 
			//主要是转 shader 编码数据，太大没意义
			// 1MB 
			CString str;
			str.Format(L"File is too large, max size is 2 MB");
			MessageBox(str);
			return 0;
		}
	
		m_strHex.Format(L"uint8_t MyData[%d] = {", length);
		//m_strHex = L"uint8_t MyData[] = {";
		std::ifstream fin(m_strName, std::ios::binary);
		int64_t sum = 0;
		if (fin.is_open()) {
			while (true) {
				uint8_t temp[Num];
				fin.read((char*)temp, Num);
				int count = fin.gcount();
				for (size_t i = 0; i < count; i++) {
					CString str;
					sum++;
					if (sum != length) {
						str.Format(L"0x%02x,", temp[i]);
						m_strHex += str;
					}else {
						str.Format(L"0x%02x", temp[i]);
						m_strHex += str;
						break;
					}
				}
				if (sum == length) {
					break;
				}
				m_strHex += L"\r\n";
			}
			m_strHex += L"};";
			fin.close();
		}
		DoDataExchange(FALSE);
	}
	return 0;
}



