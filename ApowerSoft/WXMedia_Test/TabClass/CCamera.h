//功能: 摄像头打开关闭，设置虚拟背景

#if !defined(CCamera_H)
#define CCamera_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CCamera
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
    BOOL m_bFilp = FALSE; //翻转
	CComboBox m_cbCameraDevice; //摄像头设备列表
	CComboBox m_arrFmt;//分辨率列表
	void* m_pCameraPreview = nullptr;//摄像头预览对象

public:
    enum { IDD = IDD_CAMERA};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CCamera)
        DDX_CONTROL_HANDLE(IDC_CB_NAME_CAMERA,  m_cbCameraDevice)
        DDX_CONTROL_HANDLE(IDC_CB_DECS_CAMERA, m_arrFmt)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CCamera)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        COMMAND_HANDLER(IDC_CAMERA, BN_CLICKED, OnBnClickedCamera)
        COMMAND_HANDLER(IDC_VB_CAMERA, BN_CLICKED, OnBnClickedVbCamera)
        COMMAND_HANDLER(IDC_SETTING_CAMERA, BN_CLICKED, OnBnClickedSettingCamera)
        COMMAND_HANDLER(IDC_CB_NAME_CAMERA, CBN_SELCHANGE, OnCbnSelchangeCbNameCamera)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedVbCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedSettingCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCbnSelchangeCbNameCamera(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};


#endif // !defined(CReadToHex_H)
