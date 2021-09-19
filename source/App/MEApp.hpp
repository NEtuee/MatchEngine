#pragma once

#include <Window/MEWindow.hpp>

namespace MatchEngine
{

class MEApp
{
public:
    static constexpr int WIDTH = 1280;
    static constexpr int HEIGHT = 720;

    void Run();
private:
    MEWindow meWindow{WIDTH,HEIGHT,"MyWindow"};
};

}