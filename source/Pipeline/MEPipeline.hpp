#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <Window/MEWindow.hpp>

#include "Vertex.hpp"

namespace MatchEngine
{
const int MAX_FRAMES_IN_FLIGHT = 2;

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();

    void DrawFrame();
    void UpdateUniformBuffer(uint32_t currentImage);
    void RecreateSwapChain();
    void CleanupSwapChain();
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateColorResources();

    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void CopyBufferToImage(VkBuffer buffer,VkImage image, uint32_t width, uint32_t height);
    void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,uint32_t mipLevels);
    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void CreateTextureImage();

    void CreateDescriptorSets();
    void CreateDescriptorPool();
    void CreateDescriptorSetLayout();

    void CreateTextureSampler();
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,uint32_t mipLevels);
    void CreateTextureImageView();
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void CreateDepthResources();

    void LoadModel();

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CreateUniformBuffers();
    void CreateIndexBuffer();
    void CreateVertexBuffer();
    void CreateSyncObjects();
    void CraeteCommandBuffers();
    void CreateCommandPool();
    void CreateFrameBuffers();
    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    uint32_t mipLevels;
    VkImageView textureImageView;
    VkSampler textureSampler;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetLayout descSetLayout;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

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