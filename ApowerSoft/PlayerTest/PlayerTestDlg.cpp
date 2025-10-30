
// PlayerTestDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "PlayerTest.h"
#include "PlayerTestDlg.h"
#include "afxdialogex.h"
#include <timeapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPlayerTestDlg 对话框

EXTERN_C void onFrame(void* ctx,int nID, struct AVFrame* frame) {
	CPlayerTestDlg* pThis = (CPlayerTestDlg*)ctx;
	pThis->OnFrame(nID, frame);
}

void CPlayerTestDlg::OnFrame(int index, struct AVFrame* pFrame) {
	if (index < 0 || pFrame == nullptr || pFrame->width < 0)
		return;
	if (m_vRender[index] == nullptr) {
		m_vRender[index] = WXVideoRenderCreateEx(m_hwnd[index], pFrame->width, pFrame->height);
	}
	if (m_vRender[index] != nullptr) {
		WXVideoRenderDisplay(m_vRender[index], pFrame,1,0);
	}
}

CPlayerTestDlg::CPlayerTestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PLAYERTEST_DIALOG, pParent)
	, m_nDelay(10)
	, m_bLAV(TRUE)
	, m_bHW(TRUE)
	, m_bRGB_Output(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPlayerTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER1, m_slider);
	DDX_Control(pDX, IDC_LIST2, m_list[0]);
	DDX_Text(pDX, IDC_EDIT1, m_nDelay);
	DDX_Control(pDX, IDC_LIST3, m_list[1]);
	DDX_Check(pDX, IDC_CHECK1, m_bLAV);
	DDX_Check(pDX, IDC_CHECK2, m_bHW);
	DDX_Check(pDX, IDC_CHECK3, m_bRGB_Output);
}

BEGIN_MESSAGE_MAP(CPlayerTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()	
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SLIDER1, &CPlayerTestDlg::OnNMReleasedcaptureSlider1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_Play, &CPlayerTestDlg::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_PAUSE, &CPlayerTestDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_Add, &CPlayerTestDlg::OnBnClickedAdd0)
	ON_BN_CLICKED(IDC_REMOVE, &CPlayerTestDlg::OnBnClickedRemove0)
	ON_BN_CLICKED(IDC_Add2, &CPlayerTestDlg::OnBnClickedAdd1)
	ON_BN_CLICKED(IDC_REMOVE2, &CPlayerTestDlg::OnBnClickedRemove1)
END_MESSAGE_MAP()


// CPlayerTestDlg 消息处理程序

