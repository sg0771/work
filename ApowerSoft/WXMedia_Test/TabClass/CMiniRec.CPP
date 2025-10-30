// ApowerSoft Corporation. All Rights Reserved.
// 显示器录制+声音录制+摄像头录制+系统声音录制+麦克风录制+鼠标点击录制+键盘按键录制

#include "stdafx.h"
#include "CMiniRec.h"
#include <shellapi.h>
#include <timeapi.h>
#pragma comment(lib,"winmm")
//判断目录是否存在，不存在则创建
void CLASS_NAME::ProcDir(const wchar_t* wszDir) {

	std::string strDir = WXBase::UTF16ToGB(wszDir);
	if (_access(strDir.c_str(), 0) == -1)
	{
		_mkdir(strDir.c_str());
	}
}


//录屏操作
void CLASS_NAME::Start() {
	DoDataExchange(TRUE);
	if (m_pCapture == nullptr) {
		m_bPause = FALSE;
		TWXCaptureConfig s_param;
		TWXCaptureConfigDefault(&s_param);

		m_strFileName = m_strDir;//输出目录

		time_t t1 = time(NULL);
		tm* t = localtime(&t1);

		CString strType;
		GetDlgItem(IDC_CB_FORMAT).GetWindowText(strType);

		m_strFileName.Format(L"%wsApowerSoft-%04d%02d%02d-%02d%02d%02d.%ws",
			m_strDir,
			t->tm_year + 1900,
			t->tm_mon + 1,
			t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec,
			strType);

		wcscpy(s_param.m_wszFileName, m_strFileName);

		//视频设置
		s_param.m_video.m_bUse = 1;
		int pos = m_cbDisplay.GetCurSel();
		MonitorInfo* info = WXScreenGetInfo(pos);
		wcscpy(s_param.m_video.m_wszDevName, info->wszName);//显示器名字
		s_param.m_video.m_iForceFps = 0;
		s_param.m_mode = MODE_NORMAL;
		s_param.m_video.m_bUseHW = m_bHW;  //硬编码模式
		s_param.m_video.m_bDXGI = m_bDXGI;  //DXGI桌面采集，比GDI效率高

		CString strFps;
		pos = m_cbFPS.GetCurSel();
		m_cbFPS.GetLBText(pos, strFps);

		s_param.m_video.m_iFps = _ttoi(strFps);

		CString strVB;
		pos = m_cbVB.GetCurSel();
		m_cbVB.GetLBText(pos, strVB);
		s_param.m_video.m_iBitrate = _ttoi(strVB) * 1000;

		CString strVC;
		pos = m_cbVC.GetCurSel();
		m_cbVC.GetLBText(pos, strVC);
		wcscpy(s_param.m_video.m_wszCodec, strVC);

		//音频设置
		s_param.m_audio.has_audio = 1;
		CString strSystem = "default";
		int pos1 = m_cbSystem.GetCurSel();
		if (pos1 == 1) {
			strSystem = "conf";
		}
		else if (pos1 == 2) {
			strSystem = "nullptr";
		}
		wcscpy(s_param.m_audio.m_systemName, strSystem);

		CString strMic = "default";
		int pos2 = m_cbMic.GetCurSel();
		if (pos2 == 1) {
			strMic = "conf";
		}
		else if (pos2 == 2) {
			strMic = "nullptr";
		}
		wcscpy(s_param.m_audio.m_micName, strMic);

		if (pos1 == 2 && pos2 == 2)	//	不录制音频
			s_param.m_audio.has_audio = 0;

		s_param.m_audio.bNS = TRUE;
		s_param.m_audio.bAGC = TRUE;
		s_param.m_audio.bVAD = TRUE;

		CString strAC;
		pos = m_cbAC.GetCurSel();
		m_cbAC.GetLBText(pos, strAC);
		wcscpy(s_param.m_audio.codec, strAC);

		s_param.m_audio.nSystemLevel = 100;
		s_param.m_audio.nMicLevel = 100;
		s_param.m_audio.nSampleRate = 48000;

		s_param.m_audio.nChannel = 2;
		s_param.m_audio.nBitarte = 96 * 1000;

		s_param.m_image.m_bUsed = 0;
		//wcscpy(s_param.m_image.m_wszFileName, L"D:\\中文水印.png");

		s_param.m_mouse.m_iUsed = TRUE;//录制鼠标

		s_param.m_mouse.m_bMouseHotdot = FALSE;
		s_param.m_mouse.m_fAlphaHotdot = 1.0;// 0.73579f;//热点透明度

		//鼠标点击动画
		s_param.m_mouse.m_bMouseAnimation = FALSE;
		s_param.m_mouse.m_fAlphaAnimation = 1.0;// 0.73579f;//动画透明度


		s_param.m_text.m_bUsed = FALSE;

		int error = 0;

		// s_param.m_text.m_bUsed = TRUE;
		m_pCapture = WXCaptureStartExt(&s_param, &error);

		if (m_pCapture == nullptr) {
			CString XX;

			if (error & WX_ERROR_OPEN_FILE)XX += L"Create File Failed\r\n";
			if (error & WX_WARNING_NO_VIDEO_DISPLAY_DATA)XX += L"Not Screen Shot Data\r\n";
			if (error & WX_WARNING_NO_VIDEO_CAREMA_DATA)XX += L"Not Camera Shot Data\r\n";
			if (error & WX_WARNING_NO_VIDEO_DATA)XX += L"Not Video Data\r\n";

			if (error & WX_WARNING_NO_IMAGE_WATERMARK_DATA)XX += L"No Image Watermark\r\n";
			if (error & WX_WARNING_NO_TEXT_WATERMARK_DATA)XX += L"No Text Watermark\r\n";

			if (error & WX_WARNING_NO_SOUND_SYSTEM_DATA)XX += L"No System Sound Data\r\n";
			if (error & WX_WARNING_NO_SOUND_MIC_DATA)XX += L"No Micphone Sound Data\r\n";
			if (error & WX_WARNING_NO_SOUND_DATA)XX += L"No Audio Data\r\n";

			if (error & WX_WARNING_VIDEO_RECT)XX += L"/Video locale setting error\r\n";
			if (error & WX_ERROR_VIDEO_NO_DEVICE)XX += L"Specified device name does not exist\r\n";
			if (error & WX_ERROR_VIDEO_NO_PARAM)XX += L"The camera device does not support the format parameters width/height/fps\r\n";
			if (error & WX_ERROR_VIDEO_DEVICE_OPEN)XX += L"Video Device Open Failed\r\n";

			if (error & WX_ERROR_SOUND_SYSTEM_OPEN)XX += L"System Sound Open Failed\r\n";
			if (error & WX_ERROR_SOUND_MIC_OPEN)XX += L"Micphone Sound Open Failed\r\n";
			::MessageBox(NULL, XX, L"MiniRec Error", MB_OK);
		}
		else {
			m_tsStart = ::timeGetTime();
		}
	}
}

