
// MFC-FFPLAYDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFC-FFPLAY.h"
#include "MFC-FFPLAYDlg.h"
#include "afxdialogex.h"
#include <WXMedia/WXMedia.h>
#include "FfmpegIncludes.h"

//#include <libyuv.h>
#pragma comment(lib,"WXMedia.lib")
#pragma comment(lib,"libffmpeg.lib")

#include <time.h>
//#include <fftw3.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//--
static int data[1920];

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


// CMFCFFPLAYDlg 对话框



CMFCFFPLAYDlg::CMFCFFPLAYDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCFFPLAY_DIALOG, pParent)
	, m_strInput(_T(""))
	, m_strOutput(_T(""))
	, m_lStart(60000)
	, m_lDuration(60000)
	, m_strWaterMark(_T(""))
	, m_strSubtitle(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCFFPLAYDlg::DoDataExchange(CDataExchange* pDX){
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strInput);
	DDX_Text(pDX, IDC_EDIT2, m_strOutput);
	DDX_Control(pDX, IDC_COMBO1, m_cbVideoCodec);
	DDX_Control(pDX, IDC_COMBO2, m_cbVideoFps);
	DDX_Control(pDX, IDC_COMBO3, m_cbVideoSize);
	DDX_Control(pDX, IDC_COMBO4, m_cbVideoBitrate);
	DDX_Control(pDX, IDC_COMBO9, m_cbAudioCodec);
	DDX_Control(pDX, IDC_COMBO10, m_cbAudioSampleRate);
	DDX_Control(pDX, IDC_COMBO11, m_cbAudioChannel);
	DDX_Control(pDX, IDC_COMBO12, m_cbAudioBitrate);
	DDX_Control(pDX, IDC_PROGRESS1, m_progressFfmpegTask1);
	DDX_Text(pDX, IDC_EDIT3, m_lStart);
	DDX_Text(pDX, IDC_EDIT4, m_lDuration);
	DDX_Control(pDX, IDC_PROGRESS2, m_progressFfmpegTask2);
	DDX_Text(pDX, IDC_EDIT5, m_strWaterMark);
	DDX_Text(pDX, IDC_EDIT6, m_strSubtitle);
	DDX_Control(pDX, IDC_SLIDER6, m_sliderRotate);
	DDX_Control(pDX, IDC_SLIDER1, m_sliderVolume);
	DDX_Control(pDX, IDC_SLIDER2, m_sliderSpeed);
	DDX_Control(pDX, IDC_SLIDER3, m_sliderLight);
	DDX_Control(pDX, IDC_SLIDER4, m_sliderDBD);
	DDX_Control(pDX, IDC_SLIDER5, m_sliderBHD);
	DDX_Control(pDX, IDC_COMBO5, m_comModeVideo);
}

BEGIN_MESSAGE_MAP(CMFCFFPLAYDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_PLAY1, &CMFCFFPLAYDlg::OnBnClickedPlay1)
	ON_BN_CLICKED(IDC_PLAY2, &CMFCFFPLAYDlg::OnBnClickedPlay2)
	ON_BN_CLICKED(IDC_PLAY3, &CMFCFFPLAYDlg::OnBnClickedPlay3)
	ON_BN_CLICKED(IDC_PLAY4, &CMFCFFPLAYDlg::OnBnClickedPlay4)
	ON_BN_CLICKED(IDC_PAUSE, &CMFCFFPLAYDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_RESUME, &CMFCFFPLAYDlg::OnBnClickedResume)
	ON_BN_CLICKED(IDC_DXVA, &CMFCFFPLAYDlg::OnBnClickedDxva)
	ON_BN_CLICKED(IDC_FFMPEG, &CMFCFFPLAYDlg::OnBnClickedFfmpeg)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_INPUTFILE, &CMFCFFPLAYDlg::OnBnClickedInputfile)
	ON_BN_CLICKED(IDC_OUTPUTFILE, &CMFCFFPLAYDlg::OnBnClickedOutputfile)
	ON_BN_CLICKED(IDC_START, &CMFCFFPLAYDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_ImageSpot, &CMFCFFPLAYDlg::OnBnClickedImagespot)
	ON_BN_CLICKED(IDC_BUTTON4, &CMFCFFPLAYDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_GetInfo, &CMFCFFPLAYDlg::OnBnClickedGetinfo)
	ON_BN_CLICKED(IDC_WM, &CMFCFFPLAYDlg::OnBnClickedWm)
	ON_BN_CLICKED(IDC_BACK, &CMFCFFPLAYDlg::OnBnClickedBack)
	ON_BN_CLICKED(IDC_FRONT, &CMFCFFPLAYDlg::OnBnClickedFront)
	ON_BN_CLICKED(IDC_MULSTREAM, &CMFCFFPLAYDlg::OnBnClickedMulstream)
	ON_BN_CLICKED(IDC_BUTTON8, &CMFCFFPLAYDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_WaterMark, &CMFCFFPLAYDlg::OnBnClickedWatermark)
	ON_BN_CLICKED(IDC_SEEK0, &CMFCFFPLAYDlg::OnBnClickedSeek0)
	ON_BN_CLICKED(IDC_PLAY1_STOP, &CMFCFFPLAYDlg::OnBnClickedPlay1Stop)
	ON_BN_CLICKED(IDC_SUBTITLE, &CMFCFFPLAYDlg::OnBnClickedSubtitle)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER6, &CMFCFFPLAYDlg::onRotate)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CMFCFFPLAYDlg::onVolume)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER2, &CMFCFFPLAYDlg::onSpeed)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER3, &CMFCFFPLAYDlg::onLight)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER4, &CMFCFFPLAYDlg::onDBD)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER5, &CMFCFFPLAYDlg::onBHD)
	ON_BN_CLICKED(IDC_VFLIP, &CMFCFFPLAYDlg::OnBnClickedVflip)
	ON_BN_CLICKED(IDC_HFLIP, &CMFCFFPLAYDlg::OnBnClickedHflip)
	ON_BN_CLICKED(IDC_SEEK1000, &CMFCFFPLAYDlg::OnBnClickedSeek1000)
	ON_BN_CLICKED(IDC_BUTTON12, &CMFCFFPLAYDlg::OnBnClickedButton12)
	ON_BN_CLICKED(IDC_BUTTON13, &CMFCFFPLAYDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDC_SEEK2000, &CMFCFFPLAYDlg::OnBnClickedSeek2000)
	ON_BN_CLICKED(IDC_SEEK4000, &CMFCFFPLAYDlg::OnBnClickedSeek4000)
	ON_BN_CLICKED(IDC_SEEK6000, &CMFCFFPLAYDlg::OnBnClickedSeek6000)
	ON_BN_CLICKED(IDC_SEEKNOW, &CMFCFFPLAYDlg::OnBnClickedSeeknow)
	ON_BN_CLICKED(IDC_TEST, &CMFCFFPLAYDlg::OnBnClickedTest)
	ON_BN_CLICKED(IDC_RESET, &CMFCFFPLAYDlg::OnBnClickedReset)
	ON_BN_CLICKED(IDC_AIRPLAY, &CMFCFFPLAYDlg::OnBnClickedAirplay)
	ON_BN_CLICKED(IDC_BUTTON17, &CMFCFFPLAYDlg::OnBnClickedButton17)
	ON_BN_CLICKED(IDC_CHANGE, &CMFCFFPLAYDlg::OnBnClickedChange)
	ON_BN_CLICKED(IDC_REC, &CMFCFFPLAYDlg::OnBnClickedRec)
	ON_BN_CLICKED(IDC_REC_STOP, &CMFCFFPLAYDlg::OnBnClickedRecStop)
END_MESSAGE_MAP()


// CMFCFFPLAYDlg 消息处理程序

BOOL CMFCFFPLAYDlg::OnInitDialog()
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

	//time_t t = time(nullptr);
	//tm* local = localtime(&t);
	//if (local->tm_mon > 10) {
	//	AfxMessageBox(_T("Time Out"));
	//	exit(-1);
	//}


	//wchar_t MyDirDoc[_MAX_PATH];
	//SHGetSpecialFolderPath(this->GetSafeHwnd(), MyDirDoc, CSIDL_PERSONAL, 0);
	//CString strDoc = MyDirDoc;
	//strDoc += L"\\Apowersoft.Play.log";//我的文档目录
	WXDeviceInit(L"FFplay.log");//设备枚举初始化

	//m_strInput = L"C:\\Users\\tam\\Desktop\\a.mp4"; //_T("C:\\Users\\tam\\Desktop\\a.mpg");

	//m_strInput = L"rtp://127.0.0.1:62853";
	//m_strInput = L"rtmp://58.200.131.2:1935/livetv/hunantv";//
	m_strInput = _T("C:\\Users\\ten\\Desktop\\111.mp4");

	//m_strOutput = _T("C:\\Users\\tam\\Desktop\\a.mov.mp4");
	m_strOutput = _T("C:\\Users\\ten\\Desktop\\111---aaa.mp4");
	m_strWaterMark = _T("1.png");
	m_strSubtitle = _T("a.srt");

	//m_strWaterMark = _T("C:\\Users\\momo\\Desktop\\bd_logo1.png");

	//-vcodec
	m_cbVideoCodec.InsertString(0, _T("noset"));
	m_cbVideoCodec.InsertString(1, _T("h264"));
	m_cbVideoCodec.InsertString(2, _T("mpeg4"));
	m_cbVideoCodec.SetCurSel(0);

	//-r 
	m_cbVideoFps.InsertString(0, _T("noset"));
	m_cbVideoFps.InsertString(1, _T("60"));
	m_cbVideoFps.InsertString(2, _T("59.44"));
	m_cbVideoFps.InsertString(3, _T("50"));
	m_cbVideoFps.InsertString(4, _T("30"));
	m_cbVideoFps.InsertString(5, _T("29.97"));
	m_cbVideoFps.InsertString(6, _T("25"));
	m_cbVideoFps.InsertString(7, _T("24"));
	m_cbVideoFps.InsertString(8, _T("23.976"));
	m_cbVideoFps.InsertString(9, _T("15"));
	m_cbVideoFps.InsertString(10, _T("12"));
	m_cbVideoFps.SetCurSel(0);

	//-s xxxxxx
	m_cbVideoSize.InsertString(0, _T("noset"));
	m_cbVideoSize.InsertString(1, _T("4096x2160"));
	m_cbVideoSize.InsertString(2, _T("3840x2160"));
	m_cbVideoSize.InsertString(3, _T("1920x1090"));
	m_cbVideoSize.InsertString(4, _T("1600x1200"));
	m_cbVideoSize.InsertString(5, _T("1600x1024"));
	m_cbVideoSize.InsertString(6, _T("1408x1152"));
	m_cbVideoSize.InsertString(7, _T("1366x768"));
	m_cbVideoSize.InsertString(8, _T("1280x1024"));
	m_cbVideoSize.InsertString(9, _T("1280x720"));
	m_cbVideoSize.InsertString(10, _T("1024x768"));
	m_cbVideoSize.InsertString(11, _T("960x540"));
	m_cbVideoSize.InsertString(12, _T("852x480"));
	m_cbVideoSize.InsertString(13, _T("800x600"));
	m_cbVideoSize.InsertString(14, _T("768x576"));
	m_cbVideoSize.InsertString(15, _T("720x576"));
	m_cbVideoSize.InsertString(16, _T("720x480"));
	m_cbVideoSize.InsertString(17, _T("704x576"));
	m_cbVideoSize.InsertString(18, _T("640x480"));
	m_cbVideoSize.InsertString(19, _T("640x360"));
	m_cbVideoSize.InsertString(20, _T("480x320"));
	m_cbVideoSize.InsertString(21, _T("352x288"));
	m_cbVideoSize.InsertString(22, _T("352x240"));
	m_cbVideoSize.InsertString(23, _T("320x240"));
	m_cbVideoSize.InsertString(24, _T("176x144"));
	m_cbVideoSize.InsertString(25, _T("160x120"));
	m_cbVideoSize.SetCurSel(0);

	m_comModeVideo.InsertString(0, _T("画质优先"));
	m_comModeVideo.InsertString(1, _T("普通模式"));
	m_comModeVideo.InsertString(2, _T("速度优先"));
	m_comModeVideo.SetCurSel(1);

	//-b
	m_cbVideoBitrate.InsertString(0, _T("noset"));
	m_cbVideoBitrate.InsertString(1, _T("NormalQuality"));
	m_cbVideoBitrate.InsertString(2, _T("HighQuality"));
	m_cbVideoBitrate.InsertString(3, _T("8000k"));
	m_cbVideoBitrate.InsertString(4, _T("6000k"));
	m_cbVideoBitrate.InsertString(5, _T("4000k"));
	m_cbVideoBitrate.InsertString(6, _T("3000k"));
	m_cbVideoBitrate.InsertString(7, _T("2000k"));
	m_cbVideoBitrate.InsertString(8, _T("1500k"));
	m_cbVideoBitrate.InsertString(9, _T("1200k"));
	m_cbVideoBitrate.InsertString(10, _T("1000k"));
	m_cbVideoBitrate.InsertString(11, _T("768k"));
	m_cbVideoBitrate.InsertString(12, _T("512k"));
	m_cbVideoBitrate.InsertString(13, _T("384k"));
	m_cbVideoBitrate.SetCurSel(0);

	//-acodec
	m_cbAudioCodec.InsertString(0, _T("noset"));
	m_cbAudioCodec.InsertString(1, _T("aac"));
	m_cbAudioCodec.InsertString(2, _T("mp3"));
	m_cbAudioCodec.InsertString(3, _T("ac3"));
	m_cbAudioCodec.SetCurSel(0);

	//-ar
	m_cbAudioSampleRate.InsertString(0, _T("noset"));
	m_cbAudioSampleRate.InsertString(1, _T("48000"));
	m_cbAudioSampleRate.InsertString(2, _T("44100"));
	m_cbAudioSampleRate.InsertString(3, _T("32000"));
	m_cbAudioSampleRate.InsertString(3, _T("22500"));
	m_cbAudioSampleRate.SetCurSel(0);

	//-ac
	m_cbAudioChannel.InsertString(0, _T("noset"));
	m_cbAudioChannel.InsertString(1, _T("stereo"));
	m_cbAudioChannel.InsertString(2, _T("momo"));
	m_cbAudioChannel.SetCurSel(0);

	//-ab
	m_cbAudioBitrate.InsertString(0, _T("noset"));
	m_cbAudioBitrate.InsertString(1, _T("320k"));
	m_cbAudioBitrate.InsertString(2, _T("256k"));
	m_cbAudioBitrate.InsertString(3, _T("192k"));
	m_cbAudioBitrate.InsertString(4, _T("160k"));
	m_cbAudioBitrate.InsertString(5, _T("128k"));
	m_cbAudioBitrate.InsertString(6, _T("96k"));
	m_cbAudioBitrate.InsertString(7, _T("64k"));
	m_cbAudioBitrate.InsertString(8, _T("48k"));
	m_cbAudioBitrate.InsertString(9, _T("32k"));
	m_cbAudioBitrate.SetCurSel(0);

	m_progressFfmpegTask1.SetRange(0, 100);
	m_progressFfmpegTask2.SetRange(0, 100);

	m_sliderRotate.SetRange(0, 360);// 设置角度
	m_sliderRotate.SetPos(0);

	m_sliderVolume.SetRange(0, 100);//设置音量
	m_sliderVolume.SetPos(100);

	m_sliderSpeed.SetRange(50, 200);//设置速度
	m_sliderSpeed.SetPos(100);

	m_sliderLight.SetRange(-100, 100);//
	m_sliderLight.SetPos(0);

	m_sliderDBD.SetRange(-100, 100);
	m_sliderDBD.SetPos(50);

	m_sliderBHD.SetRange(0, 300);
	m_sliderBHD.SetPos(100);
	UpdateData(FALSE);

	// TODO: 在此添加额外的初始化代码
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCFFPLAYDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}else{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCFFPLAYDlg::OnPaint()
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
HCURSOR CMFCFFPLAYDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString CurrPath() {
	CString    sPath;
	GetModuleFileName(nullptr, sPath.GetBufferSetLength(MAX_PATH + 1), MAX_PATH);
	int    nPos;
	nPos = sPath.ReverseFind('\\');
	sPath = sPath.Left(nPos);
	nPos = sPath.ReverseFind('\\');
	sPath = sPath.Left(nPos);
	return    sPath;
}

#define FFMPEG_ERROR_OK 0

extern "C" void PlayCB(void *ctx, LPCTSTR wszID, uint32_t iEvent, LPCTSTR msg) {
	CString str = wszID;
	if (iEvent == FFMPEG_ERROR_OK) {
		str += _T(" 播放完毕");
	}
	//AfxMessageBox(str);
}

extern "C" void ConvertCB(void *ctx, LPCTSTR wszID, uint32_t iEvent, LPCTSTR msg) {
	CString str = wszID;
	if (iEvent == FFMPEG_ERROR_OK) {
		str += _T(" 处理完毕");
	}else{
		str += _T(" 处理错误 ");
		str += msg;
	}
	//AfxMessageBox(str);
}


//VLC 推流
void CMFCFFPLAYDlg::OnBnClickedPlay2()
{
	//CString str = L"rtmp://live.hkstv.hk.lxdns.com/live/hks";
	UpdateData(TRUE);
	CString str = m_strInput;
	if (m_play2 == nullptr) {
		m_play2 = WXFfplayCreate(_T("FFPLAY"),(LPCWSTR)str, 100, 0);
		if (m_play2) {
			WXFfplaySetView(m_play2, GetDlgItem(IDC_PIC2)->GetSafeHwnd());
			WXFfplaySetVolume(m_play2, 100);
			//WXFfplaySetEventCb(m_play2, PlayCB);
			//WXFfplaySetEventID(m_play2, str);
			int bOpen = WXFfplayStart(m_play2);
			if (bOpen) {
				SetTimer(TIME_ID_PLAY2, 1000, nullptr);
			}
		}
	}else {
		WXFfplayDestroy(m_play2);
		m_play2 = nullptr;
		KillTimer(TIME_ID_PLAY2);
	}
}


void CMFCFFPLAYDlg::OnBnClickedPlay3()
{
	//CString str = L"http://live.hkstv.hk.lxdns.com/live/hks/playlist.m3u8";
	//CString str = L"b.mp4";
	UpdateData(TRUE);
	CString str = m_strInput;
	if (m_play3 == nullptr) {
		m_play3 = WXFfplayCreate(_T("LAV"), (LPCWSTR)str, 100, 290);
		if (m_play3) {
			WXFfplaySetView(m_play3, GetDlgItem(IDC_PIC3)->GetSafeHwnd());
			WXFfplaySetVolume(m_play3, 100);
			WXFfplaySetEventCb(m_play3, PlayCB);
			WXFfplaySetEventID(m_play3, str);
			int bOpen = WXFfplayStart(m_play3);
			if (bOpen) {
				SetTimer(TIME_ID_PLAY3, 1000, nullptr);
			}
		}
	}else {
		WXFfplayDestroy(m_play3);
		m_play3 = nullptr;
		KillTimer(TIME_ID_PLAY3);
	}
}

void CMFCFFPLAYDlg::OnBnClickedPause(){
	if(m_play1)  WXFfplayPause(m_play1);
	if (m_play2) WXFfplayPause(m_play2);
	if (m_play3) WXFfplayPause(m_play3);
	if (m_play4) WXFfplayPause(m_play4);
}

void CMFCFFPLAYDlg::OnBnClickedResume(){
	if (m_play1)WXFfplayResume(m_play1);
	if (m_play2)WXFfplayResume(m_play2);
	if (m_play3)WXFfplayResume(m_play3);
	if (m_play4)WXFfplayResume(m_play4);
}


void CMFCFFPLAYDlg::OnBnClickedDxva(){
	OnBnClickedPlay1();
	OnBnClickedPlay2();
	OnBnClickedPlay3();
	OnBnClickedPlay4();
}


#ifndef _WIN32
class CRect {
public:
	int    left = 0;
	int    top = 0;
	int    right = 0;
	int    bottom = 0;
	
	CRect(_left, _top, _right, _bottom) {
		left = _left;
		top = _top;
		right = _right;
		bottom = _bottom;
	}

	void UnionRect(const CRect *rc1, const CRect *rc2) {
		this->left = min(rc1->left, rc2->left);
		this->top = min(rc1->top, rc2->top);
		this->right = max(rc1->right, rc2->right);
		this->bottom = max(rc1->bottom, rc2->bottom);
	}
};
#endif

void CMFCFFPLAYDlg::OnBnClickedInputfile(){
	// TODO: 在此添加控件通知处理程序代码

	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		CString sName = dlg.GetPathName();
		m_strInput = sName;
		UpdateData(FALSE);
	}
}

void CMFCFFPLAYDlg::OnBnClickedOutputfile(){
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(FALSE);
	if (IDOK == dlg.DoModal()) {
		CString sName = dlg.GetPathName();
		m_strOutput = sName;
		UpdateData(FALSE);
	}
}

extern "C" void MergerCB(void *ctx, uint32_t uID, int iEvent) {
	if (iEvent == 0) AfxMessageBox(_T("合并完毕"));
}

void CMFCFFPLAYDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_convert1)WXFfmpegInterrupt(m_convert1);
}


//WXMEDIA_API void WXTestH264Encoder(WXCTSTR wszYuv, WXCTSTR wszH265, int width, int height);
//合并文件
void CMFCFFPLAYDlg::OnBnClickedFfmpeg() {

	//WXTestH264Encoder(L"D:\\cif.yuv",L"D:\\aa.hevc",352,288);
	//AfxMessageBox(L"265 ok!");
	//return;
    const wchar_t *mix = L"C:\\Users\\tam\\Desktop\\flv\\Mix.flv";

    const wchar_t *arr[6] = { L"C:\\Users\\tam\\Desktop\\flv\\0.flv",
        L"C:\\Users\\tam\\Desktop\\flv\\1.flv",
        L"C:\\Users\\tam\\Desktop\\flv\\2.flv",
        L"C:\\Users\\tam\\Desktop\\flv\\3.flv",
        L"C:\\Users\\tam\\Desktop\\flv\\4.flv",
        L"C:\\Users\\tam\\Desktop\\flv\\5.flv"
    };

    WXFfmpegMergerFiles(mix,6,arr);
    AfxMessageBox(L"ok!");
}

