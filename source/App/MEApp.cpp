#include "MEApp.hpp"

namespace MatchEngine
{

void MEApp::Run()
{
    while(!meWindow.ShouldClose())
    {
        glfwPollEvents();
    }
}

}