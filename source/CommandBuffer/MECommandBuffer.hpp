#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <CommandBuffer/MECommandPool.hpp>

namespace MatchEngine
{

class MECommandBuffer
{

public:
    MECommandBuffer(MEDevice& device, MECommandPool & commandPool);
    ~MECommandBuffer();

    void RecordCommandBuffer(int imageIndex);
    void CreateCommandBuffers(size_t size);
    void DestroyCommandBuffers();

    VkCommandBuffer& GetCommandBuffer(int target) {return commandBuffers[target];}
private:
    std::vector<VkCommandBuffer> commandBuffers;

    MEDevice& device;
    MECommandPool& commandPool;
};

}