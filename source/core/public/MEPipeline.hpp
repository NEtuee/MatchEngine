#pragma once

#include <string>
#include <vector>
#include "MEDevice.hpp"
#include "MEWindow.hpp"

#include "../Vertex.hpp"
#include "MESwapchain.hpp"
#include "METexture.hpp"
#include "MEModel.hpp"


namespace MatchEngine
{
const int MAX_FRAMES_IN_FLIGHT = 2;

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, MEWindow& window, MESwapchain& swapchain, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();

    void DrawFrame();
    void UpdateUniformBuffer(uint32_t currentImage,float plus);
    void RecreateSwapChain();
    void CleanupSwapChain();

    void RecordCommandBuffer(int imageIndex);

    uint32_t BeginRender();
    void EndRender(uint32_t imageIndex);

    VkCommandBuffer& BindPipeline(int imageIndex);
    void EndPipeline(VkCommandBuffer& commandBuffer);
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateDescriptorSets();
    void CreateDescriptorPool();
    void CreateDescriptorSetLayout();

    void CreateUniformBuffers();
    void CreateSyncObjects();
    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    uint32_t mipLevels;
    

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetLayout descSetLayout;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    std::string vertPath;
    std::string fragPath;

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    MEDevice& device;
    MEWindow& window;
    MESwapchain& swapchain;

    METexture* texture;
    MEModel* model;

    MECommandBuffer* commandBuffer;
    void CraeteCommandBuffers();
};

}