void CLASS_NAME::Stop() {
	if (m_pCapture) {
		WXCaptureStopEx(m_pCapture, 0);
		m_pCapture = nullptr;
		m_bPause = FALSE;
		::ShellExecute(NULL, L"explore", m_strDir, NULL, NULL, SW_SHOWDEFAULT);
	}
}

void CLASS_NAME::Pause() {
	if (m_pCapture) {
		WXCapturePause(m_pCapture);
		m_bPause = TRUE;
	}
}

void CLASS_NAME::Resume() {
	if (m_pCapture) {
		WXCapturePause(m_pCapture);
		m_bPause = FALSE;
	}
}

//窗体大小改变
LRESULT CLASS_NAME::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	//RECT rc;
	//::GetClientRect(m_hWnd, &rc);

	return 0;
}

//初始化对话框
LRESULT CLASS_NAME::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);

	m_atomStartAndStop = GlobalAddAtom(L"Mini Rec Start And Stop");   // 启动/结束 热键  Ctrl+Alt+1

	BOOL isKeyRegistered = RegisterHotKey(m_hWnd, m_atomStartAndStop, MOD_ALT, 'C');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)

	m_atomPauseAndResume = GlobalAddAtom(L"Mini Rec Pause And Resume");    //暂停/恢复 热键   Ctrl+Alt+2

	isKeyRegistered = RegisterHotKey(m_hWnd, m_atomStartAndStop, MOD_ALT, 'K');
	assert(isKeyRegistered != FALSE);     //调试时用(当前热键已经被注册时会返回失败)


	m_strDir = WXGetPath();
	m_strDir += L"Videos\\";
	ProcDir(m_strDir);

	m_cbSystem.InsertString(0, L"默认设备");
	m_cbSystem.InsertString(1, L"所有设备");
	m_cbSystem.InsertString(2, L"不录制");
	m_cbSystem.SetCurSel(1);

	m_cbMic.InsertString(0, L"默认设备");
	m_cbMic.InsertString(1, L"所有设备");
	m_cbMic.InsertString(2, L"不录制");
	m_cbMic.SetCurSel(1);

	m_cbVC.InsertString(0, L"H264");
	m_cbVC.InsertString(1, L"H265");
	m_cbVC.InsertString(2, L"MPEG4");
	m_cbVC.InsertString(3, L"MPEG2");
	m_cbVC.InsertString(4, L"VP8");
	m_cbVC.InsertString(5, L"VP9");
	m_cbVC.SetCurSel(0);

	m_cbAC.InsertString(0, L"AAC");
	m_cbAC.InsertString(1, L"MP3");
	m_cbAC.InsertString(2, L"AC3");
	m_cbAC.InsertString(3, L"OPUS");
	m_cbAC.SetCurSel(0);

	m_cbFPS.InsertString(0, L"10");
	m_cbFPS.InsertString(1, L"25");
	m_cbFPS.InsertString(2, L"30");
	m_cbFPS.InsertString(3, L"60");
	m_cbFPS.SetCurSel(1);

	m_cbVB.InsertString(0, L"0");
	m_cbVB.InsertString(1, L"2000");
	m_cbVB.InsertString(2, L"4000");
	m_cbVB.InsertString(3, L"8000");
	m_cbVB.SetCurSel(0);


	m_cbType.InsertString(0, L"MP4");
	m_cbType.InsertString(1, L"MOV");
	m_cbType.InsertString(2, L"TS");
	m_cbType.InsertString(3, L"AVI");
	m_cbType.InsertString(4, L"FLV");
	m_cbType.InsertString(5, L"MKV");
	m_cbType.InsertString(6, L"WEBM");
	m_cbType.InsertString(7, L"ASF");
	m_cbType.InsertString(8, L"WMV");
	m_cbType.InsertString(9, L"MPG");
	m_cbType.InsertString(10, L"VOB");
	m_cbType.InsertString(11, L"avi.xws");
	m_cbType.InsertString(12, L"ts.xws");
	m_cbType.InsertString(13, L"asf.xws");
	m_cbType.InsertString(14, L"wmv.xws");
	m_cbType.InsertString(15, L"mp4.xws");
	m_cbType.InsertString(16, L"flv.xws");
	m_cbType.InsertString(17, L"gif");
	m_cbType.InsertString(18, L"gif.xws");


	m_cbType.InsertString(19, L"aac.xws");
	m_cbType.InsertString(20, L"aac");
	m_cbType.InsertString(21, L"wav.xws");
	m_cbType.InsertString(22, L"wav");
	m_cbType.InsertString(23, L"ogg.xws");
	m_cbType.InsertString(24, L"ogg");
	m_cbType.InsertString(25, L"flac.xws");
	m_cbType.InsertString(26, L"flac");
	m_cbType.InsertString(27, L"wma");	
	m_cbType.InsertString(28, L"mp3");
	m_cbType.InsertString(29, L"mp3.xws");

	m_cbType.SetCurSel(18);

	WXScreenInit();
	int index = -1;
	int count = WXScreenGetCount();
	for (int i = 0; i < count; i++) {
		MonitorInfo* info = WXScreenGetInfo(i);
		CString str = info->wszName;
		if (info->isPrimary) {
			index = i;
			str += L"  (Default Display)";
		}
		CString szSize;
		szSize.Format(L"  (%dx%d)", info->width, info->height);
		str += szSize;
		m_cbDisplay.InsertString(i, str);
	}
	m_cbDisplay.SetCurSel(index);

	if (!WXSupportH264Codec()) {//不支持硬编码
		m_bHW = FALSE;
		GetDlgItem(IDC_CHECK_HW).EnableWindow(FALSE);
	}
	if (!WXSupportDXGI()) {//不支持加速采集
		m_bDXGI = FALSE;
		GetDlgItem(IDC_CHECK_DXGI).EnableWindow(FALSE);
	}
	SetTimer(TIMER_ID, 100, NULL);
	DoDataExchange(FALSE);
	return 0;
}

//水平滚动消息
LRESULT CLASS_NAME::OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	return 0;
}

//定时器消息
LRESULT CLASS_NAME::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if (wParam == TIMER_ID) {
		if (m_pCapture != nullptr) {
			CString StrLog;
			CString str;
			int64_t VideoSize = 0;
			int64_t ptsVideo = WXCaptureGetVideoTime(m_pCapture);
			if (ptsVideo) {

				int hour = (int)((ptsVideo / 1000) / 3600);
				int min = (int)(((ptsVideo / 1000) % 3600) / 60);
				int second = (int)((ptsVideo / 1000) % 60);
				int ms = (int)(ptsVideo % 1000);

				int64_t VideoFrame = WXCaptureGetVideoFrame(m_pCapture);

				double  fps = 0;
				if (ptsVideo != 0)
					fps = (double)(VideoFrame * 1000.0 / ptsVideo);

				VideoSize = WXCaptureGetVideoSize(m_pCapture);
				int  kbps = (int)((double)VideoSize * 8 / ptsVideo);

				str.Format(L"Video %02d:%02d:%02d-%03d ms [%0.2fFPS] [%04d KBPS]\r\n",
					hour, min, second, ms,
					fps, kbps);
				StrLog += str;
			}

			int64_t AudioSize = 0;
			int64_t ptsAudio = WXCaptureGetAudioTime(m_pCapture); //音频时间戳，比较准
			if (ptsAudio) {
				AudioSize = WXCaptureGetAudioSize(m_pCapture);
				int  kbps = (int)((double)AudioSize * 8 / ptsAudio);

				int hour = (int)((ptsAudio / 1000) / 3600);
				int min = (int)(((ptsAudio / 1000) % 3600) / 60);
				int second = (int)((ptsAudio / 1000) % 60);
				int ms = (int)(ptsAudio % 1000);
				str.Format(L"Audio %02d:%02d:%02d-%03d ms [%03dKBPS]\r\n", hour, min, second, ms, kbps);
				StrLog += str;
			}

			int64_t TotalSize1 = WXCaptureGetFileSize(m_pCapture);

			str.Format(L"TotalSize1=%0.3fMb \r\n", (double)TotalSize1 / 1024.0 / 1024.0);
			StrLog += str;

			int64_t ptsDuration = WXCaptureGetDuration();//录制运行时间
			if (ptsDuration) {
				int hour = (int)((ptsDuration / 1000) / 3600);
				int min = (int)(((ptsDuration / 1000) % 3600) / 60);
				int second = (int)((ptsDuration / 1000) % 60);
				int ms = (int)(ptsDuration % 1000);
				str.Format(L"System%02d:%02d:%02d-%03dms\r\n", hour, min, second, ms);
				StrLog += str;
			}

			if (ptsVideo && ptsAudio) {
				int64_t av_delay = abs(ptsVideo - ptsAudio);
				str.Format(L"A-V-Delay=%04dms\r\n", (int)av_delay);
				StrLog += str;
			}
			GetDlgItem(IDC_LOG_REC).SetWindowText(StrLog);
		}
	}
	return 0;
}

