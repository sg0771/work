// 运行ffmpeg.exe 等其它exe
//并且将输出结果显示在界面上

#if !defined(CCmd_H)
#define CCmd_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CCmd
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{

	CString m_strCmdName = L"ffmpeg.exe"; //命令名称
	//参数, 前面加空格，路径名如果有空格，必须加引号
    CString m_strParam;// = L" -i D:\\a.mp4 -c copy D:\\a.copy.mp4 -y ";
	CString m_strLog = L"My Log";     //日志

public:
    // 管道句柄
    HANDLE m_hPipeStdoutRead = NULL;
    HANDLE m_hPipeStdoutWrite = NULL;
    HANDLE m_hPipeStderrRead = NULL;
    HANDLE m_hPipeStderrWrite = NULL;

    // 进程句柄
    HANDLE m_hProcess = NULL;

    // ffmpeg相关路径
    CString m_strFFmpegPath;
    CString m_strInputFile;
    CString m_strOutputFile;

    //清理资源
    void CleanupResources();
    
    // 检查ffmpeg是否正在运行
    BOOL IsFFmpegRunning();

    // 读取管道数据的函数
    void ReadPipeOutput(HANDLE hPipe, CString& strOutput);
public:
    enum { IDD = IDD_FFMPEG};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
        DDX_TEXT(IDC_CMD_NAME_CMD, m_strCmdName)
        DDX_TEXT(IDC_PARAM_CMD, m_strParam)
		DDX_TEXT(IDC_LOG_CMD, m_strLog)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_TIMER,   OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_INPUT_CMD, BN_CLICKED, OnBnClickedInputCmd)
        COMMAND_HANDLER(IDC_RUN_CMD, BN_CLICKED, OnBnClickedRunCmd)
        COMMAND_HANDLER(IDC_BREAK_CMD, BN_CLICKED, OnBnClickedBreakCmd)
    END_MSG_MAP()


    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedInputCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRunCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedBreakCmd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CCmd_H)
