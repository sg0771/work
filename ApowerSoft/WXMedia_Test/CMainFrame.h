// MainFrm.h : interface of the CMainFrame class

#if !defined(CMainFrame_H)
#define CMainFrame_H


#include <stdafx.h>

//子类化的tab页

//WXMedia 播放器测试tab页
#include "./TabClass/CWXPlayer.h" //WXMedia 播放器测试tab页
#include "./TabClass/CMediaInfoTest.h"//MediaInfo.dll测试tab页
#include "./TabClass/CReadToHex.h"//读文件到十六进制tab页
#include "./TabClass/CRep.h"//WXM XWS 文件修复和格式转换 类似于ffmpeg -c copy
#include "./TabClass/CCamera.h"//摄像头预览和虚拟背景
#include "./TabClass/CMPVPlayer.h"//MPV播放器集成测试tab页

#include "./TabClass/CCmd.h"//运行ffmpeg.exe 等其它exe
#include "./TabClass/CMiniRec.h"//显示器录制+声音录制+摄像头录制+系统声音录制+麦克风录制+鼠标点击录制+键盘按键录制

#include "./TabClass/CMedialib.h"//Medialib 视频编辑 播放以及导出测试

// Add more tab types here

enum {
    TYPE_WXPlayer = 0,//播放器测试tab页
    TYPE_MPVPlayer = 1, //MPV播放器集成测试tab页

    TYPE_MediaInfoTest = 2, //MediaInfo.dll测试tab页
    TYPE_ReadToHex = 3, //读文件到十六进制tab页

    TYPE_Rep = 4, //WXM XWS 文件修复和格式转换 类似于ffmpeg -c copy
    TYPE_Camera = 5, //摄像头预览和虚拟背景
	TYPE_CMD = 6, //cmds 命令行运行exe，并且将输出结果显示在界面上
    TYPE_REC = 7, //录屏测试

    TYPE_Medialib = 8, //视频编辑测试

    // Add more tab types here
};

class CWTLTabViewCtrl : public CWindowImpl<CWTLTabViewCtrl, CTabCtrl >
{
private:
    const LONG TABVIEW_BORDER;          ///< Border Width
    const LONG TABVIEW_EDGE;            ///< Distance of tab from content
private:
    CSimpleArray<CWindow>       m_Views;            ///< An array of views for the tab
    LONG                        m_ActiveTabIndex;   ///< The index of the active tab
    CWindow                     m_ActiveTabWindow;  ///< The active tab window
    CFont                       m_HorizFont;        ///< Top/Bottom font used by tab control
    CFont                       m_LeftFont;         ///< Left font used by tab control
    CFont                       m_RightFont;        ///< Right font used by tab control
public:
    CWTLTabViewCtrl() :
        TABVIEW_BORDER(3),
        TABVIEW_EDGE(5),
        m_ActiveTabIndex(-1)
    {
    }

    virtual ~CWTLTabViewCtrl()
    {
    }


public:

    BOOL AddTab(LPCTSTR inLabel, HWND inTabWindow, BOOL inActiveFlag = TRUE, int inImage = -1, LPARAM inParam = 0)
    {
        BOOL    theReturn;
        CWindow theTabWindow(inTabWindow);
        // Make sure it's a real window
        ATLASSERT(theTabWindow.IsWindow());
        // Make sure it's a child window and is not visible
        ATLASSERT((theTabWindow.GetStyle() & WS_CHILD) != 0);
        ATLASSERT((theTabWindow.GetStyle() & WS_VISIBLE) == 0);
        // Hide the view window
        theTabWindow.EnableWindow(FALSE);
        theTabWindow.ShowWindow(SW_HIDE);
        // Store the required data for the list
        m_Views.Add(theTabWindow);
        // Add the tab to the tab control
        TC_ITEM theItem = { 0 };
        theItem.mask = TCIF_TEXT;
        theItem.pszText = const_cast<wchar_t*>(inLabel);

        // Add an image for the tab
        if (inImage != -1) {
            theItem.mask |= TCIF_IMAGE;
            theItem.iImage = inImage;
        }

        // Add the param for the tab
        if (inParam != 0) {
            theItem.mask |= TCIF_PARAM;
            theItem.lParam = inParam;
        }

        // Insert the item at the end of the tab control
        theReturn = InsertItem(32768, &theItem);
        // Set the position for the window
        CRect rcChild;
        CalcViewRect(&rcChild);
        theTabWindow.MoveWindow(rcChild);
        // Select the tab that is being added, if desired
        LONG theTabIndex = GetTabCount() - 1;

        if (inActiveFlag || theTabIndex == 0) {
            SetActiveTab(theTabIndex);
        }

        return theReturn;
    }

