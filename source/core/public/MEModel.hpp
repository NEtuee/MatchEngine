#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include "../Vertex.hpp"

namespace MatchEngine
{

class MEDevice;
class MEModel
{
public:
    MEModel(MEDevice* device);
    ~MEModel();

    void BindModel(VkCommandBuffer& commandBuffer);

    void CreateModel(std::string path);
    void LoadModel(std::string path);
    void CreateIndexBuffer();
    void CreateVertexBuffer();

    const uint32_t GetIndicesSize() {return indices.size();} 

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    MEDevice* device;
};

}