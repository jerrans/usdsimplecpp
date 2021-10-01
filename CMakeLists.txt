cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

set(MODULE_NAME "usdSimpleCpp")
project(MODULE_NAME)

set(CMAKE_CXX_STANDARD 17)

if( NOT EXISTS ${USD_ROOT} )
    message(FATAL_ERROR "USD_ROOT not specified")
endif()

add_subdirectory(submodules/glm)
add_subdirectory(submodules/glew)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(submodules/glfw)

set(MODULE_SOURCES
    camera.cpp
    camera.h
    renderer.cpp
    renderer.h
    shader.cpp
    shader.h
    source.cpp
)

add_executable(${MODULE_NAME} ${MODULE_SOURCES})

add_compile_definitions("GLM_FORCE_SWIZZLE")
add_compile_definitions("NOMINMAX")

include("${USD_ROOT}/pxrConfig.cmake")

target_include_directories(${MODULE_NAME} PUBLIC
    ${GLFW_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/submodules/glm
    ${CMAKE_SOURCE_DIR}/submodules/glew/include
    ${PXR_INCLUDE_DIRS}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    ${PXR_LIBRARIES}
    glfw
    ${CMAKE_CURRENT_BINARY_DIR}/submodules/glew/lib/Release/glew-shared.lib
)
