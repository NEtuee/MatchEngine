#pragma once

#include <string>
#include <vector>

namespace MatchEngine
{

class MEPipeline
{
public:
    MEPipeline(const std::string& vertPath, const std::string& fragPath);
private:
    static std::vector<char> ReadFile(const std::string & path);

    void CreateGraphicsPipeline(const std::string& vertPath, const std::string& fragPath);
};

}