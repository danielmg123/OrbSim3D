cmake_minimum_required(VERSION 3.18)
project(OrbSim3D LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Eigen and GLM
find_package(Eigen3 REQUIRED)
find_package(glm REQUIRED)

# Use pkg-config to find GLFW
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# Include GLFW paths
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})

# Add GLAD (manually integrated)
add_library(glad STATIC third_party/glad/src/glad.c)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC third_party/glad/include)

# Executable and source files
add_executable(OrbSim3D
  src/main.cpp
  src/Body.cpp
  src/PhysicsEngine.cpp
  src/Renderer.cpp
  src/Camera.cpp
)

# Include headers and Eigen
target_include_directories(OrbSim3D PRIVATE
  include
  ${EIGEN3_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(OrbSim3D PRIVATE
  glad
  ${GLFW_LIBRARIES}
  glm::glm
)

# Copy shaders directory to build output
file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})