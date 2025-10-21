
// ScreenCaptureDlg.cpp : 实现文件
//
/*
2017.10.14 确保不同dpi下界面一样
*/


#include "stdafx.h"
#include "ScreenCapture.h"
#include "ScreenCaptureDlg.h"
#include "afxdialogex.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CScreenCaptureDlg 对话框

CScreenCaptureDlg::CScreenCaptureDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SCREENCAPTURE_DIALOG, pParent)
	, m_strDir(_T(""))
	, m_bUseRect(FALSE)
	, m_strWaterMark(_T("Apowersoft"))
	, m_posWaterMarkX(50)
	, m_posWaterMarkY(50)
	, m_posX(100)
	, m_posY(100)
	, m_posW(800)
	, m_posH(800)
	, m_posImageWaterMarkX(0)
	, m_posImageWaterMarkY(0)
	, m_bWaterMark(FALSE)
	, m_iAplhaWaterMark(60)
	, m_iRadius(100)
	, m_bUseMouse(FALSE)
	, m_bMouseHotdot(FALSE)
	, m_iRadiusHotdot(100)
	, m_bWaterMarkImage(FALSE)
	, m_bFollowMouse(FALSE)
	, m_bHw(FALSE)
	, m_bRecordMouse(TRUE)
	, m_bDXGI(TRUE)
	, m_bPreview(FALSE)
    , m_strLog1(_T(""))
    , m_strLog2(_T(""))
	, m_bAEC(FALSE)
	, m_bHighQualityGif(FALSE)
	, m_bMixVideo(FALSE)
	, m_nCameraWndWidth(320)
	, m_bDisableQSV(FALSE)
	, m_bDisableNVENC(FALSE)
	, m_bHighQualityRec(TRUE)
	, m_bTS(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CScreenCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_cbAudioRenderDevice);
	DDX_Control(pDX, IDC_COMBO2, m_cbVideoDeviceType);
	DDX_Control(pDX, IDC_COMBO3, m_cbDisplayDevice);
	DDX_Control(pDX, IDC_COMBO4, m_cbCameraDevice);
	DDX_Control(pDX, IDC_COMBO5, m_cbVideoParamType);
	DDX_Control(pDX, IDC_COMBO6, m_cbBitrate);
	DDX_Control(pDX, IDC_COMBO9, m_cbFps);
	DDX_Text(pDX, IDC_EDIT2, m_strDir);
	DDX_Control(pDX, IDC_COMBO8, m_cbFileType);

	DDX_Control(pDX, IDC_COMBO10, m_cbAudioCaptureDevice);
	DDX_Check(pDX, IDC_CHECK1, m_bUseRect);
	DDX_Text(pDX, IDC_EDIT3, m_strWaterMark);
	DDX_Text(pDX, IDC_EDIT4, m_posWaterMarkX);
	DDV_MinMaxInt(pDX, m_posWaterMarkX, 0, 10000);
	DDX_Text(pDX, IDC_EDIT5, m_posWaterMarkY);
	DDX_Text(pDX, IDC_EDIT6, m_posX);
	DDV_MinMaxInt(pDX, m_posX, 0, 10000);
	DDX_Text(pDX, IDC_EDIT7, m_posY);
	DDV_MinMaxInt(pDX, m_posY, 0, 10000);
	DDX_Text(pDX, IDC_EDIT8, m_posW);
	DDV_MinMaxInt(pDX, m_posW, 0, 10000);
	DDX_Text(pDX, IDC_EDIT9, m_posH);
	DDV_MinMaxInt(pDX, m_posH, 0, 10000);
	DDX_Text(pDX, IDC_EDIT10, m_posImageWaterMarkX);
	DDV_MinMaxInt(pDX, m_posImageWaterMarkX, 0, 10000);
	DDX_Text(pDX, IDC_EDIT11, m_posImageWaterMarkY);
	DDV_MinMaxInt(pDX, m_posImageWaterMarkY, 0, 10000);
	DDX_Check(pDX, IDC_CHECK2, m_bWaterMark);
	DDX_Slider(pDX, IDC_SLIDER3, m_iAplhaWaterMark);
	DDV_MinMaxInt(pDX, m_iAplhaWaterMark, 0, 100);
	DDX_Text(pDX, IDC_RADIUS, m_iRadius);
	DDX_Check(pDX, IDC_USE_MOUSE, m_bUseMouse);
	DDX_Check(pDX, IDC_USE_MOUSE2, m_bMouseHotdot);
	DDX_Text(pDX, IDC_RADIUS2, m_iRadiusHotdot);
	DDX_Check(pDX, IDC_CHECK3, m_bWaterMarkImage);
	DDX_Check(pDX, IDC_CHECK4, m_bFollowMouse);
	DDX_Control(pDX, IDC_COMBO11, m_arrFmt);
	DDX_Check(pDX, IDC_CHECK5, m_bHw);
	DDX_Control(pDX, IDC_COMBO12, m_cbxMode);
	DDX_Control(pDX, IDC_CV1, m_progress1);
	DDX_Control(pDX, IDC_CV2, m_progressFfmpegTask2);
	DDX_Check(pDX, IDC_CHECK6, m_bRecordMouse);

	DDX_Control(pDX, IDC_PROGRESS3, m_volume1);
	DDX_Control(pDX, IDC_PROGRESS4, m_volume2);
	DDX_Control(pDX, IDC_PROGRESS5, m_volume3);
	DDX_Control(pDX, IDC_PROGRESS6, m_volume4);
	DDX_Control(pDX, IDC_PROGRESS7, m_volume5);
	DDX_Control(pDX, IDC_PROGRESS8, m_volume6);
	DDX_Control(pDX, IDC_PROGRESS9, m_volume7);
	DDX_Control(pDX, IDC_PROGRESS10, m_volume8);

	DDX_Check(pDX, IDC_CHECK7, m_bDXGI);
	DDX_Check(pDX, IDC_CHECK8, m_bPreview);
	DDX_Control(pDX, IDC_SLIDER1, m_slider1);
	DDX_Control(pDX, IDC_SLIDER2, m_slider2);
	DDX_Text(pDX, IDC_V1_TXT, m_strLog1);
	DDX_Text(pDX, IDC_V2_TXT, m_strLog2);
	DDX_Control(pDX, IDC_COLORLEFT, m_pictureLeft);
	DDX_Control(pDX, IDC_COLORRIGHT, m_pictureRight);
	DDX_Control(pDX, IDC_COLORMOUSE, m_pictureHotdot);
	DDX_Check(pDX, IDC_AEC, m_bAEC);
	DDX_Check(pDX, IDC_GIF, m_bHighQualityGif);
	DDX_Check(pDX, IDC_MIX, m_bMixVideo);
	DDX_Text(pDX, IDC_EDIT13, m_nCameraWndWidth);
	DDX_Control(pDX, IDC_COMBO7, m_cbTypeCameraPos);
	DDX_Check(pDX, IDC_DISABLE_QSV, m_bDisableQSV);
	DDX_Check(pDX, IDC_DISABLE_NVENC, m_bDisableNVENC);
	DDX_Check(pDX, IDC_HIGH_QUALITY_REC, m_bHighQualityRec);
	DDX_Control(pDX, IDC_COMBO13, m_cbScale);
	DDX_Control(pDX, IDC_COMBO14, m_cbVideoCodec);
	DDX_Control(pDX, IDC_COMBO15, m_cbAudioCodec);
	DDX_Check(pDX, IDC_CHECK10, m_bTS);
	DDX_Control(pDX, IDC_COMBO16, m_cbRec);
}

