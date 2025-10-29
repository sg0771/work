#pragma once
#include <aoce/layer/InputLayer.hpp>
#if _WIN32
#include "../win32/VkWinImage.hpp"
#elif __ANDROID__
#include "../android/HardwareImage.hpp"
#endif
#include "VkLayer.hpp"

namespace aoce {
namespace vulkan {
namespace layer {

// 把各种VideoFormat转化成ImageFormat,主要二种,R8/RGBA8
// InputLayer相对别的层,在
class AOCE_VULKAN_EXPORT VkInputLayer : public InputLayer, public VkLayer {
    AOCE_LAYER_QUERYINTERFACE(VkInputLayer)
   private:
    std::unique_ptr<VulkanBuffer> inBuffer = nullptr;
    // 如果需要GPU计算,需要先把inBuffer copy 到 gpu local
    std::unique_ptr<VulkanBuffer> inBufferX = nullptr;
    // 是否需要GPU计算
    bool bUsePipe = false;
    bool bDateUpdate = false;
#if _WIN32
    std::unique_ptr<VkWinImage> winImage = nullptr;
#elif __ANDROID__
    std::unique_ptr<HardwareImage> hardwareImage = nullptr;
#endif
   public:
    VkInputLayer(/* args */);
    ~VkInputLayer();

    // InputLayer
   public:
    virtual void onDataReady() override;
    virtual void inputGpuData(void* device, void* tex) override;

    // VkLayer
   protected:
    virtual void onInitGraph() override;
    virtual void onInitVkBuffer() override;
    virtual void onInitPipe() override;
    virtual void onCommand() override;
    virtual void onPreFrame() override;
    virtual bool onFrame() override;

    virtual void onUnInit() override;
};

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce