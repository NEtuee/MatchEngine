#include "../public/MEWindow.hpp"

namespace MatchEngine
{

MEWindow::MEWindow(int w, int h, std::string name)
        :width{w}, height{h}, windowName{name}
{
    InitWindow();
}

MEWindow::~MEWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void MEWindow::InitWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width,height,windowName.c_str(),nullptr,nullptr);
    glfwSetWindowUserPointer(window,this);
    glfwSetFramebufferSizeCallback(window,FramebufferResizeCallback);

}

}