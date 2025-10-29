#pragma once

#include <aoce/layer/PipeGraph.hpp>
#include <memory>

#include "../vulkan/VulkanContext.hpp"
#include "VkLayer.hpp"

#if _WIN32
#include "../win32/VkWinImage.hpp"
#include "aoce_win/dx11/Dx11Helper.hpp"
#endif

namespace aoce {
namespace vulkan {
namespace layer {

class AOCE_VULKAN_EXPORT VkPipeGraph : public PipeGraph {
   private:
    // 对应一个vkCommandBuffer
    std::unique_ptr<VulkanContext> context;
    // 输入层
    // std::vector<VkLayer*> vkInputLayers;
    // 输出层
    std::vector<VkLayer*> vkOutputLayers;
    // 余下层
    std::vector<VkLayer*> vkLayers;
    // GPU是否执行完成
    VkFence computerFence;

    bool delayGpu = false;
    // 确定是否在重置生成资源与commandbuffer中
    VkEvent outEvent = VK_NULL_HANDLE;
    VkPipelineStageFlags stageFlags = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
#if _WIN32
    CComPtr<ID3D11Device> device = nullptr;
    CComPtr<ID3D11DeviceContext> ctx = nullptr;
    // std::vector<VkDeviceMemory> outMemorys;
    bool bDX11Update = false;
#endif
   public:
    VkPipeGraph(/* args */);
    ~VkPipeGraph();

   public:
    inline VulkanContext* getContext() { return context.get(); };
    VulkanTexturePtr getOutTex(int32_t node, int32_t outIndex);
    bool getMustSampled(int32_t node, int32_t inIndex);
    bool bOutLayer(int32_t node);
    bool resourceReady();

#if _WIN32
    ID3D11Device* getD3D11Device();
#endif
    bool executeOut();

   protected:
    // 所有layer调用initbuffer后
    virtual void onReset() override;
    virtual bool onInitBuffers() override;
    virtual bool onRun() override;
};

class VkPipeGraphFactory : public PipeGraphFactory {
   public:
    VkPipeGraphFactory(){};
    virtual ~VkPipeGraphFactory(){};

   public:
    inline virtual IPipeGraph* createGraph() override {
        return new VkPipeGraph();
    };
};

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce
