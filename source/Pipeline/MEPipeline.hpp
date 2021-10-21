#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <Window/MEWindow.hpp>

#include "Vertex.hpp"
#include "CommandBuffer/MECommandBuffer.hpp"


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


    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    void CopyBufferToImage(VkBuffer buffer,VkImage image, uint32_t width, uint32_t height);
    void CreateTextureImage();

    void CreateDescriptorSets();
    void CreateDescriptorPool();
    void CreateDescriptorSetLayout();

    void CreateTextureSampler();
    void CreateTextureImageView();
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


    void LoadModel();

    void CraeteCommandBuffers();

    void CreateUniformBuffers();
    void CreateIndexBuffer();
    void CreateVertexBuffer();
    void CreateSyncObjects();
    void RecordCommandBuffer(int imageIndex);
    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

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

    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    MECommandBuffer* commandBuffer;

    MEDevice& device;
    MEWindow& window;
};

}