//加水印
void CMFCFFPLAYDlg::OnBnClickedWm(){
	if (m_convert2 == nullptr) {
		m_convert2 = WXFfmpegParamCreate();
		WXFfmpegParamAddWMImage(m_convert2, L"a.png",100, 100, 80, 80);//水印图片
		int ret = WXFfmpegConvertVideo(m_convert2, L"a.mp4", L"wm.mp4", 0);
		if (ret == FFMPEG_ERROR_OK) {
			AfxMessageBox(L"水印成功");
		}else {
			AfxMessageBox(L"水印失败");
		}

		WXFfmpegParamDestroy(m_convert2);
		m_convert2 = nullptr;
	}
}


void CMFCFFPLAYDlg::OnBnClickedImagespot(){


	if (m_play1) {
		WXFfplayShotPicture(m_play1, L"C:\\Users\\momo\\Desktop\\aba.bmp", 100);
		return;
	}

	UpdateData(TRUE);
	m_convert1 = WXFfmpegParamCreate();
	int ret = WXFfmpegShotPicture(m_convert1, m_strInput, 1000, L"t.jpg");
	WXFfmpegParamDestroy(m_convert1);
	m_convert1 = nullptr;
	AfxMessageBox(L"截图成功");

}

void CMFCFFPLAYDlg::OnBnClickedBack(){
	WXFfplaySeek(m_play1, WXFfplayGetCurrTime(m_play1) - 10 * 1000);
	WXFfplaySeek(m_play2, WXFfplayGetCurrTime(m_play2) - 10 * 1000);
	WXFfplaySeek(m_play3, WXFfplayGetCurrTime(m_play3) - 10 * 1000);
	WXFfplaySeek(m_play4, WXFfplayGetCurrTime(m_play4) - 10 * 1000);
}


void CMFCFFPLAYDlg::OnBnClickedFront(){
	WXFfplaySeek(m_play1, WXFfplayGetCurrTime(m_play1) + 30 * 1000);
	WXFfplaySeek(m_play2, WXFfplayGetCurrTime(m_play2) + 30 * 1000);
	WXFfplaySeek(m_play3, WXFfplayGetCurrTime(m_play3) + 30 * 1000);
	WXFfplaySeek(m_play4, WXFfplayGetCurrTime(m_play4) + 30 * 1000);
}

EXTERN_C void onData(void *ctx, int width, int height, uint8_t** pData, int* linesize, int64_t pts) {
	CMFCFFPLAYDlg *pdlg = (CMFCFFPLAYDlg*)ctx;
	if (pdlg) {
		pdlg->onDraw(width, height, pData, linesize);
	}
}

void CMFCFFPLAYDlg::onDraw(int width, int height, uint8_t** pData, int* linesize) {
	//if (m_pRender == nullptr) {
	//	m_pRender = IWXVideoRenderCreate2(GetDlgItem(IDC_PIC1)->GetSafeHwnd(), width, height);
	//	m_pYUVData = WXMediaUtilsAllocVideoFrame(AV_PIX_FMT_YUV420P, width, height);
	//}
	//if (m_pRender) {
	//	for (int i = 0; i < height; i++) {
	//		memcpy(m_pYUVData->data[0] + i * m_pYUVData->linesize[0], pData[0] + i * linesize[0], width);
	//	}
	//	for (int i = 0; i < height/2; i++) {
	//		memcpy(m_pYUVData->data[1] + i * m_pYUVData->linesize[1], pData[1] + i * linesize[1], width/2);
	//		memcpy(m_pYUVData->data[2] + i * m_pYUVData->linesize[2], pData[2] + i * linesize[2], width / 2);
	//	}
	//	m_pRender->Display(m_pYUVData);
	//}
}

void CMFCFFPLAYDlg::Proc1() {
	if (g_conv == nullptr) {
		g_conv = WXMediaConvertCreate(L"C:\\Users\\tam\\Desktop\\a.mp4");
		int64_t FileSize = WXMediaConvertGetTargetSize(g_conv, MODE_FAST, 10);
		int ret = WXMediaConvertProcess(g_conv, L"C:\\Users\\tam\\Desktop\\a22.mp4", MODE_FAST, 10);
		WXMediaConvertDestroy(g_conv);
		g_conv = nullptr;
		AfxMessageBox(L"OK");
	}
}
void CMFCFFPLAYDlg::Proc2() {
	while (!m_bStop) {
		if (g_conv) {
			int Rate = WXMediaConvertGetProcess(g_conv);
			CString str1;
			str1.Format(L"进程A %d", Rate);
			GetDlgItem(IDC_TEXT)->SetWindowText(str1);

			m_progressFfmpegTask1.SetPos(Rate);
		}
		Sleep(100);
	}
}

void CMFCFFPLAYDlg::OnBnClickedStart() {
	UpdateData(TRUE);

	std::thread ConvThread([this] {
		void* conv = WXMediaConvertCreate((LPCTSTR)m_strInput);
		int64_t FileSize = WXMediaConvertGetTargetSize(conv, MODE_FAST, 10);
		int ret = WXMediaConvertProcess(conv, (LPCTSTR)m_strOutput, MODE_FAST, 10);
		WXMediaConvertDestroy(conv);
	});
	ConvThread.detach();

	return;

	//clock_t t1 = clock();
	//WXMediaFastConvert((LPCTSTR)m_strInput, (LPCTSTR)m_strOutput, 28615, 28630);
	//clock_t t2 = clock() - t1;
	//CString st;
	//st.Format(L"Time = %d ms", (int)t2);
	//AfxMessageBox(st);
	//return;

	////void* conv = WXMediaConvertCreate((LPCTSTR)m_strInput);
	////int64_t FileSize = WXMediaConvertGetTargetSize(conv , MODE_FAST, 10);
	////int ret = WXMediaConvertProcess(conv, (LPCTSTR)m_strOutput, MODE_FAST, 10);
	////WXMediaConvertDestroy(conv);

	//m_bStop = FALSE;

	//m_th1 = new std::thread(&CMFCFFPLAYDlg::Proc1, this);
	//m_th2 = new std::thread(&CMFCFFPLAYDlg::Proc2, this);

	//return;

	if (m_task1 != nullptr)return;
	UpdateData(TRUE);
	m_convert1 = WXFfmpegParamCreate();//转换参数结构体
	SetTimer(TIME_ID_CONVERT1, 1000, nullptr);


	///WXFfmpegParamSetConvertTime(m_convert1, 5660, 444444);//设置处理时间 -ss xx -t yy

	//WXFfmpegParamSetVideoSize(m_convert1, 852, 480);

	//WXFfmpegParamSetVideoFmtStr(m_convert1, L"yuv420p");
	//WXFfmpegParamSetVideoCodecStr(m_convert1, L"libx264");

	//设置视频帧率
	//WXFfmpegParamSetVideoFps(m_convert1, 15);
	//设置视频比特率
	//WXFfmpegParamSetVideoBitrate(m_convert1, 2000 * 1000);
	//设置音频比特率
	//WXFfmpegParamSetAudioBitrate(m_convert1, 64 * 1000);
	//if (compressionType == 4)
	{
		//设置音频采样率
		//WXFfmpegParamSetAudioSampleRate(m_convert1, 8000);
	}
	WXFfmpegConvertVideo(m_convert1, (LPCTSTR)m_strInput, (LPCTSTR)m_strOutput, 1);
	//WXFfmpegConvertVideo(g_convert1, L"C:\\Users\\steven\\Desktop\\iphone6\\test.mp4", L"C:\\Users\\steven\\Desktop\\iphone6\\out.mp4", 0);//启动编码
}


void CMFCFFPLAYDlg::Funtion() {
	if (m_convert1 == nullptr) {
	
		m_convert1 = WXFfmpegParamCreate();//转换参数结构体

		WXFfmpegParamSetConvertTime(m_convert1, 0, 50 * 1000);
		WXFfmpegConvertVideo(m_convert1, L"b.mpg", L"01.mp4", 0);

		WXFfmpegParamSetConvertTime(m_convert1, 50 * 1000, 60 * 1000);
		WXFfmpegConvertVideo(m_convert1, L"b.mpg", L"02.mp4", 0);

		WXFfmpegParamSetConvertTime(m_convert1, 100 * 1000, 70 * 1000);
		WXFfmpegConvertVideo(m_convert1, L"b.mpg", L"03.mp4", 0);


		WXFfmpegParamSetConvertTime(m_convert1, 150 * 1000, 80 * 1000);
		WXFfmpegConvertVideo(m_convert1, L"b.mpg", L"04.mp4", 0);

		WXFfmpegParamDestroy(m_convert1);
		m_convert1 = nullptr;
		KillTimer(TIME_ID_CONVERT3);
	}
}

void CMFCFFPLAYDlg::Task1() {
	if (m_task1 != nullptr)return;
	UpdateData(TRUE);
	m_convert1 = WXFfmpegParamCreate();//转换参数结构体
	WXFfmpegParamSetConvertTime(m_convert1, 0, 100  * 1000);//设置处理时间 -ss xx -t yy

	CString str;
	GetDlgItem(IDC_COMBO1)->GetWindowText(str);
	WXFfmpegParamSetVideoCodecStr(m_convert1, str);//设置视频编码器 -vcodec h264


	GetDlgItem(IDC_COMBO9)->GetWindowText(str);
	WXFfmpegParamSetAudioCodecStr(m_convert1, str);//设置音频编码器 -acodec aac

	GetDlgItem(IDC_COMBO10)->GetWindowText(str);
	WXFfmpegParamSetAudioSampleRate(m_convert1, 44100);//设置音频采样频率 -ar 44100

	GetDlgItem(IDC_COMBO11)->GetWindowText(str);
	WXFfmpegParamSetAudioChannel(m_convert1, 2);//设置音频声道 -ac 2

	SetTimer(TIME_ID_CONVERT1, 1000, nullptr);
	WXFfmpegConvertVideo(m_convert1, L"a.mp4",  L"a.mov", 1);//启动编码
}
void CMFCFFPLAYDlg::Task2() {


	if (m_task2 != nullptr)return;
	UpdateData(TRUE);
	m_convert2 = WXFfmpegParamCreate();//转换参数结构体
	WXFfmpegParamSetConvertTime(m_convert2, 100 * 1000, 200 * 1000);//设置处理时间 -ss xx -t yy

	CString str;
	GetDlgItem(IDC_COMBO1)->GetWindowText(str);
	WXFfmpegParamSetVideoCodecStr(m_convert2, str);//设置视频编码器 -vcodec h264

	GetDlgItem(IDC_COMBO9)->GetWindowText(str);
	WXFfmpegParamSetAudioCodecStr(m_convert2, str);//设置音频编码器 -acodec aac

	GetDlgItem(IDC_COMBO10)->GetWindowText(str);
	WXFfmpegParamSetAudioSampleRate(m_convert1, 48000);//设置音频采样频率 -ar 48000

	GetDlgItem(IDC_COMBO11)->GetWindowText(str);
	WXFfmpegParamSetAudioChannel(m_convert2, 2);//设置音频声道 -ac 2

	SetTimer(TIME_ID_CONVERT2, 500, nullptr);
	WXFfmpegConvertVideo(m_convert2, L"b.mp4", L"b.mov", 1);//启动编码
}

