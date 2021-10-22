#include <stdexcept>

#include "../public/MEDevice.hpp"
#include "../public/METexture.hpp"

#define STB_IMAGE_IMPLEMENTAION
#include <stb_image.h>
#include <deprecated/stb_image.c>

namespace MatchEngine
{

METexture::METexture(MEDevice* device)
    :MEImage(device)
{

}

METexture::~METexture()
{
    vkDestroySampler(device->GetDevice(),textureSampler,nullptr);
}

void METexture::CreateTexture(std::string path)
{
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(path.c_str(),&texWidth,&texHeight,&texChannels,STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth,texHeight)))) + 1;

    if(!pixels)
    {
        throw std::runtime_error("failed to load texture image");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    device->CreateBuffer(imageSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device->GetDevice(),stagingBufferMemory,0,imageSize,0,&data);
    memcpy(data,pixels,static_cast<size_t>(imageSize));
    vkUnmapMemory(device->GetDevice(),stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,VK_IMAGE_ASPECT_COLOR_BIT);

    device->TransitionImageLayout(image,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,mipLevels);
    device->CopyBufferToImage(stagingBuffer,image,static_cast<uint32_t>(texWidth),static_cast<uint32_t>(texHeight));

    //TransitionImageLayout(textureImage,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,mipLevels);

    device->GenerateMipmaps(image,VK_FORMAT_R8G8B8A8_SRGB,texWidth,texHeight,mipLevels);

    vkDestroyBuffer(device->GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device->GetDevice(),stagingBufferMemory,nullptr);

    CreateSampler();
}

void METexture::CreateSampler()
{
    device->CreateTextureSampler(textureSampler,mipLevels);
}

}