#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
//#include <Device/MEDevice.hpp>


namespace MatchEngine
{
class MEDevice;
class MECommandPool
{

public:
    MECommandPool(MEDevice* device);
    ~MECommandPool();

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    const VkCommandPool& GetCommandPool() {return commandPool;}
private:
    void CreateCommandPool();

    VkCommandPool commandPool;
    MEDevice* device;
};

}