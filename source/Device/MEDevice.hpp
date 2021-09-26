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

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool IsComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class MEDevice
{
public:
    MEDevice(MEWindow & window);
    ~MEDevice();

    const VkDevice& GetDevice(){return device;}
    const VkExtent2D& GetExtend() {return swapChainExtent;}
    const VkRenderPass& GetRenderPass() {return renderPass;}
    const std::vector<VkImageView>& GetSwapChainImageViews() {return swapChainImageViews;}
    const QueueFamilyIndices GetQueueFamiliyIndices() {return FindQueueFamilies(physicalDevice);}
private:
    void InitVulkan();
    void CreateInstance();

    void CreateLogicalDevice();

    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

    void CreateRenderPass();

    void CreateImageViews();

    void CreateSurface();
    void CreateSwapChain();
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

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

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    VkSwapchainKHR swapChain;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

};

}