void CMFCFFPLAYDlg::OnBnClickedMulstream(){
	// TODO: 在此添加控件通知处理程序代码
	Task1();
	Task2();
}

#define FFMPEG_ERROR_PROCESS 8

void CMFCFFPLAYDlg::OnTimer(UINT_PTR nIDEvent) {

	if (nIDEvent == TIME_ID_CONVERT3 && m_convert1 != nullptr) {
		int state = WXFfmpegGetState(m_convert1);
		if (state == FFMPEG_ERROR_PROCESS) 
		{
			int64_t ptsTotal = WXFfmpegGetTotalTime(m_convert1);
			int64_t ptsCurr = WXFfmpegGetCurrTime(m_convert1);
			if (ptsCurr > 0 || ptsTotal > 0) {
				int pos = (int)(ptsCurr * 100 / ptsTotal);
				m_progressFfmpegTask1.SetPos(pos);
				CString str1;
				str1.Format(L"进程A 正在处理 总时间长度为 = %lldms 当前已经处理=%lldms", ptsTotal, ptsCurr);
				GetDlgItem(IDC_TEXT)->SetWindowText(str1);
			}
		}
	}

	if (nIDEvent == TIME_ID_CONVERT1 && m_convert1 != nullptr) {
		int state = WXFfmpegGetState(m_convert1);
		if (state == FFMPEG_ERROR_PROCESS) {
			int64_t ptsTotal = WXFfmpegGetTotalTime(m_convert1);
			int64_t ptsCurr = WXFfmpegGetCurrTime(m_convert1);
			if (ptsCurr > 0 || ptsTotal > 0){
				int pos = (int)(ptsCurr * 100 / ptsTotal);
				m_progressFfmpegTask1.SetPos(pos);
				CString str1;
				str1.Format(L"进程A 正在处理 总时间长度为 = %lldms 当前已经处理=%lldms", ptsTotal, ptsCurr);
				GetDlgItem(IDC_TEXT)->SetWindowText(str1);
			}
		}
		else {
			KillTimer(TIME_ID_CONVERT1);
			m_progressFfmpegTask1.SetPos(100);
			CString str1;
			if (state == 0) {
				str1.Format(L"进程A 处理成功");
			}else {
				CString ss = WXFfmpegGetError(state);
				str1 = L"进程A 处理失败 ";
				str1 += ss;
			}

			GetDlgItem(IDC_TEXT)->SetWindowText(str1);
			if (m_task1 != nullptr) {
				m_task1->join();
				delete m_task1;
				m_task1 = nullptr;
			}
			WXFfmpegParamDestroy(m_convert1);
			m_convert1 = nullptr;
		}
	}
	else if (nIDEvent == TIME_ID_CONVERT2 && m_convert2 != nullptr) {
		int state = WXFfmpegGetState(m_convert2);
		if (state == FFMPEG_ERROR_PROCESS) {
			int64_t ptsTotal = WXFfmpegGetTotalTime(m_convert2);
			int64_t ptsCurr = WXFfmpegGetCurrTime(m_convert2);
			if (ptsTotal > 0 && ptsTotal > 0) {
				int pos = (int)(ptsCurr * 100 / ptsTotal);
				m_progressFfmpegTask2.SetPos(pos);

				CString str1;
				str1.Format(L"进程B 正在处理 总时间长度为 = %lld 当前已经处理=%lld", ptsTotal, ptsCurr);
				GetDlgItem(IDC_TEXT2)->SetWindowText(str1);
			}
		}
		else {
			KillTimer(TIME_ID_CONVERT2);
			m_progressFfmpegTask2.SetPos(100);
			CString str1;
			if (state == FFMPEG_ERROR_OK)str1.Format(L"进程B 处理成功");
			else str1.Format(L"进程B 处理失败 %s ", WXFfmpegGetError(state));

			GetDlgItem(IDC_TEXT2)->SetWindowText(str1);
			if (m_task2 != nullptr) {
				m_task2->join();
				delete m_task2;
				m_task2 = nullptr;
			}
			WXFfmpegParamDestroy(m_convert2);
			m_convert2 = nullptr;
		}
	}
	else {
		CString strV;
		if (nIDEvent == TIME_ID_PLAY1) {
			int64_t pts = WXFfplayGetCurrTime(m_play1);
			int64_t ptsAll = WXFfplayGetTotalTime(m_play1);
			CString str;
			str.Format(L"Play1 当前播放时间 = %02fs Total=%02fs\n\r", pts/1000.0, ptsAll/1000.0);
			strV += str;
		}

		if (nIDEvent == TIME_ID_PLAY2) {
			int64_t pts = WXFfplayGetCurrTime(m_play2);
			int64_t ptsAll = WXFfplayGetTotalTime(m_play2);
			CString str;
			str.Format(L"Play2 当前播放时间 = %fs Total=%fs\n\r", pts / 1000.0, ptsAll / 1000.0);
			strV += str;
		}

		if (nIDEvent == TIME_ID_PLAY3) {
			int64_t pts = WXFfplayGetCurrTime(m_play3);
			int64_t ptsAll = WXFfplayGetTotalTime(m_play3);
			CString str;
			str.Format(L"Play3 当前播放时间 = %fs Total=%fs\n\r", pts / 1000.0, ptsAll / 1000.0);
			strV += str;
		}

		if (nIDEvent == TIME_ID_PLAY4) {
			int64_t pts = WXFfplayGetCurrTime(m_play4);
			int64_t ptsVideo = 0;
			int64_t ptsAudio = 0;
			int64_t ptsAll = WXFfplayGetTotalTime(m_play4);
			double speed = WXFfplayGetSpeed(m_play4);
			CString str;
			str.Format(L"P4[All=%0.2fs,T=%0.2fs,V=%0.2fs,A=%0.32s]speed=%0.2f", ptsAll / 1000.0, pts / 1000.0, ptsVideo / 1000.0, ptsAudio / 1000.0, speed);
			strV += str;
		}
		GetDlgItem(IDC_LOG)->SetWindowText(strV);
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CMFCFFPLAYDlg::OnBnClickedWatermark(){
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strWaterMark = dlg.GetPathName();
		UpdateData(FALSE);
	}
}


//录音测试
static void *audioCapture = nullptr;

EXTERN_C void onMyAudio(void *ctx, uint8_t * buf, int buf_size) {
	FILE *f = fopen("Capture.pcm","ab+");
	if (f) {
		fwrite(buf, buf_size, 1, f);
		fclose(f);
	}
}

static void* videoCapture = nullptr;
static int ssa = 0;
EXTERN_C void onMyVideo(void *ctx, struct AVFrame *frame) {
	if (ssa == 0) {
		ssa = 1;
		WXMediaUtilsSaveAsPicture(frame, L"aaaaaaaaa.jpg", 100);
	}
}
void CMFCFFPLAYDlg::OnBnClickedGetinfo()
{
	//if (videoCapture == nullptr) {
	//	videoCapture = WXVideoCaptureCreate();
	//	WXVideoCaptureSetDataSink(videoCapture, NULL, onMyVideo);
	//	WXVideoCaptureSetScreen(videoCapture, L"default", 10);
	//	int ret = WXVideoCaptureUpdate(videoCapture);
	//	if (ret == 0) {
	//		WXVideoCaptureStart(videoCapture);
	//	}
	//	else {
	//		WXVideoCaptureDestroy(videoCapture);
	//		videoCapture = nullptr;
	//	}
	//}
	//else {
	//	WXVideoCaptureStop(videoCapture);
	//	WXVideoCaptureDestroy(videoCapture);
	//	videoCapture = nullptr;
	//}
	UpdateData(TRUE);
	int error = 0;
	void* info = WXMediaInfoCreate(m_strInput, &error);
	if (info) {
		int ret = WXMediaInfoGetPicture(info, L"Capture.jpg");

		int vn = WXMediaInfoHasVideo(info);
		int an = WXMediaInfoHasAudio(info);
		int tn = WXMediaInfoHasAttach(info);
		int64_t time = WXMediaInfoGetFileDuration(info);
		CString str = m_strInput;

		{

			CString s1;
			s1.Format(L"\nVideo channel = %d\nAudio channel = %d\nAttach Channel = %d\nduration = %lld ms\n", vn, an, tn, time);
			str += s1;
		}

		if (an) {
			CString s1 = L"音频流 Codec=";
			CString s2 = WXMediaInfoGetAudioCodecName(info);
			s1 += s2;
			s2.Format(L"\nsr=%d ch=%d 码率=%ldKBps\n",
				WXMediaInfoGetAudioSampleRate(info),
				WXMediaInfoGetAudioChannels(info),
				(int)(WXMediaInfoGetAudioBitrate(info) / 1000)
			);
			s1 += s2;
			str += s1;
		}

		if (vn) {
			CString s1 = L"视频流 Codec = ";
			CString s2 = WXMediaInfoGetVideoCodecName(info);
			s1 += s2;
			s2.Format(L"\nSize=%dx%d 码率=%dKBps帧率=%0.2f 压缩比=%d \n",
				WXMediaInfoGetVideoWidth(info),
				WXMediaInfoGetVideoHeight(info),
				(int)(WXMediaInfoGetVideoBitrate(info) / 1000),
				WXMediaInfoGetVideoAvgFps(info),
				WXMediaInfoGetVideoCompress(info)
			);
			s1 += s2;
			str += s1;
		}

		WXMediaInfoDestroy(info);
		AfxMessageBox(str);
	}
}


void CMFCFFPLAYDlg::OnBnClickedPlay4() {

	UpdateData(TRUE);
	GetDlgItem(IDC_PIC4)->MoveWindow(10, 282, 650, 430);
	//CString str = L"D:\\lgz.mkv";
	if (m_play4 == nullptr) {
		m_play4 = WXFfplayCreate(L"FFPLAY", m_strInput, 100, 0);
		if (m_play4) {
			WXFfplaySetView(m_play4, GetDlgItem(IDC_PIC4)->GetSafeHwnd());
			WXFfplaySetVolume(m_play4, 100);
			//WXFfplaySetEventCbW(m_play4, PlayCB);
			//WXFfplaySetEventIDW(m_play4, m_strInput);
			int bOpen = WXFfplayStart(m_play4);
			if (bOpen) {
				SetTimer(TIME_ID_PLAY4, 1000, nullptr);
			}
		}
	}
	else {
		WXFfplayDestroy(m_play4);
		m_play4 = nullptr;
		KillTimer(TIME_ID_PLAY4);
	}
}


//rtp://127.0.0.1:62853
void CMFCFFPLAYDlg::OnBnClickedPlay1()
{
	// TODO: 在此添加控件通知处理程序代码
	//CString str = CurrPath() + L"\\data\\dmx.mp4";
	//CString str = _T("a.mp4");
	UpdateData(TRUE);
	GetDlgItem(IDC_PIC1)->MoveWindow(10, 282, 650, 430);
	//CString str = L"rtp://127.0.0.1:62853" /*m_strInput*/;

	CString str = m_strInput;

	int err = 0;
	void* info = WXMediaInfoCreate(str,&err);
	if (m_play1 == nullptr) {
		m_play1 = WXFfplayCreate(_T("LAV"), (LPCWSTR)str, 100, 0 * 1000);
		if (m_play1) {
			WXFfplaySetView(m_play1, GetDlgItem(IDC_PIC1)->GetSafeHwnd());
			WXFfplaySetVolume(m_play1, 100);
			WXFfplayStart(m_play1);
		}
	}else {
		WXFfplayDestroy(m_play1);
		m_play1 = nullptr;
		KillTimer(TIME_ID_PLAY1);
	}
}

void CMFCFFPLAYDlg::OnBnClickedButton8() {
	if (m_play1 != nullptr) {
		int bOpen = WXFfplayStart(m_play1);
		if (bOpen) {
			SetTimer(TIME_ID_PLAY1, 1000, nullptr);
		}
	}
}

void CMFCFFPLAYDlg::OnBnClickedSeek0()
{
	// TODO: 在此添加控件通知处理程序代码
	WXFfplaySeek(m_play1, 0);

	WXFfplaySeek(m_play1, 100 * 1000);
}


void CMFCFFPLAYDlg::OnBnClickedPlay1Stop()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play1 != nullptr) {
		WXFfplayStop(m_play1);
		KillTimer(TIME_ID_PLAY1);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSubtitle()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		m_strSubtitle = dlg.GetPathName();
		UpdateData(FALSE);
	}
}


