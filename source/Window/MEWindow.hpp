#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace MatchEngine
{

class MEWindow
{
public:
    MEWindow(int w, int h, std::string name);
    ~MEWindow();

    bool ShouldClose() {return glfwWindowShouldClose(window);}
private:
    void InitWindow();
    const int width;
    const int height;
    std::string windowName;
    GLFWwindow * window;
};

}