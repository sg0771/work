// 运行ffmpeg.exe 等其它exe
//并且将输出结果显示在界面上

#include "stdafx.h"

#include "CMedialib.h"



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

	//计算子控件在父控件上的RECT
	CWindow childWindow = GetDlgItem(IDC_LIST1);  // 获取子控件的句柄
	RECT rectChild;
	childWindow.GetWindowRect(&rectChild);
	CWindow parentWindow = childWindow.GetParent(); // 获取父控件的句柄
	parentWindow.ScreenToClient(&rectChild);

	DWORD flag = WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL;
	HWND bSuccess = m_listView.Create(m_hWnd, rectChild, NULL, flag, 0, IDC_LIST1);

	// 添加列
	m_listView.InsertColumn(0, _T("名字"), LVCFMT_LEFT, 400);
	m_listView.InsertColumn(1, _T("分辨率"), LVCFMT_LEFT, 200);

	m_s1.SetRange(0, 100);
	m_s1.SetPos(100);

	Initialize("Apowersoft", "WXTest+++", 0);//库的初始化
	TimelineSetting(m_iFps, m_iWidth, m_iHeight, 44100, 2, 0xFFFFFF, "", false);

	DoDataExchange(FALSE);
	return 0;
}

//水平滚动消息
LRESULT CLASS_NAME::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	//if ((::GetDlgCtrlID((HWND)lParam)) == IDC_SLIDER_PLAY && m_player != nullptr)
	//{
	//	int nPos = m_s1.GetPos();

	//	int64_t seekPos = (m_duration * nPos) / 100; //计算要跳转到的时间位置 MS
	//	// 使用控件变量更新滑块位置
	//	//m_slider.SetPos(nPos);
	//	WXFfplaySeek(m_player, seekPos);//播放跳转

	//	return 0; // 已处理消息
	//}

	// 不是目标滑块控件的消息，交给默认处理
	bHandled = FALSE;

	return 0;
}

//定时器消息
LRESULT CLASS_NAME::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (wParam == TIMER_ID) {
		if (m_play) {
			//播放对象
		}
		else if(m_bInConv){
			//正在转换

		}
	}
	return 0;
}

//销毁
LRESULT CLASS_NAME::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {

	return 0;
}

//添加视频文件
LRESULT CLASS_NAME::OnBnClickedAdd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bInConv || m_play) {
		return 0;
	}
	DoDataExchange(TRUE);

	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		CString strName = dlg.m_ofn.lpstrFile;
		std::string str = WXBase::UTF16ToUTF8((LPCWSTR)strName);
		MediaInfomation* dst = AnalyzeMedia((char*)str.c_str());
		if (dst) {
			int count = m_listView.GetItemCount();
			int width = dst->width;
			int height = dst->height;
			CString ss;
			ss.Format(L"%dx%d", width, height);
			m_listView.InsertItem(count + 1, strName);
			m_listView.SetItemText(count, 1, ss);

			//获取第一个dst为Size
			{
				TCHAR szText[MAX_PATH];
				LVITEM lvItem;
				lvItem.mask = LVIF_TEXT; // 指定要获取文本
				lvItem.iItem = 0; // 行索引
				lvItem.iSubItem = 0; // 列索引
				lvItem.pszText = szText; // 缓冲区用于接收文本
				lvItem.cchTextMax = 256; // 缓冲区大小
				m_listView.GetItem(&lvItem); // 获取项文本
				//std::string str_u8 = WXBase::UTF16ToUTF8(szText);
				//MediaInfomation* mi = AnalyzeMedia((char*)str_u8.c_str());
				//m_iWidth = mi->width;
				//m_iHeight = mi->height;
				DoDataExchange(FALSE);
			}
		}
	}
	return 0;
}

//删除视频文件
LRESULT CLASS_NAME::OnBnClickedDel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_bInConv || m_play) {
		return 0;
	}
	// TODO: 在此添加控件通知处理程序代码
	int selectedIndex = m_listView.GetSelectedIndex();
	if (selectedIndex != -1)  // 检查是否有选中条目
	{
		m_listView.DeleteItem(selectedIndex);  // 删除选中项
		// 此处可插入其他处理逻辑，如更新状态栏或显示信息
	}
	return 0;
}