void CMFCFFPLAYDlg::onRotate(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int pos = m_sliderRotate.GetPos();
	CString str;
	str.Format(L"角度 = %d", pos);
	GetDlgItem(IDC_JD)->SetWindowText(str);
	if (m_play4) {
		//WXFfplayRotate(m_play4, pos);
	}
}


void CMFCFFPLAYDlg::onVolume(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int pos = m_sliderVolume.GetPos();
	CString str;
	str.Format(L"音量 = %d", pos);
	GetDlgItem(IDC_VOLUME)->SetWindowText(str);
	if (m_play4) {
		WXFfplaySetVolume(m_play4, pos);
	}
}


void CMFCFFPLAYDlg::onSpeed(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int pos = m_sliderSpeed.GetPos();
	CString str;
	str.Format(L"速度 = %0.2f", pos / 100.0f);
	GetDlgItem(IDC_SPEED)->SetWindowText(str);
	if (m_play4) {
		WXFfplaySpeed(m_play4, pos);
		//WXFfplaySeek(m_play4, WXFfplayGetCurrTime(m_play4));
	}	
}


void CMFCFFPLAYDlg::onLight(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int ld = m_sliderLight.GetPos();
	int dbd = m_sliderDBD.GetPos();
	int bhd = m_sliderBHD.GetPos();

	CString str;
	str.Format(L"亮度 = %d", ld);
	GetDlgItem(IDC_LIGHT)->SetWindowText(str);
	if (m_play4) {
		//WXFfplayPictureQuality(m_play4, ld, dbd, bhd);
	}
}


