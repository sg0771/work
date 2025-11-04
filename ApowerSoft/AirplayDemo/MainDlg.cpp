// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "MainDlg.h"

//------------------------------------------
static int s_iWidth = 0;
static int s_iHeight = 0;
void ReportWindowStatus(const WindowShowStruct &stWindowShow, const uint64_t uniqueid)
{
	s_iWidth = stWindowShow.iScreenW;
	s_iHeight = stWindowShow.iScreenH;

	if (stWindowShow.eWindowStatus == LANDSCAPE)
	{
		//AfxMessageBox(_T("横屏"), MB_OK);
		int i = 0;
	}
	else
	{
		//AfxMessageBox(_T("竖屏"), MB_OK);

		int i = 0;
	}
}

//视频数据回调
void ReportBmpData(const unsigned char* data, int iLen)
{
	int i = 0;
}

WindowPixel GetPixel()
{
	return TYPE_2K;
	//return TYPE_720P;
}

HWND g_handle = 0;
uint64_t g_uniqueid = 0;

uint64_t GetParentWindow(const uint64_t uniqueid) {
	g_uniqueid = uniqueid;
	return (uint64_t)g_handle;
}

int GetMaxDevNum() {
	return 3;
}

int CallBackConnectInfo(const ConnectStatusStruct &stConnectStatus, const uint64_t uniqueid) {
	return 1;
}

int CallBackCheckWindowExist(uint64_t uniqueid) {
	return 1;
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

	const int MAX_BUFFER_LEN = 1000;
	TCHAR  wszDevName[MAX_BUFFER_LEN];
	memset(wszDevName, 0, 100 * sizeof(TCHAR));
	DWORD dwNameLen = MAX_BUFFER_LEN;
	GetComputerName(wszDevName, &dwNameLen);//计算机名字

	TCHAR  wszUserName[MAX_BUFFER_LEN];
	memset(wszUserName, 0, 100 * sizeof(TCHAR));
	DWORD dwUserLen = MAX_BUFFER_LEN;
	GetUserName(wszUserName, &dwUserLen);//计算机名字

	 //start airplay server
	AirplayManagerStruct stAirplay;
	memset(&stAirplay, 0, sizeof(AirplayManagerStruct));
	wcscpy_s(stAirplay.m_arcAppName, MAX_APP_NAME, wszUserName);
	wcscpy_s(stAirplay.m_arcLogName, MAX_LOG_NAME, L"C:\\Z_Disk\\airplay.log");
	wcscpy_s(stAirplay.m_arcPcVersion, MAX_PC_VERSION, L"1480");
	wcscpy_s(stAirplay.m_uid, MAX_UID, L"11223344556677889900");

	stAirplay.m_CallBackWindowStatus = ReportWindowStatus;
	stAirplay.m_CallBackBmpData = ReportBmpData;//视频数据回调。。
	stAirplay.m_CallBackFrameData = NULL;//视频数据回调。
	//stAirplay.m_iPort = 46666;
	//stAirplay.m_iDataPort = 46001;
	//stAirplay.m_iMirrorPort = 46000;
	stAirplay.m_CallBackConnectInfo = CallBackConnectInfo;
	stAirplay.m_CallBackWindowPixel = GetPixel;
	stAirplay.m_CallBackNeedConvertVideoData = NULL;
	stAirplay.m_CallBackGetParentWindow = GetParentWindow;
	stAirplay.m_CallBackGetMaxDevNum = GetMaxDevNum;
	stAirplay.m_CallBackCheckWindowExist = CallBackCheckWindowExist;
	int ret = -1;
	if (ret = StartAirplay(stAirplay) < 0)
	{
		return FALSE;
	}

	SetWindowsParentHandle(m_hWnd);
	g_handle = m_hWnd;

	m_atomFullScreen = GlobalAddAtom(L"FullScreen");   //全屏  ALT+Q
	BOOL isKeyRegistered = RegisterHotKey(m_hWnd, m_atomFullScreen, MOD_ALT, 'Q');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)


	m_atomStartAndStop = GlobalAddAtom(L"Start And Stop");   // 启动/结束 热键  ALT+C
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomStartAndStop, MOD_ALT, 'C');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)


	m_atomHW = GlobalAddAtom(L"Register Hardware");    //设置硬解码   ALT+H
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomHW, MOD_ALT, 'H');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)


	m_atomRotate0 = GlobalAddAtom(L"Mini Rec m_atomRotate0");    //设置旋转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomRotate0, MOD_ALT, '0');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomRotate90 = GlobalAddAtom(L"Mini Rec m_atomRotate90");    //设置旋转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomRotate90, MOD_ALT, '1');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomRotate180 = GlobalAddAtom(L"Mini Rec m_atomRotate180");    //设置旋转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomRotate180, MOD_ALT, '2');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomRotate270 = GlobalAddAtom(L"Mini Rec m_atomRotate270");    //设置旋转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomRotate270, MOD_ALT, '3');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomFlipX = GlobalAddAtom(L"Mini Rec m_atomFlipX");    //设置水平翻转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomFlipX, MOD_ALT, 'X');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomFlipY = GlobalAddAtom(L"Mini Rec m_atomFlipY");    //设置垂直翻转
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomFlipY, MOD_ALT, 'Y');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomFixed = GlobalAddAtom(L"Mini Rec Fixed");    //设置是否填充显示
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomFixed, MOD_ALT, 'F');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomI420 = GlobalAddAtom(L"Mini Rec I420");    //设置是否填充显示
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomI420, MOD_ALT, 'I');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)


	m_atomPNG = GlobalAddAtom(L"Mini Rec PNG");    //设置是否填充显示
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomPNG, MOD_CONTROL, 'P');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomJPG = GlobalAddAtom(L"Mini Rec JPG");    //设置是否填充显示
	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomJPG, MOD_CONTROL, 'J');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	WXAirplaySetShowJ420(m_bLut);
	WXAirplaySetVideoRenderFixed(m_bFixed);
	SetH264DecodeHwMode(m_bHW);


	WXCHAR szFilePath[MAX_PATH + 1] = { 0 };
	::GetModuleFileName(NULL, szFilePath, MAX_PATH);

	WXCHAR filePath[MAX_PATH + 1] = { 0 };
	wcscpy(filePath, szFilePath);

	//获取exe执行文件名字(去掉后缀".exe")
	*wcsrchr(filePath, L'.') = '\0';   // 从最左边开始最后一次出现"."的位置(注：strchr/strrchr函数使用)
	std::wstring ExeName = wcsrchr(filePath, L'\\') + 1;

	(wcsrchr(szFilePath, _T('\\')))[1] = 0;
	m_strPath = szFilePath;

	//m_strPath = WXGetPath();

	m_idTime = ::SetTimer(m_hWnd, 1001, 100, NULL);
