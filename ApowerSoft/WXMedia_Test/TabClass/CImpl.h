// 运行ffmpeg.exe 等其它exe
//并且将输出结果显示在界面上

#if !defined(CFfmpeg_H)
#define CFfmpeg_H

#include "stdafx.h"
#include "resource.h"

#undef  CLASS_NAME
#define CLASS_NAME CFfmpeg
class CLASS_NAME : public CDialogImpl<CLASS_NAME>, public CWinDataExchange<CLASS_NAME>
{
public:
    enum { IDD = IDD_FFMPEG};

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        return IsDialogMessage(pMsg);
    }

    BEGIN_DDX_MAP(CLASS_NAME)
    END_DDX_MAP()

    BEGIN_MSG_MAP(CLASS_NAME)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_TIMER,   OnTimer)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()


    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};


#endif // !defined(CFfmpeg_H)
