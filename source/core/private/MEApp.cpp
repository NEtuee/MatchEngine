#include "../public/MEApp.hpp"

namespace MatchEngine
{

void MEApp::Run()
{
    while(!meWindow.ShouldClose())
    {
        glfwPollEvents();
        //mePipeline.DrawFrame();

        DrawFrame();
    }

    vkDeviceWaitIdle(meDevice.GetDevice());
}

void MEApp::DrawFrame()
{
    auto imageIndex = mePipeline.BeginRender();

    mePipeline.UpdateUniformBuffer(imageIndex,0.3f);

    mePipeline.RecordCommandBuffer(imageIndex);

    mePipeline.EndRender(imageIndex);
}

}