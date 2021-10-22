#pragma once

#include "MEWindow.hpp"
#include "MEDevice.hpp"
#include "MEPipeline.hpp"
#include "MESwapchain.hpp"

namespace MatchEngine
{

class MEApp
{
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    void Run();
    void DrawFrame();
private:
    MEWindow meWindow{WIDTH,HEIGHT,"MyWindow"};
    MEDevice meDevice{meWindow};
    MESwapchain meSwapchain{meDevice,meWindow};
    MEPipeline mePipeline{meDevice,meWindow,meSwapchain,"../Shaders/Simple.vert.spv","../Shaders/Simple.frag.spv"};
};

}