// 启动关闭播放
LRESULT CLASS_NAME::OnBnClickedPlayStartStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_bInConv) {
		return 0;
	}
	if (m_play == nullptr) {
		DoDataExchange(TRUE);

		//ML_SetValue(L"uselog", m_bUselog);//更多日志信息
		ML_SetValue(L"HwVideoDecode", m_bHW_Dec);//H264 硬解码优化
		ML_SetValue(L"HwVideoEncode", m_bHW_Enc);//QSV 硬编码优化

	
		int count = m_listView.GetItemCount();
		if (count == 0)
			return 0;

		float total_time = 0.0;
		std::vector<VideoContext>arrVC;
		for (size_t index = 0; index < count; index++)
		{
			TCHAR szText[MAX_PATH];
			LVITEM lvItem;
			lvItem.mask = LVIF_TEXT; // 指定要获取文本
			lvItem.iItem = index; // 行索引
			lvItem.iSubItem = 0; // 列索引
			lvItem.pszText = szText; // 缓冲区用于接收文本
			lvItem.cchTextMax = 256; // 缓冲区大小
			m_listView.GetItem(&lvItem); // 获取项文本
			std::string str_u8 = WXBase::UTF16ToUTF8(szText);
			auto mi = AnalyzeMedia((char*)str_u8.c_str());
			//if (index == 0) {
			//	m_iWidth = mi->width;
			//	m_iHeight = mi->height;
			//}
			VideoContext vc;
			strcpy(vc.header.path, str_u8.c_str());
			vc.header.start = 0;

			//if (m_b10) {
			//	vc.header.end = min(30.0f, mi->duration / 1000000.0f);
			//}
			//else 
			{
				vc.header.end = mi->duration / 1000000.0;
			}
			total_time += vc.header.end;//
			vc.header.audioenhance = true;
			//vc.header.info = mi;
			{ //居中到画布
				int dx = 0;
				int dy = 0;
				GetXY(mi->width, mi->height, m_iWidth, m_iHeight, dx, dy);
				vc.rect.left = (float)dx / (float)m_iWidth;
				vc.rect.right = 1.0 - vc.rect.left;
				vc.rect.top = (float)dy / (float)m_iHeight;
				vc.rect.bottom = 1.0 - vc.rect.top;
			}
			arrVC.push_back(vc);
		}
		TimelineSetting(m_iFps, m_iWidth, m_iHeight, 44100, 2, 0xFFFFFF, "", false);
		SetVideoTrack(arrVC.data(), arrVC.size());//填充视频轨道

		//波形图
		//SubVideoContext vcWave;
		//vcWave.header.type = WX_Image;
		//strcpy(vcWave.header.path, "WaveForm");
		//vcWave.header.start = 0;
		//vcWave.header.end = total_time;
		//vcWave.rect.left = 0.1;
		//vcWave.rect.right = 0.9;
		//vcWave.rect.top = 0.7;
		//vcWave.rect.bottom = 0.9;
		//SetSubVideoTrack(&vcWave, 1);//填充视频轨道

		m_play = OpenMedia("custom", FALSE, FALSE, 0.0);
		if (m_play) {
			SetRenderHwnd(m_play, GetDlgItem(IDC_PIC_MEDIALIB).m_hWnd);	
			m_bPause = FALSE;
			Resume(m_play);//恢复播放
		}
	}
	else {
		Pause(m_play);//暂停播放
		CloseMedia(m_play);//结束播放
		m_play = nullptr;

	}
	return 0;
}

//暂停，恢复
LRESULT CLASS_NAME::OnBnClickedPlayPauseResume(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_bInConv) {
		return 0;
	}
	if (m_play) {
		m_bPause = !m_bPause;
		if (m_bPause) {
			Pause(m_play);//恢复播放
		}else {
			Resume(m_play);//恢复播放
		}
	}
	return 0;
}


//转换任务
void CLASS_NAME::funcConv() {


	//int count = m_listView.GetItemCount();

	////转换命令
	//if (count > 0) {
	//	float total_time = 0.0;
	//	std::vector<VideoContext>arrVC;
	//	for (size_t index = 0; index < count; index++)
	//	{
	//		TCHAR szText[MAX_PATH];
	//		LVITEM lvItem;
	//		lvItem.mask = LVIF_TEXT; // 指定要获取文本
	//		lvItem.iItem = index; // 行索引
	//		lvItem.iSubItem = 0; // 列索引
	//		lvItem.pszText = szText; // 缓冲区用于接收文本
	//		lvItem.cchTextMax = 256; // 缓冲区大小
	//		m_listView.GetItem(&lvItem); // 获取项文本
	//		std::string str_u8 = WXBase::UTF16ToUTF8(szText);
	//		auto mi = AnalyzeMedia((char*)str_u8.c_str());
	//		//if (index == 0) {
	//		//	m_iWidth  = mi->width;
	//		//	m_iHeight = mi->height;
	//		//}
	//		VideoContext vc;
	//		strcpy(vc.header.path, str_u8.c_str());
	//		vc.header.start = 0;

	//		if (m_b10) {
	//			vc.header.end = min(30.0f, mi->duration / 1000000.0f);
	//		}
	//		else {
	//			vc.header.end = mi->duration / 1000000.0;
	//		}
	//		total_time += vc.header.end;
	//		vc.header.audioenhance = true;
	//		//vc.header.info = mi;
	//		{ //居中到画布
	//			int dx = 0;
	//			int dy = 0;
	//			GetXY(mi->width, mi->height, m_iWidth, m_iHeight, dx, dy);
	//			vc.rect.left = (float)dx / (float)m_iWidth;
	//			vc.rect.right = 1.0 - vc.rect.left;
	//			vc.rect.top = (float)dy / (float)m_iHeight;
	//			vc.rect.bottom = 1.0 - vc.rect.top;
	//		}
	//		arrVC.push_back(vc);
	//	}
	//	TimelineSetting(m_iFps, m_iWidth, m_iHeight, 44100, 2, 0xFFFFFF, "", true);
	//	SetVideoTrack(arrVC.data(), arrVC.size());//填充视频轨道
	//	//波形图
	//	//SubVideoContext vcWave;
	//	//vcWave.header.type = WX_Image;
	//	//strcpy(vcWave.header.path, "WaveForm");
	//	//vcWave.header.start = 0;
	//	//vcWave.header.end = total_time;
	//	//vcWave.rect.left = 0.1;
	//	//vcWave.rect.right = 0.9;
	//	//vcWave.rect.top = 0.7;
	//	//vcWave.rect.bottom = 0.9;
	//	//SetSubVideoTrack(&vcWave, 1);//填充视频轨道

	//	m_bInConv = true;
	//	std::thread([this] {
	//		CString cmds;
	//		if (m_bH264) { //H264 QSV 硬编码
	//			cmds.Format(L"ffmpeg.exe|-i|custom|-vcodec|libx264|-preset|veryfast|-profile:v|high|-level|4.0|-crf|18|-threads|0|-pix_fmt|yuv420p|-r|25|-acodec|aac|-strict|-2|-b:a|320000|-ar|44100|-channel_layout|stereo|%ws", (LPCTSTR)m_StrFileName);
	//		}
	//		else {
	//			cmds.Format(L"ffmpeg.exe|-i|custom|-vcodec|libx265|-preset|ultrafast|-x265-params|crf=18|-threads|0|-pix_fmt|yuv420p|-r|25|-acodec|aac|-strict|-2|-b:a|320000|-ar|44100|-channel_layout|stereo|%ws", (LPCTSTR)m_StrFileName);
	//		}
	//		std::string u8 = WXBase::UTF16ToUTF8((LPCWSTR)cmds);
	//		FFMpegCommand((char*)u8.c_str());
	//		m_bInConv = false;
	//	}).detach();
	//}
	////转换结束
}


LRESULT CLASS_NAME::OnBnClickedExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play) {
		return 0;
	}
	DoDataExchange(TRUE);
	ML_SetValue(L"HwVideoDecode", m_bHW_Dec);//H264 硬解码优化
	ML_SetValue(L"HwVideoEncode", m_bHW_Enc);//QSV 硬编码优化
	if (m_threadTask == nullptr) {
		m_threadTask = new std::thread(&CLASS_NAME::funcConv, this);
	}
	return 0;
}

LRESULT CLASS_NAME::OnBnClickedExportBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (m_play) {
		return 0;
	}
	if(m_bInConv)
		SetConvertState(2);//Stop
	return 0;
}


//选择目录
LRESULT CLASS_NAME::OnBnClickedExportDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR szBuffer[MAX_PATH] = { 0 };
	BROWSEINFO bi;
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = szBuffer;
	bi.lpszTitle = _T("从下面选文件夹目录:");
	bi.ulFlags = BIF_RETURNFSANCESTORS;
	LPITEMIDLIST idl = SHBrowseForFolder(&bi);
	if (NULL != idl) {
		SHGetPathFromIDList(idl, szBuffer);
		m_strDIR = szBuffer;
		DoDataExchange(FALSE);
	}
	return 0;
}
