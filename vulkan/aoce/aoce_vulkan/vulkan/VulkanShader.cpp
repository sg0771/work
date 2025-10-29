#include "VulkanShader.hpp"
#include <aoce/AoceManager.hpp>
#include "VulkanHelper.hpp"
#if __ANDROID__
#include "../android/vulkan_wrapper.h"
#endif
#include <filesystem>

namespace aoce {
namespace vulkan {

VulkanShader::VulkanShader(/* args */) {}

VulkanShader::~VulkanShader() { release(); }

void VulkanShader::release() {
    if (shaderModule) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        shaderModule = VK_NULL_HANDLE;
    }
}



void VulkanShader::loadShaderModule(VkDevice device, std::string path,
                                    VkShaderStageFlagBits shaderFlag) {
    this->device = device;
    release();
#if defined(__ANDROID__)
    AAssetManager* assetManager = AoceManager::Get().getAppEnv().assetManager;
    assert(assetManager != nullptr);
    shaderModule = loadShader(assetManager, path.c_str(), device);
#else
    std::string fullPath;
    
    // 先判断文件是否存在且为普通文件
    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path)) {
        // 获取绝对路径（会自动解析相对路径）
        std::filesystem::path abs_path = std::filesystem::absolute(path);
        // 或使用 canonical（会解析符号链接，要求文件必须存在）
        // fs::path abs_path = fs::canonical(file_path);
        fullPath = abs_path.string();
    }
    else {
        fullPath = getAocePath() + "/" + path;
    }
    

    shaderModule = loadShader(fullPath.c_str(), device);
#endif
    logAssert(shaderModule != VK_NULL_HANDLE,
              "file: " + path + " load shader failed");    
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = shaderFlag;
    shaderStage.pName = "main";
    shaderStage.module = shaderModule;
}

}  // namespace vulkan
}  // namespace aoce