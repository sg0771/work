#pragma once

#include <memory>

#include "../VkExtraExport.h"
#include "aoce_vulkan/layer/VkLayer.hpp"

namespace aoce {
namespace vulkan {
namespace layer {

class VkPrewittEdgeDetectionLayer : public VkLayer, public ITLayer<float> {
    AOCE_LAYER_QUERYINTERFACE(VkPrewittEdgeDetectionLayer)
    AOCE_VULKAN_PARAMETUPDATE()
   private:
    /* data */
   public:
    VkPrewittEdgeDetectionLayer(/* args */);
    virtual ~VkPrewittEdgeDetectionLayer();

   protected:
    virtual void onInitGraph() override;
};

class VkSobelEdgeDetectionLayer : public VkPrewittEdgeDetectionLayer {
    AOCE_LAYER_QUERYINTERFACE(VkSobelEdgeDetectionLayer)
   private:
    /* data */
   public:
    VkSobelEdgeDetectionLayer(/* args */);
    virtual ~VkSobelEdgeDetectionLayer();
};

class VkSketchLayer : public VkPrewittEdgeDetectionLayer {
    AOCE_LAYER_QUERYINTERFACE(VkSketchLayer)
   private:
    /* data */
   public:
    VkSketchLayer(/* args */);
    virtual ~VkSketchLayer();
};

class VkThresholdSketchLayer : public VkLayer,
                               public ITLayer<ThresholdSobelParamet> {
    AOCE_LAYER_QUERYINTERFACE(VkThresholdSketchLayer)
    AOCE_VULKAN_PARAMETUPDATE()
   protected:
    /* data */
    bool bSignal = true;

   public:
    VkThresholdSketchLayer(bool signalChannal = true);
    virtual ~VkThresholdSketchLayer();

   protected:
    virtual void onInitGraph() override;
};

// 按GPUImage2里来,实现同VkThresholdSketchLayer
class VkThresholdEdgeDetectionLayer : public VkThresholdSketchLayer {
    AOCE_LAYER_QUERYINTERFACE(VkThresholdEdgeDetectionLayer)
    AOCE_VULKAN_PARAMETUPDATE()
   private:
    /* data */
   public:
    VkThresholdEdgeDetectionLayer(bool signalChannal = true);
    virtual ~VkThresholdEdgeDetectionLayer();
};

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce