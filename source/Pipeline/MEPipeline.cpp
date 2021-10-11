#include "MEPipeline.hpp"
#include "Vertex.hpp"
#include "UniformBufferObject.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan.h>
#include <cstring>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#define STB_IMAGE_IMPLEMENTAION
#include <stb_image.h>
#include <deprecated/stb_image.c>

namespace MatchEngine
{

MEPipeline::MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath)
            : device(device), window(window)
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline(vertPath,fragPath);
    CreateFrameBuffers();
    CreateCommandPool();
    CreateTextureImage();
    CreateTextureImageView();
    CreateTextureSampler();

    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CraeteCommandBuffers();
    CreateSyncObjects();
}

MEPipeline::~MEPipeline()
{
    CleanupSwapChain();

    vkDestroySampler(device.GetDevice(),textureSampler,nullptr);
    vkDestroyImageView(device.GetDevice(),textureImageView,nullptr);

    vkDestroyImage(device.GetDevice(),textureImage,nullptr);
    vkFreeMemory(device.GetDevice(),textureImageMemory, nullptr);

    vkDestroyDescriptorSetLayout(device.GetDevice(),descSetLayout,nullptr);

    vkDestroyBuffer(device.GetDevice(),vertexBuffer,nullptr);
    vkFreeMemory(device.GetDevice(), vertexBufferMemory,nullptr);

    vkDestroyBuffer(device.GetDevice(),indexBuffer,nullptr);
    vkFreeMemory(device.GetDevice(), indexBufferMemory,nullptr);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device.GetDevice(),renderFinishedSemaphores[i],nullptr);
        vkDestroySemaphore(device.GetDevice(),imageAvailableSemaphores[i],nullptr);
        vkDestroyFence(device.GetDevice(),inFlightFences[i],nullptr);
    }
    
    vkDestroyCommandPool(device.GetDevice(),commandPool,nullptr);
    
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

