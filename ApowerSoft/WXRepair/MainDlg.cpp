// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

// 辅助函数：获取当前EXE的版本号
static  BOOL GetExeVersion(CString& strVersion)
{
	TCHAR szExePath[MAX_PATH] = { 0 };
	if (!GetModuleFileName(NULL, szExePath, MAX_PATH))
		return FALSE;

	DWORD dwHandle = 0;
	DWORD dwVersionInfoSize = GetFileVersionInfoSize(szExePath, &dwHandle);
	if (dwVersionInfoSize == 0)
		return FALSE;

	LPBYTE pVersionInfo = new BYTE[dwVersionInfoSize];
	if (!GetFileVersionInfo(szExePath, dwHandle, dwVersionInfoSize, pVersionInfo))
	{
		delete[] pVersionInfo;
		return FALSE;
	}

	VS_FIXEDFILEINFO* pFixedInfo = NULL;
	UINT uInfoSize = 0;
	if (!VerQueryValue(pVersionInfo, _T("\\"), (LPVOID*)&pFixedInfo, &uInfoSize) || uInfoSize == 0)
	{
		delete[] pVersionInfo;
		return FALSE;
	}

	// 格式化版本号（主版本.次版本.修订号.构建号）
	strVersion.Format(_T("%d.%d.%d.%d"),
		HIWORD(pFixedInfo->dwFileVersionMS),
		LOWORD(pFixedInfo->dwFileVersionMS),
		HIWORD(pFixedInfo->dwFileVersionLS),
		LOWORD(pFixedInfo->dwFileVersionLS));

	delete[] pVersionInfo;
	return TRUE;
}


// 辅助函数：写入版本号和当前日期到注册表
static void WriteRegValues(HKEY hKey, LPCTSTR pszVersionKey, LPCTSTR pszDateKey, const CString& strVersion)
{
	// 写入版本号
	RegSetValueEx(hKey, pszVersionKey, 0, REG_SZ,
		(const BYTE*)(LPCTSTR)strVersion,
		(strVersion.GetLength() + 1) * sizeof(TCHAR));

	// 获取当前日期并格式化为"YYYY-MM-DD"
	SYSTEMTIME st;
	GetLocalTime(&st); // 获取本地时间

	TCHAR szCurrentDate[20] = { 0 };
	_stprintf_s(szCurrentDate, _countof(szCurrentDate),
		_T("%04d-%02d-%02d"),
		st.wYear, st.wMonth, st.wDay);

	// 写入当前日期
	RegSetValueEx(hKey, pszDateKey, 0, REG_SZ,
		(const BYTE*)szCurrentDate,
		(lstrlen(szCurrentDate) + 1) * sizeof(TCHAR));
}


