#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <Window/MEWindow.hpp>

namespace MatchEngine
{
const int MAX_FRAMES_IN_FLIGHT = 2;

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();

    void DrawFrame();
    void RecreateSwapChain();
    void CleanupSwapChain();
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateSyncObjects();
    void CraeteCommandBuffers();
    void CreateCommandPool();
    void CreateFrameBuffers();
    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    std::string vertPath;
    std::string fragPath;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> swapChainFrameBuffer;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    VkCommandPool commandPool;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    MEDevice& device;
    MEWindow& window;
};

}