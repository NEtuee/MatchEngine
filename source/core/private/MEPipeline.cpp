#include "../public/MEPipeline.hpp"
#include "../UniformBufferObject.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan.h>
#include <cstring>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>


namespace MatchEngine
{

const std::string MODEL_PATH = "../Models/viking_room.obj";
const std::string TEXTURE_PATH = "../Textures/viking_room.png";


MEPipeline::MEPipeline(MEDevice& device, MEWindow& window, MESwapchain& swapchain, const std::string& vertPath, const std::string& fragPath)
            : device(device), window(window), swapchain(swapchain)
{
    CreateDescriptorSetLayout();
    CreateGraphicsPipeline(vertPath,fragPath);

    commandBuffer = new MECommandBuffer(device,device.GetCommandPool());
    CraeteCommandBuffers();

    texture = new METexture(&device);
    texture->CreateTexture(TEXTURE_PATH);

    newImage = new METexture(&device);
    newImage->CreateTexture("../Textures/testImage.png");

    model = new MEModel(&device);
    model->CreateModel(MODEL_PATH);

    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();

    CreateSyncObjects();
}

MEPipeline::~MEPipeline()
{
    CleanupSwapChain();

    delete texture;
    delete newImage;
    delete model;

    vkDestroyDescriptorSetLayout(device.GetDevice(),descSetLayout,nullptr);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(device.GetDevice(),renderFinishedSemaphores[i],nullptr);
        vkDestroySemaphore(device.GetDevice(),imageAvailableSemaphores[i],nullptr);
        vkDestroyFence(device.GetDevice(),inFlightFences[i],nullptr);
    }
    
    //vkDestroyCommandPool(device.GetDevice(),commandPool,nullptr);
    
}


std::vector<char> MEPipeline::ReadFile(const std::string & path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if(!file.is_open())
    {
        throw std::runtime_error("failed to open file : " + path);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void MEPipeline::RecordCommandBuffer(int imageIndex)
{
    auto cb = BindPipeline(imageIndex);

    model->BindModel(cb);

    vkCmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&descriptorSets[0],0,nullptr);

    
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float,std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    auto extend = swapchain.GetSwapchainExtent();
    ubo.model = glm::translate(glm::mat4(1.0f),glm::vec3(0.f)) * glm::rotate(glm::mat4(1.0f),time * glm::radians(90.0f),glm::vec3(.0f,.0f,1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.f,0.f,0.f),glm::vec3(0.f,0.f,1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),extend.width / (float)extend.height,0.1f,10.f);
    ubo.proj[1][1] *= -1;


    vkCmdPushConstants(cb,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(UniformBufferObject),&ubo);
    vkCmdDrawIndexed(cb, model->GetIndicesSize(),1,0,0,0);

    UniformBufferObject ubo2{};

    ubo2.model = glm::translate(glm::mat4(1.0f),glm::vec3(2.f,0.f,0.f)) * glm::rotate(glm::mat4(1.0f),time * glm::radians(45.0f),glm::vec3(.0f,.0f,1.0f));
    ubo2.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.f,0.f,0.f),glm::vec3(0.f,0.f,1.0f));
    ubo2.proj = glm::perspective(glm::radians(45.0f),extend.width / (float)extend.height,0.1f,10.f);
    ubo2.proj[1][1] *= -1;


    vkCmdPushConstants(cb,pipelineLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(UniformBufferObject),&ubo2);
    vkCmdDrawIndexed(cb, model->GetIndicesSize(),1,0,0,0);

    // for(int i = 0; i < 2; ++i)
    // {
    //     vkCmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,pipelineLayout,0,1,&descriptorSets[i],0,nullptr);
    //     vkCmdDrawIndexed(cb, model->GetIndicesSize(),1,0,0,0);
    // }
    
    EndPipeline(cb);
}

void MEPipeline::UpdateUniformBuffer(uint32_t currentImage,float plus)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();

    float time = std::chrono::duration<float,std::chrono::seconds::period>(currentTime - startTime).count();

    // UniformBufferObject ubo{};
    // auto extend = swapchain.GetSwapchainExtent();
    // ubo.model = glm::rotate(glm::mat4(1.0f),time * glm::radians(90.0f),glm::vec3(.0f,.0f,1.0f));
    // ubo.view = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.f,0.f,0.f),glm::vec3(0.f,0.f,1.0f));
    // ubo.proj = glm::perspective(glm::radians(45.0f),extend.width / (float)extend.height,0.1f,10.f);
    // ubo.proj[1][1] *= -1;

    VkSampler sampler = newImage->GetSampler();

    void* data;
    vkMapMemory(device.GetDevice(), uniformBuffersMemory[currentImage],0,sizeof(sampler),0,&data);
    memcpy(data,&sampler,sizeof(sampler));
    vkUnmapMemory(device.GetDevice(), uniformBuffersMemory[currentImage]);
}

