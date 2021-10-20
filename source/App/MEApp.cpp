#include "MEApp.hpp"

namespace MatchEngine
{

void MEApp::Run()
{
    while(!meWindow.ShouldClose())
    {
        glfwPollEvents();
        mePipeline.DrawFrame();
    }

    vkDeviceWaitIdle(meDevice.GetDevice());
}

void MEApp::RecordCommandBuffer(int imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if(vkBeginCommandBuffer(commandBuffers[imageIndex],&beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = device.GetRenderPass();
    renderPassInfo.framebuffer = swapChainFrameBuffer[imageIndex];

    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = device.GetExtend();

    std::array<VkClearValue,2> clearValues{};
    //VkClearValue clearColor = {{{0.0f,0.0f,0.0f,1.0f}}};
    clearValues[0].color = {{0.0f,0.0f,0.0f,1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[imageIndex],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[imageIndex],VK_PIPELINE_BIND_POINT_GRAPHICS,graphicsPipeline);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffers[imageIndex],0,1,vertexBuffers,offsets);
    vkCmdBindIndexBuffer(commandBuffers[imageIndex],indexBuffer,0,VK_INDEX_TYPE_UINT32);

    //vkCmdDraw(commandBuffers[i],static_cast<uint32_t>(vertices.size()),1,0,0);
    vkCmdBindDescriptorSets(commandBuffers[imageIndex],VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&descriptorSets[imageIndex],0,nullptr);
    vkCmdDrawIndexed(commandBuffers[imageIndex], static_cast<uint32_t>(indices.size()),1,0,0,0);
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    if(vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }
}

void MEApp::CraeteCommandBuffers()
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
}

void MEApp::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = meDevice.GetQueueFamiliyIndices();
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if(vkCreateCommandPool(meDevice.GetDevice(),&poolInfo,nullptr,&commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool");
    }
}

void MEApp::DrawFrame()
{
    vkWaitForFences(meDevice.GetDevice(),1,&inFlightFences[currentFrame],VK_TRUE,UINT64_MAX);
    //vkResetFences(device.GetDevice(),1,&inFlightFences[currentFrame]);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(meDevice.GetDevice(), device.GetSwapchain(),UINT64_MAX,imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &imageIndex);

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

    RecordCommandBuffer(imageIndex);

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

void MEApp::CreateSyncObjects()
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

}