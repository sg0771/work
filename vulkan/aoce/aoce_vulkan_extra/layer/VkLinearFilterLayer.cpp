#include "VkLinearFilterLayer.hpp"

#include "aoce/layer/PipeGraph.hpp"
#if __ANDROID__
#include "aoce_vulkan/android/vulkan_wrapper.h"
#endif

namespace aoce {
namespace vulkan {
namespace layer {

VkLinearFilterLayer::VkLinearFilterLayer(ImageType imageType) {
    this->imageType = imageType;
    setUBOSize(16);
}

VkLinearFilterLayer::~VkLinearFilterLayer() {}

void VkLinearFilterLayer::onInitGraph() {
    std::string path = "glsl/filter2D.comp.spv";
    inFormats[0].imageType = imageType;
    outFormats[0].imageType = imageType;
    if (imageType == ImageType::r8) {
        path = "glsl/filter2DC1.comp.spv";
    } else if (imageType == ImageType::rgba32f) {
        path = "glsl/filter2DF4.comp.spv";
    }
    shader->loadShaderModule(context->device, path);

    std::vector<UBOLayoutItem> items = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT}};
    layout->addSetLayout(items);
    layout->generateLayout();
}

void VkLinearFilterLayer::onInitPipe() {
    inTexs[0]->descInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    outTexs[0]->descInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    layout->updateSetLayout(0, 0, &inTexs[0]->descInfo, &outTexs[0]->descInfo,
                            &constBuf->descInfo, &kernelBuffer->descInfo);
    auto computePipelineInfo = VulkanPipeline::createComputePipelineInfo(
        layout->pipelineLayout, shader->shaderStage);
    VK_CHECK_RESULT(vkCreateComputePipelines(
        context->device, context->pipelineCache, 1, &computePipelineInfo,
        nullptr, &computerPipeline));
}

VkBoxBlurLayer::VkBoxBlurLayer(ImageType imageType)
    : VkLinearFilterLayer(imageType) {}

VkBoxBlurLayer::~VkBoxBlurLayer() {}

void VkBoxBlurLayer::onUpdateParamet() {
    if (paramet == oldParamet) {
        return;
    }
    resetGraph();
}

void VkBoxBlurLayer::onInitVkBuffer() {
    std::vector<int32_t> ubo = {paramet.kernelSizeX, paramet.kernelSizeY,
                                paramet.kernelSizeX / 2,
                                paramet.kernelSizeY / 2};
    updateUBO(ubo.data());

    int kernelSize = paramet.kernelSizeX * paramet.kernelSizeY;
    float kvulve = 1.0f / (float)kernelSize;
    std::vector<float> karray(kernelSize, kvulve);

    kernelBuffer = std::make_unique<VulkanBuffer>();
    kernelBuffer->initResoure(
        BufferUsage::onestore,
        paramet.kernelSizeX * paramet.kernelSizeY * sizeof(float),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        (uint8_t*)karray.data());
}

VkGaussianBlurLayer::VkGaussianBlurLayer(ImageType imageType)
    : VkLinearFilterLayer(imageType) {}

VkGaussianBlurLayer::~VkGaussianBlurLayer() {}

void VkGaussianBlurLayer::onUpdateParamet() {
    if (paramet.blurRadius == oldParamet.blurRadius &&
        paramet.sigma == oldParamet.sigma) {
        return;
    }
    resetGraph();
}

void VkGaussianBlurLayer::onInitVkBuffer() {
    int ksize = paramet.blurRadius * 2 + 1;
    if (paramet.sigma <= 0.0f) {
        paramet.sigma = ((ksize - 1) * 0.5 - 1) * 0.3 + 0.8;
    }
    double scale = 1.0f / (paramet.sigma * paramet.sigma * 2.0);
    double cons = scale / M_PI;
    double sum = 0.0;
    std::vector<float> karray(ksize * ksize);
    for (int i = 0; i < ksize; i++) {
        for (int j = 0; j < ksize; j++) {
            int x = i - (ksize - 1) / 2;
            int y = j - (ksize - 1) / 2;
            karray[i * ksize + j] = cons * exp(-scale * (x * x + y * y));
            sum += karray[i * ksize + j];
        }
    }
    sum = 1.0 / sum;
    for (int i = ksize * ksize - 1; i >= 0; i--) {
        karray[i] *= sum;
    }
    kernelBuffer = std::make_unique<VulkanBuffer>();
    kernelBuffer->initResoure(
        BufferUsage::onestore, karray.size() * sizeof(float),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        (uint8_t*)karray.data());
    std::vector<int32_t> ubo = {ksize, ksize, paramet.blurRadius,
                                paramet.blurRadius};
    updateUBO(ubo.data());
}

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce