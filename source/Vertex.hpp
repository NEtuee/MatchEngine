#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription bindingDesc{};
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(Vertex);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDesc;
    }

    static std::array<VkVertexInputAttributeDescription,3> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription,3> attributeDesc{};

        attributeDesc[0].binding = 0;
        attributeDesc[0].location = 0;
        attributeDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDesc[0].offset = offsetof(Vertex,pos);

        attributeDesc[1].binding = 0;
        attributeDesc[1].location = 1;
        attributeDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDesc[1].offset = offsetof(Vertex,color);

        attributeDesc[2].binding = 0;
        attributeDesc[2].location = 2;
        attributeDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDesc[2].offset = offsetof(Vertex,texCoord);

        return attributeDesc;
    }
};

const std::vector<Vertex> vertices = 
{
    {{-.5f,-.5f,0.0f}, {1.0f,.0f,.0f},{1.0f,0.0f}},
    {{.5f,-.5f,0.0f}, {0.0f,1.0f,.0f},{0.0f,0.0f}},
    {{.5f,.5f,0.0f}, {0.0f,.0f,1.0f},{0.0f,1.0f}},
    {{-.5f,.5f,0.0f}, {1.0f,1.0f,1.0f},{1.0f,1.0f}},

    {{-.5f,-.5f,-0.5f}, {1.0f,.0f,.0f},{1.0f,0.0f}},
    {{.5f,-.5f,-0.5f}, {0.0f,1.0f,.0f},{0.0f,0.0f}},
    {{.5f,.5f,-0.5f}, {0.0f,.0f,1.0f},{0.0f,1.0f}},
    {{-.5f,.5f,-0.5f}, {1.0f,1.0f,1.0f},{1.0f,1.0f}},
};

const std::vector<uint16_t> indices =
{
    0,1,2,2,3,0,
    4,5,6,6,7,4
};