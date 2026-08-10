cmake_minimum_required(VERSION 3.18)
project(OrbSim3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# External dependencies (install with Homebrew: eigen, glm, glfw).
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED IMPORTED_TARGET glfw3)

# GLAD (vendored source).
add_library(glad STATIC third_party/glad/src/glad.c)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC
  ${CMAKE_SOURCE_DIR}/third_party/glad/include
)

# Dear ImGui with its GLFW and OpenGL3 backends.
file(GLOB IMGUI_SRCS
  ${CMAKE_SOURCE_DIR}/third_party/imgui/*.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SRCS})
target_include_directories(imgui PUBLIC
  ${CMAKE_SOURCE_DIR}/third_party/imgui
  ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD=1)
target_link_libraries(imgui PRIVATE glad PkgConfig::GLFW)

# Application.
add_executable(OrbSim3D
  src/main.cpp
  src/PhysicsEngine.cpp
  src/Renderer.cpp
)

target_include_directories(OrbSim3D PRIVATE
  include
  ${EIGEN3_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/stb   # stb_image.h
)

target_compile_definitions(OrbSim3D PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD=1)

target_link_libraries(OrbSim3D PRIVATE
  imgui
  glad
  PkgConfig::GLFW
  glm::glm
  OpenGL::GL
)

# Copy shaders and assets next to the executable so it can find them at runtime.
file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY assets  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
