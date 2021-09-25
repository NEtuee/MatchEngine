#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>
#include <Window/MEWindow.hpp>

namespace MatchEngine
{

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, MEWindow& window, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;

    MEDevice& device;
    MEWindow& window;
};

}