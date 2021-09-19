#include "MEPipeline.hpp"

#include <fstream>
#include <stdexcept>
#include <iostream>

namespace MatchEngine
{

MEPipeline::MEPipeline(const std::string& vertPath, const std::string& fragPath)
{
    CreateGraphicsPipeline(vertPath,fragPath);
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

void MEPipeline::CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath)
{
    auto vert = ReadFile(vertPath);
    auto frag = ReadFile(fragPath);

    std::cout << "vert code size : " << vert.size() << std::endl;
    std::cout << "frag code size : " << frag.size() << std::endl;
}

}