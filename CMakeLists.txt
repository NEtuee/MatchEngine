cmake_minimum_required(VERSION 3.0.0)
project(MatchEngine VERSION 0.1.0)

include(CTest)
enable_testing()

file(GLOB PROJ_SRC "*.h" "*.cpp" )

link_directories(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/lib-mingw-w64")
link_directories(${PROJECT_NAME} "C:/VulkanSDK/1.2.162.0/Lib")

include_directories(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include/")
include_directories(${PROJECT_NAME} "C:/VulkanSDK/1.2.162.0/Include")
include_directories(${PROJECT_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/external/glm/")

add_executable(MatchEngine ${PROJ_SRC})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

target_link_libraries(${PROJECT_NAME} "glfw3")
target_link_libraries(${PROJECT_NAME} "vulkan-1")