void MEPipeline::RecreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window.GetWindowPtr(),&width,&height);
    while(width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window.GetWindowPtr(),&width,&height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device.GetDevice());

    CleanupSwapChain();
    swapchain.CleanupSwapChain();
    swapchain.RecreateSwapChain();

    CraeteCommandBuffers();


    CreateGraphicsPipeline(vertPath,fragPath);

    CreateUniformBuffers();
    CreateDescriptorPool();
    CreateDescriptorSets();


    imagesInFlight.resize(swapchain.GetSwapchainImagesSize(),VK_NULL_HANDLE);
}

void MEPipeline::CleanupSwapChain()
{
    commandBuffer->DestroyCommandBuffers();

    vkDestroyPipeline(device.GetDevice(), graphicsPipeline,nullptr);
    vkDestroyPipelineLayout(device.GetDevice(),pipelineLayout,nullptr);

    for(size_t i = 0; i < swapchain.GetSwapchainImagesSize(); ++i)
    {
        vkDestroyBuffer(device.GetDevice(),uniformBuffers[i],nullptr);
        vkFreeMemory(device.GetDevice(),uniformBuffersMemory[i],nullptr);
    }

    descriptorPool->DestroyDescriptorPool();
    //vkDestroyDescriptorPool(device.GetDevice(),descriptorPool, nullptr);
}

uint32_t MEPipeline::BeginRender()
{
    vkWaitForFences(device.GetDevice(),1,&inFlightFences[currentFrame],VK_TRUE,UINT64_MAX);
    //vkResetFences(device.GetDevice(),1,&inFlightFences[currentFrame]);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(device.GetDevice(), swapchain.GetSwapchain(),UINT64_MAX,imageAvailableSemaphores[currentFrame],VK_NULL_HANDLE, &imageIndex);

    if(result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return imageIndex;
    }
    else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("failed to acquire swap chain image");
    }

    if(imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device.GetDevice(),1,&imagesInFlight[imageIndex],VK_TRUE,UINT64_MAX);
    }

    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    return imageIndex;
}

