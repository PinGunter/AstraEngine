cmake_minimum_required(VERSION 3.19)

project(vkTutorial
    LANGUAGES CXX
)

find_package(Vulkan REQUIRED)

file( GLOB SOURCE_FILES src/*.cpp)
add_executable( ${PROJECT_NAME} ${SOURCE_FILES} )

target_link_libraries(vkTutorial PRIVATE Vulkan::Vulkan glfw)
