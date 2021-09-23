#include "MEApp.hpp"

namespace MatchEngine
{

void MEApp::Run()
{
    meWindow = new MEWindow(WIDTH,HEIGHT,"MyWindow");
    meDevice = new MEDevice(meWindow->GetWindowPtr());
    while(!meWindow->ShouldClose())
    {
        glfwPollEvents();
    }
}

}