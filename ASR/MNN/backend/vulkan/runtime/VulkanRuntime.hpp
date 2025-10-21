//
//  VulkanRuntime.hpp
//  MNN
//
//  Created by MNN on b'2020/06/06'.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#ifndef VulkanRuntime_hpp
#define VulkanRuntime_hpp
#include <queue> 
#include "../component/VulkanBuffer.hpp"
#include "../component/VulkanCommandPool.hpp"
#include "../component/VulkanDevice.hpp"
#include "../component/VulkanFence.hpp"
#include "../component/VulkanImage.hpp"
#include "../component/VulkanInstance.hpp"
#include "../component/VulkanPipeline.hpp"
#include "../component/VulkanQueryPool.hpp"
#include "core/Backend.hpp"

#include <MNNSharedContext.h>

namespace MNN {
class VulkanRuntime : public Runtime {
public:
    VulkanRuntime(const Backend::Info& info);
    virtual ~ VulkanRuntime();

    virtual Backend* onCreate(const BackendConfig* config) const override;
    enum GPUType { ADRENO = 0, MALI = 1, OTHER = 2 };
    virtual void onGabageCollect(int level) override;
    virtual float onGetMemoryInMB() override;
    int onGetRuntimeStatus(RuntimeStatus statusEnum) const override;
    std::shared_ptr<VulkanBuffer> allocUniform(const void* src = nullptr, int size = 0);
    void recycleUniform(std::shared_ptr<VulkanBuffer> buffer);
private:
    Backend::Info mInfo;
    std::shared_ptr<BufferAllocator> mBufferPool;
    std::shared_ptr<VulkanPipelineFactory> mPipelineFactory;
    std::shared_ptr<VulkanCommandPool> mCmdPool;
    std::shared_ptr<VulkanMemoryPool> mMemoryPool;
    std::shared_ptr<VulkanSampler> mSampler;
    std::shared_ptr<VulkanSampler> mClampSampler;
    std::shared_ptr<VulkanInstance> mInstance;
    std::shared_ptr<VulkanQueryPool> mQueryPool;
    std::shared_ptr<VulkanDevice> mDevice;
    std::queue<std::shared_ptr<VulkanBuffer>> mUniformCache;
    // Limit Uniform cache size = mUniformSize * mCacheUniformLimitSize B
    int mUniformSize = 512;
    int mCacheUniformLimitSize = 1024;
    float mFlops = 0.0f;
    friend class VulkanBackend;
    GPUType mGpuType = OTHER;
};
}
#endif
