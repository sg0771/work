
// ScreenCaptureDlg.h : 头文件
//

#pragma once

#include "afxwin.h"
#define USE_HUD 0
#include <WXMedia/WXMedia.h>
#include <map>
#include <direct.h>
#include <io.h>
#include "afxcmn.h"

#include <WXBase.h>

#pragma comment(lib,"WXMedia.lib")
#pragma comment(lib,"libffmpeg.lib")

#define WX_CLOSE (WM_USER + 100)   //结束回调消息
#define WX_GAME_MSG		(WM_USER + 1001)

// CScreenCaptureDlg 对话框
class CScreenCaptureDlg : public CDialogEx
{
	BOOL m_bOK = FALSE;
	CString m_strOK;
public:
	void onWXCaptureSucceed(CString strFilename); //录制成功消息
	void wxCallBack(UINT cbID, void* cbData);//录屏消息回调

	void* m_pMiracastPlay = nullptr;

	std::map<CString, CString>m_mapSystemDevice;
	std::map<CString, CString>m_mapMicDevice;

	int m_iScreenW = 0;
	int m_iScreenH = 0;

	int m_iGameWidth = 0;
	int m_iGameHeight = 0;

	WXLocker m_lock;
	void StopImpl();
	uint64_t m_ptsStart = 0;
	const UINT_PTR time_id1 = 1001;//录制开启和结束
	const UINT_PTR time_id2 = 1002;	// 音波条测试


	BOOL m_bGameOpen = FALSE;
	BOOL m_bGameWindowOpen = FALSE;
	void* m_pVideoDevice = nullptr;//是否启动摄像头

	CString m_strJPEG1 = _T("1.jpg");
	CString m_strJPEG2 = _T("2.jpg");

public:

	std::map<HWND, CString>m_mapWindow;//桌面的所有窗口
	std::map<int, HWND>m_mapData;
	void AddHwnd(HWND hwnd);

	CString m_strLog;

	uint32_t m_colorLeft = RGB(255, 0, 0);
	uint32_t m_colorRight = RGB(0, 255, 0);
	uint32_t m_colorMouse = RGB(0, 0, 255);

	CFont m_font;
	HFONT  m_hfont = nullptr;

	uint32_t m_colorText = RGB(255, 0, 0);//字体颜色
	CString  m_strFontName = _T("Microsoft YaHei");
	int      m_iFontSize = 72;

	CString m_strImageName = L"";//水印文件名

	//int64_t m_pts = 0;
	void LogTime();


	// 构造
public:
	CScreenCaptureDlg(CWnd* pParent = NULL);	// 标准构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCREENCAPTURE_DIALOG };
#endif
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:

	void ProcDir(const wchar_t* wszDir) {
		int iSize;
		//返回接受字符串所需缓冲区的大小，已经包含字符结尾符'\0'
		iSize = WideCharToMultiByte(CP_ACP, 0, wszDir, -1, NULL, 0, NULL, NULL); //iSize =wcslen(pwsUnicode)+1=6
		char* szDir = (char*)malloc(iSize * sizeof(char)); //不需要 pszMultiByte = (char*)malloc(iSize*sizeof(char)+1);
		WideCharToMultiByte(CP_ACP, 0, wszDir, -1, szDir, iSize, NULL, NULL);

		if (_access(szDir, 0) == -1)
		{
			_mkdir(szDir);
		}
		free(szDir);
	}

	CComboBox m_cbAudioRenderDevice;
	CComboBox m_cbAudioCaptureDevice;
	CComboBox m_cbVideoDeviceType;
	CComboBox m_cbDisplayDevice;
	CComboBox m_cbCameraDevice;
	CComboBox m_cbVideoParamType;
	CComboBox m_cbBitrate;
	CComboBox m_cbFps;

	CString m_strDir;// 输出文件夹
	CString m_strFileName;//输出文件名

	CComboBox m_cbFileType;
	void ListScreean();
	void ListCamera();
	afx_msg void OnBnClickedFile();
	afx_msg void OnCbnSelchangeCombo2();
	afx_msg void OnCbnSelchangeCombo5();;
	afx_msg void OnCbnSelchangeCombo8();

	void* m_pCapture = nullptr;

	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedStop();
	afx_msg void OnCbnSelchangeCombo3();
	afx_msg void OnCbnSelchangeCombo4();
	CString m_strV1;
	CString m_strV2;

	afx_msg void OnCbnSelchangeCombo10();

	afx_msg void OnNMCustomdrawSlider1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMCustomdrawSlider2(NMHDR* pNMHDR, LRESULT* pResult);

	BOOL m_bUseRect;
	CString m_strWaterMark;
	afx_msg void OnBnClickedFont();

	int m_posX;
	int m_posY;
	int m_posW;
	int m_posH;

	int m_posWaterMarkX;
	int m_posWaterMarkY;

	afx_msg void OnBnClickedCheck1();
	afx_msg void OnBnClickedSelectImage();
	int m_posImageWaterMarkX;
	int m_posImageWaterMarkY;
	BOOL m_bWaterMark;
	afx_msg void OnBnClickedCheck2();
	int m_iAplhaWaterMark;
	afx_msg void OnNMCustomdrawSlider3(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	afx_msg void OnBnClickedLeft();
	afx_msg void OnBnClickedRight();

	int m_iRadius;
	BOOL m_bUseMouse;

	afx_msg void OnBnClickedUseMouse();
	afx_msg void OnBnClickedRight2();

	BOOL m_bMouseHotdot;
	int m_iRadiusHotdot;
	afx_msg void OnBnClickedUseMouse2();

	BOOL m_bWaterMarkImage;
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedResume();
	afx_msg void OnBnClickedCamera();
	BOOL m_bFollowMouse;
	afx_msg void OnBnClickedChangerect();
	CComboBox m_arrFmt;
	afx_msg void OnBnClickedVideosetting();
	afx_msg void OnBnClickedGetpic1();
	afx_msg void OnBnClickedEgtpic2();
	BOOL m_bHw;
	CComboBox m_cbxMode;
	afx_msg void OnBnClickedCheck5();
	CProgressCtrl m_progress1;
	CProgressCtrl m_progressFfmpegTask2;
	BOOL m_bRecordMouse;
	afx_msg void OnBnClickedCheck6();

	CProgressCtrl m_volume1;
	CProgressCtrl m_volume2;
	CProgressCtrl m_volume3;
	CProgressCtrl m_volume4;
	CProgressCtrl m_volume5;
	CProgressCtrl m_volume6;
	CProgressCtrl m_volume7;
	CProgressCtrl m_volume8;
	BOOL m_bDXGI;
	BOOL m_bPreview;


	//afx_msg LRESULT OnGameMsg(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHotKey(WPARAM     wParam, LPARAM     lParam);

	void Log(CString str);
	void LogHook(CString str);

	CSliderCtrl m_slider1;
	CSliderCtrl m_slider2;
	CString m_strLog1;
	CString m_strLog2;
	CStatic m_pictureLeft;
	CStatic m_pictureRight;
	CStatic m_pictureHotdot;
	afx_msg void OnDestroy();
	BOOL m_bAEC;
	BOOL m_bHighQualityGif;
	BOOL m_bMixVideo;

	BOOL m_bMixVideoType;
	BOOL m_bMixVideoWidth;
	BOOL m_bMixVideoHeight;
	int m_nCameraWndWidth;
	CComboBox m_cbTypeCameraPos;
	BOOL m_bDisableQSV;
	BOOL m_bDisableNVENC;
	BOOL m_bHighQualityRec;
	CComboBox m_cbScale;
	CComboBox m_cbVideoCodec;
	CComboBox m_cbAudioCodec;
	BOOL m_bTS;
	CComboBox m_cbRec;
	afx_msg void OnBnClickedCheck8();
};
