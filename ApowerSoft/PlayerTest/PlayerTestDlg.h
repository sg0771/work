
// PlayerTestDlg.h: 头文件
//

#pragma once

#include "pch.h"

#include <thread>
#include "WXMedia/WXMedia.h"
#include "WXMediaDefines.h"
#include "FfmpegIncludes.h"
#pragma comment(lib,"WXMedia.lib")	
#pragma comment(lib,"libffmpeg.lib")	
#include <WXLog.h>

//轨道上的素材，主要是视频+音频

#define CHANNEL 2
struct MediaCtx
{
	std::wstring m_strName;//
	float m_offset = 0.0f;//在轨道上的偏移量 
	float m_stop = 0.0f;//素材的持续时间    等于  m_offset+ m_speed(m_end-m_start)
	float m_start = 0.0f;//素材的开始时间
	float m_end = 0.0f;//素材的结束时间
	//float m_speed = 1.0f;//素材的播放速度
	void* m_play = nullptr;//播放器对象
};

// CPlayerTestDlg 对话框
class CPlayerTestDlg : public CDialogEx
{
	// 构造
	BOOL  m_bRunning[CHANNEL] = { FALSE,FALSE };//是否启动播放
	BOOL  m_bPause[CHANNEL]   = { FALSE,FALSE };//是否暂停

	float m_timeCurr = 0.0f; //当前播放时间
	float m_timeMax[CHANNEL] = { 0.0f,0.0f }; //当前播放时间偏移量

	float m_timeTotal= 0.0f; //列表总时间

	HWND m_hwnd[CHANNEL] = { nullptr,nullptr };//播放器窗口句柄【

	std::vector<MediaCtx> m_vecMedia[CHANNEL]; //素材列表


	//在 UpdateList 之后更新m_vecMedia

	void* m_playerCurr[CHANNEL] = { nullptr,nullptr };//播放器对象
	void* m_vRender[CHANNEL] = { nullptr,nullptr };//渲染对象

	int m_ID_LOG[CHANNEL] = {IDC_LOG, IDC_LOG2};
	BOOL m_bSeek = FALSE; //是否正在拖动进度条
	int m_nPos = 0;//当前播放位置

	DWORD m_tsCurr = 0; //当前时间戳
	UINT_PTR m_nTimerPlay = 1001; //定时器ID

	CString m_strFileName[CHANNEL]; //当前播放文件名	
	void SeekTo(int nPos); //拖动进度条到指定位置

	bool m_bRunThread = false; //播放线程运行标志
	std::thread* m_threadPlay[CHANNEL] = { nullptr,nullptr }; //播放线程
	void PlayThread(int index); //播放线程函数
	void Func(int index); //播放函数

	//在主界面上执行 停止、暂停、播放等操作，单个对象还好，多个对象顺序执行容易把主界面卡死
	bool m_bWillStop[CHANNEL] = { false,false }; //是否停止播放
	bool m_bWillPause[CHANNEL] = { false,false }; //是否暂停播放
	bool m_bWillResume[CHANNEL] = { false,false }; //是否恢复播放
public:
	void OnFrame(int index, struct AVFrame* pFrame);//视频帧回调
public:
	CPlayerTestDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PLAYERTEST_DIALOG };
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

	CListCtrl m_list[CHANNEL];
	void Add(int index);
	void Remove(int index);

	afx_msg void OnBnClickedAdd0();
	afx_msg void OnBnClickedRemove0();

	afx_msg void OnBnClickedAdd1();
	afx_msg void OnBnClickedRemove1();

	void UpdateList(int index);

	afx_msg void OnBnClickedPlay();//播放
	afx_msg void OnBnClickedPause();//暂停
	CSliderCtrl m_slider;//进度条
	int m_nDelay;//延迟时间

	afx_msg void OnNMReleasedcaptureSlider1(NMHDR* pNMHDR, LRESULT* pResult);
	BOOL m_bLAV;
	BOOL m_bHW;
	BOOL m_bRGB_Output;
};