//销毁
LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	Stop();
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedStartRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pCapture == nullptr) {
		Start();
		if (m_pCapture) {
			GetDlgItem(IDC_START_REC).SetWindowText(L"结束");
		}
	}
	else {
		Stop();
		m_pCapture = nullptr;
		GetDlgItem(IDC_START_REC).SetWindowText(L"开始");
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedSetDirRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_pCapture)
		return -1;
	TCHAR szBuffer[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = NULL;
	bi.pszDisplayName = szBuffer;
	bi.lpszTitle = _T("从下面选文件夹目录:");
	bi.ulFlags = BIF_RETURNFSANCESTORS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (NULL != idl) {
		SHGetPathFromIDList(idl, szBuffer);
		m_strDir = szBuffer;
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedOpenDirRec(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
		// 打开输出目录
	::ShellExecute(NULL, L"explore", m_strDir, NULL, NULL, SW_SHOWDEFAULT);
	return 0;
}

LRESULT CLASS_NAME::OnCbnSelchangeCbFormat(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)

{
	// TODO: 在此添加控件通知处理程序代码


	DoDataExchange(TRUE);
	m_cbVC.ResetContent();
	m_cbAC.ResetContent();
	m_cbVC.Clear();
	m_cbAC.Clear();

	CString text;
	int selIndex = m_cbType.GetCurSel();
	if (selIndex != CB_ERR)
	{
		m_cbType.GetLBText(selIndex, text);//获得修改后的值
	}

	if (text == L"MP4" ||
		text == L"MOV" ||
		text == L"TS") {
		m_cbVC.InsertString(0, L"H265");
		m_cbVC.InsertString(1, L"H264");
		m_cbAC.InsertString(0, L"AAC");
		m_cbAC.InsertString(1, L"MP3");
	}
	else	if (text == L"AVI" || text == L"FLV") {
		m_cbVC.InsertString(0, L"H264");
		m_cbVC.InsertString(1, L"MPGE4");
		m_cbAC.InsertString(0, L"AAC");
		m_cbAC.InsertString(1, L"MP3");
	}
	else if (text == L"MKV") {
		m_cbVC.InsertString(0, L"VP8");
		m_cbVC.InsertString(1, L"VP9");
		m_cbAC.InsertString(0, L"OPUS");
		m_cbAC.InsertString(1, L"AAC");
		m_cbAC.InsertString(2, L"MP3");
	}
	else if (text == L"WEBM") {
		m_cbVC.InsertString(0, L"VP8");
		m_cbVC.InsertString(1, L"VP9");
		m_cbAC.InsertString(0, L"OPUS");
	}
	else if (text == L"ASF" || text == L"WMV") {
		m_cbVC.InsertString(0, L"WMV2");
		m_cbVC.InsertString(1, L"WMV1");
		m_cbAC.InsertString(0, L"WMA");
	}
	else if (text == L"MPG" || text == L"VOB") {
		m_cbVC.InsertString(0, L"MPGE2");
		m_cbAC.InsertString(0, L"MP2");
	}
	m_cbVC.SetCurSel(0);
	m_cbAC.SetCurSel(0);
	DoDataExchange(FALSE);
	DoDataExchange(TRUE);
	return 0;
}