void CMFCFFPLAYDlg::onDBD(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int ld = m_sliderLight.GetPos();
	int dbd = m_sliderDBD.GetPos();
	int bhd = m_sliderBHD.GetPos();
	CString str;
	str.Format(L"对比度 = %d", dbd);


	GetDlgItem(IDC_DBD)->SetWindowText(str);
	if (m_play4) {
		//WXFfplayPictureQuality(m_play4, ld, dbd, bhd);
	}
}


void CMFCFFPLAYDlg::onBHD(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	UpdateData(TRUE);
	int ld = m_sliderLight.GetPos();
	int dbd = m_sliderDBD.GetPos();
	int bhd = m_sliderBHD.GetPos();
	CString str;
	str.Format(L"饱和度 = %d", bhd);
	GetDlgItem(IDC_BHD)->SetWindowText(str);
	if (m_play4) {
		//WXFfplayPictureQuality(m_play4, ld, dbd, bhd);
	}
}


void CMFCFFPLAYDlg::OnBnClickedVflip()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		m_iVFlip = !m_iVFlip;
		//WXFfplayVFlip(m_play4, m_iVFlip);
	}
}


void CMFCFFPLAYDlg::OnBnClickedHflip()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		m_iHFlip = !m_iHFlip;
		//WXFfplayHFlip(m_play4, m_iHFlip);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSeek1000()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		WXFfplaySeek(m_play4, 1000 * 1000);
	}
}

//Crop 区域不能超出图像区域
//MAC 上 Crop不允许 宽度 或者 高度 等于原来的图像
void CMFCFFPLAYDlg::OnBnClickedButton12()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		//WXFfplayCrop(m_play4,0,0,100,100);
	}
}

WXMEDIA_API  void PngTest(WXCTSTR wszName1, WXCTSTR wszName2, int width, int height);

void CMFCFFPLAYDlg::OnBnClickedButton17()
{

	PngTest(L"1.png", L"1dst22.jpg", 1040, 720);
	PngTest(L"2.png", L"2dst22.jpg", 1040, 720);

	return;

	if (m_play4) {
		WXFfplaySetSubtitle(m_play4, L"a.srt");
		int color = ((rand() % 255) << 16) | ((rand() % 255) << 8) | (rand() % 255);//颜色
		WXFfplaySetSubtitleFont(m_play4, L"宋体", rand() % 40 + rand() % 30, color);//设置 字体名字，默认就用空字符串， 字体大小， 字体颜色
		//WXFfplaySetSubtitleAlpha(m_play4, rand() % 255);//设置透明度
		WXFfplaySetSubtitlePostion(m_play4, rand() % 200);//设置距离视频底部的距离
	}
}


void CMFCFFPLAYDlg::OnBnClickedButton13()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		//WXFfplayCrop(m_play4, 0, 0, 0, 0);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSeek2000()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		WXFfplaySeek(m_play4, 2000 * 1000);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSeek4000()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		WXFfplaySeek(m_play4, 4000 * 1000);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSeek6000()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		WXFfplaySeek(m_play4, 6000 * 1000);
	}
}


void CMFCFFPLAYDlg::OnBnClickedSeeknow()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		WXFfplaySeek(m_play4, WXFfplayGetCurrTime(m_play4));
	}
}


void CMFCFFPLAYDlg::OnBnClickedTest()
{

	WXFfmpegConvertVideoFast(L"a.mp4", L"a.ts");

	AfxMessageBox(L"OK");
	return;

	void* Conv = WXFfmpegParamCreate();
	WXFfmpegParamSetConvertTime(Conv, 0, 5000);
	WXFfmpegConvertVideo(Conv, _T("a.mp4"), _T("01.mp4"),0);

	WXFfmpegParamSetConvertTime(Conv, 5000, 10000);
	WXFfmpegConvertVideo(Conv, _T("a.mp4"), _T("02.mp4"),0);

	WXFfmpegParamSetConvertTime(Conv, 10000, 20000);
	WXFfmpegConvertVideo(Conv, _T("a.mp4"), _T("03.mp4"), 0);

	WXFfmpegParamDestroy(Conv);
	AfxMessageBox(L"TestOK");


}


void CMFCFFPLAYDlg::OnBnClickedReset()
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_play4) {
		m_sliderRotate.SetPos(0);//角度
		m_sliderVolume.SetPos(100);//音量
		m_sliderSpeed.SetPos(100);//速度
		m_sliderLight.SetPos(0);//亮度
		m_sliderDBD.SetPos(50);//饱和度
		m_sliderBHD.SetPos(100);//对比度
		//WXffplayReset(m_play4);
		UpdateData(FALSE);
	}
}

void CMFCFFPLAYDlg::OnBnClickedAirplay()
{
	UpdateData(TRUE);
#if 0
	clock_t t1 = clock();
	WXGetAudioVolumeData((LPCTSTR)m_strInput, data, 1920, 0);
	clock_t t2 = clock();
	int t = t2 - t1;
	CString S;
	S.Format(L"T = %d", t);
	AfxMessageBox(S);
#else
	void* Conv = WXFfmpegParamCreate();
	WXFfmpegConvertAudio(Conv, (LPCTSTR)m_strInput, (LPCTSTR)m_strOutput,0);
	WXFfmpegParamDestroy(Conv);
	AfxMessageBox(L"WXFfmpegConvertAudio");
#endif
}




void CMFCFFPLAYDlg::OnBnClickedChange()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_play4) {
		GetDlgItem(IDC_PIC4)->MoveWindow(10  + 10, 282 + 10, 650 + 10, 430 + 10);
		WXFfplayRefresh(m_play4);
	}
}


void CMFCFFPLAYDlg::OnBnClickedRec()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CMFCFFPLAYDlg::OnBnClickedRecStop()
{
	// TODO: 在此添加控件通知处理程序代码
}
