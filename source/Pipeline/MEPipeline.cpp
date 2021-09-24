#include "MEPipeline.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan.h>

namespace MatchEngine
{

MEPipeline::MEPipeline(MEDevice& device, const std::string& vertPath, const std::string& fragPath) : device(device)
{
    CreateGraphicsPipeline(vertPath,fragPath);
}

MEPipeline::~MEPipeline()
{
    
}


std::vector<char> MEPipeline::ReadFile(const std::string & path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("failed to open file : " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void MEPipeline::CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath)
{
    auto vert = ReadFile(vertPath);
    auto frag = ReadFile(fragPath);

    auto vertShaderModule = CreateShaderModule(vert);
    auto fragShaderModule = CreateShaderModule(frag);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


    vkDestroyShaderModule(device.GetDevice(), vertShaderModule,nullptr);
    vkDestroyShaderModule(device.GetDevice(), fragShaderModule,nullptr);
}

VkShaderModule MEPipeline::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device.GetDevice(),&createInfo,nullptr,&shaderModule))
    {
        throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
}

}