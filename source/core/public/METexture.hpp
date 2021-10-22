#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "METexture.hpp"

namespace MatchEngine
{

class MEDevice;
class METexture : public MEImage
{

public:
    METexture(MEDevice* device);
    ~METexture();

    void CreateTexture(std::string path);
    void CreateSampler();

    const uint32_t& GetMipLevels() {return mipLevels;}
    const VkSampler& GetSampler() {return textureSampler;}
protected:
    uint32_t mipLevels;
    VkSampler textureSampler;
};

}