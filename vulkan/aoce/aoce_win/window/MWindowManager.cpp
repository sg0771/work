#include "MWindowManager.hpp"

namespace aoce {
namespace win {

bool check_window_valid(HWND window) {
    DWORD styles, ex_styles;
    RECT rect;

    if (!IsWindowVisible(window) || IsIconic(window)) {
        return false;
    }
    ::GetClientRect(window, &rect);
    styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
    ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

    if (ex_styles & WS_EX_TOOLWINDOW) {
        return false;
    }
    if (styles & WS_CHILD) {
        return false;
    }
    if (rect.bottom == 0 || rect.right == 0) {
        return false;
    }
    return true;
}

BOOL CALLBACK findWindowFunc(HWND hwnd, LPARAM l_param) {
    if (check_window_valid(hwnd)) {
        MWindowManager* mwManager = reinterpret_cast<MWindowManager*>(l_param);
        mwManager->addWindow(hwnd);
    }
    return true;
}

MWindowManager::MWindowManager(/* args */) {}

MWindowManager::~MWindowManager() {}

void MWindowManager::getWindowList() {
    windows.clear();
    ::EnumWindows(findWindowFunc, reinterpret_cast<LPARAM>(this));
}

void MWindowManager::addWindow(HWND hwnd) {
    std::unique_ptr<MWindow> wind = std::make_unique<MWindow>();
    wind->initWindow(hwnd);
    windows.push_back(std::move(wind));
}

int32_t MWindowManager::getWindowCount(bool bUpdate) {
    if (bUpdate || windows.size() == 0) {
        getWindowList();
    }
    return windows.size();
}

IWindow* MWindowManager::getWindow(int32_t index) {
    assert(index >= 0 && index < windows.size());
    return windows[index].get();
}

IWindow* MWindowManager::getDesktop() { return nullptr; }

void MWindowManager::setForeground(IWindow* window) {
    HWND hWnd = (HWND)window->getHwnd();
    if (!::IsWindow(hWnd)) {
        return;
    }
    // relation time of SetForegroundWindow lock
    DWORD lockTimeOut = 0;
    HWND hCurrWnd = ::GetForegroundWindow();
    DWORD dwThisTID = ::GetCurrentThreadId();
    DWORD dwCurrTID = ::GetWindowThreadProcessId(hCurrWnd, 0);

    // 我们需要绕过微软的一些限制
    if (dwThisTID != dwCurrTID) {
        ::AttachThreadInput(dwThisTID, dwCurrTID, TRUE);
        ::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &lockTimeOut,
                               0);
        ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0,
                               SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        ::AllowSetForegroundWindow(ASFW_ANY);
    }
    ::SetForegroundWindow(hWnd);
    if (dwThisTID != dwCurrTID) {
        ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0,
                               (PVOID)lockTimeOut,
                               SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
        ::AttachThreadInput(dwThisTID, dwCurrTID, FALSE);
    }
}

}  // namespace win
}  // namespace aoce