    LONG GetTabCount()
    {
        return m_Views.GetSize();
    }

    HWND GetTab(int inTab)
    {
        HWND theTabHwnd = NULL;

        if (inTab >= 0 && inTab < GetTabCount()) {
            m_Views[inTab];
        }

        return theTabHwnd;
    }

    virtual void OnTabRemoved(int inTabIndex)
    {
        UNREFERENCED_PARAMETER(inTabIndex);
    }

    HWND RemoveTab(int inTab)
    {
        HWND theTabHwnd = NULL;
        LONG theNewTab = -1;

        if (inTab >= 0 && inTab < GetTabCount()) {
            // Select a new tab if the tab is active
            if (m_ActiveTabIndex == inTab) {
                m_ActiveTabIndex = -1;
                m_ActiveTabWindow = NULL;

                if (GetTabCount() > 1) {
                    theNewTab = (inTab > 0) ? (inTab - 1) : 0;
                }
            }

            // Save the window that is begin removed
            theTabHwnd = m_Views[inTab];
            // Virtual method to notify subclasses that a tab was removed
            OnTabRemoved(inTab);
            // Remove the item from the view list
            m_Views.RemoveAt(inTab);

            // Remove the tab
            if (IsWindow()) {
                DeleteItem(inTab);
            }

            SetActiveTab(theNewTab);
        }

        return theTabHwnd;
    }

    void RemoveAllTabs()
    {
        LONG theCount = GetTabCount();

        for (LONG theIndex = theCount - 1; theIndex >= 0; theIndex--) {
            RemoveTab(theIndex);
        }
    }

    void SetActiveTab(int inNewTab)
    {
        if (inNewTab >= 0 && inNewTab < GetTabCount() &&      // Validate the tab index range
            inNewTab != m_ActiveTabIndex) {                // Don't select if already selected
            // Disable the old tab
            if (m_ActiveTabWindow.IsWindow()) {
                m_ActiveTabWindow.EnableWindow(FALSE);
                m_ActiveTabWindow.ShowWindow(SW_HIDE);
            }

            // Enable the new tab
            m_ActiveTabWindow = m_Views[inNewTab];
            m_ActiveTabWindow.EnableWindow(TRUE);
            m_ActiveTabWindow.ShowWindow(SW_SHOW);
            m_ActiveTabWindow.SetFocus();
            m_ActiveTabWindow.Invalidate(TRUE);
            m_ActiveTabIndex = inNewTab;
            // Select the tab (if tab programmatically changed)
            SetCurSel(m_ActiveTabIndex);
        }
    }

    HWND GetActiveTab()
    {
        return GetTab(m_ActiveTabIndex);
    }

    LONG GetActiveTabIndex()
    {
        return m_ActiveTabIndex;
    }

    void GetTabText(int inTab, int inSize, wchar_t* outText)
    {
        if (inTab >= 0 && inTab < GetTabCount()) {
            // Get tab item info
            TCITEM tci;
            tci.mask = TCIF_TEXT;
            tci.pszText = outText;
            tci.cchTextMax = inSize;
            GetItem(inTab, &tci);
        }
    }

#ifdef __ATLSTR_H__

    const CString GetTabText(int inTab)
    {
        CString theTabText;

        if (inTab >= 0 && inTab < GetTabCount()) {
            // Get tab item info
            wchar_t theText[128];
            GetTabText(inTab, sizeof(theText), theText);
            theTabText = theText;
        }

        return theTabText;
    }

#endif

    LPARAM GetTabParam(int inTab)
    {
        TCITEM tci = { 0 };

        if (inTab >= 0 && inTab < GetTabCount()) {
            // Get tab item info
            tci.mask = TCIF_PARAM;
            GetItem(inTab, &tci);
        }

        return tci.lParam;
    }

    int GetTabImage(int inTab)
    {
        TCITEM tci = { 0 };

        if (inTab >= 0 && inTab < GetTabCount()) {
            // Get tab item info
            tci.mask = TCIF_IMAGE;
            GetItem(inTab, &tci);
        }

        return tci.iImage;
    }

