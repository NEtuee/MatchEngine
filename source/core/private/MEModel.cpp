#include "../public/MEModel.hpp"
#include "../public/MEDevice.hpp"

#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace MatchEngine
{

MEModel::MEModel(MEDevice* device)
    :device(device)
{

}

MEModel::~MEModel()
{
    const auto& dv = device->GetDevice();

    vkDestroyBuffer(dv,vertexBuffer,nullptr);
    vkFreeMemory(dv, vertexBufferMemory,nullptr);

    vkDestroyBuffer(dv,indexBuffer,nullptr);
    vkFreeMemory(dv, indexBufferMemory,nullptr);
}

void MEModel::BindModel(VkCommandBuffer& commandBuffer)
{
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer,0,1,vertexBuffers,offsets);
    vkCmdBindIndexBuffer(commandBuffer,indexBuffer,0,VK_INDEX_TYPE_UINT32);
}

void MEModel::CreateModel(std::string path)
{
    LoadModel(path);
    CreateIndexBuffer();
    CreateVertexBuffer();
}

void MEModel::LoadModel(std::string path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if(!tinyobj::LoadObj(&attrib,&shapes,&materials,&err,path.c_str()))
    {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex,uint32_t> uniqueVertices{};

    for(const auto& shape : shapes)
    {
        for(const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            vertex.pos = 
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = 
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f,1.0f,1.0f};

            if(uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            
            indices.push_back(uniqueVertices[vertex]);// indices.size());

            // indices.push_back(indices.size());
            // vertices.push_back(vertex);
        }
    }
}

void MEModel::CreateIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device->CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device->GetDevice(),stagingBufferMemory,0,bufferSize,0,&data);
    memcpy(data,indices.data(),(size_t)bufferSize);
    vkUnmapMemory(device->GetDevice(),stagingBufferMemory);

    device->CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer,indexBufferMemory);

    device->CopyBuffer(stagingBuffer,indexBuffer,bufferSize);

    vkDestroyBuffer(device->GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device->GetDevice(),stagingBufferMemory,nullptr);
}

void MEModel::CreateVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device->CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device->GetDevice(),stagingBufferMemory,0,bufferSize,0,&data);
    memcpy(data,vertices.data(),(size_t)bufferSize);
    vkUnmapMemory(device->GetDevice(),stagingBufferMemory);

    device->CreateBuffer(bufferSize,VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer,vertexBufferMemory);

    device->CopyBuffer(stagingBuffer,vertexBuffer,bufferSize);

    vkDestroyBuffer(device->GetDevice(),stagingBuffer,nullptr);
    vkFreeMemory(device->GetDevice(),stagingBufferMemory,nullptr);
}

}