BOOL CPlayerTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标


	WXDeviceInit(L"Player.txt");//库初始化，参数为日志文件名

	WXSetGlobalValue(L"MaxHeight", 720);//设置输出最大高度，一般建议720P 或者 480P

	// 程序初始化调用

	//WXSetGlobalValue(L"MediaPlayer", MEDIAPLAYER_FFPLAY); // 强制使用FFPLAY播放器
	// 
	// TODO: 在此添加额外的初始化代码
		// 设置列表控件风格为报表视图

	for (size_t i = 0; i < CHANNEL; i++)
	{
		m_list[i].ModifyStyle(0, LVS_REPORT | LVS_SINGLESEL);

		// 设置扩展风格（网格线和整行选择）
		m_list[i].SetExtendedStyle(m_list[0].GetExtendedStyle() |
			LVS_EX_GRIDLINES |
			LVS_EX_FULLROWSELECT);

		// 添加列
		m_list[i].InsertColumn(0, _T("文件名"), LVCFMT_LEFT, 300);
		m_list[i].InsertColumn(1, _T("时长"), LVCFMT_RIGHT, 80);
		m_list[i].InsertColumn(2, _T("开始"), LVCFMT_RIGHT, 80);
		m_list[i].InsertColumn(3, _T("结束"), LVCFMT_RIGHT, 80);
	}


	m_slider.SetRange(0, 100); // 设置滑块范围
	m_slider.SetPos(0); // 设置初始位置为 0

	m_hwnd[0] = GetDlgItem(IDC_PIC)->GetSafeHwnd();
	m_hwnd[1] = GetDlgItem(IDC_PIC2)->GetSafeHwnd();
	m_bRunThread = true;
	m_threadPlay[0] = new std::thread(&CPlayerTestDlg::PlayThread, this, 0);
	m_threadPlay[1] = new std::thread(&CPlayerTestDlg::PlayThread, this, 1);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPlayerTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPlayerTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPlayerTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPlayerTestDlg::Add(int index) {
	UpdateData(TRUE); // 更新控件数据
	if (m_bRunning[index]) {
		AfxMessageBox(_T("请先停止播放，再添加文件！"), MB_ICONINFORMATION);
		return;
	}
	// 打开文件选择对话框
	CFileDialog dlg(TRUE, _T("mp4"), NULL,
		OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
		_T("视频文件 (*.mp4;*.avi;*.mkv;*.mov;*.flv;*.wmv;*.asf;*.ts)|*.mp4;*.avi;*.mkv;*.mov;*.flv;*.wmv;*.asf;*.ts|所有文件 (*.*)|*.*||"),
		this);

	// 允许选择多个文件
	CString strFile;
	dlg.GetOFN().lpstrFile = strFile.GetBuffer(1024 * 10);
	dlg.GetOFN().nMaxFile = 1024 * 10;
	strFile.ReleaseBuffer();

	BOOL bUpdate = FALSE;
	// 如果用户选择了文件
	if (dlg.DoModal() == IDOK)
	{
		POSITION pos = dlg.GetStartPosition();
		while (pos != NULL)
		{
			CString strPath = dlg.GetNextPathName(pos);  // 获取文件完整路径

			// 获取文件名
			CString strFileName = PathFindFileName(strPath);

			int error = 0;
			void* pInfo = WXMediaInfoCreateFast(strPath.GetString(), &error);
			if (pInfo) {

				// 获取时长
				float duration = WXMediaInfoGetFileDuration(pInfo) / 1000.0f;
				if (duration > 0.5f) {
					// 添加到列表
					int nItem = m_list[index].InsertItem(m_list[index].GetItemCount(), strPath);//文件名
					CString strDuration;
					strDuration.Format(_T("%.2f秒"), duration); // 格式化时长为秒
					m_list[index].SetItemText(nItem, 1, strDuration);
					bUpdate = TRUE; // 标记需要更新列表
				}
			}
		}
	}

	if (bUpdate)
		UpdateList(index);
}
void CPlayerTestDlg::Remove(int index) {
	UpdateData(TRUE); // 更新控件数据
	// TODO: 在此添加控件通知处理程序代码
	if (m_bRunning[index]) {
		AfxMessageBox(_T("请先停止播放，再删除文件！"), MB_ICONINFORMATION);
		return;
	}
	// 获取选中的项
	int nSelected = m_list[index].GetSelectionMark();

	bool bUpdate = FALSE; // 是否需要更新列表
	if (nSelected != -1)  // 有选中项
	{
		// 确认删除
		if (AfxMessageBox(_T("确定要删除选中的文件吗？"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			m_list[index].DeleteItem(nSelected);

			// 如果删除后还有项，选中下一项
			int nCount = m_list[index].GetItemCount();
			if (nCount > 0)
			{
				int nNewSel = (nSelected < nCount) ? nSelected : nCount - 1;
				m_list[index].SetSelectionMark(nNewSel);
				m_list[index].SetItemState(nNewSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				bUpdate = TRUE; // 标记需要更新列表
			}
		}
	}
	else
	{
		AfxMessageBox(_T("请先选择要删除的文件！"), MB_ICONINFORMATION);
	}

	if (bUpdate)
		UpdateList(index);
}
//添加文件按钮点击事件
void CPlayerTestDlg::OnBnClickedAdd0()
{
	Add(0);
}

// 删除文件按钮点击事件
void CPlayerTestDlg::OnBnClickedRemove0()
{
	Remove(0);
}

void CPlayerTestDlg::OnBnClickedAdd1()
{
	Add(1);
}

void CPlayerTestDlg::OnBnClickedRemove1()
{
	Remove(1);
}

void CPlayerTestDlg::UpdateList(int index) {
	//刷新列表
	int nItemCount = m_list[index].GetItemCount();  // 获取列表项总数
	m_vecMedia[index].clear();

	m_timeMax[index] = 0.0f;
	// 遍历所有列表项
	for (int nItem = 0; nItem < nItemCount; nItem++)
	{
		MediaCtx ctx;
		// 获取第i项第0列（文件名）的内容
		ctx.m_strName = m_list[index].GetItemText(nItem, 0);//名字

		CString strTime = m_list[index].GetItemText(nItem, 1);//时长
		float fValue = (float)_ttof(strTime);  // 转换为float

		ctx.m_start = 0.0f;
		ctx.m_end = fValue;
		ctx.m_offset = m_timeMax[index];//素材开始播放时间

		if (m_nDelay > 0) {
			if (nItem > 0)
				ctx.m_offset += m_nDelay; // 每个素材之间预留的间隔
		}
		else {
			if (nItem > 0) {
				ctx.m_offset += 0.01f; // 每个素材之间留0.01秒的间隔
			}
		}

		ctx.m_stop = ctx.m_offset + fValue;//结束播放时间

		CString strStart;
		strStart.Format(_T("%.2f秒"), ctx.m_offset);
		m_list[index].SetItemText(nItem, 2, strStart);
		CString strStop;
		strStop.Format(_T("%.2f秒"), ctx.m_stop);
		m_list[index].SetItemText(nItem, 3, strStop);


		m_timeMax[index] = ctx.m_stop + 0.01f;//总时长
		m_vecMedia[index].push_back(ctx);
	}

	float fMax = 0.0f;
	for (size_t i = 0; i < CHANNEL; i++)
	{
		if(m_timeMax[i] > fMax)
			fMax = m_timeMax[i];
	}
	m_timeTotal = fMax; // 更新总时长
	return;
}

void CPlayerTestDlg::OnBnClickedPlay()
{
	// TODO: 在此添加控件通知处理程序代码

	for (size_t channel = 0; channel < CHANNEL; channel++)
	{
		if (m_bRunning[channel]) {
			m_bWillStop[channel] = true;
			// 设置停止标志,在线程函数完成销毁操作
		}
		else if (!m_bWillStop[channel]) {
			UpdateData(TRUE); // 更新控件数据

			WXSetGlobalValue(L"MediaPlayer", m_bLAV ? MEDIAPLAYER_LAV : MEDIAPLAYER_FFPLAY); //是否强制使用LAV播放器
			WXSetGlobalValue(L"HwVideoDecode", m_bHW);//是否使用硬解码

			WXSetGlobalValue(L"RgbOutput", m_bRGB_Output);//优先RGB回调

			// 开始播放
			int nPos = m_slider.GetPos();
			m_timeCurr = (float)nPos * m_timeTotal / 100.0f;//计算在总时长中的位置
			m_tsCurr = timeGetTime();// 重置当前时间戳
			m_bRunning[channel] = TRUE;

			GetDlgItem(IDC_Play)->SetWindowText(_T("Stop"));
		}
	}


}

void CPlayerTestDlg::OnBnClickedPause()
{
	// TODO: 在此添加控件通知处理程序代码
	if (!m_bRunning[0] && !m_bRunning[1]) {
		AfxMessageBox(_T("请先开始播放！"), MB_ICONINFORMATION);
		return;
	}
	if(m_bPause[0]) {
		// 继续播放
		if(!m_bWillPause[0])
			m_bWillResume[0] = true; // 设置恢复标志
	}
	else {
		// 暂停播放
		if (!m_bWillResume[0])
			m_bWillPause[0] = true;
	}	
	if (m_bPause[1]) {
		// 继续播放
		if (!m_bWillPause[1])
			m_bWillResume[1] = true; // 设置恢复标志
	}
	else {
		// 暂停播放
		if (!m_bWillResume[1])
			m_bWillPause[1] = true;
	}
}

void CPlayerTestDlg::Func(int channel)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_bWillStop[channel]) {
		// 停止播放
		for (size_t i = 0; i < m_vecMedia[channel].size(); i++)
		{
			if (m_vecMedia[channel][i].m_play != nullptr) {
				WXFfplayPause(m_vecMedia[channel][i].m_play); // 暂停当前播放器
				WXFfplayDestroy(m_vecMedia[channel][i].m_play); // 销毁当前播放器
				m_vecMedia[channel][i].m_play = nullptr; // 清空当前播放器
			}
		}
		m_playerCurr[channel] = nullptr;

		m_bWillStop[channel] = false;

		m_bRunning[channel] = FALSE;
		m_bPause[channel] = false;

		m_bWillResume[channel] = false;
		m_bWillPause[channel] = false;//状态清空了

		if(!m_bRunning[0] && !m_bRunning[1])
			GetDlgItem(IDC_Play)->SetWindowText(_T("Play"));
	}

	if (m_bWillResume[channel]) {
		if (m_playerCurr[channel]) {
			WXFfplayResume(m_playerCurr[channel]);
			m_tsCurr = timeGetTime();// 重置当前时间戳
		}
		m_bPause[channel] = FALSE;
		m_bWillResume[channel] = FALSE;
		GetDlgItem(IDC_PAUSE)->SetWindowText(_T("Pause"));
	}

	if (m_bWillPause[channel]) {
		if (m_playerCurr[channel]) {
			WXFfplayPause(m_playerCurr[channel]);
		}
		m_bPause[channel] = TRUE;
		m_bWillPause[channel] = false;
		GetDlgItem(IDC_PAUSE)->SetWindowText(_T("Resume"));
	}



	// 如果是播放状态，更新进度条
	if (m_bRunning[channel] && !m_bPause[channel]) {
		DWORD tsNew = timeGetTime(); // 获取当前时间戳
		DWORD tsDiff = (tsNew - m_tsCurr);//计算和当前时间的差值

		float fCurrentTime = m_timeCurr + (tsDiff / 1000.0f);
		if (fCurrentTime > m_timeTotal)
			fCurrentTime = m_timeTotal;
		// 更新进度条位置
		m_nPos = (int)(fCurrentTime * 100.0f / m_timeTotal);
		m_slider.SetPos(m_nPos);

		// 更新当前时间显示
		CString strTime;
		strTime.Format(_T("%.2f/%0.2f"), fCurrentTime, m_timeTotal);
		SetDlgItemText(IDC_TXT, strTime);

		int nIndex = -1;
		for (size_t i = 0; i < m_vecMedia[channel].size(); i++)
		{
			if (fCurrentTime >= m_vecMedia[channel][i].m_offset && fCurrentTime <= m_vecMedia[channel][i].m_stop) {
				//播放对应的Player
				nIndex = i;
				break;
			}
		}
		if (nIndex < 0) {
			m_strFileName[channel] = _T("未找到对应的素材");
		}
		if (nIndex >= 0) {
			if (nIndex >= 1) {// 停止并销毁前一个播放器,主要用于顺序播放时的切换播放对象
				if (m_vecMedia[channel][nIndex - 1].m_play) {
					if (m_playerCurr[channel] == m_vecMedia[channel][nIndex - 1].m_play) {
						m_playerCurr[channel] = nullptr;// 清空当前播放器
					}
					WXFfplayPause(m_vecMedia[channel][nIndex - 1].m_play);
					WXFfplayDestroy(m_vecMedia[channel][nIndex - 1].m_play);
					m_vecMedia[channel][nIndex - 1].m_play = nullptr;
				}
			}

			if (m_vecMedia[channel][nIndex].m_play == nullptr) {
				//当前Index元素不存在播放对象
				float fSeek = (fCurrentTime - m_vecMedia[channel][nIndex].m_offset) + m_vecMedia[channel][nIndex].m_start;
				m_vecMedia[channel][nIndex].m_play = WXFfplayCreate(L"FFPLAY", m_vecMedia[channel][nIndex].m_strName.c_str(), 100, fSeek * 1000.0f);

				if (m_vecMedia[channel][nIndex].m_play) {
					//WXFfplaySetView(m_vecMedia[channel][nIndex].m_play, m_hwnd[channel]); // 设置显示窗口
					WXFfplaySetAVFrameCB2(m_vecMedia[channel][nIndex].m_play, this, channel, onFrame); // 设置视频帧回调函数
				}



				//关闭前一个curr
				if (m_playerCurr[channel]) {
					//m_playerCurr 不一定是m_vecMedia[nIndex-1].m_play，也可以向前跳转或者跨Index切换
					WXFfplayPause(m_playerCurr[channel]); // 暂停当前播放器
					WXFfplayDestroy(m_playerCurr[channel]); // 销毁当前播放器
					for (size_t i = 0; i < m_vecMedia[channel].size(); i++)
					{
						if (m_playerCurr == m_vecMedia[channel][i].m_play) {
							m_vecMedia[channel][i].m_play = nullptr; // 清空当前播放器
							break;
						}
					}
					m_playerCurr[channel] = nullptr; // 清空当前播放器
				}

				m_strFileName[channel] = m_vecMedia[channel][nIndex].m_strName.c_str(); // 更新当前播放文件名
				m_playerCurr[channel] = m_vecMedia[channel][nIndex].m_play;
				WXFfplayStart(m_playerCurr[channel]); // 开始播放
				if (m_bSeek) {
					m_bSeek = FALSE; // 重置拖动标志
				}
			}
			else {
				//当前player已经存在，检查是否需要切换
				//可能是跨Index切换
				if (m_playerCurr[channel] != m_vecMedia[channel][nIndex].m_play && m_playerCurr) {
					WXFfplayPause(m_playerCurr);
					WXFfplayDestroy(m_playerCurr);
					for (size_t i = 0; i < m_vecMedia[channel].size(); i++) {
						if (m_playerCurr == m_vecMedia[channel][i].m_play) {
							m_vecMedia[channel][i].m_play = nullptr;
							break;
						}
					}
					m_playerCurr[channel] = nullptr;
				}
				else if (m_playerCurr[channel] == m_vecMedia[channel][nIndex].m_play) {
					if (m_bSeek) {
						float fSeek = (fCurrentTime - m_vecMedia[channel][nIndex].m_offset) + m_vecMedia[channel][nIndex].m_start;
						WXFfplaySeek(m_playerCurr[channel], fSeek * 1000.0f); // 拖动到指定位置
						m_bSeek = FALSE; // 重置拖动标志
					}
					WXFfplayResume(m_playerCurr[channel]); // 恢复当前播放器
				}
			}

			m_timeCurr = fCurrentTime; // 更新当前时间
			m_tsCurr = tsNew; // 更新当前时间戳
		}

		if (nIndex >= 0 && nIndex < m_vecMedia[channel].size() - 1) {
			//顺序播放是提前准备下一个播放器，避免到时候的卡顿
			float nextTime = m_vecMedia[channel][nIndex + 1].m_offset;
			float diff = fabs(nextTime - fCurrentTime);//计算当前时间与下一个素材开始时间的差值
			if (diff < 2.0f) {
				//误差小于2秒，准备切换到下一个播放器
				if (m_vecMedia[channel][nIndex + 1].m_play == nullptr) {
					m_vecMedia[channel][nIndex + 1].m_play = WXFfplayCreate(L"FFPLAY", m_vecMedia[channel][nIndex + 1].m_strName.c_str(), 100, m_vecMedia[channel][nIndex + 1].m_start * 1000.0f);
					if (m_vecMedia[channel][nIndex + 1].m_play) {

						WXFfplaySetAVFrameCB2(m_vecMedia[channel][nIndex + 1].m_play, this, channel, onFrame); // 设置视频帧

						WXFfplaySetVolume(m_vecMedia[channel][nIndex + 1].m_play, 100); // 设置音量
						//WXFfplayStart(m_playerNext); //还不能Start，需要等到当前播放器结束
					}
				}
			}
		}

		GetDlgItem(m_ID_LOG[channel])->SetWindowText(m_strFileName[channel]); // 更新当前播放文件名显示
	}
}

void CPlayerTestDlg::PlayThread(int channel) {
	while (m_bRunThread) {
		DWORD t1 = timeGetTime();
		Func(channel);
		DWORD t2 = timeGetTime()-t1;
		int delay = 1000 - t2; // 计算剩余延迟时间
		if(delay < 100) {
			delay = 100; // 最小延迟时间
		}
		Sleep(delay);
	}
}

void CPlayerTestDlg::SeekTo(int nPos) {
	if (m_bPause) {
		//暂停状态下拖动进度条
		m_timeCurr = nPos * m_timeTotal / 100.0f; // 计算拖动到的时间点
		
		for (size_t channel = 0; channel < CHANNEL; channel++)
		{
			int nIndex = -1;
			for (size_t i = 0; i < m_vecMedia[channel].size(); i++)
			{
				if (m_timeCurr >= m_vecMedia[channel][i].m_offset && m_timeCurr <= m_vecMedia[channel][i].m_stop) {
					//播放对应的Player
					nIndex = i;
					break;
				}
			}
			if (nIndex == -1) {
				//没有对应的素材，直接返回
				m_strFileName[channel] = _T("未找到对应的素材");
			}
			else
			{
				m_strFileName[channel] = m_vecMedia[channel][nIndex].m_strName.c_str(); // 更新当前播放文件名
				if (m_playerCurr[channel] == m_vecMedia[channel][nIndex].m_play) {
					// 当前播放器已经是目标播放器
					float fSeek = (m_timeCurr - m_vecMedia[channel][nIndex].m_offset) + m_vecMedia[channel][nIndex].m_start;
					WXFfplaySeek(m_playerCurr[channel], fSeek * 1000.0f); // 拖动到指定位置
					WXFfplayRefresh(m_playerCurr[channel]); // 刷新播放器
				}
				else
				{
					// 当前播放器不是目标播放器，先暂停并销毁当前播放器
					if (m_playerCurr[channel]) {
						WXFfplayPause(m_playerCurr[channel]); // 暂停当前播放器
						WXFfplayDestroy(m_playerCurr[channel]); // 销毁当前播放器
						for (size_t i = 0; i < m_vecMedia[channel].size(); i++)
						{
							if (m_playerCurr[channel] == m_vecMedia[channel][i].m_play) {
								m_vecMedia[channel][i].m_play = nullptr; // 清空当前播放器
								break;
							}
						}
						m_playerCurr[channel] = nullptr; // 清空当前播放器
					}
					// 创建新的播放器
					float fSeek = (m_timeCurr - m_vecMedia[channel][nIndex].m_offset) + m_vecMedia[channel][nIndex].m_start;
					m_vecMedia[channel][nIndex].m_play = WXFfplayCreate(L"FFPLAY", m_vecMedia[channel][nIndex].m_strName.c_str(), 100, fSeek * 1000.0f);
					m_playerCurr[channel] = m_vecMedia[channel][nIndex].m_play; // 设置当前播放器为新创建的播放器
					if (m_playerCurr[channel]) {

						WXFfplaySetAVFrameCB2(m_playerCurr[channel], this, channel, onFrame); // 设置视频帧

						WXFfplaySetVolume(m_playerCurr[channel], 100); // 设置音量
						WXFfplayStart(m_playerCurr[channel]); // 开始播放
						WXFfplayPause(m_playerCurr[channel]); // 暂停播放器，等待拖动完成
						WXFfplayRefresh(m_playerCurr[channel]); // 刷新播放器
					}
					else {
						m_strFileName[channel] = _T("无法创建播放器");
					}
				}
			}

			SetDlgItemText(m_ID_LOG[channel], m_strFileName[channel]);
		}
		CString strTime;
		strTime.Format(_T("%.2f/%0.2f"), m_timeCurr, m_timeTotal);
		SetDlgItemText(IDC_TXT, strTime);
		m_bSeek = FALSE; // 重置拖动标志
	}
	else {
		//播放状态下拖动进度条
		m_timeCurr = nPos * m_timeTotal / 100.0f; // 计算拖动到的时间点
		m_tsCurr = timeGetTime(); // 重置当前时间戳
	}
}

void CPlayerTestDlg::OnNMReleasedcaptureSlider1(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	// 获取滑动条控件
	CSliderCtrl* pSlider = (CSliderCtrl*)GetDlgItem(IDC_SLIDER1);
	if (pSlider) //每次只处理一次拖动事件
	{
		// 获取当前滑块位置
		int nPos = pSlider->GetPos();
		if (nPos != m_nPos) {
			m_bSeek = TRUE; // 标记正在拖动进度条
			SeekTo(nPos); // 拖动进度条到指定位置
		}
	}
}

