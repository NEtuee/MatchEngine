#include "../public/MECommandBuffer.hpp"
#include <stdexcept>

namespace MatchEngine
{

MECommandBuffer::MECommandBuffer(MEDevice& device, MECommandPool & commandPool)
    :device(device), commandPool(commandPool)
{

}

MECommandBuffer::~MECommandBuffer()
{
    DestroyCommandBuffers();
}

void MECommandBuffer::CreateCommandBuffers(size_t size)
{
    commandBuffers.resize(size);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool.GetCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if(vkAllocateCommandBuffers(device.GetDevice(),&allocInfo,commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers");
    }
}

void MECommandBuffer::DestroyCommandBuffers()
{
    vkFreeCommandBuffers(device.GetDevice(),commandPool.GetCommandPool(),static_cast<uint32_t>(commandBuffers.size()),commandBuffers.data());
}

}