#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <Window/MEWindow.hpp>

namespace MatchEngine
{

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();

    void DrawFrame();
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateSemaphores();
    void CraeteCommandBuffers();
    void CreateCommandPool();
    void CreateFrameBuffers();
    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkFramebuffer> swapChainFrameBuffer;

    VkCommandPool commandPool;
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;

    MEDevice& device;
    MEWindow& window;
};

}