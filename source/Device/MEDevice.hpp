#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

namespace MatchEngine
{

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;

    bool IsComplete()
    {
        return graphicsFamily.has_value();
    }
};

class MEDevice
{
public:
    MEDevice();
    ~MEDevice();
private:
    void InitVulkan();
    void CreateInstance();

    void CreateLogicalDevice();

    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    
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

    VkQueue graphicsQueue;
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
};

}