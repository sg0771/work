#pragma once

#include "../VkExtraExport.h"
#include "aoce_vulkan/layer/VkLayer.hpp"

namespace aoce {
namespace vulkan {
namespace layer {

class VkVoronoiConsumerLayer : public VkLayer {
    AOCE_LAYER_GETNAME(VkVoronoiConsumerLayer)
   private:
    /* data */
   public:
    VkVoronoiConsumerLayer(/* args */);
    ~VkVoronoiConsumerLayer();

   protected:
    virtual bool getSampled(int32_t inIndex) override;
    virtual void onInitLayer() override;
};

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce