#include "WinRTModule.hpp"

#include <aoce/AoceManager.hpp>

#include "RTCaptureWindow.hpp"

#include <aoce/module/ModuleManager.hpp>

namespace aoce {
namespace awinrt {

WinRTModule::WinRTModule(/* args */) {}

WinRTModule::~WinRTModule() {}

bool WinRTModule::loadModule() {
    try {
        /* no contract for IGraphicsCaptureItemInterop, verify 10.0.18362.0 */
        winrt::Windows::Foundation::Metadata::ApiInformation::
            IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
        // winrt::init_apartment(winrt::apartment_type::multi_threaded);
        AoceManager::Get().addICaptureWindow(CaptureType::win_rt,
                                             new RTCaptureWindow());
        if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::
                IsSupported()) {
            return false;
        }
    } catch (...) {
        logMessage(LogLevel::error, "winrt capture not supported");
        return false;
    }
    return true;
}

void WinRTModule::unloadModule() {
    AoceManager::Get().removeICaptureWindow(CaptureType::win_rt);
    // winrt::uninit_apartment();
}

ADD_MODULE(WinRTModule, aoce_winrt)

}  // namespace awinrt
}  // namespace aoce