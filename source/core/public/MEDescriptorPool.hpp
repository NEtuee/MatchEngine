#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <memory>

#include "MEDevice.hpp"

namespace MatchEngine
{

class MEDescriptorPool
{
public:
    class Builder
    {
    public:
        Builder(MEDevice& device) : device(device){}
        Builder& AddPoolSize(VkDescriptorType type, uint32_t size);
        Builder& SetMaxSets(uint32_t set);
        Builder& SetFlags(VkDescriptorPoolCreateFlags flags);
        MEDescriptorPool* Build();
    private:
        MEDevice& device;
        std::vector<VkDescriptorPoolSize> poolSizes;
        uint32_t maxSets = 1000;
        VkDescriptorPoolCreateFlags createFlags = 0;
    };

    MEDescriptorPool(MEDevice& device,
        std::vector<VkDescriptorPoolSize> poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags createFlags);
    ~MEDescriptorPool();

    void AllocateDescriptorSet(VkDescriptorSetLayout& layout,VkDescriptorSet* set);
    void DestroyDescriptorPool();

    inline VkDescriptorPool& GetDescriptorPool() {return descriptorPool;}

private:
    MEDevice& device;
    VkDescriptorPool descriptorPool;
};

}