cmake_minimum_required(VERSION 4.2)
project(VulkanTutorial VERSION 1.0)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/deps.cmake)
include(cmake/utils.cmake)


# ---------------
# Assets
# ---------------

set(MODELS_DIR ${CMAKE_BINARY_DIR}/models)
set(TEXTURES_DIR ${CMAKE_BINARY_DIR}/textures)

# Models
file(COPY ${CMAKE_SOURCE_DIR}/assets/viking_room.obj DESTINATION ${MODELS_DIR})

# Textures
file(COPY ${CMAKE_SOURCE_DIR}/assets/viking_room.png DESTINATION ${TEXTURES_DIR})


# ---------------
# Executable
# ---------------

set(EXEC_TARGET VulkanTutorial)

add_executable(${EXEC_TARGET} main.cpp)
set_target_properties(${EXEC_TARGET} PROPERTIES CXX_STANDARD 20)
target_link_libraries(${EXEC_TARGET} PRIVATE Vulkan::Vulkan glfw)
target_include_directories(${EXEC_TARGET} PRIVATE ${stb_INCLUDE_DIRS})

# Define VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS to treat VK_ERROR_OUT_OF_DATE_KHR as a success code
target_compile_definitions(${EXEC_TARGET} PRIVATE "VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS")


# ---------------
# Shaders
# ---------------

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

set(SHADER_TARGET ${EXEC_TARGET}_shader)
file(GLOB SHADER_SOURCES ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.comp)
message(STATUS "Shader sources: ${SHADER_SOURCES}")
if (SHADER_SOURCES)
    add_shaders_target(${SHADER_TARGET} SOURCES ${SHADER_SOURCES})
    add_dependencies(${EXEC_TARGET} ${SHADER_TARGET})
endif ()

set(SHADER_SLANG_TARGET ${EXEC_TARGET}_slang_shader)
file(GLOB SHADER_SLANG_SOURCES ${SHADER_DIR}/*.slang)
message(STATUS "Slang shader sources: ${SHADER_SLANG_SOURCES}")
if (SHADER_SLANG_SOURCES)
    add_slang_shader_target(${SHADER_SLANG_TARGET} SOURCES ${SHADER_SLANG_SOURCES})
    add_dependencies(${EXEC_TARGET} ${SHADER_SLANG_TARGET})
endif ()