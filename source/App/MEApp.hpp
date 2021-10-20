#pragma once

#include <Window/MEWindow.hpp>
#include <Device/MEDevice.hpp>
#include <Pipeline/MEPipeline.hpp>

namespace MatchEngine
{

const int MAX_FRAMES_IN_FLIGHT = 2;

class MEApp
{
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    void Run();
    void DrawFrame();
private:
    MEWindow meWindow{WIDTH,HEIGHT,"MyWindow"};
    MEDevice meDevice{meWindow};
    MEPipeline mePipeline{meDevice,meWindow,"../Shaders/Simple.vert.spv","../Shaders/Simple.frag.spv"};

    void RecordCommandBuffer(int imageIndex);
    void CraeteCommandBuffers();
    void CreateCommandPool();
    void CreateSyncObjects();

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
};

}