#pragma once

#include <string>
#include <vector>
#include "MEDevice.hpp"
#include "MECommandPool.hpp"

namespace MatchEngine
{

class MECommandBuffer
{

public:
    MECommandBuffer(MEDevice& device, MECommandPool & commandPool);
    ~MECommandBuffer();

    void CreateCommandBuffers(size_t size);
    void DestroyCommandBuffers();

    VkCommandBuffer& GetCommandBuffer(int target) {return commandBuffers[target];}
private:
    std::vector<VkCommandBuffer> commandBuffers;

    MEDevice& device;
    MECommandPool& commandPool;
};

}