    BOOL ModifyTabStyle(DWORD dwRemove, DWORD dwAdd, UINT nFlags)
    {
        // Modify the style
        BOOL theReturn = ModifyStyle(dwRemove, dwAdd, nFlags);
        // Update all the views in case the window positions changes
        UpdateViews();
        // Set the font in case the tab position changed
        SetTabFont(dwAdd);
        return theReturn;
    }

public:
    BEGIN_MSG_MAP_EX(CWTLTabViewCtrl)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        REFLECTED_NOTIFY_CODE_HANDLER_EX(TCN_SELCHANGE, OnSelectionChanged)
    END_MSG_MAP()

protected:
    void UpdateViews()
    {
        CRect rcChild;
        CalcViewRect(&rcChild);
        LONG theCount = GetTabCount();

        for (LONG theIndex = 0; theIndex < theCount; theIndex++) {
            m_Views[theIndex].MoveWindow(rcChild);
        }
    }
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
    {
        LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
        // Get the log font.
        NONCLIENTMETRICS ncm;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
        // Top and Bottom Tab Font
        m_HorizFont.CreateFontIndirect(&ncm.lfMessageFont);
        // Left Tab Font
        ncm.lfMessageFont.lfOrientation = 900;
        ncm.lfMessageFont.lfEscapement = 900;
        m_LeftFont.CreateFontIndirect(&ncm.lfMessageFont);
        // Right Tab Font
        ncm.lfMessageFont.lfOrientation = 2700;
        ncm.lfMessageFont.lfEscapement = 2700;
        m_RightFont.CreateFontIndirect(&ncm.lfMessageFont);
        // Check styles to determine which font to set
        SetTabFont(GetStyle());
        return lRet;
    }

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
    {
        RemoveAllTabs();
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnWindowPosChanged(UINT, WPARAM, LPARAM, BOOL& bHandled)
    {
        UpdateViews();
        bHandled = FALSE;
        return 0;
    }

    LRESULT OnSelectionChanged(LPNMHDR)
    {
        SetActiveTab(GetCurSel());
        return 0;
    }

protected:

    void CalcViewRect(CRect* pRect)
    {
        GetClientRect((*pRect));

        if (pRect->Height() > 0 && pRect->Width() > 0) {
            // Calculate the Height (or Width) of the tab . . .
            // cause it could be Multiline
            CRect theTabRect;
            GetItemRect(0, &theTabRect);
            LONG theRowCount = GetRowCount();
            LONG theEdgeWidth = (theTabRect.Width() * theRowCount) + TABVIEW_EDGE;
            LONG theEdgeHeight = (theTabRect.Height() * theRowCount) + TABVIEW_EDGE;
            // Set the size based on the style
            DWORD dwStyle = GetStyle();

            if ((dwStyle & TCS_BOTTOM) && !(dwStyle & TCS_VERTICAL)) {      // Bottom
                (*pRect).top += TABVIEW_BORDER;
                (*pRect).left += TABVIEW_BORDER;
                (*pRect).right -= TABVIEW_BORDER;
                (*pRect).bottom -= theEdgeHeight;
            }
            else if ((dwStyle & TCS_RIGHT) && (dwStyle & TCS_VERTICAL)) { // Right
                (*pRect).top += TABVIEW_BORDER;
                (*pRect).left += TABVIEW_BORDER;
                (*pRect).right -= theEdgeWidth;
                (*pRect).bottom -= TABVIEW_BORDER;
            }
            else if (dwStyle & TCS_VERTICAL) {                            // Left
                (*pRect).top += TABVIEW_BORDER;
                (*pRect).left += theEdgeWidth;
                (*pRect).right -= TABVIEW_BORDER;
                (*pRect).bottom -= TABVIEW_BORDER;
            }
            else {                                                        // Top
                (*pRect).top += theEdgeHeight;
                (*pRect).left += TABVIEW_BORDER;
                (*pRect).right -= TABVIEW_BORDER;
                (*pRect).bottom -= TABVIEW_BORDER;
            }
        }
    }

    void SetTabFont(DWORD inStyleBits)
    {
        if (inStyleBits & TCS_VERTICAL) {
            if (inStyleBits & TCS_RIGHT) {
                SetFont(m_RightFont);
            }
            else {
                SetFont(m_LeftFont);
            }
        }
        else {
            SetFont(m_HorizFont);
        }
    }
};

#define AddDemoViewTab(ViewType, pCtrl , index,inTabName) \
{   \
    ViewType* theDemoView = new ViewType; \
    theDemoView->Create(pCtrl);\
    pCtrl.AddTab(inTabName, *theDemoView, TRUE, index, (LPARAM)theDemoView);\
} \

#define DELETE_TABWINDOW(theTabClass, index) \
            case index: { \
                theTabClass* theWindowPtr = reinterpret_cast<theTabClass*>(theTabParam); \
                theWindowPtr->DestroyWindow(); \
                delete theWindowPtr; \
                break; \
            }

class CMyTabViewCtrl : public CWTLTabViewCtrl
{
public:

    CMyTabViewCtrl()
    {
    }

    virtual ~CMyTabViewCtrl()
    {
    }

    virtual void OnTabRemoved(int inTabIndex)
    {
        LPARAM theTabParam = GetTabParam(inTabIndex);

        if (theTabParam != 0) {
            int theTabImage = GetTabImage(inTabIndex);



            switch (theTabImage) {
                    DELETE_TABWINDOW(CWXPlayer, TYPE_WXPlayer)
                    DELETE_TABWINDOW(CMediaInfoTest, TYPE_MediaInfoTest)
                    DELETE_TABWINDOW(CReadToHex, TYPE_ReadToHex)
                    DELETE_TABWINDOW(CRep, TYPE_Rep)
                    DELETE_TABWINDOW(CCamera, TYPE_Camera)
                    DELETE_TABWINDOW(CMPVPlayer, TYPE_MPVPlayer)
                    DELETE_TABWINDOW(CCmd, TYPE_CMD)
                    DELETE_TABWINDOW(CMiniRec, TYPE_REC)
                    DELETE_TABWINDOW(CMedialib, TYPE_Medialib)
            }
        }
    }

public:
    DECLARE_WND_SUPERCLASS(NULL, CWTLTabViewCtrl::GetWndClassName())

    BOOL PreTranslateMessage(MSG* pMsg)
    {
        pMsg;
        return FALSE;
    }

    BEGIN_MSG_MAP_EX(CMyTabViewCtrl)
        CHAIN_MSG_MAP(CWTLTabViewCtrl)
        END_MSG_MAP()
};

class CMainFrame : public CFrameWindowImpl<CMainFrame>
{
protected:
    CMyTabViewCtrl      m_TabViewDemo;  ///< The demonstration window
    CImageList          m_ImageList;  ///< The image list to associate with the tab ctrl
    LONG                m_TotalTabCount = 1;    ///< The count of total tabs created
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg)
    {
        if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
            return TRUE;

        return m_TabViewDemo.PreTranslateMessage(pMsg);
    }


    BEGIN_MSG_MAP(CMainFrame)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        REFLECT_NOTIFICATIONS()
        CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
    END_MSG_MAP()


public:

    CMainFrame() : m_TotalTabCount(1)
    {
    }

    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        // remove old menu
        SetMenu(NULL);
        CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
        CreateSimpleStatusBar();
        //TODO: Replace with a URL of your choice
        m_hWndClient = m_TabViewDemo.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE, WS_EX_STATICEDGE);
        m_ImageList.Create(IDB_TABBITMAP, 16, 1, RGB(0, 255, 0));
        m_TabViewDemo.SetImageList(m_ImageList);
        AddDemoViewTab(CWXPlayer , m_TabViewDemo, TYPE_WXPlayer, L"WX播放器");
        AddDemoViewTab(CMPVPlayer, m_TabViewDemo, TYPE_MPVPlayer, L"MPV播放");
        AddDemoViewTab(CMediaInfoTest , m_TabViewDemo, TYPE_MediaInfoTest, L"媒体信息");
        AddDemoViewTab(CReadToHex , m_TabViewDemo, TYPE_ReadToHex, L"转16进制");
        AddDemoViewTab(CRep, m_TabViewDemo, TYPE_Rep, L"临时文件修复");
        AddDemoViewTab(CCamera, m_TabViewDemo, TYPE_Camera, L"摄像头预览");
        AddDemoViewTab(CCmd, m_TabViewDemo, TYPE_CMD, L"命令行");
        AddDemoViewTab(CMiniRec, m_TabViewDemo, TYPE_REC, L"录屏");

        AddDemoViewTab(CMedialib, m_TabViewDemo, TYPE_Medialib, L"视频编辑");
        m_TabViewDemo.SetActiveTab(0);

        this->SetWindowText(L"My TabView");

        WXDeviceInit(L"WXMedia.log");//WXMedia初始化

        return 0;
    }

};


#endif // !defined(CMainFrame_H)
