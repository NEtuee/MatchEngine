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

    MEWindow(const MEWindow &) = delete;
    MEWindow &operator=(const MEWindow &) = delete;

    int GetWidth() {return width;}
    int GetHeight() {return height;}
    bool ShouldClose() {return glfwWindowShouldClose(window);}
    GLFWwindow * GetWindowPtr() {return window;}

    bool framebufferResized = false;
private:
    void InitWindow();
    const int width;
    const int height;
    std::string windowName;
    GLFWwindow * window;

    static void FramebufferResizeCallback(GLFWwindow* window,int width, int height)
    {
        if(width == 0 || height == 0)
            return;
        auto app = reinterpret_cast<MEWindow*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
};

}