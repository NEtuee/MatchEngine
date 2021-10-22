#include "../public/MEDevice.hpp"
#include "../public/MEImage.hpp"

namespace MatchEngine
{

MEImage::MEImage(MEDevice* device)
    :device(device), create(false)
{

}

MEImage::~MEImage()
{
    DestroyImage();
}

void MEImage::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSample, VkFormat format, VkImageTiling tiling, 
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties,VkImageAspectFlags aspectFlags)
{
    device->CreateImage(width,height,mipLevels,numSample,format,tiling,usage,properties,image,imageMemory);
    imageView = device->CreateImageView(image,format,aspectFlags,mipLevels);

    create = true;
}

void MEImage::DestroyImage()
{
    if(create)
    {
        vkDestroyImageView(device->GetDevice(),imageView,nullptr);
        vkDestroyImage(device->GetDevice(),image,nullptr);
        vkFreeMemory(device->GetDevice(),imageMemory,nullptr);

        create = false;
    }
}

}