//WXAirplaySetBroadcastSize(4096,2160);
	return TRUE;
}

//热键
LRESULT CMainDlg::OnHotKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (g_uniqueid == 0) {
		return 0;
	}
	if (wParam == m_atomStartAndStop) {
		m_bCaptureState == 0 ? Start() : Stop();
	}
	else if (wParam == m_atomFullScreen) {
		m_bFullScreen = !m_bFullScreen;
		if (m_bFullScreen) {
			::GetWindowRect(m_hWnd, &m_rect);
			m_style = ::GetWindowLongPtrW(m_hWnd, GWL_STYLE);
			m_ex_style = ::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);

			HWND hwndDesktop = ::GetDesktopWindow();
			HDC  windowHDC = ::GetDC(hwndDesktop);
			int fullscreenWidth = GetDeviceCaps(windowHDC, HORZRES);
			int fullscreenHeight = GetDeviceCaps(windowHDC, VERTRES);
			int colourBits = GetDeviceCaps(windowHDC, BITSPIXEL);
			int refreshRate = GetDeviceCaps(windowHDC, VREFRESH);
			::ReleaseDC(hwndDesktop, windowHDC);

			DEVMODE fullscreenSettings = { 0 };
			bool isChangeSuccessful = false;
			//RECT windowBoundary;

			EnumDisplaySettings(NULL, 0, &fullscreenSettings);
			fullscreenSettings.dmPelsWidth = fullscreenWidth;
			fullscreenSettings.dmPelsHeight = fullscreenHeight;
			fullscreenSettings.dmBitsPerPel = colourBits;
			fullscreenSettings.dmDisplayFrequency = refreshRate;
			fullscreenSettings.dmFields = DM_PELSWIDTH |
				DM_PELSHEIGHT |
				DM_BITSPERPEL |
				DM_DISPLAYFREQUENCY;

			::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_ex_style & (~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE)));
			::SetWindowLongPtr(m_hWnd, GWL_STYLE, m_style & ~(WS_CAPTION | WS_THICKFRAME));
			::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, fullscreenWidth, fullscreenHeight, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			::ChangeDisplaySettings(&fullscreenSettings, CDS_FULLSCREEN);
			::ShowWindow(m_hWnd, SW_MAXIMIZE);

		}
		else {
			::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_ex_style);
			::SetWindowLongPtr(m_hWnd, GWL_STYLE, m_style);
			::ChangeDisplaySettings(NULL, CDS_RESET);
			::SetWindowPos(m_hWnd, HWND_NOTOPMOST,
				m_rect.left, m_rect.top,
				m_rect.right - m_rect.left, m_rect.bottom - m_rect.top,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			::ShowWindow(m_hWnd, SW_RESTORE);
		}
	}
	
	else if (wParam == m_atomHW) {
		m_bHW = !m_bHW;
		SetH264DecodeHwMode(m_bHW);
	}else if (wParam == m_atomFixed) {
		m_bFixed = !m_bFixed;
		WXAirplaySetVideoRenderFixed(m_bFixed);
	}else if (wParam == m_atomI420) {
		m_bLut = !m_bLut;
		WXAirplaySetShowJ420(m_bLut);
	}else if (wParam == m_atomRotate0) { //强制复位
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_RESET);
	}else if (wParam == m_atomRotate90) { //顺时针旋转90度
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_ROTATE90);
	}else if (wParam == m_atomRotate180) {//顺时针旋转180度
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_ROTATE180);
	}else if (wParam == m_atomRotate270) {//顺时针旋转270度
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_ROTATE270);
	}else if (wParam == m_atomFlipX) { //水平翻转
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_FLIPX);
	}else if (wParam == m_atomFlipY) { //垂直翻转
		WXAirplaySetDisplayRotateFlip(g_uniqueid, ROTATEFLIP_TYPE_FLIPY);
	}else if (wParam == m_atomPNG) {
		time_t t1 = time(NULL);
		tm *t = localtime(&t1);
		CString strTime;
		strTime.Format(L"%ws%04d%02d%02d-%02d%02d%02d.png",(LPCTSTR)m_strPath, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		WXAirplayShotPicture(g_uniqueid, strTime);
	}else if (wParam == m_atomJPG) {
		time_t t1 = time(NULL);
		tm *t = localtime(&t1);
		CString strTime;
		strTime.Format(L"%ws%04d%02d%02d-%02d%02d%02d.jpg", (LPCTSTR)m_strPath, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
		WXAirplayShotPicture(g_uniqueid, strTime);
	}
	return 0;
}

void CMainDlg::Exit() {

	StopAirplay();



	UnregisterHotKey(m_hWnd, m_atomFullScreen);
	GlobalDeleteAtom(m_atomFullScreen);

	UnregisterHotKey(m_hWnd, m_atomStartAndStop);
	GlobalDeleteAtom(m_atomStartAndStop);

	UnregisterHotKey(m_hWnd, m_atomHW);
	GlobalDeleteAtom(m_atomHW);

	UnregisterHotKey(m_hWnd, m_atomRotate0);
	GlobalDeleteAtom(m_atomRotate0);

	UnregisterHotKey(m_hWnd, m_atomRotate90);
	GlobalDeleteAtom(m_atomRotate90);

	UnregisterHotKey(m_hWnd, m_atomRotate180);
	GlobalDeleteAtom(m_atomRotate180);

	UnregisterHotKey(m_hWnd, m_atomRotate270);
	GlobalDeleteAtom(m_atomRotate270);

	UnregisterHotKey(m_hWnd, m_atomFlipX);
	GlobalDeleteAtom(m_atomFlipX);

	UnregisterHotKey(m_hWnd, m_atomFlipY);
	GlobalDeleteAtom(m_atomFlipY);

	UnregisterHotKey(m_hWnd, m_atomFixed);
	GlobalDeleteAtom(m_atomFixed);

	UnregisterHotKey(m_hWnd, m_atomI420);
	GlobalDeleteAtom(m_atomI420);

	UnregisterHotKey(m_hWnd, m_atomPNG);
	GlobalDeleteAtom(m_atomPNG);

	UnregisterHotKey(m_hWnd, m_atomJPG);
	GlobalDeleteAtom(m_atomJPG);
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
	Exit();
	EndDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	Exit();
	EndDialog(wID);
	return 0;
}


LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	Exit();
	return 0;
}

