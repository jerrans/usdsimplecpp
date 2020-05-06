cmake_minimum_required(VERSION 3.12.0 FATAL_ERROR)

set(MODULE_NAME "usdSimpleCpp")
project(MODULE_NAME)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory(submodules/glm)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(submodules/glfw)

set(MODULE_SOURCES
    camera.cpp
    renderer.cpp
    source.cpp
)

add_executable(${MODULE_NAME} ${MODULE_SOURCES})

add_compile_definitions("GLM_FORCE_SWIZZLE")

include("${CMAKE_CURRENT_BINARY_DIR}/pxrConfig.cmake")

target_include_directories(${MODULE_NAME} PUBLIC
    ${GLFW_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/submodules/glm
    ${PXR_INCLUDE_DIRS}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    ${PXR_LIBRARIES}
    glfw
)
