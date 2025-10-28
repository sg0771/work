#pragma once

// #include <DispatcherQueue.h>
#include <Unknwn.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <atlcomcli.h>
#include <d3d11.h>
#include <dwmapi.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.System.h>

#include <Aoce/Aoce.hpp>

#include "aoce_win/BCaptureWindow.hpp"

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"windowsapp.lib")


namespace winrt {
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
}  // namespace winrt

using namespace aoce::win;

namespace aoce {
namespace awinrt {

class RTCaptureWindow : public BCaptureWindow {
   private:
    // winrtCapture::GraphicsCaptureItem item{nullptr};
    winrt::IDirect3DDevice rtDevice{nullptr};
    winrt::Direct3D11CaptureFramePool framePool{nullptr};
    winrt::GraphicsCaptureSession session{nullptr};

    // winrt::com_ptr<IInspectable> inspectable;
    // winrt::com_ptr<IGraphicsCaptureItemInterop> interop = nullptr;

    // std::mutex stopMtx;
    // std::condition_variable stopSignal;

   public:
    RTCaptureWindow(/* args */);
    virtual ~RTCaptureWindow();
    // bool renderCapture();

    void onClosed(winrt::GraphicsCaptureItem const& sender,
                  winrt::IInspectable const& inspectable);
    void onFrameArrived(winrt::Direct3D11CaptureFramePool framePool,
                        winrt::IInspectable const& inspectable);

   public:
    // 初始化d3d device等信息
    virtual bool startCapture(IWindow* window, bool bSync) override;
    virtual bool renderCapture() override;
    virtual void stopCapture() override;
};

}  // namespace awinrt
}  // namespace aoce