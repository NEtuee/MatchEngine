#pragma once

#include <string>
#include <vector>
#include <Device/MEDevice.hpp>

namespace MatchEngine
{

class MEPipeline
{
public:
    MEPipeline(MEDevice& device, const std::string& vertPath, const std::string& fragPath);
    ~MEPipeline();
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    MEDevice& device;
};

}