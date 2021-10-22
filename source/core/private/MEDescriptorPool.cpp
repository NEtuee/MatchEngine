#include "../public/MEDescriptorPool.hpp"
#include <stdexcept>

namespace MatchEngine
{

MEDescriptorPool::Builder& MEDescriptorPool::Builder::AddPoolSize(VkDescriptorType type, uint32_t size)
{
    poolSizes.push_back({type,size});
    return *this;
}
MEDescriptorPool::Builder& MEDescriptorPool::Builder::SetMaxSets(uint32_t set)
{
    maxSets = set;
    return *this;
}
MEDescriptorPool::Builder& MEDescriptorPool::Builder::SetFlags(VkDescriptorPoolCreateFlags flags)
{
    createFlags = flags;
    return *this;
}
MEDescriptorPool* MEDescriptorPool::Builder::Build()
{
    return new MEDescriptorPool(device,poolSizes,maxSets,createFlags);
}


MEDescriptorPool::MEDescriptorPool(MEDevice& device,
        std::vector<VkDescriptorPoolSize> poolSizes,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags createFlags)
            :device(device)
{
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;
    poolInfo.flags = createFlags;

    if(vkCreateDescriptorPool(device.GetDevice(),&poolInfo,nullptr,&descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool");
    }
}

MEDescriptorPool::~MEDescriptorPool()
{
    DestroyDescriptorPool();
}

void MEDescriptorPool::AllocateDescriptorSet(VkDescriptorSetLayout& layout,VkDescriptorSet* set)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
    allocInfo.pSetLayouts = &layout;

    if(vkAllocateDescriptorSets(device.GetDevice(),&allocInfo,set) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets");
    }
}

void MEDescriptorPool::DestroyDescriptorPool()
{
    vkDestroyDescriptorPool(device.GetDevice(),descriptorPool, nullptr);
}

}