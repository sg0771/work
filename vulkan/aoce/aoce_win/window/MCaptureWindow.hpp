#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

#include "../BCaptureWindow.hpp"
#include "../WinHelp.hpp"
#include "../dx11/Dx11Helper.hpp"
namespace aoce {
namespace win {

class MCaptureWindow : public BCaptureWindow {
   private:
    //
    std::mutex stopMtx;
    std::condition_variable stopSignal;
    bool bSync = false;

   public:
    MCaptureWindow(/* args */);
    ~MCaptureWindow();

   public:
    // 初始化d3d device等信息
    virtual bool startCapture(IWindow* window, bool bSync) override;
    virtual bool renderCapture() override;
    virtual void stopCapture() override;
};

}  // namespace win
}  // namespace aoce