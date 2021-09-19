#define _DEBUG

#include <App/MEApp.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int, char**) 
{
    MatchEngine::MEApp app{};

    try
    {
        app.Run();
    }
    catch(const std::exception &e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