void CMainDlg::Start()
{
	//WXAutoLock al(m_lock);
	//if (m_bCaptureState == 0) { //创建录屏器
	//	time_t t1 = time(NULL);
	//	tm *t = localtime(&t1);
	//	CString strTime;
	//	strTime.Format(L"%ws%04d%02d%02d-%02d%02d%02d.mp4", (LPCTSTR)m_strPath, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	//
	//	m_bCaptureState = DataCaptureStart(strTime, 1280, 720, 30,
	//		3, 0, MODE_BEST, 1);
	//	if (m_bCaptureState) {
	//		//m_tsStartCapture = WXGetTimeMs();
	//		SetRecording(1, g_uniqueid);//  Airplay 录制标记
	//		SetWindowText(L"开始录制");
	//	}
	//}
}

void CMainDlg::Stop()
{
	//WXAutoLock al(m_lock);
	//if (m_bCaptureState) {
	//	SetRecording(0, g_uniqueid);//
	//	DataCaptureStop();
	//	m_bCaptureState = 0;
	//	m_tsStartCapture = 0;
	//	SetWindowText(L"停止录制");
	//}
}

LRESULT CMainDlg::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (wParam == 1001) {
		//if (m_bCaptureState) {
		//	int64_t ts = WXGetTimeMs() - m_tsStartCapture;
		//	if (ts >= 1000 * 30) { // 30s
		//		Stop();
		//	}
		//}
	}
	return 0;
}