void MEPipeline::CraeteCommandBuffers()
{
    commandBuffers.resize(swapChainFrameBuffer.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if(vkAllocateCommandBuffers(device.GetDevice(),&allocInfo,commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for(size_t i = 0; i < commandBuffers.size(); ++i)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if(vkBeginCommandBuffer(commandBuffers[i],&beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = device.GetRenderPass();
        renderPassInfo.framebuffer = swapChainFrameBuffer[i];

        renderPassInfo.renderArea.offset = {0,0};
        renderPassInfo.renderArea.extent = device.GetExtend();

        VkClearValue clearColor = {{{0.0f,0.0f,0.0f,1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,graphicsPipeline);

        VkBuffer vertexBuffers[] = {vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[i],0,1,vertexBuffers,offsets);
        vkCmdBindIndexBuffer(commandBuffers[i],indexBuffer,0,VK_INDEX_TYPE_UINT16);

        //vkCmdDraw(commandBuffers[i],static_cast<uint32_t>(vertices.size()),1,0,0);
        vkCmdBindDescriptorSets(commandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&descriptorSets[i],0,nullptr);
        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()),1,0,0,0);
        vkCmdEndRenderPass(commandBuffers[i]);
        if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer");
        }
    }

    
}

void MEPipeline::DrawFrame()
{
    vkWaitForFences(device.GetDevice(),1,&inFlightFences[currentFrame],VK_TRUE,UINT64_MAX);
    vkResetFences(device.GetDevice(),1,&inFlightFences[currentFrame]);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(device.GetDevice(), device.GetSwapchain(),UINT64_MAX,imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    if(imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device.GetDevice(),1,&imagesInFlight[imageIndex],VK_TRUE,UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    UpdateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.GetDevice(),1,&inFlightFences[currentFrame]);

    if(vkQueueSubmit(device.GetGraphicsQueue(),1,&submitInfo,inFlightFences[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {device.GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    result = vkQueuePresentKHR(device.GetPresentQueue(),&presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.framebufferResized)
    {
        window.framebufferResized = false;
        RecreateSwapChain();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void MEPipeline::UpdateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float,std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    auto extend = device.GetExtend();
    ubo.model = glm::rotate(glm::mat4(1.0f),time * glm::radians(90.0f),glm::vec3(.0f,.0f,1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.f,0.f,0.f),glm::vec3(0.f,0.f,1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),extend.width / (float)extend.height,0.1f,10.f);
    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(device.GetDevice(), uniformBuffersMemory[currentImage],0,sizeof(ubo),0,&data);
    memcpy(data,&ubo,sizeof(ubo));
    vkUnmapMemory(device.GetDevice(), uniformBuffersMemory[currentImage]);
}

void MEPipeline::RecreateSwapChain()
{
    vkDeviceWaitIdle(device.GetDevice());

    CleanupSwapChain();
    device.CleanupSwapChain();

    device.RecreateSwapChain();
    CreateGraphicsPipeline(vertPath,fragPath);

    CreateFrameBuffers();
    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CraeteCommandBuffers();
}

void MEPipeline::CleanupSwapChain()
{
    // for(auto framebuffer : swapChainFrameBuffer)
    // {
    //     vkDestroyFramebuffer(device.GetDevice(),framebuffer,nullptr);
    // }
    for(size_t i = 0; i < swapChainFrameBuffer.size(); ++i)
    {
        vkDestroyFramebuffer(device.GetDevice(),swapChainFrameBuffer[i],nullptr);
    }

    vkFreeCommandBuffers(device.GetDevice(),commandPool,static_cast<uint32_t>(commandBuffers.size()),commandBuffers.data());

    vkDestroyPipeline(device.GetDevice(), graphicsPipeline,nullptr);
    vkDestroyPipelineLayout(device.GetDevice(),pipelineLayout,nullptr);

    for(size_t i = 0; i < device.GetSwapChainImages().size(); ++i)
    {
        vkDestroyBuffer(device.GetDevice(),uniformBuffers[i],nullptr);
        vkFreeMemory(device.GetDevice(),uniformBuffersMemory[i],nullptr);
    }

    vkDestroyDescriptorPool(device.GetDevice(),descriptorPool, nullptr);
}

void MEPipeline::CopyBufferToImage(VkBuffer buffer,VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0,0,0};
    region.imageExtent = 
    {
        width, height, 1
    };

    vkCmdCopyBufferToImage(commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    EndSingleTimeCommands(commandBuffer);
}

void MEPipeline::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition");
    }

    vkCmdPipelineBarrier(commandBuffer, 
        sourceStage, destinationStage,
        0,
        0,nullptr,
        0,nullptr,
        1,&barrier);

    EndSingleTimeCommands(commandBuffer);



}

void MEPipeline::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    if(vkCreateImage(device.GetDevice(),&imageInfo,nullptr,&image) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.GetDevice(),image,&memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirements.memoryTypeBits, properties);

    if(vkAllocateMemory(device.GetDevice(),&allocInfo,nullptr,&imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate image memory");
    }

    vkBindImageMemory(device.GetDevice(), image, imageMemory, 0);
}

void MEPipeline::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device.GetPhysicalDevice(),&properties);

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if(vkCreateSampler(device.GetDevice(),&samplerInfo,nullptr,&textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler");
    }
}

VkImageView MEPipeline::CreateImageView(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if(vkCreateImageView(device.GetDevice(),&viewInfo,nullptr,&imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture image view");
    }

    return imageView;
}

void MEPipeline::CreateTextureImageView()
{
    textureImageView = CreateImageView(textureImage,VK_FORMAT_R8G8B8A8_SRGB);
}

void MEPipeline::CreateTextureImage()
{
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load("../Textures/testImage.png",&texWidth,&texHeight,&texChannels,STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if(!pixels)
    {
        throw std::runtime_error("failed to load texture image");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(imageSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device.GetDevice(),stagingBufferMemory,0,imageSize,0,&data);
    memcpy(data,pixels,static_cast<size_t>(imageSize));
    vkUnmapMemory(device.GetDevice(),stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage,textureImageMemory);

    TransitionImageLayout(textureImage,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer,textureImage,static_cast<uint32_t>(texWidth),static_cast<uint32_t>(texHeight));

    TransitionImageLayout(textureImage,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device.GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device.GetDevice(),stagingBufferMemory,nullptr);
}

void MEPipeline::CreateDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(device.GetSwapChainImages().size(),descSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(device.GetSwapChainImages().size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(device.GetSwapChainImages().size());
    if(vkAllocateDescriptorSets(device.GetDevice(),&allocInfo,descriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for(size_t i = 0; i < device.GetSwapChainImages().size(); ++i)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet,2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;


        vkUpdateDescriptorSets(device.GetDevice(),static_cast<uint32_t>(descriptorWrites.size()),descriptorWrites.data(),0,nullptr);
    }
}

void MEPipeline::CreateDescriptorPool()
{
    // VkDescriptorPoolSize poolSize{};
    // poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // poolSize.descriptorCount = static_cast<uint32_t>(device.GetSwapChainImages().size());

    std::array<VkDescriptorPoolSize,2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(device.GetSwapChainImages().size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(device.GetSwapChainImages().size());
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(device.GetSwapChainImages().size());

    if(vkCreateDescriptorPool(device.GetDevice(),&poolInfo,nullptr,&descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void MEPipeline::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding,2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(device.GetDevice(),&layoutInfo, nullptr,&descSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    // VkPipelineLayoutCreateInfo pipelineCreateInfo{};
    // pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineCreateInfo.setLayoutCount = 1;
    // pipelineCreateInfo.pSetLayouts = &descSetLayout;

}

VkCommandBuffer MEPipeline::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device.GetDevice(),&allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void MEPipeline::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.GetGraphicsQueue(),1,&submitInfo,VK_NULL_HANDLE);
    vkQueueWaitIdle(device.GetGraphicsQueue());

    vkFreeCommandBuffers(device.GetDevice(),commandPool,1,&commandBuffer);
}


void MEPipeline::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    auto commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,1,&copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

void MEPipeline::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if(vkCreateBuffer(device.GetDevice(),&bufferInfo,nullptr,&buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirments;
    vkGetBufferMemoryRequirements(device.GetDevice(),buffer,&memRequirments);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirments.size;
    allocInfo.memoryTypeIndex = device.FindMemoryType(memRequirments.memoryTypeBits,properties);
    
    if(vkAllocateMemory(device.GetDevice(),&allocInfo,nullptr,&bufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate buffer memory");
    }

    vkBindBufferMemory(device.GetDevice(),buffer,bufferMemory,0);
}

void MEPipeline::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(device.GetSwapChainImages().size());
    uniformBuffersMemory.resize(device.GetSwapChainImages().size());

    for(size_t i = 0; i < device.GetSwapChainImages().size(); ++i)
    {
        CreateBuffer(bufferSize,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],uniformBuffersMemory[i]);

    }
}

void MEPipeline::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device.GetDevice(),stagingBufferMemory,0,bufferSize,0,&data);
    memcpy(data,indices.data(),(size_t)bufferSize);
    vkUnmapMemory(device.GetDevice(),stagingBufferMemory);

    CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer,indexBufferMemory);

    CopyBuffer(stagingBuffer,indexBuffer,bufferSize);

    vkDestroyBuffer(device.GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device.GetDevice(),stagingBufferMemory,nullptr);
}

void MEPipeline::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device.GetDevice(),stagingBufferMemory,0,bufferSize,0,&data);
    memcpy(data,vertices.data(),(size_t)bufferSize);
    vkUnmapMemory(device.GetDevice(),stagingBufferMemory);

    CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer,vertexBufferMemory);

    CopyBuffer(stagingBuffer,vertexBuffer,bufferSize);

    vkDestroyBuffer(device.GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device.GetDevice(),stagingBufferMemory,nullptr);
}

void MEPipeline::CreateSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(device.GetSwapChainImages().size(),VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if(vkCreateSemaphore(device.GetDevice(),&semaphoreInfo,nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.GetDevice(),&semaphoreInfo,nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.GetDevice(),&fenceInfo,nullptr,&inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create sync objects for frame");
        }
    }

    
}

void MEPipeline::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = device.GetQueueFamiliyIndices();
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if(vkCreateCommandPool(device.GetDevice(),&poolInfo,nullptr,&commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }
}

void MEPipeline::CreateFrameBuffers()
{
    swapChainFrameBuffer.resize(device.GetSwapChainImageViews().size());

    for(size_t i = 0; i < device.GetSwapChainImageViews().size(); ++i)
    {
        VkImageView attachments[] = 
        {
            device.GetSwapChainImageViews()[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = device.GetRenderPass();
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = device.GetExtend().width;
        framebufferInfo.height = device.GetExtend().height;
        framebufferInfo.layers = 1;

        if(vkCreateFramebuffer(device.GetDevice(),&framebufferInfo,nullptr,&swapChainFrameBuffer[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}

void MEPipeline::CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath)
{
    this->vertPath = vertPath;
    this->fragPath = fragPath;

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributeDesc = Vertex::GetAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)device.GetExtend().width;
    viewport.height = (float)device.GetExtend().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = device.GetExtend();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = device.GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if(vkCreateGraphicsPipelines(device.GetDevice(),VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

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