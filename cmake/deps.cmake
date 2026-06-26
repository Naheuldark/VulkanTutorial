# ---------------
# Dependencies
# ---------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# glfw
message(STATUS "Fetching glfw...")
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw)

# glm
message(STATUS "Fetching glm...")
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)

# tinyobjloader
message(STATUS "Fetching tinyobjloader...")
FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG origin/release
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tinyobjloader)

# stb
message(STATUS "Fetching stb...")
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(stb)

# tinygltf
message(STATUS "Fetching tinygltf...")
FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG origin/release
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tinygltf)

# nlohmann-json
message(STATUS "Fetching nlohmann-json...")
FetchContent_Declare(
        nlohmann-json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(nlohmann-json)

# ktx[vulkan]
message(STATUS "Fetching ktx[vulkan]...")
FetchContent_Declare(
        ktx[vulkan]
        GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
        GIT_TAG v4.4.2
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(ktx[vulkan])

# openal
message(STATUS "Fetching openal...")
FetchContent_Declare(
        openal
        GIT_REPOSITORY https://github.com/kcat/openal-soft.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(openal)

# vulkan
find_package(Vulkan)