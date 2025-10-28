#include "VkHelper.hpp"

namespace aoce {
namespace vulkan {
namespace layer {

VkFormat ImageFormat2Vk(ImageType imageType) {
    switch (imageType) {
        case ImageType::bgra8:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case ImageType::r16:
            return VK_FORMAT_R16_UINT;
        case ImageType::r8:
            return VK_FORMAT_R8_UNORM;  // VK_FORMAT_R8_UINT VK_FORMAT_S8_UINT
                                        // VK_FORMAT_R8_UNORM
        case ImageType::rgba8:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case ImageType::rgba32f:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case ImageType::r32f:
            return VK_FORMAT_R32_SFLOAT;
        case ImageType::r32:
            return VK_FORMAT_R32_SINT;
        case ImageType::rgba32:
            return VK_FORMAT_R32G32B32A32_SINT;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

}  // namespace layer
}  // namespace vulkan
}  // namespace aoce