BEGIN_MESSAGE_MAP(CScreenCaptureDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_FILE, &CScreenCaptureDlg::OnBnClickedFile)
	ON_CBN_SELCHANGE(IDC_COMBO5, &CScreenCaptureDlg::OnCbnSelchangeCombo5)
	ON_CBN_SELCHANGE(IDC_COMBO2, &CScreenCaptureDlg::OnCbnSelchangeCombo2)
	ON_CBN_SELCHANGE(IDC_COMBO8, &CScreenCaptureDlg::OnCbnSelchangeCombo8)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CScreenCaptureDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_START, &CScreenCaptureDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_STOP, &CScreenCaptureDlg::OnBnClickedStop)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CScreenCaptureDlg::OnCbnSelchangeCombo3)
	ON_CBN_SELCHANGE(IDC_COMBO4, &CScreenCaptureDlg::OnCbnSelchangeCombo4)
	ON_CBN_SELCHANGE(IDC_COMBO10, &CScreenCaptureDlg::OnCbnSelchangeCombo10)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CScreenCaptureDlg::OnNMCustomdrawSlider1)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER2, &CScreenCaptureDlg::OnNMCustomdrawSlider2)
	ON_BN_CLICKED(IDC_FONT, &CScreenCaptureDlg::OnBnClickedFont)
	ON_BN_CLICKED(IDC_CHECK1, &CScreenCaptureDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_SELECT_IMAGE, &CScreenCaptureDlg::OnBnClickedSelectImage)
	ON_BN_CLICKED(IDC_CHECK2, &CScreenCaptureDlg::OnBnClickedCheck2)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER3, &CScreenCaptureDlg::OnNMCustomdrawSlider3)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_LEFT, &CScreenCaptureDlg::OnBnClickedLeft)
	ON_BN_CLICKED(IDC_RIGHT, &CScreenCaptureDlg::OnBnClickedRight)
	ON_BN_CLICKED(IDC_USE_MOUSE, &CScreenCaptureDlg::OnBnClickedUseMouse)
	ON_BN_CLICKED(IDC_RIGHT2, &CScreenCaptureDlg::OnBnClickedRight2)
	ON_BN_CLICKED(IDC_USE_MOUSE2, &CScreenCaptureDlg::OnBnClickedUseMouse2)
	ON_BN_CLICKED(IDC_PAUSE, &CScreenCaptureDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_RESUME, &CScreenCaptureDlg::OnBnClickedResume)
	ON_BN_CLICKED(IDC_CAMERA, &CScreenCaptureDlg::OnBnClickedCamera)
	ON_BN_CLICKED(IDC_CHANGERECT, &CScreenCaptureDlg::OnBnClickedChangerect)
	ON_BN_CLICKED(IDC_VIDEOSETTING, &CScreenCaptureDlg::OnBnClickedVideosetting)
	ON_BN_CLICKED(IDC_GETPIC1, &CScreenCaptureDlg::OnBnClickedGetpic1)
	ON_BN_CLICKED(IDC_EGTPIC2, &CScreenCaptureDlg::OnBnClickedEgtpic2)
	ON_BN_CLICKED(IDC_CHECK5, &CScreenCaptureDlg::OnBnClickedCheck5)
	ON_BN_CLICKED(IDC_CHECK6, &CScreenCaptureDlg::OnBnClickedCheck6)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK8, &CScreenCaptureDlg::OnBnClickedCheck8)
END_MESSAGE_MAP()



afx_msg LRESULT CScreenCaptureDlg::OnHotKey(WPARAM wParam, LPARAM lParam) {

	if (wParam == 1001) {
		m_pCapture == nullptr ? OnBnClickedStart() : OnBnClickedStop();
	}

	//用户可在此添加代码          
	return     0;

}



// CScreenCaptureDlg 消息处理程序
// lParam is a pointer to CFont object
static BOOL __stdcall SetChildFont(HWND hwnd, LPARAM lparam)
{
	CFont *pFont = (CFont*)lparam;
	CWnd *pWnd = CWnd::FromHandle(hwnd);
	pWnd->SetFont(pFont);
	return TRUE;
}


BOOL CScreenCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	const char * szG = getenv("me");//不是UTF8

	const wchar_t * wszG = _wgetenv(L"me");

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	WXDeviceInit(L"WXMedia.log");//设备枚举初始化
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	m_volume1.SetRange(0, 100);
	m_volume1.SetPos(0);

	m_volume2.SetRange(0, 100);
	m_volume2.SetPos(0);

	m_volume3.SetRange(0, 100);
	m_volume3.SetPos(0);

	m_volume4.SetRange(0, 100);
	m_volume4.SetPos(0);

	m_volume5.SetRange(0, 100);
	m_volume5.SetPos(0);

	m_volume6.SetRange(0, 100);
	m_volume6.SetPos(0);

	m_volume7.SetRange(0, 100);
	m_volume7.SetPos(0);

	m_volume8.SetRange(0, 100);
	m_volume8.SetPos(0);
	WXSetWaveLength(8);
	WXSetListenWaveLength(8);

	int version = WXGetSystemVersion();

	m_strDir = WXGetGlobalString(L"ExePath");
    m_strDir += L"Videos\\";
    ProcDir(m_strDir);

    wchar_t MyDir22[_MAX_PATH]; //桌面目录
    SHGetSpecialFolderPath(this->GetSafeHwnd(), MyDir22, CSIDL_DESKTOP, 0);

	CString str1 = MyDir22;
	str1 += "\\WX1.jpg";
	m_strJPEG1 = str1;
	CString str2 = MyDir22;
	str2 += "\\WX2.jpg";
	m_strJPEG2 = str2;

	SetTimer(time_id2, 100, NULL);//回调时间

	int sys_version = WXGetSystemVersion();
	HDC hdc = ::GetDC(::GetDesktopWindow());
	int hdpi_int = GetDeviceCaps(hdc, LOGPIXELSX);
	int vdpi_int = GetDeviceCaps(hdc, LOGPIXELSY);
	::ReleaseDC(NULL, hdc);
	CFont *oldfont = GetFont();// pDC->GetCurrentFont();
	LOGFONT lf;
	oldfont->GetLogFont(&lf);
	int fixsize = 144;
	lf.lfWidth = lf.lfWidth  * fixsize / hdpi_int;
	lf.lfHeight = lf.lfHeight * fixsize / hdpi_int;
	CFont newfont;
	newfont.CreateFontIndirect(&lf);
	::EnumChildWindows(m_hWnd, ::SetChildFont, (LPARAM)&newfont);

	if (WXSupportHarewareCodec())m_bHw = TRUE;

	int RenderCount = WXWasapiGetRenderCount();
    if (RenderCount > 0) {
        {
            CString vstr = L"All Speaker";
            m_mapSystemDevice[vstr] = L"conf";
            m_cbAudioRenderDevice.InsertString(0, vstr);
        }
        {
            CString vstr = L"default";
            m_mapSystemDevice[vstr] = L"conf";
            m_cbAudioRenderDevice.InsertString(1, vstr);
        }
        {
            CString vstr = L"disable";
            m_mapSystemDevice[vstr] = L"nullptr";
            m_cbAudioRenderDevice.InsertString(2, vstr);
        }

        for (int i = 0; i < RenderCount; i++) {
            SoundDeviceInfo*info = WXWasapiGetRenderInfo(i);
            CString vstr = info->m_strName;
            if (info->isDefalut) {
                vstr += L"(Default)";
			}
			else if(info->isDefalutComm){
				vstr += L"(Comm)";
			}
            m_cbAudioRenderDevice.InsertString(i+3, vstr);
            m_mapSystemDevice[vstr] = info->m_strGuid;
        }
		m_cbAudioRenderDevice.SetCurSel(0);
    }else { //没有扬声器设备
        {
            CString vstr = L"Disable Speaker";
            m_mapSystemDevice[vstr] = L"nullptr";
            m_cbAudioRenderDevice.InsertString(0, vstr);
			m_cbAudioRenderDevice.SetCurSel(0);
        }
    }




	int CaptureCount = WXWasapiGetCaptureCount();
    if (CaptureCount > 0) {
        {
            CString vstr = L"All Mic";
            m_mapMicDevice[vstr] = L"conf";
            m_cbAudioCaptureDevice.InsertString(0, vstr);
        }
        {
            CString vstr = L"default";
            m_mapMicDevice[vstr] = L"default";
            m_cbAudioCaptureDevice.InsertString(1, vstr);
        }
        {
            CString vstr = L"disable";
            m_mapMicDevice[vstr] = L"nullptr";
            m_cbAudioCaptureDevice.InsertString(2, vstr);
        }
        for (int i = 0; i < CaptureCount; i++) {
            SoundDeviceInfo*info = WXWasapiGetCaptureInfo(i);
            CString vstr = info->m_strName;
            if (info->isDefalut) {
                vstr += L"(Default)";
            }else if (info->isDefalutComm) {
				vstr += L"(Comm)";
			}
            m_cbAudioCaptureDevice.InsertString(i+3, vstr);
            m_mapMicDevice[vstr] = info->m_strGuid;
        }
		m_cbAudioCaptureDevice.SetCurSel(0);
    }else {
        {
            CString vstr = L"Disable Mic";
            m_mapMicDevice[vstr] = L"nullptr";
            m_cbAudioCaptureDevice.InsertString(0, vstr);
        }
		m_cbAudioCaptureDevice.SetCurSel(0);
    }


	m_cbxMode.InsertString(0, L"标清");
	m_cbxMode.InsertString(1, L"高清");
	m_cbxMode.InsertString(2, L"原画");
	m_cbxMode.SetCurSel(2);

	m_cbVideoDeviceType.SetWindowText(L"Video Divice");
	m_cbVideoDeviceType.InsertString(0, L"Disable");
	m_cbVideoDeviceType.InsertString(1, L"Display");
	m_cbVideoDeviceType.InsertString(2, L"Camera");
	m_cbVideoDeviceType.InsertString(3, L"Game");//游戏捕获
	m_cbVideoDeviceType.InsertString(4, L"GameWindow");//游戏捕获
	m_cbVideoDeviceType.InsertString(5, L"Window");//游戏捕获
	m_cbVideoDeviceType.InsertString(6, L"Miracast");//游戏捕获

	m_cbDisplayDevice.EnableWindow(FALSE);
	m_cbCameraDevice.EnableWindow(FALSE);

#if 1
	m_cbVideoDeviceType.SetCurSel(1);
	m_cbDisplayDevice.EnableWindow(TRUE);
	ListScreean();
