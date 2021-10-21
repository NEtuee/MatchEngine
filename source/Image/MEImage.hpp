#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
//#include <Device/MEDevice.hpp>

namespace MatchEngine
{

class MEDevice;
class MEImage
{

public:
    MEImage(MEDevice* device);
    ~MEImage();

    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, 
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties,VkImageAspectFlags aspectFlags);
    void DestroyImage();

    const VkImage& GetImage() {return image;}
    const VkImageView& GetImageView() {return imageView;}
private:
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;

    MEDevice* device;

    bool create;
};

}