void MEPipeline::EndRender(uint32_t imageIndex)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer->GetCommandBuffer(imageIndex);

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device.GetDevice(),1,&inFlightFences[currentFrame]);

    if(vkQueueSubmit(device.GetGraphicsQueue(),1,&submitInfo,inFlightFences[currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain.GetSwapchain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    auto result = vkQueuePresentKHR(device.GetPresentQueue(),&presentInfo);

    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.framebufferResized)
    {
        window.framebufferResized = false;
        RecreateSwapChain();
    }
    else if(result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkCommandBuffer& MEPipeline::BindPipeline(int imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    auto cb = commandBuffer->GetCommandBuffer(imageIndex);

    if(vkBeginCommandBuffer(cb,&beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer");
    }

    swapchain.BindSwapchain(cb,imageIndex);
    vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,graphicsPipeline);

    return commandBuffer->GetCommandBuffer(imageIndex);
}

void MEPipeline::EndPipeline(VkCommandBuffer& commandBuffer)
{
    swapchain.EndSwapchain(commandBuffer);

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer");
    }
}

void MEPipeline::CreateDescriptorSets()
{
    // std::vector<VkDescriptorSetLayout> layouts(2,descSetLayout);
    // VkDescriptorSetAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // allocInfo.descriptorPool = descriptorPool;
    // allocInfo.descriptorSetCount = static_cast<uint32_t>(2);
    // allocInfo.pSetLayouts = layouts.data();

    // descriptorSets.resize(2);
    // if(vkAllocateDescriptorSets(device.GetDevice(),&allocInfo,descriptorSets.data()) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("failed to allocate descriptor sets");
    // }

    descriptorSets.resize(1);
    descriptorPool->AllocateDescriptorSet(descSetLayout,descriptorSets.data());

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[0];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = texture->GetImageView();
    imageInfo.sampler = texture->GetSampler();

    std::array<VkWriteDescriptorSet,1> descriptorWrites{};

    // descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    // descriptorWrites[0].dstSet = descriptorSets[i];
    // descriptorWrites[0].dstBinding = 0;
    // descriptorWrites[0].dstArrayElement = 0;
    // descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // descriptorWrites[0].descriptorCount = 1;
    // descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = descriptorSets[0];
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pImageInfo = &imageInfo;


    vkUpdateDescriptorSets(device.GetDevice(),static_cast<uint32_t>(descriptorWrites.size()),descriptorWrites.data(),0,nullptr);
}

void MEPipeline::CreateDescriptorPool()
{
    // VkDescriptorPoolSize poolSize{};
    // poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // poolSize.descriptorCount = static_cast<uint32_t>(device.GetSwapChainImages().size());

    // std::array<VkDescriptorPoolSize,1> poolSizes{};
    // // poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // // poolSizes[0].descriptorCount = static_cast<uint32_t>(2);
    // poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // poolSizes[0].descriptorCount = static_cast<uint32_t>(2);
    
    // VkDescriptorPoolCreateInfo poolInfo{};
    // poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    // poolInfo.pPoolSizes = poolSizes.data();
    // poolInfo.maxSets = static_cast<uint32_t>(2);

    // if(vkCreateDescriptorPool(device.GetDevice(),&poolInfo,nullptr,&descriptorPool->GetDescriptorPool()) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("failed to create descriptor pool");
    // }

    descriptorPool = MEDescriptorPool::Builder(device)
        .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,static_cast<uint32_t>(2))
        .Build();
}

void MEPipeline::CreateDescriptorSetLayout()
{
    // VkDescriptorSetLayoutBinding uboLayoutBinding{};
    // uboLayoutBinding.binding = 0;
    // uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // uboLayoutBinding.descriptorCount = 1;
    // uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding,1> bindings = { samplerLayoutBinding};//{uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(device.GetDevice(),&layoutInfo, nullptr,&descSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout");
    }

    // VkPipelineLayoutCreateInfo pipelineCreateInfo{};
    // pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // pipelineCreateInfo.setLayoutCount = 1;
    // pipelineCreateInfo.pSetLayouts = &descSetLayout;

}

void MEPipeline::CreateUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapchain.GetSwapchainImagesSize());
    uniformBuffersMemory.resize(swapchain.GetSwapchainImagesSize());

    for(size_t i = 0; i < swapchain.GetSwapchainImagesSize(); ++i)
    {
        device.CreateBuffer(bufferSize,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],uniformBuffersMemory[i]);

    }
}

void MEPipeline::CreateSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapchain.GetSwapchainImagesSize(),VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if(vkCreateSemaphore(device.GetDevice(),&semaphoreInfo,nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device.GetDevice(),&semaphoreInfo,nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device.GetDevice(),&fenceInfo,nullptr,&inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create sync objects for frame");
        }
    }

    
}

void MEPipeline::CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath)
{
    this->vertPath = vertPath;
    this->fragPath = fragPath;

    auto vert = ReadFile(vertPath);
    auto frag = ReadFile(fragPath);

    auto vertShaderModule = CreateShaderModule(vert);
    auto fragShaderModule = CreateShaderModule(frag);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDesc = Vertex::GetBindingDescription();
    auto attributeDesc = Vertex::GetAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDesc.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain.GetSwapchainExtent().width;
    viewport.height = (float)swapchain.GetSwapchainExtent().height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain.GetSwapchainExtent();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_TRUE;
    multisampling.minSampleShading = 0.2f;
    multisampling.rasterizationSamples = device.msaaSamples;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(UniformBufferObject);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descSetLayout;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    if (vkCreatePipelineLayout(device.GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = swapchain.GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    if(vkCreateGraphicsPipelines(device.GetDevice(),VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    vkDestroyShaderModule(device.GetDevice(), vertShaderModule,nullptr);
    vkDestroyShaderModule(device.GetDevice(), fragShaderModule,nullptr);
}

VkShaderModule MEPipeline::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device.GetDevice(),&createInfo,nullptr,&shaderModule))
    {
        throw std::runtime_error("failed to create shader module");
    }

    return shaderModule;
}

void MEPipeline::CraeteCommandBuffers()
{
    commandBuffer->CreateCommandBuffers(swapchain.GetSwapchainFrameBufferSize());
}

}