#else
	m_cbVideoDeviceType.SetCurSel(3); //Game                   
#endif

	m_cbVideoParamType.SetWindowText(L"VideoBitrate");

	m_cbBitrate.EnableWindow(TRUE);
	m_cbBitrate.SetWindowText(L"VideoBitrate(kbps)");
	m_cbBitrate.InsertString(0, L"300");
	m_cbBitrate.InsertString(1, L"450");
	m_cbBitrate.InsertString(2, L"750");
	m_cbBitrate.InsertString(3, L"1000");
	m_cbBitrate.InsertString(4, L"2000");
	m_cbBitrate.InsertString(5, L"4000");
	m_cbBitrate.InsertString(6, L"8000");
	m_cbBitrate.InsertString(7, L"12000");
	m_cbBitrate.InsertString(8, L"0");
	m_cbBitrate.SetCurSel(8);

	m_cbFps.EnableWindow(TRUE);
	m_cbFps.InsertString(0, L"1fps");
	m_cbFps.InsertString(1, L"5fps");
	m_cbFps.InsertString(2, L"10fps");
	m_cbFps.InsertString(3, L"15fps");
	m_cbFps.InsertString(4, L"20fps");
	m_cbFps.InsertString(5, L"24fps");
	m_cbFps.InsertString(6, L"25fps");
	m_cbFps.InsertString(7, L"30fps");
	m_cbFps.InsertString(8, L"60fps");
	m_cbFps.InsertString(9, L"120fps");
	m_cbFps.SetCurSel(5);

	m_cbFileType.InsertString(0,  L".mp4");
	m_cbFileType.InsertString(1,  L".ts");
	m_cbFileType.InsertString(2,  L".flv");
	m_cbFileType.InsertString(3,  L".avi");
	m_cbFileType.InsertString(4,  L".mpeg");
	m_cbFileType.InsertString(5,  L".mpg");
	m_cbFileType.InsertString(6,  L".vob");
	m_cbFileType.InsertString(7,  L".wmv");
	m_cbFileType.InsertString(8,  L".asf");
	m_cbFileType.InsertString(9,  L".gif");//需要RGB8数据格式，而且无音频
	m_cbFileType.InsertString(10, L".aac");//纯音频将禁用视频输出
	m_cbFileType.InsertString(11, L".mp3");//纯音频将禁用视频输出
	m_cbFileType.InsertString(12, L".ogg");//自定义格式
	m_cbFileType.InsertString(13, L".flac");//自定义格式
	m_cbFileType.InsertString(14, L".wav");//自定义格式
	m_cbFileType.InsertString(15, L".xws");//自定义格式
	m_cbFileType.InsertString(16, L".mp4.xws");//自定义格式
	m_cbFileType.InsertString(17, L".wav.xws");//自定义格式
	m_cbFileType.InsertString(18, L".ogg.xws");//自定义格式

	m_cbFileType.InsertString(19, L".avi.xws");//自定义格式
	m_cbFileType.InsertString(20, L".ts.xws");//自定义格式
	m_cbFileType.InsertString(21, L".asf.xws");//自定义格式
	m_cbFileType.InsertString(22, L".wmv.xws");//自定义格式


	m_cbFileType.SetCurSel(20);

	//视频编码器
	m_cbVideoCodec.InsertString(0, L"H264");
	m_cbVideoCodec.InsertString(1, L"MPEG4");
	m_cbVideoCodec.InsertString(2, L"H265");
	m_cbVideoCodec.SetCurSel(0);

	//音频编码器
	m_cbAudioCodec.InsertString(0, L"AAC");
	m_cbAudioCodec.InsertString(1, L"MP3");
	m_cbAudioCodec.SetCurSel(0);

	m_cbScale.InsertString(0, L"100%");//自定义格式
	m_cbScale.InsertString(1, L"50%");//自定义格式
	m_cbScale.InsertString(2, L"200%");//自定义格式
	m_cbScale.SetCurSel(0);

	GetDlgItem(IDC_EDIT6)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT7)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT8)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT9)->EnableWindow(FALSE);

	m_colorText = RGB(255, 0, 0);

	UpdateData(false);
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_PAUSE)->EnableWindow(FALSE);
	GetDlgItem(IDC_RESUME)->EnableWindow(FALSE);

	GetDlgItem(IDC_USE_MOUSE)->EnableWindow(TRUE);
	GetDlgItem(IDC_USE_MOUSE2)->EnableWindow(TRUE);

	m_progress1.SetRange(0, 32767);
	m_progressFfmpegTask2.SetRange(0, 32767);

    m_slider1.SetRange(50, 200);
    m_slider1.SetPos(100);

    m_slider2.SetRange(50, 200);
    m_slider2.SetPos(100);

	AudioDeviceOpen(L"default", AUDIO_DEVIDE_ALL);

	WXGameShowFps(3);
	WXGameShowFpsMode(1);
	//WXGameSetScale(50);

	::RegisterHotKey(m_hWnd, 1001, MOD_CONTROL | MOD_SHIFT , VK_F9);

	//int memory =  WXGetMemory();//获取内存大小，单位G
	//int cpu_num = WXGetCpuNum();//获取CPU数量，双核，4核，6核，
	//int cpu_speed = WXGetCpuSpeed();//获取cpu速度，MHz
	//CString strr;
	//strr.Format(L"Memory=%d G  CpuNum=%d CpuSpeed=%d",memory,cpu_num,cpu_speed);
	//AfxMessageBox(strr);

	int cx = ::GetSystemMetrics(SM_CXSCREEN);
	int cy = ::GetSystemMetrics(SM_CYSCREEN);
	m_posX = (100);
	m_posY = (100);
	m_posW = 762;
	m_posH = 642;

	m_cbTypeCameraPos.InsertString(0, L"RightTop");	
	m_cbTypeCameraPos.InsertString(1, L"RightButtom");
	m_cbTypeCameraPos.InsertString(2, L"LeftButtom");
	m_cbTypeCameraPos.InsertString(3, L"LeftTop");
	m_cbTypeCameraPos.SetCurSel(0);


	m_cbRec.InsertString(0, L"100%");
	m_cbRec.InsertString(1, L"80%");
	m_cbRec.InsertString(2, L"50%"); 
	m_cbRec.SetCurSel(0);

	int nMemory = WXGetGlobalValue(L"Memory");
	int nCpu = WXGetGlobalValue(L"Cpu");
	if (nMemory >= 16 && nCpu >= 8) {
		//内存大于等16G 且 Cpu数量大于等于8
		//可以使用高性能录屏模式
		m_bHighQualityRec = TRUE;
		WXSetGlobalValue(L"MachLevel", LEVEL_BEST);//2
	}
	else {
		//默认处理
		m_bHighQualityRec = FALSE;
		WXSetGlobalValue(L"MachLevel", LEVEL_GOOD);//1
	}
	UpdateData(FALSE);

	//int nBlueTooth = WXGetGlobalValue(L"BlueToothCount");//蓝牙音频设备数量
	//int nHandsFree = WXGetGlobalValue(L"HandsFreeCount");//蓝牙免提音频设备数量
	//WXSetGlobalValue(L"BlueToothMode", BLUETOOTH_DEFAULT);////默认不处理蓝牙
	//if (nBlueTooth == 3 && nHandsFree == 2) { //Win11 以下连接一个蓝牙耳机的情况
	//	CString str = L"是否禁用蓝牙免提设备";
	//	int   ret = ::MessageBox(NULL, str, L"蓝牙提示", MB_OK | MB_OKCANCEL);
	//	if (ret == IDOK) {
	//		WXSetGlobalValue(L"BlueToothMode", BLUETOOTH_STEREO);//禁用蓝牙免提，启用蓝牙立体声
	//	}else if (ret == IDCANCEL) {
	//		WXSetGlobalValue(L"BlueToothMode", BLUETOOTH_HANDSFREE);//启用蓝牙免提，禁用立体声
	//	}
	//}


	//WXSetGlobalString(L"Dir", L"C:\\Z_Disk");//设置输出目录
	//int testDir = WXGetGlobalValue(L"CheckDir"); //检测目录是否可写 1 表示目录可写， 0 表示不可写


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CScreenCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{

	CDialogEx::OnSysCommand(nID, lParam);
	
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CScreenCaptureDlg::OnPaint()
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

        if (m_bRecordMouse) {
            if (m_bUseMouse) {
                {
                    CRect prect;
                    m_pictureLeft.GetClientRect(&prect);   //获取区域
                    FillRect(m_pictureLeft.GetDC()->GetSafeHdc(), &prect, CBrush(m_colorLeft));
                }
                {
                    CRect prect;
                    m_pictureRight.GetClientRect(&prect);   //获取区域
                    FillRect(m_pictureRight.GetDC()->GetSafeHdc(), &prect, CBrush(m_colorRight));
                }
            }
            if (m_bMouseHotdot) {
                CRect prect;
                m_pictureHotdot.GetClientRect(&prect);   //获取区域
                FillRect(m_pictureHotdot.GetDC()->GetSafeHdc(), &prect, CBrush(m_colorMouse));
            }
        }
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CScreenCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CScreenCaptureDlg::ListScreean() {
	GetDlgItem(IDC_COMBO11)->EnableWindow(FALSE);
	WXScreenInit();
	int count = WXScreenGetCount();
	for (int i = 0; i < count; i++) {
		MonitorInfo* info = WXScreenGetInfo(i);
		CString str = info->wszName;
		if (info->isPrimary)str += L"  (Default Display)";
		CString szSize;
		szSize.Format(L"  (%dx%d)", info->width, info->height);
		str += szSize;
		m_cbDisplayDevice.InsertString(i, str);
	}
	//m_cbDisplayDevice.SetCurSel(0);

	m_cbDisplayDevice.InsertString(count, L"all");
	m_cbDisplayDevice.SetCurSel(count);
}

void CScreenCaptureDlg::ListCamera() {
	GetDlgItem(IDC_COMBO11)->EnableWindow(TRUE);
	WXCameraInit();
	int count = WXCameraGetCount();
	if (count > 0) {
		m_cbCameraDevice.ResetContent();
		for (int i = 0; i < count; i++) {
			CameraInfo*info = WXCameraGetInfo(i);
			CString stName = info->m_strName;
			m_cbCameraDevice.InsertString(i, stName);
		}
		m_cbCameraDevice.SetCurSel(0);
		m_cbCameraDevice.EnableWindow(TRUE);
		m_arrFmt.ResetContent();
		CameraInfo*info = WXCameraGetInfo(0);
		if (info) {
			for (int i = 0; i < info->size_fmt; i++) {
				CString str;
				str.Format(L"%dx%d %dfps", info->m_arrFmt[i].width, info->m_arrFmt[i].height, info->m_arrFmt[i].fps);
				m_arrFmt.InsertString(i, str);
			}
			m_arrFmt.SetCurSel(0);
		}
	}
}



struct MyData {
public:
	HWND m_hwnd = NULL;
	DWORD  m_processID = 0;
	DWORD  m_threadID = 0;

	//子窗口的ProcessID 和 ThreadId与父窗口不一致
	HWND m_hwnd2 = NULL;
	DWORD  m_processID2 = 0;
	DWORD  m_threadID2 = 0;
};
BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
{
	MyData *obj = (MyData*)lParam;
	DWORD  _processID = 0;
	DWORD  _threadID = GetWindowThreadProcessId(hwndChild, &_processID);
	if (_processID != obj->m_processID || _threadID != obj->m_threadID) {
		obj->m_hwnd2 = hwndChild;
		obj->m_processID2 = _processID;
		obj->m_threadID2 = _threadID;
	}
	return TRUE;
}
void CScreenCaptureDlg::AddHwnd(HWND hwnd) {
	if (hwnd == GetSafeHwnd()) { //不录制自身
		//AfxMessageBox(L"Dont!");
		return;
	}
	if (::IsIconic(hwnd)) {
		return;
	}
	DWORD  _processID = 0;
	DWORD  _threadID = ::GetWindowThreadProcessId(hwnd, &_processID);
	MyData obj;
	obj.m_hwnd = hwnd;
	obj.m_processID = _processID;
	obj.m_threadID = _threadID;
	EnumChildWindows(hwnd, EnumChildProc, (LPARAM)&obj);

	TCHAR exe_name[MAX_PATH];
	WXGetProcessName(obj.m_hwnd2 ? obj.m_processID2 : obj.m_processID, exe_name);
	if (!WXBlackListedExe(exe_name,FALSE)) {
		TCHAR text_name[MAX_PATH];
		::GetWindowTextW(obj.m_hwnd2 ? obj.m_hwnd : obj.m_hwnd, text_name, MAX_PATH);

		CString str = exe_name;
		str += " - ";
		str += text_name;
		m_mapWindow[hwnd] = str;
	}

}

BOOL CALLBACK EnumWindowProc(HWND hWnd, LPARAM lParam)
{
	//遍历所有子窗口
	if (IsWindow(hWnd) && IsWindowVisible(hWnd)/* && GetWindowLong(hWnd, GWL_HWNDPARENT) != 0*/){
		CScreenCaptureDlg *p = (CScreenCaptureDlg*)lParam;
		p->AddHwnd(hWnd);
	}
	return TRUE;
}



//视频设备选择
void CScreenCaptureDlg::OnCbnSelchangeCombo2()
{
	// TODO: 在此添加控件通知处理程序代码
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(index); //设置成想要索引值的值  
	UpdateData(true);// 来获取关联变量，即改变后的值

	CString str;
	m_cbVideoDeviceType.GetWindowText(str);
	if (str == L"Display") {
		m_cbDisplayDevice.ResetContent();
		m_cbDisplayDevice.EnableWindow(TRUE);
		m_cbCameraDevice.ResetContent();
		m_cbCameraDevice.EnableWindow(FALSE);
		ListScreean();//列举显示器信息
	}
	else if (str == L"Camera") {
		m_cbDisplayDevice.ResetContent();
		m_cbDisplayDevice.EnableWindow(FALSE);
		ListCamera();//列举摄像头信息
	}
	else if (str == L"Game") { //游戏录制
		m_cbDisplayDevice.ResetContent();
		m_cbDisplayDevice.EnableWindow(FALSE);
		m_cbCameraDevice.ResetContent();
		m_cbCameraDevice.EnableWindow(FALSE);
	}else if (str == L"Window" || str == L"GameWindow") { //窗口录制, 也可能是游戏窗口
		m_cbDisplayDevice.ResetContent();
		m_cbDisplayDevice.EnableWindow(FALSE);
		m_cbCameraDevice.ResetContent();
		m_cbCameraDevice.EnableWindow(FALSE);

		//在某个函数里面调用
		//函数功能 该函数枚举所有屏幕上的顶层窗口，并将窗口句柄传送给应用程序定义的回调函数。
		m_mapWindow.clear();
		m_mapData.clear();
		m_cbDisplayDevice.EnableWindow(TRUE);
		::EnumWindows(EnumWindowProc, (LPARAM)this);

		int index = 0;
		for (auto &v : m_mapWindow) {
			m_mapData[index] = v.first;
			m_cbDisplayDevice.InsertString(index++, v.second);
		}
		m_cbDisplayDevice.SetCurSel(0);
	}else { //不选择
		m_cbDisplayDevice.ResetContent();
		m_cbDisplayDevice.EnableWindow(FALSE);
		m_cbCameraDevice.ResetContent();
		m_cbCameraDevice.EnableWindow(FALSE);
	}
}


void CScreenCaptureDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(index); //设置成想要索引值的值  
}

void CScreenCaptureDlg::OnCbnSelchangeCombo10() {
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO10))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO10))->SetCurSel(index); //设置成想要索引值的值  
	UpdateData(true);// 来获取关联变量，即改变后的值
}


//视频编码参数设置选择
void CScreenCaptureDlg::OnCbnSelchangeCombo5() {
	// TODO: 在此添加控件通知处理程序代码
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO5))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO5))->SetCurSel(index); //设置成想要索引值的值  
	UpdateData(true);// 来获取关联变量，即改变后的值
}

void CScreenCaptureDlg::OnCbnSelchangeCombo8()
{
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO8))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO8))->SetCurSel(index); //设置成想要索引值的值  
}

void CScreenCaptureDlg::OnBnClickedFile()
{
	// TODO: 在此添加控件通知处理程序代码
    TCHAR szBuffer[MAX_PATH] = { 0 };
    BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = NULL;
    bi.pszDisplayName = szBuffer;
    bi.lpszTitle = _T("从下面选文件夹目录:");
    bi.ulFlags = BIF_RETURNFSANCESTORS;
    LPITEMIDLIST idl = SHBrowseForFolder(&bi);
    if (NULL == idl) {
        return;
    }
    SHGetPathFromIDList(idl, szBuffer);
    m_strDir = szBuffer;
	m_strDir += L"\\";
    UpdateData(FALSE);
}


//回调函数不要堵塞在UI线程
extern "C" void wxCallBack_(void* cbSink, UINT cbID, void* cbData) {
	CScreenCaptureDlg* pThis = (CScreenCaptureDlg*)cbSink;
	pThis->wxCallBack(cbID, cbData);
}



void CScreenCaptureDlg::onWXCaptureSucceed(CString strFilename) {
	//录制正常结束
	//m_pCapture = nullptr;
	int error = 0;
	void* info = WXMediaInfoCreate(strFilename, &error);
	if (info) {
		int vn = WXMediaInfoHasVideo(info);
		int an = WXMediaInfoHasAudio(info);
		int tn = WXMediaInfoHasAttach(info);
		int64_t time = WXMediaInfoGetFileDuration(info);
		CString str = L"Recoder File \r\n";

		//{

		//	CString s1;
		//	s1.Format(L"\nVideo channel = %d\nAudio channel = %d\nAttach Channel = %d\nduration = %lld ms\n", vn, an, tn, time);
		//	str += s1;
		//}

		if (an) {
			CString s1 = L"Audio Codec=";
			CString s2 = WXMediaInfoGetAudioCodecName(info);
			s1 += s2;
			s2.Format(L"%d Hz %d Channel Bitrate=%ldKBps\n",
				WXMediaInfoGetAudioSampleRate(info),
				WXMediaInfoGetAudioChannels(info),
				(int)(WXMediaInfoGetAudioBitrate(info) / 1000)
			);
			s1 += s2;
			str += s1;
		}

		if (vn) {
			CString s1 = L"Video Codec = ";
			CString s2 = WXMediaInfoGetVideoCodecName(info);
			s1 += s2;
			s2.Format(L"[%dx%d] %dKBps %0.2ffps CompressRate=%d \n",
				(int)WXMediaInfoGetVideoWidth(info),
				(int)WXMediaInfoGetVideoHeight(info),
				(int)(WXMediaInfoGetVideoBitrate(info) / 1000),
				WXMediaInfoGetVideoAvgFps(info),
				(int)WXMediaInfoGetVideoCompress(info)
			);
			s1 += s2;
			str += s1;
		}
		WXMediaInfoDestroy(info);

		m_strOK = str;
		m_bOK = TRUE;
		//AfxMessageBox(str);
	}
}

void CScreenCaptureDlg::StopImpl() {
	WXAutoLock al(m_lock);
	if (m_pCapture != nullptr) {
		//AfxMessageBox(L"WXCaptureStop AAA");
		WXCaptureStop(m_pCapture);
		//AfxMessageBox(L"WXCaptureStop BBB");
	}
}

void CScreenCaptureDlg::wxCallBack(UINT cbID, void* cbData)
{
	CString str;
	int* intValue = 0;
	TCHAR* wszValue = nullptr;

	switch (cbID){
	case WX_EVENT_HOOK_STOP:
		wszValue = (TCHAR*)cbData;
		str = wszValue;
		str += L" HOOK 结束";
		LogHook(str);
		break;
	case WX_EVENT_HOOK_START:
		wszValue = (TCHAR*)cbData;
		str = wszValue;
		str += L" HOOK 成功";
		LogHook(str);
		break;
	case WX_EVENT_HOOK_WIDTH:
		intValue = (int*)cbData;
		m_iGameWidth = *intValue;
		if (m_iGameWidth && m_iGameHeight)
			str.Format(L"Game Size %dx%d", m_iGameWidth, m_iGameHeight);
		LogHook(str);
		break;
	case WX_EVENT_HOOK_HEIGHT:
		intValue = (int*)cbData;
		m_iGameHeight = *intValue;
		if (m_iGameWidth && m_iGameHeight)
			str.Format(L"Game Size %dx%d", m_iGameWidth, m_iGameHeight);
		LogHook(str);
		break;

	case WX_EVENT_ID_CLOSE_FILE:
		
		wszValue = (TCHAR*)cbData;
		str = wszValue;
		onWXCaptureSucceed(str);
		
		m_pCapture = nullptr;
		break;

		//已经弃用
	//case WX_EVENT_ID_WINDOWCAPTURE_NO_DATA:
	//	//onWindowCaptureClose();
	//	StopImpl();
	default:
		break;
	}
}

void CScreenCaptureDlg::OnBnClickedStart() {

	UpdateData(true);
	if (m_pCapture == nullptr) {
		UpdateData(true);

		int nScaleIndex =  m_cbScale.GetCurSel();
		if (nScaleIndex == 0) {
			WXSetGlobalValue(L"Capture_Scale", 100);
		}else if (nScaleIndex == 1) {
			WXSetGlobalValue(L"Capture_Scale", 50);
		}else if (nScaleIndex == 2) {
			WXSetGlobalValue(L"Capture_Scale", 200);
		}

		//是否禁用QSV硬编码
		WXSetGlobalValue(L"DisableQSV",   m_bDisableQSV);

		//是否禁用NVENC硬编码
		WXSetGlobalValue(L"DisableNVENC", m_bDisableNVENC);//是否禁用NVENC硬编码

		if (m_bHighQualityRec) {
			//使用性能优化模式优化游戏录制等
			WXSetGlobalValue(L"MachLevel", LEVEL_BEST);
		}else {
			//使用默认模式
			WXSetGlobalValue(L"MachLevel", LEVEL_GOOD);
		}

		//设置是否使用音频回声消除采集
		WXSetGlobalValue(L"AEC", m_bAEC);

		//设置是否使用高质量GIF，速度较慢
		WXSetGlobalValue(L"HighQualityGif", m_bHighQualityGif);

		TWXCaptureConfig s_param;
        TWXCaptureConfigDefault(&s_param);

        m_strFileName = m_strDir;

        CString strFileType;
        m_cbFileType.GetWindowText(strFileType);

		CString strVideoCodec = L"";
		CString strAudioCodec = L"";

		//标准H264
		if (strFileType == L".mp4" || strFileType == L".mov") {
			m_cbVideoCodec.GetWindowText(strVideoCodec);
			m_cbAudioCodec.GetWindowText(strAudioCodec);
		}

        CTime m_time = CTime::GetCurrentTime();             //获取当前时间日期  
        CString strTime = m_time.Format(_T("ApowerSoft-%Y%m%d-%H%M%S"));   //格式化日期时间
        m_strFileName += strTime;//new file name!!
        m_strFileName += strFileType;//new file name!!

		//m_strFileName = L"rtmp://127.0.0.1/live/k2";
		wcscpy(s_param.m_wszFileName, m_strFileName);

		s_param.m_sink = this;
		s_param.m_cb = wxCallBack_;

		CString strSystem;
		m_cbAudioRenderDevice.GetWindowText(strSystem);
        CString strSystemGuid = m_mapSystemDevice[strSystem];

		CString strMic;
		m_cbAudioCaptureDevice.GetWindowText(strMic);
        CString strMicGuid = m_mapMicDevice[strMic];

        s_param.m_audio.has_audio = 1;
        wcscpy(s_param.m_audio.m_systemName, strSystemGuid);
        wcscpy(s_param.m_audio.m_micName, strMicGuid);
        s_param.m_audio.nSystemScale = m_slider1.GetPos();
        s_param.m_audio.nMicScale    = m_slider2.GetPos();

		s_param.m_audio.nSampleRate = 48000;
		s_param.m_audio.nChannel = 2;

        s_param.m_audio.bAGC = TRUE;
        s_param.m_audio.bNS  = TRUE;
        s_param.m_audio.bVAD = TRUE;


		if (strVideoCodec.GetLength()) {
			wcscpy(s_param.m_video.m_wszCodec, strVideoCodec);
		}

		if (strAudioCodec.GetLength()) {
			wcscpy(s_param.m_audio.codec, strAudioCodec);
		}
		//wcscpy(s_param.m_audio.m_systemName, L"conf");
		//wcscpy(s_param.m_audio.m_micName, L"conf");

		s_param.m_video.m_iForceFps = 0;//

		//fps = 0;
		//Bitrate
		int iVideoBitrate = 0;
		int iFps = 25;
		CString str;
		m_cbVideoDeviceType.GetWindowText(str);
		if (str == "Game" || str == L"GameWindow" || str == "Window" || str == "Display" || str == "Camera") {
			s_param.m_video.m_bUseHW = m_bHw ? 1 : 0;  //硬编码模式
			s_param.m_video.m_bDXGI = m_bDXGI ? 1 : 0;  //DXGI桌面采集，比GDI效率高
			int video_mode = m_cbxMode.GetCurSel();
			if (video_mode < 0)video_mode = 0;
			if (video_mode > 2)video_mode = 2;
			s_param.m_mode = WXCaptureMode(video_mode);//录屏模式

			//视频编码参数设置
			s_param.m_video.m_bUse = 1;

			if (m_cbFps.IsWindowEnabled()) {
				int fps_arr[] = { 1, 5, 10, 15, 20, 24, 25 , 30,60,120 };
				iFps = fps_arr[m_cbFps.GetCurSel()];
			}
			s_param.m_video.m_iFps = iFps;//

			if (m_cbBitrate.IsWindowEnabled()) {
				int bitrate_arr[] = {
					300 * 1000,
					450 * 1000,
					750 * 100,
					1000 * 1000,
					2000 * 1000,
					4000 * 1000,
					8000 * 1000,
					12000 * 1000,
					0
				};
				int k = m_cbBitrate.GetCurSel();
				if (k == -1)k = 6;
				iVideoBitrate = bitrate_arr[k];
			}
			s_param.m_video.m_iBitrate = iVideoBitrate;
			if (m_bWaterMark) {
				//文字水印
				s_param.m_text.m_bUsed = true;
				wcscpy(s_param.m_text.m_wszText, (LPCWSTR)m_strWaterMark);
				//s_param.m_text.m_hfont = m_hfont;
				s_param.m_text.m_iPosX = m_posWaterMarkX;
				s_param.m_text.m_iPosY = m_posWaterMarkY;
				s_param.m_text.m_iFontSize = m_iFontSize * 2;
				s_param.m_text.m_iColor = m_colorText;
				s_param.m_text.m_nStyle = 15;//水印文字样式
				wcscpy(s_param.m_text.m_wszFontName, (LPCWSTR)m_strFontName);
			}

			if ( m_bWaterMarkImage) {
				//图像水印
				s_param.m_image.m_bUsed = true;
				wcscpy(s_param.m_image.m_wszFileName, (LPCWSTR)m_strImageName);
				s_param.m_image.m_fAlpha = m_iAplhaWaterMark / 100.0f;
				s_param.m_image.m_iPosX = m_posImageWaterMarkX;
				s_param.m_image.m_iPosY = m_posImageWaterMarkY;

				//WXCaptureSetWM((LPCWSTR)m_strImageName, 0, 0);
			}

			if (m_bRecordMouse) {
				s_param.m_mouse.m_iUsed = 1;
				//桌面采集的鼠标动画！！
				if (m_bUseMouse) {
					s_param.m_mouse.m_bMouseAnimation = true;
					s_param.m_mouse.m_colorLeft = m_colorLeft;
					s_param.m_mouse.m_colorRight = m_colorRight;
					s_param.m_mouse.m_iAnimationRadius = m_iRadius;
				}

				if (m_bMouseHotdot) {
					s_param.m_mouse.m_bMouseHotdot = true;
					s_param.m_mouse.m_fAlphaHotdot = 0.8f;
					s_param.m_mouse.m_iHotdotRadius = m_iRadiusHotdot;
					s_param.m_mouse.m_colorMouse = m_colorMouse;
				}
			}
			if (m_bUseRect) {
				MonitorInfo*info =  WXScreenGetDefaultInfo();
				m_iScreenW = info->width;
				m_iScreenH = info->height;

				s_param.m_video.m_rcScreen.top = m_posY;
				s_param.m_video.m_rcScreen.left = m_posX;
				s_param.m_video.m_rcScreen.bottom = m_posY + m_posH;
				s_param.m_video.m_rcScreen.right = m_posX + m_posW;
				s_param.m_video.m_bRect = TRUE;
				if (m_bFollowMouse) {
					s_param.m_video.m_bFollowMouse = TRUE;//跟随鼠标
				}
			}
		}
		
		if (str == L"Game") {
			wcscpy(s_param.m_video.m_wszDevName, L"Game");
			s_param.m_video.m_bCamera = 1;//
			s_param.m_video.m_iCameraWidth = WXGameGetWidth() ? WXGameGetWidth() : 1600;//  指定输出宽度
			s_param.m_video.m_iCameraHeight = WXGameGetHeight() ? WXGameGetHeight() : 900;// 指定输出高度
			//s_param.m_video.m_idProcess = 0;
			//s_param.m_video.m_idThread = 0;
		}else if (str == L"GameWindow") { //从窗口选择游戏来录制
			wcscpy(s_param.m_video.m_wszDevName, L"Game");
			s_param.m_video.m_bUse = 1;
			s_param.m_video.m_bCamera = 1;//
			s_param.m_video.m_iCameraWidth = WXGameGetWidth() ? WXGameGetWidth() : 1600;//  指定输出宽度
			s_param.m_video.m_iCameraHeight = WXGameGetHeight() ? WXGameGetHeight() : 900;// 指定输出高度
			//s_param.m_video.m_hwndGame = m_mapData[m_cbDisplayDevice.GetCurSel()];
			s_param.m_video.m_iFps = 60;//
		}else if (str == L"Window") {
			//wcscpy(s_param.m_video.m_wszDevName, L"Window");
			s_param.m_video.m_bCamera = 2;
			s_param.m_video.m_hwndCapture = m_mapData[m_cbDisplayDevice.GetCurSel()];
		}else if (str == "Display") {

			//禁用摄像头叠加处理
			WXSetGlobalValue(L"Capture_CameraWindowWidth", 0);

			if (m_bMixVideo) {
				//增加默认摄像头
				WXCameraInit();
				CameraInfo* info = WXCameraGetInfo(0);
				if (info && info->size_fmt > 0) {

					//打开一个摄像头
					//设置摄像头叠加后的位置， 默认0右上角，1右下角，2左下角，3左上角
					WXSetGlobalValue(L"Capture_CameraType", m_cbTypeCameraPos.GetCurSel());
					
					//设置摄像头叠加后的宽度，0 表示禁用
					WXSetGlobalValue(L"Capture_CameraWindowWidth", m_nCameraWndWidth);
				}
			}

			//屏幕录制
			s_param.m_video.m_bCamera = 0;
			
			int count = WXScreenGetCount();
			int index = m_cbDisplayDevice.GetCurSel();
			if (index < count) {
				MonitorInfo* info = WXScreenGetInfo(index);
				wcscpy(s_param.m_video.m_wszDevName, info->wszName);
			}
			else {
				wcscpy(s_param.m_video.m_wszDevName, L"all");
			}
		}else if (str == "Camera") {
			//摄像头录制
			CameraInfo *info = WXCameraGetInfo(m_cbCameraDevice.GetCurSel());
			if (info && info->size_fmt > 0) {
				s_param.m_video.m_bCamera = 1;
				wcscpy(s_param.m_video.m_wszDevName, L"CAMERA"/*info->m_strGuid*/);

				s_param.m_video.onVideoData = nullptr;//数据回调

				int index = m_arrFmt.GetCurSel();
				if (index < 0)index = 0;
				s_param.m_video.m_iFps = info->m_arrFmt[index].fps;
				s_param.m_video.m_iCameraWidth = info->m_arrFmt[index].width;
				s_param.m_video.m_iCameraHeight = info->m_arrFmt[index].height;
			}
		}
		else if (str == "Miracast" && m_pMiracastPlay) { //Miracast

			wcscpy(s_param.m_video.m_wszDevName, L"AIRPLAY");
			s_param.m_video.m_bUse = 1;
			s_param.m_video.m_bCamera = 1;//
			s_param.m_video.m_iCameraWidth = WXGetNetStreamWidth();//  指定输出宽度
			s_param.m_video.m_iCameraHeight = WXGetNetStreamHeight();// 指定输出高度
		}
		int error = 0;

		int indexScale = m_cbRec.GetCurSel();
		if(indexScale == 0)
			s_param.nVideoScale = 100;
		else if (indexScale == 1)
			s_param.nVideoScale = 80;
		else if (indexScale == 2)
			s_param.nVideoScale = 50;
		//s_param.m_video.m_iForceFps = 1;//CFR
		m_pCapture = WXCaptureStartExt2(&s_param, &error);

		CString XX;

		if (error & WX_ERROR_OPEN_FILE)XX += L"Crate File Failed\r\n";
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

		Log(XX);
		if (m_pCapture) {
			UpdateData(FALSE);

			GetDlgItem(IDC_START)->EnableWindow(FALSE);
			GetDlgItem(IDC_STOP)->EnableWindow(TRUE);
			GetDlgItem(IDC_PAUSE)->EnableWindow(TRUE);
			GetDlgItem(IDC_RESUME)->EnableWindow(FALSE);
			m_ptsStart = WXGetTimeMs();
			SetTimer(time_id1, 50, NULL);//回调时间
		}
	}
}

void CScreenCaptureDlg::OnBnClickedChangerect()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_pCapture) {
		WXCaptureChangeRect2(m_pCapture, m_posX, m_posY, m_posW, m_posH);
	}
}



void CScreenCaptureDlg::OnBnClickedStop()
{
	StopImpl();
}


//选择显示器
void CScreenCaptureDlg::OnCbnSelchangeCombo3()
{
	// TODO: 在此添加控件通知处理程序代码
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO3))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(index); //设置成想要索引值的值  
	UpdateData(true);// 来获取关联变量，即改变后的值
}

//选择摄像头
void CScreenCaptureDlg::OnCbnSelchangeCombo4() {
	// TODO: 在此添加控件通知处理程序代码
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO4))->GetCurSel(); //获得想要的索引值  
	((CComboBox*)GetDlgItem(IDC_COMBO4))->SetCurSel(index); //设置成想要索引值的值  
	UpdateData(true);// 来获取关联变量，即改变后的值
	m_arrFmt.ResetContent();
	CameraInfo*info = WXCameraGetInfo(index);
	if (info) {
		for (int i = 0; i < info->size_fmt; i++) {
			CString str;
			str.Format(L"%dx%d %dfps", info->m_arrFmt[i].width, info->m_arrFmt[i].height, info->m_arrFmt[i].fps);
			m_arrFmt.InsertString(i, str);
		}
		m_arrFmt.SetCurSel(0);
	}
}

void CScreenCaptureDlg::OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
    UpdateData(TRUE);
    int pos = m_slider1.GetPos();
    m_strLog1.Format(_T("Speed=%d"), pos);
    UpdateData(FALSE);
	if (nullptr != m_pCapture) {
        WXCaptureSetAudioScale(m_pCapture, TRUE, pos);
    }
}

