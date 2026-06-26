# ---------------
# Utils
# ---------------

find_program(GLSLANG_VALIDATOR "glslangValidator" HINTS $ENV{VULKAN_SDK}/bin REQUIRED)
find_program(SLANGC_EXECUTABLE slangc HINTS $ENV{VULKAN_SDK}/bin REQUIRED)

add_executable(glslang::validator IMPORTED)
set_property(TARGET glslang::validator PROPERTY IMPORTED_LOCATION "${GLSLANG_VALIDATOR}")

function(add_shaders_target TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders)

    # Ensure the shaders directory exists
    file(MAKE_DIRECTORY ${SHADERS_DIR})

    add_custom_command(
            OUTPUT ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv
            COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders..."
            COMMAND glslangValidator --target-env vulkan1.0 ${SHADER_SOURCES} -o ${SHADERS_DIR}/shader.spv --quiet
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/shaders
            DEPENDS ${SHADER_SOURCES}
            COMMENT "Compiling GLSL Shaders to SPIR-V"
            VERBATIM
    )
    add_custom_target(${TARGET} ALL DEPENDS ${SHADERS_DIR}/frag.spv ${SHADERS_DIR}/vert.spv)
endfunction()

function(add_slang_shader_target SHADER_TARGET)
    cmake_parse_arguments("SHADER" "" "" "SOURCES" ${ARGN})
    set(SHADERS_DIR ${CMAKE_BINARY_DIR}/shaders)

    # Ensure the shaders directory exists
    file(MAKE_DIRECTORY ${SHADERS_DIR})

    # Check for compute shader
    file(GLOB HAS_COMPUTE ${SHADER_SOURCES})
    set(ENTRY_POINTS -entry vertMain -entry fragMain)

    add_custom_command(
            OUTPUT ${SHADERS_DIR}/slang.spv
            COMMAND ${SLANGC_EXECUTABLE} ${SHADER_SOURCES} -target spirv -profile spirv_1_4+spvRayQueryKHR -emit-spirv-directly -fvk-use-entrypoint-name ${ENTRY_POINTS} -o slang.spv
            WORKING_DIRECTORY ${SHADERS_DIR}
            DEPENDS ${SHADER_SOURCES}
            COMMENT "Compiling Slang Shaders to SPIR-V"
            VERBATIM
    )
    add_custom_target(${SHADER_TARGET} ALL DEPENDS ${SHADERS_DIR}/slang.spv)
endfunction()