// 辅助函数：计算保存的日期与当前日期的差值（返回天数，-1表示格式错误）
static int CalculateDateDiff(LPCTSTR pszSavedDate)
{
	// 解析保存的日期（格式：YYYY-MM-DD）
	int nYear, nMonth, nDay;
	if (_stscanf_s(pszSavedDate, _T("%d-%d-%d"), &nYear, &nMonth, &nDay) != 3)
		return -1;

	// 转换为FILETIME（用于计算差值）
	SYSTEMTIME stSaved = { 0 };
	stSaved.wYear = nYear;
	stSaved.wMonth = nMonth;
	stSaved.wDay = nDay;
	stSaved.wHour = 0;
	stSaved.wMinute = 0;
	stSaved.wSecond = 0;

	FILETIME ftSaved, ftNow;
	if (!SystemTimeToFileTime(&stSaved, &ftSaved))
		return -1;

	// 获取当前日期的FILETIME
	SYSTEMTIME stNow;
	GetLocalTime(&stNow);
	stNow.wHour = 0; // 忽略时间部分，只比较日期
	stNow.wMinute = 0;
	stNow.wSecond = 0;
	if (!SystemTimeToFileTime(&stNow, &ftNow))
		return -1;

	// 计算差值（FILETIME单位为100纳秒，转换为天数）
	ULARGE_INTEGER ulSaved, ulNow;
	ulSaved.LowPart = ftSaved.dwLowDateTime;
	ulSaved.HighPart = ftSaved.dwHighDateTime;
	ulNow.LowPart = ftNow.dwLowDateTime;
	ulNow.HighPart = ftNow.dwHighDateTime;

	// 差值（1天 = 86400秒 = 86400 * 10^7 纳秒）
	__int64 llDiff = (ulNow.QuadPart - ulSaved.QuadPart) / 864000000000LL;
	return (int)llDiff;
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
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	m_prog.SetRange(0, 100);
	m_prog.SetPos(0);

	// 注册表路径（替换为你的实际路径）
	LPCTSTR pszRegPath = _T("Software\\ApowerSoft\\WXRepair");
	LPCTSTR pszVersionKey = _T("CurrentVersion");  // 版本号键名
	LPCTSTR pszDateKey = _T("InstallDate");        // 安装日期键名

	// 1. 获取当前EXE的版本号
	CString strCurrentVersion;
	if (!GetExeVersion(strCurrentVersion))
	{
		::MessageBox(NULL, _T("获取程序版本号失败！"), _T("错误"), MB_ICONERROR);
		return FALSE;
	}

	// 2. 打开/创建注册表项
	HKEY hKey = NULL;
	LONG lResult = RegCreateKeyEx(
		HKEY_CURRENT_USER,
		pszRegPath,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE,
		NULL,
		&hKey,
		NULL
	);
	if (lResult != ERROR_SUCCESS)
	{
		::MessageBox(NULL, _T("打开注册表项失败！"), _T("错误"), MB_ICONERROR);
		return FALSE;
	}

	// 3. 读取已保存的版本号
	TCHAR szSavedVersion[256] = { 0 };
	DWORD dwType = REG_SZ;
	DWORD dwSize = sizeof(szSavedVersion);
	lResult = RegQueryValueEx(hKey, pszVersionKey, NULL, &dwType, (LPBYTE)szSavedVersion, &dwSize);

	if (lResult != ERROR_SUCCESS)
	{
		// 3.1 版本号不存在（首次运行）：写入版本号和当前日期
		WriteRegValues(hKey, pszVersionKey, pszDateKey, strCurrentVersion);
		::MessageBox(NULL, _T("首次运行，已记录版本信息和安装日期"), _T("提示"), MB_ICONINFORMATION);
	}
	else
	{
		// 3.2 比较版本号
		CString strSavedVersion = szSavedVersion;
		if (strSavedVersion.Compare(strCurrentVersion) != 0)
		{
			// 版本号不一致（程序更新）：更新版本号和当前日期
			WriteRegValues(hKey, pszVersionKey, pszDateKey, strCurrentVersion);
			::MessageBox(NULL, _T("程序已更新，已重置有效期"), _T("提示"), MB_ICONINFORMATION);
		}
		else
		{
			// 3.3 版本号一致：检查安装日期是否超过30天
			TCHAR szSavedDate[256] = { 0 };
			dwSize = sizeof(szSavedDate);
			lResult = RegQueryValueEx(hKey, pszDateKey, NULL, &dwType, (LPBYTE)szSavedDate, &dwSize);

			if (lResult == ERROR_SUCCESS)
			{
				// 计算与当前日期的差值（天数）
				int nDays = CalculateDateDiff(szSavedDate);
				if (nDays == -1)
				{
					::MessageBox(NULL, _T("日期格式错误！"), _T("错误"), MB_ICONERROR);
				}
				else if (nDays > 30)
				{
					::MessageBox(NULL, _T("程序使用已超过30天，请联系管理员！"), _T("提示"), MB_ICONWARNING);
					RegCloseKey(hKey);
					exit(-1);
					return FALSE; // 强制退出
				}
				else
				{
					// 显示剩余天数（可选）
					CString strMsg;
					strMsg.Format(_T("当前版本：%s\n已使用：%d天\n剩余：%d天"),
						strCurrentVersion, nDays, 30 - nDays);
					::MessageBox(NULL, strMsg, _T("提示"), MB_ICONINFORMATION);
				}
			}
			else
			{
				::MessageBox(NULL, _T("读取安装日期失败！"), _T("错误"), MB_ICONERROR);
			}
		}
	}


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
		int pRate = WXMediaFileEditGetRate();
		if (pRate == 100) {
			//转换完成
			//结束线程
		}
		m_prog.SetPos(pRate);
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedInputRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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

LRESULT CMainDlg::OnBnClickedOutputRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// 设置输出路径
	CFileDialog dlg(FALSE);
	if (dlg.DoModal() == IDOK) {
		m_strOutput = dlg.m_ofn.lpstrFile;
		DoDataExchange(FALSE);
	}
	return 0;
}

LRESULT CMainDlg::OnBnClickedRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
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
			}
			else if (ret == 0) {
				::MessageBox(NULL, L"解析输入文件失败", L"Error", MB_OK);
			}
			else {
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

LRESULT CMainDlg::OnBnClickedBreakRep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WXMediaFileEditBreak();//中断运行
	return 0;
}
