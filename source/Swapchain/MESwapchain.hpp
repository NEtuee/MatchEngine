#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <memory>

#include "Device/MEDevice.hpp"
#include "Window/MEWindow.hpp"
#include "CommandBuffer/MECommandBuffer.hpp"

namespace MatchEngine
{

class MESwapchain
{
public:
    MESwapchain(MEDevice& device, MEWindow& window);
    ~MESwapchain();

    void RecreateSwapChain();
    void CleanupSwapChain();

    inline size_t GetSwapchainImagesSize() {return swapChainImages.size();}
    inline size_t GetSwapchainFrameBufferSize() {return swapChainFrameBuffer.size();}
    inline const VkSwapchainKHR& GetSwapchain() {return swapChain;}
    inline const VkExtent2D& GetSwapchainExtent() {return swapChainExtent;}
    inline const VkRenderPass& GetRenderPass() {return renderPass;}
    inline const std::vector<VkFramebuffer>& GetSwapchainFrameBuffer() {return swapChainFrameBuffer;}
    
private:
    void CreateSwapChain();
    void CreateSwapChainImageViews();
    void CreateRenderPass();
    void CreateFrameBuffers();
    void CreateColorResources();
    void CreateDepthResources();

    MEDevice& device;
    MEWindow& window;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFrameBuffer;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapChain;
    VkRenderPass renderPass;

    std::unique_ptr<MEImage> colorImage;
    std::unique_ptr<MEImage> depthImage;

};


}