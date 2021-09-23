#pragma once

#include <Window/MEWindow.hpp>
#include <Device/MEDevice.hpp>
#include <Pipeline/MEPipeline.hpp>

namespace MatchEngine
{

class MEApp
{
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    void Run();
private:
    MEWindow * meWindow;
    MEDevice * meDevice;
    //MEPipeline mePipeline{"../Shaders/Simple.vert.spv","../Shaders/Simple.frag.spv"};
};

}