void CScreenCaptureDlg::OnNMCustomdrawSlider2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
    UpdateData(TRUE);
    int pos = m_slider2.GetPos();
    m_strLog2.Format(_T("Speed=%d"), pos);
    UpdateData(FALSE);
    if (nullptr != m_pCapture) {
        WXCaptureSetAudioScale(m_pCapture, FALSE, pos);
    }
}


void CScreenCaptureDlg::OnBnClickedFont()
{

	//WXConvertDelay(L"D:\\z.mp4",L"D:\\z+30.mp4",30 * 1000);
	//WXConvertDelay(L"D:\\z.mp4", L"D:\\z-40.mp4", - 40 * 1000);
	//AfxMessageBox(L"OK");
	//return;
	////"C:\ATV.mp4"
	//int64_t tsArray[6] = { 0,10 * 1000, 30 * 1000,50 * 1000, 80*1000, 150 *1000};

	//WXMediaFastTrimAndMerge(L"C:\\Users\\momo\\Desktop\\Release.mp4",
	//	
	//	L"C:\\Users\\momo\\Desktop\\Release000.mp4", 3,tsArray);
	//AfxMessageBox(L"OK");
	//return;
//	WXCTSTR arr1[23] = {
//		L"C:\\Users\\tam\\Desktop\\ts\\0-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\1-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\2-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\3-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\4-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\5-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\6-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\7-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\8-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\9-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\10-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\11-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\12-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\13-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\14-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\15-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\16-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\17-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\18-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\19-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\20-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\21-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//		L"C:\\Users\\tam\\Desktop\\ts\\22-03000B01005C53C622594BF389BB83D929DA39-78F3-45AB-8345-4E8D0F906C20.mp4.ts.ts",
//	};
////	WXFfmpegConvertMergerTsFiles(L"C:\\Users\\tam\\Desktop\\ts\\AAA.mp4", 23, arr1);
//
//	//WXCTSTR arr2[2] = { L"2.mp4", L"1.mp4" };
//	//WXFfmpegMergerFiles(L"BBB.mp4", 2, arr2);
//	AfxMessageBox(L"OK");
//	return;

	//H264ProcessDestroy(m_pH264);
	//m_pH264 = NULL;
	//return;

	//int w = 1920 * 1080 * 4;
	//int s = 1920 * 1080;
	//uint8_t *buf = new uint8_t[w];
	//int color = 0xff00ff00;
	//return;

	CFontDialog dlg;
	if (IDOK == dlg.DoModal()) {
		if (m_font.m_hObject)    // 如果m_font对象已经和某个字体资源相关联，要先释放这个资源，之后才能和新的资源进行关联，否则会报错
			m_font.DeleteObject();
		m_font.CreateFontIndirectW(dlg.m_cf.lpLogFont);
		m_hfont = (HFONT)m_font;
		m_colorText = dlg.GetColor(); //字体颜色
	}
}


void CScreenCaptureDlg::OnBnClickedSelectImage() {

	//std::thread th([this] {
	//	//AfxMessageBox(L"Start!!");

	//	WXMediaMergerFileReset(); //参数结构体初始化
	//	WXMediaMergerFileAdd(L"D:\\中桥带ASR12.AVI");
	//	WXMediaMergerFileAdd(L"D:\\20210128_165758 (2).mp4");
	//	WXMediaMergerFileProcess(L"D:\\output1.mp4", TRUE, MODE_NORMAL,640,480,24);//开始转化！

	//	WXMediaMergerFileReset(); //参数结构体初始化
	//	WXMediaMergerFileAdd(L"D:\\中桥带ASR12.AVI");
	//	WXMediaMergerFileAdd(L"D:\\20210128_165758 (2).mp4");
	//	WXMediaMergerFileProcess(L"D:\\output2.mp4", TRUE, MODE_NORMAL, 640, 480, 24);//开始转化！

	//	WXMediaMergerFileReset(); //参数结构体初始化
	//	WXMediaMergerFileAdd(L"D:\\中桥带ASR12.AVI");
	//	WXMediaMergerFileAdd(L"D:\\20210128_165758 (2).mp4");
	//	WXMediaMergerFileProcess(L"D:\\output3.mp4", TRUE, MODE_NORMAL, 640, 480, 24);//开始转化！

	//	WXMediaMergerFileReset(); //参数结构体初始化
	//	WXMediaMergerFileAdd(L"D:\\中桥带ASR12.AVI");
	//	WXMediaMergerFileAdd(L"D:\\20210128_165758 (2).mp4");
	//	WXMediaMergerFileProcess(L"D:\\output4.mp4", TRUE, MODE_NORMAL, 640, 480, 24);//开始转化！

	//	WXMediaMergerFileReset(); //参数结构体初始化
	//	WXMediaMergerFileAdd(L"D:\\中桥带ASR12.AVI");
	//	WXMediaMergerFileAdd(L"D:\\20210128_165758 (2).mp4");
	//	WXMediaMergerFileProcess(L"D:\\output5.mp4", TRUE, MODE_NORMAL, 640, 480, 24);//开始转化！


	//	AfxMessageBox(L"Stop!!"); });
	//th.detach();

	//return;

	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		CString sName = dlg.GetPathName();

		m_strImageName = sName.GetBuffer(sName.GetLength());

		WXVideoRenderCheck(GetDlgItem(IDC_PIC)->GetSafeHwnd(), RENDER_TYPE_D3DX, m_strImageName);
		//WXDrawA(GetDlgItem(IDC_PIC)->GetSafeHwnd(), sName);

	}
	else {
		WXVideoRenderCheck(GetDlgItem(IDC_PIC)->GetSafeHwnd(), RENDER_TYPE_D3DX, NULL);
	}
}




void  CScreenCaptureDlg::LogTime() {

	if (m_pCapture) {
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

		str.Format(L"TotalSize1=%0.3fMb \r\n",(double)TotalSize1 / 1024.0 / 1024.0);
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
		Log(StrLog);
	}
}


void CScreenCaptureDlg::Log(CString str) {
	GetDlgItem(IDC_LOG)->SetWindowText(str);
}

void CScreenCaptureDlg::LogHook(CString str) {
	this->SetWindowText(str);
}

void CScreenCaptureDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (nIDEvent == time_id1) {
		LogTime();
		int pWave[8] = { 0 };
		int ret = WXGetWaveData(pWave);
		if (ret) {
			m_volume1.SetPos(pWave[0] * 100 / 32768);
			m_volume2.SetPos(pWave[1] * 100 / 32768);
			m_volume3.SetPos(pWave[2] * 100 / 32768);
			m_volume4.SetPos(pWave[3] * 100 / 32768);
			m_volume5.SetPos(pWave[4] * 100 / 32768);
			m_volume6.SetPos(pWave[5] * 100 / 32768);
			m_volume7.SetPos(pWave[6] * 100 / 32768);
			m_volume8.SetPos(pWave[6] * 100 / 32768);
		}

		
		if (m_pCapture) {
			int bStopFlag = FALSE;
			bStopFlag = WXCaptureGetStopFlag();//检查当前录制是否需要停止
			if (bStopFlag) {
				int yyy = 0;
			}
			if (!bStopFlag && WXGetTimeMs() - m_ptsStart >= 60 * 60 * 1000) {
				bStopFlag = TRUE;
			}
			if (bStopFlag) {
				KillTimer(time_id1);//录制正常结束
				StopImpl();//一分钟限制后强制结束
			}
		}

	}else if (nIDEvent == time_id2) {
		int v1 = AudioDeviceGetVolume(1);
		m_progress1.SetPos(v1);
		int v0 = AudioDeviceGetVolume(0);
		m_progressFfmpegTask2.SetPos(v0);

		int pWave[8] = { 0 };
		int ret = WXGetSystemData(pWave);
		if (ret) {
			m_volume1.SetPos(pWave[0] * 100 / 32768);
			m_volume2.SetPos(pWave[1] * 100 / 32768);
			m_volume3.SetPos(pWave[2] * 100 / 32768);
			m_volume4.SetPos(pWave[3] * 100 / 32768);
			m_volume5.SetPos(pWave[4] * 100 / 32768);
			m_volume6.SetPos(pWave[5] * 100 / 32768);
			m_volume7.SetPos(pWave[6] * 100 / 32768);
			m_volume8.SetPos(pWave[6] * 100 / 32768);
		}

		if (m_pCapture == nullptr) {
			GetDlgItem(IDC_START)->EnableWindow(TRUE);
			GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
			GetDlgItem(IDC_PAUSE)->EnableWindow(FALSE);
			GetDlgItem(IDC_RESUME)->EnableWindow(FALSE);
		}

		if (m_bOK) {
			Log(m_strOK);
			m_bOK = FALSE;
		}

	}
	__super::OnTimer(nIDEvent);
}


void CScreenCaptureDlg::OnBnClickedCheck1()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_bUseRect) {
		GetDlgItem(IDC_EDIT6)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT7)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT8)->EnableWindow(TRUE);
		GetDlgItem(IDC_EDIT9)->EnableWindow(TRUE);
	}
	else {
		GetDlgItem(IDC_EDIT6)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT7)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT8)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT9)->EnableWindow(FALSE);
	}
}

void CScreenCaptureDlg::OnBnClickedCheck2()
{
	UpdateData(TRUE);
}

void CScreenCaptureDlg::OnNMCustomdrawSlider3(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = 0;
	UpdateData(TRUE);
	CString str;
	str.Format(L"Alpha=%3.2f", m_iAplhaWaterMark / 100.0);
	GetDlgItem(IDC_TXT_ALPHA)->SetWindowText(str);
}

void CScreenCaptureDlg::OnBnClickedLeft() {
	CColorDialog m_setClrDlg;
	m_setClrDlg.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;   // CC_RGBINIT可以让上次选择的颜色作为初始颜色显示出来
	m_setClrDlg.m_cc.rgbResult = m_colorLeft;        //记录上次选择的颜色
	if (IDOK == m_setClrDlg.DoModal()) {
		m_colorLeft = m_setClrDlg.m_cc.rgbResult;            // 保存用户选择的颜色
        Invalidate();
	}
}


void CScreenCaptureDlg::OnBnClickedRight() {
	CColorDialog m_setClrDlg;
	m_setClrDlg.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;   // CC_RGBINIT可以让上次选择的颜色作为初始颜色显示出来
	m_setClrDlg.m_cc.rgbResult = m_colorRight;        //记录上次选择的颜色
	if (IDOK == m_setClrDlg.DoModal()) {
		m_colorRight = m_setClrDlg.m_cc.rgbResult;            // 保存用户选择的颜色
        Invalidate();
	}
}

void CScreenCaptureDlg::OnBnClickedRight2() {
	CColorDialog m_setClrDlg;
	m_setClrDlg.m_cc.Flags |= CC_FULLOPEN | CC_RGBINIT;   // CC_RGBINIT可以让上次选择的颜色作为初始颜色显示出来
	m_setClrDlg.m_cc.rgbResult = m_colorMouse;        //记录上次选择的颜色
	if (IDOK == m_setClrDlg.DoModal()) {
		m_colorMouse = m_setClrDlg.m_cc.rgbResult;            // 保存用户选择的颜色
        Invalidate();
	}
}

void CScreenCaptureDlg::OnBnClickedUseMouse()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(true);
	if (m_bUseMouse) {
        Invalidate();
	}

}


void CScreenCaptureDlg::OnBnClickedUseMouse2()
{
	UpdateData(true);
	if (m_bMouseHotdot) {
        Invalidate();
	}
}

void CScreenCaptureDlg::OnBnClickedPause()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pCapture) {
		WXCapturePause(m_pCapture);
		GetDlgItem(IDC_PAUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RESUME)->EnableWindow(TRUE);
	}
}

void CScreenCaptureDlg::OnBnClickedResume()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pCapture) {
		WXCaptureResume(m_pCapture);
		GetDlgItem(IDC_PAUSE)->EnableWindow(TRUE);
		GetDlgItem(IDC_RESUME)->EnableWindow(FALSE);
	}
}




void CScreenCaptureDlg::OnBnClickedCamera() {
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	CString strType;
	m_cbVideoDeviceType.GetWindowText(strType);
	if (strType == L"Camera") { //摄像头预览

		if (m_pVideoDevice) {
			WXCameraClose(m_pVideoDevice);
			m_pVideoDevice = nullptr;

			void* temp = WXCameraGetCurrDevice();
			int xx = 0;
		}
		else {
			CameraInfo* info = WXCameraGetInfo(m_cbCameraDevice.GetCurSel());
			if (info) {
				if (info->size_fmt > 0) {
					int index = m_arrFmt.GetCurSel();
					if (index < 0)index = 0;
					m_pVideoDevice = WXCameraGetCurrDevice();//查询当前摄像头设备
					if (m_pVideoDevice != nullptr) {
						WXCameraClose(m_pVideoDevice);
						m_pVideoDevice = nullptr;
					}

					//m_pVideoDevice =   //打开设备，可通过WXCameraGetCurrDevice查询是否成功
					m_pVideoDevice = WXCameraOpenWithHwndExt(info->m_strGuid,
						info->m_arrFmt[index].width,
						info->m_arrFmt[index].height,
						info->m_arrFmt[index].fps,
						GetDlgItem(IDC_PIC)->GetSafeHwnd(), TRUE, TRUE);//0为填充窗体，1 为保持比例，有可能留边

					if (m_pVideoDevice != nullptr) {
						//GetDlgItem(IDC_COMBO2)->EnableWindow(FALSE);
						WXCameraSetDrawCamera(m_pVideoDevice, 1, 100, 200, 400, 100);
					}
				}
			}
		}

	}
	else if (strType == L"Game") { //游戏预览
		if (m_bGameOpen == FALSE) {
			HWND hwndPreview = GetDlgItem(IDC_PIC)->GetSafeHwnd();
			WXGameCreate(this, wxCallBack_);
			WXGameSetPreview(hwndPreview);
			WXGameStart();
			SetWindowText(L"WXGameStart OK");
			m_bGameOpen = TRUE;
		}
		else {
			WXGameStop();
			SetWindowText(L"WXGameStop OK");
			m_bGameOpen = FALSE;
			m_bGameWindowOpen = FALSE;
		}
	}
	else if (strType == L"GameWindow") { //游戏预览
		if (m_bGameWindowOpen == FALSE) {
			HWND hwndPreview = GetDlgItem(IDC_PIC)->GetSafeHwnd();
			HWND hwndCapture = m_mapData[m_cbDisplayDevice.GetCurSel()];
			WXGameCreateByWindow(hwndCapture, this, wxCallBack_);
			WXGameSetPreview(hwndPreview);
			WXGameStart();
			SetWindowText(L"WXGameStart OK");
			m_bGameWindowOpen = TRUE;
		}
		else {
			WXGameStop();
			SetWindowText(L"WXGameStop OK");
			m_bGameOpen = FALSE;
			m_bGameWindowOpen = FALSE;
		}
	}
	else if (strType == L"Miracast") { //Miracast预览

		if (m_pMiracastPlay == nullptr) {
			HWND hwndPreview = GetDlgItem(IDC_PIC)->GetSafeHwnd();
			m_pMiracastPlay = WXFfplayCreate(L"FFPLAY", L"rtp://127.0.0.1:62853", 100, 0);
			WXFfplaySetView(m_pMiracastPlay, hwndPreview);
			SetWindowText(L"Miracast OK");
			GetDlgItem(IDC_COMBO2)->EnableWindow(FALSE);
			WXFfplayStart(m_pMiracastPlay);
		}
		else {
			WXFfplayStop(m_pMiracastPlay);
			WXFfplayDestroy(m_pMiracastPlay);
			m_pMiracastPlay = nullptr;
		}
	}
}

void CScreenCaptureDlg::OnBnClickedVideosetting()
{
	// TODO: 在此添加控件通知处理程序代码
	WXCameraSetting(m_pVideoDevice, GetSafeHwnd());
}

//WXMEDIA_API void WXSaveBmp32(WXCTSTR wszName);

void CScreenCaptureDlg::OnBnClickedGetpic1() {

	CFileDialog dlg(TRUE);
	if (IDOK == dlg.DoModal()) {
		CString sName = dlg.GetPathName();
		//m_strImageName = sName.GetBuffer(sName.GetLength());
		//WXSaveBmp32(sName);
	}
	return;

	if (m_pCapture) {
		WXCaptureGetPicture1(m_pCapture, (LPCWSTR)m_strJPEG1, 100);
		WXCaptureGetPicture2(m_pCapture, (LPCWSTR)m_strJPEG2, 70);
	}
	else {
		static int pos = 0;
		pos = pos % 5;
		WXGameShowFps(pos);
		pos++;

		WXGameSetScale(20);
	}
}


void CScreenCaptureDlg::OnBnClickedEgtpic2() {
	//if (m_pCapture) {
	//	WXCaptureGetPicture2(m_pCapture, (LPCWSTR)m_strJPEG2, 70);
	//}
    UpdateData(TRUE);
    ::ShellExecute(NULL, L"explore", m_strDir, NULL, NULL, SW_SHOWDEFAULT);
}


void CScreenCaptureDlg::OnBnClickedCheck5()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	//GetDlgItem(IDC_COMBO12)->EnableWindow(!m_bHw);
}


void CScreenCaptureDlg::OnBnClickedCheck6()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (!m_bRecordMouse) {
		GetDlgItem(IDC_USE_MOUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_USE_MOUSE2)->EnableWindow(FALSE);
	}else {
		GetDlgItem(IDC_USE_MOUSE)->EnableWindow(TRUE);
		GetDlgItem(IDC_USE_MOUSE2)->EnableWindow(TRUE);
        Invalidate();
	}
}



void CScreenCaptureDlg::OnDestroy()
{
	::UnregisterHotKey(m_hWnd, 1001);
	CDialogEx::OnDestroy();
	// TODO: 在此处添加消息处理程序代码
}





void CScreenCaptureDlg::OnBnClickedCheck8()
{
	// TODO: 在此添加控件通知处理程序代码
}
