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

namespace MatchEngine
{

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class MEDevice
{
public:
    MEDevice(MEWindow & window);
    ~MEDevice();

    bool HasStencilComponent(VkFormat format);
    uint32_t FindMemoryType(uint32_t typeFilter,VkMemoryPropertyFlags properties);
    VkFormat FindDepthFormat();
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,VkImageTiling tiling,VkFormatFeatureFlags features);

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, 
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,uint32_t mipLevels);

    //const VkFormat GetSwapChainImageFormat() {return swapChainImageFormat;}
    const VkPhysicalDevice& GetPhysicalDevice() {return physicalDevice;}
    const VkDevice& GetDevice(){return device;}
    //const VkExtent2D& GetExtend() {return swapChainExtent;}
    //const VkRenderPass& GetRenderPass() {return renderPass;}
    //const VkSwapchainKHR& GetSwapchain() {return swapChain;}
    const VkQueue& GetGraphicsQueue() {return graphicsQueue;}
    const VkQueue& GetPresentQueue() {return presentQueue;}
    //const std::vector<VkImageView>& GetSwapChainImageViews() {return swapChainImageViews;}
    //const std::vector<VkImage>& GetSwapChainImages() {return swapChainImages;}
    const QueueFamilyIndices GetQueueFamiliyIndices() {return FindQueueFamilies(physicalDevice);}
    const VkSurfaceKHR& GetSurface() {return surface;}
private:
    void InitVulkan();
    void CreateInstance();
    void CreateLogicalDevice();
    
    VkSampleCountFlagBits GetMaxUsableSampleCount();

    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    void CreateSurface();

    void SetupDebugMessenger();
    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool CheckValidationLayerSupport();
    std::vector<const char*> GetRequiredExtensions();

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
        const VkAllocationCallbacks* pAllocator, 
        VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
        VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    MEWindow & window;

    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;



};

}