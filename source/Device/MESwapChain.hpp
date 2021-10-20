#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Window/MEWindow.hpp>

#include "MEDevice.hpp"
#include "../Window/MEWindow.hpp"

namespace MatchEngine
{

class MESwapChain
{
public:
    MESwapChain(MEDevice& device, MEWindow& window);
    ~MESwapChain();

    void RecreateSwapChain();
    void CleanupSwapChain();
private:
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();

    void CreateFrameBuffers();
    void CreateColorResources();
    void CreateDepthResources();

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    std::vector<VkFramebuffer> swapChainFrameBuffer;


    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapChain;

    VkRenderPass renderPass;

    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    MEDevice& device;
    MEWindow& window;
};

}