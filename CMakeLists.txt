cmake_minimum_required (VERSION 3.8)
if (WIN32)
list(APPEND CMAKE_PREFIX_PATH "C:/usr")
endif(WIN32)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()
set(PROJECT_NAME "AstraEngine")
project (${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 20)

set(VULKAN_SDK C:/VulkanSDK/1.3.268.0)
set(GLFW_DIR C:/usr/glfw-3.3.8.bin.WIN64)

file (GLOB SOURCE_FILES src/*.cpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

add_executable (${PROJECT_NAME} ${SOURCE_FILES})

if (WIN32)
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMAND compile_shaders.bat
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
		VERBATIM
	)
endif(WIN32)

if (UNIX)
	add_custom_command(
		TARGET ${PROJECT_NAME}
		COMMAND compile_shaders.sh
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shaders
		VERBATIM
	)
endif(UNIX)

include_directories(include/ ${VULKAN_SDK}/include ${GLFW_DIR}/include )
link_directories(${VULKAN_SDK}/Lib ${GLFW_DIR}/lib-vc2022)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan glfw)
 
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY .)