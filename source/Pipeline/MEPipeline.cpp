#include "MEPipeline.hpp"
#include "Vertex.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan.h>
#include <cstring>

namespace MatchEngine
{

MEPipeline::MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath)
            : device(device), window(window)
{
    CreateGraphicsPipeline(vertPath,fragPath);
    CreateFrameBuffers();
    CreateCommandPool();

    CreateVertexBuffer();
    CraeteCommandBuffers();
    CreateSyncObjects();
}

MEPipeline::~MEPipeline()
{
    CleanupSwapChain();

    vkDestroyBuffer(device.GetDevice(),vertexBuffer,nullptr);
    vkFreeMemory(device.GetDevice(), vertexBufferMemory,nullptr);

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

        vkCmdDraw(commandBuffers[i],static_cast<uint32_t>(vertices.size()),1,0,0);
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

void MEPipeline::RecreateSwapChain()
{
    vkDeviceWaitIdle(device.GetDevice());

    CleanupSwapChain();
    device.CleanupSwapChain();

    device.RecreateSwapChain();
    CreateGraphicsPipeline(vertPath,fragPath);
    CreateFrameBuffers();
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
}

void MEPipeline::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
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

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer,srcBuffer,dstBuffer,1,&copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(device.GetGraphicsQueue(),1,&submitInfo,VK_NULL_HANDLE);
    vkQueueWaitIdle(device.GetGraphicsQueue());

    vkFreeCommandBuffers(device.GetDevice(),commandPool,1,&commandBuffer);
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
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineLayoutInfo.setLayoutCount = 0;
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