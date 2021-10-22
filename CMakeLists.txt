cmake_minimum_required(VERSION 3.0.0)
project(MatchEngine VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

file(GLOB PROJECT_MAIN "*.hpp" "*.cpp" )
file(GLOB PROJECT_SOURCE "source/*.*"
                         "source/*/*.*"
                         "source/*/*/*.*")

link_directories(${PROJECT_NAME} "external/glfw/lib-mingw-w64")
link_directories(${PROJECT_NAME} "C:/VulkanSDK/1.2.162.0/Lib")

#include_directories(${PROJECT_NAME} "source/")
include_directories(${PROJECT_NAME} "external/glfw/include/")
include_directories(${PROJECT_NAME} "external/glm/")
include_directories(${PROJECT_NAME} "external/stb-master/")
include_directories(${PROJECT_NAME} "external/tinyobjloader/")
include_directories(${PROJECT_NAME} "C:/VulkanSDK/1.2.162.0/Include/")

add_executable(MatchEngine ${PROJECT_SOURCE} ${PROJECT_MAIN})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(${PROJECT_NAME} "glfw3")
target_link_libraries(${PROJECT_NAME} "vulkan-1")