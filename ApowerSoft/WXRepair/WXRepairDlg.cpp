
// WXRepairDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "WXRepair.h"
#include "WXRepairDlg.h"
#include "afxdialogex.h"

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


// CWXRepairDlg 对话框



CWXRepairDlg::CWXRepairDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WXREPAIR_DIALOG, pParent)
	, m_strInput(_T(""))
	, m_strOutput(_T(""))
	, m_nDelay(0)
	, m_nTime(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWXRepairDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strInput);
	DDX_Text(pDX, IDC_EDIT2, m_strOutput);
	DDX_Text(pDX, IDC_EDIT3, m_nDelay);
	DDX_Text(pDX, IDC_EDIT4, m_nTime);
}

BEGIN_MESSAGE_MAP(CWXRepairDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CWXRepairDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CWXRepairDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_DELAY_VIDEO, &CWXRepairDlg::OnBnClickedDelayVideo)
	ON_BN_CLICKED(IDC_DELAY_AUDIO, &CWXRepairDlg::OnBnClickedDelayAudio)
END_MESSAGE_MAP()


// CWXRepairDlg 消息处理程序

BOOL CWXRepairDlg::OnInitDialog()
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

	// TODO: 在此添加额外的初始化代码

	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	(_tcsrchr(szFilePath, _T('\\')))[1] = 0;
	//s_strPath = szFilePath;

	CString strDoc = szFilePath;//MyDirDoc;
	strDoc += L"WX.log";//我的文档目录


	WXDeviceInit(strDoc);//设备枚举初始化
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWXRepairDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWXRepairDlg::OnPaint()
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
HCURSOR CWXRepairDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWXRepairDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(TRUE);
	if (dlg.DoModal() == IDOK) {
		m_strInput = dlg.GetPathName();

		std::wstring strName = m_strInput;
		std::wstring strNewName;
		// 查找最后一个 '.' 的位置
		size_t dotPos = strName.find_last_of(L'.');

		// 提取后缀名
		std::wstring suffix = strName.substr(dotPos + 1);

		// 不区分大小写判断是否为mp4
		if (wcsicmp(suffix.c_str(), L"xws")==0 || wcsicmp(suffix.c_str(), L"wxm") == 0) {
			// 替换后缀为 .flv
			strNewName = strName.substr(0, dotPos + 1) + L"mp4";
			m_strOutput = strNewName.c_str();
		}

		UpdateData(FALSE);
	}
}


void CWXRepairDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog dlg(FALSE);
	if (dlg.DoModal() == IDOK) {
		m_strOutput = dlg.GetPathName();
		UpdateData(FALSE);
	}
}


void CWXRepairDlg::OnBnClickedDelayVideo()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_nTime > 0) {
		int ret = WXConvertFast2(m_strInput, m_strOutput, m_nTime);
		if (ret ) {
			AfxMessageBox(L"WXConvertFast2 OK");
		}
		else {
			AfxMessageBox(L"WXConvertFast2 ERROR");
		}
	}
	else {
		int ret = WXConvertDelay(m_strInput, m_strOutput, -m_nDelay);
		//int ret = WXConvertFast(m_strInput, m_strOutput);
		if (ret > 0) {
			AfxMessageBox(L"WXConvertDelay OK");
		}
		else {
			AfxMessageBox(L"WXConvertDelay ERROR");
		}
	}

}


void CWXRepairDlg::OnBnClickedDelayAudio()
{
	UpdateData(TRUE);
	int ret = WXConvertDelay(m_strInput, m_strOutput, m_nDelay);
	if (ret) {
		AfxMessageBox(L"OK");
	}
	else {
		AfxMessageBox(L"ERROR");
	}
}
